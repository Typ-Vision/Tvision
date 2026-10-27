#include "application/CommunicationTcpEndpoint.hpp"
#include "application/PlcRuntimeSocket.hpp"
#include "application/ProjectService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QImage>
#include <QThread>
#include <QTextStream>
#include <QLockFile>
#include <QUuid>
#include <QProcess>
#include <QFile>
using namespace visionflow;
namespace {
int checks=0;void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
template<class F>void until(F f,const char* why,int ms=10000){QElapsedTimer t;t.start();while(!f() && t.elapsed()<ms){QCoreApplication::processEvents();QThread::msleep(1);}check(f(),why);}
struct Peer {
 QTcpSocket socket;QByteArray bytes;QVector<QJsonObject> messages;QString session;
 Peer(){QObject::connect(&socket,&QTcpSocket::readyRead,[this]{bytes+=socket.readAll();for(;;){auto n=bytes.indexOf('\n');if(n<0)break;QTextStream(stdout)<<"RX "<<bytes.left(n)<<'\n';messages.append(CommunicationWireCodec::decode(bytes.left(n)));bytes.remove(0,n+1);}});}
 QJsonObject take(QString type,QString requestId={}){QJsonObject o;until([&]{if(!o.isEmpty())return true;for(int i=0;i<messages.size();++i)if(messages[i]["type"]==type && (requestId.isEmpty() || messages[i]["requestId"]==requestId)){o=messages.takeAt(i);return true;}return false;},qPrintable("Expected "+type));return o;}
 void connectTo(quint16 port){messages.clear();socket.connectToHost(QHostAddress::LocalHost,port);session=take("HELLO")["sessionId"].toString();}
 void send(QJsonObject o){o["v"]=3;o["sessionId"]=session;socket.write(CommunicationWireCodec::encode(o));}
 void ack(QJsonObject o){send({{"type","ACK"},{"requestId",o["requestId"]},{"resultId",o["resultId"]},{"payloadHash",o["payloadHash"]}});}
 void disconnect(){socket.abort();QElapsedTimer t;t.start();while(t.elapsed()<20)QCoreApplication::processEvents();}
};
}
#include "tcp_measurement_acceptance.hpp"
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 if(app.arguments().contains("--measurement"))return measurementAcceptance();
 QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/tcp3-XXXXXX");check(dir.isValid(),"fixture");
 QImage image(30,30,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("part.png")),"actual image");
 ProjectService p;p.createProject("TCP3","",dir.filePath("p.vfproject"),"");p.createRecipe("Part","",ApplicationType::DefectInspection);p.setImageFile(dir.filePath("part.png"));const auto blob=p.addTool("blob"),decision=p.addTool("result");
 for(auto t:p.tools())if(t.id==decision){t.enabled=true;t.parameters=encodeResultConfig({ResultOperator::All,{{blob,"found",4}}});p.updateTool(t);}
 VariableDefinition variable{uuid(),"Count"};variable.type=VariableType::Float64;variable.id.clear();variable.id=p.addVariable(variable);
 VariableDefinition acceptedVariable{uuid(),"Accepted"};acceptedVariable.type=VariableType::Boolean;acceptedVariable.id.clear();acceptedVariable.id=p.addVariable(acceptedVariable);
 const auto producer=p.addTool("set_variables");for(auto t:p.tools())if(t.id==producer){t.parameters=encodeAssignmentConfig({producer,1,{{variable.id,ToolOutputAssignmentSource{blob,"count",4}},{acceptedVariable.id,ToolOutputAssignmentSource{blob,"found",4}}}});p.updateTool(t);}
 const auto profileId=uuid();OutputGroup group{uuid(),profileId,1,true,"vf.fields/1",producer,1,{{"count",variable.id},{"accepted",acceptedVariable.id}}};p.setOutputGroups({group});const auto publisher=p.addTool("publish_data");for(auto t:p.tools())if(t.id==publisher){t.enabled=true;t.parameters=encodePublishConfig({{group.id}});p.updateTool(t);}p.enableMultiSource();p.save();
 CommunicationProfiles profiles(dir.filePath("profiles.json"));EndpointProfile profile;profile.id=profileId;profile.enabled=true;profile.role="server";profile.fieldMapping={{"count","count"},{"accepted","accepted"}};profiles.save({{profileId,profile}});
 const auto archive=dir.filePath("production"),catalog=dir.filePath("catalog.json");
 RuntimeService runtime(archive,catalog,RuntimeIdentity::localEngineer(),{},{},{},[&](const Recipe& r){MultiSourceDependencies d;for(const auto& t:editorTools(r.definition))if(t.enabled && t.kind=="image_file")d.sources[t.id]={};d.publication=PublicationService::capture(r.definition,profiles);return d;});
 RuntimeCoordinator coordinator(runtime);InputSnapshotStore inputs;QString fail;DeliveryStore ledger(archive,[&](const QString& boundary){if(boundary==fail)throw std::runtime_error("injected disk boundary");});TriggerRouter router(coordinator,runtime,inputs,ledger,[]{return QString{};});
 CommunicationTcpEndpoint server(runtime,coordinator,router,inputs,ledger,archive,catalog);check(!server.listening(),"no constructor/Load network");
 const bool retryCapacity=app.arguments().contains("--retry-capacity");
 const bool optionalBacklog=app.arguments().contains("--optional-backlog");
 CommunicationTcpOptions options;options.workcell="cell";options.project=p.project().id;options.endpoint=profileId;options.peer="configured-peer";options.outputGroupId=group.id;if(retryCapacity){options.ackTimeoutMs=100;options.retryCount=1;}if(optionalBacklog){options.required=false;options.ackTimeoutMs=100;options.retryCount=0;}ValueEnvelope input;input.value=false;input.validity=ValueValidity::Valid;options.inputs={{"ready",{input,retryCapacity?3600000:60000}}};server.listen(options);profile.port=server.port();profile.revision=2;profiles.save({{profileId,profile}});
 PlcRuntimeSocket v2;PlcSocketOptions old;old.port=server.port();bool collision=false;try{v2.listen(old);}catch(...){collision=true;}check(collision && !v2.listening(),"v2/v3 profile port collision explicit");old.port=0;v2.listen(old);
 coordinator.load(*p.activeRecipe(),dir.path(),RuntimeTrigger::SoftwareTrigger);coordinator.start();
 // Separate v2 endpoint keeps its legacy HELLO schema.
 QTcpSocket legacy;legacy.connectToHost(QHostAddress::LocalHost,v2.port());until([&]{return legacy.bytesAvailable()>0;},"v2 HELLO");const auto legacyHello=QJsonDocument::fromJson(legacy.readLine()).object();check(legacyHello["v"]==2 && !legacyHello.contains("inputSnapshotId") && !legacyHello.contains("payloadHash") && !legacyHello.contains("fields"),"v3 fields never leak into v2");legacy.abort();
 Peer peer;peer.connectTo(server.port());const auto config=QString::fromLatin1(runtime.active()->validation().contentId);
 auto put=[&](QString id="snapshot"){peer.send({{"type","INPUT"},{"snapshotId",id},{"revision","1"},{"recipeId",p.activeRecipe()->id},{"configId",config},{"fields",QJsonArray{CommunicationWireCodec::encodeField("ready",input)}},{"sourceTime","remote"},{"clockId","peer-clock"}});peer.take("STATUS");};put();
 auto request=[&](QString id){return QJsonObject{{"type","TRIGGER"},{"requestId",id},{"sequence","1"},{"recipeId",p.activeRecipe()->id},{"configId",config},{"inputSnapshotId","snapshot"},{"inputRevision","1"}};};
 if(optionalBacklog){
  int cycles=0;server.completed=[&](const CycleResult&){++cycles;};
  peer.send(request("a"));peer.take("ACCEPTED");const auto first=peer.take("RESULT","a");
  peer.send(request("b"));peer.take("ACCEPTED");until([&]{return cycles==2;},"optional second cycle without ACK");
  check(!coordinator.deliveryPending(),"optional pending does not block execution");
  for(const auto& message:peer.messages)check(message["type"]!="RESULT","only one result in-flight");
  server.started=[&]{peer.ack(first);};peer.send(request("c"));peer.take("ACCEPTED");peer.take("ACKED","a");until([&]{return cycles==3;},"ACK during next optional cycle");
  check(server.state()!="RECOVERY_REQUIRED","active begin is not crash recovery");
  auto second=peer.take("RESULT","b");peer.ack(second);peer.take("ACKED","b");auto third=peer.take("RESULT","c");peer.ack(third);peer.take("ACKED","c");
  check(cycles==3 && server.state()=="READY","optional backlog drains one at a time");
  server.stop();v2.stop();QTextStream(stdout)<<checks<<" optional backlog checks passed\n";return 0;
 }
 if(retryCapacity){
  int cycles=0,acquisitions=0;server.completed=[&](const CycleResult& r){++cycles;acquisitions+=r.acquisitions;};
  peer.send(request("lost-ack"));peer.take("ACCEPTED");const auto initial=peer.take("RESULT");const auto retry=peer.take("RESULT");check(initial==retry,"automatic retry exact result");
  until([&]{return server.state()=="ACK_TIMEOUT";},"bounded retry exhaustion");check(cycles==1 && acquisitions==1,"retry never executes twice");
  auto durableRequest=TriggerRouter::deliveryRequest({options.workcell,options.project,options.endpoint,options.peer,"lost-ack",p.activeRecipe()->id,config,"snapshot",1,1});
  const auto beforeRetry=ledger.inspect(durableRequest);server.retryDelivery(durableRequest);const auto manual=peer.take("RESULT","lost-ack");
  check(CommunicationWireCodec::resultPayload(manual)==beforeRetry.payload && ledger.inspect(durableRequest).attempts==beforeRetry.attempts+1 && cycles==1 && acquisitions==1,"manual diagnostics retry exact payload no acquisition");
  peer.take("RESULT","lost-ack");until([&]{return server.state()=="ACK_TIMEOUT";},"manual retry bounded budget");
  bool rejected=false;auto unknown=durableRequest;unknown.requestId="unknown";try{server.retryDelivery(unknown);}catch(...){rejected=true;}check(rejected && ledger.inspect(unknown).state=="ABSENT","unknown retry never creates ACK or intent");
  for(int n=0;n<140;++n){peer.send({{"type","STATUS"}});peer.take("STATUS");}
  const auto diagnostics=server.diagnosticEvents();check(diagnostics.size()==128,"bounded diagnostics trace");
  for(const auto& event:diagnostics)check(!event.contains("fields") && !event.contains("password") && !event.contains("error") && event.contains("timestamp"),"trace metadata whitelist");
  check(server.diagnosticStatus()["state"]=="ACK_TIMEOUT","delivery timeout distinct from vision decision");
  peer.send(request("blocked"));check(peer.take("INVALID")["reason"]=="DELIVERY_PENDING","required pending blocks cycle");
  QElapsedTimer silence;silence.start();while(silence.elapsed()<250){QCoreApplication::processEvents();QThread::msleep(1);}check(peer.messages.isEmpty(),"no automatic retries after budget");
  peer.disconnect();peer.connectTo(server.port());const auto reconnected=peer.take("RESULT");check(CommunicationWireCodec::resultPayload(reconnected)==CommunicationWireCodec::resultPayload(initial),"reconnect rearms same durable payload");
  // Late ACK after exhaustion/reconnect is still authoritative.
  peer.ack(reconnected);peer.take("ACKED");check(cycles==1,"late ACK no new cycle");
  for(int n=0;n<129;++n){const auto id="continuous-"+QString::number(n);peer.send(request(id));peer.take("ACCEPTED");const auto result=peer.take("RESULT",id);peer.ack(result);peer.take("ACKED",id);}
  check(cycles==130 && acquisitions==130 && server.state()=="READY","more than 128 ACKed v3 requests have no RAM session limit");
  peer.send(request("lost-ack"));const auto old=peer.take("RESULT","lost-ack");check(CommunicationWireCodec::resultPayload(old)==CommunicationWireCodec::resultPayload(initial) && cycles==130,"old tombstone cannot reuse request key");
  server.stop();v2.stop();
  auto from=TriggerRouter::deliveryRequest({options.workcell,options.project,options.endpoint,options.peer,"lost-ack",p.activeRecipe()->id,config,"snapshot",1,1});auto to=from;to.peer="peer-generation-2";
  ledger.rolloverNamespace(from,to,"operator and peer explicitly agreed generation 2");
  bool retired=false;try{ledger.inspect(from);}catch(...){retired=true;}check(retired,"drained namespace rejects old replay");
  retired=false;try{server.listen(options);}catch(...){retired=true;}check(retired && !server.listening(),"retired namespace cannot listen/re-arm");
  check(ledger.inspect(to).state=="ABSENT","new peer generation explicitly permits fresh key");
  QTextStream(stdout)<<checks<<" retry/capacity checks passed\n";return 0;
 }
 // Exact snapshot revision, input profile authority and current config precede acquisition.
 auto wrong=request("wrong-revision");wrong["inputRevision"]="2";peer.send(wrong);check(peer.take("INVALID")["reason"]=="INPUT_MISMATCH" && !runtime.busy(),"exact input revision before acquisition");
 auto unknown=CommunicationWireCodec::encodeField("unconfigured",input);peer.send({{"type","INPUT"},{"snapshotId","unknown"},{"revision","1"},{"recipeId",p.activeRecipe()->id},{"configId",config},{"fields",QJsonArray{unknown}},{"sourceTime","remote"},{"clockId","clock"}});peer.take("ERROR");
 auto enumeration=input;enumeration.type=ScalarType::Enum;enumeration.value=EnumValue{"untrusted-catalog",7};peer.send({{"type","INPUT"},{"snapshotId","enum"},{"revision","1"},{"recipeId",p.activeRecipe()->id},{"configId",config},{"fields",QJsonArray{CommunicationWireCodec::encodeField("ready",enumeration)}},{"sourceTime","remote"},{"clockId","clock"}});peer.take("ERROR");check(inputs.get(options.endpoint,options.peer,"enum",1)==nullptr,"wire enum cannot create authoritative catalog");
 int completions=0,acquisitions=0;server.completed=[&](const CycleResult& r){++completions;acquisitions+=r.acquisitions;};
 auto trigger=request("one");auto fragmented=trigger;fragmented["v"]=3;fragmented["sessionId"]=peer.session;const auto raw=CommunicationWireCodec::encode(fragmented);peer.socket.write(raw.left(11));QCoreApplication::processEvents();check(!runtime.busy(),"partial frame cannot admit");peer.socket.write(raw.mid(11));const auto accepted=peer.take("ACCEPTED");peer.send(trigger);auto result=peer.take("RESULT");check(result["cycleId"]==accepted["cycleId"] && result["execution"]=="DONE" && result["decision"]=="OK" && result["resultValid"]==true,"real Runtime RESULT and reserved cycle identity");
 const auto fields=result["fields"].toArray();check(fields.size()==2 && fields[1].toObject()["name"]=="count" && fields[1].toObject()["value"].toInt()==1 && fields[0].toObject()["value"].toBool(),"actual measured blob count from committed bundle");check(completions==1 && acquisitions==1 && runtime.records().size()==1,"duplicate exactly one acquisition/terminal");
 const auto record=runtime.records().first();check(record["communication"].toObject()["inputSnapshotId"]=="snapshot" && record["schemaVersion"]==10,"terminal input identity and bundle");
 auto dr=TriggerRouter::deliveryRequest({options.workcell,options.project,options.endpoint,options.peer,"one",p.activeRecipe()->id,config,"snapshot",1,1});check(ledger.inspect(dr).payload==CommunicationWireCodec::resultPayload(result),"persisted canonical RESULT bytes");
 auto conflict=trigger;conflict["sequence"]="2";peer.send(conflict);check(peer.take("ERROR")["code"]=="REQUEST_CONFLICT","fingerprint conflict");
 peer.send({{"type","ACK"},{"requestId","one"},{"resultId",result["resultId"]},{"payloadHash",QString(64,'0')}});peer.take("ERROR");check(coordinator.deliveryPending(),"wrong ACK cannot release lease");
 fail="ACK_COMMITTED";peer.ack(result);peer.take("ERROR");check(ledger.inspect(dr).state=="ACK_RECEIVED" && coordinator.deliveryPending() && server.state()!="READY","failed ACK commit never READY/ACKED");fail.clear();peer.ack(result);peer.take("ACKED");check(ledger.inspect(dr).state=="ACK_COMMITTED" && !coordinator.deliveryPending(),"durable ACK commit");
 peer.messages.clear();peer.send(trigger);check(peer.take("RESULT","one")==result && acquisitions==1,"ACKed duplicate persisted replay");
 // Terminal persistence failure, retry must not acquire again; outbox failure too.
 QLockFile lock(archive+"/archive.lock");server.started=[&]{check(lock.tryLock(),"lock terminal after admission");};peer.send(request("disk"));peer.take("ACCEPTED");until([&]{return server.recordingPending();},"retained pending completion");for(auto o:peer.messages)check(!(o["type"]=="RESULT" && o["requestId"]=="disk"),"no RESULT before terminal commit");check(completions==1,"no delivery/completion on disk failure");lock.unlock();server.started={};fail="DELIVERY_PENDING";server.retryRecording();check(server.recordingPending() && runtime.records().size()==2,"terminal alone cannot send RESULT");fail.clear();server.retryRecording();result=peer.take("RESULT","disk");check(completions==2 && acquisitions==2,"disk retry same acquisition");peer.ack(result);peer.take("ACKED");
 // Disconnect after buffered RESULT does not reopen admission; reconnect session is fresh.
 peer.send(request("reconnect"));peer.take("ACCEPTED");result=peer.take("RESULT","reconnect");const auto oldSession=peer.session;peer.disconnect();check(server.state()!="READY" && coordinator.deliveryPending(),"disconnect retains barrier");peer.connectTo(server.port());check(peer.session!=oldSession,"fresh connection session");
 auto stale=QJsonObject{{"v",3},{"type","STATUS"},{"sessionId",oldSession}};peer.socket.write(CommunicationWireCodec::encode(stale));check(peer.take("ERROR")["code"]=="STALE_SESSION","stale session rejected");peer.send(request("reconnect"));auto replay=peer.take("RESULT","reconnect");check(CommunicationWireCodec::resultPayload(replay)==CommunicationWireCodec::resultPayload(result),"reconnect persisted payload stable");peer.ack(replay);peer.take("ACKED");
 // Stop while accepted suppresses all valid decision/fields.
 server.started=[&]{coordinator.stop();};peer.send(request("cancel"));peer.take("ACCEPTED");result=peer.take("RESULT","cancel");check(result["execution"]=="SKIPPED" && result["decision"]=="NOT_EVALUATED" && result["resultValid"]==false,"Stop cannot publish stale OK");peer.ack(result);peer.take("ACKED");peer.send(request("stopped"));check(peer.take("INVALID")["reason"]=="NOT_RUNNING" && server.state()!="READY","Stop closes new admission");server.started={};coordinator.start();
 // Local simulator v3 validates actual field and control decision, sends exact ACK.
 peer.disconnect();QJsonObject snapshot{{"snapshotId","simulator"},{"revision","1"},{"fields",QJsonArray{CommunicationWireCodec::encodeField("ready",input)}},{"sourceTime","remote"},{"clockId","clock"}};QFile file(dir.filePath("input.json"));check(file.open(QIODevice::WriteOnly),"sim input");file.write(QJsonDocument(snapshot).toJson());file.close();
 QProcess simulator;simulator.start(QCoreApplication::applicationDirPath()+"/visionflow_plc_simulator.exe",{"--v3","--port",QString::number(server.port()),"--input",file.fileName(),"--expect-decision","OK","--expect-field","count","--expect-value","1"});until([&]{return simulator.state()==QProcess::NotRunning;},"simulator completes");if(simulator.exitCode()!=0)QTextStream(stderr)<<simulator.readAllStandardOutput()<<simulator.readAllStandardError();check(simulator.exitCode()==0,"v3 simulator real Runtime acceptance");
 // The actual Boolean producer returns false for an empty raster; valid NOK is not Missing.
 coordinator.stop();for(auto t:p.tools())if(t.id==blob){t.parameters["minArea"]=10000.;p.updateTool(t);}p.save();coordinator.load(*p.activeRecipe(),dir.path(),RuntimeTrigger::SoftwareTrigger);coordinator.start();
 const auto nokConfig=QString::fromLatin1(runtime.active()->validation().contentId);peer.connectTo(server.port());
 peer.send({{"type","INPUT"},{"snapshotId","nok-input"},{"revision","1"},{"recipeId",p.activeRecipe()->id},{"configId",nokConfig},{"fields",QJsonArray{CommunicationWireCodec::encodeField("ready",input)}},{"sourceTime","remote"},{"clockId","clock"}});peer.take("STATUS");
 auto nokRequest=request("nok");nokRequest["configId"]=nokConfig;nokRequest["inputSnapshotId"]="nok-input";peer.send(nokRequest);peer.take("ACCEPTED");const auto nok=peer.take("RESULT","nok");
 check(nok["execution"]=="DONE" && nok["decision"]=="NOK" && nok["resultValid"]==true && nok["fields"].toArray()[0].toObject()["value"]==false && nok["fields"].toArray()[0].toObject()["validity"]=="valid","real Boolean false is valid NOK");peer.ack(nok);peer.take("ACKED");peer.disconnect();
 // Framing deadlines are polled even with no more bytes, malformed batch is closed.
 Peer framing;framing.connectTo(server.port());framing.socket.write("{");until([&]{return framing.socket.state()==QAbstractSocket::UnconnectedState;},"partial frame disconnect",4000);check(server.state()!="READY","disconnected endpoint cannot report Ready");
 framing.connectTo(server.port());framing.socket.write(QByteArray(65537,'x'));until([&]{return framing.socket.state()==QAbstractSocket::UnconnectedState;},"bounded oversized frame disconnect");
 // Failed intent prevents acquisition and leaves admission visibly closed.
 framing.connectTo(server.port());fail="REQUEST_INTENT";auto intentFailure=request("intent-failure");intentFailure["configId"]=nokConfig;intentFailure["inputSnapshotId"]="nok-input";framing.send(intentFailure);framing.take("INVALID");check(server.state()=="RECOVERY_REQUIRED" && !runtime.busy(),"intent disk failure closes admission before acquisition");fail.clear();
 server.stop();v2.stop();check(server.state()=="STOPPED" && !runtime.busy(),"endpoint Stop drains safely");
 QTextStream(stdout)<<checks<<" checks passed: real localhost v3 INPUT/Runtime/bundle/outbox/ACK, duplicate single acquisition, disk boundaries, session/reconnect, Stop, simulator, v2 port isolation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
