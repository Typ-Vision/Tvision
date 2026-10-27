#pragma once
#include "ProductionStore.hpp"
#include <functional>
namespace visionflow {
struct DeliveryRequest {
    QString workcell,project,endpoint,peer,requestId,fingerprint;
};
struct DeliveryStatus {
    QString state="ABSENT",cycleId,payloadHash;
    QByteArray payload;
    int attempts=0;
    bool dispatchable() const {return state=="DELIVERY_PENDING" || state=="ATTEMPT";}
};
struct DeliveryRecovery { DeliveryRequest request; DeliveryStatus status; };
// Durable evidence only. No execution, network, automatic recovery or Run authority.
class DeliveryStore {
public:
    using Fault=std::function<void(const QString& boundary)>;
    explicit DeliveryStore(QString directory,Fault fault={});
    QVector<DeliveryRecovery> recovery() const;
    QJsonObject recoveryTerminal(const DeliveryRequest&) const;
    DeliveryStatus inspect(const DeliveryRequest&) const;
    DeliveryStatus request(const DeliveryRequest&,bool required=false,bool online=true) const;
    void begin(const DeliveryRequest&,const QString& cycleId) const;
    void commit(const DeliveryRequest&,const QJsonObject& terminal,const QByteArray& payload) const;
    void attempt(const DeliveryRequest&) const;
    void acknowledge(const DeliveryRequest&,const QString& payloadHash) const;
    void commitAcknowledgement(const DeliveryRequest&) const;
    // Caller supplies a transport; callback never receives uncommitted bytes.
    bool dispatch(const DeliveryRequest&,const std::function<void(const QByteArray&)>&) const;
    // Admission preflight; no eviction. Required blocks on offline/pending;
    // optional retains at most 128 unresolved requests per endpoint.
    QString admission(const DeliveryRequest&,bool required=true,bool online=true) const;
    void requireNamespace(const DeliveryRequest&) const;
    // Peer identity includes its operator-agreed generation. Retire only drained
    // namespaces, persist peer agreement, and never re-arm a retired generation.
    void rolloverNamespace(const DeliveryRequest& from,const DeliveryRequest& to,const QString& peerAgreement) const;
private:
    QString directory_;Fault fault_;
    // Derived filename lookup only; no evidence or validation result is cached.
    mutable QString lookupIdentity_;mutable QVector<QString> lookupEventIds_;
    QJsonObject load(const DeliveryRequest&,QMap<QString,QJsonObject>* all=nullptr) const;
    QVector<DeliveryRecovery> readRecovery() const;
    QString admissionLocked(const DeliveryRequest&,bool required,bool online) const;
    void append(const DeliveryRequest&,QJsonObject,const QString&) const;
};
}
