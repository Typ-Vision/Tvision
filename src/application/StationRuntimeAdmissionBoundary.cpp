#include "StationRuntimeAdmissionBoundary.hpp"
#include <stdexcept>
namespace visionflow {
StationRuntimeAdmissionBoundary::StationRuntimeAdmissionBoundary(StationRuntimeBoundary& runtime,Manifest manifest,Reserve reserve,Dispatch dispatch)
    :runtime_(runtime),manifest_(std::move(manifest)),reserve_(std::move(reserve)),dispatch_(std::move(dispatch)){}
void StationRuntimeAdmissionBoundary::owner() const {if(QThread::currentThread()!=owner_)throw std::runtime_error("Station admission adapter owner thread required");}
void StationRuntimeAdmissionBoundary::bind(StationRunController& controller){
    owner();const auto id=controller.station();
    if(controllers_.contains(id) || &controller.boundary_!=&runtime_)throw std::runtime_error("Invalid station admission binding");
    runtime_.binding(id);controllers_.insert(id,&controller);
}
std::optional<StationAdmissionSnapshot> StationRuntimeAdmissionBoundary::snapshot(const QUuid& id) const {
    owner();if(!controllers_.contains(id) || !manifest_ || !reserve_ || !dispatch_)return {};
    return manifest_(id);
}
StationAdmissionBarriers StationRuntimeAdmissionBoundary::barriers(const QUuid& id) const {
    owner();if(!controllers_.contains(id))return {};
    const auto state=controllers_[id]->state();const auto& b=runtime_.binding(id);
    const bool armed=state.mode==StationMode::ArmedAuto || state.mode==StationMode::ArmedManual;
    const auto s=manifest_?manifest_(id):std::nullopt;
    const bool activationReady=s && s->station && s->station->activation==controllers_[id]->activation();
    return {state.mode,armed && activationReady && !b.runtime->sourceVerificationBusy(),b.coordinator->recordingPending(),state.deliveryPending,0};
}
StationAdmissionReservation StationRuntimeAdmissionBoundary::reserve(const StationAdmissionSnapshot& s){owner();return reserve_?reserve_(s):StationAdmissionReservation{20,{}};}
void StationRuntimeAdmissionBoundary::begin(const StationAdmissionIntent& intent,const std::shared_ptr<void>& lease){
    owner();auto& c=*runtime_.binding(intent.identity.station).coordinator;
    const auto cycle=c.begin(c.mode(),{{"protocol","vf.socket/4"},{"requestId",QString::number(intent.identity.request)},
        {"station",intent.identity.station.toString(QUuid::WithoutBraces)},{"activation",intent.snapshot.station->activation.toString(QUuid::WithoutBraces)}});
    dispatch_(intent,cycle,lease);
}
bool StationRuntimeAdmissionBoundary::drained(const QUuid& id) const {
    owner();const auto& b=runtime_.binding(id);
    return !b.runtime->busy() && !b.runtime->camerasPending() && !b.runtime->camerasDraining() && !b.coordinator->recordingPending();
}
}
