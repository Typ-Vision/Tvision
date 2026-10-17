#include "application/IndustrialSoftwareCapture.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <deque>
#include <thread>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Event {quint64 epoch;quint32 frame,trigger;bool proof=true;};
struct Hardware {
    QStringList calls;
    std::deque<Event> events;
    quint64 epoch=0;
    quint32 frame=10,trigger=20;
    QString scenario;
    QByteArray pixels=QByteArray::fromHex("010203040506");
    bool held=false,running=false;
    int commands=0,waitMs=0,readyMs=0;
    std::function<void(const QString&)> hook;
    void call(const QString& name){calls<<name;if(hook)hook(name);}
};
class Fake final: public OecsCApi {
public:
    explicit Fake(std::shared_ptr<Hardware> h):h(h){}
    bool load(const QString&,QString&) override{return true;}
    bool unload() override{check(!h->held&&!h->running,"resources at unload");return true;}
    quint32 sdkVersion() override{return 1;}
    int enumerateUsb(QVector<OecsDeviceInfo>& d) override{d={{"VF","CAM","TRIGGER"}};return 0;}
    int createHandle(int,void** p) override{*p=this;return 0;}
    int openExclusive(void*) override{return 0;}
    int closeDevice(void*) override{return 0;}
    int destroyHandle(void*) override{h->held=h->running=false;return 0;}
    OecsTriggerCorrelation softwareTriggerCorrelation() const override {
        return h->scenario=="no-correlation"?OecsTriggerCorrelation::Unavailable:OecsTriggerCorrelation::ActualCounterEpoch;
    }
    int readFeature(void*,const QString& n,CameraFeature& f) override{
        h->call("read:"+n);f={};f.name=n;f.access=FeatureAccess::ReadOnly;
        if(n=="Width"||n=="Height"||n=="PayloadSize"){
            f.type=FeatureType::Integer;f.value=qint64(n=="Width"?3:n=="Height"?2:6);
        }else{
            f.type=FeatureType::Enumeration;
            f.value=QString(n=="TriggerSelector"?"FrameStart":n=="TriggerMode"?"On":n=="TriggerSource"?"Software":"Mono8");
            if(h->scenario=="external"&&n=="TriggerSource")f.value=QString("Line0");
        }return 0;
    }
    int setBufferCount(void*,quint32) override{h->call("pool");return 0;}
    int startStream(void*) override{h->running=true;h->call("start");return 0;}
    int stopStream(void*) override{h->running=false;h->call("stop");return h->scenario=="stop-error"?-1:0;}
    int flushStream(void*) override{h->events.clear();h->call("flush");return 0;}
    int prepareSoftwareTrigger(void*,quint64 epoch,int ms,OecsTriggerArm& arm) override{
        h->call("ready");h->readyMs=ms;
        if(h->scenario=="unsupported")return oecs::errorUnsupported;
        // Fake hardware has independently maintained counters and a quiescence barrier.
        // A production backend must establish these using device-specific event semantics.
        h->epoch=epoch;arm={epoch,h->frame,h->trigger};
        if(h->scenario=="wrong-arm")arm.generation=epoch-1;
        return 0;
    }
    int executeSoftwareTrigger(void*) override{
        ++h->commands;h->call("command");
        if(h->scenario=="command-error")return -1;
        ++h->frame;++h->trigger;
        Event event{h->epoch,h->frame,h->trigger};
        if(h->scenario=="old")--event.frame;
        if(h->scenario=="duplicate-trigger")--event.trigger;
        if(h->scenario=="late-epoch")--event.epoch;
        if(h->scenario=="future-epoch")++event.epoch;
        if(h->scenario=="reset")event.frame=0;
        if(h->scenario=="missing-proof")event.proof=false;
        if(h->scenario!="timeout")h->events.push_back(event);
        return 0;
    }
    int waitBuffer(void*,int ms,OecsBuffer& b) override{
        h->waitMs=ms;h->call("wait");
        if(h->events.empty())return -1;
        const auto event=h->events.front();h->events.pop_front();
        b={h->pixels.constData(),6,3,event.frame,123,3,2,0x01080001,true,false,false};
        b.triggerEvidence=event.proof;b.triggerGeneration=event.epoch;b.triggerCounter=event.trigger;
        h->held=true;return 0;
    }
    int releaseBuffer(void*) override{
        check(h->held,"release without buffer");h->call("release");h->held=false;h->pixels.fill(char(99));
        return h->scenario=="release-error"?-1:0;
    }
private:std::shared_ptr<Hardware> h;
};
std::unique_ptr<OecsSession> open(std::shared_ptr<Hardware> h){
    auto session=std::make_unique<OecsSession>(std::make_unique<Fake>(h),QDir::temp().absoluteFilePath("trigger.dll"));
    session->open({"VF","CAM","TRIGGER"});return session;
}
void noResult(const IndustrialTriggerResult& r){check(r.acquisition.state!=AcquisitionState::Done&&!r.acquisition.frame&&!r.evidence,"false DONE/evidence");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    {
        // Both the SDK build and the SDK-less implementation fail closed by default.
        auto native=makeOecsCApi();
        check(native->softwareTriggerCorrelation()==OecsTriggerCorrelation::Unavailable,"native does not advertise diagnostic trigger as admission");
        auto h=std::make_shared<Hardware>();h->scenario="no-correlation";auto s=open(h);
        check(s->softwareTriggerCorrelation()==OecsTriggerCorrelation::Unavailable,"session preserves unavailable capability");
        for(int n=0;n<2;++n) {
            auto r=IndustrialSoftwareCapture::capture(*s,"source");noResult(r);
            check(r.acquisition.state==AcquisitionState::Error&&!r.acquisition.error.isEmpty(),"explicit unsupported acquisition error");
        }
        bool copied=false,rejected=false;
        try{s->softwareFrame({},[]{return 1000;},[&](const OecsBuffer&){copied=true;});}
        catch(const OecsSessionError& e){rejected=e.failure()==OecsFailure::Unavailable&&e.stage()=="trigger-correlation"&&e.code()==oecs::errorUnsupported;}
        check(rejected&&!copied&&h->calls.isEmpty()&&h->commands==0,"direct and application rejection precede feature/pool/start/flush/command/wait");
        check(s->stage()==OecsSessionStage::Open,"unsupported preflight leaves idle session usable");
        s->close();
        bool closedRejected=false;
        try{s->softwareTriggerCorrelation();}catch(const OecsSessionError& e){closedRejected=e.failure()==OecsFailure::InvalidState;}
        check(closedRejected,"capability query respects session lifetime");
    }
    {
        auto h=std::make_shared<Hardware>();auto s=open(h);
        h->events.push_back({0,1,1}); // preview residue must be drained
        auto a=IndustrialSoftwareCapture::capture(*s,"source");
        check(a.acquisition.state==AcquisitionState::Done&&a.evidence&&h->commands==1,"single command DONE");
        const auto first=*a.acquisition.frame;
        check(first.image.pixelText(2,1)=="Gray: 6"&&first.image.metadata().sourceId=="source"&&!first.image.metadata().exposureAtUtc,"owned actual frame and metadata");
        check(a.evidence->actual.hardwareFrameId==11&&a.evidence->triggerCounter==21,"actual hardware evidence");
        h->pixels=QByteArray::fromHex("010203040506");
        auto b=IndustrialSoftwareCapture::capture(*s,"source");
        check(b.acquisition.state==AcquisitionState::Done&&b.evidence->arm.generation!=a.evidence->arm.generation,"new generation");
        check(first.image.contentId()==b.acquisition.frame->image.contentId(),"equal pixels allowed for fresh counters");
        check(first.cycleId!=b.acquisition.frame->cycleId&&first.frameId!=b.acquisition.frame->frameId,"private admission identities");
        check(h->calls.indexOf("start")<h->calls.indexOf("ready")&&h->calls.indexOf("ready")<h->calls.indexOf("command")&&h->calls.indexOf("command")<h->calls.indexOf("wait"),"ready before trigger before wait");
        s->close();s->open({"VF","CAM","TRIGGER"});h->frame=0;h->trigger=0;
        auto c=IndustrialSoftwareCapture::capture(*s,"source");
        check(c.acquisition.state==AcquisitionState::Done&&c.evidence->arm.generation>b.evidence->arm.generation,"reopen reset uses new epoch");s->close();
    }
    {
        auto h=std::make_shared<Hardware>();h->frame=h->trigger=0xFFFFFFFFu;auto s=open(h);
        auto r=IndustrialSoftwareCapture::capture(*s,"source");
        check(r.acquisition.state==AcquisitionState::Done&&r.evidence->actual.hardwareFrameId==0&&r.evidence->triggerCounter==0,"32-bit counter wrap");s->close();
    }
    for(const auto& scenario:QStringList{"old","duplicate-trigger","late-epoch","future-epoch","reset","missing-proof","wrong-arm","command-error","unsupported","timeout","release-error","stop-error","external"}){
        auto h=std::make_shared<Hardware>();h->scenario=scenario;auto s=open(h);
        auto r=IndustrialSoftwareCapture::capture(*s,"source");noResult(r);
        if(scenario=="external"||scenario=="unsupported"||scenario=="wrong-arm")check(h->commands==0,"no unproven command");
        if(scenario=="command-error")check(!h->calls.contains("wait"),"failed command must not wait");
        if(scenario!="external"){
            check(s->stage()==OecsSessionStage::Faulted,"failure requires explicit reopen");
            const auto commands=h->commands;noResult(IndustrialSoftwareCapture::capture(*s,"source"));check(h->commands==commands,"no automatic retry");
            check(h->calls.contains("stop")&&h->calls.contains("flush"),"cleanup after error");
        }
        s->close();check(!h->held&&!h->running,"cleanup balances resources");
    }
    // Cancellation at blocking SDK boundaries and after owned copy/cleanup.
    for(const auto& point:QStringList{"pool","start","ready","command","wait","release","stop"}){
        auto h=std::make_shared<Hardware>();auto s=open(h);auto cancel=std::make_shared<std::atomic_bool>(false);
        h->hook=[&](const QString& name){if(name==point)cancel->store(true);};
        auto r=IndustrialSoftwareCapture::capture(*s,"source",1000,cancel);noResult(r);
        check(r.acquisition.state==AcquisitionState::Cancelled,"cancel suppresses late success");
        if(point=="ready"||point=="start"||point=="pool")check(h->commands==0,"cancel before trigger");
        check(!h->held&&!h->running,"cancel cleanup");s->close();
    }
    for(const auto& point:QStringList{"ready","command","wait","release","stop"}){
        auto h=std::make_shared<Hardware>();auto s=open(h);
        h->hook=[&](const QString& name){if(name==point)std::this_thread::sleep_for(std::chrono::milliseconds(35));};
        auto r=IndustrialSoftwareCapture::capture(*s,"source",20);noResult(r);
        check(r.acquisition.state==AcquisitionState::Timeout,"late SDK success suppressed");s->close();
    }
    {
        auto h=std::make_shared<Hardware>();auto s=open(h);
        h->hook=[&](const QString& name){if(name=="ready")std::this_thread::sleep_for(std::chrono::milliseconds(25));};
        auto r=IndustrialSoftwareCapture::capture(*s,"source",1000,{},Deadline(200));
        check(r.acquisition.state==AcquisitionState::Done&&h->waitMs>0&&h->waitMs<h->readyMs&&h->readyMs<=200,"remaining monotonic budget reaches SDK");s->close();
    }
    {
        auto h=std::make_shared<Hardware>();auto s=open(h);bool nested=false;
        h->hook=[&](const QString& name){if(name=="wait"){noResult(IndustrialSoftwareCapture::capture(*s,"source"));nested=true;}};
        check(IndustrialSoftwareCapture::capture(*s,"source").acquisition.state==AcquisitionState::Done&&nested&&h->commands==1,"one in-flight admission");
        h->hook={};IndustrialTriggerResult foreign;
        std::thread([&]{foreign=IndustrialSoftwareCapture::capture(*s,"source");}).join();noResult(foreign);check(h->commands==1,"foreign thread cannot trigger");s->close();
    }
    {
        auto h=std::make_shared<Hardware>();auto s=open(h);
        noResult(IndustrialSoftwareCapture::capture(*s,"",100));
        noResult(IndustrialSoftwareCapture::capture(*s,"source",0));
        noResult(IndustrialSoftwareCapture::capture(*s,"source",100,{},Deadline(0)));
        auto cancel=std::make_shared<std::atomic_bool>(true);noResult(IndustrialSoftwareCapture::capture(*s,"source",100,cancel));
        check(h->calls.isEmpty(),"invalid/pre-cancelled admission has no SDK effects");s->close();
    }
    QTextStream(stdout)<<"industrial software trigger fixture passed (not physical acceptance)"<<Qt::endl;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<Qt::endl;return 1;}}
