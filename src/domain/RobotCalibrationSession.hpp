#pragma once

#include "HandEyeReadiness.hpp"
#include "MeasurementSetupContract.hpp"

#include <QByteArray>
#include <QString>
#include <QUuid>

#include <optional>
#include <vector>

namespace visionflow {

// COMM4-20 lifecycle subset of contract S8. Numeric values are frozen by W7
// CalibrationState; they are never derived from display text. SOLVING..ACTIVE are
// declared by the contract but are only produced by later cards (22/23).
enum class CalibrationState : quint32 {
    Created = 0,
    Collecting = 1,
    Solving = 2,
    Candidate = 3,
    Validating = 4,
    Validated = 5,
    Active = 6,
    Failed = 7,
    Aborted = 8
};

inline QString calibrationStateName(CalibrationState state) {
    switch (state) {
    case CalibrationState::Created: return QStringLiteral("CREATED");
    case CalibrationState::Collecting: return QStringLiteral("COLLECTING");
    case CalibrationState::Solving: return QStringLiteral("SOLVING");
    case CalibrationState::Candidate: return QStringLiteral("CANDIDATE");
    case CalibrationState::Validating: return QStringLiteral("VALIDATING");
    case CalibrationState::Validated: return QStringLiteral("VALIDATED");
    case CalibrationState::Active: return QStringLiteral("ACTIVE");
    case CalibrationState::Failed: return QStringLiteral("FAILED");
    case CalibrationState::Aborted: return QStringLiteral("ABORTED");
    }
    return QStringLiteral("UNKNOWN");
}

inline bool calibrationStateTerminal(CalibrationState state) {
    return state == CalibrationState::Failed || state == CalibrationState::Aborted;
}

// Frozen W7 reason enum. Only the lifecycle-relevant values are produced here;
// the complete set is kept so no display-text-to-number mapping is invented.
enum class CalibrationReason : quint32 {
    None = 0,
    NotAuto = 1,
    StationBusy = 2,
    CameraBusy = 3,
    StaleConfig = 4,
    StaleSession = 5,
    Conflict = 6,
    QueueFull = 7,
    RecordingError = 8,
    RecoveryRequired = 9,
    UnknownKey = 10,
    InvalidValue = 11,
    UnsupportedVersion = 12,
    BadFrame = 13,
    AckMismatch = 14,
    AckTimeout = 15,
    LeaseMismatch = 16,
    PhysicalGateBlocked = 17,
    PeerBusy = 18,
    RetiredEpoch = 19,
    NotReady = 20,
    CaptureTimeout = 21,
    ResultInvalid = 22,
    ResourceLimit = 23,
    ContractMismatch = 24,
    RemoteDisabled = 25
};

inline quint32 calibrationReasonCode(CalibrationReason reason) {
    return static_cast<quint32>(reason);
}

inline QString calibrationReasonName(CalibrationReason reason) {
    switch (reason) {
    case CalibrationReason::None: return QStringLiteral("NONE");
    case CalibrationReason::CameraBusy: return QStringLiteral("CAMERA_BUSY");
    case CalibrationReason::StaleConfig: return QStringLiteral("STALE_CONFIG");
    case CalibrationReason::StaleSession: return QStringLiteral("STALE_SESSION");
    case CalibrationReason::Conflict: return QStringLiteral("CONFLICT");
    case CalibrationReason::RecordingError: return QStringLiteral("RECORDING_ERROR");
    case CalibrationReason::RecoveryRequired: return QStringLiteral("RECOVERY_REQUIRED");
    case CalibrationReason::UnknownKey: return QStringLiteral("UNKNOWN_KEY");
    case CalibrationReason::InvalidValue: return QStringLiteral("INVALID_VALUE");
    case CalibrationReason::NotReady: return QStringLiteral("NOT_READY");
    case CalibrationReason::CaptureTimeout: return QStringLiteral("CAPTURE_TIMEOUT");
    case CalibrationReason::ContractMismatch: return QStringLiteral("CONTRACT_MISMATCH");
    default: return QStringLiteral("REASON_") + QString::number(calibrationReasonCode(reason));
    }
}

// Persistent request namespace. The transport connection session is deliberately
// absent: reconnect must not create a new persistent session or lease.
struct CalibrationCommandKey {
    QUuid installation;
    QUuid peer;
    QUuid epoch;
    QUuid station;
    quint64 request = 0;
    bool operator==(const CalibrationCommandKey&) const = default;
};

enum class CalibrationCommandKind : quint32 { Begin = 20, Abort = 25 };

// W4 PoseBlock contract part of CALIB_BEGIN. Translation/quaternion are zero on
// the wire; only the exact frame/revision binding and contract-only convention
// are authority. It is never a fake identity pose.
struct CalibrationPoseContract {
    QUuid targetFrame;
    QUuid targetFrameRevision;
    QUuid sourceFrame;
    QUuid sourceFrameRevision;
    quint32 convention = 0;
    bool complete() const {
        return !targetFrame.isNull() && !targetFrameRevision.isNull()
            && !sourceFrame.isNull() && !sourceFrameRevision.isNull() && convention == 2;
    }
    bool operator==(const CalibrationPoseContract&) const = default;
};

// Immutable dependency snapshot frozen at BEGIN. Wire-supplied keys are checked
// against the authoritative provider; acquisition/model identities are only
// taken from that provider (they are not present in the CALIB_BEGIN body).
struct CalibrationDependencySnapshot {
    HandEyeMode mode = HandEyeMode::EyeInHand;
    quint32 cameraKey = 0;
    QUuid cameraRevision;
    quint32 intrinsicRevisionKey = 0;
    QUuid intrinsicRevision;
    quint32 plateRevisionKey = 0;
    QUuid plateRevision;
    quint32 flangeTcpRevisionKey = 0;
    QUuid flangeTcpRevision;
    quint32 robotAdapterRevisionKey = 0;
    QUuid robotAdapterRevision;
    quint32 acquisitionProfileRevisionKey = 0;
    QUuid acquisitionProfileRevision;
    quint32 modelRevisionKey = 0;
    QUuid modelRevision;
    CalibrationPoseContract pose;
    QString cameraId;
    PhysicalDeviceIdentity physical;
    bool complete() const {
        const bool tcp = (flangeTcpRevisionKey == 0 && flangeTcpRevision.isNull())
            || (flangeTcpRevisionKey != 0 && !flangeTcpRevision.isNull());
        return cameraKey != 0 && !cameraRevision.isNull()
            && intrinsicRevisionKey != 0 && !intrinsicRevision.isNull()
            && plateRevisionKey != 0 && !plateRevision.isNull() && tcp
            && robotAdapterRevisionKey != 0 && !robotAdapterRevision.isNull()
            && acquisitionProfileRevisionKey != 0 && !acquisitionProfileRevision.isNull()
            && modelRevisionKey != 0 && !modelRevision.isNull()
            && pose.complete() && !cameraId.isEmpty() && physical.usable()
            && !physical.vendor.isEmpty() && !physical.model.isEmpty() && !physical.serial.isEmpty();
    }
    bool operator==(const CalibrationDependencySnapshot&) const = default;
};

// One durable calibration session. recoveryRequired is a process/restart fact:
// a restored nonterminal session has no live lease and cannot capture until an
// explicit recovery revalidates dependencies and reacquires ownership.
struct CalibrationSessionRecord {
    QUuid session;
    CalibrationCommandKey beginCommand;
    QByteArray fingerprint;
    QByteArray beginRequest; // exact frozen BEGIN header/body; absent legacy records cannot SAMPLE
    CalibrationState state = CalibrationState::Created;
    bool recoveryRequired = false;
    bool abortRequested = false;
    quint64 generation = 0;
    CalibrationDependencySnapshot dependencies;
    bool operator==(const CalibrationSessionRecord&) const = default;
};

// Durable command dedup record. It never carries capture results or solve output.
struct CalibrationCommandRecord {
    CalibrationCommandKey key;
    QByteArray fingerprint;
    QUuid session;
    CalibrationCommandKind kind = CalibrationCommandKind::Begin;
    CalibrationState resultState = CalibrationState::Created;
    bool accepted = false;
    bool operator==(const CalibrationCommandRecord&) const = default;
};

// Durable store port. commit must atomically persist the session update and the
// optional command dedup record; a throw means neither is known to be durable.
class CalibrationSessionStore {
public:
    virtual ~CalibrationSessionStore() = default;
    virtual std::optional<CalibrationCommandRecord> findCommand(const CalibrationCommandKey&) const = 0;
    virtual std::optional<CalibrationSessionRecord> findSession(const QUuid& session) const = 0;
    virtual std::vector<CalibrationSessionRecord> loadAll() const = 0;
    virtual void commit(const CalibrationSessionRecord& session,
                        const std::optional<CalibrationCommandRecord>& command = std::nullopt) = 0;
};

} // namespace visionflow
