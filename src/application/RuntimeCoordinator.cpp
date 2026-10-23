#include "RuntimeCoordinator.hpp"
#include "hardware/AcquisitionTiming.hpp"

namespace visionflow {
void RuntimeCoordinator::owner() const {
    if(QThread::currentThread()!=ownerThread_)throw ProductionException(ProductionError::WrongThread,"Runtime coordinator owner thread required.");
}
void RuntimeCoordinator::replaceable() const {
    owner();
    if(runtime_.busy() || runtime_.camerasPending() || deliveryPending_ || state_==RuntimeState::Starting || state_==RuntimeState::Running || state_==RuntimeState::Stopping)
        throw ProductionException(ProductionError::Busy,"Önce Stop ve bekleyen kayıt/teslimin tamamlanması gerekir.");
}
void RuntimeCoordinator::load(const Recipe& recipe,const QString& directory,RuntimeTrigger mode) {
    AcquisitionTimingSpan timing("runtime.load-offline");
    replaceable();
    runtime_.checkConfiguration(recipe,directory); // failure preserves previous load
    if(runtime_.requiresManualCycles(recipe,directory) && mode!=RuntimeTrigger::Manual)throw ProductionException(ProductionError::InvalidConfiguration,"Bu kamera akışı yalnız manuel çevrimleri destekler");
    loaded_=recipe;directory_=directory;mode_=mode;
    ++generation_;error_.clear();reloadRequired_=false;state_=RuntimeState::LoadedStopped;
}
quint64 RuntimeCoordinator::requestStart() {
    replaceable();
    if(!loaded_)throw ProductionException(ProductionError::UnknownValidation,"NO_RECIPE_LOADED");
    if(reloadRequired_)throw ProductionException(ProductionError::DependencyChanged,"Bağımlılık değişti; Stop sonrası yeniden Load gerekir.");
    error_.clear();faultAfterDrain_=false;state_=RuntimeState::Starting;++generation_;
    try{runtime_.requestCameraPreparation(*loaded_,directory_);}
    catch(const std::exception& e){error_=QString::fromUtf8(e.what());faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;}
    return generation_;
}
bool RuntimeCoordinator::completeStart(quint64 generation,const QString& preparationError) {
    const auto inherited=AcquisitionTiming::current();AcquisitionTimingBinding binding(inherited.request.isEmpty()?AcquisitionTiming::request("runtime"):inherited);AcquisitionTimingSpan timing("runtime.prepare");
    owner();
    if(generation!=generation_ || state_!=RuntimeState::Starting)return false;
    if(runtime_.camerasPending() && preparationError.isEmpty())return false;
    try {
        if(!preparationError.isEmpty())throw ProductionException(ProductionError::InvalidConfiguration,preparationError);
        if(!runtime_.cameraError().isEmpty())throw ProductionException(ProductionError::InvalidConfiguration,runtime_.cameraError());
        runtime_.prepare(*loaded_,directory_);
        runtime_.requireReady(); // Run permission/current dependency before accepting events
        state_=RuntimeState::Running;return true;
    }catch(const ProductionException& e){if(e.code()==ProductionError::DependencyChanged)reloadRequired_=true;error_=QString::fromUtf8(e.what());faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;
    }catch(const std::exception& e){error_=QString::fromUtf8(e.what());faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;}
}
void RuntimeCoordinator::start(){const auto ticket=requestStart();completeStart(ticket);}
bool RuntimeCoordinator::poll(){
    owner();const bool changed=runtime_.pollCameras();
    if(state_==RuntimeState::Stopping && runtime_.cameraCloseFailed()){error_=runtime_.cameraError();faultAfterDrain_=true;}
    if(state_==RuntimeState::Starting){
        if(!runtime_.cameraError().isEmpty()){error_=runtime_.cameraError();faultAfterDrain_=true;runtime_.stopCameras();state_=RuntimeState::Stopping;}
        else if(!runtime_.camerasPending()){try{completeStart(generation_);}catch(const std::exception&){}return true;}
    }
    if(state_==RuntimeState::Stopping && !runtime_.camerasPending() && !runtime_.busy()){state_=faultAfterDrain_?RuntimeState::Fault:loaded_?RuntimeState::LoadedStopped:RuntimeState::Unloaded;return true;}
    return changed;
}
void RuntimeCoordinator::stop() {
    AcquisitionTimingSpan timing("runtime.stop-command");
    owner();++generation_;
    if(cancel_)cancel_->store(true);
    faultAfterDrain_=false;runtime_.stopCameras();
    state_=runtime_.busy() || runtime_.camerasPending() ? RuntimeState::Stopping : loaded_ ? RuntimeState::LoadedStopped : RuntimeState::Unloaded;
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
    }catch(const ProductionException& e){if(e.code()==ProductionError::DependencyChanged)reloadRequired_=true;if(e.code()!=ProductionError::Busy){error_=QString::fromUtf8(e.what());faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;}throw;
    }catch(const std::exception& e){error_=QString::fromUtf8(e.what());faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;}
}
void RuntimeCoordinator::finish(quint64 ticket,const CycleResult& result) {
    AcquisitionTimingSpan timing("runtime.finish-persist");
    owner();
    bool cameraFault=false;if(loaded_)for(const auto& t:editorTools(loaded_->definition))if(t.enabled && t.kind=="industrial_camera")for(const auto& toolResult:result.tools)if(toolResult.toolId==t.id && toolResult.state==ExecutionState::Error)cameraFault=true;
    if(cameraFault){faultAfterDrain_=true;error_=result.error.isEmpty()?"Camera acquisition failed; manual Run required":result.error;runtime_.stopCameras();state_=RuntimeState::Stopping;}
    try {runtime_.finish(ticket,result);}
    catch(const ProductionException& e){if(e.code()==ProductionError::PersistenceFailure)recordingPending_=true;throw;}
    recordingPending_=false;cancel_.reset();
    if(runtime_.experimental() && result.state==ExecutionState::Error){error_=result.error;faultAfterDrain_=true;runtime_.stopCameras();state_=RuntimeState::Stopping;}
    if(state_==RuntimeState::Stopping && !runtime_.camerasPending())state_=faultAfterDrain_?RuntimeState::Fault:RuntimeState::LoadedStopped;
}
void RuntimeCoordinator::setDeliveryPending(bool pending){owner();deliveryPending_=pending;}
RuntimeState RuntimeCoordinator::state() const {owner();return state_;}
const std::optional<Recipe>& RuntimeCoordinator::loaded() const {owner();return loaded_;}
QString RuntimeCoordinator::error() const {owner();return error_;}
bool RuntimeCoordinator::recordingPending() const {owner();return recordingPending_;}
bool RuntimeCoordinator::deliveryPending() const {owner();return deliveryPending_;}
std::shared_ptr<std::atomic_bool> RuntimeCoordinator::cancellation() const {owner();return cancel_;}
}
