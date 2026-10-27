#pragma once
#include "RuntimeService.hpp"

namespace visionflow {
enum class RuntimeState { Unloaded, LoadedStopped, Starting, Running, Stopping, Fault };
enum class RuntimeFailure { None, Persistence, InvalidCompletion, Dependency, Acquisition, Execution };
enum class RuntimeTrigger { Manual, SoftwareTrigger };

// Owner-thread lifecycle over the existing immutable/persistent Runtime service.
// Device adapters may prepare asynchronously between requestStart/completeStart.
class RuntimeCoordinator {
public:
    explicit RuntimeCoordinator(RuntimeService& runtime):runtime_(runtime) {}
    void load(const Recipe& saved,const QString& directory,RuntimeTrigger mode=RuntimeTrigger::Manual);
    quint64 requestStart();
    bool completeStart(quint64 generation,const QString& preparationError={});
    void start();
    bool poll();
    void stop();
    ProductionCycle begin(RuntimeTrigger source,const QJsonObject& communication={});
    void finish(quint64 ticket,const CycleResult& result);
    void setDeliveryPending(bool pending);
    // Each adapter owns its barrier; another endpoint's ACK cannot clear it.
    void setDeliveryPending(const QString& owner,bool pending);
    RuntimeState state() const;
    const std::optional<Recipe>& loaded() const;
    RuntimeTrigger mode() const {owner();return mode_;}
    QString error() const;
    std::shared_ptr<const RuntimeVariableContext> variables() const;
    RuntimeFailure failure() const {owner();return failure_;}
    QString recovery() const;
    void retryRecording();
    bool recordingPending() const;
    bool deliveryPending() const;
    std::shared_ptr<std::atomic_bool> cancellation() const;
private:
    template<class F> auto diagnose(const QString& action,F&& operation,QJsonObject context={}) -> decltype(operation());
    void remember(RuntimeFailure,const QString&);
    void owner() const;
    void replaceable() const;
    RuntimeService& runtime_;
    QThread* ownerThread_=QThread::currentThread();
    RuntimeState state_=RuntimeState::Unloaded;
    RuntimeTrigger mode_=RuntimeTrigger::Manual;
    std::optional<Recipe> loaded_;
    QString directory_,error_;
    RuntimeFailure failure_=RuntimeFailure::None;
    std::optional<CycleResult> frozenCompletion_;
    quint64 frozenTicket_=0,diagnosticTicket_=0;
    quint64 generation_=0;
    std::shared_ptr<const RuntimeVariableContext> variables_;
    bool recordingPending_=false,deliveryPending_=false;
    QSet<QString> deliveryOwners_;
    bool faultAfterDrain_=false,reloadRequired_=false;
    std::shared_ptr<std::atomic_bool> cancel_;
};
}
