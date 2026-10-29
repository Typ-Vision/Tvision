#include "StationRuntimeBoundary.hpp"

namespace visionflow {
const StationRuntimeBoundary::Binding& StationRuntimeBoundary::binding(const QUuid& id) const {
    auto found=bindings_.constFind(id);
    if(found==bindings_.cend())throw std::runtime_error("Station runtime not bound");return found.value();
}
void StationRuntimeBoundary::bind(const QUuid& id,RuntimeService& runtime,RuntimeCoordinator& coordinator) {
    if(id.isNull() || bindings_.contains(id) || &coordinator.runtime_!=&runtime)throw std::runtime_error("Invalid/duplicate station runtime binding");
    for(const auto& b:bindings_)if(b.runtime==&runtime || b.coordinator==&coordinator)throw std::runtime_error("Station runtime must be independent");
    bindings_.insert(id,{&runtime,&coordinator});
}
bool StationRuntimeBoundary::idleAndDrained(const QUuid& id) const {
    const auto& b=binding(id);const auto state=b.coordinator->state();
    // Pending delivery deliberately excluded: the station ledger keeps its own barrier.
    return (state==RuntimeState::Unloaded || state==RuntimeState::LoadedStopped) && !b.runtime->busy() && !b.runtime->camerasPending()
        && !b.runtime->camerasDraining() && !b.runtime->sourceVerificationBusy() && !b.coordinator->recordingPending();
}
void StationRuntimeBoundary::checkConfiguration(const StationSnapshot& snapshot) const {
    const auto& b=binding(snapshot.station.id);
    b.runtime->checkConfiguration(snapshot.selection.recipe,snapshot.selection.projectDirectory);
}
StationRuntimeState StationRuntimeBoundary::state(const QUuid& id) const {
    const auto& b=binding(id);StationRuntimeState result;result.deliveryPending=b.coordinator->deliveryPending();result.health=b.coordinator->error();
    switch(b.coordinator->state()) {
    case RuntimeState::Unloaded:case RuntimeState::LoadedStopped:result.mode=StationMode::Stopped;break;
    case RuntimeState::Starting:result.mode=StationMode::Preparing;break;
    case RuntimeState::Stopping:result.mode=StationMode::Draining;break;
    case RuntimeState::Fault:result.mode=StationMode::Blocked;break;
    case RuntimeState::Running:result.mode=b.runtime->busy()?StationMode::Running:StationMode::ArmedManual;break;
    }
    return result;
}
void StationRuntimeBoundary::load(const std::shared_ptr<const StationSnapshot>& snapshot) {
    if(!snapshot || !idleAndDrained(snapshot->station.id))throw std::runtime_error("Station not idle/drained");
    const auto& b=binding(snapshot->station.id);
    // Legacy load also blocks pending delivery. Selection itself remains independent;
    // the immutable pending terminal must be ACKed before preparing the next Runtime.
    b.coordinator->load(snapshot->selection.recipe,snapshot->selection.projectDirectory,RuntimeTrigger::Manual);
}
}
