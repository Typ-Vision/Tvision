#include "RuntimeService.hpp"
#include <QDir>
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
RuntimeService::RuntimeService(QString directory,QString catalogPath,RuntimeIdentity identity)
    :store_(std::move(directory)),catalogPath_(std::move(catalogPath)),identity_(std::move(identity)),
     production_(identity_.authorize,[this](const Recipe& r){return dependencies(r);},16,
        [this](const QString& kind,const QString& id,const QJsonObject& o){store_.append(kind,id,o);}) {}
ProductionDependencies RuntimeService::dependencies(const Recipe& r) const {
    ProductionDependencies result;
    for(const auto& t:editorTools(r.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")) {
        const QJsonObject declared{{"kind",t.kind},{"parameters",t.parameters},{"enabled",t.enabled},{"planarSetup",r.definition.value("planarSetup")}};
        if(declaredInstallations_.value(t.id)!=declared)throw std::runtime_error("Kaynak veya beyan edilen fiziksel kurulum değişti; yeniden doğrulayın.");
    }
    if(r.calibrationBinding || !r.calibrationReference.isEmpty()) {
        QString source;for(const auto& t:editorTools(r.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam"))source=t.id;
        const auto found=installations_.constFind(source);
        if(found==installations_.cend())throw std::runtime_error("Runtime validation requires fresh source acquisition.");
        const auto setup=CalibrationWorkflow::installation(r,found.value());
        result.calibration=CalibrationService(catalogPath_,[setup](const QString&){return setup;}).resolve(r).dependency();
    }return result;
}
RecipeValidation RuntimeService::validate(const Recipe& r,const QString& directory,const ImageFrame& frame) {
    if(production_.busy())throw ProductionException(ProductionError::Busy,"Runtime cycle/recording pending.");
    // Permission is checked before accepting installation data or catalog work.
    if(identity_.principal.isEmpty() || !identity_.authorize || !identity_.authorize(identity_.principal,ProductionAction::Validate))throw ProductionException(ProductionError::Unauthorized,"Runtime validation denied.");
    const auto& m=frame.metadata();int sources=0;bool matches=false;
    for(const auto& t:editorTools(r.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){++sources;matches=t.id==m.sourceId;}
    if(sources!=1 || !matches || m.frameId.isEmpty() || m.cycleId.isEmpty() || !m.completedAtUtc.isValid())throw ProductionException(ProductionError::InvalidConfiguration,"Fresh acquisition provenance required.");
    observeInstallation(r);installations_.insert(m.sourceId,frame);
    validation_.reset();const auto v=production_.validate(identity_.principal,r,directory);validation_=v;return v;
}
void RuntimeService::observeInstallation(const Recipe& r) {
    for(const auto& t:editorTools(r.definition))if(t.kind=="image_file" || t.kind=="webcam")declaredInstallations_[t.id]=QJsonObject{{"kind",t.kind},{"parameters",t.parameters},{"enabled",t.enabled},{"planarSetup",r.definition.value("planarSetup")}};
}
void RuntimeService::activate(const Recipe& r,const QString& directory) {
    if(!validation_)throw ProductionException(ProductionError::UnknownValidation,"Validate the current draft first.");
    production_.activate(identity_.principal,validation_->id,r,directory);
}
void RuntimeService::rollback(const QString& id){production_.rollback(identity_.principal,id);}
ProductionCycle RuntimeService::begin(){return production_.beginCycle(identity_.principal);}
void RuntimeService::finish(quint64 ticket,const CycleResult& r){production_.finishCycle(ticket,r);}
ImageFrame RuntimeService::probe(const Recipe& recipe,const QString& directory,const std::shared_ptr<std::atomic_bool>& cancel) {
    QString source;int count=0;for(const auto& t:editorTools(recipe.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){source=t.id;++count;}
    if(count!=1)throw std::runtime_error("Tek enabled edinim kaynağı gerekir.");
    const auto result=ExecutionService{}.run(recipe.definition,directory,recipe.id,"validation-probe",cancel,source);
    if(result.state!=ExecutionState::Done)throw std::runtime_error(("Doğrulama karesi edinilemedi: "+result.error).toUtf8().constData());
    for(const auto& t:result.tools)if(t.toolId==source && t.state==ExecutionState::Done && t.outputs.value("image").valid)return std::get<ImageFrame>(t.outputs.value("image").data);
    throw std::runtime_error("Doğrulama karesi yok.");
}
CycleResult RuntimeService::execute(const ProductionCycle& cycle,const QString& catalogPath,const std::shared_ptr<std::atomic_bool>& cancel) {
    const auto s=cycle.snapshot;const auto recipe=s->recipe();
    return ExecutionService{}.run(recipe.definition,s->projectDirectory(),recipe.id,s->validation().contentId,cancel,{}, {},
        [recipe,catalogPath](const ImageFrame& frame){return CalibrationWorkflow::resolve(recipe,frame,catalogPath);});
}
}
