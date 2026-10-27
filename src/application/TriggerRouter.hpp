#pragma once
#include "RuntimeCoordinator.hpp"
#include "DeliveryStore.hpp"
#include "domain/AssignmentConfig.hpp"
#include <chrono>
namespace visionflow {
struct InputSnapshot {
    QString endpoint,peer,id,recipeId,configId,sourceTime,clockId;
    quint64 revision=0;
    QMap<QString,AssignmentValue> fields;
};
struct InputRequirement { AssignmentValue shape; qint64 maxAgeMs=0; };
// Owner-thread, bounded immutable exact revisions. Local monotonic receipt time is
// authoritative for age; remote clocks are provenance, never synchronization proof.
class InputSnapshotStore {
public:
    using Clock=std::function<qint64()>;
    explicit InputSnapshotStore(Clock clock={});
    void put(const InputSnapshot&);
    std::shared_ptr<const InputSnapshot> get(const QString& endpoint,const QString& peer,const QString& id,quint64 revision) const;
    bool fresh(const InputSnapshot&,qint64 maxAgeMs) const;
private:
    struct Entry {std::shared_ptr<const InputSnapshot> snapshot;qint64 received;};
    void owner() const;
    QThread* thread_=QThread::currentThread();Clock clock_;QMap<QByteArray,Entry> entries_;
};
struct TriggerRequest {
    QString workcell,project,endpoint,peer,requestId,recipeId,configId,inputSnapshotId;
    quint64 sequence=0,inputRevision=0;
};
struct AdmissionResult {
    QString reason,existingState;
    std::optional<ProductionCycle> cycle;
    std::shared_ptr<const InputSnapshot> inputs;
    bool accepted() const {return cycle.has_value();}
};
// All new adapters share one owner-thread instance. Legacy/manual continue through
// coordinator.begin with their original identity/dedup and persistence contracts.
// No wire, execution, journal writes, retry or automatic recovery in this boundary.
class TriggerRouter {
public:
    TriggerRouter(RuntimeCoordinator& coordinator,RuntimeService& runtime,InputSnapshotStore& inputs,DeliveryStore& ledger,std::function<QString()> unavailable)
        :coordinator_(coordinator),runtime_(runtime),inputs_(inputs),ledger_(ledger),unavailable_(std::move(unavailable)) { if(!unavailable_)throw std::invalid_argument("Shared acquisition guard required"); }
    AdmissionResult admit(const TriggerRequest&,const QMap<QString,InputRequirement>& required);
    static DeliveryRequest deliveryRequest(const TriggerRequest&);
private:
    QThread* thread_=QThread::currentThread();
    RuntimeCoordinator& coordinator_;RuntimeService& runtime_;InputSnapshotStore& inputs_;DeliveryStore& ledger_;
    std::function<QString()> unavailable_;
    QMap<QByteArray,QString> admitted_;
};
}
