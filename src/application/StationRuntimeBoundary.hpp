#pragma once
#include "StationActivationService.hpp"
#include "RuntimeCoordinator.hpp"

namespace visionflow {
// One existing Runtime per station. Binding/lifetimes belong to the composition owner.
// No worker dispatch, Run, Auto, socket or delivery mutation is performed here.
class StationRuntimeBoundary final:public StationActivationBoundary {
public:
    void bind(const QUuid&,RuntimeService&,RuntimeCoordinator&);
    bool idleAndDrained(const QUuid&) const override;
    void checkConfiguration(const StationSnapshot&) const override;
    StationRuntimeState state(const QUuid&) const;
    void load(const std::shared_ptr<const StationSnapshot>&);
private:
    struct Binding {RuntimeService* runtime;RuntimeCoordinator* coordinator;};
    const Binding& binding(const QUuid&) const;
    QMap<QUuid,Binding> bindings_;
};
}
