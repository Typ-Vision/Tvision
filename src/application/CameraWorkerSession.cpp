#include "CameraWorkerSession.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <stdexcept>

namespace visionflow {
namespace {
std::mutex leasesMutex;
QSet<QByteArray> leases;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
CameraWorkerSession::CameraWorkerSession(PhysicalDeviceIdentity identity,CameraLeaseUse use,
    QString source,FrameQueueMode mode,Factory factory,size_t capacity,PreviewAcquisitionKind kind)
    :kind_(kind),previewCompatible_(mode==FrameQueueMode::PreviewLatest && capacity==1),lifecycle_(std::move(source),mode,capacity) {
    identity.validate();require(identity.usable(),"Camera lease requires exact physical identity.");
    require(use==CameraLeaseUse::Calibration || use==CameraLeaseUse::Editor || use==CameraLeaseUse::Runtime || use==CameraLeaseUse::Preview,"Invalid camera lease use.");
    require(bool(factory),"Camera worker factory missing.");
    leaseKey_=QJsonDocument(QJsonArray{identity.vendor,identity.model,identity.serial}).toJson(QJsonDocument::Compact);
    {std::lock_guard lock(leasesMutex);require(!leases.contains(leaseKey_),"Camera busy: exclusive process-local lease held.");leases.insert(leaseKey_);}
    try {job_=jobs_.begin(SessionJob::Connection);worker_=std::thread([this,factory=std::move(factory)]() mutable {run(std::move(factory));});}
    catch(...){releaseLease();throw;}
}
void CameraWorkerSession::owner() const {require(std::this_thread::get_id()==owner_,"Camera session accessed outside owner thread.");}
void CameraWorkerSession::releaseLease(){std::lock_guard lock(leasesMutex);if(leaseHeld_){leases.remove(leaseKey_);leaseHeld_=false;}}
CameraWorkerSession::~CameraWorkerSession(){
    // Owner lifetime must outlast this join; no detached callback captures UI/session.
    requestShutdown();if(worker_.joinable())worker_.join();
    if(clean_)releaseLease();
    jobs_.finish(job_);
}
void CameraWorkerSession::run(Factory factory){
    std::unique_ptr<CameraWorkerBackend> backend;
    try {backend=factory();require(bool(backend),"Camera worker factory returned null.");}
    catch(const std::exception& e){startupError_=QString::fromUtf8(e.what());}
    catch(...){startupError_="Camera worker factory failed.";}
    for(;;){
        CameraTicket ticket;
        {std::unique_lock lock(mutex_);wake_.wait(lock,[this]{return closing_ || command_.has_value();});
            if(closing_)break;
            ticket=*command_;command_.reset();}
        CameraWorkerResult result;
        try {
            if(!backend)result.error=startupError_;
            else if(ticket.cancel->load() || ticket.deadline.expired())result.error="Camera operation cancelled/expired before dispatch.";
            else result=backend->execute(ticket);
        }catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}
        catch(...){result.error="Camera worker operation failed.";}
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
    command_=lifecycle_.begin(operation,timeout);pending_=true;wake_.notify_one();
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
