#pragma once
#include "StationDeliveryLedger.hpp"
#include <deque>

namespace visionflow {
// Owner-thread transport state only. The ledger remains the pending authority.
class StationDeliveryScheduler final {
public:
    using Authorized=std::function<bool(const CommunicationV4Frame&)>;
    explicit StationDeliveryScheduler(StationDeliveryLedger* ledger=nullptr):ledger_(ledger){}
    static constexpr int maxFrames=64, highWaterBytes=65536, retryMs=2000, maxRetries=3;
    bool enqueue(const CommunicationV4Frame&);
    void tick(qint64 now,const QUuid& session,const Authorized&,qint64 socketBytes=0);
    QByteArrayView pendingBytes(qint64 socketBytes=0);
    void consumeWritten(qsizetype,qint64 now);
    bool canRead(qint64 socketBytes) const;
    int queuedFrames() const {return count_;}
    qsizetype queuedBytes() const {return bytes_;}
    bool timedOut(const V4RequestIdentity&) const;
    void bindSession(const QUuid& session);
    void reset();
private:
    struct Frame {QByteArray bytes;QString delivery;};
    struct Attempt {V4RequestIdentity key;int sent=0;qint64 last=0;bool queued=false,alarm=false;};
    bool add(const CommunicationV4Frame&,const QString& delivery={});
    static QString identity(const V4RequestIdentity&);
    StationDeliveryLedger* ledger_;
    QMap<quint32,std::deque<Frame>> lanes_;
    std::deque<quint32> ready_;
    std::optional<Frame> active_;
    qsizetype offset_=0,bytes_=0;int count_=0;
    QMap<QString,Attempt> attempts_;
    QUuid session_;
};
}
