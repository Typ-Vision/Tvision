#pragma once
#include "AcquisitionService.hpp"
#include <deque>
#include <thread>

namespace visionflow {
enum class CameraState { Disconnected, Connecting, Connected, Configuring, Configured, Starting, Streaming, Grabbing, Stopping, Disconnecting, Fault };
enum class CameraOperation { Connect, Configure, Start, Grab, Stop, Disconnect };
enum class FrameQueueMode { PreviewLatest, ProductionFifo };
enum class FrameAdmission { Accepted, DroppedOldest, Full, Expired, Stale, Invalid };
enum class StopBudgetState { NotRequested, Pending, Exceeded, Complete };
struct CameraTicket {
    quint64 serial=0, generation=0;
    CameraOperation operation=CameraOperation::Connect;
    Deadline deadline{0};
    std::shared_ptr<std::atomic_bool> cancel;
};
struct FrameQueueStats {
    quint64 accepted=0,dropped=0,rejected=0,expired=0,stale=0,invalid=0,cleared=0;
    size_t depth=0,highWater=0;
};
// Owner-thread controller, no SDK calls. A worker owns handles and returns tickets.
// No transition releases a worker/SDK handle or claims a hard interruption.
class CameraLifecycle {
public:
    CameraLifecycle(QString sourceId,FrameQueueMode mode,size_t capacity=2);
    CameraLifecycle(const CameraLifecycle&)=delete;
    CameraLifecycle& operator=(const CameraLifecycle&)=delete;
    CameraTicket begin(CameraOperation operation,int timeoutMs=5000);
    bool complete(const CameraTicket& ticket,bool success,const QString& error={});
    void requestStop(int budgetMs=1000);
    void transportLost(const QString& reason);
    StopBudgetState stopBudget() const;
    FrameAdmission enqueue(AcquiredFrame frame,quint64 generation,Deadline deadline);
    std::optional<AcquiredFrame> take();
    CameraState state() const;
    quint64 generation() const;
    FrameQueueStats stats() const;
    QString error() const;
private:
    void owner() const;
    void clear();
    void expire();
    struct Entry {AcquiredFrame frame;Deadline deadline;};
    const std::thread::id owner_=std::this_thread::get_id();
    QString source_,error_;
    FrameQueueMode mode_;
    size_t capacity_;
    CameraState state_=CameraState::Disconnected;
    quint64 serial_=0,generation_=0;
    std::optional<CameraTicket> active_;
    std::optional<Deadline> stopDeadline_;
    bool stopping_=false;
    std::deque<Entry> queue_;
    FrameQueueStats stats_;
};
}
