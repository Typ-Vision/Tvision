#pragma once
#include "domain/StationDefinition.hpp"
#include "StationIdentityRegistry.hpp"
#include <QThread>
#include <memory>

namespace visionflow {
class StationActivationStore {
public:
    virtual ~StationActivationStore()=default;
    virtual StationActivationCatalogue load() const=0;
    virtual void create(const StationActivationCatalogue&) const=0;
    virtual void compareExchange(const QUuid& expectedRevision,const StationActivationCatalogue&) const=0;
};
// Trusted owner-thread application boundary, not UI-supplied ready/busy flags.
class StationActivationBoundary {
public:
    virtual ~StationActivationBoundary()=default;
    virtual bool idleAndDrained(const QUuid& station) const=0;
    virtual void checkConfiguration(const StationSnapshot&) const=0;
};
class StationActivationService {
public:
    StationActivationService(StationActivationStore&,StationIdentityStore&,StationActivationBoundary&);
    void initialize(); // explicit; restore never creates or activates production
    std::shared_ptr<const StationSnapshot> snapshot(const QUuid& station) const;
    QVector<std::shared_ptr<const StationSnapshot>> snapshots() const;
    std::shared_ptr<const StationSnapshot> activate(const StationDefinition&,const QUuid& expectedActivation,const StationRecipeSelection&);
private:
    void owner() const;
    StationActivationStore& store_;
    StationIdentityStore& identities_;
    StationActivationBoundary& boundary_;
    QThread* owner_=QThread::currentThread();
};
}
