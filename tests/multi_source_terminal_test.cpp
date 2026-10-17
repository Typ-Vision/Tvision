#include "application/MeasurementWorkspace.hpp"
#include "application/ProjectService.hpp"
#include "application/EditorSession.hpp"
#include "application/SourceFrameIndex.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QTextStream>
#include <QThread>
#include <QFile>
#include <thread>
#include <cmath>
#include <QUuid>
#include <QLockFile>
#include <QJsonDocument>
#include <QJsonArray>
#include "application/ProductionResultCodec.hpp"
#include "application/RuntimeCoordinator.hpp"
#include "application/PlcRuntimeSocket.hpp"
#include <QElapsedTimer>
using namespace visionflow;
namespace {int checks=0;void check(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}template<class F>void reject(F f){++checks;try{f();}catch(...){return;}throw std::runtime_error("Expected rejection");}
QImage stripe(int index=0){QImage im(640,200,QImage::Format_Grayscale8);im.fill(10+index);for(int y=0;y<200;++y)for(int x=100+index;x<400+11*index;++x)im.scanLine(y)[x]=220;return im;}
class Adapter:public AcquisitionAdapter {public:int index=0;explicit Adapter(int i):index(i){}AcquisitionCapabilities capabilities()const override{return {"webcam",true};}ImageFrame acquire(const AcquisitionRequest&)const override{auto im=stripe(index);return ImageFrame::fromBuffer(im.constBits(),im.sizeInBytes(),640,200,im.bytesPerLine(),PixelFormat::Gray8);}};
CalibrationModelRecord teach(MeasurementWorkspace& w,const MeasurementCamera& c,const MeasurementSetupDescriptor& s,double mm,int index){IndependentCaptureRequest req;req.origin=SourceOrigin::CameraAcquired;req.camera=c;req.setup=s;IndependentCaptureBackend backend{[&]{return QVector<DeviceBinding>{c.device};},[index](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<Adapter>(index);}};IndependentCalibrationSession session;check(session.accept(IndependentCalibrationSession::run(session.begin(req),backend)),"Actual fixture probe");auto& math=session.math();check(math.accept(CalibrationWizardSession::run(math.scanPair("width",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"Real caliper");math.setKnownMillimetres(mm);auto r=math.calculate().named("Scale");const SetupReference ref{c.id,s.id,s.revision};w.calibrations().save(ref,r);w.calibrations().setOperational(c.id,s.id);w.calibrations().activate(ref,r.contentId(),session);return r;}
const ToolResult& result(const CycleResult& c,const QString& id){for(const auto& r:c.tools)if(r.toolId==id)return r;throw std::runtime_error("Missing result");}
}

class DelayedSource final:public AcquisitionAdapter {
public:
    int delay;std::shared_ptr<std::atomic_bool> stop;explicit DelayedSource(int ms,std::shared_ptr<std::atomic_bool> cancel={}):delay(ms),stop(std::move(cancel)){}
    AcquisitionCapabilities capabilities()const override{return {"webcam",true};}
    ImageFrame acquire(const AcquisitionRequest&)const override{QThread::msleep(delay);if(stop)stop->store(true);auto image=stripe(2);return ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),640,200,image.bytesPerLine(),PixelFormat::Gray8);}
};

namespace {
template<class Predicate> void until(Predicate predicate,const char* text,int ms=10000){QElapsedTimer t;t.start();while(!predicate() && t.elapsed()<ms){QCoreApplication::processEvents();QThread::msleep(1);}check(predicate(),text);}
class Peer {
public:
    QTcpSocket socket;QByteArray input;QVector<QJsonObject> messages;QString session;
    Peer(){QObject::connect(&socket,&QTcpSocket::readyRead,[this]{input+=socket.readAll();for(;;){const auto n=input.indexOf('\n');if(n<0)break;QJsonParseError error;const auto doc=QJsonDocument::fromJson(input.left(n),&error);input.remove(0,n+1);check(error.error==QJsonParseError::NoError && doc.isObject(),"server framed JSON");messages.append(doc.object());}});}
    void connectTo(quint16 port){input.clear();messages.clear();socket.connectToHost(QHostAddress::LocalHost,port);const auto hello=take("HELLO");check(hello["v"]==2,"protocol version");session=hello["sessionId"].toString();}
    QJsonObject take(const QString& type,int ms=10000){QJsonObject result;until([&]{if(!result.isEmpty())return true;for(int i=0;i<messages.size();++i)if(messages[i]["type"]==type){result=messages.takeAt(i);return true;}return false;},qPrintable("expected server "+type),ms);return result;}
    QByteArray bytes(QJsonObject o)const{o["v"]=2;o["sessionId"]=session;return QJsonDocument(o).toJson(QJsonDocument::Compact)+'\n';}
    void send(QJsonObject o){socket.write(bytes(o));}
    void ack(const QString& id){send({{"type","ACK"},{"requestId",id}});check(take("ACKED")["requestId"]==id,"ACK correlation");}
    void reconnect(quint16 port){socket.abort();until([&]{QCoreApplication::processEvents();return socket.state()==QAbstractSocket::UnconnectedState;},"disconnect");QElapsedTimer t;t.start();while(t.elapsed()<20)QCoreApplication::processEvents();connectTo(port);}
};

}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
QTemporaryDir dir(QDir::currentPath()+"/kmc10c1-XXXXXX");check(dir.isValid(),"Build fixture");const auto path=dir.filePath("project.vfproject");ProjectService project;project.createProject("Multi Runtime core","",path,"file");project.save();MeasurementWorkspace workspace(path,project.project().id,[]{return true;});MeasurementSetupContract contract;
for(int i=0;i<10;++i){const auto id=QString::number(i);contract.cameras.append({{"c"+id},"Camera "+id,{"webcam","717"+id,DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}});contract.setups.append({{"s"+id},contract.cameras.last().id,"Setup "+id,"lens","plane",1,1,{{640,200},{},{},true}});check(stripe(i==1 ? 0:i).save(dir.filePath("source-"+id+".png")),"Source raster");}
workspace.replace(contract);for(int i=0;i<10;++i)teach(workspace,contract.cameras[i],contract.setups[i],i+1,i==1 ? 0:i);
QVector<SetupReference> refs;for(int i=0;i<2;++i)refs.append({contract.cameras[i].id,contract.setups[i].id,1});
reject([&]{workspace.calibrations().snapshots(refs,[&]{auto renamed=contract;renamed.cameras[0].name="Renamed during read";workspace.replace(renamed);});});check(workspace.calibrations().snapshots(refs).size()==2,"Retry reads whole stable registry set");
reject([&]{workspace.calibrations().snapshots(refs,[&]{teach(workspace,contract.cameras[0],contract.setups[0],101,0);});});teach(workspace,contract.cameras[0],contract.setups[0],1,0);check(workspace.calibrations().snapshots(refs).size()==2,"Retry reads whole stable model/permission set");
for(int n:{1,2,8,10})for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
project.createRecipe(QString("Production %1 %2").arg(n).arg(typeKey(type)),"",type,"file");project.enableMultiSource();QVector<QString> sources,metrics;QVector<ToolConnection> links;
for(int i=0;i<n;++i){const auto source=project.addTool("image_file");project.setImageFile(dir.filePath(QString("source-%1.png").arg(i)),source);const auto left=project.addTool("edge_point"),right=project.addTool("edge_point"),metric=project.addTool("measurement");sources<<source;metrics<<metric;
for(auto t:project.tools())if(t.id==left || t.id==right){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();t.parameters["polarity"]=t.id==left ? "rising":"falling";project.updateTool(t);}
links<<ToolConnection{source,"image",left,"image"}<<ToolConnection{source,"image",right,"image"}<<ToolConnection{left,"point",metric,"p1"}<<ToolConnection{right,"point",metric,"p2"};project.bindSourceSetup(source,SetupReference{contract.cameras[i].id,contract.setups[i].id,1}.toJson());}
project.setConnections(links);project.save();const auto recipe=*project.activeRecipe();bool allow=true;int corrupt=0,providerCalls=0;RuntimeIdentity identity{"engineer",[&](const QString& principal,ProductionAction){return allow && principal=="engineer";}};

int captures=0;RuntimeService runtime(dir.filePath("archive-"+QString::number(n)+typeKey(type)),path+".calibrations.json",identity,{}, {}, {},[&](const Recipe& r){++captures;return workspace.productionSources(r);});
RuntimeCoordinator coordinator(runtime);coordinator.load(recipe,dir.path(),RuntimeTrigger::Manual);coordinator.start();check(!runtime.records().size(),"Preparation produces no result");const QJsonObject communication{{"requestId","trace"}};auto lease=coordinator.begin(RuntimeTrigger::Manual,communication);auto done=RuntimeService::execute(lease,"",{});check(done.state==ExecutionState::Done && done.frames.size()==n,"Actual N source terminal fixture");
const auto reads=captures;QLockFile lock(dir.filePath("archive-"+QString::number(n)+typeKey(type)+"/archive.lock"));lock.setStaleLockTime(0);check(lock.tryLock(0),"Archive lock fixture");reject([&]{coordinator.finish(lease.ticket,done);});check(coordinator.recordingPending() && runtime.busy() && runtime.records().isEmpty(),"Record failure keeps lease and publishes nothing");reject([&]{coordinator.begin(RuntimeTrigger::Manual);});auto changed=done;changed.error="changed retry";reject([&]{coordinator.finish(lease.ticket,changed);});lock.unlock();
coordinator.stop();coordinator.finish(lease.ticket,done);check(!runtime.busy() && !coordinator.recordingPending() && captures==reads,"Retry has no capture/provider/model read");const auto records=runtime.records();check(records.size()==1 && records[0]["schemaVersion"]==7 && records[0]["communication"]==communication,"Schema7 committed once with socket context");const auto record=records.first();ProductionResultCodec::validate(record);quint64 rawBytes=0,previewBytes=0;for(const auto& tool:done.tools)if(sources.contains(tool.toolId)){const auto image=std::get<ImageFrame>(tool.outputs["image"].data);rawBytes+=image.stride()*image.size().height();previewBytes+=image.preview().sizeInBytes();}QTextStream(stdout)<<"CAPACITY n="<<n<<" type="<<typeKey(type)<<" raster=640x200 rawBytes="<<rawBytes<<" previewBytes="<<previewBytes<<" executionMs="<<done.milliseconds<<" terminalBytes="<<QJsonDocument(record).toJson(QJsonDocument::Compact).size()<<" deadlineMs=30000 retainedBudgetBytes=536870912\n";check(record["branches"].toObject().size()==n && record["frames"].toArray().size()==n,"Complete persisted source/model set");check(ProductionResultCodec::csv(records).contains("sourceSet") && ProductionResultCodec::summary(record).contains("Kaynak:"),"CSV and summary source evidence");
for(int mutation=0;mutation<8;++mutation){auto bad=record;auto branches=bad["branches"].toObject();auto frames=bad["frames"].toArray();if(mutation==0)branches.remove(sources[0]);if(mutation==1)branches["foreign"]=branches[sources[0]];if(mutation==2)frames.append(frames[0]);if(mutation==3)frames.removeLast();if(mutation==4){auto f=frames[0].toObject();f["sourceId"]="foreign";frames[0]=f;}if(mutation==5){auto branch=branches[sources[0]].toObject();branch["systemCalibration"]=QJsonObject{};branches[sources[0]]=branch;}if(mutation==6)bad["configId"]=QString(64,'0');if(mutation==7){auto tools=bad["tools"].toArray();auto tool=tools.last().toObject();tool["decision"]=tool["decision"]=="PASS" ? "FAIL":"PASS";tools[tools.size()-1]=tool;bad["tools"]=tools;}bad["branches"]=branches;bad["frames"]=frames;reject([&]{ProductionResultCodec::validate(bad);});}
const auto archivePath=dir.filePath("archive-"+QString::number(n)+typeKey(type));ProductionStore store(archivePath);store.append("cycles",record["recordId"].toString(),record);check(store.recent("cycles").size()==1,"Idempotent append same record ID");
const auto diskPath=archivePath+"/cycles/"+record["recordId"].toString()+".json";QFile disk(diskPath);check(disk.open(QIODevice::ReadOnly),"Read actual terminal archive");const auto original=disk.readAll();disk.close();auto envelope=QJsonDocument::fromJson(original).object();auto payload=envelope["payload"].toObject();payload["cycleId"]="tampered";envelope["payload"]=payload;check(disk.open(QIODevice::WriteOnly|QIODevice::Truncate),"Disk tamper fixture");disk.write(QJsonDocument(envelope).toJson());disk.close();reject([&]{store.recent("cycles");});check(disk.open(QIODevice::WriteOnly|QIODevice::Truncate),"Restore archive bytes");disk.write(original);disk.close();
RuntimeService restart(dir.filePath("archive-"+QString::number(n)+typeKey(type)),path+".calibrations.json",identity,{}, {}, {},[&](const Recipe& r){return workspace.productionSources(r);});check(!restart.active() && !restart.busy() && restart.records()==records,"Restart reads history without permission/admission");reject([&]{restart.begin();});
coordinator.start();lease=coordinator.begin(RuntimeTrigger::Manual);auto stop=RuntimeService::execute(lease,"",std::make_shared<std::atomic_bool>(true));coordinator.finish(lease.ticket,stop);check(runtime.records().size()==2,"Stopped terminal persists without invented frame");QJsonObject stoppedRecord;for(const auto& record:runtime.records())if(record["cycleId"]==stop.cycleId)stoppedRecord=record;const auto stoppedCsv=ProductionResultCodec::csv({stoppedRecord});check(stoppedCsv.contains(stop.cycleId.toUtf8()) && stoppedCsv.contains("SKIPPED"),"CSV preserves no-output terminal without inventing scalar");coordinator.stop();
if(n==2 && type==ApplicationType::Measurement){
    coordinator.start();const auto errorLease=coordinator.begin(RuntimeTrigger::Manual);check(QFile::remove(dir.filePath("source-1.png")),"Remove source after admission");const auto error=RuntimeService::execute(errorLease,"",{});check(error.state==ExecutionState::Error && error.frames.size()<n,"Partial acquisition error");coordinator.finish(errorLease.ticket,error);check(runtime.records().first()["schemaVersion"]==7,"Partial ERROR terminal archived");check(stripe(0).save(dir.filePath("source-1.png")),"Restore actual source");coordinator.stop();
    const auto historical=runtime.records();teach(workspace,contract.cameras[0],contract.setups[0],101,0);check(runtime.records()==historical,"History survives active model version change");teach(workspace,contract.cameras[0],contract.setups[0],1,0);runtime.prepare(recipe,dir.path());

    const auto archive=dir.filePath("archive-"+QString::number(n)+typeKey(type));PlcRuntimeSocket server(runtime,archive,path+".calibrations.json");PlcSocketOptions options;options.port=0;options.ackTimeoutMs=100;options.retryCount=1;options.measurementToolId=metrics[1];server.listen(options);Peer peer;peer.connectTo(server.port());
    auto trigger=[&](const QString& id){return QJsonObject{{"type","TRIGGER"},{"requestId",id},{"recipeId",recipe.id},{"configId",QString::fromLatin1(runtime.active()->validation().contentId)}};};
    auto responseFor=[&](const QString& id){for(int i=0;i<16;++i){auto response=peer.take("DONE");if(response["requestId"]==id)return response;}throw std::runtime_error("Missing correlated socket result");};
    int completed=0;server.completed=[&](const CycleResult&){++completed;};peer.send(trigger("multi"));peer.take("BUSY");const auto response=peer.take("DONE");check(response["resultValid"]==true && std::abs(response["valueMm"].toDouble()-2)<.001 && response["measurementToolId"]==metrics[1],"Socket chooses exact scalar from second source");until([&]{return server.state()=="ACK_TIMEOUT";},"Multi-source bounded ACK timeout",3000);check(completed==1,"ACK timeout never reacquires multi-source set");peer.ack("multi");peer.send(trigger("multi"));check(peer.take("DONE")==response && completed==1,"Multi-source duplicate replays exact result");
    QLockFile socketLock(archive+"/archive.lock");socketLock.setStaleLockTime(0);server.started=[&]{check(socketLock.tryLock(0),"Socket terminal lock after begin");};peer.send(trigger("retry"));peer.take("BUSY");until([&]{return server.recordingPending();},"Multi-source socket pending record");check(runtime.busy() && completed==1,"No delivery before durable multi-source terminal");socketLock.unlock();server.started={};server.retryRecording();check(responseFor("retry")["resultValid"]==true && completed==2,"Socket retries retained completion");peer.ack("retry");
    server.started=[&]{server.cancel();};peer.send(trigger("stop"));peer.take("BUSY");const auto cancelled=responseFor("stop");check(cancelled["executionState"]=="SKIPPED" && cancelled["resultValid"]==false && cancelled["valueMm"].isNull(),"Multi-source socket Stop cannot deliver old decision");peer.ack("stop");server.started={};server.stop();
}
}
QTextStream(stdout)<<checks<<" multi-source terminal checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
