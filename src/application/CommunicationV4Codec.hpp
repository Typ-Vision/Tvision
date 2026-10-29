#pragma once
#include <QByteArray>
#include <QByteArrayView>
#include <QUuid>
#include <QVector>
#include <deque>

namespace visionflow {
enum class V4Direction { Request, Response };
struct CommunicationV4Header {
    quint32 kind=0, stationKey=0;
    quint64 requestId=0;
    QUuid session, peer, epoch;
    quint32 recipeRevisionKey=0, templateRevisionKey=0;
    QUuid activation;
};
// Fixed wire body, with checked BE accessors. No JSON/double conversion for IDs.
// Catalogue membership, session ownership, ACK matching and admission are NOT
// codec responsibilities. A syntactically valid frame confers no authority.
struct CommunicationV4Frame {
    CommunicationV4Header header;
    QByteArray body;
};
class CommunicationV4Codec {
public:
    static constexpr qsizetype requestBytes=512, responseBytes=1024, headerBytes=128;
    static qsizetype frameBytes(V4Direction);
    static CommunicationV4Frame decode(QByteArrayView, V4Direction);
    static QByteArray encode(const CommunicationV4Frame&, V4Direction);
    // Validates the first 128 bytes without allocating from untrusted size.
    static void validateHeader(QByteArrayView, V4Direction);
    static quint32 uint32(QByteArrayView, qsizetype offset);
    static qint32 int32(QByteArrayView, qsizetype offset);
    static quint64 uint64(QByteArrayView, qsizetype offset);
    static double float64(QByteArrayView, qsizetype offset);
    static void putUInt32(QByteArray&, qsizetype, quint32);
    static void putInt32(QByteArray&, qsizetype, qint32);
    static void putUInt64(QByteArray&, qsizetype, quint64);
    static void putFloat64(QByteArray&, qsizetype, double);
    static QByteArray requestFingerprint(QByteArrayView);
    static QByteArray terminalDigest(QByteArrayView);
};

// Caller reads <=2048 bytes, calls tick even without traffic, and schedules a
// continuation for buffered full frames. Fatal errors require close/reset.
class CommunicationV4Framer {
public:
    static constexpr qsizetype maxReadBytes=2048, maxBufferBytes=4096, maxBatch=4;
    static constexpr qint64 partialDeadlineMs=2000;
    explicit CommunicationV4Framer(V4Direction direction=V4Direction::Request):direction_(direction){}
    QVector<CommunicationV4Frame> feed(QByteArrayView, qint64 monotonicMs);
    void tick(qint64 monotonicMs);
    void reset();
    qsizetype bufferedBytes() const {return buffer_.size();}
private:
    V4Direction direction_;
    QByteArray buffer_;
    // Arrival time belongs to each byte, including an incomplete frame behind
    // queued complete frames. Trickle and dispatch scheduling cannot renew it.
    QVector<qint64> arrival_;
    qint64 last_=-1;
    bool failed_=false;
    [[noreturn]] void fail(const char*);
};

// One transport owner uses pendingBytes -> socket.write -> consumeWritten.
// A partial frame remains first; queue saturation is explicit backpressure.
class CommunicationV4WriteQueue {
public:
    static constexpr qsizetype maxFrames=64, highWaterBytes=65536;
    bool enqueue(const CommunicationV4Frame&);
    QByteArrayView pendingBytes(qsizetype socketBytesToWrite=0) const;
    void consumeWritten(qsizetype);
    qsizetype queuedBytes() const {return queuedBytes_;}
    qsizetype queuedFrames() const {return qsizetype(frames_.size());}
    bool backpressured(qsizetype socketBytesToWrite=0) const;
    void reset();
private:
    std::deque<QByteArray> frames_;
    qsizetype offset_=0, queuedBytes_=0;
};
}
