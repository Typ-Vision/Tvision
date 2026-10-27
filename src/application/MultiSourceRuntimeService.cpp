#include "MeasurementDiagnostic.hpp"
#include "MultiSourceRuntimeService.hpp"
#include "domain/ToolRegistry.hpp"
#include "ProductionResultCodec.hpp"
#include "IndustrialRuntime.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QUuid>
#include <limits>
namespace visionflow {
namespace {
void require(bool ok,ProductionError code,const char* message){if(!ok)throw ProductionException(code,message);}
QByteArray hash(const QJsonObject& o){return QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();}
}
MultiSourceRuntimeService::MultiSourceRuntimeService(RuntimeIdentity identity,Provider provider,ProductionRecipeService::Persist persist):identity_(std::move(identity)),provider_(std::move(provider)),persist_(std::move(persist)) {require(bool(provider_),ProductionError::InvalidConfiguration,"Whole source-set provider required");}
void MultiSourceRuntimeService::owner() const {require(QThread::currentThread()==thread_,ProductionError::WrongThread,"Multi-source controller owner thread required");}
void MultiSourceRuntimeService::permission(ProductionAction action) const {owner();require(!identity_.principal.isEmpty() && identity_.authorize && identity_.authorize(identity_.principal,action),ProductionError::Unauthorized,"Multi-source action denied");}
bool MultiSourceRuntimeService::busy() const {owner();return cycle_.ticket!=0;}
std::shared_ptr<const ProductionSnapshot> MultiSourceRuntimeService::active() const {owner();return prepared_.snapshot;}
QString MultiSourceRuntimeService::failure() const {owner();return failure_;}
const std::optional<CycleResult>& MultiSourceRuntimeService::lastCompletion() const {owner();return last_;}
MultiSourceRuntimeService::Prepared MultiSourceRuntimeService::capture(const Recipe& recipe,const QString& directory,bool configurationOnly) const {
    require(QJsonDocument(recipe.definition).toJson(QJsonDocument::Compact).size()<=16*1024*1024,ProductionError::InvalidConfiguration,"Whole recipe definition exceeds 16 MiB snapshot budget");
    require(recipe.definition["editor"].toObject()["schemaVersion"]==9,ProductionError::InvalidConfiguration,"Explicit editor9 required");validateEditorDefinition(recipe.definition);
    const auto graph=validateSourceGraph(editorTools(recipe.definition),executionConnections(recipe.definition));require(graph.valid() && !graph.requiredSources.isEmpty(),ProductionError::InvalidConfiguration,"Invalid/empty multi-source graph");
    const auto set=provider_(recipe);require(configurationOnly || !set.configurationOnly,ProductionError::DependencyChanged,"Configuration source set cannot grant Runtime admission");require(set.experimentalCalibrations.isEmpty(),ProductionError::DependencyChanged,"Experimental models cannot enter trusted Runtime");require(set.sources.size()==graph.requiredSources.size(),ProductionError::DependencyChanged,"Missing/foreign source snapshot set");Prepared out;QJsonObject sourceSet;QSet<QString> devices;QMap<QString,QJsonObject> cameraConfigurations;
    const auto configured=editorTools(recipe.definition);bool hasIndustrial=false;
    for(const auto& t:configured)if(t.enabled && t.kind=="industrial_camera")hasIndustrial=true;
    require(hasIndustrial==bool(set.industrial),ProductionError::DependencyChanged,"Missing/foreign industrial execution composition");
    std::optional<IndustrialRuntimeAdmission> industrial;
    if(hasIndustrial && !configurationOnly)industrial=freezeIndustrialRuntime(*set.industrial,configured);
    for(const auto& source:graph.requiredSources){
        require(set.sources.contains(source),ProductionError::DependencyChanged,"Required source snapshot missing");auto branch=SystemCalibration::branch(recipe,source);auto editor=branch.definition["editor"].toObject();editor["schemaVersion"]=SystemCalibration::source(branch).kind=="industrial_camera" ? 9:8;branch.definition["editor"]=editor;
        auto dependencies=set.sources[source];
        require(dependencies.industrialSource.isEmpty(),ProductionError::DependencyChanged,"Caller cannot supply industrial admission evidence");
        const bool industrialBranch=SystemCalibration::source(branch).kind=="industrial_camera";
        if(industrialBranch){if(configurationOnly){
            const auto request=industrialExecutionRequest(*set.industrial,SystemCalibration::source(branch));
            const QJsonObject configuration{{"setup",request.setup.toJson()},{"profile",request.profile.toJson()}};
            require(!cameraConfigurations.contains(request.camera.id.value) || cameraConfigurations[request.camera.id.value]==configuration,ProductionError::InvalidConfiguration,"Conflicting profiles for shared camera");cameraConfigurations[request.camera.id.value]=configuration;
        }else dependencies.industrialSource=industrial->sources.value(source);}

        if(!dependencies.systemCalibration.isEmpty())branch=SystemCalibration::prepareRecipe(SystemCalibration::decodeEvidence(dependencies.systemCalibration),branch,directory);
        else {branch.calibrationBinding.reset();branch.calibrationReference.clear();}
        const auto tool=SystemCalibration::source(branch);if(tool.kind=="image_file"){const QFileInfo file(QDir(directory).absoluteFilePath(tool.parameters["path"].toString()));require(file.isFile() && file.isReadable(),ProductionError::InvalidConfiguration,"Required source file unavailable");}QString device;
        if(branch.definition.contains("sourceSetup"))device="physical:"+branch.definition["sourceSetup"].toObject()["target"].toObject()["cameraId"].toString();
        else if(tool.kind=="webcam")device="legacy:"+tool.parameters["backend"].toString()+":"+QString::number(tool.parameters["deviceIndex"].toInt());
        if(!device.isEmpty()){require(!devices.contains(device) || (industrialBranch && set.industrial && (configurationOnly || set.industrial->sessions)),ProductionError::InvalidConfiguration,"Repeated physical source requires an explicit reuse policy");devices.insert(device);}
        if(configurationOnly && industrialBranch)continue;
        ProductionRecipeService validator([](const QString&,ProductionAction){return true;},[dependencies](const Recipe&){return dependencies;});validator.industrialAdmission_=industrialBranch;const auto validation=validator.validate(identity_.principal,branch,directory);if(configurationOnly)continue;validator.activate(identity_.principal,validation.id,branch,directory);
        out.branches[source]=validator.active();sourceSet[source]=validator.active()->dependencies();
    }
    if(configurationOnly)return {}; // No executable snapshot escapes static inspection.
    auto snapshot=std::make_shared<ProductionSnapshot>();snapshot->recipe_=recipe;snapshot->directory_=QDir(directory).absolutePath();snapshot->dependencies_={{"engine",ExecutionService::version()},{"sourceSet",sourceSet},{"scope",(persist_ ? "multi_source_runtime/1":"multi_source_in_memory/1")}};
    const auto pub=PublicationService::dependency(recipe.definition,set.publication);
    if(!pub.isEmpty()){snapshot->dependencies_["publication"]=pub;snapshot->publication_=set.publication;}
    if(industrial)snapshot->industrial_=std::make_shared<const IndustrialExecutionInputs>(industrial->inputs);
    snapshot->validation_={QUuid::createUuid().toString(QUuid::WithoutBraces),identity_.principal,QByteArray{},hash(snapshot->dependencies_),QDateTime::currentDateTimeUtc(),(persist_ ? "multi_source_runtime/1":"multi_source_in_memory/1")};
    // Canonical complete recipe identity uses the same fields as legacy admission.
    snapshot->validation_.contentId=ProductionRecipeService::recipeContent(recipe,directory);
    out.snapshot=std::move(snapshot);return out;
}
void MultiSourceRuntimeService::checkConfiguration(const Recipe& r,const QString& d) const {permission(ProductionAction::Validate);require(!busy(),ProductionError::Busy,"Cycle pending");(void)capture(r,d,true);}
void MultiSourceRuntimeService::prepare(const Recipe& recipe,const QString& directory){permission(ProductionAction::Validate);permission(ProductionAction::Activate);require(!busy(),ProductionError::Busy,"Cycle pending");auto candidate=capture(recipe,directory);prepared_=std::move(candidate);last_.reset();failure_.clear();}
void MultiSourceRuntimeService::current() const {require(bool(prepared_.snapshot),ProductionError::UnknownValidation,"No admitted source set");const auto next=capture(prepared_.snapshot->recipe(),prepared_.snapshot->projectDirectory());require(next.snapshot->validation().contentId==prepared_.snapshot->validation().contentId && next.snapshot->validation().dependencyId==prepared_.snapshot->validation().dependencyId,ProductionError::DependencyChanged,"Source/model set changed; prepare again");}
void MultiSourceRuntimeService::requireReady() const {permission(ProductionAction::Run);require(!busy(),ProductionError::Busy,"Cycle pending");current();}
void MultiSourceRuntimeService::persist(const QString& kind,const QString& id,const QJsonObject& value) const {
    if(!persist_)return;try{persist_(kind,id,value);}catch(const std::exception& e){throw ProductionException(ProductionError::PersistenceFailure,QString::fromUtf8(e.what()));}
}
ProductionCycle MultiSourceRuntimeService::begin(const QJsonObject& communication){
    requireReady();require(serial_<std::numeric_limits<quint64>::max(),ProductionError::HistoryFull,"Cycle ticket exhausted");
    const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);const auto now=QDateTime::currentDateTimeUtc();
    persist("events",id,{{"schemaVersion",1},{"eventId",id},{"action","CYCLE_BEGIN"},{"principal",identity_.principal},{"validationId",prepared_.snapshot->validation().id},{"startedAtUtc",now.toString(Qt::ISODateWithMs)},{"communication",communication}});
    beginId_=id;started_=now;communication_=communication;cycle_={++serial_,prepared_.snapshot};last_.reset();failure_.clear();pending_={};return cycle_;
}
void MultiSourceRuntimeService::finish(quint64 ticket,const CycleResult& result){
    owner();require(ticket && ticket==cycle_.ticket,ProductionError::InvalidCompletion,"Foreign/stale multi-source ticket");
    if(!pending_.isEmpty()){
        require(pendingCompletion_ && result.completionSnapshot_==cycle_.snapshot && result.completionTicket_==ticket && result.completionDigest_==ProductionRecipeService::completionDigest(result) && ProductionRecipeService::completionDigest(*pendingCompletion_)==result.completionDigest_,ProductionError::InvalidCompletion,"Pending completion cannot change");
        persist("cycles",pending_["recordId"].toString(),pending_);pending_={};pendingCompletion_.reset();last_=result;failure_.clear();cycle_={};return;
    }
    const auto branches=ProductionRecipeService::validateMultiSourceCompletion(cycle_,result,prepared_.branches);
    require(!cycle_.snapshot->publication(),ProductionError::InvalidCompletion,"Publication terminal bundle codec required (COMM-13)");
    if(pending_.isEmpty()){
        const auto& s=*cycle_.snapshot;QJsonArray frames,tools;
        for(const auto& f:result.frames)frames.append(QJsonObject{{"frameId",f.frameId},{"cycleId",f.cycleId},{"sourceId",f.sourceId},{"coordinateSystemId",f.coordinateSystemId},{"completedAtUtc",f.completedAtUtc.toString(Qt::ISODateWithMs)}});
        for(const auto& t:result.tools){QJsonObject outputs;for(auto i=t.outputs.cbegin();i!=t.outputs.cend();++i)outputs[i.key()]=ProductionResultCodec::output(*i);if(result.industrialEvidence.contains(t.toolId))for(auto i=t.outputs.begin();i!=t.outputs.end();++i)if(i->valid && std::holds_alternative<ImageFrame>(i->data)){auto o=outputs[i.key()].toObject(), image=o["imageInfo"].toObject();image["contentId"]=QString::fromLatin1(std::get<ImageFrame>(i->data).contentId().toHex());o["imageInfo"]=image;outputs[i.key()]=o;}QString name,kind;for(const auto& configured:editorTools(s.recipe().definition))if(configured.id==t.toolId){name=configured.name;kind=configured.kind;}
            tools.append(QJsonObject{{"toolId",t.toolId},{"name",name},{"kind",kind},{"state",executionStateName(t.state)},{"message",t.message},{"milliseconds",t.milliseconds},{"outputs",outputs},{"errorCode",t.errorCode},{"decision",t.decision==PartDecision::Pass ? "PASS":t.decision==PartDecision::Fail ? "FAIL":"NOT_EVALUATED"},{"calibration",t.calibration ? QJsonValue(t.calibration->toJson()):QJsonValue(QJsonValue::Null)}});
            if(t.fitDiagnostic){auto item=tools.last().toObject();item["measurementDiagnostic"]=encodeMeasurementDiagnostic(*t.fitDiagnostic);tools.replace(tools.size()-1,item);}
        }
        const auto& recipe=s.recipe();const QJsonObject configuration{{"recipeId",recipe.id},{"recipeVersion",recipe.recipeVersion},{"schemaVersion",recipe.schemaVersion},{"name",recipe.name},{"description",recipe.description},{"type",typeKey(recipe.applicationType)},{"camera",recipe.cameraReference},{"calibration",recipe.calibrationReference},{"calibrationBinding",recipe.calibrationBinding ? QJsonValue(recipe.calibrationBinding->toJson()):QJsonValue(QJsonValue::Null)},{"definition",recipe.definition},{"projectDirectory",s.projectDirectory()}};
        QJsonObject candidate{{"configuration",configuration},{"schemaVersion",s.industrial() ? 9:7},{"recordId",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"beginEventId",beginId_},{"ticket",QString::number(ticket)},{"principal",identity_.principal},{"recipeId",s.recipe().id},{"recipeName",s.recipe().name},{"recipeVersion",s.recipe().recipeVersion},{"applicationType",typeKey(s.recipe().applicationType)},{"validationId",s.validation().id},{"configId",QString::fromLatin1(s.validation().contentId)},{"dependencyId",QString::fromLatin1(s.validation().dependencyId)},{"dependencies",s.dependencies()},{"definition",s.recipe().definition},{"branches",branches},{"cycleId",result.cycleId},{"evaluationId",result.evaluationId},{"state",executionStateName(result.state)},{"error",result.error},{"milliseconds",result.milliseconds},{"startedAtUtc",started_.toString(Qt::ISODateWithMs)},{"completedAtUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},{"frames",frames},{"tools",tools},{"communicationState",communication_.isEmpty() ? "NOT_REQUESTED":"RESULT_PENDING"},{"communication",communication_}};
        ProductionResultCodec::validate(candidate);pending_=std::move(candidate);pendingCompletion_=result;
    }
    persist("cycles",pending_["recordId"].toString(),pending_);pending_={};pendingCompletion_.reset();last_=result;failure_.clear();cycle_={};
}
void MultiSourceRuntimeService::abandon(quint64 ticket,const QString& reason){owner();require(ticket && ticket==cycle_.ticket && !reason.trimmed().isEmpty() && reason.size()<=2048,ProductionError::InvalidCompletion,"Invalid abandonment ticket/reason");require(pending_.isEmpty(),ProductionError::PersistenceFailure,"Trusted completion recording pending");failure_=reason;last_.reset();cycle_={};}

}
