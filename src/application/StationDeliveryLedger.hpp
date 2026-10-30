#pragma once
#include "StationAdmissionService.hpp"
#include "StationControlService.hpp"
#include <QJsonObject>

namespace visionflow {
// Versioned, append-only namespace in the existing bounded production archive.
// The store performs no execution, event pumping, retry scheduling or retention.
class StationDeliveryLedger final {
public:
    using Fault=std::function<void(const QString&)>;
    explicit StationDeliveryLedger(QString directory, QUuid installation, Fault={});
    struct Record {
        V4RequestIdentity key;
        StationControlRecord value;
        QString state;
        bool control=false;
    };
    std::optional<Record> inspect(const V4RequestIdentity&) const;
    QVector<Record> recovery() const;
    quint32 reserve(const StationControlIntent&);
    void seal(const V4RequestIdentity&,const CommunicationV4Frame&);
    // Untrusted wire ACK: every stable header field, result UUID and digest.
    void acknowledge(const V4RequestIdentity&,const CommunicationV4Frame& ack);
    CommunicationV4Frame replay(const V4RequestIdentity&,const QUuid& session) const;
    // Called only after PeerSessionService authorization. Compose after the
    // control handler returns nullopt; RECOVER never creates an intent.
    std::optional<CommunicationV4Frame> handle(const V4RequestIdentity&,const CommunicationV4Frame&);
    bool pending(const QUuid& station,bool control=false) const;

    class Admission final:public StationAdmissionStore {
    public:
        explicit Admission(StationDeliveryLedger& l):ledger_(l){}
        std::optional<StationAdmissionRecord> find(const V4RequestIdentity&) const override;
        void commit(const StationAdmissionIntent&) override;
    private:StationDeliveryLedger& ledger_;
    };
    class Control final:public StationControlStore {
    public:
        explicit Control(StationDeliveryLedger& l):ledger_(l){}
        std::optional<StationControlRecord> find(const V4RequestIdentity&) const override;
        quint32 reserve(const StationControlIntent& i) override{return ledger_.reserve(i);}
        void seal(const V4RequestIdentity& k,const CommunicationV4Frame& t) override{ledger_.seal(k,t);}
        // Legacy key-only calls fail closed. Production adapter requires bytes.
        void acknowledge(const V4RequestIdentity&) override;
        void acknowledge(const V4RequestIdentity& k,const CommunicationV4Frame& a) override{ledger_.acknowledge(k,a);}
    private:StationDeliveryLedger& ledger_;
    };
private:
    struct Entry {Record record;QJsonObject event;int sequence=0;};
    QMap<QString,Entry> load() const;
    QString identity(const V4RequestIdentity&) const;
    void append(Entry&,const QString& state);
    StationControlRecord view(const Entry&) const;
    void acknowledgeLocked(Entry&,const CommunicationV4Frame&);
    QString directory_;QUuid installation_;Fault fault_;
    QSet<QString> live_; // only this process may replay unresolved ACCEPTED
};
}
