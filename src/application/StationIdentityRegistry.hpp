#pragma once
#include "domain/StationIdentityRegistry.hpp"
#include <functional>
namespace visionflow {
class StationIdentityStore {
public:
    virtual ~StationIdentityStore()=default;
    virtual IdentityCatalogue load() const=0;
    virtual void create(const IdentityCatalogue&) const=0;
    virtual void compareExchange(const QUuid& expectedRevision, const IdentityCatalogue&) const=0;
};
class StationIdentityRegistry {
public:
    explicit StationIdentityRegistry(StationIdentityStore& store):store_(store){}
    // Explicit enrollment only; load never creates or rewrites an installation.
    IdentityCatalogue initialize(const QUuid& installation);
    IdentityCatalogue snapshot(const QUuid& expectedRevision) const;
    CatalogueEntry bind(const QUuid& expectedRevision, const CatalogueIdentity&, const QString& name);
    void rename(const QUuid& expectedRevision, std::uint32_t key, const QString& name);
    void retire(const QUuid& expectedRevision, std::uint32_t key);
    // Caller must establish drained rollover. This port records history only, not admission permission.
    void enrollEpoch(const QUuid& expectedRevision, const QUuid& peer, const QUuid& epoch);
    void retireEpoch(const QUuid& expectedRevision, const QUuid& peer, const QUuid& epoch);
private:
    void update(const QUuid&, const std::function<void(IdentityCatalogue&)>&);
    StationIdentityStore& store_;
};
}
