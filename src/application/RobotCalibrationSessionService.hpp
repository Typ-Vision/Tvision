#pragma once

#include "CameraLeaseArbiter.hpp"
#include "CommunicationPeerSessionService.hpp"
#include "CommunicationV4Codec.hpp"
#include "domain/RobotCalibrationSession.hpp"

#include <QMap>
#include <QThread>

#include <memory>
#include <optional>

namespace visionflow {

// Application authority for the current dependency identities of one station
// camera. Everything here comes from the immutable local manifest/catalogue, never
// from network body text. A missing or incomplete snapshot is an explicit reject.
class CalibrationDependencyProvider {
public:
    virtual ~CalibrationDependencyProvider() = default;
    virtual std::optional<CalibrationDependencySnapshot> current(const QUuid& station,
                                                                quint32 cameraKey) const = 0;
};

// Real camera close/drain boundary. It must be idempotent and must not return
// true before the underlying worker/acquisition is actually drained. It never
// creates a second ownership registry; the lease itself stays in CameraLeaseArbiter.
class CalibrationDrainPort {
public:
    virtual ~CalibrationDrainPort() = default;
    virtual bool drain(const QUuid& session, const QString& cameraId) = 0;
};

// COMM4-20 session/lease lifecycle. Production admission shares the COMM4-10
// CameraLeaseArbiter: a calibration lease on the target camera makes a production
// reservation return CAMERA_BUSY, while an independent station on another camera
// continues. No second lease registry exists.
class RobotCalibrationSessionService {
public:
    struct Result {
        quint32 reason = 0;
        bool accepted = false;
        bool replay = false;
        QUuid session;
        CalibrationState state = CalibrationState::Created;
        CommunicationV4Frame response;
    };

    RobotCalibrationSessionService(CalibrationSessionStore&, CalibrationDependencyProvider&,
                                   CalibrationDrainPort&);
    RobotCalibrationSessionService(const RobotCalibrationSessionService&) = delete;
    RobotCalibrationSessionService& operator=(const RobotCalibrationSessionService&) = delete;
    ~RobotCalibrationSessionService();

    // Loads durable records. It never reacquires a lease, starts capture or opens
    // Run/Auto/production. Nonterminal records become recovery-required in memory.
    void restore();

    Result begin(const V4RequestIdentity&, const CommunicationV4Frame&);
    Result abort(const V4RequestIdentity&, const CommunicationV4Frame&);
    // Explicit recovery after restart/reconnect. Dependencies are rechecked against
    // the frozen snapshot before any lease is reacquired.
    Result recover(const QUuid& session);

    // Minimal COMM4-21 boundary: admission is a fence, not a capture implementation.
    std::optional<quint64> admitCapture(const QUuid& session) const;
    bool acceptCaptureCallback(const QUuid& session, quint64 generation) const;
    bool collect(const QUuid& session);
    CameraLeaseArbiter::Ticket captureLease(const QUuid& session) const;

    std::optional<CalibrationSessionRecord> session(const QUuid&) const;
    bool ownsCamera(const QString& cameraId) const;
    int activeCount() const { return active_.size(); }

private:
    struct Active {
        CalibrationSessionRecord record;
        CameraLeaseArbiter::Ticket ticket;
    };
    void owner() const;
    Result reject(const CommunicationV4Frame&, CalibrationReason, const char* detail = "") const;
    Result accepted(const CommunicationV4Frame&, const CalibrationSessionRecord&, bool replay) const;
    Result terminal(const CommunicationV4Frame&, const CalibrationSessionRecord&, bool replay) const;
    std::optional<CalibrationSessionRecord> known(const QUuid&) const;

    CalibrationSessionStore& store_;
    CalibrationDependencyProvider& dependencies_;
    CalibrationDrainPort& drain_;
    QMap<QUuid, CalibrationSessionRecord> records_;
    QMap<QUuid, Active> active_;
    quint64 nextGeneration_ = 1;
    QThread* owner_ = QThread::currentThread();
};

} // namespace visionflow
