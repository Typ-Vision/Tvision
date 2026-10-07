#include "application/CameraLifecycle.hpp"
#include "application/EditorSession.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QThread>
#include <QTextStream>
#include <future>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejected(F f,const char* message){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,message);}
Deadline past(){return Deadline(Deadline::Clock::now()-std::chrono::seconds(1));}
void streaming(CameraLifecycle& camera){
    for(auto op:{CameraOperation::Connect,CameraOperation::Configure,CameraOperation::Start}){
        auto ticket=camera.begin(op);check(camera.complete(ticket,true),"successful lifecycle command");
    }
    check(camera.state()==CameraState::Streaming,"stream state");
}
AcquiredFrame frame(QString id="frame"){
    const unsigned char byte=73;FrameMetadata m;m.sourceId="source";m.frameId=id;m.cycleId="cycle";m.coordinateSystemId="source/image";m.completedAtUtc=QDateTime::currentDateTimeUtc();
    auto image=ImageFrame::fromBuffer(&byte,1,1,1,1,PixelFormat::Gray8,m);
    return {image,m.sourceId,m.cycleId,m.frameId,m.completedAtUtc};
}
class BudgetAdapter final:public AcquisitionAdapter {
public:
    mutable int calls=0;
    AcquisitionCapabilities capabilities() const override{return {"fake_camera",true,false,false,false,false};}
    ImageFrame acquire(const AcquisitionRequest& r) const override{
        ++calls;check(r.deadline && r.deadline->remainingMs()>0 && r.deadline->remainingMs()<=r.timeoutMs,"adapter gets remaining monotonic budget");return frame().image;
    }
};
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        rejected([]{CameraLifecycle c("source",FrameQueueMode::PreviewLatest,0);},"zero capacity denied");
        CameraLifecycle preview("source",FrameQueueMode::PreviewLatest,2);
        rejected([&]{preview.begin(CameraOperation::Grab);},"grab before connect rejected");
        auto connect=preview.begin(CameraOperation::Connect);
        rejected([&]{preview.begin(CameraOperation::Configure);},"overlap rejected");
        auto wrong=connect;++wrong.serial;check(!preview.complete(wrong,true) && preview.state()==CameraState::Connecting,"stale operation cannot clear active");
        check(preview.complete(connect,true),"connect completion");
        rejected([&]{preview.begin(CameraOperation::Start);},"start before configure rejected");
        check(preview.complete(preview.begin(CameraOperation::Configure),true),"configure");
        check(preview.complete(preview.begin(CameraOperation::Start),true),"start");
        auto generation=preview.generation();
        check(preview.enqueue(frame("1"),generation,Deadline(5000))==FrameAdmission::Accepted,"first preview");
        check(preview.enqueue(frame("2"),generation,Deadline(5000))==FrameAdmission::Accepted,"second preview");
        check(preview.enqueue(frame("3"),generation,Deadline(5000))==FrameAdmission::DroppedOldest,"preview drops oldest");
        check(preview.take()->frameId=="2" && preview.take()->frameId=="3" && !preview.take(),"preview order");
        check(preview.stats().dropped==1 && preview.stats().depth==0 && preview.stats().highWater==2,"queue telemetry");
        check(preview.enqueue(frame(),generation,past())==FrameAdmission::Expired,"late frame rejection");
        auto foreign=frame();foreign.sourceId="other";check(preview.enqueue(foreign,generation,Deadline(5000))==FrameAdmission::Invalid,"foreign source rejection");
        check(std::async(std::launch::async,[&]{try{preview.state();return false;}catch(...){return true;}}).get(),"controller thread ownership enforced");
        // A blocked SDK call remains owned after Stop budget expires.
        auto grab=preview.begin(CameraOperation::Grab);preview.enqueue(frame(),generation,Deadline(5000));
        preview.requestStop(1);check(grab.cancel->load() && preview.state()==CameraState::Stopping && preview.stats().depth==0,"stop invalidates queue and cancels active");
        QThread::msleep(10);preview.requestStop(5000);check(preview.stopBudget()==StopBudgetState::Exceeded,"repeat stop cannot reset exceeded budget");
        rejected([&]{preview.begin(CameraOperation::Disconnect);},"cannot close handle during grab");
        check(!preview.complete(grab,true) && preview.state()==CameraState::Stopping,"late grab does not restore stream");
        check(preview.enqueue(frame(),generation,Deadline(5000))==FrameAdmission::Stale,"old stream frame suppressed");
        check(preview.complete(preview.begin(CameraOperation::Stop),true) && preview.stopBudget()==StopBudgetState::Complete,"stop needs SDK acknowledgement");
        check(preview.complete(preview.begin(CameraOperation::Disconnect),true),"explicit cleanup");
        streaming(preview);check(preview.generation()!=generation && preview.stopBudget()==StopBudgetState::NotRequested,"reconnect gets new stream identity and stop budget");
        check(!preview.complete(grab,true) && preview.state()==CameraState::Streaming,"old ticket cannot change new stream");
        auto inflight=preview.begin(CameraOperation::Grab);preview.transportLost("unplugged");
        rejected([&]{preview.begin(CameraOperation::Connect);},"fault cannot auto reconnect while worker owns handles");
        check(!preview.complete(inflight,true) && preview.state()==CameraState::Fault && preview.error()=="unplugged","disconnect late completion stays fault");
        check(preview.complete(preview.begin(CameraOperation::Disconnect),true),"fault cleanup ack");streaming(preview);
        auto failure=preview.begin(CameraOperation::Grab);check(!preview.complete(failure,false,"SDK failure") && preview.state()==CameraState::Fault,"adapter failure is fault");
        CameraLifecycle production("source",FrameQueueMode::ProductionFifo,1);streaming(production);auto pg=production.generation();
        check(production.enqueue(frame("p1"),pg,Deadline(5000))==FrameAdmission::Accepted,"production accepted");
        rejected([&]{production.begin(CameraOperation::Grab);},"production backpressure before trigger dispatch");
        check(production.enqueue(frame("p2"),pg,Deadline(5000))==FrameAdmission::Full,"production full explicit rejection");
        check(production.take()->frameId=="p1" && production.stats().dropped==0 && production.stats().rejected==1,"production does not replace cycle");
        production.enqueue(frame("expired-in-queue"),pg,Deadline(1));QThread::msleep(10);check(!production.take() && production.stats().expired==1,"deadline rechecked on consumption");
        auto deadlineOp=production.begin(CameraOperation::Grab,1);QThread::msleep(10);check(!production.complete(deadlineOp,true) && production.state()==CameraState::Fault,"late SDK success fails operation");
        BudgetAdapter adapter;AcquisitionRequest request{"source","cycle","frame",TriggerMode::Software,5000,{}};
        request.deadline=past();auto result=AcquisitionService{}.acquire(adapter,request);check(result.state==AcquisitionState::Timeout && !result.frame && adapter.calls==0,"queue wait included before SDK call");
        request.deadline=Deadline(1000);result=AcquisitionService{}.acquire(adapter,request);check(result.state==AcquisitionState::Done && adapter.calls==1,"remaining budget delivered");
        // Uninterruptible fake worker: shutdown reports exceeded and refuses new work.
        JobSession session;QObject owner;auto* watcher=session.watch<int>(&owner,SessionJob::Discovery);
        std::promise<void> release;auto gate=release.get_future().share();auto returned=std::make_shared<std::atomic_bool>(false);
        session.launch(watcher,[gate,returned]{gate.wait();returned->store(true);return 9;});
        session.requestShutdown(1);QThread::msleep(10);
        check(session.shutdownState()==ShutdownState::BudgetExceeded && !returned->load(),"shutdown does not pretend SDK interruption");
        rejected([&]{session.begin(SessionJob::Connection);},"no new operation during shutdown");
        release.set_value();watcher->future().waitForFinished();
        check(session.shutdownState()!=ShutdownState::Ready,"pending owner completion still blocks shutdown");
        QCoreApplication::processEvents();check(session.shutdownState()==ShutdownState::Ready && returned->load(),"worker and owner completion permit shutdown");
        QTextStream(stdout)<<"PASS: synthetic lifecycle, generation, queue/drop, deadlines, owner thread and shutdown budgets\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}
}
