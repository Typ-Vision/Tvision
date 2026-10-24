#pragma once
#include "CameraAcquisitionSessions.hpp"

namespace visionflow {
struct EditorCameraTarget {IndustrialCameraRequest request;bool experimental=false;};
// UI-thread policy over the thread-safe acquisition service. poll() never waits.
// Only explicit open/acquire activates it; restore and catalog reads do not.
class EditorCameraSessions {
public:
    EditorCameraSessions();
    ~EditorCameraSessions();
    void reconcile(const QString& recipeContext,const IndustrialExecutionInputs&,
        QVector<EditorCameraTarget>,const QString& retrySource={});
    void stop();
    bool poll();
    bool active() const;
    bool hasTargets() const;
    bool draining() const;
    bool pending(const QString& source={}) const;
    std::optional<AcquisitionSourceStatus> source(const QString&) const;
    IndustrialExecutionInputs executionInputs() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
