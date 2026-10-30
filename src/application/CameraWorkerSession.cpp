#include "CameraWorkerSession.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
CameraWorkerSession::CameraWorkerSession(PhysicalDeviceIdentity identity,CameraLeaseUse use,
    QString source,FrameQueueMode mode,Factory factory,size_t capacity,PreviewAcquisitionKind kind,CameraLeaseArbiter::Ticket lease,QString cameraId)
    :kind_(kind),previewCompatible_(mode==FrameQueueMode::PreviewLatest && capacity==1),lifecycle_(std::move(source),mode,capacity) {
    identity.validate();require(identity.usable(),"Camera lease requires exact physical identity.");
    require(use==CameraLeaseUse::Calibration || use==CameraLeaseUse::Editor || use==CameraLeaseUse::Runtime || use==CameraLeaseUse::Preview,"Invalid camera lease use.");
    require(bool(factory),"Camera worker factory missing.");
    if(!lease){auto reservation=CameraLeaseArbiter::reserve(cameraId.isEmpty()?CameraLeaseArbiter::physicalKey(identity):cameraId,use,{{cameraId,identity}});
        require(!reservation.reason,"CAMERA_BUSY: exclusive process-local lease held");lease=reservation.leases.first();}
    CameraLeaseArbiter::claim(lease,identity,use,cameraId);lease_=std::move(lease);
    timing_=AcquisitionTiming::current();if(timing_.request.isEmpty())timing_=AcquisitionTiming::request("camera-worker");
    if(!timing_.request.isEmpty())timing_.session=AcquisitionTiming::nextId();
    try {job_=jobs_.begin(SessionJob::Connection);worker_=std::thread([this,factory=std::move(factory)]() mutable {run(std::move(factory));});}
    catch(...){clean_=true;releaseLease();throw;}
}
void CameraWorkerSession::owner() const {require(std::this_thread::get_id()==owner_,"Camera session accessed outside owner thread.");}
void CameraWorkerSession::releaseLease(){if(leaseHeld_){CameraLeaseArbiter::drained(lease_,clean_);lease_.reset();leaseHeld_=false;}}
CameraWorkerSession::~CameraWorkerSession(){
    AcquisitionTimingBinding binding(timing_);AcquisitionTimingSpan timing("session.close-and-join");
    // Owner lifetime must outlast this join; no detached callback captures UI/session.
    requestShutdown();if(worker_.joinable())worker_.join();
    releaseLease();
    jobs_.finish(job_);
    timing.finish(clean_?"success":"error");
}
void CameraWorkerSession::run(Factory factory){
    AcquisitionTimingBinding binding(timing_);
    std::unique_ptr<CameraWorkerBackend> backend;
    try {AcquisitionTimingSpan timing("backend.create");backend=factory();require(bool(backend),"Camera worker factory returned null.");}
    catch(const std::exception& e){startupError_=QString::fromUtf8(e.what());}
    catch(...){startupError_="Camera worker factory failed.";}
    for(;;){
        CameraTicket ticket;AcquisitionTimingContext timing;AcquisitionTiming::Clock::time_point submitted;
        {std::unique_lock lock(mutex_);wake_.wait(lock,[this]{return closing_ || command_.has_value();});
            if(closing_)break;
            ticket=*command_;command_.reset();timing=commandTiming_;submitted=submittedAt_;}
        AcquisitionTimingBinding commandBinding(timing);
        AcquisitionTiming::record(timing,"worker.queue",submitted);
        AcquisitionTimingSpan execution("worker.operation");
        CameraWorkerResult result;
        try {
            if(!backend)result.error=startupError_;
            else if(ticket.cancel->load() || ticket.deadline.expired())result.error="Camera operation cancelled/expired before dispatch.";
            else result=backend->execute(ticket);
        }catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}
        catch(...){result.error="Camera worker operation failed.";}
        execution.finish(ticket.cancel->load()?"cancelled":ticket.deadline.expired()?"timeout":result.success?"success":"error",{{"operation",int(ticket.operation)}});
        {std::lock_guard lock(mutex_);completion_=Completion{ticket,std::move(result),Deadline::Clock::now()};}
    }
    bool clean=true;QString error;
    try {if(backend)backend->shutdown();}
    catch(const std::exception& e){clean=false;error=QString::fromUtf8(e.what());}
    catch(...){clean=false;error="Camera worker cleanup failed; lease quarantined.";}
    backend.reset(); // SDK/handle/library destructor on the worker, before exited
    {std::lock_guard lock(mutex_);clean_=clean;cleanupError_=error;exited_=true;}
}
void CameraWorkerSession::submit(CameraOperation operation,int timeout){
    owner();poll();std::lock_guard lock(mutex_);
    require(!closing_ && !pending_,"Camera worker busy or shutting down.");
    command_=lifecycle_.begin(operation,timeout);commandTiming_=AcquisitionTiming::current();
    if(commandTiming_.request.isEmpty())commandTiming_=timing_;commandTiming_.session=timing_.session;
    submittedAt_=AcquisitionTiming::Clock::now();pending_=true;wake_.notify_one();
}
bool CameraWorkerSession::poll(){
    owner();std::optional<Completion> completed;bool exited,clean;QString cleanupError;
    {std::lock_guard lock(mutex_);completed=std::move(completion_);completion_.reset();exited=exited_;clean=clean_;cleanupError=cleanupError_;}
    bool accepted=false;
    if(completed){
        pending_=false;auto& [ticket,result,completedAt]=*completed;
        if(result.success && ticket.operation==CameraOperation::Grab && !result.frame){
            result.success=false;result.error="Camera grab completed without an owned frame.";
        }
        accepted=lifecycle_.complete(ticket,result.success,result.error);
        if(accepted && result.readback)readback_=result.readback;
        if(accepted && ticket.operation==CameraOperation::Grab && result.frame){
            const auto admission=lifecycle_.enqueue(std::move(*result.frame),ticket.generation,ticket.deadline);
            if(admission!=FrameAdmission::Accepted && admission!=FrameAdmission::DroppedOldest){
                lifecycle_.transportLost("Camera worker frame admission failed.");accepted=false;
            }else frameCompletedAt_=completedAt;
        }
    }
    if(exited && worker_.joinable()){
        worker_.join();pending_=false;jobs_.finish(job_);
        if(clean)releaseLease();
        lifecycle_.transportLost(clean ? "Camera worker shut down; create an explicit new session to reconnect." : cleanupError);
    }
    return accepted;
}
void CameraWorkerSession::requestStop(int budget){owner();lifecycle_.requestStop(budget);}
void CameraWorkerSession::transportLost(const QString& reason){owner();lifecycle_.transportLost(reason);}
void CameraWorkerSession::requestShutdown(int budget){
    owner();jobs_.requestShutdown(budget);lifecycle_.requestStop(budget);
    {std::lock_guard lock(mutex_);closing_=true;}wake_.notify_one();
}
ShutdownState CameraWorkerSession::shutdownState(){owner();poll();return jobs_.shutdownState();}
std::optional<bool> CameraWorkerSession::cleanShutdown() const {
    owner(); // exited_/clean_ are read after the owner has joined in poll().
    if(worker_.joinable())return {};
    return clean_;
}
std::optional<AcquiredFrame> CameraWorkerSession::take(){owner();poll();return lifecycle_.take();}
}
