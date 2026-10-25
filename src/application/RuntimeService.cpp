#include "RuntimeService.hpp"
#include "AcquisitionPolicy.hpp"
#include "MultiSourceRuntimeService.hpp"
#include "EditorSession.hpp"
#include "IndustrialExecution.hpp"
#include "ExperimentalRuntime.hpp"
#include "RuntimeCameraSessions.hpp"
#include <QDir>
#include <QFileInfo>
#include <QUuid>
#include <QElapsedTimer>
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
RuntimeService::RuntimeService(QString directory,QString catalogPath,RuntimeIdentity identity,SystemProvider system,ScopedProvider scoped,AcquisitionProvider acquisition,MultiProvider multi)
    :store_(std::move(directory)),catalogPath_(std::move(catalogPath)),identity_(std::move(identity)),system_(std::move(system)),scoped_(std::move(scoped)),acquisition_(std::move(acquisition)),
     production_(identity_.authorize,[this](const Recipe& r){return dependencies(r);},16,
        [this](const QString& kind,const QString& id,const QJsonObject& o){store_.append(kind,id,o);},[this](const ImageFrame& frame){return completionCalibration(frame);}),multiProvider_(std::move(multi)) {}
RuntimeService::~RuntimeService()=default;
MultiSourceDependencies RuntimeService::cameraComposition(const Recipe& r,const MultiProvider& provider) const{
    auto set=provider?provider(r):MultiSourceDependencies{};
    return cameraSessions_?cameraSessions_->compose(std::move(set)):set;
}
void RuntimeService::requestCameraPreparation(const Recipe& recipe,const QString& directory){
    bool industrial=false;for(const auto& t:editorTools(recipe.definition))if(t.enabled && t.kind=="industrial_camera")industrial=true;
    if(!industrial){cameraSessions_.reset();return;}
    if(identity_.principal.isEmpty() || !identity_.authorize || !identity_.authorize(identity_.principal,ProductionAction::Run) ||
       !identity_.authorize(identity_.principal,ProductionAction::Validate) || !identity_.authorize(identity_.principal,ProductionAction::Activate))
        throw ProductionException(ProductionError::Unauthorized,"Runtime camera preparation denied");
    const bool experimental=requiresManualCycles(recipe,directory);const auto& provider=experimental?automaticProvider_:multiProvider_;
    if(!provider)throw ProductionException(ProductionError::InvalidConfiguration,"Runtime source-set provider required");
    if(!cameraSessions_)cameraSessions_=std::make_unique<RuntimeCameraSessions>();
    cameraSessions_->start(recipe,provider(recipe),experimental);
}
bool RuntimeService::pollCameras(){return cameraSessions_ && cameraSessions_->poll();}
void RuntimeService::stopCameras(){if(cameraSessions_)cameraSessions_->stop();}
bool RuntimeService::camerasPending() const{return cameraSessions_ && cameraSessions_->pending();}
bool RuntimeService::camerasDraining() const{return cameraSessions_ && cameraSessions_->draining();}
QString RuntimeService::cameraError() const{return cameraSessions_?cameraSessions_->error():QString{};}
bool RuntimeService::cameraCloseFailed() const{return cameraSessions_ && cameraSessions_->closeFailed();}
bool RuntimeService::busy() const {return production_.busy() || (multi_ && multi_->busy()) || (experimental_ && experimental_->busy());}
void RuntimeService::setAutomaticAcquisition(MultiProvider provider){
    if(busy())throw ProductionException(ProductionError::Busy,"Runtime cycle/recording pending");
    stopCameras();automaticAcquisition_=true;automaticProvider_=std::move(provider);
    experimental_.reset();multi_.reset();multiMode_=false;validation_.reset();policyReset_=true;
}
void RuntimeService::setExperimental(bool enabled,MultiProvider provider){
    if(busy())throw ProductionException(ProductionError::Busy,"Runtime cycle/recording pending");
    stopCameras();automaticAcquisition_=false;
    if(enabled){automaticProvider_=std::move(provider);experimental_=std::make_unique<ExperimentalRuntime>(identity_,[this](const Recipe& r){return cameraComposition(r,automaticProvider_);},[this](const QString& kind,const QString& id,const QJsonObject& o){store_.append(kind,id,o);});}else experimental_.reset();
    // Switching policies must not resurrect a previous prepared production lease.
    multi_.reset();multiMode_=false;validation_.reset();policyReset_=true;
}
std::shared_ptr<const ProductionSnapshot> RuntimeService::active() const {return policyReset_ ? nullptr:experimental_ ? experimental_->active():multiMode_ ? multi_->active():production_.active();}
void RuntimeService::requireReady() const {if(policyReset_)throw ProductionException(ProductionError::UnknownValidation,"Run required after Runtime policy change");if(experimental_)experimental_->requireReady();else if(multiMode_)multi_->requireReady();else production_.requireReady(identity_.principal);}
std::optional<ActiveCalibrationSnapshot> RuntimeService::systemSnapshot(const Recipe& r) const {
    if(r.definition.contains("sourceSetup") || r.definition.contains("calibrationPolicy")){if(!scoped_)throw std::runtime_error("Scoped Runtime provider required");auto s=scoped_(r);if(!s || s->scope.isEmpty())throw std::runtime_error("Required setup is not active");return s;}
    return system_ ? system_():std::optional<ActiveCalibrationSnapshot>{};
}
QJsonObject RuntimeService::sourceDependencies(const Recipe& r) const {
    if(!r.definition.contains("sourceSetup"))return {};if(!acquisition_)throw std::runtime_error("Scoped acquisition provider required");const auto acquisition=acquisition_(r);const auto ref=SystemCalibration::setupReference(r);
    if(!ref || acquisition["target"]!=ref->toJson())throw std::runtime_error("Runtime acquisition snapshot target mismatch");
    return {{"schemaVersion",1},{"binding",r.definition["sourceSetup"]},{"acquisition",acquisition},{"origin",SystemCalibration::source(r).kind=="image_file" ? "offline_replay":"camera_acquired"}};
}
ProductionDependencies RuntimeService::dependencies(const Recipe& r) const {auto result=calibrationDependencies(r);result.measurementSource=sourceDependencies(r);return result;}
ProductionDependencies RuntimeService::declaredDependencies(const Recipe& r) const {auto result=declaredCalibrationDependencies(r);result.measurementSource=sourceDependencies(r);return result;}
Recipe RuntimeService::systemRecipe(const Recipe& r,const QString& directory) const {
    if(r.definition["editor"].toObject()["schemaVersion"]==9)throw ProductionException(ProductionError::InvalidConfiguration,"Editor9 requires whole source-set Runtime preparation");
    if(!system_ && !r.definition.contains("sourceSetup") && !r.definition.contains("calibrationPolicy"))return r;auto raw=[&]{auto copy=r;copy.calibrationBinding.reset();copy.calibrationReference.clear();return copy;};
    try{if(!SystemCalibration::requestsCalibration(r))return raw();const auto s=systemSnapshot(r);if(!s)return r;return SystemCalibration::prepareRecipe(*s,r,directory);}
    catch(...){if(SystemCalibration::requestsCalibration(r,false))throw;return raw();}
}
std::optional<ProductionDependencies> RuntimeService::systemDependencies(const Recipe& r) const {
    if(!system_ && !r.definition.contains("sourceSetup") && !r.definition.contains("calibrationPolicy"))return {};std::optional<ActiveCalibrationSnapshot> s;
    try{if(!SystemCalibration::requestsCalibration(r))return ProductionDependencies{};s=systemSnapshot(r);if(!s)return {};SystemCalibration::prepareRecipe(*s,r,QFileInfo(catalogPath_).absolutePath());}
    catch(...){if(SystemCalibration::requestsCalibration(r,false))throw;return ProductionDependencies{};}
    const auto source=SystemCalibration::source(r);
    const auto coordinates=r.calibrationBinding ? r.calibrationBinding->source.id:source.id+"/image";
    ProductionDependencies result;result.calibration=SystemCalibration::binding(*s,source.id,coordinates);result.systemCalibration=SystemCalibration::evidence(*s,source.id,coordinates);return result;
}
MeasurementExecutionContext RuntimeService::completionCalibration(const ImageFrame& frame) const {
    return calibration(*production_.active(),frame,catalogPath_);
}
MeasurementExecutionContext RuntimeService::calibration(const ProductionSnapshot& s,const ImageFrame& frame,const QString& path){
    if(s.validation().scope=="experimental_runtime/2")return ExperimentalRuntime::calibration(s,frame);
    if(s.dependencies().contains("sourceSet")){const auto entry=s.dependencies()["sourceSet"].toObject()[frame.metadata().sourceId].toObject();if(entry.isEmpty())throw std::runtime_error("Foreign source in immutable model set");if(entry.contains("systemCalibration"))return SystemCalibration::resolve(SystemCalibration::decodeEvidence(entry["systemCalibration"].toObject()),SystemCalibration::branch(s.recipe(),frame.metadata().sourceId),s.projectDirectory(),frame);MeasurementExecutionContext c;c.rawSize=frame.size();c.error="Raw-only source has no admitted model";return c;}
if(s.dependencies().contains("systemCalibration"))return SystemCalibration::resolve(SystemCalibration::decodeEvidence(s.dependencies()["systemCalibration"].toObject()),s.recipe(),s.projectDirectory(),frame);return CalibrationWorkflow::resolve(s.recipe(),frame,path);}
ProductionDependencies RuntimeService::calibrationDependencies(const Recipe& r) const {
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
ProductionDependencies RuntimeService::declaredCalibrationDependencies(const Recipe& r) const {
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
bool RuntimeService::requiresManualCycles(const Recipe& r,const QString& directory) const {
    if(!automaticAcquisition_)return bool(experimental_);
    try{return AcquisitionPolicy::recipe(r.definition,directory);}
    catch(const std::exception& e){throw ProductionException(ProductionError::InvalidConfiguration,e.what());}
}
void RuntimeService::checkConfiguration(const Recipe& r,const QString& directory) const {
    if(automaticAcquisition_ && requiresManualCycles(r,directory)){ExperimentalRuntime checker(identity_,automaticProvider_,{});checker.prepare(r,directory);return;}
    if(experimental_ && !automaticAcquisition_){validateEditorDefinition(r.definition);return;}
    if(r.definition["editor"].toObject()["schemaVersion"]==9){
        if(!multiProvider_)throw ProductionException(ProductionError::InvalidConfiguration,"Whole source-set provider required");
        const auto tools=editorTools(r.definition);bool industrial=false;for(const auto& t:tools)if(t.enabled && t.kind=="industrial_camera")industrial=true;
        if(industrial){
            // Load is a read-only declaration. Run/prepare and begin retain full
            // immutable admission; offline presentation grants no backend authority.
            validateEditorDefinition(r.definition);const auto graph=validateSourceGraph(tools,editorConnections(r.definition));
            if(!graph.valid() || graph.requiredSources.isEmpty())throw ProductionException(ProductionError::InvalidConfiguration,"Invalid required source graph");
            for(const auto& t:tools)if(t.enabled && t.kind=="industrial_camera")IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject());
            return;
        }
        MultiSourceRuntimeService checker(identity_,multiProvider_);checker.prepare(r,directory);return;
    }
    ProductionRecipeService checker(identity_.authorize,[this](const Recipe& recipe){return declaredDependencies(recipe);});
    checker.validate(identity_.principal,systemRecipe(r,directory),directory);
    for(const auto& tool:editorTools(r.definition))if(tool.enabled && tool.kind=="image_file") {
        const QFileInfo file(QDir(directory).absoluteFilePath(tool.parameters["path"].toString()));
        if(!file.isFile() || !file.isReadable())throw ProductionException(ProductionError::InvalidConfiguration,tool.name+": Kaynak görüntü dosyası bulunamadı veya okunamıyor.");
    }
}
void RuntimeService::prepare(const Recipe& r,const QString& directory) {
    if(busy())throw ProductionException(ProductionError::Busy,"Runtime cycle/recording pending.");
    if(automaticAcquisition_){
        const bool selected=requiresManualCycles(r,directory);
        if(selected){
            auto candidate=std::make_unique<ExperimentalRuntime>(identity_,[this](const Recipe& recipe){return cameraComposition(recipe,automaticProvider_);},[this](const QString& kind,const QString& id,const QJsonObject& o){store_.append(kind,id,o);});
            candidate->prepare(r,directory);experimental_=std::move(candidate);multi_.reset();multiMode_=false;validation_=experimental_->active()->validation();policyReset_=false;return;
        }
        if(experimental_){experimental_.reset();validation_.reset();policyReset_=true;}
    }
    if(experimental_){experimental_->prepare(r,directory);validation_=experimental_->active()->validation();policyReset_=false;return;}
    if(identity_.principal.isEmpty() || !identity_.authorize || !identity_.authorize(identity_.principal,ProductionAction::Run))throw ProductionException(ProductionError::Unauthorized,"Runtime Run denied.");
    if(r.definition["editor"].toObject()["schemaVersion"]==9){
        if(!multiProvider_)throw ProductionException(ProductionError::InvalidConfiguration,"Whole source-set provider required");
        if(!multi_)multi_=std::make_unique<MultiSourceRuntimeService>(identity_,[this](const Recipe& recipe){return cameraComposition(recipe,multiProvider_);},[this](const QString& kind,const QString& id,const QJsonObject& record){store_.append(kind,id,record);});
        multi_->prepare(r,directory);multiMode_=true;validation_=multi_->active()->validation();policyReset_=false;return;
    }
    checkConfiguration(r,directory);
    const auto previousDeclarations=declaredInstallations_;const auto previousValidation=validation_;const bool previousMode=declaredPreparation_;
    try {
        observeInstallation(r);declaredPreparation_=true;
        const auto effective=systemRecipe(r,directory);const auto validation=production_.validate(identity_.principal,effective,directory);
        production_.activate(identity_.principal,validation.id,effective,directory);validation_=validation;multiMode_=false;policyReset_=false;
    }catch(...){declaredInstallations_=previousDeclarations;validation_=previousValidation;declaredPreparation_=previousMode;throw;}
}
RecipeValidation RuntimeService::validate(const Recipe& r,const QString& directory,const ImageFrame& frame) {
    if(experimental_ || !frame.metadata().experimentalAcquisition.isEmpty())throw ProductionException(ProductionError::InvalidConfiguration,"Experimental acquisition cannot enter production validation");
    if(busy())throw ProductionException(ProductionError::Busy,"Runtime cycle/recording pending.");
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
    if(experimental_)throw ProductionException(ProductionError::InvalidConfiguration,"Experimental Runtime uses separate preparation");
    if(busy())throw ProductionException(ProductionError::Busy,"Runtime cycle/recording pending.");
    if(!validation_)throw ProductionException(ProductionError::UnknownValidation,"Validate the current draft first.");
    production_.activate(identity_.principal,validation_->id,systemRecipe(r,directory),directory);multiMode_=false;
}
void RuntimeService::rollback(const QString& id){if(experimental_)throw ProductionException(ProductionError::InvalidConfiguration,"Experimental Runtime cannot activate production history");if(busy())throw ProductionException(ProductionError::Busy,"Cycle pending");production_.rollback(identity_.principal,id);multiMode_=false;}
ProductionCycle RuntimeService::begin(const QJsonObject& communication){
    QElapsedTimer timer;timer.start();
    QJsonObject event{{"action","admission"},{"phase","begin"},{"plcRequest",!communication.isEmpty()}};
    if(const auto s=active())event.insert("recipe",RuntimeDiagnostics::recipeContext(s->recipe()));
    try {
        if(policyReset_)requireReady();
        auto cycle=experimental_ ? experimental_->begin(communication):multiMode_ ? multi_->begin(communication):production_.beginCycle(identity_.principal,communication);
        cycle.diagnostics=diagnostics_;event["ticket"]=QString::number(cycle.ticket);event["elapsedMs"]=timer.elapsed();
        event["outcome"]="accepted";diagnostics_->append(event);return cycle;
    }catch(const ProductionException& e){event["reason"]=QString::fromUtf8(e.what());event["errorCode"]=int(e.code());event["elapsedMs"]=timer.elapsed();diagnostics_->append(event,e.code()!=ProductionError::Busy);throw;}
    catch(const std::exception& e){event["reason"]=QString::fromUtf8(e.what());diagnostics_->append(event,true);throw;}
}
bool RuntimeService::sameCompletion(const CycleResult& a,const CycleResult& b){return ProductionRecipeService::completionDigest(a)==ProductionRecipeService::completionDigest(b);}
void RuntimeService::finish(quint64 ticket,const CycleResult& r){
    QElapsedTimer timer;timer.start();QJsonObject event{{"action","record"},{"ticket",QString::number(ticket)},{"cycleId",r.cycleId},{"recipeId",r.recipeId},{"executionState",executionStateName(r.state)}};
    try {
        if(r.partial)throw ProductionException(ProductionError::InvalidCompletion,"Editor partial evaluation cannot be recorded as a Runtime cycle.");
        if(experimental_)experimental_->finish(ticket,r);else if(multiMode_)multi_->finish(ticket,r);else production_.finishCycle(ticket,r);
        event["outcome"]="committed";event["elapsedMs"]=timer.elapsed();diagnostics_->append(event);
    }catch(const ProductionException& e){event["reason"]=QString::fromUtf8(e.what());event["errorCode"]=int(e.code());event["recordPending"]=e.code()==ProductionError::PersistenceFailure;event["elapsedMs"]=timer.elapsed();diagnostics_->append(event,true);throw;}
    catch(const std::exception& e){event["reason"]=QString::fromUtf8(e.what());diagnostics_->append(event,true);throw;}
}
ImageFrame RuntimeService::probe(const Recipe& recipe,const QString& directory,const std::shared_ptr<std::atomic_bool>& cancel) {
    QString source;int count=0;for(const auto& t:editorTools(recipe.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){source=t.id;++count;}
    if(count!=1)throw std::runtime_error("Tek enabled edinim kaynağı gerekir.");
    const auto result=ExecutionService{}.run(recipe.definition,directory,recipe.id,"validation-probe",cancel,source);
    if(result.state!=ExecutionState::Done)throw std::runtime_error(("Doğrulama karesi edinilemedi: "+result.error).toUtf8().constData());
    for(const auto& t:result.tools)if(t.toolId==source && t.state==ExecutionState::Done && t.outputs.value("image").valid)return std::get<ImageFrame>(t.outputs.value("image").data);
    throw std::runtime_error("Doğrulama karesi yok.");
}
void RuntimeService::suppressCancelled(const ProductionCycle& cycle,CycleResult& result) {
    if(ExperimentalRuntime::accepts(cycle)){ExperimentalRuntime::suppressCancelled(cycle,result);return;}
    if(result.completionSnapshot_!=cycle.snapshot || result.completionTicket_!=cycle.ticket ||
       result.completionDigest_.isEmpty() || result.completionDigest_!=ProductionRecipeService::completionDigest(result))
        throw std::runtime_error("Cannot suppress changed or foreign Runtime completion");
    EditorSession::suppressCancelled(result);
    result.completionDigest_=ProductionRecipeService::completionDigest(result);
}
CycleResult RuntimeService::execute(const ProductionCycle& cycle,const QString& catalogPath,const std::shared_ptr<std::atomic_bool>& cancel,ExecutionService::SourceAdapterFactory factory) {
    const auto report=[&](const CycleResult& result) noexcept {try{
        const auto recipe=cycle.snapshot->recipe();
    if(cycle.diagnostics){
        const QJsonObject context{{"ticket",QString::number(cycle.ticket)},{"cycleId",result.cycleId},{"recipeId",recipe.id}};
        QMap<QString,QString> sources;for(const auto& tool:editorTools(recipe.definition))if(tool.enabled && (tool.kind=="image_file" || tool.kind=="webcam" || tool.kind=="industrial_camera"))sources[tool.id]=tool.kind;
        if(result.state==ExecutionState::Error && sources.contains(result.acquisitionFailureSourceId_)){
            auto event=context;event["action"]="acquisition";event["phase"]="policyPreflight";event["attempted"]=false;
            event["sourceId"]=result.acquisitionFailureSourceId_;event["sourceKind"]=sources[result.acquisitionFailureSourceId_];
            event["executionState"]="ERROR";event["reason"]=result.error;cycle.diagnostics->append(event,true);
        }
        for(const auto& t:result.tools)if(sources.contains(t.toolId)){
            auto event=context;event["action"]="acquisition";event["sourceId"]=t.toolId;event["sourceKind"]=sources[t.toolId];event["executionState"]=executionStateName(t.state);event["reason"]=t.message;event["errorCode"]=t.errorCode;event["elapsedMs"]=t.milliseconds;
            for(const auto& frame:result.frames)if(frame.sourceId==t.toolId)event["frameId"]=frame.frameId;
            cycle.diagnostics->append(event,t.state==ExecutionState::Error);
        }
        auto event=context;event["action"]="completion";event["executionState"]=executionStateName(result.state);event["reason"]=result.error;event["elapsedMs"]=result.milliseconds;cycle.diagnostics->append(event,result.state==ExecutionState::Error);
    }
    }catch(...){} };
    if(ExperimentalRuntime::accepts(cycle)){if(factory)throw std::runtime_error("Experimental Runtime does not accept injected source adapters");auto result=ExperimentalRuntime::execute(cycle,cancel);report(result);return result;}
    if(!cycle.snapshot || !cycle.ticket)throw std::runtime_error("Runtime execution requires a production lease");
    const auto s=cycle.snapshot;const auto recipe=s->recipe();
    QJsonObject acquisition=s->dependencies()["measurementSource"].toObject()["acquisition"].toObject();
    if(s->dependencies().contains("sourceSet")){QJsonObject sources;const auto set=s->dependencies()["sourceSet"].toObject();for(auto i=set.begin();i!=set.end();++i)if(i->toObject().contains("measurementSource"))sources[i.key()]=i->toObject()["measurementSource"].toObject()["acquisition"];acquisition={{"sources",sources}};}
    CycleResult result;
    const auto execution=s->industrial() ? ExecutionService(*s->industrial(),std::move(factory)):ExecutionService(std::move(factory));
    try{result=execution.run(recipe.definition,s->projectDirectory(),recipe.id,s->validation().contentId,cancel,{}, {},
        [catalogPath,s](const ImageFrame& frame){return calibration(*s,frame,catalogPath);},nullptr,acquisition);}
    catch(const std::exception& e){result.recipeId=recipe.id;result.configId=s->validation().contentId;result.cycleId=QUuid::createUuid().toString(QUuid::WithoutBraces);result.state=ExecutionState::Error;result.error=QString::fromUtf8(e.what());}
    result.completionDigest_=ProductionRecipeService::completionDigest(result);
    result.completionTicket_=cycle.ticket;result.completionSnapshot_=s;
    report(result);
    return result;
}
}
