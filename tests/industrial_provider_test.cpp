#include "IndustrialTriggerFixture.hpp"
#include "application/IndustrialCameraProvider.hpp"
using namespace industrial_fixture;
namespace {
template<class F> void reject(F f){try{f();}catch(const std::exception&){return;}throw std::runtime_error("expected rejection");}
template<class F> void until(F f){Deadline d(5000);while(!f()){check(!d.expired(),"worker timeout");std::this_thread::yield();}}
IndustrialCameraRequest request(){
    IndustrialCameraRequest r;
    r.device={{"VF","CAM","TRIGGER",DeviceIdentityConfidence::SerialReported},
              {"oecs",{},"test","test"},{"oecs","test-only",QString(64,'a'),1}};
    r.camera={{"camera"},"Synthetic test camera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},r.device.identity};
    GeometryDescriptor g;g.resolution={3,2};g.sensorRoi=QRect(0,0,3,2);g.binning=QSize(1,1);
    g.decimation=QSize(1,1);g.reverse=GeometryMirror{};g.complete=true;
    r.setup={{"setup"},r.camera.id,"Test setup","lens","plane",1,1,g};
    r.profile={{"profile"},r.camera.id,r.setup.id,"Test profile",1,{"Mono8",{true,"FrameStart","Software"},AutoMode::Off,100,AutoMode::Off,1,{}}};
    r.sourceId="source";r.validate();return r;
}
struct State {
    std::shared_ptr<Hardware> hardware=std::make_shared<Hardware>();
    bool foreign=false,geometry=false,settings=false,cleanup=false;
};
class Backend final:public IndustrialCameraBackend {
    IndustrialCameraRequest r;
    std::shared_ptr<State> state;
    std::unique_ptr<OecsSession> session;
public:
    Backend(IndustrialCameraRequest request,std::shared_ptr<State> s):r(std::move(request)),state(std::move(s)){}
    PhysicalDeviceIdentity actualIdentity() const override {auto id=r.device.identity;if(state->foreign)id.serial="other";return id;}
    AcquisitionReadback readback() const override {
        GeometryReadback g;g.resolution=state->geometry?QSize(2,3):QSize(3,2);g.offset={0,0};
        g.offsetEvidence=g.binningEvidence=g.decimationEvidence=g.reverseEvidence=GeometryEvidence::Reported;
        auto settings=r.profile.requested;if(state->settings)settings.gain=2;
        return {g,settings};
    }
    CameraWorkerResult execute(const CameraTicket& t) override {
        switch(t.operation){
        case CameraOperation::Connect:session=industrial_fixture::open(state->hardware);break;
        case CameraOperation::Grab:{
            if(!r.profile.requested.trigger.enabled){
                // Independent preview event, never a software command or cycle frame.
                const unsigned char pixels[6]={1,2,3,4,5,6};
                FrameMetadata m;m.sourceId=r.sourceId;m.frameId="preview-frame";m.cycleId="preview-session";
                m.completedAtUtc=QDateTime::currentDateTimeUtc();
                auto image=ImageFrame::fromBuffer(pixels,6,3,2,3,PixelFormat::Gray8,m);
                return {true,{},AcquiredFrame{image,m.sourceId,m.cycleId,m.frameId,m.completedAtUtc}};
            }
            auto result=IndustrialSoftwareCapture::capture(*session,r.sourceId,t.deadline.remainingMs(),t.cancel,t.deadline);
            return {result.acquisition.state==AcquisitionState::Done,result.acquisition.error,result.acquisition.frame};
        }
        case CameraOperation::Disconnect:session->close();break;
        default:break;
        }return {true,{},{}};
    }
    void shutdown() override {if(session)session->close();if(state->cleanup)throw std::runtime_error("fixture cleanup failed");}
};
class Provider final:public IndustrialCameraProvider {
    std::shared_ptr<State> state;
public:
    explicit Provider(std::shared_ptr<State> s):state(std::move(s)){}
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice& d) const override {
        check(d.identity==request().device.identity,"test identity only");return {true,true,true,true,true,true,{}};
    }
    std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest& r) const override {return std::make_unique<Backend>(r,state);}
};
void command(CameraWorkerSession& s,CameraOperation op){s.submit(op);until([&]{s.poll();return s.lifecycle().state()==CameraState::Fault ||
    (op==CameraOperation::Connect?s.lifecycle().state()==CameraState::Connected:
     op==CameraOperation::Configure||op==CameraOperation::Stop?s.lifecycle().state()==CameraState::Configured:
     s.lifecycle().state()==CameraState::Streaming);});}
void start(CameraWorkerSession& s){command(s,CameraOperation::Connect);command(s,CameraOperation::Configure);command(s,CameraOperation::Start);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    auto r=request();CameraProviderFactory product;
    check(!product.capabilities(r.device).softwareTrigger,"normal factory denies synthetic/native unadmitted backend");
    reject([&]{product.open(r,CameraLeaseUse::Runtime,FrameQueueMode::ProductionFifo);});
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
    {auto s=factory.open(preview,CameraLeaseUse::Editor,FrameQueueMode::PreviewLatest);start(*s);command(*s,CameraOperation::Grab);
     check(state->hardware->commands==0 && s->lifecycle().stats().depth==1,"preview event without trigger");
     s->requestStop();command(*s,CameraOperation::Stop);check(!s->take(),"preview Stop clears raster");}
    {auto s=factory.open(r,CameraLeaseUse::Runtime,FrameQueueMode::ProductionFifo);start(*s);command(*s,CameraOperation::Grab);check(s->take().has_value(),"new capture session after preview");}
    QTextStream(stdout)<<"PASS: test composition, exact snapshot/readback, independent counter events, equal pixels, preview/capture lease boundary\n";
    return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
