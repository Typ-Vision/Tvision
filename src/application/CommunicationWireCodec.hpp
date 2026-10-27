#pragma once
#include "domain/AssignmentConfig.hpp"
#include <QByteArrayView>
#include <QVector>

namespace visionflow {
// vf.socket/3 shape validation only. Session, catalog membership, Runtime and
// durable ACK authority belong to the adapter/router, never this codec.
class CommunicationWireCodec {
public:
    static constexpr qsizetype maxLineBytes=64*1024;
    static QJsonObject decode(QByteArrayView line);
    static QByteArray encode(const QJsonObject& message); // canonical JSON + LF
    static void validate(const QJsonObject& message);
    static QJsonObject encodeField(const QString& name,const AssignmentValue&);
    static AssignmentValue decodeField(const QJsonObject&);
    // vf.canonical-json/1: Qt compact JSON, recursively sorted UTF-16 object keys,
    // array order retained, UTF-8, no LF. Excludes transport session and hash.
    static QByteArray resultPayload(const QJsonObject& result);
    static QString payloadHash(QByteArrayView persistedPayload);
    static QJsonObject sealResult(QJsonObject result);
};

// Feed bounded chunks read from the socket. No network or wall-clock dependency.
// All errors are terminal until reset (the adapter must close the peer).
class CommunicationLineFramer {
public:
    static constexpr qint64 partialDeadlineMs=2000;
    QVector<QByteArray> feed(QByteArrayView bytes,qint64 monotonicMs);
    void tick(qint64 monotonicMs);
    void reset();
    qsizetype bufferedBytes() const {return partial_.size();}
private:
    QByteArray partial_;
    qint64 started_=-1,last_=-1;
    bool failed_=false;
    void fail(const char*);
};
}
