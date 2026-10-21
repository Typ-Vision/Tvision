#include "RuntimeCoordinator.hpp"

namespace visionflow {
void RuntimeCoordinator::owner() const {
    if(QThread::currentThread()!=ownerThread_)throw ProductionException(ProductionError::WrongThread,"Runtime coordinator owner thread required.");
}
void RuntimeCoordinator::replaceable() const {
    owner();
    if(runtime_.busy() || deliveryPending_ || state_==RuntimeState::Starting || state_==RuntimeState::Running || state_==RuntimeState::Stopping)
        throw ProductionException(ProductionError::Busy,"Önce Stop ve bekleyen kayıt/teslimin tamamlanması gerekir.");
}
void RuntimeCoordinator::load(const Recipe& recipe,const QString& directory,RuntimeTrigger mode) {
    replaceable();
    runtime_.checkConfiguration(recipe,directory); // failure preserves previous load
    if(runtime_.experimental() && mode!=RuntimeTrigger::Manual)throw ProductionException(ProductionError::InvalidConfiguration,"Experimental Runtime permits manual cycles only");
    loaded_=recipe;directory_=directory;mode_=mode;
    ++generation_;error_.clear();state_=RuntimeState::LoadedStopped;
}
quint64 RuntimeCoordinator::requestStart() {
    replaceable();
    if(!loaded_)throw ProductionException(ProductionError::UnknownValidation,"NO_RECIPE_LOADED");
    error_.clear();state_=RuntimeState::Starting;return ++generation_;
}
bool RuntimeCoordinator::completeStart(quint64 generation,const QString& preparationError) {
    owner();
    if(generation!=generation_ || state_!=RuntimeState::Starting)return false;
    try {
        if(!preparationError.isEmpty())throw ProductionException(ProductionError::InvalidConfiguration,preparationError);
        runtime_.prepare(*loaded_,directory_);
        runtime_.requireReady(); // Run permission/current dependency before accepting events
        state_=RuntimeState::Running;return true;
    }catch(const std::exception& e){error_=QString::fromUtf8(e.what());state_=RuntimeState::Fault;throw;}
}
void RuntimeCoordinator::start(){const auto ticket=requestStart();completeStart(ticket);}
void RuntimeCoordinator::stop() {
    owner();++generation_;
    if(cancel_)cancel_->store(true);
    state_=runtime_.busy() ? RuntimeState::Stopping : loaded_ ? RuntimeState::LoadedStopped : RuntimeState::Unloaded;
}
ProductionCycle RuntimeCoordinator::begin(RuntimeTrigger source,const QJsonObject& communication) {
    owner();
    if(state_!=RuntimeState::Running)throw ProductionException(ProductionError::InvalidConfiguration,"NOT_RUNNING");
    if(source!=mode_)throw ProductionException(ProductionError::InvalidConfiguration,"TRIGGER_SOURCE_DISABLED");
    if(deliveryPending_)throw ProductionException(ProductionError::Busy,"DELIVERY_PENDING");
    // Busy is transient; dependency/authorization/persistence failures close admission.
    try {
        const auto cycle=runtime_.begin(communication);
        cancel_=std::make_shared<std::atomic_bool>(false);return cycle;
    }catch(const ProductionException& e){if(e.code()!=ProductionError::Busy){error_=QString::fromUtf8(e.what());state_=RuntimeState::Fault;}throw;}
}
void RuntimeCoordinator::finish(quint64 ticket,const CycleResult& result) {
    owner();
    try {runtime_.finish(ticket,result);}
    catch(const ProductionException& e){if(e.code()==ProductionError::PersistenceFailure)recordingPending_=true;throw;}
    recordingPending_=false;cancel_.reset();
    if(runtime_.experimental() && result.state==ExecutionState::Error){error_=result.error;state_=RuntimeState::Fault;}
    if(state_==RuntimeState::Stopping)state_=RuntimeState::LoadedStopped;
}
void RuntimeCoordinator::setDeliveryPending(bool pending){owner();deliveryPending_=pending;}
RuntimeState RuntimeCoordinator::state() const {owner();return state_;}
const std::optional<Recipe>& RuntimeCoordinator::loaded() const {owner();return loaded_;}
QString RuntimeCoordinator::error() const {owner();return error_;}
bool RuntimeCoordinator::recordingPending() const {owner();return recordingPending_;}
bool RuntimeCoordinator::deliveryPending() const {owner();return deliveryPending_;}
std::shared_ptr<std::atomic_bool> RuntimeCoordinator::cancellation() const {owner();return cancel_;}
}
