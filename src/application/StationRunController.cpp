#include "StationRunController.hpp"
namespace visionflow {
StationRunController::StationRunController(StationRuntimeBoundary& boundary,QUuid station):boundary_(boundary),station_(station) {
    if(station.isNull() || !boundary_.idleAndDrained(station))throw std::runtime_error("Station must start stopped/drained");
}
void StationRunController::owner() const {if(owner_!=QThread::currentThread())throw std::runtime_error("Station owner thread required");}
void StationRunController::run(const std::shared_ptr<const StationSnapshot>& snapshot) {
    owner();if(!snapshot || snapshot->station.id!=station_)throw std::runtime_error("Wrong station snapshot");
    auto_=false;boundary_.load(snapshot);activation_=snapshot->activation;
    boundary_.binding(station_).coordinator->requestStart();poll();
}
void StationRunController::setAuto(bool value) {
    owner();if(value && state().mode!=StationMode::ArmedManual && state().mode!=StationMode::ArmedAuto)throw std::runtime_error("Station not armed");
    auto_=value;
}
void StationRunController::stop(){owner();auto_=false;boundary_.binding(station_).coordinator->stop();}
void StationRunController::poll(){owner();boundary_.binding(station_).coordinator->poll();if(state().mode!=StationMode::ArmedAuto && state().mode!=StationMode::Running)auto_=false;}
StationRuntimeState StationRunController::state() const {
    owner();auto result=boundary_.state(station_);
    if(result.mode==StationMode::ArmedManual && auto_)result.mode=StationMode::ArmedAuto;
    return result;
}
bool StationRunController::drained() const {owner();return boundary_.idleAndDrained(station_);}
bool StationRunController::automatic() const {owner();const auto mode=state().mode;return auto_ && (mode==StationMode::ArmedAuto || mode==StationMode::Running);}
}
