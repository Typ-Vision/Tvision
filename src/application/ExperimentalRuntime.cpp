#include "ExperimentalRuntime.hpp"
#include "ExperimentalOecsAcquisition.hpp"
#include "IndustrialExecution.hpp"
#include "EditorSession.hpp"
#include "ProductionResultCodec.hpp"
#include "MeasurementDiagnostic.hpp"
#include "SourceFrameIndex.hpp"
#include "ToolResultCodec.hpp"
#include "ScopedActiveCalibration.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <cmath>
#include <limits>

namespace visionflow {
namespace {
constexpr auto scope="experimental_runtime/2";
void require(bool value,const char* why){if(!value)throw ProductionException(ProductionError::InvalidCompletion,why);}
QByteArray hash(const QJsonObject& o){return QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();}
QJsonObject configuration(const Recipe& r,const QString& directory){return {{"recipeId",r.id},{"recipeName",r.name},{"recipeVersion",r.recipeVersion},{"applicationType",typeKey(r.applicationType)},{"definition",r.definition},{"directory",QDir(directory).absolutePath()}};}
void keys(const QJsonObject& o,const QStringList& expected){require(o.size()==expected.size(),"Experimental record fields missing/extra");for(auto i=o.begin();i!=o.end();++i)require(expected.contains(i.key()),"Unknown experimental field");}
QJsonObject imageInfo(const ImageFrame& image){return {{"width",image.size().width()},{"height",image.size().height()},{"pixelFormat",image.pixelFormat()}};}
QJsonObject modelEvidence(const ExperimentalScopedCalibrationSnapshot& t,const QString& source){
    const auto& s=t.snapshot.calibration;const auto& a=s.selection;
    const auto binding=SystemCalibration::experimentalBinding(t,source,source+"/image");
    require(t.snapshot.device.toJson()==s.scope["device"].toObject(),"Experimental snapshot device differs from model scope");
    return {{"target",t.snapshot.target.toJson()},{"device",t.snapshot.device.toJson()},{"record",s.record.encode()},
        {"selection",QJsonObject{{"id",a.id},{"contentId",a.contentId},{"version",a.version},{"revision",a.revision}}},
        {"scope",s.scope},{"acquisition",s.acquisition},{"executionDependency",binding.toJson()}};
}
ExperimentalScopedCalibrationSnapshot decodeModel(const QJsonObject& o,const QString& source){
    keys(o,{"target","device","record","selection","scope","acquisition","executionDependency"});
    const auto a=o["selection"].toObject();keys(a,{"id","contentId","version","revision"});
    ExperimentalScopedCalibrationSnapshot t{{SetupReference::fromJson(o["target"].toObject()),DeviceBinding::fromJson(o["device"].toObject()),
        {CalibrationModelRecord::decode(o["record"].toObject()),{a["id"].toString(),a["contentId"].toString(),a["version"].toInt(),a["revision"].toInt()},o["scope"].toObject(),o["acquisition"].toObject()}}};
    require(modelEvidence(t,source)==o,"Experimental model evidence differs");return t;
}
void modelFrame(const QJsonObject& entry,const Recipe& branch,const ImageFrame& image){
    if(entry.contains("imageInfo"))require(entry["imageInfo"].toObject()==imageInfo(image),"Experimental file geometry changed after snapshot");
    if(entry.contains("contentId"))require(entry["contentId"].toString().toLatin1()==image.contentId().toHex() && entry["experimentalAcquisition"].toObject()==image.metadata().experimentalAcquisition,"Experimental file/sidecar changed after snapshot");
    if(entry.contains("experimentalCalibration")){
        const auto context=SystemCalibration::resolveExperimental(decodeModel(entry["experimentalCalibration"].toObject(),image.metadata().sourceId),branch,image);
        require(context.calibration && context.current,"Experimental frame/model mismatch");
    }
}
QString cell(QString s){s.replace('"',"\"\"");return '"'+s+'"';}
void provenance(const QJsonObject& p){
    require(p["schema"]=="visionflow.oecs-experimental1" && p["policy"]=="UnverifiedNextArrival" && p["productionAdmission"]==false && p["triggerAssociationVerified"]==false,"Experimental provenance/admission differs");
    require(p["vendor"]=="Opto Engineering Srl" && p["model"]=="COE-050-M-USB-050-IR-C" && !p["serial"].toString().isEmpty() && p["width"]==2448 && p["height"]==2048 && p["originX"]==0 && p["originY"]==0 && p["embeddedMetadataEnabled"]==false,"Experimental identity/geometry differs");
    const auto actual=AcquisitionReadback::fromJson(p["readback"].toObject());const auto profile=AcquisitionProfile::fromJson(p["profileSnapshot"].toObject());
    const auto geometry=actual.geometry.descriptor();auto requested=profile.requested;
    if(actual.settings.exposureTimeUs==double(float(requested.exposureTimeUs)))requested.exposureTimeUs=actual.settings.exposureTimeUs;
    if(actual.settings.gain==double(float(requested.gain)))requested.gain=actual.settings.gain;
    require(geometry.resolution==QSize(2448,2048) && actual.geometry.offset==QPoint(0,0) && actual.geometry.binning==QSize(1,1) && actual.geometry.decimation==QSize(1,1) && actual.geometry.reverse==GeometryMirror{} && actual.settings==requested && actual.settings.trigger==TriggerSettings{true,"FrameBurstStart","Software"} && profile.id.value==p["profileId"].toString() && profile.revision==p["profileRevision"].toInt() && p["pixelFormat"]=="Mono8" && !p["acquisitionId"].toString().isEmpty() && p["contentId"].toString().size()==64,"Experimental readback/profile/hash differs");
}
}
ExperimentalRuntime::ExperimentalRuntime(RuntimeIdentity identity,RuntimeService::MultiProvider provider,ProductionRecipeService::Persist persist):identity_(std::move(identity)),provider_(std::move(provider)),persist_(std::move(persist)){}
void ExperimentalRuntime::permission() const {
    if(QThread::currentThread()!=thread_)throw ProductionException(ProductionError::WrongThread,"Experimental Runtime owner thread required");
    if(identity_.principal.isEmpty() || !identity_.authorize || !identity_.authorize(identity_.principal,ProductionAction::Run))throw ProductionException(ProductionError::Unauthorized,"Experimental Runtime Run denied");
}
std::shared_ptr<const ProductionSnapshot> ExperimentalRuntime::capture(const Recipe& recipe,const QString& directory,bool configurationOnly) const {
    require(recipe.definition["editor"].toObject()["schemaVersion"]==9,"Experimental Runtime requires editor9");validateEditorDefinition(recipe.definition);
    const auto tools=editorTools(recipe.definition);const auto graph=validateSourceGraph(tools,editorConnections(recipe.definition));require(graph.valid() && !graph.requiredSources.isEmpty(),"Experimental source graph invalid");
    auto s=std::make_shared<ProductionSnapshot>();s->recipe_=recipe;s->directory_=QDir(directory).absolutePath();
    bool industrial=false;for(const auto& t:tools)if(graph.requiredSources.contains(t.id) && t.kind=="industrial_camera")industrial=true;
    MultiSourceDependencies supplied;if(provider_)supplied=provider_(recipe);require(configurationOnly || !supplied.configurationOnly,"Configuration source set cannot grant experimental admission");if(industrial){require(bool(provider_),"Experimental source provider required");require(bool(supplied.industrial),"Experimental industrial composition required");require(supplied.industrial->leaseUse==CameraLeaseUse::Editor && supplied.industrial->admittedDevices.isEmpty(),"Experimental composition cannot contain production admission");s->industrial_=supplied.industrial;}
    QJsonObject sources;QSet<QString> cameras;QMap<QString,QJsonObject> cameraConfigurations;for(auto i=supplied.experimentalCalibrations.begin();i!=supplied.experimentalCalibrations.end();++i)require(graph.requiredSources.contains(i.key()),"Foreign experimental model source");
    for(const auto& source:graph.requiredSources){const auto branch=SystemCalibration::branch(recipe,source);const bool metric=SystemCalibration::requestsCalibration(branch,false);require(!branch.calibrationBinding && branch.calibrationReference.isEmpty(),"Experimental Runtime forbids legacy/global model fallback");
        const auto t=SystemCalibration::source(branch);QJsonObject entry;
        if(t.kind=="industrial_camera"){
            const auto request=industrialExecutionRequest(*s->industrial_,t);
            if(configurationOnly){const QJsonObject configuration{{"setup",request.setup.toJson()},{"profile",request.profile.toJson()}};require(!cameraConfigurations.contains(request.camera.id.value) || cameraConfigurations[request.camera.id.value]==configuration,"Conflicting experimental shared camera profiles");cameraConfigurations[request.camera.id.value]=configuration;}
            require(!cameras.contains(request.camera.id.value) || configurationOnly || s->industrial_->sessions,"Repeated physical experimental source");cameras.insert(request.camera.id.value);
            require(request.profile.requested.trigger==TriggerSettings{true,"FrameBurstStart","Software"},"Explicit FrameBurstStart profile required");
            require(request.camera.physical && request.camera.physical->vendor=="Opto Engineering Srl" && request.camera.physical->model=="COE-050-M-USB-050-IR-C" && !request.camera.physical->serial.isEmpty(),"Experimental camera identity unsupported");
            const auto physical=QString::fromLatin1(hash(request.camera.physical->toJson()));require(!cameras.contains(physical) || configurationOnly || s->industrial_->sessions,"Repeated physical experimental device");cameras.insert(physical);
            entry={{"camera",request.camera.toJson()},{"setup",request.setup.toJson()},{"profile",request.profile.toJson()}};
        }else if(t.kind=="image_file"){
            const auto image=ImageFileService{}.load(QDir(directory).absoluteFilePath(t.parameters["path"].toString()),true);entry={{"contentId",QString::fromLatin1(image.contentId().toHex())},{"experimentalAcquisition",image.metadata().experimentalAcquisition},{"imageInfo",imageInfo(image)}};
        }else require(false,"Experimental Runtime supports industrial camera / file sources");
        if(metric){
            const auto typed=supplied.experimentalCalibrations.value(source);require(bool(typed),"Metric source requires typed experimental model");
            const auto ref=SystemCalibration::setupReference(branch);require(ref && *ref==typed->snapshot.target,"Experimental model belongs to another source installation");
            if(configurationOnly){
                const auto& m=typed->snapshot.calibration;const auto& dep=m.record.model().dependency();
                require(m.record.experimentallyActivatable() && m.selection.id==dep.calibrationId && m.selection.version==dep.version && m.selection.contentId==m.record.contentId(),"Invalid remembered experimental model");
                const auto& expected=*m.record.image().experimental;
                require(m.record.setup()==IndependentCaptureRequest::installation(expected.camera,expected.setup),"Experimental model installation changed");
                if(t.kind=="image_file"){const auto image=ImageFileService{}.load(QDir(directory).absoluteFilePath(t.parameters["path"].toString()),true);ExperimentalCalibrationEvidence{SourceOrigin::SetupFile,expected.camera,expected.setup,expected.profile,image.metadata().experimentalAcquisition}.verify(image);}
                else {const auto request=industrialExecutionRequest(*s->industrial_,t);require(expected.camera.physical==request.camera.physical && expected.camera.id==request.camera.id && expected.setup.geometry==request.setup.geometry && expected.setup.id==request.setup.id && expected.setup.revision==request.setup.revision,"Experimental model/source installation differs");}
                continue;
            }
            entry["experimentalCalibration"]=modelEvidence(*typed,source);
            if(t.kind=="image_file"){const auto image=ImageFileService{}.load(QDir(directory).absoluteFilePath(t.parameters["path"].toString()),true);const auto proof=MeasurementSourceEvidence::fromJson(typed->snapshot.calibration.acquisition["probe"].toObject());const auto& expected=*proof.experimental;ExperimentalCalibrationEvidence{SourceOrigin::SetupFile,expected.camera,expected.setup,expected.profile,image.metadata().experimentalAcquisition}.verify(image);}
            else {const auto request=industrialExecutionRequest(*s->industrial_,t);const auto proof=MeasurementSourceEvidence::fromJson(typed->snapshot.calibration.acquisition["probe"].toObject());
                require(proof.experimental && proof.experimental->camera.id==request.camera.id && proof.experimental->setup==request.setup && proof.experimental->profile==request.profile,"Experimental model/acquisition settings differ");}
        }else require(!supplied.experimentalCalibrations.contains(source),"Raw source cannot inherit metric model");
        if(t.kind=="image_file" && supplied.sources.contains(source) && !supplied.sources[source].measurementSource.isEmpty())entry["measurementSource"]=supplied.sources[source].measurementSource;
        sources[source]=entry;
    }
    if(configurationOnly)return {}; // Static inspection never publishes an execution capability.
    s->dependencies_={{"scope",scope},{"engine",ExecutionService::version()},{"sourceSet",sources}};
    require(QJsonDocument(s->dependencies_).toJson(QJsonDocument::Compact).size()<=ProductionResultCodec::maxRecordBytes,"Experimental snapshot exceeds budget");
    s->validation_={QUuid::createUuid().toString(QUuid::WithoutBraces),identity_.principal,hash(configuration(recipe,directory)),hash(s->dependencies_),QDateTime::currentDateTimeUtc(),scope};return s;
}
void ExperimentalRuntime::checkConfiguration(const Recipe& r,const QString& d) const {
    if(QThread::currentThread()!=thread_)throw ProductionException(ProductionError::WrongThread,"Experimental owner thread required");
    if(identity_.principal.isEmpty() || !identity_.authorize || !identity_.authorize(identity_.principal,ProductionAction::Validate))throw ProductionException(ProductionError::Unauthorized,"Configuration inspection denied");
    if(busy())throw ProductionException(ProductionError::Busy,"Cycle pending");(void)capture(r,d,true);
}
void ExperimentalRuntime::prepare(const Recipe& r,const QString& directory){permission();if(!identity_.authorize(identity_.principal,ProductionAction::Validate) || !identity_.authorize(identity_.principal,ProductionAction::Activate))throw ProductionException(ProductionError::Unauthorized,"Experimental Runtime preparation denied");require(!busy(),"Experimental recording/cycle pending");auto candidate=capture(r,directory);snapshot_=std::move(candidate);faulted_=false;}
void ExperimentalRuntime::requireReady() const {permission();if(busy())throw ProductionException(ProductionError::Busy,"Experimental cycle/recording pending");require(snapshot_ && !faulted_,"Experimental Runtime not ready / busy / manual Run required after fault or cancellation");const auto next=capture(snapshot_->recipe(),snapshot_->projectDirectory());require(next->validation().contentId==snapshot_->validation().contentId && next->validation().dependencyId==snapshot_->validation().dependencyId,"Experimental source/profile/model changed; Run again");}
ProductionCycle ExperimentalRuntime::begin(const QJsonObject& communication){requireReady();require(communication.isEmpty(),"Experimental Runtime forbids PLC/robot communication");require(serial_!=std::numeric_limits<quint64>::max(),"Experimental ticket exhausted");cycle_={++serial_,snapshot_};started_=QDateTime::currentDateTimeUtc();return cycle_;}
bool ExperimentalRuntime::accepts(const ProductionCycle& c){return c.snapshot && c.snapshot->validation().scope==scope;}
QByteArray ExperimentalRuntime::digest(const CycleResult& r){
    const auto dependency=r.experimentalSnapshot_ ? r.experimentalSnapshot_->validation().dependencyId:QByteArray{};
    return QCryptographicHash::hash(QByteArray("experimental-runtime-completion/2:")+QByteArray::number(r.experimentalTicket_)+":"+dependency+":"+ProductionRecipeService::completionDigest(r),QCryptographicHash::Sha256);
}
void ExperimentalRuntime::completion(const ProductionCycle& c,const CycleResult& r){require(accepts(c) && c.ticket && r.experimentalSnapshot_==c.snapshot && r.experimentalTicket_==c.ticket && !r.experimentalDigest_.isEmpty() && r.experimentalDigest_==digest(r) && r.completionDigest_.isEmpty() && !r.completionSnapshot_ && r.industrialEvidence.isEmpty(),"Missing/changed/foreign experimental completion");}
CycleResult ExperimentalRuntime::execute(const ProductionCycle& c,const std::shared_ptr<std::atomic_bool>& cancel){
    require(accepts(c) && c.ticket,"Experimental lease required");const auto s=c.snapshot;
    auto service=s->industrial() ? ExecutionService(*s->industrial()):ExecutionService{};
    const auto sources=s->dependencies()["sourceSet"].toObject();QJsonObject acquisitions;
    for(auto i=sources.begin();i!=sources.end();++i){const auto entry=i->toObject();const auto model=entry["experimentalCalibration"].toObject();
        if(!model.isEmpty()){const auto typed=decodeModel(model,i.key());acquisitions[i.key()]=QJsonObject{{"target",typed.snapshot.target.toJson()},{"device",typed.snapshot.device.toJson()},{"geometry",typed.snapshot.calibration.scope["setup"].toObject()["geometry"]}};}
        else if(entry.contains("measurementSource"))acquisitions[i.key()]=entry["measurementSource"].toObject()["acquisition"];
    }
    const auto resolver=[sources,s](const ImageFrame& image){const auto id=image.metadata().sourceId;const auto entry=sources[id].toObject();const auto branch=SystemCalibration::branch(s->recipe(),id);
        modelFrame(entry,branch,image);if(entry.contains("experimentalCalibration"))return SystemCalibration::resolveExperimental(decodeModel(entry["experimentalCalibration"].toObject(),id),branch,image);
        MeasurementExecutionContext raw;raw.rawSize=image.size();raw.error="Raw/px source has no experimental model";return raw;};
    auto result=service.withExperimentalEditorReplay().run(s->recipe().definition,s->projectDirectory(),s->recipe().id,s->validation().contentId,cancel,{},{},resolver,nullptr,{{"sources",acquisitions}});
    try{for(const auto& t:result.tools)if(sources.contains(t.toolId))for(const auto& output:t.outputs)if(output.valid && std::holds_alternative<ImageFrame>(output.data))modelFrame(sources[t.toolId].toObject(),SystemCalibration::branch(s->recipe(),t.toolId),std::get<ImageFrame>(output.data));}
    catch(const std::exception& e){EditorSession::suppressCancelled(result);result.state=ExecutionState::Error;result.error=QString::fromUtf8(e.what());}
    for(auto& t:result.tools){t.decision=PartDecision::NotEvaluated;if(t.calibration)t.message="Metrik sonuç; parça kararı değerlendirilmedi.";}
    if(result.state==ExecutionState::Error && result.error.isEmpty())for(const auto& t:result.tools)if(t.state==ExecutionState::Error){if(!result.error.isEmpty())result.error+='\n';result.error+=t.errorCode+" "+t.message;}
    result.variables_=runtimeVariables(*s,result);
    result.experimentalSnapshot_=s;result.experimentalTicket_=c.ticket;result.experimentalDigest_=digest(result);return result;
}
void ExperimentalRuntime::suppressCancelled(const ProductionCycle& c,CycleResult& r){completion(c,r);EditorSession::suppressCancelled(r);r.variables_=runtimeVariables(*c.snapshot,r);r.experimentalDigest_=digest(r);}
void ExperimentalRuntime::finish(quint64 ticket,const CycleResult& r){
    permission();require(ticket && ticket==cycle_.ticket,"Foreign experimental ticket");completion(cycle_,r);
    if(pending_.isEmpty()){
        require(!r.partial && r.recipeId==snapshot_->recipe().id && r.configId==snapshot_->validation().contentId,"Experimental configuration/completion differs");
        QJsonArray frames,tools;
        for(const auto& f:r.frames){QByteArray content;for(const auto& t:r.tools)if(t.toolId==f.sourceId)for(const auto& v:t.outputs)if(v.valid && std::holds_alternative<ImageFrame>(v.data))content=std::get<ImageFrame>(v.data).contentId();require(!content.isEmpty(),"Experimental frame missing raw raster");frames.append(QJsonObject{{"sourceId",f.sourceId},{"frameId",f.frameId},{"cycleId",f.cycleId},{"coordinateSystemId",f.coordinateSystemId},{"completedAtUtc",f.completedAtUtc.toString(Qt::ISODateWithMs)},{"experimentalAcquisition",f.experimentalAcquisition},{"contentId",QString::fromLatin1(content.toHex())}});}
        for(const auto& t:r.tools){require(t.decision==PartDecision::NotEvaluated,"Experimental Runtime cannot publish a part decision");QJsonObject outputs;for(auto i=t.outputs.begin();i!=t.outputs.end();++i){auto o=ProductionResultCodec::output(*i);o["experimentalAcquisition"]=i->origin.experimentalAcquisition;outputs[i.key()]=o;}
            QJsonObject o{{"calibration",t.calibration ? QJsonValue(t.calibration->toJson()):QJsonValue(QJsonValue::Null)},{"toolId",t.toolId},{"state",executionStateName(t.state)},{"message",t.message},{"errorCode",t.errorCode},{"decision","NOT_EVALUATED"},{"outputs",outputs}};if(t.fitDiagnostic)o["measurementDiagnostic"]=encodeMeasurementDiagnostic(*t.fitDiagnostic);tools.append(o);
        }
        const auto& recipe=snapshot_->recipe();QJsonObject candidate{{"schema","visionflow.experimental-runtime2"},{"schemaVersion",2},{"recordId",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"principal",identity_.principal},{"recipeId",recipe.id},{"recipeName",recipe.name},{"recipeVersion",recipe.recipeVersion},{"applicationType",typeKey(recipe.applicationType)},{"configuration",configuration(recipe,snapshot_->projectDirectory())},{"configId",QString::fromLatin1(snapshot_->validation().contentId)},{"dependencies",snapshot_->dependencies()},{"dependencyId",QString::fromLatin1(snapshot_->validation().dependencyId)},{"cycleId",r.cycleId},{"evaluationId",r.evaluationId},{"state",executionStateName(r.state)},{"error",r.error},{"startedAtUtc",started_.toString(Qt::ISODateWithMs)},{"completedAtUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},{"frames",frames},{"tools",tools},{"productionAdmission",false},{"triggerAssociationVerified",false},{"communicationState","BLOCKED_EXPERIMENTAL"}};
        validateRecord(candidate);pending_=std::move(candidate);pendingDigest_=r.experimentalDigest_;
    }
    require(r.experimentalDigest_==pendingDigest_,"Pending experimental completion cannot change");
    try{persist_("experimental_cycles",pending_["recordId"].toString(),pending_);}catch(const std::exception& e){throw ProductionException(ProductionError::PersistenceFailure,QString::fromUtf8(e.what()));}
    pending_={};pendingDigest_.clear();cycle_={};faulted_=r.state!=ExecutionState::Done;
}
void ExperimentalRuntime::validateRecord(const QJsonObject& r){
    require(QJsonDocument(r).toJson(QJsonDocument::Compact).size()<=ProductionResultCodec::maxRecordBytes,"Experimental record exceeds budget");
    keys(r,{"schema","schemaVersion","recordId","principal","recipeId","recipeName","recipeVersion","applicationType","configuration","configId","dependencies","dependencyId","cycleId","evaluationId","state","error","startedAtUtc","completedAtUtc","frames","tools","productionAdmission","triggerAssociationVerified","communicationState"});
    const bool metricVersion=r["schema"]=="visionflow.experimental-runtime2" && r["schemaVersion"]==2;
    require((metricVersion || (r["schema"]=="visionflow.experimental-runtime1" && r["schemaVersion"]==1)) && r["productionAdmission"]==false && r["triggerAssociationVerified"]==false && r["communicationState"]=="BLOCKED_EXPERIMENTAL","Experimental envelope/admission invalid");
    require(!QUuid(r["recordId"].toString()).isNull() && !r["cycleId"].toString().isEmpty() && !r["principal"].toString().isEmpty() && QDateTime::fromString(r["startedAtUtc"].toString(),Qt::ISODateWithMs).isValid() && QDateTime::fromString(r["completedAtUtc"].toString(),Qt::ISODateWithMs).isValid(),"Experimental identity/time missing");
    const auto config=r["configuration"].toObject(),deps=r["dependencies"].toObject();
    keys(config,{"recipeId","recipeName","recipeVersion","applicationType","definition","directory"});keys(deps,{"scope","engine","sourceSet"});
    require(config["definition"].toObject()["editor"].toObject()["schemaVersion"]==9 && QStringList{"measurement","defect_inspection","robot_guidance"}.contains(config["applicationType"].toString()),"Experimental configuration type/schema invalid");
    require(r["configId"].toString().toLatin1()==hash(config) && r["dependencyId"].toString().toLatin1()==hash(deps) && deps["scope"]==(metricVersion ? scope:"experimental_runtime/1") && config["recipeId"]==r["recipeId"] && config["recipeName"]==r["recipeName"] && config["recipeVersion"]==r["recipeVersion"] && config["applicationType"]==r["applicationType"],"Experimental immutable configuration hash differs");
    const auto definition=config["definition"].toObject();validateEditorDefinition(definition);const auto configured=editorTools(definition);const auto graph=validateSourceGraph(configured,editorConnections(definition));
    require(graph.valid() && !graph.requiredSources.isEmpty() && deps["sourceSet"].toObject().size()==graph.requiredSources.size() && r["frames"].isArray() && r["tools"].isArray() && r["tools"].toArray().size()<=256 && QStringList{"DONE","ERROR","SKIPPED"}.contains(r["state"].toString()),"Experimental graph/state invalid");
    QMap<QString,QString> contentIds;QMap<QString,FrameMetadata> bySource;QVector<FrameMetadata> frames;QStringList acquired;
    for(const auto& v:r["frames"].toArray()){const auto f=v.toObject();keys(f,{"sourceId","frameId","cycleId","coordinateSystemId","completedAtUtc","experimentalAcquisition","contentId"});FrameMetadata m;m.sourceId=f["sourceId"].toString();m.frameId=f["frameId"].toString();m.cycleId=f["cycleId"].toString();m.coordinateSystemId=f["coordinateSystemId"].toString();m.completedAtUtc=QDateTime::fromString(f["completedAtUtc"].toString(),Qt::ISODateWithMs);m.experimentalAcquisition=f["experimentalAcquisition"].toObject();require(graph.requiredSources.contains(m.sourceId) && !bySource.contains(m.sourceId) && m.completedAtUtc.isValid(),"Experimental duplicate/foreign frame");if(!m.experimentalAcquisition.isEmpty())provenance(m.experimentalAcquisition);require(f["contentId"].toString().size()==64,"Experimental raster hash missing");contentIds[m.sourceId]=f["contentId"].toString();bySource[m.sourceId]=m;frames.append(m);acquired.append(m.sourceId);}
    if(r["state"]=="DONE")require(acquired.size()==graph.requiredSources.size(),"Experimental DONE missing source");
    if(r["state"]=="SKIPPED")require(frames.isEmpty(),"Experimental Stop retained frames");
    const auto sources=deps["sourceSet"].toObject();
    for(const auto& source:graph.requiredSources){require(sources.contains(source),"Experimental admission missing source");const auto entry=sources[source].toObject();const auto node=std::find_if(configured.begin(),configured.end(),[&](const auto& t){return t.id==source;});require(node!=configured.end(),"Experimental source missing");
        Recipe graphRecipe;graphRecipe.definition=definition;const auto branch=SystemCalibration::branch(graphRecipe,source);const bool metric=SystemCalibration::requestsCalibration(branch,false);
        if(entry.contains("measurementSource")){const auto context=entry["measurementSource"].toObject();keys(context,{"schemaVersion","binding","acquisition","origin"});const auto acquisition=context["acquisition"].toObject();keys(acquisition,{"target","device","geometry"});const auto ref=SystemCalibration::setupReference(branch);require(ref && context["schemaVersion"]==1 && context["origin"]=="offline_replay" && context["binding"]==branch.definition["sourceSetup"] && acquisition["target"]==ref->toJson(),"Experimental replay context differs");DeviceBinding::fromJson(acquisition["device"].toObject());GeometryDescriptor::fromJson(acquisition["geometry"].toObject());}
        require(metricVersion || !metric,"Legacy experimental record cannot carry metric graph");require(metric==entry.contains("experimentalCalibration"),"Experimental metric source model missing/foreign");
        if(metric){const auto typed=decodeModel(entry["experimentalCalibration"].toObject(),source);const auto ref=SystemCalibration::setupReference(branch);require(ref && *ref==typed.snapshot.target,"Experimental model/recipe target differs");
            const auto proof=MeasurementSourceEvidence::fromJson(typed.snapshot.calibration.acquisition["probe"].toObject());
            if(node->kind=="industrial_camera")require(entry["setup"].toObject()==proof.experimental->setup.toJson() && entry["profile"].toObject()==proof.experimental->profile.toJson() && entry["camera"].toObject()==proof.experimental->camera.toJson(),"Frozen industrial model/source settings differ");
            if(bySource.contains(source)){ExperimentalCalibrationEvidence actual{node->kind=="image_file" ? SourceOrigin::SetupFile:SourceOrigin::CameraAcquired,proof.experimental->camera,proof.experimental->setup,proof.experimental->profile,bySource[source].experimentalAcquisition};actual.validate();require(actual.acquisition["contentId"]==contentIds[source],"Experimental model/frame raster differs");}
        }
        if(node->kind=="industrial_camera"){
            QStringList entryKeys{"camera","setup","profile"};if(metricVersion && entry.contains("experimentalCalibration"))entryKeys<<"experimentalCalibration";keys(entry,entryKeys);const auto camera=MeasurementCamera::fromJson(entry["camera"].toObject());const auto setup=MeasurementSetupDescriptor::fromJson(entry["setup"].toObject());const auto profile=AcquisitionProfile::fromJson(entry["profile"].toObject());const auto ref=IndustrialSourceReference::fromJson(node->parameters["sourceReference"].toObject());ref.validateAgainst({{camera},{setup},{profile}});
            if(bySource.contains(source)){const auto p=bySource[source].experimentalAcquisition;provenance(p);require(p["contentId"]==contentIds[source],"Experimental raster/provenance hash differs");require(camera.physical && p["vendor"]==camera.physical->vendor && p["model"]==camera.physical->model && p["serial"]==camera.physical->serial && p["profileSnapshot"]==entry["profile"],"Experimental acquired identity/profile differs from admission");}
        }else{require(node->kind=="image_file","Experimental source kind invalid");QStringList entryKeys{"contentId","experimentalAcquisition"};if(metricVersion)entryKeys<<"imageInfo";if(metricVersion && entry.contains("experimentalCalibration"))entryKeys<<"experimentalCalibration";if(metricVersion && entry.contains("measurementSource"))entryKeys<<"measurementSource";keys(entry,entryKeys);if(bySource.contains(source))require(contentIds[source]==entry["contentId"] && bySource[source].experimentalAcquisition==entry["experimentalAcquisition"].toObject(),"Experimental replay provenance differs from frozen admission");}
    }
    std::optional<SourceFrameIndex> index;if(!frames.isEmpty())index.emplace(r["cycleId"].toString(),acquired,frames);QSet<QString> seen;
    for(const auto& v:r["tools"].toArray()){const auto t=v.toObject();QStringList fields{"toolId","state","message","errorCode","decision","outputs"};if(metricVersion)fields<<"calibration";if(t.contains("measurementDiagnostic"))fields<<"measurementDiagnostic";keys(t,fields);const auto id=t["toolId"].toString();auto node=std::find_if(configured.begin(),configured.end(),[&](const auto& x){return x.id==id;});require(node!=configured.end() && !seen.contains(id) && t["decision"]=="NOT_EVALUATED" && t["outputs"].isObject(),"Experimental foreign tool/decision");seen.insert(id);
        if(t.contains("measurementDiagnostic")){validateMeasurementDiagnostic(t,r);require(decodeMeasurementDiagnostic(t["measurementDiagnostic"].toObject()).sourceId==graph.toolSources.value(id),"Experimental diagnostic source differs");}
        require(QStringList{"DONE","ERROR","SKIPPED"}.contains(t["state"].toString()),"Experimental tool state invalid");
        const auto outputs=t["outputs"].toObject();if(node->kind=="set_variables" && t["state"]=="DONE")require(outputs.size()==1 && outputs.contains("bundle"),"Experimental assignment bundle missing");if(r["state"]=="SKIPPED" || (!graph.requiredSources.contains(graph.toolSources.value(id)) && !(node->enabled && node->kind=="set_variables")))require(t["state"]=="SKIPPED" && outputs.isEmpty() && !t.contains("measurementDiagnostic"),"Experimental stopped/disabled tool retained output");
        const auto source=graph.toolSources.value(id);
        const auto model=sources[source].toObject()["experimentalCalibration"].toObject();std::optional<CalibrationDependency> binding;
        if(!model.isEmpty())binding=SystemCalibration::experimentalBinding(decodeModel(model,source),source,source+"/image");
        if(metricVersion){require(t["calibration"].isNull() || (binding && t["calibration"].toObject()==binding->toJson()),"Experimental tool/model binding differs");if(t["state"]!="DONE")require(t["calibration"].isNull() && outputs.isEmpty(),"Error tool retained metrology");}
        for(auto p=outputs.begin();p!=outputs.end();++p){const auto o=p->toObject();const auto ports=outputPorts(node->kind);const auto port=std::find_if(ports.begin(),ports.end(),[&](const auto& x){return x.id==p.key();});require(port!=ports.end(),"Undeclared experimental output");
            const bool valid=o.contains("typedPort") ? o["typedPort"].toObject()["valid"].toBool():!o["imageInfo"].isNull();
            if(o.contains("typedPort")){const auto value=ToolResultCodec::decode(o["typedPort"].toObject());if(value.valid && value.port.unit=="mm")require(metricVersion && binding && t["calibration"].toObject()==binding->toJson(),"Metric port missing exact model evidence");if(value.valid && !value.evaluationId.isEmpty())require(value.evaluationId==r["evaluationId"].toString(),"Experimental output evaluation differs");auto expected=ProductionResultCodec::output(value);expected["experimentalAcquisition"]=o["experimentalAcquisition"];require(o==expected,"Experimental outer/typed output differs");require(value.port.id==port->id && value.port.type==port->type && value.port.unit==port->unit && value.port.frame==port->frame,"Experimental port declaration differs");}
            else{keys(o,{"type","unit","coordinateFrame","frameId","cycleId","sourceId","imageInfo","experimentalAcquisition"});require(port->type==PortType::Image && o["type"]=="Image" && o["unit"]==port->unit && o["coordinateFrame"]==port->frame,"Experimental image declaration differs");if(valid){const auto info=o["imageInfo"].toObject();if(metricVersion && graph.requiredSources.contains(id) && node->kind=="image_file")require(info==sources[source].toObject()["imageInfo"].toObject(),"Experimental source geometry differs from frozen raster");require(info["width"].toInt()>0 && info["height"].toInt()>0 && !info["pixelFormat"].toString().isEmpty(),"Experimental image geometry missing");}}
            if(valid && port->type==PortType::AssignmentBundle){
                const auto value=ToolResultCodec::decode(o["typedPort"].toObject());const auto& b=std::get<AssignmentBundle>(value.data);const auto c=decodeAssignmentConfig(node->parameters,id);
                require(node->kind=="set_variables" && b.producerId==id && b.revision==c.revision && b.configId==QString::fromLatin1(r["configId"].toString().toLatin1().toHex()) && b.cycleId==r["cycleId"].toString() && o["experimentalAcquisition"].toObject().isEmpty(),"Experimental assignment identity differs");require(b.fields.size()==c.rows.size(),"Experimental assignment field count differs");for(const auto& row:c.rows)require(b.fields.contains(row.targetId),"Experimental assignment target differs");continue;
            }
            if(valid){require(index.has_value() && bySource.contains(source) && o["experimentalAcquisition"].toObject()==bySource[source].experimentalAcquisition,"Experimental output lost provenance");if(o.contains("typedPort")){auto value=ToolResultCodec::decode(o["typedPort"].toObject());index->validatePort(source,value,*port);}else{const auto f=bySource[source];require(port->type==PortType::Image && o["sourceId"]==source && o["frameId"]==f.frameId && o["cycleId"]==f.cycleId && o["imageInfo"].isObject(),"Experimental image/frame differs");}}
        }
    }
    if(r["state"]=="DONE")require(seen.size()==configured.size(),"Experimental DONE missing tools");
}
MeasurementExecutionContext ExperimentalRuntime::calibration(const ProductionSnapshot& snapshot,const ImageFrame& frame){
    require(snapshot.validation().scope==scope,"Experimental metric snapshot required");const auto id=frame.metadata().sourceId;
    const auto entry=snapshot.dependencies()["sourceSet"].toObject()[id].toObject();require(!entry.isEmpty(),"Foreign experimental frame");
    const auto branch=SystemCalibration::branch(snapshot.recipe(),id);modelFrame(entry,branch,frame);
    if(entry.contains("experimentalCalibration"))return SystemCalibration::resolveExperimental(decodeModel(entry["experimentalCalibration"].toObject(),id),branch,frame);
    MeasurementExecutionContext raw;raw.rawSize=frame.size();raw.error="Raw/px source has no experimental model";return raw;
}
QString ExperimentalRuntime::sourceSummary(const QJsonObject& entry,const QString& source){
    QString text="Kaynak: "+source;
    const auto model=entry["experimentalCalibration"].toObject();
    QJsonObject camera=entry["camera"].toObject();
    if(!model.isEmpty())camera=model["acquisition"].toObject()["probe"].toObject()["experimental"].toObject()["camera"].toObject();
    if(camera.isEmpty() && !model.isEmpty())text+=" • Kamera: "+model["target"].toObject()["cameraId"].toString();
    else if(!camera.isEmpty())text+=" • Kamera: "+camera["id"].toString()+" / "+camera["name"].toString();
    if(model.isEmpty())return text+" • Raw / px • metrik model kullanılmıyor";
    const auto typed=decodeModel(model,source);const auto& active=typed.snapshot.calibration;
    return text+QString(" • mm • %1 v%2 • Model: %3 • Content: %4")
        .arg(active.record.name()).arg(active.selection.version).arg(active.selection.id,active.selection.contentId);
}
namespace {
QJsonArray recordedResults(const QJsonObject& r){
    QJsonArray results;if(r["state"]!="DONE")return results;
    for(const auto& v:r["tools"].toArray()){const auto t=v.toObject();if(t["state"]!="DONE")continue;
        const auto outputs=t["outputs"].toObject();for(auto i=outputs.begin();i!=outputs.end();++i){const auto o=i->toObject();if(!o.contains("typedPort"))continue;
            const auto port=ToolResultCodec::decode(o["typedPort"].toObject());if(!port.valid || !std::holds_alternative<double>(port.data))continue;
            results.append(QJsonObject{{"sourceId",port.origin.sourceId},{"toolId",t["toolId"]},{"portId",i.key()},{"value",std::get<double>(port.data)},{"unit",port.port.unit},{"coordinateFrame",port.port.frame},{"calibration",t["calibration"]}});
        }
    }return results;
}
QJsonArray recordedModels(const QJsonObject& r){QJsonArray models;const auto sources=r["dependencies"].toObject()["sourceSet"].toObject();for(auto i=sources.begin();i!=sources.end();++i){const auto entry=i->toObject(),model=entry["experimentalCalibration"].toObject();models.append(QJsonObject{{"sourceId",i.key()},{"experimental",true},{"unit",model.isEmpty()?"px":"mm"},{"model",model.isEmpty()?QJsonObject{}:model["executionDependency"].toObject()},{"description",ExperimentalRuntime::sourceSummary(entry,i.key())}});}return models;}
}
QString ExperimentalRuntime::summary(const QJsonObject& r){
    validateRecord(r);QStringList lines{r["state"].toString()+" • "+r["cycleId"].toString(),"Parça: Değerlendirilmedi • PLC / robot: Kullanılamıyor"};
    QMap<QString,QString> names;for(const auto& tool:editorTools(r["configuration"].toObject()["definition"].toObject()))names[tool.id]=tool.name;
    QStringList metric,pixels;const auto sources=r["dependencies"].toObject()["sourceSet"].toObject();
    for(const auto& result:recordedResults(r)){const auto value=result.toObject();const auto source=value["sourceId"].toString(),unit=value["unit"].toString();const auto text=QString("%1 / %2: %3 %4").arg(names.value(source,source),value["portId"].toString()).arg(value["value"].toDouble(),0,'g',12).arg(unit);if(unit=="mm")metric<<text;else if(!sources[source].toObject().contains("experimentalCalibration") && unit.startsWith("px"))pixels<<text;}
    lines+=metric+pixels;
    for(const auto& model:recordedModels(r))lines<<model.toObject()["description"].toString();
    if(!r["error"].toString().isEmpty())lines<<r["error"].toString();return lines.join('\n');
}
QByteArray ExperimentalRuntime::csv(const QVector<QJsonObject>& records){
    require(records.size()<=256,"CSV kayıt sınırı");QByteArray data="schema,cycleId,recipeId,executionState,productionAdmission,triggerAssociationVerified,communicationState,sourceModels,results,evidence\r\n";
    for(const auto& r:records){validateRecord(r);QStringList row;for(const auto& key:{"schema","cycleId","recipeId","state"})row<<cell(r[key].toString());row<<"false"<<"false"<<cell("BLOCKED_EXPERIMENTAL")<<cell(QString::fromUtf8(QJsonDocument(recordedModels(r)).toJson(QJsonDocument::Compact)))<<cell(QString::fromUtf8(QJsonDocument(recordedResults(r)).toJson(QJsonDocument::Compact)))<<cell(QString::fromUtf8(QJsonDocument(r).toJson(QJsonDocument::Compact)));data+=row.join(',').toUtf8()+"\r\n";}return data;
}
}
