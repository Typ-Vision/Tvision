#include "RuntimeCoordinator.hpp"
#include "hardware/AcquisitionTiming.hpp"
#include <QElapsedTimer>
#include <type_traits>

namespace visionflow {
namespace {QString stateName(RuntimeState state){switch(state){case RuntimeState::Unloaded:return "Unloaded";case RuntimeState::LoadedStopped:return "LoadedStopped";case RuntimeState::Starting:return "Starting";case RuntimeState::Running:return "Running";case RuntimeState::Stopping:return "Stopping";case RuntimeState::Fault:return "Fault";}return "Unknown";}}
template<class F> auto RuntimeCoordinator::diagnose(const QString& action,F&& operation,QJsonObject context) -> decltype(operation()) {
    owner();QElapsedTimer timer;timer.start();const auto before=state_;
    auto report=[&](const QString& reason,bool failure,std::optional<ProductionError> code={}) noexcept {try {
        auto event=context;event["action"]=action;event["stateBefore"]=stateName(before);event["stateAfter"]=stateName(state_);
        event["generation"]=QString::number(generation_);event["lastAdmittedTicket"]=QString::number(diagnosticTicket_);
        event["trigger"]=mode_==RuntimeTrigger::Manual ? "manual":"software";event["recordPending"]=recordingPending_;event["deliveryPending"]=deliveryPending_;event["cameraPending"]=runtime_.camerasPending();event["cameraDraining"]=runtime_.camerasDraining();event["elapsedMs"]=timer.elapsed();
        if(loaded_)event["recipe"]=RuntimeDiagnostics::recipeContext(*loaded_);if(!reason.isEmpty())event["reason"]=reason;if(code)event["errorCode"]=int(*code);runtime_.diagnostics()->append(event,failure);
    }catch(...){}};
    try {if constexpr(std::is_void_v<decltype(operation())>){operation();report({},false);return;}else {auto result=operation();if constexpr(std::is_same_v<decltype(result),bool>){context["accepted"]=result;if(action=="poll" && !result && before==state_)return result;}report({},false);return result;}}
    catch(const ProductionException& e){report(QString::fromUtf8(e.what()),e.code()!=ProductionError::Busy,e.code());throw;}
    catch(const std::exception& e){report(QString::fromUtf8(e.what()),true);throw;}
}

void RuntimeCoordinator::owner() const {
    if(QThread::currentThread()!=ownerThread_)throw ProductionException(ProductionError::WrongThread,"Runtime coordinator owner thread required.");
}
void RuntimeCoordinator::replaceable() const {
    owner();
    if(runtime_.busy() || runtime_.camerasPending() || deliveryPending_ || state_==RuntimeState::Starting || state_==RuntimeState::Running || state_==RuntimeState::Stopping)
        throw ProductionException(ProductionError::Busy,"Önce Stop ve bekleyen kayıt/teslimin tamamlanması gerekir.");
}
void RuntimeCoordinator::load(const Recipe& recipe,const QString& directory,RuntimeTrigger mode) {
    return diagnose("load",[&]{
    AcquisitionTimingSpan timing("runtime.load-offline");
    replaceable();
    runtime_.checkConfiguration(recipe,directory); // failure preserves previous load
    if(runtime_.requiresManualCycles(recipe,directory) && mode!=RuntimeTrigger::Manual)throw ProductionException(ProductionError::InvalidConfiguration,"Bu kamera akışı yalnız manuel çevrimleri destekler");
    loaded_=recipe;directory_=directory;mode_=mode;
    ++generation_;error_.clear();failure_=RuntimeFailure::None;reloadRequired_=false;state_=RuntimeState::LoadedStopped;

    });
}
quint64 RuntimeCoordinator::requestStart() {
    return diagnose("requestStart",[&]{
    replaceable();
    if(!loaded_)throw ProductionException(ProductionError::UnknownValidation,"NO_RECIPE_LOADED");
    if(reloadRequired_)throw ProductionException(ProductionError::DependencyChanged,"Bağımlılık değişti; Stop sonrası yeniden Load gerekir.");
    error_.clear();failure_=RuntimeFailure::None;faultAfterDrain_=false;state_=RuntimeState::Starting;++generation_;
    try{runtime_.requestCameraPreparation(*loaded_,directory_);}
    catch(const ProductionException& e){remember(e.code()==ProductionError::PersistenceFailure?RuntimeFailure::Persistence:RuntimeFailure::Dependency,QString::fromUtf8(e.what()));faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;}
    catch(const std::exception& e){remember(RuntimeFailure::Dependency,QString::fromUtf8(e.what()));faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;}
    return generation_;

    });
}
bool RuntimeCoordinator::completeStart(quint64 generation,const QString& preparationError) {
    return diagnose("completeStart",[&]() -> bool{
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
    }catch(const ProductionException& e){if(e.code()==ProductionError::DependencyChanged)reloadRequired_=true;remember(e.code()==ProductionError::PersistenceFailure?RuntimeFailure::Persistence:RuntimeFailure::Dependency,QString::fromUtf8(e.what()));faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;
    }catch(const std::exception& e){remember(RuntimeFailure::Dependency,QString::fromUtf8(e.what()));faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;}

    });
}
void RuntimeCoordinator::start(){const auto ticket=requestStart();completeStart(ticket);}
bool RuntimeCoordinator::poll(){
    return diagnose("poll",[&]() -> bool{
    owner();const bool changed=runtime_.pollCameras();
    if(state_==RuntimeState::Stopping && runtime_.cameraCloseFailed() && !faultAfterDrain_){remember(RuntimeFailure::Dependency,runtime_.cameraError());runtime_.diagnostics()->append({{"action","cameraFailure"},{"reason",runtime_.cameraError()},{"generation",QString::number(generation_)},{"draining",runtime_.camerasDraining()}},true);faultAfterDrain_=true;}
    if(state_==RuntimeState::Starting){
        if(!runtime_.cameraError().isEmpty()){remember(RuntimeFailure::Dependency,runtime_.cameraError());runtime_.diagnostics()->append({{"action","cameraFailure"},{"reason",runtime_.cameraError()},{"generation",QString::number(generation_)},{"draining",runtime_.camerasDraining()}},true);faultAfterDrain_=true;runtime_.stopCameras();state_=RuntimeState::Stopping;}
        else if(!runtime_.camerasPending()){try{completeStart(generation_);}catch(const std::exception&){}return true;}
    }
    if(state_==RuntimeState::Stopping && !runtime_.camerasPending() && !runtime_.busy()){state_=faultAfterDrain_?RuntimeState::Fault:loaded_?RuntimeState::LoadedStopped:RuntimeState::Unloaded;return true;}
    return changed;

    });
}
void RuntimeCoordinator::stop() {
    return diagnose("stop",[&]{
    AcquisitionTimingSpan timing("runtime.stop-command");
    owner();++generation_;
    if(cancel_)cancel_->store(true);
    faultAfterDrain_=false;runtime_.stopCameras();
    state_=runtime_.busy() || runtime_.camerasPending() ? RuntimeState::Stopping : loaded_ ? RuntimeState::LoadedStopped : RuntimeState::Unloaded;

    });
}
ProductionCycle RuntimeCoordinator::begin(RuntimeTrigger source,const QJsonObject& communication) {
    return diagnose("begin",[&]{
    owner();
    if(state_!=RuntimeState::Running)throw ProductionException(ProductionError::InvalidConfiguration,"NOT_RUNNING");
    if(source!=mode_)throw ProductionException(ProductionError::InvalidConfiguration,"TRIGGER_SOURCE_DISABLED");
    if(deliveryPending_)throw ProductionException(ProductionError::Busy,"DELIVERY_PENDING");
    // Busy is transient; dependency/authorization/persistence failures close admission.
    try {
        const auto cycle=runtime_.begin(communication);
        diagnosticTicket_=cycle.ticket;cancel_=std::make_shared<std::atomic_bool>(false);return cycle;
    }catch(const ProductionException& e){if(e.code()==ProductionError::DependencyChanged)reloadRequired_=true;if(e.code()!=ProductionError::Busy){remember(e.code()==ProductionError::PersistenceFailure?RuntimeFailure::Persistence:RuntimeFailure::Dependency,QString::fromUtf8(e.what()));faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;}throw;
    }catch(const std::exception& e){remember(RuntimeFailure::Dependency,QString::fromUtf8(e.what()));faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;throw;}

    });
}
void RuntimeCoordinator::finish(quint64 ticket,const CycleResult& result){return diagnose("finish",[&]{
    AcquisitionTimingSpan timing("runtime.finish-persist");
    owner();
    if(frozenCompletion_ && (ticket!=frozenTicket_ || !RuntimeService::sameCompletion(result,*frozenCompletion_)))throw ProductionException(ProductionError::InvalidCompletion,"Kayıt retry sonucu değiştirilemez.");
    const auto cameraFailure=[&]{
        if(runtime_.experimental() && result.state==ExecutionState::Error)return true;
        if(loaded_)for(const auto& t:editorTools(loaded_->definition))if(t.enabled && t.kind=="industrial_camera")for(const auto& r:result.tools)if(r.toolId==t.id && r.state==ExecutionState::Error)return true;
        return false;
    };
    const auto rememberResult=[&]{
    if(result.state==ExecutionState::Error){auto kind=result.acquisitionFailureSourceId_.isEmpty()?RuntimeFailure::Execution:RuntimeFailure::Acquisition;QString cause=result.error;
        if(loaded_)for(const auto& tool:editorTools(loaded_->definition))if(tool.kind=="webcam" || tool.kind=="image_file" || tool.kind=="industrial_camera")for(const auto& outcome:result.tools)if(outcome.toolId==tool.id && outcome.state==ExecutionState::Error){kind=RuntimeFailure::Acquisition;cause=outcome.errorCode+" "+outcome.message;break;}
        remember(kind,cause);
    }
    };
    try {runtime_.finish(ticket,result);}
    catch(const ProductionException& e){
        if(e.code()==ProductionError::PersistenceFailure){recordingPending_=true;rememberResult();
            if(cameraFailure()){runtime_.stopCameras();if(state_==RuntimeState::Running){faultAfterDrain_=true;state_=RuntimeState::Stopping;}}if(!frozenCompletion_){frozenCompletion_=result;frozenTicket_=ticket;}remember(RuntimeFailure::Persistence,QString::fromUtf8(e.what()));}
        else if(e.code()!=ProductionError::Busy){remember(RuntimeFailure::InvalidCompletion,QString::fromUtf8(e.what()));faultAfterDrain_=true;runtime_.stopCameras();state_=runtime_.camerasPending()?RuntimeState::Stopping:RuntimeState::Fault;}
        throw;
    }
    recordingPending_=false;frozenCompletion_.reset();frozenTicket_=0;cancel_.reset();
    if(failure_==RuntimeFailure::Persistence){failure_=RuntimeFailure::None;error_.clear();}
    rememberResult();
    // Valid terminal ERROR keeps the existing acquisition/admission policy.
    // C1 classifies recovery; it does not invent a capture lifecycle fix.
    if(cameraFailure()){faultAfterDrain_=true;runtime_.stopCameras();state_=RuntimeState::Stopping;}
    if(state_==RuntimeState::Stopping && !runtime_.camerasPending())state_=faultAfterDrain_?RuntimeState::Fault:RuntimeState::LoadedStopped;
},{{"ticket",QString::number(ticket)},{"cycleId",result.cycleId}});}
void RuntimeCoordinator::remember(RuntimeFailure kind,const QString& reason){if(error_.isEmpty()){failure_=kind;error_=reason;}}
QString RuntimeCoordinator::recovery()const {owner();switch(failure_){
case RuntimeFailure::Persistence:return recordingPending_ ? "Kayıt hatası — Kaydı tekrar yaz aynı sonucu kaydeder; yeni görüntü almaz.":"Kayıt hatası — disk/erişim sorununu giderip Run ile yeniden hazırlayın; completion retry yok.";
case RuntimeFailure::InvalidCompletion:return "Geçersiz completion — sonuç yayımlanmadı; kayıt retry kullanılamaz. Tanıyı inceleyin; çevrim kapısı korunur.";
case RuntimeFailure::Dependency:return "Bağımlılık / hazırlık hatası — Stop → kaynağı denetle → yeniden Load → Run; model/kurulum için Setup → Calibration.";
case RuntimeFailure::Acquisition:return "Edinim hatası — kaynağı/bağlantıyı denetleyin; Stop → kaynağı denetle → yeniden Load → Run. Otomatik edinim yok.";
case RuntimeFailure::Execution:return "Yürütme hatası — reçete ve tanıyı denetleyin; Stop → Run ile yeniden hazırlayın.";
case RuntimeFailure::None:return {}; }return {};}
void RuntimeCoordinator::retryRecording(){owner();if(!recordingPending_ || !frozenCompletion_)throw ProductionException(ProductionError::InvalidCompletion,"Tekrar yazılacak dondurulmuş completion yok.");const auto result=*frozenCompletion_;finish(frozenTicket_,result);}
void RuntimeCoordinator::setDeliveryPending(bool pending){return diagnose("setDeliveryPending",[&]{deliveryPending_=pending;});}
RuntimeState RuntimeCoordinator::state() const {owner();return state_;}
const std::optional<Recipe>& RuntimeCoordinator::loaded() const {owner();return loaded_;}
QString RuntimeCoordinator::error() const {owner();return error_;}
bool RuntimeCoordinator::recordingPending() const {owner();return recordingPending_;}
bool RuntimeCoordinator::deliveryPending() const {owner();return deliveryPending_;}
std::shared_ptr<std::atomic_bool> RuntimeCoordinator::cancellation() const {owner();return cancel_;}
}
