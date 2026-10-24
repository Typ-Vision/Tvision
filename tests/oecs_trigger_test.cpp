#include "IndustrialTriggerFixture.hpp"
using namespace industrial_fixture;
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
