#include "application/PlcRuntimeSocket.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QJsonDocument>
#include <QTextStream>
#include <QThread>

using namespace visionflow;
namespace {
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class Predicate> void until(Predicate predicate,const char* text,int ms=10000){QElapsedTimer t;t.start();while(!predicate() && t.elapsed()<ms){QCoreApplication::processEvents();QThread::msleep(1);}check(predicate(),text);}
class Peer {
public:
    QTcpSocket socket;QByteArray input;QVector<QJsonObject> messages;QString session;
    Peer(){QObject::connect(&socket,&QTcpSocket::readyRead,[this]{input+=socket.readAll();for(;;){const auto n=input.indexOf('\n');if(n<0)break;QJsonParseError error;const auto doc=QJsonDocument::fromJson(input.left(n),&error);input.remove(0,n+1);check(error.error==QJsonParseError::NoError && doc.isObject(),"server framed JSON");messages.append(doc.object());}});}
    void connectTo(quint16 port){input.clear();messages.clear();socket.connectToHost(QHostAddress::LocalHost,port);const auto hello=take("HELLO");check(hello["v"]==2,"protocol version");session=hello["sessionId"].toString();}
    QJsonObject take(const QString& type,int ms=10000,const QString& requestId={}){QJsonObject result;until([&]{if(!result.isEmpty())return true;for(int i=0;i<messages.size();++i)if(messages[i]["type"]==type && (requestId.isEmpty() || messages[i]["requestId"]==requestId)){result=messages.takeAt(i);return true;}return false;},qPrintable("expected server "+type+" request="+requestId),ms);return result;}
    QByteArray bytes(QJsonObject o)const{o["v"]=2;o["sessionId"]=session;return QJsonDocument(o).toJson(QJsonDocument::Compact)+'\n';}
    void send(QJsonObject o){socket.write(bytes(o));}
    void ack(const QString& id){send({{"type","ACK"},{"requestId",id}});check(take("ACKED")["requestId"]==id,"ACK correlation");}
    void reconnect(quint16 port){socket.abort();until([&]{QCoreApplication::processEvents();return socket.state()==QAbstractSocket::UnconnectedState;},"disconnect");QElapsedTimer t;t.start();while(t.elapsed()<20)QCoreApplication::processEvents();connectTo(port);}
};
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);QStringList trace;try {
    QTemporaryDir dir(QDir::currentPath()+"/plc-fixture-XXXXXX");check(dir.isValid(),"build fixture");
    QImage image(1000,600,QImage::Format_Grayscale8);image.fill(0);for(int y=100;y<400;++y)for(int x=200;x<600;++x)image.scanLine(y)[x]=255;
    const auto path=dir.path()+"/part.png";check(image.save(path),"actual image");
    ProjectService project;project.createProject("PLC","",dir.path()+"/part.vfproject","file");project.createRecipe("Part","",ApplicationType::Measurement,"file");
    const auto source=project.setImageFile(path),left=project.addTool("edge"),right=project.addTool("edge"),measurement=project.addTool("measurement");
    for(auto t:project.tools()){if(t.id==right)t.parameters["direction"]="right_to_left";if(t.id==measurement){t.parameters["lowerMm"]=9.97;t.parameters["upperMm"]=9.98;}project.updateTool(t);}
    project.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",measurement,"p1"},{right,"point",measurement,"p2"}});project.setCalibrationSetup("lens","plane");
    const auto frame=RuntimeService::probe(*project.activeRecipe(),dir.path(),{});const auto installation=CalibrationWorkflow::installation(*project.activeRecipe(),frame);
    auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,installation.source},{b,installation.source},mm};};
    const auto catalog=project.path()+".calibrations.json";const auto calibration=MeasurementService{}.validate(CalibrationWorkflow::teach(*project.activeRecipe(),frame,"scale",1,sample("fit",{0,0},{400,0},10)),{sample("x",{10,30},{810,30},20),sample("y",{40,10},{40,510},12.5)},.001);
    CalibrationService(catalog).save(calibration);CalibrationWorkflow::bind(project,frame,catalog,"scale");project.save();
    const auto archive=project.path()+".production";RuntimeService runtime(archive,catalog,{"engineer",[](const QString&,ProductionAction){return true;}});
    auto recipe=*project.activeRecipe();runtime.validate(recipe,dir.path(),frame);runtime.activate(recipe,dir.path());
    QString unavailable;PlcRuntimeSocket server(runtime,archive,catalog,[&]{return unavailable;});server.changed=[&]{trace=server.diagnostics();};PlcSocketOptions options;options.port=0;options.ackTimeoutMs=100;options.retryCount=1;options.measurementToolId=measurement;server.listen(options);
    int completions=0;server.completed=[&](const CycleResult&){++completions;};Peer peer;peer.connectTo(server.port());
    auto request=[&](QString id){return QJsonObject{{"type","TRIGGER"},{"requestId",id},{"recipeId",recipe.id},{"configId",QString::fromLatin1(runtime.active()->validation().contentId)}};};
    auto trigger=request("split");const auto bytes=peer.bytes(trigger);peer.socket.write(bytes.left(13));QCoreApplication::processEvents();check(!server.blocking(),"partial frame cannot trigger");peer.socket.write(bytes.mid(13)+bytes);
    peer.take("BUSY");const auto done=peer.take("DONE");check(done["resultValid"]==true && done["decision"]=="PASS" && std::abs(done["valueMm"].toDouble()-9.975)<1e-9,"actual calibrated TCP PASS");
    check(runtime.records().size()==1 && completions==1 && runtime.records().first()["communicationState"]=="RESULT_PENDING","one immutable Runtime record with communication origin");
    const auto duplicate=peer.take("DONE");check(duplicate==done,"duplicate result byte values stable");
    auto conflict=trigger;conflict["configId"]="other";peer.send(conflict);check(peer.take("ERROR")["code"]=="REQUEST_CONFLICT","request identity conflict");
    peer.send(request("blocked"));check(peer.take("INVALID")["reason"]=="DELIVERY_PENDING","no new cycle before ACK");
    until([&]{return server.state()=="ACK_TIMEOUT";},"bounded result retries",3000);check(completions==1,"ACK loss never reacquires");
    bool stopRefused=false;try{server.stop();}catch(...){stopRefused=true;}check(stopRefused,"pending ACK cannot be discarded");
    const auto session=peer.session;peer.reconnect(server.port());check(peer.session==session && peer.take("DONE")==done,"reconnect replays pending result");
    peer.ack("split");peer.ack("split");check(server.state()=="READY","late/idempotent ACK");
    peer.send(trigger);check(peer.take("DONE")==done && completions==1,"acknowledged request replay does not acquire");
    peer.send({{"type","ACK"},{"requestId","unknown"}});check(peer.take("ERROR")["code"]=="UNKNOWN_REQUEST","unknown ACK rejected");
    auto mismatch=request("mismatch");mismatch["recipeId"]="foreign";peer.send(mismatch);check(peer.take("ERROR")["code"]=="SNAPSHOT_MISMATCH","no remote recipe switching");
    unavailable="editor acquiring";peer.send(request("unavailable"));check(peer.take("INVALID")["reason"]=="NOT_READY","shared acquisition guard");unavailable.clear();
    // Lock the terminal archive only after intent/begin have committed.
    QLockFile lock(archive+"/archive.lock");lock.setStaleLockTime(0);server.started=[&]{check(lock.tryLock(),"lock after begin");};
    peer.messages.clear();peer.send(request("recording"));peer.take("BUSY");until([&]{return server.recordingPending();},"record failure retained");check(runtime.busy() && completions==1,"recording failure blocks delivery");
    for(const auto& o:peer.messages)check(o["type"]!="DONE","no result before durable record");
    int admittedBeforeRetry=0;for(const auto& v:runtime.diagnostics()->snapshot()["events"].toArray()){const auto e=v.toObject();admittedBeforeRetry+=e["action"]=="admission" && e["outcome"]=="accepted";}
    check(QFile::rename(path,path+".held"),"hide source while retrying PLC record");
    lock.unlock();server.started={};server.retryRecording();check(QFile::rename(path+".held",path),"restore PLC retry source");
    int admittedAfterRetry=0;for(const auto& v:runtime.diagnostics()->snapshot()["events"].toArray()){const auto e=v.toObject();admittedAfterRetry+=e["action"]=="admission" && e["outcome"]=="accepted";}
    check(admittedAfterRetry==admittedBeforeRetry && server.blocking(),"PLC disk retry does not admit another ticket and retains ACK barrier");const auto recorded=peer.take("DONE");check(recorded["requestId"]=="recording" && completions==2 && runtime.records().size()==2,"retry commits same cycle without acquisition");
    check(lock.tryLock(),"ACK lock");peer.send({{"type","ACK"},{"requestId","recording"}});check(peer.take("ERROR")["code"]=="SERVICE_ERROR" && server.blocking(),"failed ACK journal retains lease");lock.unlock();peer.ack("recording");
    // A retry/replay for the previous request may still be queued after ACK.
    // Keep one deliberately: the cancellation assertion must inspect its own ID.
    peer.send(request("recording"));until([&]{for(const auto& message:peer.messages)if(message["type"]=="DONE" && message["requestId"]=="recording")return true;return false;},"prior result replay queued");
    // Cancellation before worker entry deterministically suppresses all outputs.
    server.started=[&]{server.cancel();};peer.send(request("cancelled"));peer.take("BUSY",10000,"cancelled");const auto cancelled=peer.take("DONE",10000,"cancelled");check(cancelled["executionState"]=="SKIPPED" && cancelled["resultValid"]==false && cancelled["valueMm"].isNull() && cancelled["decision"]=="NOT_EVALUATED","Stop cannot send stale PASS");peer.ack("cancelled");server.started={};
    check(peer.take("DONE",10000,"recording")==recorded,"prior replay remains unchanged and cannot impersonate cancelled result");
    // Change immutable active tolerances: the mapped actual decision remains FAIL with DONE.
    for(auto t:editorTools(recipe.definition))if(t.id==measurement){t.parameters["upperMm"]=9.9749;auto tools=editorTools(recipe.definition);for(auto& x:tools)if(x.id==t.id)x=t;recipe.definition=withEditorTools(recipe.definition,tools);}
    runtime.validate(recipe,dir.path(),frame);runtime.activate(recipe,dir.path());
    peer.send(request("fail"));peer.take("BUSY");const auto fail=peer.take("DONE",10000,"fail");check(fail["executionState"]=="DONE" && fail["decision"]=="FAIL" && fail["resultValid"]==true,"FAIL distinct from execution error");peer.ack("fail");
    const auto before=completions;auto invalid=recipe;auto setup=invalid.definition["planarSetup"].toObject();setup["lensId"]="changed";invalid.definition["planarSetup"]=setup;runtime.observeInstallation(invalid);
    peer.send(request("invalid"));check(peer.take("INVALID")["reason"]=="DEPENDENCY_INVALID" && completions==before && !runtime.busy(),"invalid calibration prevents acquisition");runtime.observeInstallation(recipe);
    // Actual decode error has no decision/value, even following a valid FAIL.
    QFile::remove(path);peer.send(request("missing"));peer.take("BUSY");const auto missing=peer.take("DONE",10000,"missing");check(missing["executionState"]=="ERROR" && missing["resultValid"]==false && missing["decision"]=="NOT_EVALUATED" && missing["valueMm"].isNull(),"acquisition error cannot become PASS");peer.ack("missing");
    auto bad=peer.bytes({{"type","STATUS"}});bad.replace("\"v\":2","\"v\":3");peer.socket.write(bad);check(peer.take("ERROR")["code"]=="UNSUPPORTED_VERSION","future protocol rejected");
    auto extra=QJsonObject{{"type","STATUS"},{"extra",true}};peer.send(extra);check(peer.take("ERROR")["code"]=="BAD_MESSAGE","strict command schema");
    peer.socket.write(QByteArray(2050,'x'));until([&]{return peer.socket.state()==QAbstractSocket::UnconnectedState;},"oversized frame disconnect");peer.connectTo(server.port());
    peer.socket.write("{");until([&]{return peer.socket.state()==QAbstractSocket::UnconnectedState;},"partial message deadline",4000);peer.connectTo(server.port());
    const auto oldSession=server.sessionId();server.stop();options.measurementToolId.clear();server.listen(options);check(server.sessionId()!=oldSession,"new listener has new dedup scope");peer.reconnect(server.port());
    QJsonObject stale{{"type","STATUS"},{"v",2},{"sessionId",oldSession}};peer.socket.write(QJsonDocument(stale).toJson(QJsonDocument::Compact)+'\n');check(peer.take("ERROR")["code"]=="PROTOCOL_OR_SESSION","previous session refused");
    check(image.save(path),"restore source");peer.send(request("unmapped"));peer.take("BUSY");const auto unmapped=peer.take("DONE",10000,"unmapped");check(unmapped["executionState"]=="DONE" && unmapped["resultValid"]==false && unmapped["valueMm"].isNull() && unmapped["decision"]=="NOT_EVALUATED","mapping absence never invents recipe decision");peer.ack("unmapped");
    server.stop();RuntimeService restored(archive,catalog,RuntimeIdentity::localEngineer());check(!restored.active() && restored.records().size()==6,"restart retains records without activation/automatic rerun");
    const auto events=ProductionStore(archive).recent("events",256);int results=0,acks=0;for(const auto& e:events){if(e["action"]=="PLC_RESULT")++results;if(e["action"]=="PLC_ACK")++acks;}
    check(results==6 && acks==6,"durable request/result/ACK evidence");
    // Independent transport remains connected across load/start/stop and safe target rebind.
    PlcRuntimeSocket transport;PlcSocketOptions independent;independent.port=0;transport.listen(independent);
    PlcRuntimeSocket collision;auto occupied=independent;occupied.port=transport.port();bool rejectedPort=false;try{collision.listen(occupied);}catch(...){rejectedPort=true;}check(rejectedPort && !collision.listening(),"occupied endpoint reports error without a Runtime");
    Peer p;p.connectTo(transport.port());const auto transportSession=p.session;
    QJsonObject req{{"type","TRIGGER"},{"requestId","no-recipe"},{"recipeId",""},{"configId",""}};
    p.send(req);auto rejected=p.take("INVALID");check(rejected["reason"]=="NO_RECIPE_LOADED" && rejected["valueMm"].isNull() && !rejected.contains("cycleId"),"unbound listener INVALID without cycle");
    RuntimeCoordinator coordinator(restored);transport.bind(&restored,&coordinator,archive,catalog);
    coordinator.load(recipe,dir.path(),RuntimeTrigger::Manual);req["requestId"]="stopped";p.send(req);check(p.take("INVALID")["reason"]=="NOT_RUNNING","loaded stopped INVALID");
    coordinator.start();req["requestId"]="manual";p.send(req);check(p.take("INVALID")["reason"]=="TRIGGER_SOURCE_DISABLED","manual source gating");coordinator.stop();
    coordinator.load(recipe,dir.path(),RuntimeTrigger::SoftwareTrigger);coordinator.start();
    req["recipeId"]=recipe.id;req["configId"]=QString::fromLatin1(restored.active()->validation().contentId);req["requestId"]="software";
    p.send(req);p.take("BUSY");auto delivered=p.take("DONE");check(coordinator.deliveryPending(),"delivery lease bound to coordinator");
    coordinator.stop();p.send(req);check(p.take("DONE")==delivered,"Stop retains accepted duplicate result");
    auto fresh=req;fresh["requestId"]="after-stop";p.send(fresh);check(p.take("INVALID")["reason"]=="DELIVERY_PENDING","pending delivery survives Stop");
    p.ack("software");check(!coordinator.deliveryPending(),"ACK releases coordinator delivery lease");p.send(fresh);check(p.take("INVALID")["reason"]=="NOT_RUNNING","Stop closes admission without connection loss");
    transport.bind(nullptr,nullptr);p.send(fresh);check(p.take("INVALID")["reason"]=="NO_RECIPE_LOADED" && p.session==transportSession && transport.connected(),"unbind keeps transport session");
    p.send(req);check(p.take("DONE")==delivered,"old snapshot replay after unbind");p.ack("software");transport.stop();
    QTextStream(stdout)<<"PASS: real TCP framing, immutable Runtime/mm PASS/FAIL, duplicate/reconnect, ACK loss, bounded retry, recording/ACK failure, cancellation, error and session scope\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n'<<trace.join('\n')<<'\n';return 1;}}
