#include "application/ExperimentalOecsAcquisition.hpp"
#include "application/CameraPreviewSession.hpp"
#include "application/IndustrialCameraConfiguration.hpp"
#include "application/IndustrialRuntime.hpp"
#include "application/EditorSession.hpp"
#include "application/ProjectService.hpp"
#include "application/ExperimentalRuntime.hpp"
#include "application/RuntimeCoordinator.hpp"
#include "application/ProductionResultCodec.hpp"
#include "application/PlcRuntimeSocket.hpp"
#include <QLockFile>
#include <QJsonArray>
#include "vision/ImageAccess.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <future>
using namespace visionflow;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> void rejects(F f){try{f();}catch(const std::exception&){return;}throw std::runtime_error("expected rejection");}
struct State {
    QMap<QString,CameraFeature> nodes;QStringList calls;QByteArray pixels=QByteArray(2448*2048,char(17));
    bool held=false,running=false,metadata=false,wrongIdentity=false,writeFailed=false,dynamicAuto=false;QString failure;
    std::function<void()> waitHook;
};
IndustrialCameraRequest request(){
    IndustrialCameraRequest r;r.device={{"Opto Engineering Srl","COE-050-M-USB-050-IR-C","SN",DeviceIdentityConfidence::SerialReported},{"oecs",{},"usb","exact"},{"oecs","test",QString(64,'a'),1}};
    r.camera={{"camera"},"Synthetic OECS",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},r.device.identity};
    GeometryDescriptor g;g.resolution={2448,2048};g.sensorRoi=QRect(0,0,2448,2048);g.binning=QSize(1,1);g.decimation=QSize(1,1);g.reverse=GeometryMirror{};g.complete=true;
    r.setup={{"setup"},r.camera.id,"Synthetic setup","lens","plane",1,1,g};
    r.profile={{"profile"},r.camera.id,r.setup.id,"Synthetic experiment",1,{"Mono8",{true,"FrameBurstStart","Software"},AutoMode::Off,5000,AutoMode::Off,0,{}}};r.sourceId="preview";return r;
}
std::shared_ptr<State> state(){auto s=std::make_shared<State>();const auto r=request();
    const auto add=[&](QString n,FeatureValue v,FeatureType type){CameraFeature f;f.name=n;f.type=type;f.value=v;f.access=FeatureAccess::ReadWrite;f.minimum=0;f.maximum=100000000;f.increment=type==FeatureType::Integer?std::optional<double>(1):std::nullopt;s->nodes[n]=f;};
    for(auto n:{"DeviceVendorName","DeviceModelName","DeviceSerialNumber"})add(n,QString(n==QString("DeviceVendorName")?r.device.identity.vendor:n==QString("DeviceModelName")?r.device.identity.model:r.device.identity.serial),FeatureType::String);
    for(auto n:{"SensorWidth","Width"})add(n,qint64(2448),FeatureType::Integer);for(auto n:{"SensorHeight","Height"})add(n,qint64(2048),FeatureType::Integer);
    add("PayloadSize",qint64(2448*2048),FeatureType::Integer);for(auto n:{"OffsetX","OffsetY"})add(n,qint64(0),FeatureType::Integer);
    for(auto n:{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical","AcquisitionBurstFrameCount"})add(n,qint64(1),FeatureType::Integer);
    for(auto n:{"ReverseX","ReverseY","TriggerCacheEnable","FrameSpecInfo"})add(n,false,FeatureType::Boolean);
    for(auto n:{"ExposureTime","Gain","TriggerDelay"}){add(n,n==QString("ExposureTime")?5000.:0.,FeatureType::Float);s->nodes[n].unit=n==QString("Gain")?"dB":"us";}
    const auto enumeration=[&](QString n,QString v,QStringList entries){add(n,v,FeatureType::Enumeration);for(int i=0;i<entries.size();++i)s->nodes[n].entries[entries[i]]=i;};
    enumeration("PixelFormat","Mono8",{"Mono8"});enumeration("TriggerSelector","FrameBurstStart",{"FrameBurstStart"});
    enumeration("TriggerMode","Off",{"Off","On"});enumeration("TriggerSource","Line0",{"Line0","Software"});
    enumeration("AcquisitionMode","Continuous",{"Continuous"});for(auto n:{"ExposureAuto","GainAuto"})enumeration(n,"Off",{"Off"});
    enumeration("FrameSpecInfoSelector","Timestamp",{"Timestamp","FrameCounter","TriggerCounter"});return s;
}
class Api:public OecsCApi {
    std::shared_ptr<State> s;
public:
    explicit Api(std::shared_ptr<State> x):s(std::move(x)){}
    bool load(const QString&,QString&) override{return true;}bool unload() override{check(!s->held&&!s->running,"SDK resources leaked");return true;}
    quint32 sdkVersion() override{return 1;}
    int enumerateUsb(QVector<OecsDeviceInfo>& d) override{const auto id=request().device.identity;d={{id.vendor,id.model,s->wrongIdentity?"OTHER":id.serial}};return 0;}
    int createHandle(int,void** h) override{*h=this;return 0;}int openExclusive(void*) override{return 0;}
    int closeDevice(void*) override{return 0;}int destroyHandle(void*) override{s->held=s->running=false;return 0;}
    int readFeature(void*,const QString& n,CameraFeature& f) override{if(!s->nodes.contains(n))return -1;f=s->nodes[n];if(n=="FrameSpecInfo")f.value=s->metadata;
        if(s->dynamicAuto && (n=="ExposureAuto" || n=="GainAuto") && s->nodes["TriggerMode"].value==FeatureValue(QString("On"))){f.access=FeatureAccess::Unavailable;f.type=FeatureType::Unsupported;}return 0;}
    int writeFeature(void*,const QString& n,const FeatureValue& v) override{s->nodes[n].value=v;if(s->failure=="configure:"+n && !s->writeFailed){s->writeFailed=true;return -1;}return 0;}
    int setBufferCount(void*,quint32) override{return 0;}
    int startStream(void*) override{s->calls<<"start";s->running=true;return 0;}
    int stopStream(void*) override{s->calls<<"stop";s->running=false;return s->failure=="stop"?-1:0;}
    int flushStream(void*) override{check(s->running,"Clear must precede Stop");s->calls<<"clear";return 0;}
    int executeSoftwareTrigger(void*) override{s->calls<<"command";return s->failure=="command"?-1:0;}
    int waitBuffer(void*,int ms,OecsBuffer& b) override{
        check(ms>0 && ms<=3000,"bounded monotonic wait");s->calls<<"wait";if(s->waitHook)s->waitHook();
        if(s->failure=="timeout")return -1;
        // Simulated old delayed command survives Clear/Stop/Start. No epoch proof.
        b={s->pixels.constData(),quint64(s->pixels.size()),2448,7,123,2448,2048,0x01080001,true,false,false};
        if(s->failure=="geometry")b.height=2047;if(s->failure=="incomplete")b.complete=false;
        b.triggerEvidence=true;b.triggerGeneration=999;b.triggerCounter=42; // Must NEVER promote policy.
        s->held=true;return 0;
    }
    int releaseBuffer(void*) override{s->calls<<"release";s->held=false;s->pixels.fill(char(99));return 0;}
};
std::unique_ptr<CameraWorkerSession> open(std::shared_ptr<State> s){return ExperimentalOecsAcquisition::open(request(),QDir::temp().absoluteFilePath("experimental.dll"),[s]{return std::make_unique<Api>(s);});}
template<class F> void until(F f,const char* why="controller timeout"){Deadline d(5000);while(!f()){check(!d.expired(),why);QThread::msleep(1);}}
void command(CameraWorkerSession& worker,CameraOperation op,CameraState desired){worker.submit(op,3000);until([&]{worker.poll();return worker.lifecycle().state()==desired || worker.lifecycle().state()==CameraState::Fault;});}
void start(CameraWorkerSession& w){command(w,CameraOperation::Connect,CameraState::Connected);if(w.lifecycle().state()==CameraState::Fault)return;command(w,CameraOperation::Configure,CameraState::Configured);if(w.lifecycle().state()!=CameraState::Fault)command(w,CameraOperation::Start,CameraState::Streaming);}
void close(CameraWorkerSession& w){w.requestShutdown();until([&]{return w.shutdownState()==ShutdownState::Ready;});}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(makeOecsCApi()->softwareTriggerCorrelation()==OecsTriggerCorrelation::Unavailable,"native trusted capability stays unavailable");
    CameraProviderFactory native;check(!native.capabilities(request().device).softwareTrigger && !native.capabilities(request().device).preview,"production factory unchanged");
    {auto r=request();r.profile.requested.trigger.selector="FrameStart";rejects([&]{ExperimentalOecsAcquisition::open(r,"C:/fake.dll");});}
    {auto s=state();CameraPreviewSession preview;
        preview.start([s]{return open(s);},true,3000);
        until([&]{preview.poll();return preview.stats().completed>=2 || preview.state()==PreviewState::Error;});
        auto frame=preview.takeLatest();
        check(frame && frame->kind==PreviewAcquisitionKind::SequentialSingleFrame,"OECS preview remains sequential");
        const auto evidence=frame->frame.image.metadata().experimentalAcquisition;
        check(evidence["productionAdmission"]==false && evidence["triggerAssociationVerified"]==false &&
              evidence["profileSnapshot"].toObject()==request().profile.toJson(),"preview preserves explicit FrameBurstStart and experimental provenance");
        preview.stop();until([&]{preview.poll();return preview.drained();});
        check(!preview.takeLatest() && preview.state()==PreviewState::Idle && !s->running && !s->held,"OECS preview Stop drains resources without late raster");
    }
    {auto s=state();auto r=request();r.profile.requested.exposureAuto=AutoMode::Continuous;rejects([&]{ExperimentalOecsAcquisition::open(r,"C:/fake.dll",[s]{return std::make_unique<Api>(s);});});}
    {auto s=state();auto worker=open(s);rejects([&]{open(s);});start(*worker);check(worker->lifecycle().state()==CameraState::Streaming,"experimental configured");
        command(*worker,CameraOperation::Grab,CameraState::Streaming);auto frame=worker->take();check(frame.has_value(),"owned experimental frame");
        const auto evidence=frame->image.metadata().experimentalAcquisition;
        check(evidence["triggerAssociationVerified"]==false && evidence["productionAdmission"]==false && !evidence.contains("arm"),"old/fake proof never upgraded");
        IndustrialExecutionEvidence completion;completion.request=request();completion.acquired=frame->image.metadata();
        rejects([&]{industrialCompletionEvidence(completion);});
        check(frame->image.pixelText(0,0).contains("17"),"owned pixels survive buffer release");
        check(s->calls==QStringList{"start","clear","command","wait","release","clear","stop"},"Clear while running then Stop");
        QTemporaryDir directory(QDir::currentPath()+"/experimental-save-XXXXXX");check(directory.isValid(),"evidence temp directory");const auto path=directory.filePath("frame.pgm");
        close(*worker);ExperimentalOecsAcquisition::save(*frame,path);
        rejects([&]{ImageFileService{}.load(path);}); // No silent replay as an ordinary source.
        QFile file(path+".json");check(file.open(QIODevice::ReadOnly),"sidecar exists");auto saved=QJsonDocument::fromJson(file.readAll()).object();
        check(saved["contentId"]==QString::fromLatin1(frame->image.contentId().toHex()) && saved["triggerAssociationVerified"]==false,"saved hash and unverified provenance");
        file.close();
        const auto replay=ImageFileService{}.load(path,true);
        check(replay.contentId()==frame->image.contentId() && replay.metadata().experimentalAcquisition==saved,"explicit replay preserves exact owned raster and evidence");
        int replayIndex=0;
        for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
            ProjectService project;project.createProject("Experimental","",directory.filePath(QString("replay-%1.vfproject").arg(replayIndex++)),"file");project.createRecipe("Replay","",type,"file");
            const auto source=project.setImageFile(path);project.enableMultiSource();const auto blob=project.addTool("blob");project.setConnections({{source,"image",blob,"image"}});
            project.save();const auto recipe=project.activeRecipe()->id;const auto version=project.activeRecipe()->recipeVersion;const auto definition=project.activeRecipe()->definition;
            project.openProject(project.path());project.selectRecipe(recipe);project.save();check(project.activeRecipe()->recipeVersion==version && project.activeRecipe()->definition==definition,"experimental replay no-op reopen");
            {
                const auto archive=project.path()+".production";RuntimeService runtime(archive,{},RuntimeIdentity::localEngineer());runtime.setExperimental(true);RuntimeCoordinator coordinator(runtime);
                rejects([&]{coordinator.load(*project.activeRecipe(),directory.path(),RuntimeTrigger::SoftwareTrigger);});
                coordinator.load(*project.activeRecipe(),directory.path());check(!runtime.active() && coordinator.state()==RuntimeState::LoadedStopped,"experimental Load grants no admission");coordinator.start();
                check(runtime.active()->validation().scope=="experimental_runtime/1","separate experimental admission");
                rejects([&]{runtime.validate(*project.activeRecipe(),directory.path(),replay);});rejects([&]{runtime.activate(*project.activeRecipe(),directory.path());});rejects([&]{runtime.rollback("foreign-production-history");});
                {PlcRuntimeSocket server(runtime,archive,{});PlcSocketOptions options;options.port=0;server.listen(options);QTcpSocket peer;peer.connectToHost(QHostAddress::LocalHost,server.port());QByteArray input;until([&]{QCoreApplication::processEvents();input+=peer.readAll();return input.contains('\n');},"experimental socket HELLO timeout");auto hello=QJsonDocument::fromJson(input.left(input.indexOf('\n'))).object();check(hello["ready"]==false,"experimental socket never READY");input.clear();peer.write(QJsonDocument(QJsonObject{{"v",2},{"sessionId",server.sessionId()},{"type","TRIGGER"},{"requestId","experimental-block"},{"recipeId",recipe},{"configId",QString::fromLatin1(runtime.active()->validation().contentId)}}).toJson(QJsonDocument::Compact)+'\n');until([&]{QCoreApplication::processEvents();input+=peer.readAll();return input.contains('\n');},"experimental socket rejection timeout");check(input.contains("EXPERIMENTAL_OUTPUT_BLOCKED"),input.constData());check(!server.working() && !server.blocking() && !runtime.busy() && !input.contains("\"type\":\"DONE\""),"PLC trigger cannot acquire/publish experimental scalar");server.stop();}
                rejects([&]{runtime.begin({{"plc",true}});});const auto lease=coordinator.begin(RuntimeTrigger::Manual);
                auto result=RuntimeService::execute(lease,{},coordinator.cancellation());check(result.state==ExecutionState::Done && result.industrialEvidence.isEmpty(),"experimental Runtime actual replay");
                rejects([&]{coordinator.begin(RuntimeTrigger::Manual);});check(coordinator.state()==RuntimeState::Running,"busy rejection preserves Running");rejects([&]{runtime.setExperimental(false);});
                auto changed=result;changed.frames[0].experimentalAcquisition["productionAdmission"]=true;rejects([&]{runtime.finish(lease.ticket,changed);});
                auto editorResult=ExecutionService{}.withExperimentalEditorReplay().run(definition,directory.path(),recipe,lease.snapshot->validation().contentId,{});rejects([&]{runtime.finish(lease.ticket,editorResult);});
                QDir().mkpath(archive);QLockFile lock(archive+"/archive.lock");check(lock.tryLock(0),"fixture archive lock");rejects([&]{coordinator.finish(lease.ticket,result);});check(coordinator.recordingPending() && runtime.busy(),"experimental persistence failure blocks admission");lock.unlock();
                changed=result;changed.error="changed pending completion";rejects([&]{coordinator.finish(lease.ticket,changed);});coordinator.finish(lease.ticket,result);check(!runtime.busy(),"retry uses unchanged completion");
                const auto records=ProductionStore(archive).recent("experimental_cycles");check(records.size()==1 && ProductionStore(archive).recent("cycles").isEmpty(),"experimental archive isolated from trusted terminal");
                const auto record=records.front();ExperimentalRuntime::validateRecord(record);rejects([&]{ProductionResultCodec::validate(record);});check(ExperimentalRuntime::csv(records).contains("BLOCKED_EXPERIMENTAL") && ExperimentalRuntime::summary(record).contains(ExperimentalOecsAcquisition::notice()),"offline Logs/CSV preserve experimental state");
                for(const auto& field:{"productionAdmission","triggerAssociationVerified","communicationState","schema","configId"}){auto bad=record;bad[field]=true;rejects([&]{ExperimentalRuntime::validateRecord(bad);});}
                auto bad=record;auto frames=bad["frames"].toArray();auto frame=frames[0].toObject();frame["experimentalAcquisition"]=QJsonObject{};frames[0]=frame;bad["frames"]=frames;rejects([&]{ExperimentalRuntime::validateRecord(bad);});
                const auto stopLease=coordinator.begin(RuntimeTrigger::Manual);auto stopped=RuntimeService::execute(stopLease,{},coordinator.cancellation());coordinator.stop();RuntimeService::suppressCancelled(stopLease,stopped);check(stopped.frames.isEmpty(),"late Stop clears experimental frames");coordinator.finish(stopLease.ticket,stopped);check(coordinator.state()==RuntimeState::LoadedStopped,"Stop returns passive");
                RuntimeService restarted(archive,{},RuntimeIdentity::localEngineer());check(!restarted.experimental() && !restarted.active() && ProductionStore(archive).recent("experimental_cycles").size()==2,"restart reads history without permission");rejects([&]{restarted.begin();});
                coordinator.start();const auto errorLease=coordinator.begin(RuntimeTrigger::Manual);check(QFile::rename(path,path+".hidden"),"hide source after begin");auto failed=RuntimeService::execute(errorLease,{},coordinator.cancellation());check(QFile::rename(path+".hidden",path),"restore source");check(failed.state==ExecutionState::Error,"real acquisition error");coordinator.finish(errorLease.ticket,failed);check(coordinator.state()==RuntimeState::Fault,"experimental failure closes admission");rejects([&]{coordinator.begin(RuntimeTrigger::Manual);});
            }
            EditorSession editor;const EditorContext ctx{project.project().id,project.path(),recipe,definition,{}};const auto signature=editor.signature(ctx);
            auto execution=ExecutionService{}.withExperimentalEditorReplay();const auto ticket=editor.begin(SessionJob::Execution);
            auto cycle=execution.run(definition,directory.path(),recipe,signature,{});check(cycle.state==ExecutionState::Done && cycle.industrialEvidence.isEmpty() && editor.acceptCycle(cycle,signature,ctx),"three application types share experimental flow");editor.finish(ticket);
            auto target=execution.runTool(definition,directory.path(),recipe,signature,{},blob,editor.evaluation(ctx,blob));
            check(target.state==ExecutionState::Done && target.acquisitions==0 && target.tools.front().outputs["area"].origin.experimentalAcquisition==saved,"TargetOnly retains experimental origin without acquisition");
            {QFile sidecar(path+".json");check(sidecar.open(QIODevice::WriteOnly|QIODevice::Truncate),"change cached evidence");auto changed=saved;changed["notice"]="changed fixture evidence";sidecar.write(QJsonDocument(changed).toJson());sidecar.close();
                check(editor.evaluation(ctx,blob).cache.isEmpty(),"JSON change invalidates source and dependent TargetOnly cache");
                check(sidecar.open(QIODevice::WriteOnly|QIODevice::Truncate),"restore cached evidence");sidecar.write(QJsonDocument(saved).toJson());}
            auto calibrationContext=ctx;calibrationContext.systemCalibration={{source,QJsonObject{{"revision",2}}}};
            check(editor.evaluation(calibrationContext,blob).cache.isEmpty(),"calibration identity change invalidates experimental cache");
            check(ExecutionService{}.runTool(definition,directory.path(),recipe,signature,{},blob,editor.evaluation(ctx,blob)).state==ExecutionState::Error,"default policy denies experimental cached input");
            const auto pending=editor.begin(SessionJob::Execution);editor.stop(SessionJob::Execution);check(editor.acceptCycle(cycle,signature,ctx) && !editor.frame && editor.cycle->state==ExecutionState::Skipped,"Stop suppresses late experimental cycle");editor.finish(pending);
            check(ExecutionService{}.run(definition,directory.path(),recipe,signature,{}).state==ExecutionState::Error,"default execution denies marked replay");
            ExecutionService injected([&](const ToolDefinition& t)->std::unique_ptr<AcquisitionAdapter>{if(t.kind=="image_file")return std::make_unique<ImageFileAdapter>(path,true);return {};});
            check(injected.run(definition,directory.path(),recipe,signature,{}).state==ExecutionState::Error,"injected experimental pixels cannot bypass default execution policy");
        }
        {
            ProjectService project;project.createProject("Two source experimental","",directory.filePath("two-source.vfproject"),"file");project.createRecipe("Two files","",ApplicationType::Measurement,"file");
            const auto first=project.setImageFile(path);project.enableMultiSource();const auto second=project.addTool("image_file");for(auto t:project.tools())if(t.id==second){t.parameters["path"]=path;project.updateTool(t);}const auto blob=project.addTool("blob");project.setConnections({{second,"image",blob,"image"}});project.save();
            RuntimeService runtime(project.path()+".production",{},RuntimeIdentity::localEngineer());runtime.setExperimental(true);runtime.prepare(*project.activeRecipe(),directory.path());const auto lease=runtime.begin();const auto result=RuntimeService::execute(lease,{},{});check(result.state==ExecutionState::Done && result.frames.size()==2 && result.frameId.isEmpty(),"experimental Runtime has distinct N source frames");runtime.finish(lease.ticket,result);const auto record=ProductionStore(project.path()+".production").recent("experimental_cycles").front();
            auto bad=record;auto frames=bad["frames"].toArray();frames[1]=frames[0];bad["frames"]=frames;rejects([&]{ExperimentalRuntime::validateRecord(bad);});
            bad=record;auto tools=bad["tools"].toArray();for(int i=0;i<tools.size();++i){auto t=tools[i].toObject();if(t["toolId"]!=blob)continue;auto outputs=t["outputs"].toObject();auto area=outputs["area"].toObject();area["value"]=123456.;outputs["area"]=area;t["outputs"]=outputs;tools[i]=t;}bad["tools"]=tools;rejects([&]{ExperimentalRuntime::validateRecord(bad);});
            runtime.setExperimental(false);check(!runtime.active(),"policy change discards admission");rejects([&]{runtime.begin();});
            runtime.setExperimental(true);const auto circle=project.addTool("circle_fit");for(auto t:project.tools())if(t.id==circle){t.parameters["mode"]="calibrated";project.updateTool(t);}project.setConnections({{second,"image",blob,"image"},{first,"image",circle,"image"}});project.save();rejects([&]{runtime.prepare(*project.activeRecipe(),directory.path());});
        }
        {IndustrialExecutionInputs inputs;inputs.leaseUse=CameraLeaseUse::Runtime;rejects([&]{acquireIndustrial(inputs,request(),"cycle","frame",1000,{},true);});}
        auto writeEvidence=[&](const QJsonObject& e){QFile f(path+".json");check(f.open(QIODevice::WriteOnly|QIODevice::Truncate),"rewrite fixture evidence");f.write(QJsonDocument(e).toJson());};
        for(const auto& field:QStringList{"triggerAssociationVerified","productionAdmission","contentId","width","serial","schema","readback","profileSnapshot"}){
            auto bad=saved;if(field=="triggerAssociationVerified" || field=="productionAdmission")bad[field]=true;else if(field=="width")bad[field]=1;else if(field=="readback" || field=="profileSnapshot")bad[field]=QJsonObject{{"schemaVersion",1}};else bad[field]=field=="serial"?QString{}:QString("tampered");
            writeEvidence(bad);rejects([&]{ImageFileService{}.load(path,true);});
        }
        writeEvidence(saved);check(QFile::rename(path+".json",path+".backup"),"hide fixture evidence");rejects([&]{ImageFileService{}.load(path,true);});check(QFile::rename(path+".backup",path+".json"),"restore fixture evidence");
        QFile raster(path);check(raster.open(QIODevice::ReadOnly),"read fixture raster");auto bytes=raster.readAll();raster.close();bytes[bytes.size()-1]=char(bytes.back()^1);check(raster.open(QIODevice::WriteOnly|QIODevice::Truncate),"tamper fixture raster");raster.write(bytes);raster.close();rejects([&]{ImageFileService{}.load(path,true);});
        rejects([&]{ExperimentalOecsAcquisition::save(*frame,path);});
        auto next=open(s);start(*next);check(next->lifecycle().state()==CameraState::Streaming,"manual reopen after shutdown");close(*next);
    }
    for(const auto& failure:QStringList{"command","timeout","geometry","incomplete","stop"}){auto s=state();s->failure=failure;auto w=open(s);start(*w);command(*w,CameraOperation::Grab,CameraState::Streaming);check(w->lifecycle().state()==CameraState::Fault && !w->take(),"failed grab yields no frame");rejects([&]{w->submit(CameraOperation::Grab);});close(*w);}
    for(int scenario=0;scenario<4;++scenario){auto s=state();if(scenario==0)s->metadata=true;if(scenario==1)s->wrongIdentity=true;if(scenario==2)s->nodes["TriggerSelector"].value=QString("FrameStart");if(scenario==3)s->nodes["Gain"].unit="unknown";
        auto w=open(s);start(*w);check(w->lifecycle().state()==CameraState::Fault && !s->calls.contains("command"),"invalid configuration/identity/metadata rejected");close(*w);
    }
    {auto s=state();s->nodes["TriggerDelay"].value=500000.;s->nodes["AcquisitionBurstFrameCount"].value=qint64(3);s->nodes["TriggerCacheEnable"].value=true;
        auto w=open(s);start(*w);check(w->lifecycle().state()==CameraState::Streaming && s->nodes["TriggerDelay"].value==FeatureValue(0.) && s->nodes["AcquisitionBurstFrameCount"].value==FeatureValue(qint64(1)) && s->nodes["TriggerCacheEnable"].value==FeatureValue(false),"delay/burst/cache writes and readbacks");close(*w);
    }
    for(bool valid:{true,false}){auto s=state();for(const auto& n:QStringList{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"}){
            auto& f=s->nodes[n];f.type=FeatureType::Enumeration;f.value=n+"2";f.entries={{n+"1",valid?1u:0u},{n+"2",2}};
        }auto w=open(s);start(*w);check(w->lifecycle().state()==(valid?CameraState::Streaming:CameraState::Fault),"documented enum factor semantics; wrong raw value rejected");close(*w);
    }
    for(int scenario=0;scenario<3;++scenario){auto s=state();for(const auto& n:QStringList{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"}){
            s->nodes[n].access=scenario==1?FeatureAccess::Unavailable:FeatureAccess::Missing;s->nodes[n].type=FeatureType::Unsupported;
        }if(scenario==2)s->nodes["BinningVertical"].access=FeatureAccess::Unavailable;
        auto w=open(s);start(*w);check(w->lifecycle().state()==(scenario==0?CameraState::Streaming:CameraState::Fault),"explicit NI pair is not-applicable; NA/mixed never default");
        if(scenario==0)check(w->readback()->geometry.binningEvidence==GeometryEvidence::NotApplicable,"NI is never reported factor readback");close(*w);
    }
    {auto s=state();s->dynamicAuto=true;s->nodes["TriggerMode"].value=QString("On");auto w=open(s);start(*w);check(w->lifecycle().state()==CameraState::Streaming,"auto controls verified Off before dynamic trigger gating");close(*w);}
    for(const auto& node:QStringList{"AcquisitionBurstFrameCount","TriggerCacheEnable","TriggerDelay"}){
        auto s=state();s->nodes["TriggerDelay"].value=500000.;s->nodes["AcquisitionBurstFrameCount"].value=qint64(3);s->nodes["TriggerCacheEnable"].value=true;
        s->failure="configure:"+node;auto w=open(s);start(*w);check(w->lifecycle().state()==CameraState::Fault && s->writeFailed && s->nodes["TriggerDelay"].value==FeatureValue(500000.) && s->nodes["AcquisitionBurstFrameCount"].value==FeatureValue(qint64(3)) && s->nodes["TriggerCacheEnable"].value==FeatureValue(true),"partial experimental write rolls back original values");close(*w);
    }
    {auto s=state();std::promise<void> entered,release;auto released=release.get_future().share();s->waitHook=[&]{entered.set_value();released.wait();};
        auto w=open(s);start(*w);w->submit(CameraOperation::Grab,3000);entered.get_future().wait();w->requestStop();w->requestShutdown();check(!w->take(),"Stop clears preview immediately");
        rejects([&]{open(s);});release.set_value();until([&]{return w->shutdownState()==ShutdownState::Ready;});check(!w->take(),"late/cancelled frame suppressed");
    }
    {auto s=state();s->waitHook=[] {QThread::msleep(20);};auto w=open(s);start(*w);w->submit(CameraOperation::Grab,1);until([&]{w->poll();return w->lifecycle().state()==CameraState::Fault;});check(!w->take(),"expired frame suppressed");close(*w);}
    QTextStream(stdout)<<"PASS: experimental configuration/identity/metadata, owned delayed-old raster stays unverified, save/hash, lease, Stop/late/cancel, timeout/failure, manual reopen, native isolation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
