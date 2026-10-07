#include "CameraLifecycle.hpp"
#include <stdexcept>
namespace visionflow {
namespace {void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}}
CameraLifecycle::CameraLifecycle(QString source,FrameQueueMode mode,size_t capacity):source_(std::move(source)),mode_(mode),capacity_(capacity) {
    require(!source_.trimmed().isEmpty() && capacity>0 && capacity<=64,"Camera source/capacity invalid.");
    require(mode==FrameQueueMode::PreviewLatest || mode==FrameQueueMode::ProductionFifo,"Camera queue mode invalid.");
}
void CameraLifecycle::owner() const {require(owner_==std::this_thread::get_id(),"Camera controller accessed outside owner thread.");}
CameraState CameraLifecycle::state() const {owner();return state_;}
quint64 CameraLifecycle::generation() const {owner();return generation_;}
FrameQueueStats CameraLifecycle::stats() const {owner();auto result=stats_;result.depth=queue_.size();return result;}
QString CameraLifecycle::error() const {owner();return error_;}
void CameraLifecycle::clear(){stats_.cleared+=queue_.size();queue_.clear();}
CameraTicket CameraLifecycle::begin(CameraOperation op,int timeout) {
    owner();require(timeout>0 && !active_,"Invalid camera budget or operation still in flight.");
    CameraState next;
    switch(op) {
    case CameraOperation::Connect:require(state_==CameraState::Disconnected,"Connect requires cleanup/disconnected.");next=CameraState::Connecting;break;
    case CameraOperation::Configure:require(state_==CameraState::Connected || state_==CameraState::Configured,"Configure requires stopped connection.");next=CameraState::Configuring;break;
    case CameraOperation::Start:require(state_==CameraState::Configured && !stopping_,"Start requires configured camera.");next=CameraState::Starting;clear();stopDeadline_.reset();++generation_;break;
    case CameraOperation::Grab:
        require(state_==CameraState::Streaming && !stopping_,"Grab requires stream.");
        expire();require(mode_!=FrameQueueMode::ProductionFifo || queue_.size()<capacity_,"Production frame queue full; trigger rejected.");
        next=CameraState::Grabbing;break;
    case CameraOperation::Stop:require(state_==CameraState::Streaming || state_==CameraState::Stopping,"Stop requires stream or stop request.");if(!stopping_)requestStop(timeout);next=CameraState::Stopping;break;
    case CameraOperation::Disconnect:require(state_==CameraState::Connected || state_==CameraState::Configured || state_==CameraState::Fault || state_==CameraState::Stopping,"Disconnect requires drained operation.");next=CameraState::Disconnecting;clear();++generation_;break;
    default:throw std::runtime_error("Unknown camera operation.");
    }
    state_=next;active_=CameraTicket{++serial_,generation_,op,Deadline(timeout),std::make_shared<std::atomic_bool>(false)};return *active_;
}
bool CameraLifecycle::complete(const CameraTicket& ticket,bool success,const QString& error) {
    owner();if(!active_ || active_->serial!=ticket.serial || active_->operation!=ticket.operation || active_->generation!=ticket.generation)return false;
    const auto operation=*active_;active_.reset();
    if(state_==CameraState::Fault)return false;
    // Stop invalidates an outstanding capture/connect/configure/start result.
    if(stopping_ && operation.operation!=CameraOperation::Stop && operation.operation!=CameraOperation::Disconnect){state_=CameraState::Stopping;return false;}
    if(!success || operation.deadline.expired() || operation.cancel->load()){
        error_=error.isEmpty() ? "Camera operation failed/cancelled/deadline exceeded; cleanup required." : error;
        state_=CameraState::Fault;clear();++generation_;return false;
    }
    switch(operation.operation){
    case CameraOperation::Connect:state_=CameraState::Connected;break;
    case CameraOperation::Configure:state_=CameraState::Configured;break;
    case CameraOperation::Start:state_=CameraState::Streaming;break;
    case CameraOperation::Grab:state_=CameraState::Streaming;break;
    case CameraOperation::Stop:state_=CameraState::Configured;stopping_=false;break;
    case CameraOperation::Disconnect:state_=CameraState::Disconnected;stopping_=false;break;
    }
    error_.clear();return true;
}
void CameraLifecycle::requestStop(int budget) {
    owner();require(budget>0,"Stop budget must be positive.");
    if(stopDeadline_ && stopping_)return; // Repeated requests cannot extend the budget.
    stopDeadline_=Deadline(budget);
    if(state_==CameraState::Disconnected || state_==CameraState::Configured || state_==CameraState::Connected){clear();return;}
    stopping_=true;clear();++generation_;
    if(active_)active_->cancel->store(true);
    if(state_!=CameraState::Fault)state_=CameraState::Stopping;
}
StopBudgetState CameraLifecycle::stopBudget() const {
    owner();if(!stopDeadline_)return StopBudgetState::NotRequested;
    if(!stopping_ && !active_ && (state_==CameraState::Disconnected || state_==CameraState::Configured || state_==CameraState::Connected))return StopBudgetState::Complete;
    return stopDeadline_->expired() ? StopBudgetState::Exceeded : StopBudgetState::Pending;
}
void CameraLifecycle::transportLost(const QString& reason) {
    owner();error_=reason;state_=CameraState::Fault;clear();++generation_;
    if(active_)active_->cancel->store(true);
}
void CameraLifecycle::expire(){
    for(auto i=queue_.begin();i!=queue_.end();)if(i->deadline.expired()){i=queue_.erase(i);++stats_.expired;}else ++i;
}
FrameAdmission CameraLifecycle::enqueue(AcquiredFrame frame,quint64 generation,Deadline deadline) {
    owner();
    if(generation!=generation_ || stopping_ || (state_!=CameraState::Streaming && state_!=CameraState::Grabbing)){++stats_.stale;return FrameAdmission::Stale;}
    if(deadline.expired()){++stats_.expired;return FrameAdmission::Expired;}
    const auto& m=frame.image.metadata();
    if(frame.image.size().isEmpty() || frame.sourceId!=source_ || frame.frameId.isEmpty() || frame.cycleId.isEmpty() || m.sourceId!=frame.sourceId || m.frameId!=frame.frameId || m.cycleId!=frame.cycleId){++stats_.invalid;return FrameAdmission::Invalid;}
    expire();auto result=FrameAdmission::Accepted;
    if(queue_.size()==capacity_){
        if(mode_==FrameQueueMode::ProductionFifo){++stats_.rejected;return FrameAdmission::Full;}
        queue_.pop_front();++stats_.dropped;result=FrameAdmission::DroppedOldest;
    }
    queue_.push_back({std::move(frame),deadline});++stats_.accepted;stats_.highWater=std::max(stats_.highWater,queue_.size());return result;
}
std::optional<AcquiredFrame> CameraLifecycle::take(){owner();expire();if(queue_.empty())return {};auto frame=std::move(queue_.front().frame);queue_.pop_front();return frame;}
}
