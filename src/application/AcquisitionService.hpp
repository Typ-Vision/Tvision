#pragma once
#include "ImageFileService.hpp"
#include "Deadline.hpp"
#include <QDateTime>
#include <atomic>
#include <memory>
#include <optional>

namespace visionflow {
enum class TriggerMode { Software, External, Continuous };
enum class AcquisitionState { Done, Error, Cancelled, Timeout };
struct AcquisitionCapabilities {
    QString sourceKind;
    bool softwareTrigger=false, externalTrigger=false, continuous=false;
    bool exposure=false, gain=false;
};
struct AcquisitionRequest {
    QString sourceId, cycleId, frameId;
    TriggerMode trigger=TriggerMode::Software;
    int timeoutMs=5000;
    std::shared_ptr<std::atomic_bool> cancel;
    // Set at admission to include queue time; service also caps it by timeoutMs.
    std::optional<Deadline> deadline;
};
struct AcquiredFrame {
    ImageFrame image;
    QString sourceId, cycleId, frameId;
    QDateTime acquiredAtUtc;
};
struct AcquisitionResult {
    AcquisitionState state=AcquisitionState::Error;
    std::optional<AcquiredFrame> frame;
    QString error;
    double milliseconds=0;
};
// Adapters run on the caller's worker thread; they never access UI objects.
class AcquisitionAdapter {
public:
    virtual ~AcquisitionAdapter()=default;
    virtual AcquisitionCapabilities capabilities() const=0;
    virtual ImageFrame acquire(const AcquisitionRequest& request) const=0;
};
class ImageFileAdapter final : public AcquisitionAdapter {
public:
    explicit ImageFileAdapter(QString absolutePath,bool experimentalEditorReplay=false):path_(std::move(absolutePath)),experimentalEditorReplay_(experimentalEditorReplay) {}
    AcquisitionCapabilities capabilities() const override;
    ImageFrame acquire(const AcquisitionRequest& request) const override;
private:
    QString path_;
    bool experimentalEditorReplay_=false;
};
class AcquisitionService {
public:
    AcquisitionResult acquire(const AcquisitionAdapter& adapter,const AcquisitionRequest& request) const;
};
}
