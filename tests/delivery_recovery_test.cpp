#include "application/CommunicationTcpEndpoint.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QProcess>
#include <QThread>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QTextStream>
#include <QUuid>
using namespace visionflow;
namespace {
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool b=false;try{f();}catch(...){b=true;}check(b,"expected rejection");}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
TriggerRequest request(){return {"cell","project","endpoint","peer","request","recipe","config","snapshot",1,1};}
QJsonObject terminal(){
 const auto cycle=id();QJsonObject b{{"schemaVersion",1},{"cycleId",cycle},{"configHash","config"},{"dependencyId","dependency"},{"decision","NOT_EVALUATED"},{"groups",QJsonArray{}}};
 b["bundleId"]=CommunicationWireCodec::payloadHash(QJsonDocument(b).toJson(QJsonDocument::Compact));
 QJsonObject t{{"schemaVersion",10},{"baseSchemaVersion",3},{"recordId",id()},{"cycleId",cycle},{"recipeId","recipe"},{"configId","config"},{"dependencyId","dependency"},{"state","ERROR"},{"error","fixture failure"},{"tools",QJsonArray{}},{"resultBundle",b},{"communication",QJsonObject{{"protocol","vf.socket/3"},{"endpoint","endpoint"},{"peer","peer"},{"requestId","request"},{"sequence","1"},{"inputSnapshotId","snapshot"},{"inputRevision","1"}}}};
 ProductionResultCodec::validate(t);return t;
}
QByteArray payload(const QJsonObject& t){return CommunicationWireCodec::resultPayload(CommunicationWireCodec::sealResult({{"v",3},{"type","RESULT"},{"sessionId","old-session"},{"requestId","request"},{"sequence","1"},{"recipeId","recipe"},{"configId","config"},{"inputSnapshotId","snapshot"},{"inputRevision","1"},{"resultId",t["resultBundle"].toObject()["bundleId"]},{"cycleId",t["cycleId"]},{"execution","ERROR"},{"decision","NOT_EVALUATED"},{"resultValid",false},{"fields",QJsonArray{}},{"error","fixture failure"}}));}
QJsonObject receive(QTcpSocket& peer){QElapsedTimer timer;timer.start();while(!peer.canReadLine() && timer.elapsed()<5000){QCoreApplication::processEvents();QThread::msleep(1);}check(peer.canReadLine(),"wire response");auto b=peer.readLine();b.chop(1);return CommunicationWireCodec::decode(b);}
void receiveAcknowledged(QTcpSocket& peer,const QByteArray& expected){for(int n=0;n<20;++n){const auto o=receive(peer);if(o["type"]=="ACKED")return;check(o["type"]=="RESULT" && CommunicationWireCodec::resultPayload(o)==expected,"only exact in-flight retry may precede ACKED");}throw std::runtime_error("ACKED missing after retries");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 const auto args=app.arguments();
 if(args.size()==4 && args[1]=="--child"){
  const auto path=args[2],boundary=args[3];DeliveryStore store(path,[&](const QString& b){if(boundary=="TERMINAL" && b=="DELIVERY_PENDING")throw std::runtime_error("power loss after terminal");});const auto r=TriggerRouter::deliveryRequest(request());const auto t=terminal();store.request(r);
  if(boundary!="INTENT")store.begin(r,t["cycleId"].toString());
  if(boundary!="INTENT" && boundary!="BEGIN"){try{store.commit(r,t,payload(t));}catch(...){if(boundary!="TERMINAL")throw;}}
  if(boundary=="ACK_RECEIVED" || boundary=="ACK_COMMITTED")store.acknowledge(r,store.inspect(r).payloadHash);
  if(boundary=="ACK_COMMITTED")store.commitAcknowledgement(r);
  QTextStream out(stdout);out<<"DURABLE\n";out.flush();for(;;)QThread::msleep(100);
 }
 QTemporaryDir root(QCoreApplication::applicationDirPath()+"/recovery-XXXXXX");check(root.isValid(),"fixture root");
 for(const QString boundary:{"INTENT","BEGIN","TERMINAL","PENDING","ACK_RECEIVED","ACK_COMMITTED"}){
  const auto path=root.filePath(boundary);QProcess child;child.start(app.applicationFilePath(),{"--child",path,boundary});check(child.waitForStarted() && child.waitForReadyRead(10000),"child durable boundary");check(child.readAllStandardOutput().contains("DURABLE"),"child reached boundary");child.kill();check(child.waitForFinished(),"separate process killed");
  DeliveryStore store(path);auto recovery=store.recovery();check(recovery.size()==1,"startup stable key scan");const auto before=recovery.first().status;const auto r=request();const auto d=TriggerRouter::deliveryRequest(r);
  RuntimeService runtime(path,root.filePath("catalog.json"),RuntimeIdentity::localEngineer());RuntimeCoordinator coordinator(runtime);InputSnapshotStore inputs;TriggerRouter router(coordinator,runtime,inputs,store,[]{return QString{};});CommunicationTcpEndpoint endpoint(runtime,coordinator,router,inputs,store,path,root.filePath("catalog.json"));
  check(!endpoint.listening() && !runtime.busy(),"startup no network/acquisition");CommunicationTcpOptions options;options.workcell=r.workcell;options.project=r.project;options.endpoint=r.endpoint;options.peer=r.peer;endpoint.listen(options);
  check(endpoint.state()!="READY" && coordinator.state()!=RuntimeState::Running,"restart cannot grant Run");check(endpoint.inspectRecovery().size()==1,"inspection command");
  auto unknown=r;unknown.requestId="unknown";rejects([&]{endpoint.resumeDelivery(unknown);});auto conflict=r;conflict.sequence=2;rejects([&]{endpoint.resumeDelivery(conflict);});check(store.recovery().size()==1,"unknown/conflict no synthetic ledger");
  const auto admission=router.admit(r,{});check(!admission.accepted() && admission.reason=="DUPLICATE","durable duplicate no cycle");
  if(boundary=="INTENT" || boundary=="BEGIN"){
   check(endpoint.state()=="RECOVERY_REQUIRED","unfinished request closes startup");rejects([&]{endpoint.resumeDelivery(r);});check(store.inspect(d).state=="RECOVERY_REQUIRED","no fake terminal or ACK");
  }else{
   if(boundary=="TERMINAL"){endpoint.resumeDelivery(r);check(store.inspect(d).dispatchable(),"explicit terminal recovery works without connected peer");}
   QTcpSocket peer;peer.connectToHost(QHostAddress::LocalHost,endpoint.port());const auto hello=receive(peer);check(hello["ready"]==false,"startup HELLO not ready");
   if(boundary=="PENDING" || boundary=="ACK_COMMITTED"){
    QJsonObject trigger{{"v",3},{"type","TRIGGER"},{"sessionId",hello["sessionId"]},{"requestId",r.requestId},{"sequence","1"},{"recipeId",r.recipeId},{"configId",r.configId},{"inputSnapshotId",r.inputSnapshotId},{"inputRevision","1"}};
    peer.write(CommunicationWireCodec::encode(trigger));const auto duplicate=receive(peer);check(CommunicationWireCodec::resultPayload(duplicate)==before.payload,"wire duplicate across process exact replay without INPUT or Run");
    if(boundary=="ACK_COMMITTED"){
     peer.write(CommunicationWireCodec::encode(QJsonObject{{"v",3},{"type","ACK"},{"sessionId",hello["sessionId"]},{"requestId",r.requestId},{"resultId",duplicate["resultId"]},{"payloadHash",duplicate["payloadHash"]}}));receiveAcknowledged(peer,before.payload);
    }
   }
   endpoint.resumeDelivery(r);const auto after=store.inspect(d);
   if(boundary=="ACK_RECEIVED" || boundary=="ACK_COMMITTED"){
    check(after.state=="ACK_COMMITTED" && after.payload==before.payload,"durable ACK reconciliation no new effect");check(!coordinator.deliveryPending(),"committed ACK clears barrier");
   }else{
    const auto wire=receive(peer);check(wire["type"]=="RESULT" && wire["sessionId"]!=QString("old-session"),"fresh session replay");
    check(CommunicationWireCodec::resultPayload(wire)==after.payload && wire["payloadHash"]==after.payloadHash,"exact durable bytes and result identity");
    if(boundary=="PENDING")check(before.payload==after.payload,"pending restart byte identity");
    const auto t=store.recoveryTerminal(d);check(payload(t)==after.payload,"terminal gap reconstruction canonical bytes");
    if(boundary=="PENDING"){auto unresolved=d;unresolved.requestId="unfinished-other";store.request(unresolved);}
    QJsonObject ack{{"v",3},{"type","ACK"},{"sessionId",hello["sessionId"]},{"requestId",r.requestId},{"resultId",wire["resultId"]},{"payloadHash",wire["payloadHash"]}};peer.write(CommunicationWireCodec::encode(ack));receiveAcknowledged(peer,after.payload);check(coordinator.deliveryPending()==(boundary=="PENDING"),"ACK preserves other unresolved request barrier");if(boundary=="PENDING")check(endpoint.state()=="RECOVERY_REQUIRED","ACK cannot clear unrelated recovery");
   }
  }
  check(!runtime.busy() && coordinator.state()!=RuntimeState::Running,"recovery never starts execution");endpoint.stop();
 }
 // V2 history has its independent policy and creates no v3 recovery request.
 const auto legacy=root.filePath("legacy");ProductionStore(legacy).append("events",id(),{{"event","PLC_REQUEST"}});check(DeliveryStore(legacy).recovery().isEmpty(),"v2 isolation");
 QTextStream(stdout)<<"6 killed-process restart boundaries, stable dedup, exact replay, explicit recovery and v2 isolation passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
