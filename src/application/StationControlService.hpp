#pragma once
#include "CommunicationPeerSessionService.hpp"
#include "StationRunController.hpp"
#include <QSet>

namespace visionflow {
// Only the saved, immutable revision catalogue may supply this value. Editor
// drafts/imports are not a source. templateKey belongs to the same saved manifest.
struct StationSavedRecipe {StationRecipeSelection selection;quint32 templateKey=0;};
struct StationControlRecord {
    CommunicationV4Frame request, replay;
    QByteArray fingerprint;
    bool acknowledged=false;
};
struct StationControlIntent {V4RequestIdentity identity;StationControlRecord record;bool controlReserve=false;};
class StationControlStore {
public:
    virtual ~StationControlStore()=default;
    // Lookup spans ALL command kinds in the shared request namespace. Production
    // records retain their original request too; their ACK is delegated onward.
    virtual std::optional<StationControlRecord> find(const V4RequestIdentity&) const=0;
    // Same ledger/budget as production: normal1 + STOP/ABORT control1 per station.
    // Atomically reserve durable intent and terminal headroom. Return frozen W7
    // transient reason without writing on rejection. Throws are ambiguous.
    // These operations MUST NOT pump events or call application control ports.
    virtual quint32 reserve(const StationControlIntent&)=0;
    virtual void seal(const V4RequestIdentity&,const CommunicationV4Frame& terminal)=0;
    // Commit exact ACK idempotently; must never release another record/slot.
    virtual void acknowledge(const V4RequestIdentity&)=0;
};
class StationControlService final {
public:
    using SavedRecipes=std::function<std::optional<StationSavedRecipe>(quint32 recipeKey)>;
    StationControlService(StationActivationService&,StationIdentityStore&,StationControlStore*,SavedRecipes);
    void bind(const StationDefinition&,StationRunController&);
    void setRemoteEnabled(bool); // machine-local setting; never taken from wire
    std::optional<CommunicationV4Frame> handle(const V4RequestIdentity&,const CommunicationV4Frame&);
    // Local UI and remote dispatch share these application operations. Stop is
    // always available even without a ledger. Run/Auto requires a durable port.
    std::shared_ptr<const StationSnapshot> select(const QUuid&,const QUuid& expected,quint32 recipeKey,quint32 templateKey);
    void setAuto(const QUuid&,const QUuid& expected,bool);
    void stop(const QUuid&);
    void poll(); // owner event loop; READY only after real Coordinator preparation
private:
    struct Binding {StationDefinition station;StationRunController* run;};
    struct Pending {V4RequestIdentity key;CommunicationV4Frame request;quint64 generation=0;bool automatic=false;std::optional<CommunicationV4Header> selected;};
    void prepareAuto(const QUuid&,const QUuid& expected);
    void owner() const;
    Binding& binding(const QUuid&);
    bool exact(const QUuid&,const CommunicationV4Header&) const;
    quint32 templateKey(const StationSnapshot&) const;
    quint32 peerReason(const V4RequestIdentity&) const;
    void finish(const Pending&,quint32 reason,bool canceled=false);
    StationActivationService& activations_;StationIdentityStore& identities_;
    StationControlStore* store_;SavedRecipes recipes_;
    QMap<QUuid,Binding> bindings_;QVector<Pending> pending_;
    QMap<QUuid,quint64> localAuto_;
    QSet<QUuid> blocked_,dispatching_;
    bool remoteEnabled_=false;
    QThread* owner_=QThread::currentThread();
};
}
