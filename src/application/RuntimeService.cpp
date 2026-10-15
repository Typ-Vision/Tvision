#include "RuntimeService.hpp"
#include "EditorSession.hpp"
#include <QDir>
#include <QFileInfo>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif
namespace visionflow {
RuntimeIdentity RuntimeIdentity::localEngineer() {
    QString principal;
#ifdef Q_OS_WIN
    wchar_t name[257];DWORD size=257;if(GetUserNameW(name,&size))principal="windows:"+QString::fromWCharArray(name);
#else
    if(const auto user=getpwuid(geteuid()))principal="uid:"+QString::number(geteuid())+":"+QString::fromLocal8Bit(user->pw_name);
#endif
    // Single-user desktop policy: authenticated local OS session is the engineer.
    // It grants no Users/Administrator/system-management permissions.
    return {principal,[principal](const QString& actor,ProductionAction){return !principal.isEmpty() && actor==principal;}};
}
RuntimeService::RuntimeService(QString directory,QString catalogPath,RuntimeIdentity identity,SystemProvider system)
    :store_(std::move(directory)),catalogPath_(std::move(catalogPath)),identity_(std::move(identity)),system_(std::move(system)),
     production_(identity_.authorize,[this](const Recipe& r){return dependencies(r);},16,
        [this](const QString& kind,const QString& id,const QJsonObject& o){store_.append(kind,id,o);},[this](const ImageFrame& frame){return completionCalibration(frame);}) {}
Recipe RuntimeService::systemRecipe(const Recipe& r,const QString& directory) const {
    if(!system_)return r;auto raw=[&]{auto copy=r;copy.calibrationBinding.reset();copy.calibrationReference.clear();return copy;};
    try{const auto s=system_();if(!s)return r;if(!SystemCalibration::requestsCalibration(r))return raw();return SystemCalibration::prepareRecipe(*s,r,directory);}
    catch(...){if(SystemCalibration::requestsCalibration(r,false))throw;return raw();}
}
std::optional<ProductionDependencies> RuntimeService::systemDependencies(const Recipe& r) const {
    if(!system_)return {};std::optional<ActiveCalibrationSnapshot> s;
    try{s=system_();if(!s)return {};if(!SystemCalibration::requestsCalibration(r))return ProductionDependencies{};SystemCalibration::prepareRecipe(*s,r,QFileInfo(catalogPath_).absolutePath());}
    catch(...){if(SystemCalibration::requestsCalibration(r,false))throw;return ProductionDependencies{};}
    const auto source=SystemCalibration::source(r);
    const auto coordinates=r.calibrationBinding ? r.calibrationBinding->source.id:source.id+"/image";
    ProductionDependencies result;result.calibration=SystemCalibration::binding(*s,source.id,coordinates);result.systemCalibration=SystemCalibration::evidence(*s,source.id,coordinates);return result;
}
MeasurementExecutionContext RuntimeService::completionCalibration(const ImageFrame& frame) const {
    return calibration(*production_.active(),frame,catalogPath_);
}
MeasurementExecutionContext RuntimeService::calibration(const ProductionSnapshot& s,const ImageFrame& frame,const QString& path){if(s.dependencies().contains("systemCalibration"))return SystemCalibration::resolve(SystemCalibration::decodeEvidence(s.dependencies()["systemCalibration"].toObject()),s.recipe(),s.projectDirectory(),frame);return CalibrationWorkflow::resolve(s.recipe(),frame,path);}
ProductionDependencies RuntimeService::dependencies(const Recipe& r) const {
    ProductionDependencies result;
    for(const auto& t:editorTools(r.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")) {
        const QJsonObject declared{{"kind",t.kind},{"parameters",t.parameters},{"enabled",t.enabled},{"planarSetup",r.definition.value("planarSetup")}};
        if(declaredInstallations_.value(t.id)!=declared)throw std::runtime_error("Kaynak veya beyan edilen fiziksel kurulum değişti; yeniden doğrulayın.");
    }
    if(const auto result=systemDependencies(r))return *result;
    if(declaredPreparation_)return declaredDependencies(r);
    if(r.calibrationBinding || !r.calibrationReference.isEmpty()) {
        QString source;for(const auto& t:editorTools(r.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam"))source=t.id;
        const auto found=installations_.constFind(source);
        if(found==installations_.cend())throw std::runtime_error("Runtime validation requires fresh source acquisition.");
        const auto setup=CalibrationWorkflow::installation(r,found.value());
        result.calibration=CalibrationService(catalogPath_,[setup](const QString&){return setup;}).resolve(r).dependency();
    }return result;
}
ProductionDependencies RuntimeService::declaredDependencies(const Recipe& r) const {
    if(const auto result=systemDependencies(r))return *result;
    ProductionDependencies result;
    if(r.calibrationBinding || !r.calibrationReference.isEmpty()) {
        if(!r.calibrationBinding)throw std::runtime_error("Exact calibration binding required.");
        const auto& binding=*r.calibrationBinding;
        const auto setup=r.definition.value("planarSetup").toObject();
        if(setup.size()!=2 || !setup["lensId"].isString() || !setup["workingPlaneId"].isString() || setup["lensId"].toString().trimmed().isEmpty() || setup["workingPlaneId"].toString().trimmed().isEmpty())
            throw std::runtime_error("Lens ve çalışma düzlemini Calibration ekranında uygulayın.");
        QString source;for(const auto& t:editorTools(r.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam"))source=t.id;
        // Resolution/coordinates here are requirements from the binding, not a
        // fabricated observation. execute() independently checks each actual frame.
        const CalibrationInstallation declared{source,setup["lensId"].toString(),setup["workingPlaneId"].toString(),binding.resolution,binding.source,binding.target};
        result.calibration=CalibrationService(catalogPath_,[declared](const QString&){return declared;}).resolve(r).dependency();
    }
    return result;
}
void RuntimeService::checkConfiguration(const Recipe& r,const QString& directory) const {
    ProductionRecipeService checker(identity_.authorize,[this](const Recipe& recipe){return declaredDependencies(recipe);});
    checker.validate(identity_.principal,systemRecipe(r,directory),directory);
    for(const auto& tool:editorTools(r.definition))if(tool.enabled && tool.kind=="image_file") {
        const QFileInfo file(QDir(directory).absoluteFilePath(tool.parameters["path"].toString()));
        if(!file.isFile() || !file.isReadable())throw ProductionException(ProductionError::InvalidConfiguration,tool.name+": Kaynak görüntü dosyası bulunamadı veya okunamıyor.");
    }
}
void RuntimeService::prepare(const Recipe& r,const QString& directory) {
    if(production_.busy())throw ProductionException(ProductionError::Busy,"Runtime cycle/recording pending.");
    if(identity_.principal.isEmpty() || !identity_.authorize || !identity_.authorize(identity_.principal,ProductionAction::Run))throw ProductionException(ProductionError::Unauthorized,"Runtime Run denied.");
    checkConfiguration(r,directory);
    const auto previousDeclarations=declaredInstallations_;const auto previousValidation=validation_;const bool previousMode=declaredPreparation_;
    try {
        observeInstallation(r);declaredPreparation_=true;
        const auto effective=systemRecipe(r,directory);const auto validation=production_.validate(identity_.principal,effective,directory);
        production_.activate(identity_.principal,validation.id,effective,directory);validation_=validation;
    }catch(...){declaredInstallations_=previousDeclarations;validation_=previousValidation;declaredPreparation_=previousMode;throw;}
}
RecipeValidation RuntimeService::validate(const Recipe& r,const QString& directory,const ImageFrame& frame) {
    if(production_.busy())throw ProductionException(ProductionError::Busy,"Runtime cycle/recording pending.");
    // Permission is checked before accepting installation data or catalog work.
    if(identity_.principal.isEmpty() || !identity_.authorize || !identity_.authorize(identity_.principal,ProductionAction::Validate))throw ProductionException(ProductionError::Unauthorized,"Runtime validation denied.");
    const auto& m=frame.metadata();int sources=0;bool matches=false;
    for(const auto& t:editorTools(r.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){++sources;matches=t.id==m.sourceId;}
    if(sources!=1 || !matches || m.frameId.isEmpty() || m.cycleId.isEmpty() || !m.completedAtUtc.isValid())throw ProductionException(ProductionError::InvalidConfiguration,"Fresh acquisition provenance required.");
    observeInstallation(r);installations_.insert(m.sourceId,frame);declaredPreparation_=false;
    validation_.reset();const auto effective=systemRecipe(r,directory);const auto v=production_.validate(identity_.principal,effective,directory);validation_=v;return v;
}
void RuntimeService::observeInstallation(const Recipe& r) {
    for(const auto& t:editorTools(r.definition))if(t.kind=="image_file" || t.kind=="webcam")declaredInstallations_[t.id]=QJsonObject{{"kind",t.kind},{"parameters",t.parameters},{"enabled",t.enabled},{"planarSetup",r.definition.value("planarSetup")}};
}
void RuntimeService::activate(const Recipe& r,const QString& directory) {
    if(!validation_)throw ProductionException(ProductionError::UnknownValidation,"Validate the current draft first.");
    production_.activate(identity_.principal,validation_->id,systemRecipe(r,directory),directory);
}
void RuntimeService::rollback(const QString& id){production_.rollback(identity_.principal,id);}
ProductionCycle RuntimeService::begin(const QJsonObject& communication){return production_.beginCycle(identity_.principal,communication);}
void RuntimeService::finish(quint64 ticket,const CycleResult& r){
    if(r.partial)throw std::runtime_error("Editor partial evaluation cannot be recorded as a Runtime cycle.");production_.finishCycle(ticket,r);}
ImageFrame RuntimeService::probe(const Recipe& recipe,const QString& directory,const std::shared_ptr<std::atomic_bool>& cancel) {
    QString source;int count=0;for(const auto& t:editorTools(recipe.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){source=t.id;++count;}
    if(count!=1)throw std::runtime_error("Tek enabled edinim kaynağı gerekir.");
    const auto result=ExecutionService{}.run(recipe.definition,directory,recipe.id,"validation-probe",cancel,source);
    if(result.state!=ExecutionState::Done)throw std::runtime_error(("Doğrulama karesi edinilemedi: "+result.error).toUtf8().constData());
    for(const auto& t:result.tools)if(t.toolId==source && t.state==ExecutionState::Done && t.outputs.value("image").valid)return std::get<ImageFrame>(t.outputs.value("image").data);
    throw std::runtime_error("Doğrulama karesi yok.");
}
void RuntimeService::suppressCancelled(const ProductionCycle& cycle,CycleResult& result) {
    if(result.completionSnapshot_!=cycle.snapshot || result.completionTicket_!=cycle.ticket ||
       result.completionDigest_.isEmpty() || result.completionDigest_!=ProductionRecipeService::completionDigest(result))
        throw std::runtime_error("Cannot suppress changed or foreign Runtime completion");
    EditorSession::suppressCancelled(result);
    result.completionDigest_=ProductionRecipeService::completionDigest(result);
}
CycleResult RuntimeService::execute(const ProductionCycle& cycle,const QString& catalogPath,const std::shared_ptr<std::atomic_bool>& cancel) {
    if(!cycle.snapshot || !cycle.ticket)throw std::runtime_error("Runtime execution requires a production lease");
    const auto s=cycle.snapshot;const auto recipe=s->recipe();
    auto result=ExecutionService{}.run(recipe.definition,s->projectDirectory(),recipe.id,s->validation().contentId,cancel,{}, {},
        [recipe,catalogPath,s](const ImageFrame& frame){if(s->dependencies().contains("systemCalibration"))return SystemCalibration::resolve(SystemCalibration::decodeEvidence(s->dependencies()["systemCalibration"].toObject()),recipe,s->projectDirectory(),frame);return CalibrationWorkflow::resolve(recipe,frame,catalogPath);});
    result.completionDigest_=ProductionRecipeService::completionDigest(result);
    result.completionTicket_=cycle.ticket;result.completionSnapshot_=s;
    return result;
}
}
