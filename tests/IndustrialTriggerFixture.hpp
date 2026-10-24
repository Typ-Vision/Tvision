#pragma once
#include "application/IndustrialSoftwareCapture.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <deque>
#include <thread>
using namespace visionflow;
namespace industrial_fixture {
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
