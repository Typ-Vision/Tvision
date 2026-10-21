#include "application/ExperimentalOecsAcquisition.hpp"
#include "application/IndustrialCameraConfiguration.hpp"
#include "application/IndustrialRuntime.hpp"
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
template<class F> void until(F f){Deadline d(5000);while(!f()){check(!d.expired(),"controller timeout");QThread::msleep(1);}}
void command(CameraWorkerSession& worker,CameraOperation op,CameraState desired){worker.submit(op,3000);until([&]{worker.poll();return worker.lifecycle().state()==desired || worker.lifecycle().state()==CameraState::Fault;});}
void start(CameraWorkerSession& w){command(w,CameraOperation::Connect,CameraState::Connected);if(w.lifecycle().state()==CameraState::Fault)return;command(w,CameraOperation::Configure,CameraState::Configured);if(w.lifecycle().state()!=CameraState::Fault)command(w,CameraOperation::Start,CameraState::Streaming);}
void close(CameraWorkerSession& w){w.requestShutdown();until([&]{return w.shutdownState()==ShutdownState::Ready;});}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(makeOecsCApi()->softwareTriggerCorrelation()==OecsTriggerCorrelation::Unavailable,"native trusted capability stays unavailable");
    CameraProviderFactory native;check(!native.capabilities(request().device).softwareTrigger && !native.capabilities(request().device).preview,"production factory unchanged");
    {auto r=request();r.profile.requested.trigger.selector="FrameStart";rejects([&]{ExperimentalOecsAcquisition::open(r,"C:/fake.dll");});}
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
