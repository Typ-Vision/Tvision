#include "application/CameraWorkerSession.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <future>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,"expected rejection");}
template<class F> void until(F f){auto end=Deadline(5000);while(!f()){check(!end.expired(),"fixture wait timed out");std::this_thread::yield();}}
PhysicalDeviceIdentity identity(QString serial="device"){return {"vendor","model",serial,DeviceIdentityConfidence::SerialReported};}
struct Probe {
    std::thread::id worker;
    std::atomic_int calls=0;
    std::atomic_bool entered=false,closed=false,destroyed=false,wrongThread=false;
    bool failCleanup=false,failGrab=false,missingFrame=false,foreignFrame=false;
    std::shared_future<void> gate;
};
class Backend final:public CameraWorkerBackend {
    std::shared_ptr<Probe> p;
    void thread(){if(p->worker!=std::this_thread::get_id())p->wrongThread=true;}
public:
    explicit Backend(std::shared_ptr<Probe> probe):p(std::move(probe)){p->worker=std::this_thread::get_id();}
    ~Backend() override{thread();p->destroyed=true;}
    void shutdown() override{thread();p->closed=true;if(p->failCleanup)throw std::runtime_error("cleanup failed");}
    CameraWorkerResult execute(const CameraTicket& ticket) override{
        thread();++p->calls;
        if(ticket.operation!=CameraOperation::Grab)return {true,{},{}};
        p->entered=true;if(p->gate.valid())p->gate.wait();
        if(p->failGrab)throw std::runtime_error("grab failed");
        if(p->missingFrame)return {true,{},{}};
        const unsigned char pixel=42;FrameMetadata m;m.sourceId="source";m.cycleId="cycle";m.frameId="frame";m.coordinateSystemId="source/image";m.completedAtUtc=QDateTime::currentDateTimeUtc();
        auto image=ImageFrame::fromBuffer(&pixel,1,1,1,1,PixelFormat::Gray8,m);
        if(p->foreignFrame)m.sourceId="foreign";
        return {true,{},AcquiredFrame{image,m.sourceId,m.cycleId,m.frameId,m.completedAtUtc}};
    }
};
CameraWorkerSession::Factory factory(std::shared_ptr<Probe> p){return [p]{return std::make_unique<Backend>(p);};}
void command(CameraWorkerSession& s,CameraOperation op){s.submit(op);until([&]{return s.poll();});}
void streaming(CameraWorkerSession& s){for(auto op:{CameraOperation::Connect,CameraOperation::Configure,CameraOperation::Start})command(s,op);}
void shutdown(CameraWorkerSession& s){s.requestShutdown();until([&]{return s.shutdownState()==ShutdownState::Ready;});}
void stress(){
    // Reuse the exact identity after every joined teardown; both queue policies
    // must clear pending images across every explicit Stop/Start generation.
    for(int session=0;session<100;++session){
        auto p=std::make_shared<Probe>();
        const auto mode=session%2 ? FrameQueueMode::PreviewLatest : FrameQueueMode::ProductionFifo;
        CameraWorkerSession s(identity("stress"),CameraLeaseUse::Editor,"source",mode,factory(p),1);
        command(s,CameraOperation::Connect);command(s,CameraOperation::Configure);
        for(int cycle=0;cycle<10;++cycle){
            command(s,CameraOperation::Start);const auto generation=s.lifecycle().generation();
            command(s,CameraOperation::Grab);auto frame=s.take();
            check(frame && frame->image.size()==QSize(1,1),"stress owned image missing");
            command(s,CameraOperation::Grab);check(s.lifecycle().stats().depth==1,"stress queued image missing");
            s.requestStop();check(!s.take() && s.lifecycle().generation()>generation,"stress Stop did not invalidate queue");
            command(s,CameraOperation::Stop);
            check(s.lifecycle().state()==CameraState::Configured,"stress explicit Stop acknowledgement missing");
        }
        check(s.lifecycle().stats().accepted==20 && s.lifecycle().stats().cleared==10 && s.lifecycle().stats().highWater==1,"stress queue accounting");
        command(s,CameraOperation::Disconnect);shutdown(s);
        check(p->calls==43 && p->closed && p->destroyed && !p->wrongThread,"stress cleanup/thread accounting");
    }
    // Block each grab until shutdown has invalidated it. The final Ready must
    // imply joined destruction and no late frame before the next lease opens.
    for(int session=0;session<20;++session){
        auto p=std::make_shared<Probe>();std::promise<void> release;p->gate=release.get_future().share();
        CameraWorkerSession s(identity("stress-stop"),CameraLeaseUse::Calibration,"source",FrameQueueMode::PreviewLatest,factory(p));
        streaming(s);s.submit(CameraOperation::Grab);until([&]{return p->entered.load();});
        s.requestShutdown();
        rejects([&]{CameraWorkerSession rival(identity("stress-stop"),CameraLeaseUse::Runtime,"source",FrameQueueMode::PreviewLatest,factory(p));});
        release.set_value();until([&]{return s.shutdownState()==ShutdownState::Ready;});
        check(!s.take() && p->destroyed && p->closed && !p->wrongThread,"stress shutdown leaked late frame or backend");
    }
}
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        if(app.arguments().contains("--stress")){
            stress();QTextStream(stdout)<<"PASS: 1000 fixture cycles, 2000 owned grabs, 100 teardown/release and 20 blocked-grab shutdowns\n";return 0;
        }
        auto p=std::make_shared<Probe>();
        auto invalid=identity();invalid.confidence=DeviceIdentityConfidence::ProviderReported;invalid.serial.clear();
        rejects([&]{CameraWorkerSession s(invalid,CameraLeaseUse::Editor,"source",FrameQueueMode::PreviewLatest,factory(p));});
        CameraWorkerSession s(identity(),CameraLeaseUse::Calibration,"source",FrameQueueMode::ProductionFifo,factory(p),1);
        for(auto use:{CameraLeaseUse::Calibration,CameraLeaseUse::Editor,CameraLeaseUse::Runtime})
            rejects([&]{CameraWorkerSession rival(identity(),use,"other",FrameQueueMode::PreviewLatest,factory(p));});
        check(std::async(std::launch::async,[&]{try{s.submit(CameraOperation::Connect);return false;}catch(...){return true;}}).get(),"foreign controller rejected");
        streaming(s);command(s,CameraOperation::Grab);check(s.lifecycle().stats().depth==1,"owned worker frame published");
        const int calls=p->calls;rejects([&]{s.submit(CameraOperation::Grab);});check(p->calls==calls,"FIFO rejects before SDK dispatch");
        check(s.take().has_value(),"consume frame");
        std::promise<void> release;p->gate=release.get_future().share();p->entered=false;
        s.submit(CameraOperation::Grab);until([&]{return p->entered.load();});
        rejects([&]{s.submit(CameraOperation::Grab);});
        s.requestStop(1);until([&]{return s.lifecycle().stopBudget()==StopBudgetState::Exceeded;});s.requestStop(5000);
        check(s.lifecycle().stopBudget()==StopBudgetState::Exceeded && !p->closed,"Stop budget never releases blocked worker");
        rejects([&]{s.submit(CameraOperation::Disconnect);});
        release.set_value();until([&]{s.poll();return s.lifecycle().state()==CameraState::Stopping;});
        // Wait until owner consumes completion; rejected submit also polls it.
        until([&]{try{s.submit(CameraOperation::Stop);return true;}catch(...){return false;}});
        until([&]{return s.poll();});check(!s.take(),"late success suppressed");
        command(s,CameraOperation::Start);command(s,CameraOperation::Grab);check(s.take().has_value(),"explicit software restart");
        std::promise<void> shutdownRelease;p->gate=shutdownRelease.get_future().share();p->entered=false;
        s.submit(CameraOperation::Grab);until([&]{return p->entered.load();});s.transportLost("unplugged");
        s.requestShutdown(1);until([&]{return s.shutdownState()==ShutdownState::BudgetExceeded;});s.requestShutdown(5000);
        check(!p->destroyed && s.lifecycle().state()==CameraState::Fault,"fault cannot release blocked SDK");
        rejects([&]{CameraWorkerSession rival(identity(),CameraLeaseUse::Runtime,"source",FrameQueueMode::PreviewLatest,factory(p));});
        shutdownRelease.set_value();until([&]{return s.shutdownState()==ShutdownState::Ready;});
        check(p->closed && p->destroyed && !p->wrongThread && p->worker!=std::this_thread::get_id() && !s.take(),"worker cleanup and late result suppression");
        auto next=std::make_shared<Probe>();CameraWorkerSession reconnected(identity(),CameraLeaseUse::Runtime,"source",FrameQueueMode::PreviewLatest,factory(next));
        // Repeated polls of the old session must never release the new lease.
        s.poll();rejects([&]{CameraWorkerSession rival(identity(),CameraLeaseUse::Editor,"source",FrameQueueMode::PreviewLatest,factory(p));});
        streaming(reconnected);next->failGrab=true;reconnected.submit(CameraOperation::Grab);
        until([&]{reconnected.poll();return reconnected.lifecycle().state()==CameraState::Fault;});check(!reconnected.take(),"worker exception yields no frame");shutdown(reconnected);
        auto bad=std::make_shared<Probe>();bad->failCleanup=true;
        {CameraWorkerSession broken(identity("quarantine"),CameraLeaseUse::Editor,"source",FrameQueueMode::PreviewLatest,factory(bad));shutdown(broken);check(broken.lifecycle().error()=="cleanup failed","cleanup error retained");}
        rejects([&]{CameraWorkerSession rival(identity("quarantine"),CameraLeaseUse::Runtime,"source",FrameQueueMode::PreviewLatest,factory(p));});
        {CameraWorkerSession failed(identity("factory"),CameraLeaseUse::Editor,"source",FrameQueueMode::PreviewLatest,[]{throw std::runtime_error("factory failed");return std::unique_ptr<CameraWorkerBackend>{};});
            failed.submit(CameraOperation::Connect);until([&]{failed.poll();return failed.lifecycle().state()==CameraState::Fault;});shutdown(failed);}
        for(bool foreign:{false,true}){
            auto malformed=std::make_shared<Probe>();malformed->missingFrame=!foreign;malformed->foreignFrame=foreign;
            CameraWorkerSession invalidFrame(identity("malformed"),CameraLeaseUse::Editor,"source",FrameQueueMode::PreviewLatest,factory(malformed));
            streaming(invalidFrame);invalidFrame.submit(CameraOperation::Grab);
            until([&]{invalidFrame.poll();return invalidFrame.lifecycle().state()==CameraState::Fault;});
            check(!invalidFrame.take(),"missing/foreign owned frame faults instead of successful completion");shutdown(invalidFrame);
        }
        auto preview=std::make_shared<Probe>();
        {CameraWorkerSession latest(identity("preview"),CameraLeaseUse::Editor,"source",FrameQueueMode::PreviewLatest,factory(preview),1);
            streaming(latest);command(latest,CameraOperation::Grab);command(latest,CameraOperation::Grab);
            check(latest.lifecycle().stats().dropped==1 && latest.lifecycle().stats().depth==1,"worker preview drops oldest");shutdown(latest);}
        QTextStream(stdout)<<"PASS: persistent worker, exclusive leases, Stop/shutdown budgets, late frames, fault and cleanup quarantine\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}
}
