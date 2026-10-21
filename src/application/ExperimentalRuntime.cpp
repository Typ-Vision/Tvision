#include "ExperimentalRuntime.hpp"
#include "ExperimentalOecsAcquisition.hpp"
#include "IndustrialExecution.hpp"
#include "EditorSession.hpp"
#include "ProductionResultCodec.hpp"
#include "MeasurementDiagnostic.hpp"
#include "SourceFrameIndex.hpp"
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
constexpr auto scope="experimental_runtime/1";
void require(bool value,const char* why){if(!value)throw ProductionException(ProductionError::InvalidCompletion,why);}
QByteArray hash(const QJsonObject& o){return QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();}
QJsonObject configuration(const Recipe& r,const QString& directory){return {{"recipeId",r.id},{"recipeName",r.name},{"recipeVersion",r.recipeVersion},{"applicationType",typeKey(r.applicationType)},{"definition",r.definition},{"directory",QDir(directory).absolutePath()}};}
void keys(const QJsonObject& o,const QStringList& expected){require(o.size()==expected.size(),"Experimental record fields missing/extra");for(auto i=o.begin();i!=o.end();++i)require(expected.contains(i.key()),"Unknown experimental field");}
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
std::shared_ptr<const ProductionSnapshot> ExperimentalRuntime::capture(const Recipe& recipe,const QString& directory) const {
    require(recipe.definition["editor"].toObject()["schemaVersion"]==9,"Experimental Runtime requires editor9");validateEditorDefinition(recipe.definition);
    const auto tools=editorTools(recipe.definition);const auto graph=validateSourceGraph(tools,editorConnections(recipe.definition));require(graph.valid() && !graph.requiredSources.isEmpty(),"Experimental source graph invalid");
    auto s=std::make_shared<ProductionSnapshot>();s->recipe_=recipe;s->directory_=QDir(directory).absolutePath();
    bool industrial=false;for(const auto& t:tools)if(graph.requiredSources.contains(t.id) && t.kind=="industrial_camera")industrial=true;
    MultiSourceDependencies supplied;if(industrial){require(bool(provider_),"Experimental source provider required");supplied=provider_(recipe);require(bool(supplied.industrial),"Experimental industrial composition required");require(supplied.industrial->leaseUse==CameraLeaseUse::Editor && supplied.industrial->admittedDevices.isEmpty(),"Experimental composition cannot contain production admission");s->industrial_=supplied.industrial;}
    QJsonObject sources;QSet<QString> cameras;
    for(const auto& source:graph.requiredSources){const auto branch=SystemCalibration::branch(recipe,source);require(!SystemCalibration::requestsCalibration(branch,false) && !branch.calibrationBinding && branch.calibrationReference.isEmpty(),"Experimental Runtime currently requires raw/px; experimental calibration provenance is pending");
        const auto t=SystemCalibration::source(branch);QJsonObject entry;
        if(t.kind=="industrial_camera"){
            const auto request=industrialExecutionRequest(*s->industrial_,t);require(!cameras.contains(request.camera.id.value),"Repeated physical experimental source");cameras.insert(request.camera.id.value);
            require(request.profile.requested.trigger==TriggerSettings{true,"FrameBurstStart","Software"},"Explicit FrameBurstStart profile required");
            require(request.camera.physical && request.camera.physical->vendor=="Opto Engineering Srl" && request.camera.physical->model=="COE-050-M-USB-050-IR-C" && !request.camera.physical->serial.isEmpty(),"Experimental camera identity unsupported");
            const auto physical=QString::fromLatin1(hash(request.camera.physical->toJson()));require(!cameras.contains(physical),"Repeated physical experimental device");cameras.insert(physical);
            entry={{"camera",request.camera.toJson()},{"setup",request.setup.toJson()},{"profile",request.profile.toJson()}};
        }else if(t.kind=="image_file"){
            const auto image=ImageFileService{}.load(QDir(directory).absoluteFilePath(t.parameters["path"].toString()),true);entry={{"contentId",QString::fromLatin1(image.contentId().toHex())},{"experimentalAcquisition",image.metadata().experimentalAcquisition}};
        }else require(false,"Experimental Runtime supports industrial camera / file sources");
        sources[source]=entry;
    }
    s->dependencies_={{"scope",scope},{"engine",ExecutionService::version()},{"sourceSet",sources}};
    s->validation_={QUuid::createUuid().toString(QUuid::WithoutBraces),identity_.principal,hash(configuration(recipe,directory)),hash(s->dependencies_),QDateTime::currentDateTimeUtc(),scope};return s;
}
void ExperimentalRuntime::prepare(const Recipe& r,const QString& directory){permission();if(!identity_.authorize(identity_.principal,ProductionAction::Validate) || !identity_.authorize(identity_.principal,ProductionAction::Activate))throw ProductionException(ProductionError::Unauthorized,"Experimental Runtime preparation denied");require(!busy(),"Experimental recording/cycle pending");auto candidate=capture(r,directory);snapshot_=std::move(candidate);faulted_=false;}
void ExperimentalRuntime::requireReady() const {permission();if(busy())throw ProductionException(ProductionError::Busy,"Experimental cycle/recording pending");require(snapshot_ && !faulted_,"Experimental Runtime not ready / busy / manual Run required after fault or cancellation");const auto next=capture(snapshot_->recipe(),snapshot_->projectDirectory());require(next->validation().contentId==snapshot_->validation().contentId && next->validation().dependencyId==snapshot_->validation().dependencyId,"Experimental source/profile/model changed; Run again");}
ProductionCycle ExperimentalRuntime::begin(const QJsonObject& communication){requireReady();require(communication.isEmpty(),"Experimental Runtime forbids PLC/robot communication");require(serial_!=std::numeric_limits<quint64>::max(),"Experimental ticket exhausted");cycle_={++serial_,snapshot_};started_=QDateTime::currentDateTimeUtc();return cycle_;}
bool ExperimentalRuntime::accepts(const ProductionCycle& c){return c.snapshot && c.snapshot->validation().scope==scope;}
QByteArray ExperimentalRuntime::digest(const CycleResult& r){return ProductionRecipeService::completionDigest(r);}
void ExperimentalRuntime::completion(const ProductionCycle& c,const CycleResult& r){require(accepts(c) && c.ticket && r.experimentalSnapshot_==c.snapshot && r.experimentalTicket_==c.ticket && !r.experimentalDigest_.isEmpty() && r.experimentalDigest_==digest(r) && r.completionDigest_.isEmpty() && !r.completionSnapshot_ && r.industrialEvidence.isEmpty(),"Missing/changed/foreign experimental completion");}
CycleResult ExperimentalRuntime::execute(const ProductionCycle& c,const std::shared_ptr<std::atomic_bool>& cancel){
    require(accepts(c) && c.ticket,"Experimental lease required");const auto s=c.snapshot;
    auto service=s->industrial() ? ExecutionService(*s->industrial()):ExecutionService{};
    auto result=service.withExperimentalEditorReplay().run(s->recipe().definition,s->projectDirectory(),s->recipe().id,s->validation().contentId,cancel);
    if(result.state==ExecutionState::Error && result.error.isEmpty())for(const auto& t:result.tools)if(t.state==ExecutionState::Error){if(!result.error.isEmpty())result.error+='\n';result.error+=t.errorCode+" "+t.message;}
    result.experimentalSnapshot_=s;result.experimentalTicket_=c.ticket;result.experimentalDigest_=digest(result);return result;
}
void ExperimentalRuntime::suppressCancelled(const ProductionCycle& c,CycleResult& r){completion(c,r);EditorSession::suppressCancelled(r);r.experimentalDigest_=digest(r);}
void ExperimentalRuntime::finish(quint64 ticket,const CycleResult& r){
    permission();require(ticket && ticket==cycle_.ticket,"Foreign experimental ticket");completion(cycle_,r);
    if(pending_.isEmpty()){
        require(!r.partial && r.recipeId==snapshot_->recipe().id && r.configId==snapshot_->validation().contentId,"Experimental configuration/completion differs");
        QJsonArray frames,tools;
        for(const auto& f:r.frames){QByteArray content;for(const auto& t:r.tools)if(t.toolId==f.sourceId)for(const auto& v:t.outputs)if(v.valid && std::holds_alternative<ImageFrame>(v.data))content=std::get<ImageFrame>(v.data).contentId();require(!content.isEmpty(),"Experimental frame missing raw raster");frames.append(QJsonObject{{"sourceId",f.sourceId},{"frameId",f.frameId},{"cycleId",f.cycleId},{"coordinateSystemId",f.coordinateSystemId},{"completedAtUtc",f.completedAtUtc.toString(Qt::ISODateWithMs)},{"experimentalAcquisition",f.experimentalAcquisition},{"contentId",QString::fromLatin1(content.toHex())}});}
        for(const auto& t:r.tools){require(t.decision==PartDecision::NotEvaluated,"Experimental raw Runtime cannot publish a part decision");QJsonObject outputs;for(auto i=t.outputs.begin();i!=t.outputs.end();++i){auto o=ProductionResultCodec::output(*i);o["experimentalAcquisition"]=i->origin.experimentalAcquisition;outputs[i.key()]=o;}
            QJsonObject o{{"toolId",t.toolId},{"state",executionStateName(t.state)},{"message",t.message},{"errorCode",t.errorCode},{"decision","NOT_EVALUATED"},{"outputs",outputs}};if(t.fitDiagnostic)o["measurementDiagnostic"]=encodeMeasurementDiagnostic(*t.fitDiagnostic);tools.append(o);
        }
        const auto& recipe=snapshot_->recipe();QJsonObject candidate{{"schema","visionflow.experimental-runtime1"},{"schemaVersion",1},{"recordId",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"principal",identity_.principal},{"recipeId",recipe.id},{"recipeName",recipe.name},{"recipeVersion",recipe.recipeVersion},{"applicationType",typeKey(recipe.applicationType)},{"configuration",configuration(recipe,snapshot_->projectDirectory())},{"configId",QString::fromLatin1(snapshot_->validation().contentId)},{"dependencies",snapshot_->dependencies()},{"dependencyId",QString::fromLatin1(snapshot_->validation().dependencyId)},{"cycleId",r.cycleId},{"evaluationId",r.evaluationId},{"state",executionStateName(r.state)},{"error",r.error},{"startedAtUtc",started_.toString(Qt::ISODateWithMs)},{"completedAtUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},{"frames",frames},{"tools",tools},{"productionAdmission",false},{"triggerAssociationVerified",false},{"communicationState","BLOCKED_EXPERIMENTAL"}};
        validateRecord(candidate);pending_=std::move(candidate);pendingDigest_=r.experimentalDigest_;
    }
    require(r.experimentalDigest_==pendingDigest_,"Pending experimental completion cannot change");
    try{persist_("experimental_cycles",pending_["recordId"].toString(),pending_);}catch(const std::exception& e){throw ProductionException(ProductionError::PersistenceFailure,QString::fromUtf8(e.what()));}
    pending_={};pendingDigest_.clear();cycle_={};faulted_=r.state!=ExecutionState::Done;
}
void ExperimentalRuntime::validateRecord(const QJsonObject& r){
    require(QJsonDocument(r).toJson(QJsonDocument::Compact).size()<=ProductionResultCodec::maxRecordBytes,"Experimental record exceeds budget");
    keys(r,{"schema","schemaVersion","recordId","principal","recipeId","recipeName","recipeVersion","applicationType","configuration","configId","dependencies","dependencyId","cycleId","evaluationId","state","error","startedAtUtc","completedAtUtc","frames","tools","productionAdmission","triggerAssociationVerified","communicationState"});
    require(r["schema"]=="visionflow.experimental-runtime1" && r["schemaVersion"]==1 && r["productionAdmission"]==false && r["triggerAssociationVerified"]==false && r["communicationState"]=="BLOCKED_EXPERIMENTAL","Experimental envelope/admission invalid");
    require(!QUuid(r["recordId"].toString()).isNull() && !r["cycleId"].toString().isEmpty() && !r["principal"].toString().isEmpty() && QDateTime::fromString(r["startedAtUtc"].toString(),Qt::ISODateWithMs).isValid() && QDateTime::fromString(r["completedAtUtc"].toString(),Qt::ISODateWithMs).isValid(),"Experimental identity/time missing");
    const auto config=r["configuration"].toObject(),deps=r["dependencies"].toObject();
    keys(config,{"recipeId","recipeName","recipeVersion","applicationType","definition","directory"});keys(deps,{"scope","engine","sourceSet"});
    require(config["definition"].toObject()["editor"].toObject()["schemaVersion"]==9 && QStringList{"measurement","defect_inspection","robot_guidance"}.contains(config["applicationType"].toString()),"Experimental configuration type/schema invalid");
    require(r["configId"].toString().toLatin1()==hash(config) && r["dependencyId"].toString().toLatin1()==hash(deps) && deps["scope"]==scope && config["recipeId"]==r["recipeId"] && config["recipeName"]==r["recipeName"] && config["recipeVersion"]==r["recipeVersion"] && config["applicationType"]==r["applicationType"],"Experimental immutable configuration hash differs");
    const auto definition=config["definition"].toObject();validateEditorDefinition(definition);const auto configured=editorTools(definition);const auto graph=validateSourceGraph(configured,editorConnections(definition));
    require(graph.valid() && !graph.requiredSources.isEmpty() && deps["sourceSet"].toObject().size()==graph.requiredSources.size() && r["frames"].isArray() && r["tools"].isArray() && r["tools"].toArray().size()<=256 && QStringList{"DONE","ERROR","SKIPPED"}.contains(r["state"].toString()),"Experimental graph/state invalid");
    QMap<QString,QString> contentIds;QMap<QString,FrameMetadata> bySource;QVector<FrameMetadata> frames;QStringList acquired;
    for(const auto& v:r["frames"].toArray()){const auto f=v.toObject();keys(f,{"sourceId","frameId","cycleId","coordinateSystemId","completedAtUtc","experimentalAcquisition","contentId"});FrameMetadata m;m.sourceId=f["sourceId"].toString();m.frameId=f["frameId"].toString();m.cycleId=f["cycleId"].toString();m.coordinateSystemId=f["coordinateSystemId"].toString();m.completedAtUtc=QDateTime::fromString(f["completedAtUtc"].toString(),Qt::ISODateWithMs);m.experimentalAcquisition=f["experimentalAcquisition"].toObject();require(graph.requiredSources.contains(m.sourceId) && !bySource.contains(m.sourceId) && m.completedAtUtc.isValid(),"Experimental duplicate/foreign frame");if(!m.experimentalAcquisition.isEmpty())provenance(m.experimentalAcquisition);require(f["contentId"].toString().size()==64,"Experimental raster hash missing");contentIds[m.sourceId]=f["contentId"].toString();bySource[m.sourceId]=m;frames.append(m);acquired.append(m.sourceId);}
    if(r["state"]=="DONE")require(acquired.size()==graph.requiredSources.size(),"Experimental DONE missing source");
    if(r["state"]=="SKIPPED")require(frames.isEmpty(),"Experimental Stop retained frames");
    const auto sources=deps["sourceSet"].toObject();
    for(const auto& source:graph.requiredSources){require(sources.contains(source),"Experimental admission missing source");const auto entry=sources[source].toObject();const auto node=std::find_if(configured.begin(),configured.end(),[&](const auto& t){return t.id==source;});require(node!=configured.end(),"Experimental source missing");
        if(node->kind=="industrial_camera"){
            keys(entry,{"camera","setup","profile"});const auto camera=MeasurementCamera::fromJson(entry["camera"].toObject());const auto setup=MeasurementSetupDescriptor::fromJson(entry["setup"].toObject());const auto profile=AcquisitionProfile::fromJson(entry["profile"].toObject());const auto ref=IndustrialSourceReference::fromJson(node->parameters["sourceReference"].toObject());ref.validateAgainst({{camera},{setup},{profile}});
            if(bySource.contains(source)){const auto p=bySource[source].experimentalAcquisition;provenance(p);require(p["contentId"]==contentIds[source],"Experimental raster/provenance hash differs");require(camera.physical && p["vendor"]==camera.physical->vendor && p["model"]==camera.physical->model && p["serial"]==camera.physical->serial && p["profileSnapshot"]==entry["profile"],"Experimental acquired identity/profile differs from admission");}
        }else{require(node->kind=="image_file","Experimental source kind invalid");keys(entry,{"contentId","experimentalAcquisition"});if(bySource.contains(source))require(contentIds[source]==entry["contentId"] && bySource[source].experimentalAcquisition==entry["experimentalAcquisition"].toObject(),"Experimental replay provenance differs from frozen admission");}
    }
    std::optional<SourceFrameIndex> index;if(!frames.isEmpty())index.emplace(r["cycleId"].toString(),acquired,frames);QSet<QString> seen;
    for(const auto& v:r["tools"].toArray()){const auto t=v.toObject();QStringList fields{"toolId","state","message","errorCode","decision","outputs"};if(t.contains("measurementDiagnostic"))fields<<"measurementDiagnostic";keys(t,fields);const auto id=t["toolId"].toString();auto node=std::find_if(configured.begin(),configured.end(),[&](const auto& x){return x.id==id;});require(node!=configured.end() && !seen.contains(id) && t["decision"]=="NOT_EVALUATED" && t["outputs"].isObject(),"Experimental foreign tool/decision");seen.insert(id);
        if(t.contains("measurementDiagnostic")){validateMeasurementDiagnostic(t,r);require(decodeMeasurementDiagnostic(t["measurementDiagnostic"].toObject()).sourceId==graph.toolSources.value(id),"Experimental diagnostic source differs");}
        require(QStringList{"DONE","ERROR","SKIPPED"}.contains(t["state"].toString()),"Experimental tool state invalid");
        const auto outputs=t["outputs"].toObject();if(r["state"]=="SKIPPED" || !graph.requiredSources.contains(graph.toolSources.value(id)))require(t["state"]=="SKIPPED" && outputs.isEmpty() && !t.contains("measurementDiagnostic"),"Experimental stopped/disabled tool retained output");
        const auto source=graph.toolSources.value(id);for(auto p=outputs.begin();p!=outputs.end();++p){const auto o=p->toObject();const auto ports=outputPorts(node->kind);const auto port=std::find_if(ports.begin(),ports.end(),[&](const auto& x){return x.id==p.key();});require(port!=ports.end(),"Undeclared experimental output");
            const bool valid=o.contains("typedPort") ? o["typedPort"].toObject()["valid"].toBool():!o["imageInfo"].isNull();
            if(o.contains("typedPort")){const auto value=ToolResultCodec::decode(o["typedPort"].toObject());if(value.valid && !value.evaluationId.isEmpty())require(value.evaluationId==r["evaluationId"].toString(),"Experimental output evaluation differs");auto expected=ProductionResultCodec::output(value);expected["experimentalAcquisition"]=o["experimentalAcquisition"];require(o==expected,"Experimental outer/typed output differs");require(value.port.id==port->id && value.port.type==port->type && value.port.unit==port->unit && value.port.frame==port->frame,"Experimental port declaration differs");}
            else{keys(o,{"type","unit","coordinateFrame","frameId","cycleId","sourceId","imageInfo","experimentalAcquisition"});require(port->type==PortType::Image && o["type"]=="Image" && o["unit"]==port->unit && o["coordinateFrame"]==port->frame,"Experimental image declaration differs");if(valid){const auto info=o["imageInfo"].toObject();require(info["width"].toInt()>0 && info["height"].toInt()>0 && !info["pixelFormat"].toString().isEmpty(),"Experimental image geometry missing");}}
            if(valid){require(index.has_value() && bySource.contains(source) && o["experimentalAcquisition"].toObject()==bySource[source].experimentalAcquisition,"Experimental output lost provenance");if(o.contains("typedPort")){auto value=ToolResultCodec::decode(o["typedPort"].toObject());index->validatePort(source,value,*port);}else{const auto f=bySource[source];require(port->type==PortType::Image && o["sourceId"]==source && o["frameId"]==f.frameId && o["cycleId"]==f.cycleId && o["imageInfo"].isObject(),"Experimental image/frame differs");}}
        }
    }
    if(r["state"]=="DONE")require(seen.size()==configured.size(),"Experimental DONE missing tools");
}
QString ExperimentalRuntime::summary(const QJsonObject& r){validateRecord(r);return ExperimentalOecsAcquisition::notice()+"\n"+r["state"].toString()+" • "+r["cycleId"].toString()+"\nPLC / robot: BLOCKED_EXPERIMENTAL";}
QByteArray ExperimentalRuntime::csv(const QVector<QJsonObject>& records){
    require(records.size()<=256,"Experimental CSV record limit");QByteArray data="schema,cycleId,recipeId,executionState,productionAdmission,triggerAssociationVerified,communicationState,evidence\r\n";
    for(const auto& r:records){validateRecord(r);QStringList row;for(const auto& key:{"schema","cycleId","recipeId","state"})row<<cell(r[key].toString());row<<"false"<<"false"<<cell("BLOCKED_EXPERIMENTAL")<<cell(QString::fromUtf8(QJsonDocument(r).toJson(QJsonDocument::Compact)));data+=row.join(',').toUtf8()+"\r\n";}return data;
}
}
