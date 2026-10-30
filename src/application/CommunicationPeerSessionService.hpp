#pragma once
#include "CommunicationV4Codec.hpp"
#include "StationIdentityRegistry.hpp"
#include <QTcpSocket>
#include <QThread>
#include <optional>

namespace visionflow {
class StationDeliveryLedger;
// Transport session is deliberately absent from the durable request namespace.
struct V4RequestIdentity {
    QUuid installation, peer, epoch, station;
    quint64 request=0;
    bool operator==(const V4RequestIdentity&) const = default;
};
class CommunicationPeerSessionService final {
public:
    // Metadata must come from the immutable local manifest, never synthesized
    // from display names/registry UUIDs. Missing publication remains NOT_READY.
    using Metadata=std::function<QByteArray(const IdentityCatalogue&,const CatalogueEntry&)>;
    using TriggerHandler=std::function<CommunicationV4Frame(const V4RequestIdentity&,const CommunicationV4Frame&)>;
    explicit CommunicationPeerSessionService(StationIdentityStore&,Metadata={});
    void setMetadata(Metadata metadata){owner();metadata_=std::move(metadata);}
    using StatusHandler=std::function<CommunicationV4Frame(const CommunicationV4Frame&)>;
    void setStatusHandler(StatusHandler handler){owner();status_=std::move(handler);}
    CommunicationPeerSessionService(const CommunicationPeerSessionService&)=delete;
    CommunicationPeerSessionService& operator=(const CommunicationPeerSessionService&)=delete;
    static QUuid contractRevision();
    bool open(const QUuid& connection);
    void close(const QUuid& connection);
    quint32 authorize(const QUuid& connection,const CommunicationV4Frame&) const;
    CommunicationV4Frame handle(const QUuid& connection,const CommunicationV4Frame&);
    V4RequestIdentity requestIdentity(const QUuid& connection,const CommunicationV4Frame&) const;
    QUuid session() const {owner();return session_;}
    void setTriggerHandler(TriggerHandler handler){owner();trigger_=std::move(handler);}
    // SELECT_RECIPE/SET_AUTO/STOP and exact control ACK. The application port
    // returns nullopt for ACKs belonging to the production/calibration ledger.
    using ControlHandler=std::function<std::optional<CommunicationV4Frame>(const V4RequestIdentity&,const CommunicationV4Frame&)>;
    void setControlHandler(ControlHandler handler){owner();control_=std::move(handler);}
    void setDeliveryLedger(StationDeliveryLedger* ledger){owner();ledger_=ledger;}
    StationDeliveryLedger* deliveryLedger() const {owner();return ledger_;}
private:
    void owner() const;
    quint32 identityReason(const IdentityCatalogue&,const CommunicationV4Header&) const;
    CommunicationV4Frame response(const CommunicationV4Frame&,quint32 kind,quint32 reason) const;
    StationIdentityStore& store_;
    Metadata metadata_;
    StatusHandler status_;
    TriggerHandler trigger_;
    ControlHandler control_;
    StationDeliveryLedger* ledger_=nullptr; // application-owned; outlives connections
    QUuid connection_,session_,peer_,epoch_,installation_;
    bool exact_=false;
    QThread* owner_=QThread::currentThread();
};
// Owns bounded framing/write state for one socket. All callbacks are confined to
// the Qt owner thread. No Runtime/SDK/ledger mutation is performed by this port.
void attachCommunicationPeer(QTcpSocket&,CommunicationPeerSessionService&);
}
