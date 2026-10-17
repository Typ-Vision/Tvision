#include "application/IndustrialFrameProbe.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <limits>
#include <thread>
using namespace visionflow;
namespace {
void check(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
template<class F> void rejects(F action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
struct State {
    QStringList calls;
    QString fail;
    QByteArray pixels=QByteArray::fromHex("010203eeee040506eeee");
    OecsBuffer frame;
    qint64 payload=10,width=3,height=2;
    int handles=0,held=0,running=0,timeout=0,poolCalls=0;
    quint32 pool=0;
    bool loaded=false,mutated=false;
    State(){frame={pixels.constData(),10,5,42,0x123456789ULL,3,2,0x01080001,true,false,false};}
    int call(const QString& name){calls<<name;return fail==name?oecs::errorHandle:0;}
};
class Fake final: public OecsCApi {
public:
    explicit Fake(std::shared_ptr<State> s):s(std::move(s)){}
    bool load(const QString&,QString&) override {s->loaded=true;return true;}
    bool unload() override {check(s->handles==0&&s->held==0&&s->running==0,"unload with live resources");s->calls<<"unload";if(s->fail=="unload")return false;s->loaded=false;return true;}
    quint32 sdkVersion() override {return 1;}
    int enumerateUsb(QVector<OecsDeviceInfo>& d) override {d={{"VF","CAM","STREAM"}};return 0;}
    int createHandle(int,void** h) override {*h=this;++s->handles;return 0;}
    int openExclusive(void*) override {return 0;}
    int closeDevice(void*) override {return s->call("close");}
    int destroyHandle(void*) override {const auto rc=s->call("destroy");if(!rc){--s->handles;s->held=s->running=0;}return rc;}
    int readFeature(void*,const QString& n,CameraFeature& f) override {
        f={};f.name=n;f.access=FeatureAccess::ReadOnly;f.type=FeatureType::Integer;
        if(n=="PixelFormat"){f.type=FeatureType::Enumeration;f.value=QString("Mono8");}
        else f.value=n=="PayloadSize"?s->payload:n=="Width"?s->width:s->height;
        return 0;
    }
    int setBufferCount(void*,quint32 n) override {++s->poolCalls;s->pool=n;return s->call("pool");}
    int startStream(void*) override {s->running=1;return s->call("start");}
    int waitBuffer(void*,int timeout,OecsBuffer& f) override {s->timeout=timeout;const int rc=s->call("wait");if(!rc){f=s->frame;++s->held;}return rc;}
    int releaseBuffer(void*) override {
        check(s->held==1,"release exactly one borrowed buffer");const int rc=s->call("release");
        if(!rc){--s->held;s->pixels.fill(char(99));s->mutated=true;}return rc;
    }
    int stopStream(void*) override {const int rc=s->call("stop");if(!rc)s->running=0;return rc;}
    int flushStream(void*) override {return s->call("flush");}
private:
    std::shared_ptr<State> s;
};
std::unique_ptr<OecsSession> session(const std::shared_ptr<State>& state){
    auto result=std::make_unique<OecsSession>(std::make_unique<Fake>(state),QDir::temp().absoluteFilePath("fake.dll"));
    result->open({"VF","CAM","STREAM"});return result;
}
void balanced(const State& s){check(!s.loaded&&s.handles==0&&s.held==0&&s.running==0,"balanced SDK resources");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    {
        auto s=std::make_shared<State>();auto owner=session(s);
        OecsStreamLimits limits;limits.poolBytes=40;limits.timeoutMs=27;
        auto result=IndustrialFrameProbe::capture(*owner,limits);
        check(s->calls==QStringList{"pool","start","wait","release","stop","flush"},"stream order");
        check(s->pool==4&&s->timeout==27&&s->held==0&&s->running==0&&s->mutated,"bounded SDK pool/wait/release");
        const auto hash=result.frame.contentId();
        check(result.frame.pixelText(0,0)=="Gray: 1"&&result.frame.pixelText(2,1)=="Gray: 6","padded Mono8 copied without padding");
        check(result.actual.width==3&&result.actual.height==2&&result.actual.stride==5&&result.actual.receivedBytes==10&&result.actual.hardwareFrameId==42&&result.actual.deviceTicks==0x123456789ULL,"actual evidence");
        check(result.frame.metadata().deviceTimestampTicks==0x123456789ULL&&!result.frame.metadata().exposureAtUtc&&result.frame.metadata().cycleId.isEmpty()&&result.frame.metadata().frameId.isEmpty(),"diagnostic is not production evidence");
        owner->close();owner.reset();balanced(*s);
        check(result.frame.contentId()==hash&&result.frame.pixelText(2,1)=="Gray: 6","owned frame survives mutation and session cleanup");
    }
    for(int scenario=0;scenario<11;++scenario){
        auto s=std::make_shared<State>();
        switch(scenario){case 0:s->frame.bytes=9;break;case 1:s->frame.complete=false;break;case 2:s->frame.overflow=true;break;
        case 3:s->frame.pixelFormat=0x010C0004;break;case 4:s->frame.chunks=true;break;case 5:s->frame.width=4;break;
        case 6:s->frame.stride=2;break;case 7:s->frame.stride=std::numeric_limits<quint64>::max();break;
        case 8:s->frame.data=nullptr;break;case 9:s->frame.bytes=11;break;case 10:s->frame.height=0;break;}
        auto owner=session(s);rejects([&]{IndustrialFrameProbe::capture(*owner);});
        check(s->calls.contains("release")&&s->calls.contains("stop")&&s->calls.contains("flush"),"invalid frame cleanup");
        owner->close();balanced(*s);
    }
    for(int scenario=0;scenario<8;++scenario){
        auto s=std::make_shared<State>();OecsStreamLimits limits;
        switch(scenario){case 0:limits.poolBytes=39;break;case 1:limits.buffers=0;break;case 2:limits.buffers=65;break;
        case 3:limits.timeoutMs=0;break;case 4:limits.timeoutMs=30001;break;case 5:s->payload=std::numeric_limits<qint64>::max();break;
        case 6:s->width=std::numeric_limits<qint64>::max();break;case 7:s->height=0;break;}
        auto owner=session(s);rejects([&]{IndustrialFrameProbe::capture(*owner,limits);});
        check(s->poolCalls==0,"invalid budget rejected before pool allocation");owner->close();balanced(*s);
    }
    for(const auto& failure:QStringList{"pool","start","wait","release","stop","flush"}){
        auto s=std::make_shared<State>();s->fail=failure;auto owner=session(s);
        rejects([&]{IndustrialFrameProbe::capture(*owner);});
        check(owner->stage()==OecsSessionStage::Faulted,"failed probe requires explicit close/reopen");
        if(failure!="pool")check(s->calls.contains("stop")&&s->calls.contains("flush"),"failure still attempts stop and flush");
        if(failure=="wait")check(!s->calls.contains("release"),"no release after unsuccessful wait");
        const auto count=s->calls.size();rejects([&]{IndustrialFrameProbe::capture(*owner);});check(count==s->calls.size(),"fault prohibits automatic recovery");
        owner->close();balanced(*s);
    }
    {
        auto s=std::make_shared<State>();auto owner=session(s);
        rejects([&]{owner->probeFrame({},[&](const OecsBuffer&){throw std::bad_alloc();});});
        check(s->calls.contains("release")&&s->calls.contains("flush"),"copy exception cleanup");owner->close();balanced(*s);
    }
    {
        auto s=std::make_shared<State>();auto owner=session(s);bool rejected=false;
        std::thread([&]{try{IndustrialFrameProbe::capture(*owner);}catch(const OecsSessionError& e){rejected=e.failure()==OecsFailure::WrongThread;}}).join();
        check(rejected&&s->poolCalls==0,"foreign worker rejected before SDK call");
        owner->probeFrame({},[&](const OecsBuffer&){rejects([&]{owner->close();});rejects([&]{IndustrialFrameProbe::capture(*owner);});rejects([&]{owner->feature("Width");});});
        owner->close();balanced(*s);
    }
    {
        auto s=std::make_shared<State>();auto owner=session(s);
        for(int i=0;i<20;++i){auto result=IndustrialFrameProbe::capture(*owner);check(result.actual.hardwareFrameId==42&&s->held==0&&s->running==0,"repeated diagnostic pool remains balanced");}
        owner->close();balanced(*s);
    }
    for(const auto& failure:QStringList{"close","unload","destroy"}){
        auto s=std::make_shared<State>();auto owner=session(s);s->fail="release";
        rejects([&]{IndustrialFrameProbe::capture(*owner);});s->fail=failure;
        rejects([&]{owner->close();});
        if(failure=="close")balanced(*s);
        else {
            check(owner->libraryPinned()&&owner->stage()==OecsSessionStage::Faulted,"uncertain SDK teardown pins library");
            check(s->loaded&&((failure=="destroy"&&s->handles==1&&s->held==1)||(failure=="unload"&&s->handles==0&&s->held==0)),"only unconfirmed resources retained");
            const auto count=s->calls.size();owner.reset();check(s->calls.size()==count,"no speculative cleanup after destroy/unload failure");
        }
    }
    QTextStream(stdout)<<"SYNTHETIC oecs_stream_test passed: padded Mono8, evidence, limits, invalid payload, all stream failures, owned lifetime"<<Qt::endl;
    return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<Qt::endl;return 1;}}
