#include "IndustrialProviderFixture.hpp"
#include "application/CameraPreviewSession.hpp"
using namespace industrial_provider_fixture;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    auto r=request();CameraProviderFactory product;
    check(!product.capabilities(r.device).softwareTrigger,"normal factory denies synthetic/native unadmitted backend");
    reject([&]{product.open(r,CameraLeaseUse::Runtime,FrameQueueMode::ProductionFifo);});
    reject([&]{product.configure(r,CameraLeaseUse::Editor);});
    {auto state=std::make_shared<State>();state->triggerUnavailable=true;
     auto factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));
     reject([&]{factory.open(r,CameraLeaseUse::Runtime,FrameQueueMode::ProductionFifo);});
     auto s=factory.configure(r,CameraLeaseUse::Editor);command(*s,CameraOperation::Connect);command(*s,CameraOperation::Configure);
     check(s->readback().has_value(),"configure independent of trigger admission");
     command(*s,CameraOperation::Start);check(s->lifecycle().state()==CameraState::Fault,"configure-only session cannot acquire");}
    auto bad=r;bad.profile.setup={"other"};reject([&]{bad.validate();});
    bad=r;bad.camera.physical->serial="foreign";reject([&]{bad.validate();});
    for(const auto scenario:{"","old","duplicate-trigger","late-epoch","timeout","stop-error"}){
        auto state=std::make_shared<State>();state->hardware->scenario=scenario;
        auto factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));
        auto s=factory.resolveOpen(r,{{r.device},{},false},{},CameraLeaseUse::Editor,FrameQueueMode::ProductionFifo,1);
        reject([&]{factory.open(r,CameraLeaseUse::Calibration,FrameQueueMode::PreviewLatest);});
        start(*s);command(*s,CameraOperation::Grab);
        if(QString(scenario).isEmpty()){
            auto first=s->take();check(first.has_value(),"provider owned frame");
            state->hardware->pixels=QByteArray::fromHex("010203040506");command(*s,CameraOperation::Grab);
            auto second=s->take();check(second && second->frameId!=first->frameId && second->image.contentId()==first->image.contentId(),"equal pixels new frame");
            state->settings=true;command(*s,CameraOperation::Grab);check(!s->take() && state->hardware->commands==2,"changed readback rejected before trigger");
        }else check(!s->take() && s->lifecycle().state()==CameraState::Fault,"stale/error cannot publish frame");
    }
    for(int failure=0;failure<2;++failure){
        auto state=std::make_shared<State>();state->foreign=failure==0;state->geometry=failure==1;
        auto factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));
        auto s=factory.open(r,CameraLeaseUse::Calibration,FrameQueueMode::ProductionFifo);
        command(*s,CameraOperation::Connect);if(failure)command(*s,CameraOperation::Configure);
        check(s->lifecycle().state()==CameraState::Fault && !s->take(),"actual identity/geometry mismatch rejected");
    }
    // Preview has its own idle profile/session; transition drains the worker and lease.
    auto state=std::make_shared<State>();auto factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));
    auto preview=r;preview.profile.requested.trigger={false,{},{}};
    for(bool continuous:{false,true}){
        state->continuous=continuous;
        CameraPreviewSession session;
        session.start([&]{return factory.open(preview,CameraLeaseUse::Preview,FrameQueueMode::PreviewLatest,1);},true);
        until([&]{session.poll();return session.stats().completed>=10;});
        auto frame=session.takeLatest();
        check(frame && frame->kind==(continuous ? PreviewAcquisitionKind::NativeContinuous : PreviewAcquisitionKind::SequentialSingleFrame)
            && state->hardware->commands==0,"provider preview kind and no software trigger conversion");
        session.stop();until([&]{session.poll();return session.drained();});check(!session.takeLatest(),"provider preview stops publishing");
    }
    {auto s=factory.open(preview,CameraLeaseUse::Editor,FrameQueueMode::PreviewLatest);start(*s);command(*s,CameraOperation::Grab);
     check(state->hardware->commands==0 && s->lifecycle().stats().depth==1,"preview event without trigger");
     s->requestStop();command(*s,CameraOperation::Stop);check(!s->take(),"preview Stop clears raster");}
    {auto s=factory.open(r,CameraLeaseUse::Runtime,FrameQueueMode::ProductionFifo);start(*s);command(*s,CameraOperation::Grab);check(s->take().has_value(),"new capture session after preview");}
    QTextStream(stdout)<<"PASS: test composition, exact snapshot/readback, independent counter events, equal pixels, preview/capture lease boundary\n";
    return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
