#include "CameraPreviewSession.hpp"
#include <stdexcept>

namespace visionflow {
void CameraPreviewSession::owner() const {
    if(owner_!=std::this_thread::get_id())throw std::runtime_error("Preview accessed outside owner thread");
}
void CameraPreviewSession::start(Opener opener,bool live,int timeout) {
    owner();
    if(worker_ || timeout<=0 || !opener)throw std::runtime_error("Preview busy or invalid start; drain before camera change/restart");
    ++generation_;latest_.reset();stats_={};firstCompletion_.reset();error_.clear();
    live_=live;timeoutMs_=timeout;state_=PreviewState::Starting;
    try {
        worker_=opener();if(!worker_)throw std::runtime_error("Preview opener returned null");
        if(!worker_->previewCompatible() || worker_->lifecycle().state()!=CameraState::Disconnected)
            throw std::runtime_error("Preview requires a fresh latest-frame worker with capacity one");
        dispatch(CameraOperation::Connect);
    }
    catch(const std::exception& e){fail(QString::fromUtf8(e.what()));}
}
void CameraPreviewSession::dispatch(CameraOperation op) {
    operation_=op;deadline_=Deadline(timeoutMs_);worker_->submit(op,timeoutMs_);
}
void CameraPreviewSession::close(int budget,bool discard) {
    if(discard){++generation_;latest_.reset();}
    deadline_.reset();
    if(worker_)worker_->requestShutdown(budget);
}
void CameraPreviewSession::stop(int budget) {
    owner();if(budget<=0)throw std::runtime_error("Preview stop budget must be positive");
    close(budget,true);
    if(state_!=PreviewState::Error)state_=worker_ ? PreviewState::Stopping : PreviewState::Idle;
}
void CameraPreviewSession::fail(const QString& reason) {
    error_=reason;state_=PreviewState::Error;close(1000,true);
}
void CameraPreviewSession::transportLost(const QString& reason) {owner();fail(reason);}
void CameraPreviewSession::poll() {
    owner();if(!worker_)return;
    try {
        if(state_==PreviewState::Stopping || state_==PreviewState::Error){
            const auto shutdown=worker_->shutdownState();
            if(shutdown==ShutdownState::BudgetExceeded){
                if(state_!=PreviewState::Error){++generation_;latest_.reset();}
                error_="Preview shutdown deadline exceeded; worker still owns device";state_=PreviewState::Error;
            }
            if(shutdown==ShutdownState::Ready){
                // A cleanup failure remains quarantined by CameraWorkerSession.
                const auto reason=worker_->lifecycle().error();
                if(worker_->cleanShutdown()!=std::optional<bool>(true)){
                    error_=reason;state_=PreviewState::Error;++generation_;latest_.reset();
                }
                worker_.reset();if(state_!=PreviewState::Error)state_=PreviewState::Idle;
            }
            return;
        }
        const bool completed=worker_->poll();
        if(worker_->lifecycle().state()==CameraState::Fault){fail(worker_->lifecycle().error());return;}
        if(deadline_ && deadline_->expired()){fail("Preview operation deadline exceeded; cleanup pending");return;}
        if(!completed)return;
        switch(operation_){
        case CameraOperation::Connect:dispatch(CameraOperation::Configure);break;
        case CameraOperation::Configure:dispatch(CameraOperation::Start);break;
        case CameraOperation::Start:state_=PreviewState::Streaming;dispatch(CameraOperation::Grab);break;
        case CameraOperation::Grab:{
            auto frame=worker_->take();if(!frame){fail("Preview completed without a valid frame");return;}
            const auto now=*worker_->lastFrameCompletedAt();
            if(!firstCompletion_)firstCompletion_=now;
            ++stats_.completed;stats_.completedAtUtc=frame->acquiredAtUtc;
            const auto seconds=std::chrono::duration<double>(now-*firstCompletion_).count();
            stats_.fps=stats_.completed>1 && seconds>0 ? double(stats_.completed-1)/seconds : 0;
            if(latest_)++stats_.replaced;
            latest_=PreviewFrame{std::move(*frame),generation_,worker_->acquisitionKind()};
            if(live_)dispatch(CameraOperation::Grab);
            else {state_=PreviewState::Stopping;close(1000,false);}
            break;
        }
        default:fail("Unexpected preview operation");break;
        }
    }catch(const std::exception& e){fail(QString::fromUtf8(e.what()));}
}
std::optional<PreviewFrame> CameraPreviewSession::takeLatest() {
    owner();auto frame=std::move(latest_);latest_.reset();return frame;
}
}
