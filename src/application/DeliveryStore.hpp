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
    DeliveryStatus request(const DeliveryRequest&) const;
    void begin(const DeliveryRequest&,const QString& cycleId) const;
    void commit(const DeliveryRequest&,const QJsonObject& terminal,const QByteArray& payload) const;
    void attempt(const DeliveryRequest&) const;
    void acknowledge(const DeliveryRequest&,const QString& payloadHash) const;
    void commitAcknowledgement(const DeliveryRequest&) const;
    // Caller supplies a transport; callback never receives uncommitted bytes.
    bool dispatch(const DeliveryRequest&,const std::function<void(const QByteArray&)>&) const;
private:
    QString directory_;Fault fault_;
    QJsonObject load(const DeliveryRequest&) const;
    void append(const DeliveryRequest&,QJsonObject,const QString&) const;
};
}
