#pragma once
#include "CameraLifecycle.hpp"
#include "EditorSession.hpp"
#include "domain/MeasurementSetupContract.hpp"
#include <condition_variable>
#include <mutex>

namespace visionflow {
enum class CameraLeaseUse { Calibration, Editor, Runtime };
struct CameraWorkerResult {
    bool success=false;
    QString error;
    std::optional<AcquiredFrame> frame;
};
// Factory, execute, shutdown and destruction all run on the same persistent worker.
// shutdown must release resources even after a failed/partial operation. Throwing
// quarantines the process-local lease; it never permits another owner to open.
class CameraWorkerBackend {
public:
    virtual ~CameraWorkerBackend()=default;
    virtual CameraWorkerResult execute(const CameraTicket&)=0;
    virtual void shutdown()=0;
};
// Software preparation only: no native backend/factory or Runtime trust is installed.
class CameraWorkerSession {
public:
    using Factory=std::function<std::unique_ptr<CameraWorkerBackend>()>;
    CameraWorkerSession(PhysicalDeviceIdentity,CameraLeaseUse,QString source,
                        FrameQueueMode,Factory,size_t capacity=2);
    ~CameraWorkerSession(); // cancels then joins; can block on an uninterruptible SDK
    CameraWorkerSession(const CameraWorkerSession&)=delete;
    CameraWorkerSession& operator=(const CameraWorkerSession&)=delete;
    void submit(CameraOperation,int timeoutMs=5000);
    bool poll(); // delivers completion on controller owner thread
    // Invalidates/cancels immediately. After the pending command drains, submit
    // Stop for backend acknowledgement (or requestShutdown for final cleanup).
    void requestStop(int budgetMs=1000);
    void transportLost(const QString&);
    void requestShutdown(int budgetMs=1000);
    ShutdownState shutdownState();
    const CameraLifecycle& lifecycle() const {return lifecycle_;}
    std::optional<AcquiredFrame> take();
private:
    void owner() const;
    void run(Factory);
    void releaseLease();
    const std::thread::id owner_=std::this_thread::get_id();
    QByteArray leaseKey_;
    bool leaseHeld_=true;
    CameraLifecycle lifecycle_;
    JobSession jobs_;
    quint64 job_=0;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::optional<CameraTicket> command_;
    struct Completion {CameraTicket ticket;CameraWorkerResult result;};
    std::optional<Completion> completion_;
    bool pending_=false,closing_=false,exited_=false,clean_=false;
    QString startupError_,cleanupError_;
    std::thread worker_;
};
}
