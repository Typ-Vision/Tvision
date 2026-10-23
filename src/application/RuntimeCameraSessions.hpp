#pragma once
#include "CameraAcquisitionSessions.hpp"
#include "ProductionRecipeService.hpp"
namespace visionflow {
// Owner-thread Runtime policy. SDK preparation/close run on the common service.
class RuntimeCameraSessions {
public:
    RuntimeCameraSessions();
    ~RuntimeCameraSessions();
    void start(const Recipe&,MultiSourceDependencies,bool experimental,
        std::shared_ptr<const AcquisitionOecsBackend> backend={});
    bool poll();
    void stop();
    bool pending() const;
    bool draining() const;
    bool ready() const;
    QString error() const;
    bool closeFailed() const;
    MultiSourceDependencies compose(MultiSourceDependencies) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
