#pragma once
#include "IndustrialExecution.hpp"
#include "ExperimentalOecsAcquisition.hpp"
#include <future>

namespace visionflow {
enum class AcquisitionSessionState { Closed, Opening, Configuring, Ready, Capturing, Closing, Fault };
struct AcquisitionSourceStatus {
    AcquisitionSessionState state=AcquisitionSessionState::Closed;
    IndustrialCameraRequest request;
    QString physicalKey,acquisitionId,error;
    std::optional<AcquisitionReadback> readback;
};
struct AcquisitionContextStatus {
    QString context;
    quint64 generation=0;
    AcquisitionSessionState state=AcquisitionSessionState::Closed;
    QMap<QString,AcquisitionSourceStatus> sources;
    bool preparing=false,closing=false,drained=true,clean=true,closeBudgetExceeded=false;
    bool allReady() const;
};
struct AcquisitionContextClose {bool clean=true;QString error;};
// Optional in-memory test composition, never decoded from recipes/configuration.
struct AcquisitionOecsBackend {
    QString runtime=ExperimentalOecsAcquisition::installedRuntime();
    ExperimentalOecsAcquisition::ApiFactory api=makeOecsCApi;
};
// Thread-safe command facade. One self-owned coordinator holds every worker on a
// fixed owner thread; SDK objects remain on their existing hardware workers.
// Futures contain values only. Await them off the UI thread or poll wait_for(0).
// Destruction requests shutdown without joining the UI. The coordinator survives
// until actual worker drain, including late/uninterruptible SDK completion. Hosts
// should await shutdown before final application/project teardown.
class CameraAcquisitionSessions {
public:
    CameraAcquisitionSessions();
    ~CameraAcquisitionSessions();
    CameraAcquisitionSessions(const CameraAcquisitionSessions&)=delete;
    CameraAcquisitionSessions& operator=(const CameraAcquisitionSessions&)=delete;
    std::future<AcquisitionContextStatus> prepare(const QString& context,const IndustrialExecutionInputs&,
        QVector<IndustrialCameraRequest>,bool experimental,int timeoutMs=5000,
        std::shared_ptr<const AcquisitionOecsBackend> backend={});
    std::future<IndustrialExecutionFrame> acquire(const QString& context,quint64 generation,
        IndustrialCameraRequest,const QString& cycle,const QString& frame,int timeoutMs,
        std::shared_ptr<std::atomic_bool> cancel={},bool experimental=false,
        std::optional<DiscoveredDevice> admitted={});
    std::future<AcquisitionContextClose> close(const QString& context,int budgetMs=1000);
    std::shared_future<AcquisitionContextClose> shutdown(int budgetMs=1000);
    AcquisitionContextStatus status(const QString& context) const;
private:
    struct Core;
    std::shared_ptr<Core> core_;
};
}
