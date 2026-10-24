#pragma once
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
namespace oecs_experimental_fixture {
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
