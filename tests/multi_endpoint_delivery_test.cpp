#include "application/CommunicationTcpEndpoint.hpp"
#include "application/AssignmentEvaluator.hpp"
#include "application/ProjectService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <QTextStream>
#include <QUuid>
#include <cmath>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
template<class F>void rejects(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
template<class F>void until(F f,const char* why){QElapsedTimer t;t.start();while(!f() && t.elapsed()<10000){QCoreApplication::processEvents();QThread::msleep(1);}check(f(),why);}
struct Peer {
    QTcpSocket socket;QByteArray buffer;QVector<QJsonObject> messages;QString session;
    Peer(){QObject::connect(&socket,&QTcpSocket::readyRead,[this]{buffer+=socket.readAll();for(;;){auto n=buffer.indexOf('\n');if(n<0)break;const auto message=CommunicationWireCodec::decode(buffer.left(n));QTextStream(stdout)<<"RX "<<QJsonDocument(message).toJson(QJsonDocument::Compact)<<'\n';messages.append(message);buffer.remove(0,n+1);}});}
    QJsonObject take(const QString& type){QJsonObject found;until([&]{if(!found.isEmpty())return true;for(int i=0;i<messages.size();++i)if(messages[i]["type"]==type){found=messages.takeAt(i);return true;}return false;},qPrintable("Expected "+type));return found;}
    void connectTo(quint16 port){socket.connectToHost(QHostAddress::LocalHost,port);session=take("HELLO")["sessionId"].toString();}
    void send(QJsonObject o){o["v"]=3;o["sessionId"]=session;socket.write(CommunicationWireCodec::encode(o));}
    void ack(const QJsonObject& o){send({{"type","ACK"},{"requestId",o["requestId"]},{"resultId",o["resultId"]},{"payloadHash",o["payloadHash"]}});take("ACKED");}
    void disconnect(){socket.abort();until([&]{return socket.state()==QAbstractSocket::UnconnectedState;},"peer disconnect");QCoreApplication::processEvents();}
};
SpatialAssignmentValue pose(){
    SpatialAssignmentValue p;p.validity=ValueValidity::Valid;
    FrameRef from{"simulator","","group",uuid(),2},to=from;to.frameUuid=uuid();to.revision=4;
    p.value=SpatialTransform{from,to,{12.25,-23.5,34.75},{std::sqrt(.5),0,0,std::sqrt(.5)}};return p;
}
void recoverProcess(const QString& path){
    QFile file(path);check(file.open(QIODevice::ReadOnly),"recovery fixture opens");const auto o=QJsonDocument::fromJson(file.readAll()).object();
    const auto archive=o["archive"].toString();RuntimeService runtime(archive,o["catalog"].toString(),RuntimeIdentity::localEngineer());RuntimeCoordinator coordinator(runtime);InputSnapshotStore inputs;DeliveryStore ledger(archive);TriggerRouter router(coordinator,runtime,inputs,ledger,[]{return QString{};});
    CommunicationTcpEndpoint a(runtime,coordinator,router,inputs,ledger,archive,o["catalog"].toString()),b(runtime,coordinator,router,inputs,ledger,archive,o["catalog"].toString());
    auto options=[&](const QString& key){const auto v=o[key].toObject();CommunicationTcpOptions p;p.workcell="cell";p.project=o["project"].toString();p.endpoint=v["endpoint"].toString();p.peer=v["peer"].toString();p.outputGroupId=v["group"].toString();p.port=quint16(v["port"].toInt());p.required=v["required"].toBool();return p;};
    const auto oa=options("a"),ob=options("b");a.listen(oa);b.listen(ob);a.setPublicationTargets({&a,&b});
    check(coordinator.state()==RuntimeState::Unloaded && !runtime.busy(),"fresh process never loads or starts production");
    TriggerRequest ar{"cell",oa.project,oa.endpoint,oa.peer,"one",o["recipe"].toString(),o["config"].toString(),"pose-input",7,1};auto br=ar;br.endpoint=ob.endpoint;br.peer=ob.peer;br.requestId=o["cycle"].toString();
    const auto da=TriggerRouter::deliveryRequest(ar),db=TriggerRouter::deliveryRequest(br);
    check(ledger.inspect(da).state=="ACK_COMMITTED" && ledger.inspect(db).payload==QByteArray::fromBase64(o["payload"].toString().toLatin1()),"new process reconstructs independent states and bytes");
    Peer peer;peer.connectTo(b.port());const auto result=peer.take("RESULT");check(CommunicationWireCodec::resultPayload(result)==ledger.inspect(db).payload && result["cycleId"]==o["cycle"],"new process replays only pending target exact bytes");peer.ack(result);
    check(ledger.inspect(da).state=="ACK_COMMITTED" && ledger.inspect(db).state=="ACK_COMMITTED" && !coordinator.deliveryPending(),"independent target ACKs survive process boundary");
    check(ProductionStore(archive).all("cycles").size()==1 && coordinator.state()==RuntimeState::Unloaded,"recovery produces no second cycle");
}
void scenario(bool optional,ApplicationType applicationType){
    QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/multi-delivery-XXXXXX");check(dir.isValid(),"fixture");
    ProjectService project;project.createProject("COMM-36 simulator","",dir.filePath("p.vfproject"),"");project.createRecipe("Pose transport fixture","",applicationType);
    QImage image(16,16,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("part.png")),"fixture image");project.setImageFile(dir.filePath("part.png"));
    const auto inputPose=pose();VariableDefinition variable;variable.name="WholePose";variable.type=VariableType::SpatialTransform;variable.unit=ScalarUnit::Millimetre;variable.frameConstraint=VariableFrameConstraint{inputPose.value->source,inputPose.value->target};variable.id=project.addVariable(variable);
    const auto producer=project.addTool("set_variables");
    // Explicit simulation constant. Normal Runtime has no real pose producer;
    // the wire-input -> evaluator path is exercised separately below.
    for(auto t:project.tools())if(t.id==producer){t.parameters=encodeAssignmentConfig({producer,1,{{variable.id,ConstantAssignmentSource{inputPose}}}});project.updateTool(t);}
    EndpointProfile a,b;a.id=uuid();a.enabled=true;a.role="server";a.fieldMapping={{"pose","whole-pose"}};b=a;b.id=uuid();
    OutputGroup ga{uuid(),a.id,1,true,"vf.fields/1",producer,1,{{"pose",variable.id}}},gb=ga;gb.id=uuid();gb.profileId=b.id;gb.required=!optional;
    project.setOutputGroups({ga,gb});const auto publisher=project.addTool("publish_data");for(auto t:project.tools())if(t.id==publisher){t.enabled=true;t.parameters=encodePublishConfig({{ga.id,gb.id}});project.updateTool(t);}project.enableMultiSource();project.save();
    CommunicationProfiles profiles(dir.filePath("profiles.json"));profiles.save({{a.id,a},{b.id,b}});
    const auto archive=dir.filePath("archive"),catalog=dir.filePath("catalog.json");
    RuntimeService runtime(archive,catalog,RuntimeIdentity::localEngineer(),{},{},{},[&](const Recipe& r){MultiSourceDependencies d;for(const auto& t:editorTools(r.definition))if(t.enabled && t.kind=="image_file")d.sources[t.id]={};d.publication=PublicationService::capture(r.definition,profiles);return d;});
    RuntimeCoordinator coordinator(runtime);InputSnapshotStore inputs;bool failSecondCommit=optional;int commitCount=0;
    DeliveryStore ledger(archive,[&](const QString& boundary){if(boundary=="DELIVERY_PENDING" && ++commitCount==2 && failSecondCommit)throw std::runtime_error("second target disk failure");});TriggerRouter router(coordinator,runtime,inputs,ledger,[]{return QString{};});
    auto first=std::make_unique<CommunicationTcpEndpoint>(runtime,coordinator,router,inputs,ledger,archive,catalog);
    auto second=std::make_unique<CommunicationTcpEndpoint>(runtime,coordinator,router,inputs,ledger,archive,catalog);
    CommunicationTcpOptions oa;oa.workcell="cell";oa.project=project.project().id;oa.endpoint=a.id;oa.peer="sim-A-generation-1";oa.outputGroupId=ga.id;oa.ackTimeoutMs=100;oa.retryCount=1;
    oa.inputs={{"pose",{inputPose,60000}}};auto ob=oa;ob.inputs.clear();ob.endpoint=b.id;ob.peer="sim-B-generation-1";ob.outputGroupId=gb.id;ob.required=!optional;
    first->listen(oa);second->listen(ob);a.port=first->port();b.port=second->port();a.revision=b.revision=2;profiles.save({{a.id,a},{b.id,b}});
    first->setPublicationTargets({first.get(),second.get()});
    coordinator.load(*project.activeRecipe(),dir.path(),RuntimeTrigger::SoftwareTrigger);coordinator.start();
    Peer pa,pb;pa.connectTo(first->port());
    const auto config=QString::fromLatin1(runtime.active()->validation().contentId),recipe=project.activeRecipe()->id;
    pa.send({{"type","INPUT"},{"snapshotId","pose-input"},{"revision","1"},{"recipeId",recipe},{"configId",config},{"fields",QJsonArray{CommunicationWireCodec::encodeField("pose",inputPose)}},{"sourceTime","simulation-time"},{"clockId","simulator"}});pa.take("STATUS");
    auto trigger=[&](QString id){return QJsonObject{{"type","TRIGGER"},{"requestId",id},{"sequence","7"},{"recipeId",recipe},{"configId",config},{"inputSnapshotId","pose-input"},{"inputRevision","1"}};};
    int cycles=0,acquisitions=0;first->completed=[&](const CycleResult& r){++cycles;acquisitions+=r.acquisitions;};
    if(!optional){pa.send(trigger("offline"));check(pa.take("INVALID")["reason"]=="ENDPOINT_OFFLINE","required offline preflight before any acquisition");check(ledger.recovery().isEmpty() && cycles==0,"offline creates no journal or result");pb.connectTo(second->port());}
    pa.send(trigger("one"));const auto accepted=pa.take("ACCEPTED");
    QJsonObject preAcked;
    if(optional){
        until([&]{return first->recordingPending() && !runtime.busy();},"second target recording fault retained");check(runtime.records().size()==1 && cycles==0,"partial target commit does not repeat or claim completed execution");
        pa.send(trigger("one"));preAcked=pa.take("RESULT");pa.ack(preAcked);
        pa.send(trigger("still-recording"));check(pa.take("INVALID")["reason"]=="RECORDING_PENDING" && first->recordingPending(),"ACK of committed target cannot clear another target recording fault");
        failSecondCommit=false;first->retryRecording();
    }
    const auto ra=preAcked.isEmpty()?pa.take("RESULT"):preAcked;check(ra["cycleId"]==accepted["cycleId"],"recording retry retains admitted cycle");if(preAcked.isEmpty())pa.ack(ra);
    const auto cycle=ra["cycleId"].toString();TriggerRequest ar{oa.workcell,oa.project,oa.endpoint,oa.peer,"one",recipe,config,"pose-input",7,1},br=ar;br.endpoint=ob.endpoint;br.peer=ob.peer;br.requestId=cycle;
    const auto da=TriggerRouter::deliveryRequest(ar),db=TriggerRouter::deliveryRequest(br);
    check(ledger.inspect(da).state=="ACK_COMMITTED" && ledger.inspect(db).dispatchable(),"independent ACK and pending states");
    check(coordinator.deliveryPending()==!optional,"one endpoint ACK never clears other required barrier");
    if(optional)check(first->state()=="READY","ACK before second target recording retry still releases required source after commit");
    check(ra["decision"]=="NOT_EVALUATED" && ra["resultValid"]==true,"valid Pose does not manufacture a part decision");
    const auto decoded=std::get<SpatialAssignmentValue>(CommunicationWireCodec::decodeField(ra["fields"].toArray().first().toObject()));
    check(decoded.value->toJson()==inputPose.value->toJson() && decoded.unit==ScalarUnit::Millimetre,"nonzero mm/quaternion/frame/revision wire roundtrip");
    check(cycles==1 && acquisitions==1 && runtime.records().size()==1,"two destinations share exactly one committed cycle");
    if(!optional){auto rb=pb.take("RESULT");check(rb["resultId"]==ra["resultId"] && rb["cycleId"]==ra["cycleId"] && rb["fields"]==ra["fields"],"two actual TCP peers get same bundle and whole Pose");pb.take("RESULT");until([&]{return second->state()=="ACK_TIMEOUT";},"required bounded retry");pa.send(trigger("blocked"));check(pa.take("INVALID")["reason"]=="DELIVERY_PENDING","required pending blocks next cycle");pb.disconnect();}
    const auto before=ledger.inspect(db).payload;
    // Recreate both adapters and the ledger reader: no execution/reacquisition.
    coordinator.stop();pa.disconnect();second.reset();first.reset();
    DeliveryStore restarted(archive);check(restarted.inspect(da).state=="ACK_COMMITTED" && restarted.inspect(db).payload==before,"restart preserves each target ACK and pending bytes");
    auto targetJson=[](const CommunicationTcpOptions& p,int port){return QJsonObject{{"endpoint",p.endpoint},{"peer",p.peer},{"group",p.outputGroupId},{"port",port},{"required",p.required}};};
    QFile fixture(dir.filePath("recover.json"));check(fixture.open(QIODevice::WriteOnly),"recovery fixture");fixture.write(QJsonDocument(QJsonObject{{"archive",archive},{"catalog",catalog},{"project",project.project().id},{"recipe",recipe},{"config",config},{"cycle",cycle},{"payload",QString::fromLatin1(before.toBase64())},{"a",targetJson(oa,a.port)},{"b",targetJson(ob,b.port)}}).toJson());fixture.close();
    QProcess child;child.start(QCoreApplication::applicationFilePath(),{"--recover",fixture.fileName()});check(child.waitForFinished(30000),"separate recovery process finished");QTextStream(stdout)<<child.readAllStandardOutput();if(child.exitCode()!=0)QTextStream(stderr)<<child.readAllStandardError();check(child.exitStatus()==QProcess::NormalExit && child.exitCode()==0,"actual process restart independent delivery ACK acceptance");
    check(ledger.inspect(da).state=="ACK_COMMITTED" && ledger.inspect(db).state=="ACK_COMMITTED" && runtime.records().size()==1,"parent reads durable independent child ACKs");
    // Typed INPUT -> immutable snapshot -> whole variable -> logical projection.
    auto wire=inputPose;wire.producerId="simulator";wire.producerPort="pose";wire.cycleId=cycle;wire.configId=QString::fromLatin1(config.toLatin1().toHex());
    auto fromWire=CommunicationWireCodec::decodeField(CommunicationWireCodec::encodeField("pose",wire));
    InputSnapshot snapshot{oa.endpoint,oa.peer,"typed-pose",recipe,config,"sim-time","sim-clock",2,{{"pose",fromWire}}};inputs.put(snapshot);
    const auto frozen=inputs.get(oa.endpoint,oa.peer,"typed-pose",2);
    AssignmentInputView view;view.cycleId=cycle;view.configId=wire.configId;view.variables={variable};view.toolOutputs["simulator"]["pose"]={1,frozen->fields["pose"]};
    const AssignmentConfig assignment{producer,1,{{variable.id,ToolOutputAssignmentSource{"simulator","pose",1}}}};
    const auto assigned=AssignmentEvaluator::evaluate(assignment,view);check(assigned.bundle.has_value(),"wire pose assigned as one coherent variable");
    const auto publication=runtime.active()->publication();const auto definition=project.activeRecipe()->definition;
    auto publish=[&](const AssignmentBundle& bundle,const PublicationSnapshot& s){return PublicationService::evaluate({{ga.id,gb.id}},definition,{{producer,bundle}},cycle,wire.configId,s);};
    const auto intents=publish(*assigned.bundle,*publication);check(intents.size()==2 && std::get<SpatialAssignmentValue>(intents[0].fields["pose"]).value->toJson()==inputPose.value->toJson(),"input -> typed variable -> two logical whole-record mappings");
    auto bad=*assigned.bundle;auto value=std::get<SpatialAssignmentValue>(bad.fields[variable.id]);value.cycleId=uuid();bad.fields[variable.id]=value;rejects([&]{publish(bad,*publication);},"mixed pose cycle rejected");
    bad=*assigned.bundle;bad.configId="other";rejects([&]{publish(bad,*publication);},"mixed config rejected");bad=*assigned.bundle;bad.revision=2;rejects([&]{publish(bad,*publication);},"mixed assignment revision rejected");
    bad=*assigned.bundle;value=std::get<SpatialAssignmentValue>(bad.fields[variable.id]);value.value->source.revision++;bad.fields[variable.id]=value;rejects([&]{publish(bad,*publication);},"frame revision mismatch rejected");
    bad=*assigned.bundle;value=std::get<SpatialAssignmentValue>(bad.fields[variable.id]);value.value.reset();value.validity=ValueValidity::Missing;value.reason="no pose";bad.fields[variable.id]=value;rejects([&]{publish(bad,*publication);},"missing pose never becomes identity or zero");
    auto changed=*publication;changed.profiles[a.id].revision++;rejects([&]{publish(*assigned.bundle,changed);},"mapping/profile revision coherence checked at evaluation");
    // Same endpoint backlog bound remains enforced even while its peer is offline.
    if(optional){for(int i=0;i<128;++i){auto q=db;q.requestId="queued-"+QString::number(i);ledger.request(q,false,false);}auto overflow=db;overflow.requestId="overflow";check(ledger.admission(overflow,false,false)=="QUEUE_FULL","optional offline backlog bounded at 128");rejects([&]{ledger.request(overflow,false,false);},"optional full queue refuses silent eviction");check(ledger.inspect(da).state=="ACK_COMMITTED","optional queue does not rewrite required ACK");}
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{if(app.arguments().size()==3 && app.arguments()[1]=="--recover"){recoverProcess(app.arguments()[2]);return 0;}for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){scenario(false,type);scenario(true,type);}QTextStream(stdout)<<checks<<" multi-endpoint delivery and pose checks passed\n";return 0;}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
