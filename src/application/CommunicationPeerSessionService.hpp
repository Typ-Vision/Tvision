#pragma once
#include "CommunicationV4Codec.hpp"
#include "StationIdentityRegistry.hpp"
#include <QTcpSocket>
#include <QThread>
#include <optional>

namespace visionflow {
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
    explicit CommunicationPeerSessionService(StationIdentityStore&,Metadata={});
    CommunicationPeerSessionService(const CommunicationPeerSessionService&)=delete;
    CommunicationPeerSessionService& operator=(const CommunicationPeerSessionService&)=delete;
    static QUuid contractRevision();
    bool open(const QUuid& connection);
    void close(const QUuid& connection);
    quint32 authorize(const QUuid& connection,const CommunicationV4Frame&) const;
    CommunicationV4Frame handle(const QUuid& connection,const CommunicationV4Frame&);
    V4RequestIdentity requestIdentity(const QUuid& connection,const CommunicationV4Frame&) const;
    QUuid session() const {owner();return session_;}
private:
    void owner() const;
    quint32 identityReason(const IdentityCatalogue&,const CommunicationV4Header&) const;
    CommunicationV4Frame response(const CommunicationV4Frame&,quint32 kind,quint32 reason) const;
    StationIdentityStore& store_;
    Metadata metadata_;
    QUuid connection_,session_,peer_,epoch_,installation_;
    bool exact_=false;
    QThread* owner_=QThread::currentThread();
};
// Owns bounded framing/write state for one socket. All callbacks are confined to
// the Qt owner thread. No Runtime/SDK/ledger mutation is performed by this port.
void attachCommunicationPeer(QTcpSocket&,CommunicationPeerSessionService&);
}
