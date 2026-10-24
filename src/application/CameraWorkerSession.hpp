#pragma once
#include "CameraLifecycle.hpp"
#include "EditorSession.hpp"
#include "domain/MeasurementSetupContract.hpp"
#include "hardware/AcquisitionTiming.hpp"
#include <condition_variable>
#include <mutex>

namespace visionflow {
enum class CameraLeaseUse { Calibration, Editor, Runtime, Preview };
enum class PreviewAcquisitionKind { SequentialSingleFrame, NativeContinuous };
struct CameraWorkerResult {
    bool success=false;
    QString error;
    std::optional<AcquiredFrame> frame;
    std::optional<AcquisitionReadback> readback;
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
                        FrameQueueMode,Factory,size_t capacity=2,
                        PreviewAcquisitionKind kind=PreviewAcquisitionKind::SequentialSingleFrame);
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
    PreviewAcquisitionKind acquisitionKind() const {owner();return kind_;}
    bool previewCompatible() const {owner();return previewCompatible_;}
    std::optional<bool> cleanShutdown() const;
    std::optional<Deadline::Clock::time_point> lastFrameCompletedAt() const {owner();return frameCompletedAt_;}
    std::optional<AcquisitionReadback> readback() const {owner();return readback_;}
private:
    void owner() const;
    void run(Factory);
    void releaseLease();
    const std::thread::id owner_=std::this_thread::get_id();
    QByteArray leaseKey_;
    PreviewAcquisitionKind kind_;
    bool previewCompatible_=false;
    bool leaseHeld_=true;
    CameraLifecycle lifecycle_;
    JobSession jobs_;
    quint64 job_=0;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::optional<CameraTicket> command_;
    AcquisitionTimingContext timing_,commandTiming_;
    AcquisitionTiming::Clock::time_point submittedAt_;
    struct Completion {CameraTicket ticket;CameraWorkerResult result;Deadline::Clock::time_point completedAt;};
    std::optional<Completion> completion_;
    bool pending_=false,closing_=false,exited_=false,clean_=false;
    std::optional<AcquisitionReadback> readback_;
    std::optional<Deadline::Clock::time_point> frameCompletedAt_;
    QString startupError_,cleanupError_;
    std::thread worker_;
};
}
