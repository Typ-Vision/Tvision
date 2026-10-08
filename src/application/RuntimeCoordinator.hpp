#pragma once
#include "RuntimeService.hpp"

namespace visionflow {
enum class RuntimeState { Unloaded, LoadedStopped, Starting, Running, Stopping, Fault };
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
    void stop();
    ProductionCycle begin(RuntimeTrigger source,const QJsonObject& communication={});
    void finish(quint64 ticket,const CycleResult& result);
    void setDeliveryPending(bool pending);
    RuntimeState state() const;
    const std::optional<Recipe>& loaded() const;
    RuntimeTrigger mode() const {owner();return mode_;}
    QString error() const;
    bool recordingPending() const;
    bool deliveryPending() const;
    std::shared_ptr<std::atomic_bool> cancellation() const;
private:
    void owner() const;
    void replaceable() const;
    RuntimeService& runtime_;
    QThread* ownerThread_=QThread::currentThread();
    RuntimeState state_=RuntimeState::Unloaded;
    RuntimeTrigger mode_=RuntimeTrigger::Manual;
    std::optional<Recipe> loaded_;
    QString directory_,error_;
    quint64 generation_=0;
    bool recordingPending_=false,deliveryPending_=false;
    std::shared_ptr<std::atomic_bool> cancel_;
};
}
