#include "RobotCalibrationSessionService.hpp"

#include <QScopeGuard>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace visionflow {
namespace {
using C = CommunicationV4Codec;

QUuid uuid(QByteArrayView body, qsizetype offset) {
    return QUuid::fromRfc4122(body.sliced(offset, 16));
}

void putUuid(QByteArray& body, qsizetype offset, const QUuid& id) {
    body.replace(offset, 16, id.toRfc4122());
}

bool zero(QByteArrayView body, qsizetype from) {
    for (qsizetype i = from; i < body.size(); ++i)
        if (body.at(i) != 0) return false;
    return true;
}

struct BeginBody {
    QUuid session;
    HandEyeMode mode = HandEyeMode::EyeInHand;
    quint32 cameraKey = 0;
    QUuid cameraRevision;
    quint32 intrinsicRevisionKey = 0;
    quint32 plateRevisionKey = 0;
    quint32 flangeTcpRevisionKey = 0;
    quint32 robotAdapterRevisionKey = 0;
    CalibrationPoseContract pose;
};

std::optional<BeginBody> decodeBegin(const CommunicationV4Frame& frame) {
    if (frame.body.size() != 384 || !zero(frame.body, 184)) return std::nullopt;
    BeginBody body;
    body.session = uuid(frame.body, 0);
    const quint32 mode = C::uint32(frame.body, 16);
    if (mode != 1 && mode != 2) return std::nullopt;
    body.mode = mode == 1 ? HandEyeMode::EyeToHand : HandEyeMode::EyeInHand;
    body.cameraKey = C::uint32(frame.body, 20);
    body.cameraRevision = uuid(frame.body, 24);
    body.intrinsicRevisionKey = C::uint32(frame.body, 40);
    body.plateRevisionKey = C::uint32(frame.body, 44);
    body.flangeTcpRevisionKey = C::uint32(frame.body, 48);
    body.robotAdapterRevisionKey = C::uint32(frame.body, 52);
    // The BEGIN PoseBlock is a contract-only record: translation and quaternion
    // must be exactly zero, never a fabricated identity sample.
    for (qsizetype offset : {56, 64, 72, 80, 88, 96, 104}) {
        if (C::float64(frame.body, offset) != 0.0) return std::nullopt;
    }
    body.pose.targetFrame = uuid(frame.body, 112);
    body.pose.targetFrameRevision = uuid(frame.body, 128);
    body.pose.sourceFrame = uuid(frame.body, 144);
    body.pose.sourceFrameRevision = uuid(frame.body, 160);
    body.pose.convention = C::uint32(frame.body, 176);
    if (body.session.isNull() || body.cameraKey == 0 || body.cameraRevision.isNull()
        || body.intrinsicRevisionKey == 0 || body.plateRevisionKey == 0
        || body.robotAdapterRevisionKey == 0 || !body.pose.complete()) {
        return std::nullopt;
    }
    return body;
}

std::optional<QUuid> decodeAbort(const CommunicationV4Frame& frame) {
    if (frame.body.size() != 384 || !zero(frame.body, 20)) return std::nullopt;
    const QUuid session = uuid(frame.body, 0);
    if (session.isNull()) return std::nullopt;
    return session;
}

bool matchesBegin(const BeginBody& body, const CalibrationDependencySnapshot& snapshot) {
    // Wire-carried dependency keys are exact. Acquisition/model identity is not in
    // the CALIB_BEGIN body and is frozen only from the authoritative provider.
    return body.mode == snapshot.mode
        && body.cameraRevision == snapshot.cameraRevision
        && body.intrinsicRevisionKey == snapshot.intrinsicRevisionKey
        && body.plateRevisionKey == snapshot.plateRevisionKey
        && body.flangeTcpRevisionKey == snapshot.flangeTcpRevisionKey
        && body.robotAdapterRevisionKey == snapshot.robotAdapterRevisionKey
        && body.pose == snapshot.pose;
}

QString leaseOwner(const QUuid& session) {
    return QStringLiteral("calibration:") + session.toString(QUuid::WithoutBraces);
}

CommunicationV4Frame response(const CommunicationV4Frame& request, quint32 kind,
                              CalibrationReason reason, const QUuid& session,
                              quint32 cameraKey, quint32 commandState,
                              const char* detail, bool resultValid = false) {
    CommunicationV4Frame out{request.header, QByteArray(896, 0)};
    out.header.kind = kind;
    C::putUInt32(out.body, 72, resultValid ? 1 : 0);
    C::putUInt32(out.body, 76, calibrationReasonCode(reason));
    C::putUInt32(out.body, 100, commandState);
    putUuid(out.body, 104, session);
    const QByteArray text(detail);
    if (text.size() > 128) throw std::runtime_error("Calibration detail exceeds wire limit");
    C::putUInt32(out.body, 616, quint32(text.size()));
    out.body.replace(620, text.size(), text);
    C::putUInt32(out.body, 820, cameraKey);
    C::putUInt32(out.body, 824, request.header.kind);
    return out;
}
} // namespace

RobotCalibrationSessionService::RobotCalibrationSessionService(
    CalibrationSessionStore& store, CalibrationDependencyProvider& dependencies,
    CalibrationDrainPort& drain)
    : store_(store), dependencies_(dependencies), drain_(drain) {
    restore();
}

RobotCalibrationSessionService::~RobotCalibrationSessionService() = default;

void RobotCalibrationSessionService::owner() const {
    if (QThread::currentThread() != owner_) throw std::runtime_error("Calibration session owner thread required");
}

void RobotCalibrationSessionService::restore() {
    owner();
    if (!active_.isEmpty()) throw std::runtime_error("Restore requires no live calibration sessions");
    records_.clear();
    nextGeneration_ = 1;
    for (auto record : store_.loadAll()) {
        if (!calibrationStateTerminal(record.state)) record.recoveryRequired = true;
        nextGeneration_ = std::max(nextGeneration_, record.generation + 1);
        records_.insert(record.session, record);
    }
}

std::optional<CalibrationSessionRecord> RobotCalibrationSessionService::session(const QUuid& id) const {
    auto it = records_.constFind(id);
    if (it == records_.cend()) return std::nullopt;
    return *it;
}

std::optional<CalibrationSessionRecord> RobotCalibrationSessionService::known(const QUuid& id) const {
    if (auto record = session(id)) return record;
    try {
        return store_.findSession(id);
    } catch (...) {
        return std::nullopt;
    }
}

bool RobotCalibrationSessionService::ownsCamera(const QString& cameraId) const {
    for (auto it = active_.cbegin(); it != active_.cend(); ++it)
        if (it->record.dependencies.cameraId == cameraId) return true;
    return false;
}

RobotCalibrationSessionService::Result RobotCalibrationSessionService::reject(
    const CommunicationV4Frame& request, CalibrationReason reason, const char* detail) const {
    Result result;
    result.reason = calibrationReasonCode(reason);
    result.response = response(request, 190, reason, QUuid{}, 0, 0, detail);
    return result;
}

RobotCalibrationSessionService::Result RobotCalibrationSessionService::accepted(
    const CommunicationV4Frame& request, const CalibrationSessionRecord& record, bool replay) const {
    Result result;
    result.reason = calibrationReasonCode(CalibrationReason::None);
    result.accepted = true;
    result.replay = replay;
    result.session = record.session;
    result.state = record.state;
    result.response = response(request, 110, CalibrationReason::None, record.session,
                               record.dependencies.cameraKey, 1, "", false);
    return result;
}

RobotCalibrationSessionService::Result RobotCalibrationSessionService::terminal(
    const CommunicationV4Frame& request, const CalibrationSessionRecord& record, bool replay) const {
    Result result;
    result.reason = calibrationReasonCode(CalibrationReason::None);
    result.accepted = true;
    result.replay = replay;
    result.session = record.session;
    result.state = record.state;
    result.response = response(request, 111, CalibrationReason::None, record.session,
                               record.dependencies.cameraKey, 11, "", false);
    return result;
}

RobotCalibrationSessionService::Result RobotCalibrationSessionService::begin(
    const V4RequestIdentity& key, const CommunicationV4Frame& request) {
    owner();
    if (request.header.kind != 20 || key.installation.isNull() || key.station.isNull()
        || request.header.stationKey == 0 || key.peer != request.header.peer
        || key.epoch != request.header.epoch || key.request != request.header.requestId) {
        return reject(request, CalibrationReason::InvalidValue, "INVALID_REQUEST_IDENTITY");
    }
    const auto body = decodeBegin(request);
    if (!body) return reject(request, CalibrationReason::InvalidValue, "INVALID_CALIB_BEGIN");
    QByteArray encoded;
    try {
        encoded = C::encode(request, V4Direction::Request);
    } catch (...) {
        return reject(request, CalibrationReason::BadFrame, "BAD_FRAME");
    }
    const QByteArray fingerprint = C::requestFingerprint(encoded);
    const CalibrationCommandKey commandKey{key.installation, key.peer, key.epoch, key.station, key.request};

    // Durable dedup precedes all mutable dependency/lease checks. Reconnect changes
    // only the header session envelope; it never re-executes BEGIN or adds a lease.
    try {
        if (const auto command = store_.findCommand(commandKey)) {
            if (command->fingerprint != fingerprint || command->kind != CalibrationCommandKind::Begin)
                return reject(request, CalibrationReason::Conflict, "REQUEST_CONFLICT");
            const auto prior = known(command->session);
            if (!prior || prior->beginCommand != command->key)
                return reject(request, CalibrationReason::RecordingError, "MISSING_SESSION_RECORD");
            if (prior->session != body->session)
                return reject(request, CalibrationReason::Conflict, "SESSION_CONFLICT");
            if (prior->state == CalibrationState::Aborted) return terminal(request, *prior, true);
            if (!active_.contains(prior->session) || prior->recoveryRequired)
                return reject(request, CalibrationReason::RecoveryRequired, "RECOVERY_REQUIRED");
            return accepted(request, *prior, true);
        }
    } catch (...) {
        return reject(request, CalibrationReason::RecordingError, "PENDING_STORAGE");
    }

    if (known(body->session))
        return reject(request, CalibrationReason::Conflict, "SESSION_EXISTS");

    std::optional<CalibrationDependencySnapshot> current;
    try {
        current = dependencies_.current(key.station, body->cameraKey);
    } catch (...) {
        return reject(request, CalibrationReason::NotReady, "DEPENDENCY_UNAVAILABLE");
    }
    if (!current || !current->complete() || current->cameraKey != body->cameraKey)
        return reject(request, CalibrationReason::StaleConfig, "STALE_DEPENDENCY");
    // Stale/mismatched frame/revision, intrinsic, plate, adapter, TCP or mode is an
    // explicit reject before any camera ownership is reserved.
    if (!matchesBegin(*body, *current))
        return reject(request, CalibrationReason::StaleConfig, "STALE_DEPENDENCY");

    CameraLeaseArbiter::Reservation reservation;
    try {
        reservation = CameraLeaseArbiter::reserve(leaseOwner(body->session), CameraLeaseUse::Calibration,
                                                  {CameraLeaseTarget{current->cameraId, current->physical}});
    } catch (...) {
        return reject(request, CalibrationReason::NotReady, "CAMERA_LEASE_UNAVAILABLE");
    }
    if (reservation.reason)
        return reject(request, reservation.reason == 3 ? CalibrationReason::CameraBusy
                                                       : CalibrationReason::ResourceLimit,
                      "CAMERA_LEASE_BUSY");
    if (reservation.leases.size() != 1)
        return reject(request, CalibrationReason::ResourceLimit, "CAMERA_LEASE_COUNT");
    auto ticket = reservation.leases.takeFirst();

    CalibrationSessionRecord record;
    record.session = body->session;
    record.beginCommand = commandKey;
    record.fingerprint = fingerprint;
    record.state = CalibrationState::Created;
    record.generation = nextGeneration_++;
    record.dependencies = *current;
    try {
        store_.commit(record, CalibrationCommandRecord{commandKey, fingerprint, record.session,
                                                       CalibrationCommandKind::Begin,
                                                       CalibrationState::Created, true});
    } catch (...) {
        // No session or command is durable: release the just-reserved lease so the
        // target camera is not orphaned and production admission reopens.
        ticket.reset();
        return reject(request, CalibrationReason::RecordingError, "PENDING_STORAGE");
    }
    records_.insert(record.session, record);
    active_.insert(record.session, Active{record, std::move(ticket)});
    return accepted(request, record, false);
}

RobotCalibrationSessionService::Result RobotCalibrationSessionService::abort(
    const V4RequestIdentity& key, const CommunicationV4Frame& request) {
    owner();
    if (request.header.kind != 25 || key.installation.isNull() || key.station.isNull()
        || request.header.stationKey == 0 || key.peer != request.header.peer
        || key.epoch != request.header.epoch || key.request != request.header.requestId) {
        return reject(request, CalibrationReason::InvalidValue, "INVALID_REQUEST_IDENTITY");
    }
    const auto sessionId = decodeAbort(request);
    if (!sessionId) return reject(request, CalibrationReason::InvalidValue, "INVALID_CALIB_ABORT");
    QByteArray encoded;
    try {
        encoded = C::encode(request, V4Direction::Request);
    } catch (...) {
        return reject(request, CalibrationReason::BadFrame, "BAD_FRAME");
    }
    const QByteArray fingerprint = C::requestFingerprint(encoded);
    const CalibrationCommandKey commandKey{key.installation, key.peer, key.epoch, key.station, key.request};

    try {
        if (const auto command = store_.findCommand(commandKey)) {
            if (command->fingerprint != fingerprint || command->kind != CalibrationCommandKind::Abort)
                return reject(request, CalibrationReason::Conflict, "REQUEST_CONFLICT");
            const auto prior = known(command->session);
            if (!prior) return reject(request, CalibrationReason::RecordingError, "MISSING_SESSION_RECORD");
            if (prior->state != CalibrationState::Aborted)
                return reject(request, CalibrationReason::RecoveryRequired, "ABORT_NOT_COMPLETE");
            // Duplicate ABORT never drains again and never touches another session's lease.
            return terminal(request, *prior, true);
        }
    } catch (...) {
        return reject(request, CalibrationReason::RecordingError, "PENDING_STORAGE");
    }

    const auto prior = known(*sessionId);
    if (!prior) return reject(request, CalibrationReason::UnknownKey, "UNKNOWN_SESSION");
    if (prior->beginCommand.station != key.station)
        return reject(request, CalibrationReason::ContractMismatch, "SESSION_STATION_MISMATCH");

    const CalibrationCommandRecord commandRecord{commandKey, fingerprint, prior->session,
                                                 CalibrationCommandKind::Abort,
                                                 CalibrationState::Aborted, true};

    if (prior->state == CalibrationState::Aborted) {
        try {
            store_.commit(*prior, commandRecord);
        } catch (...) {
            return reject(request, CalibrationReason::RecordingError, "PENDING_STORAGE");
        }
        records_.insert(prior->session, *prior);
        return terminal(request, *prior, false);
    }

    auto active = active_.find(prior->session);
    if (active == active_.end()) {
        // After restart there is no live ownership to drain. Explicit ABORT is still
        // the only way this record becomes terminal; no capture/lease is restarted.
        auto finished = *prior;
        finished.state = CalibrationState::Aborted;
        finished.abortRequested = false;
        finished.recoveryRequired = false;
        try {
            store_.commit(finished, commandRecord);
        } catch (...) {
            return reject(request, CalibrationReason::RecordingError, "PENDING_STORAGE");
        }
        records_.insert(finished.session, finished);
        return terminal(request, finished, false);
    }

    // First durable write closes new work before any drain starts. A crash from
    // here on restores abortRequested and cannot silently resume collection.
    if (!active->record.abortRequested) {
        auto closing = active->record;
        closing.abortRequested = true;
        try {
            store_.commit(closing);
        } catch (...) {
            return reject(request, CalibrationReason::RecordingError, "PENDING_STORAGE");
        }
        active->record = closing;
        records_.insert(closing.session, closing);
    }

    bool clean = false;
    try {
        clean = drain_.drain(active->record.session, active->record.dependencies.cameraId);
    } catch (...) {
        clean = false;
    }
    if (!clean) {
        // Drain failure/timeout is never completion. Ownership is retained and the
        // session stays nonterminal until a later explicit ABORT drains for real.
        auto pending = active->record;
        pending.recoveryRequired = true;
        try {
            store_.commit(pending);
        } catch (...) {
            // abortRequested is already durable and the lease is still held.
        }
        active->record = pending;
        records_.insert(pending.session, pending);
        return reject(request, CalibrationReason::RecoveryRequired, "DRAIN_INCOMPLETE");
    }

    auto finished = active->record;
    finished.state = CalibrationState::Aborted;
    finished.abortRequested = false;
    finished.recoveryRequired = false;
    try {
        // Keep the ticket until the terminal state is durable. If the write fails,
        // the lease is not released and ABORT is not reported as completed.
        store_.commit(finished, commandRecord);
    } catch (...) {
        return reject(request, CalibrationReason::RecordingError, "PENDING_STORAGE");
    }
    auto ticket = std::move(active->ticket);
    active_.remove(prior->session);
    records_.insert(finished.session, finished);
    ticket.reset();
    return terminal(request, finished, false);
}

RobotCalibrationSessionService::Result RobotCalibrationSessionService::recover(const QUuid& sessionId) {
    owner();
    Result result;
    result.session = sessionId;
    auto record = records_.find(sessionId);
    if (record == records_.end()) {
        result.reason = calibrationReasonCode(CalibrationReason::UnknownKey);
        return result;
    }
    result.state = record->state;
    if (calibrationStateTerminal(record->state)) {
        result.accepted = true;
        result.replay = true;
        return result;
    }
    if (record->abortRequested) {
        result.reason = calibrationReasonCode(CalibrationReason::RecoveryRequired);
        return result;
    }
    if (active_.contains(sessionId)) {
        result.accepted = true;
        result.replay = true;
        return result;
    }

    std::optional<CalibrationDependencySnapshot> current;
    try {
        current = dependencies_.current(record->beginCommand.station, record->dependencies.cameraKey);
    } catch (...) {
        result.reason = calibrationReasonCode(CalibrationReason::NotReady);
        return result;
    }
    // Explicit recovery rechecks the complete frozen snapshot, including
    // acquisition/model revisions that are not in the wire BEGIN body.
    if (!current || *current != record->dependencies) {
        result.reason = calibrationReasonCode(CalibrationReason::StaleConfig);
        return result;
    }

    CameraLeaseArbiter::Reservation reservation;
    try {
        reservation = CameraLeaseArbiter::reserve(leaseOwner(sessionId), CameraLeaseUse::Calibration,
                                                  {CameraLeaseTarget{current->cameraId, current->physical}});
    } catch (...) {
        result.reason = calibrationReasonCode(CalibrationReason::NotReady);
        return result;
    }
    if (reservation.reason) {
        result.reason = calibrationReasonCode(reservation.reason == 3 ? CalibrationReason::CameraBusy
                                                                     : CalibrationReason::ResourceLimit);
        return result;
    }
    if (reservation.leases.size() != 1) {
        result.reason = calibrationReasonCode(CalibrationReason::ResourceLimit);
        return result;
    }
    auto ticket = reservation.leases.takeFirst();
    auto recovered = *record;
    recovered.recoveryRequired = false;
    try {
        store_.commit(recovered);
    } catch (...) {
        ticket.reset();
        result.reason = calibrationReasonCode(CalibrationReason::RecordingError);
        return result;
    }
    records_.insert(sessionId, recovered);
    active_.insert(sessionId, Active{recovered, std::move(ticket)});
    result.reason = calibrationReasonCode(CalibrationReason::None);
    result.accepted = true;
    result.state = recovered.state;
    return result;
}

std::optional<quint64> RobotCalibrationSessionService::admitCapture(const QUuid& sessionId) const {
    auto it = active_.constFind(sessionId);
    if (it == active_.cend()) return std::nullopt;
    if (it->record.state != CalibrationState::Created || it->record.recoveryRequired
        || it->record.abortRequested) {
        return std::nullopt;
    }
    return it->record.generation;
}

bool RobotCalibrationSessionService::acceptCaptureCallback(const QUuid& sessionId, quint64 generation) const {
    auto it = active_.constFind(sessionId);
    if (it == active_.cend()) return false;
    return it->record.generation == generation && it->record.state == CalibrationState::Created
        && !it->record.recoveryRequired && !it->record.abortRequested;
}

} // namespace visionflow
