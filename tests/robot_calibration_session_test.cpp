#include "application/RobotCalibrationSessionService.hpp"
#include "persistence/RobotCalibrationSessionStore.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <vector>

using namespace visionflow;
using C = CommunicationV4Codec;

namespace {
int checks = 0;
quint64 nextRequest = 1;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

QUuid kInstallation(QStringLiteral("11111111-1111-4111-8111-111111111111"));
QUuid kPeer(QStringLiteral("22222222-2222-4222-8222-222222222222"));
QUuid kEpoch(QStringLiteral("33333333-3333-4333-8333-333333333333"));
QUuid kStation(QStringLiteral("44444444-4444-4444-8444-444444444444"));
QUuid kConnection(QStringLiteral("55555555-5555-4555-8555-555555555555"));
QUuid kOtherStation(QStringLiteral("66666666-6666-4666-8666-666666666666"));

struct Provider final : CalibrationDependencyProvider {
    struct Entry {
        QUuid station;
        quint32 cameraKey = 0;
        CalibrationDependencySnapshot snapshot;
    };
    std::vector<Entry> entries;
    bool fail = false;
    std::optional<CalibrationDependencySnapshot> current(const QUuid& station, quint32 cameraKey) const override {
        if (fail) throw std::runtime_error("synthetic dependency provider failure");
        for (const auto& entry : entries)
            if (entry.station == station && entry.cameraKey == cameraKey) return entry.snapshot;
        return std::nullopt;
    }
    void set(const QUuid& station, const CalibrationDependencySnapshot& snapshot) {
        entries.push_back({station, snapshot.cameraKey, snapshot});
    }
};

struct Drain final : CalibrationDrainPort {
    int calls = 0;
    bool clean = true;
    std::function<void()> during;
    bool drain(const QUuid&, const QString&) override {
        ++calls;
        if (during) during();
        return clean;
    }
};

struct MemoryStore final : CalibrationSessionStore {
    std::vector<CalibrationSessionRecord> sessions;
    std::vector<CalibrationCommandRecord> commands;
    int commitCalls = 0;
    int failAtCommit = -1;

    std::optional<CalibrationCommandRecord> findCommand(const CalibrationCommandKey& key) const override {
        for (const auto& record : commands)
            if (record.key == key) return record;
        return std::nullopt;
    }
    std::optional<CalibrationSessionRecord> findSession(const QUuid& id) const override {
        for (const auto& record : sessions)
            if (record.session == id) return record;
        return std::nullopt;
    }
    std::vector<CalibrationSessionRecord> loadAll() const override { return sessions; }
    void commit(const CalibrationSessionRecord& record,
                const std::optional<CalibrationCommandRecord>& command = std::nullopt) override {
        ++commitCalls;
        if (failAtCommit == commitCalls) {
            failAtCommit = -1;
            throw std::runtime_error("synthetic persistence failure");
        }
        bool replaced = false;
        for (auto& existing : sessions)
            if (existing.session == record.session) { existing = record; replaced = true; break; }
        if (!replaced) sessions.push_back(record);
        if (command) {
            bool found = false;
            for (auto& existing : commands)
                if (existing.key == command->key) { existing = *command; found = true; break; }
            if (!found) commands.push_back(*command);
        }
    }
    void failNext() { failAtCommit = commitCalls + 1; }
    void failAfter(int commits) { failAtCommit = commitCalls + commits; }
};

CalibrationDependencySnapshot dependency(const QString& tag, quint32 cameraKey,
                                         HandEyeMode mode = HandEyeMode::EyeInHand) {
    auto uuid = [] { return QUuid::createUuid(); };
    CalibrationDependencySnapshot snapshot;
    snapshot.mode = mode;
    snapshot.cameraKey = cameraKey;
    snapshot.cameraRevision = uuid();
    snapshot.intrinsicRevisionKey = 100 + cameraKey;
    snapshot.intrinsicRevision = uuid();
    snapshot.plateRevisionKey = 200 + cameraKey;
    snapshot.plateRevision = uuid();
    snapshot.flangeTcpRevisionKey = 0;
    snapshot.flangeTcpRevision = QUuid{};
    snapshot.robotAdapterRevisionKey = 300 + cameraKey;
    snapshot.robotAdapterRevision = uuid();
    snapshot.acquisitionProfileRevisionKey = 400 + cameraKey;
    snapshot.acquisitionProfileRevision = uuid();
    snapshot.modelRevisionKey = 500 + cameraKey;
    snapshot.modelRevision = uuid();
    snapshot.pose = CalibrationPoseContract{uuid(), uuid(), uuid(), uuid(), 2};
    snapshot.cameraId = QStringLiteral("camera-") + tag;
    snapshot.physical = {QStringLiteral("VF"), QStringLiteral("CAL"), tag,
                         DeviceIdentityConfidence::SerialReported};
    check(snapshot.complete(), "fixture dependency snapshot is complete");
    return snapshot;
}

struct Wire {
    QUuid installation = kInstallation;
    QUuid peer = kPeer;
    QUuid epoch = kEpoch;
    QUuid station = kStation;
    QUuid connection = kConnection;
    quint64 request = 0;
    quint32 stationKey = 1;
    QUuid session;
    HandEyeMode mode = HandEyeMode::EyeInHand;
    quint32 cameraKey = 0;
    QUuid cameraRevision;
    quint32 intrinsic = 0;
    quint32 plate = 0;
    quint32 flangeTcp = 0;
    quint32 adapter = 0;
    CalibrationPoseContract pose;
};

Wire wireFor(const CalibrationDependencySnapshot& snapshot) {
    Wire wire;
    wire.request = nextRequest++;
    wire.session = QUuid::createUuid();
    wire.mode = snapshot.mode;
    wire.cameraKey = snapshot.cameraKey;
    wire.cameraRevision = snapshot.cameraRevision;
    wire.intrinsic = snapshot.intrinsicRevisionKey;
    wire.plate = snapshot.plateRevisionKey;
    wire.flangeTcp = snapshot.flangeTcpRevisionKey;
    wire.adapter = snapshot.robotAdapterRevisionKey;
    wire.pose = snapshot.pose;
    return wire;
}

V4RequestIdentity identity(const Wire& wire) {
    return {wire.installation, wire.peer, wire.epoch, wire.station, wire.request};
}

V4RequestIdentity identity(const CommunicationV4Frame& frame) {
    return {kInstallation, frame.header.peer, frame.header.epoch, kStation, frame.header.requestId};
}

V4RequestIdentity abortIdentity(const CommunicationV4Frame& frame, const QUuid& station) {
    return {kInstallation, frame.header.peer, frame.header.epoch, station, frame.header.requestId};
}

CommunicationV4Frame beginFrame(const Wire& wire) {
    CommunicationV4Frame frame;
    frame.header.kind = 20;
    frame.header.stationKey = wire.stationKey;
    frame.header.requestId = wire.request;
    frame.header.session = wire.connection;
    frame.header.peer = wire.peer;
    frame.header.epoch = wire.epoch;
    frame.body = QByteArray(384, 0);
    auto& body = frame.body;
    body.replace(0, 16, wire.session.toRfc4122());
    C::putUInt32(body, 16, wire.mode == HandEyeMode::EyeToHand ? 1 : 2);
    C::putUInt32(body, 20, wire.cameraKey);
    body.replace(24, 16, wire.cameraRevision.toRfc4122());
    C::putUInt32(body, 40, wire.intrinsic);
    C::putUInt32(body, 44, wire.plate);
    C::putUInt32(body, 48, wire.flangeTcp);
    C::putUInt32(body, 52, wire.adapter);
    body.replace(112, 16, wire.pose.targetFrame.toRfc4122());
    body.replace(128, 16, wire.pose.targetFrameRevision.toRfc4122());
    body.replace(144, 16, wire.pose.sourceFrame.toRfc4122());
    body.replace(160, 16, wire.pose.sourceFrameRevision.toRfc4122());
    C::putUInt32(body, 176, wire.pose.convention);
    return frame;
}

CommunicationV4Frame abortFrame(const Wire& source, quint64 request, const QUuid& session,
                                const QUuid& station = kStation) {
    Wire wire = source;
    wire.request = request;
    wire.station = station;
    CommunicationV4Frame frame;
    frame.header.kind = 25;
    frame.header.stationKey = 1;
    frame.header.requestId = request;
    frame.header.session = source.connection;
    frame.header.peer = source.peer;
    frame.header.epoch = source.epoch;
    frame.body = QByteArray(384, 0);
    frame.body.replace(0, 16, session.toRfc4122());
    C::putUInt32(frame.body, 16, 1);
    return frame;
}

quint32 productionReserve(const CalibrationDependencySnapshot& snapshot, const QString& owner) {
    auto reservation = CameraLeaseArbiter::reserve(owner, CameraLeaseUse::Runtime,
                                                   {CameraLeaseTarget{snapshot.cameraId, snapshot.physical}});
    return reservation.reason;
}

int activeLeaseDiagnostics(const QUuid& session) {
    const QString owner = QStringLiteral("calibration:") + session.toString(QUuid::WithoutBraces);
    const auto diagnostics = CameraLeaseArbiter::diagnostics();
    return int(std::count_if(diagnostics.begin(), diagnostics.end(),
                             [&](const CameraLeaseDiagnostic& diagnostic) {
                                 return diagnostic.owner == owner;
                             }));
}

void acceptBegin(const RobotCalibrationSessionService::Result& result, const Wire& wire) {
    check(result.accepted && result.reason == 0, "calibration BEGIN accepted");
    check(result.session == wire.session, "calibration BEGIN returns the requested session");
    check(result.state == CalibrationState::Created, "calibration BEGIN creates CREATED");
    check(result.response.header.kind == 110, "calibration BEGIN response is ACCEPTED");
    check(C::uint32(result.response.body, 76) == 0, "ACCEPTED reason is zero");
    check(C::uint32(result.response.body, 100) == 1, "ACCEPTED commandState is one");
    check(QUuid::fromRfc4122(result.response.body.sliced(104, 16)) == wire.session,
          "response carries the calibration session UUID");
    check(C::uint32(result.response.body, 820) == wire.cameraKey, "response carries cameraKey");
    check(C::uint32(result.response.body, 824) == 20, "response carries originating kind");
}

void validation() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto dep = dependency(QStringLiteral("validation"), 10);
    provider.set(kStation, dep);
    RobotCalibrationSessionService service(store, provider, drain);

    Wire wire = wireFor(dep);
    auto frame = beginFrame(wire);
    frame.header.kind = 25;
    auto result = service.begin(identity(wire), frame);
    check(!result.accepted && result.reason == 11, "wrong command kind is INVALID_VALUE");
    check(productionReserve(dep, QStringLiteral("probe-validation-1")) == 0, "invalid BEGIN reserves no camera");

    wire = wireFor(dep);
    auto badSession = beginFrame(wire);
    badSession.body.fill(0, 16);
    result = service.begin(identity(wire), badSession);
    check(result.reason == 11, "missing calibration session UUID is rejected");

    wire = wireFor(dep);
    auto badMode = beginFrame(wire);
    C::putUInt32(badMode.body, 16, 3);
    check(service.begin(identity(wire), badMode).reason == 11, "unknown calibration mode is rejected");

    wire = wireFor(dep);
    auto badCamera = beginFrame(wire);
    C::putUInt32(badCamera.body, 20, 0);
    check(service.begin(identity(wire), badCamera).reason == 11, "zero cameraKey is rejected");

    wire = wireFor(dep);
    auto badRevision = beginFrame(wire);
    badRevision.body.replace(24, 16, QByteArray(16, 0));
    check(service.begin(identity(wire), badRevision).reason == 11, "missing camera revision is rejected");

    wire = wireFor(dep);
    auto fakePose = beginFrame(wire);
    C::putFloat64(fakePose.body, 56, 1.0);
    check(service.begin(identity(wire), fakePose).reason == 11, "BEGIN PoseBlock translation must be zero");

    wire = wireFor(dep);
    auto fakeQuaternion = beginFrame(wire);
    C::putFloat64(fakeQuaternion.body, 80, 1.0);
    check(service.begin(identity(wire), fakeQuaternion).reason == 11, "BEGIN PoseBlock quaternion must be zero");

    wire = wireFor(dep);
    auto badConvention = beginFrame(wire);
    C::putUInt32(badConvention.body, 176, 1);
    check(service.begin(identity(wire), badConvention).reason == 11, "BEGIN convention must be contract-only");

    wire = wireFor(dep);
    auto nonzeroReserved = beginFrame(wire);
    nonzeroReserved.body[200] = 1;
    check(service.begin(identity(wire), nonzeroReserved).reason == 11, "nonzero BEGIN reserved bytes are rejected");

    wire = wireFor(dep);
    frame = beginFrame(wire);
    V4RequestIdentity wrong = identity(wire);
    wrong.request += 1;
    check(service.begin(wrong, frame).reason == 11, "identity must match the header requestId");

    check(productionReserve(dep, QStringLiteral("probe-validation-2")) == 0,
          "validation rejections never leave ownership");
    check(service.activeCount() == 0, "validation rejections create no session");
}

void beginIdempotencyAndLease() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto a = dependency(QStringLiteral("begin-A"), 20);
    auto b = dependency(QStringLiteral("begin-B"), 21);
    provider.set(kStation, a);
    provider.set(kStation, b);
    RobotCalibrationSessionService service(store, provider, drain);

    Wire wire = wireFor(a);
    auto result = service.begin(identity(wire), beginFrame(wire));
    acceptBegin(result, wire);
    check(!result.replay, "first BEGIN is not a replay");
    const auto record = service.session(wire.session);
    check(record.has_value() && record->dependencies == a, "BEGIN freezes the complete dependency snapshot");
    check(record->recoveryRequired == false && record->abortRequested == false,
          "new session is not recovery/abort pending");
    check(record->generation >= 1, "session receives a capture generation");
    check(service.activeCount() == 1, "one active calibration session");
    const auto generation = service.admitCapture(wire.session);
    check(generation.has_value() && *generation == record->generation, "capture admission returns the fence");
    check(service.acceptCaptureCallback(wire.session, *generation), "current capture callback is accepted");
    check(!service.acceptCaptureCallback(wire.session, *generation + 1), "wrong capture generation is rejected");
    check(activeLeaseDiagnostics(wire.session) == 1, "exactly one calibration lease diagnostic");
    check(productionReserve(a, QStringLiteral("probe-production-A")) == 3,
          "production admission on the calibration camera is CAMERA_BUSY");
    check(productionReserve(b, QStringLiteral("probe-independent-B")) == 0,
          "an independent station on another camera continues");

    Wire reconnect = wire;
    reconnect.connection = QUuid::createUuid();
    auto replay = service.begin(identity(reconnect), beginFrame(reconnect));
    check(replay.accepted && replay.replay && replay.session == wire.session,
          "duplicate BEGIN with a new connection replays the same session");
    check(service.activeCount() == 1 && activeLeaseDiagnostics(wire.session) == 1,
          "reconnect duplicate does not create another lease");
    check(service.session(wire.session)->generation == record->generation,
          "reconnect duplicate does not advance the generation");
    check(productionReserve(a, QStringLiteral("probe-production-A2")) == 3,
          "reconnect duplicate keeps the same ownership");

    Wire conflicting = wireFor(a);
    conflicting.request = wire.request;
    conflicting.plate += 1;
    auto conflict = service.begin(identity(conflicting), beginFrame(conflicting));
    check(conflict.reason == 6 && !conflict.accepted, "same command key with different content is CONFLICT");
    check(service.activeCount() == 1, "conflicting BEGIN creates no session");

    Wire sameSession = wireFor(a);
    sameSession.session = wire.session;
    auto duplicateSession = service.begin(identity(sameSession), beginFrame(sameSession));
    check(duplicateSession.reason == 6, "a new command key cannot reuse an existing calibration session UUID");
    check(service.activeCount() == 1, "duplicate session UUID creates no lease");

    Wire second = wireFor(a);
    auto busy = service.begin(identity(second), beginFrame(second));
    check(busy.reason == 3 && !busy.accepted, "a second calibration session on the same camera is CAMERA_BUSY");
    check(service.activeCount() == 1, "busy calibration BEGIN creates no session");

    Wire independent = wireFor(b);
    auto other = service.begin(identity(independent), beginFrame(independent));
    acceptBegin(other, independent);
    check(service.activeCount() == 2, "independent camera calibration can run concurrently");

    for (const QUuid& session : {wire.session, independent.session}) {
        auto abort = abortFrame(wire, nextRequest++, session);
        check(service.abort(identity(abort), abort).state == CalibrationState::Aborted,
              "test cleanup aborts each calibration session");
    }
}

void dependencyRejects() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto base = dependency(QStringLiteral("dependency"), 30);
    provider.set(kStation, base);
    RobotCalibrationSessionService service(store, provider, drain);

    auto expectStale = [&](CalibrationDependencySnapshot changed, const char* why) {
        provider.entries.clear();
        provider.entries.push_back({kStation, changed.cameraKey, changed});
        Wire wire = wireFor(base);
        auto result = service.begin(identity(wire), beginFrame(wire));
        check(result.reason == 4 && !result.accepted, why);
        check(service.activeCount() == 0, "stale dependency creates no session");
        check(productionReserve(base, QStringLiteral("probe-stale")) == 0,
              "stale dependency leaves no lease");
    };

    auto changed = base;
    changed.cameraRevision = QUuid::createUuid();
    expectStale(changed, "camera revision mismatch is STALE_CONFIG");
    changed = base;
    changed.intrinsicRevisionKey += 1;
    expectStale(changed, "intrinsic revision mismatch is STALE_CONFIG");
    changed = base;
    changed.plateRevisionKey += 1;
    expectStale(changed, "plate revision mismatch is STALE_CONFIG");
    changed = base;
    changed.robotAdapterRevisionKey += 1;
    expectStale(changed, "robot adapter revision mismatch is STALE_CONFIG");
    changed = base;
    changed.flangeTcpRevisionKey = 9;
    changed.flangeTcpRevision = QUuid::createUuid();
    expectStale(changed, "TCP offset presence mismatch is STALE_CONFIG");
    changed = base;
    changed.pose.targetFrameRevision = QUuid::createUuid();
    expectStale(changed, "frame revision mismatch is STALE_CONFIG");
    changed = base;
    changed.mode = HandEyeMode::EyeToHand;
    expectStale(changed, "mode mismatch is STALE_CONFIG");
    changed = base;
    changed.cameraKey = 0;
    expectStale(changed, "incomplete dependency snapshot is STALE_CONFIG");

    provider.entries.clear();
    provider.entries.push_back({kStation, base.cameraKey, base});
    changed = base;
    changed.modelRevisionKey = 0;
    provider.entries[0].snapshot = changed;
    Wire wire = wireFor(base);
    check(service.begin(identity(wire), beginFrame(wire)).reason == 4,
          "missing model dependency is STALE_CONFIG");
    check(productionReserve(base, QStringLiteral("probe-model")) == 0, "missing model leaves no lease");

    provider.entries.clear();
    provider.entries.push_back({kStation, base.cameraKey, base});
    provider.fail = true;
    wire = wireFor(base);
    check(service.begin(identity(wire), beginFrame(wire)).reason == 20,
          "dependency provider failure is NOT_READY");
    provider.fail = false;
    check(productionReserve(base, QStringLiteral("probe-provider")) == 0,
          "provider failure leaves no lease");

    // A successful BEGIN freezes acquisition/model identities from the provider.
    provider.entries.clear();
    provider.entries.push_back({kStation, base.cameraKey, base});
    wire = wireFor(base);
    auto accepted = service.begin(identity(wire), beginFrame(wire));
    acceptBegin(accepted, wire);
    check(service.session(wire.session)->dependencies.acquisitionProfileRevision ==
              base.acquisitionProfileRevision,
          "acquisition dependency is frozen at BEGIN");
    check(service.session(wire.session)->dependencies.modelRevision == base.modelRevision,
          "model dependency is frozen at BEGIN");

    auto abort = abortFrame(wire, nextRequest++, wire.session);
    check(service.abort(identity(abort), abort).state == CalibrationState::Aborted, "dependency test cleans up");
}

void beginPersistenceRollback() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto dep = dependency(QStringLiteral("rollback"), 40);
    provider.set(kStation, dep);
    RobotCalibrationSessionService service(store, provider, drain);

    Wire wire = wireFor(dep);
    store.failNext();
    auto failed = service.begin(identity(wire), beginFrame(wire));
    check(failed.reason == 8 && !failed.accepted, "BEGIN persistence failure is RECORDING_ERROR");
    check(service.activeCount() == 0 && !service.session(wire.session).has_value(),
          "failed BEGIN creates no in-memory session");
    check(productionReserve(dep, QStringLiteral("probe-rollback")) == 0,
          "failed BEGIN releases the reserved lease");

    auto retry = service.begin(identity(wire), beginFrame(wire));
    acceptBegin(retry, wire);
    check(productionReserve(dep, QStringLiteral("probe-rollback-2")) == 3,
          "retry after persistence recovery owns the camera");
    auto abort = abortFrame(wire, nextRequest++, wire.session);
    check(service.abort(identity(abort), abort).state == CalibrationState::Aborted,
          "rollback retry session aborts cleanly");
}

void abortLifecycle() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto dep = dependency(QStringLiteral("abort"), 50);
    provider.set(kStation, dep);
    RobotCalibrationSessionService service(store, provider, drain);

    Wire wire = wireFor(dep);
    acceptBegin(service.begin(identity(wire), beginFrame(wire)), wire);
    const quint64 generation = service.session(wire.session)->generation;
    bool admissionClosedDuringDrain = false;
    drain.during = [&] { admissionClosedDuringDrain = !service.admitCapture(wire.session).has_value(); };

    const quint64 abortRequest = nextRequest++;
    auto abort = abortFrame(wire, abortRequest, wire.session);
    auto result = service.abort(identity(abort), abort);
    check(result.accepted && result.reason == 0 && result.state == CalibrationState::Aborted,
          "ABORT completes after a real drain");
    check(result.response.header.kind == 111, "ABORT returns TERMINAL after durable completion");
    check(C::uint32(result.response.body, 100) == 11, "ABORT terminal commandState is ABORTED");
    check(admissionClosedDuringDrain, "new capture is closed while ABORT drains");
    check(drain.calls == 1, "ABORT drains exactly once");
    check(!service.admitCapture(wire.session).has_value(), "ABORT closes later capture admission");
    check(!service.acceptCaptureCallback(wire.session, generation), "late callback cannot revive an aborted session");
    check(productionReserve(dep, QStringLiteral("probe-after-abort")) == 0,
          "ownership is released only after the drain completed");
    check(activeLeaseDiagnostics(wire.session) == 0, "aborted session has no lease diagnostic");

    auto duplicate = service.abort(identity(abort), abort);
    check(duplicate.accepted && duplicate.replay && duplicate.state == CalibrationState::Aborted,
          "duplicate ABORT replays the terminal");
    check(drain.calls == 1, "duplicate ABORT does not drain again");

    auto conflicting = abort;
    C::putUInt32(conflicting.body, 16, 2);
    check(service.abort(identity(abort), conflicting).reason == 6,
          "same ABORT key with different content is CONFLICT");

    Wire unknown = wireFor(dep);
    auto unknownAbort = abortFrame(unknown, nextRequest++, QUuid::createUuid());
    check(service.abort(identity(unknownAbort), unknownAbort).reason == 10,
          "ABORT with an unknown session is UNKNOWN_KEY");

    auto foreignStation = abortFrame(wire, nextRequest++, wire.session, kOtherStation);
    check(service.abort(abortIdentity(foreignStation, kOtherStation), foreignStation).reason == 24,
          "ABORT from another station is CONTRACT_MISMATCH");

    auto replayBegin = service.begin(identity(wire), beginFrame(wire));
    check(replayBegin.accepted && replayBegin.replay && replayBegin.state == CalibrationState::Aborted,
          "duplicate BEGIN after ABORT returns the terminal state");
    check(service.activeCount() == 0 && productionReserve(dep, QStringLiteral("probe-terminal")) == 0,
          "duplicate BEGIN after ABORT acquires no new lease");

    Wire next = wireFor(dep);
    acceptBegin(service.begin(identity(next), beginFrame(next)), next);
    const quint64 nextGeneration = service.session(next.session)->generation;
    check(nextGeneration > generation, "a new session gets a newer capture generation");
    check(!service.acceptCaptureCallback(wire.session, generation),
          "old session callback remains stale after a new session starts");
    check(!service.acceptCaptureCallback(next.session, generation),
          "old generation cannot address the new session");
    check(productionReserve(dep, QStringLiteral("probe-new-owner")) == 3,
          "late old callback did not release the new session lease");

    auto nextAbort = abortFrame(next, nextRequest++, next.session);
    check(service.abort(identity(nextAbort), nextAbort).state == CalibrationState::Aborted,
          "new session aborts");
}

void abortDrainFailureAndRetry() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto dep = dependency(QStringLiteral("drain"), 60);
    provider.set(kStation, dep);
    RobotCalibrationSessionService service(store, provider, drain);

    Wire wire = wireFor(dep);
    acceptBegin(service.begin(identity(wire), beginFrame(wire)), wire);
    const quint64 generation = service.session(wire.session)->generation;
    drain.clean = false;
    const quint64 firstRequest = nextRequest++;
    auto first = abortFrame(wire, firstRequest, wire.session);
    auto failed = service.abort(identity(first), first);
    check(failed.reason == 9 && !failed.accepted, "drain failure is RECOVERY_REQUIRED, not completion");
    check(drain.calls == 1, "failed drain was attempted once");
    auto pending = service.session(wire.session);
    check(pending->abortRequested && pending->recoveryRequired, "failed drain persists abort/recovery state");
    check(pending->state == CalibrationState::Created, "failed drain leaves the session nonterminal");
    check(!service.admitCapture(wire.session).has_value(), "failed abort keeps new work closed");
    check(!service.acceptCaptureCallback(wire.session, generation), "failed abort fences late callbacks");
    check(productionReserve(dep, QStringLiteral("probe-drain-pending")) == 3,
          "failed drain retains ownership");

    auto duplicate = service.abort(identity(first), first);
    check(duplicate.reason == 9, "uncommitted failed ABORT is retried, not reported complete");
    check(drain.calls == 2, "retry drains again");

    drain.clean = true;
    auto retry = abortFrame(wire, nextRequest++, wire.session);
    auto completed = service.abort(identity(retry), retry);
    check(completed.accepted && completed.state == CalibrationState::Aborted,
          "later clean drain completes ABORT");
    check(drain.calls == 3, "completion is tied to the final real drain");
    check(productionReserve(dep, QStringLiteral("probe-drain-done")) == 0,
          "ownership is released after the final clean drain");
    auto recovered = service.recover(wire.session);
    check(recovered.accepted && recovered.replay && recovered.state == CalibrationState::Aborted,
          "recovering an aborted session is terminal and does not reacquire a lease");
    check(productionReserve(dep, QStringLiteral("probe-drain-terminal")) == 0,
          "terminal recovery creates no lease");
}

void abortPersistenceFailure() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto dep = dependency(QStringLiteral("abort-store"), 70);
    provider.set(kStation, dep);
    RobotCalibrationSessionService service(store, provider, drain);

    Wire wire = wireFor(dep);
    acceptBegin(service.begin(identity(wire), beginFrame(wire)), wire);

    // First durable write (abortRequested) fails: no drain, lease retained.
    store.failNext();
    auto first = abortFrame(wire, nextRequest++, wire.session);
    auto failed = service.abort(identity(first), first);
    check(failed.reason == 8, "abort-requested persistence failure is RECORDING_ERROR");
    check(drain.calls == 0, "no drain starts before abort intent is durable");
    check(productionReserve(dep, QStringLiteral("probe-abort-store-1")) == 3,
          "failed abort persistence retains ownership");
    check(!service.session(wire.session)->abortRequested,
          "failed abort persistence does not pretend the close was durable");

    auto retry = abortFrame(wire, nextRequest++, wire.session);
    check(service.abort(identity(retry), retry).state == CalibrationState::Aborted,
          "abort retry succeeds after storage recovers");
    check(drain.calls == 1, "successful abort retry drains once");
    check(productionReserve(dep, QStringLiteral("probe-abort-store-2")) == 0,
          "successful abort releases ownership");

    // Final terminal write fails after a clean drain: ownership must stay until the
    // terminal state is durable.
    Wire second = wireFor(dep);
    acceptBegin(service.begin(identity(second), beginFrame(second)), second);
    const int before = store.commitCalls;
    store.failAtCommit = before + 2; // abortRequested write, then terminal write
    auto third = abortFrame(second, nextRequest++, second.session);
    auto terminalFailed = service.abort(identity(third), third);
    check(terminalFailed.reason == 8, "terminal persistence failure is RECORDING_ERROR");
    check(drain.calls == 2, "terminal write failure happened after one drain");
    check(service.session(second.session)->state == CalibrationState::Created,
          "terminal write failure leaves the session nonterminal");
    check(productionReserve(dep, QStringLiteral("probe-abort-store-3")) == 3,
          "terminal write failure retains ownership");
    auto finalRetry = abortFrame(second, nextRequest++, second.session);
    check(service.abort(identity(finalRetry), finalRetry).state == CalibrationState::Aborted,
          "final abort retry completes after storage recovery");
    check(productionReserve(dep, QStringLiteral("probe-abort-store-4")) == 0,
          "final abort retry releases ownership");
}

void restartRecovery() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto dep = dependency(QStringLiteral("restart"), 80);
    provider.set(kStation, dep);
    QUuid session;
    quint64 generation = 0;
    {
        RobotCalibrationSessionService service(store, provider, drain);
        Wire wire = wireFor(dep);
        session = wire.session;
        acceptBegin(service.begin(identity(wire), beginFrame(wire)), wire);
        generation = service.session(session)->generation;
    }
    check(productionReserve(dep, QStringLiteral("probe-restart-free")) == 0,
          "destroying the service does not leave a process lease");

    RobotCalibrationSessionService restored(store, provider, drain);
    auto record = restored.session(session);
    check(record.has_value() && record->recoveryRequired, "restart marks a nonterminal session recovery-required");
    check(restored.activeCount() == 0, "restart does not reacquire ownership");
    check(!restored.admitCapture(session).has_value(), "restart does not admit capture automatically");
    check(productionReserve(dep, QStringLiteral("probe-restart-noauto")) == 0,
          "restart opens no production/lease permission");

    auto beginAgain = restored.begin({kInstallation, kPeer, kEpoch, kStation, record->beginCommand.request},
                                     beginFrame(Wire{kInstallation, kPeer, kEpoch, kStation, kConnection,
                                                     record->beginCommand.request, 1, session,
                                                     dep.mode, dep.cameraKey, dep.cameraRevision,
                                                     dep.intrinsicRevisionKey, dep.plateRevisionKey,
                                                     dep.flangeTcpRevisionKey, dep.robotAdapterRevisionKey,
                                                     dep.pose}));
    check(beginAgain.reason == 9 && !beginAgain.accepted,
          "duplicate BEGIN after restart requires explicit recovery");
    check(restored.activeCount() == 0 && productionReserve(dep, QStringLiteral("probe-restart-noauto2")) == 0,
          "duplicate BEGIN after restart reacquires nothing");

    auto changed = dep;
    changed.modelRevision = QUuid::createUuid();
    provider.entries.clear();
    provider.entries.push_back({kStation, changed.cameraKey, changed});
    check(restored.recover(session).reason == 4,
          "explicit recovery rejects a changed dependency revision");
    check(restored.activeCount() == 0 && productionReserve(dep, QStringLiteral("probe-restart-stale")) == 0,
          "stale recovery acquires no lease");

    provider.entries.clear();
    provider.entries.push_back({kStation, dep.cameraKey, dep});
    auto recovered = restored.recover(session);
    check(recovered.accepted && recovered.state == CalibrationState::Created,
          "explicit recovery succeeds after dependency recheck");
    check(restored.session(session)->recoveryRequired == false, "recovery clears the recovery flag");
    check(restored.session(session)->generation == generation, "recovery preserves the capture generation");
    check(restored.admitCapture(session).has_value(), "recovery reopens the capture boundary");
    check(productionReserve(dep, QStringLiteral("probe-restart-owned")) == 3,
          "explicit recovery reacquires calibration ownership");

    const quint64 replayRequest = record->beginCommand.request;
    Wire reconnectWire{kInstallation, kPeer, kEpoch, kStation, QUuid::createUuid(), replayRequest, 1,
                       session, dep.mode, dep.cameraKey, dep.cameraRevision, dep.intrinsicRevisionKey,
                       dep.plateRevisionKey, dep.flangeTcpRevisionKey, dep.robotAdapterRevisionKey, dep.pose};
    auto replay = restored.begin({kInstallation, kPeer, kEpoch, kStation, replayRequest},
                                 beginFrame(reconnectWire));
    check(replay.accepted && replay.replay && restored.activeCount() == 1,
          "reconnect after recovery replays without a second lease");

    auto abort = abortFrame(reconnectWire, nextRequest++, session);
    check(restored.abort(identity(abort), abort).state == CalibrationState::Aborted,
          "recovered session aborts");
    check(restored.recover(session).state == CalibrationState::Aborted,
          "terminal recovery is idempotent");
}

void restartAbortedAndFileStore() {
    Provider provider;
    Drain drain;
    MemoryStore store;
    auto dep = dependency(QStringLiteral("restart-aborted"), 90);
    provider.set(kStation, dep);
    QUuid session;
    {
        RobotCalibrationSessionService service(store, provider, drain);
        Wire wire = wireFor(dep);
        session = wire.session;
        acceptBegin(service.begin(identity(wire), beginFrame(wire)), wire);
        auto abort = abortFrame(wire, nextRequest++, session);
        check(service.abort(identity(abort), abort).state == CalibrationState::Aborted,
              "session aborts before restart");
    }
    RobotCalibrationSessionService restored(store, provider, drain);
    check(restored.session(session)->state == CalibrationState::Aborted, "restart preserves ABORTED");
    check(restored.session(session)->recoveryRequired == false, "terminal restart is not recovery-required");
    auto recovered = restored.recover(session);
    check(recovered.accepted && recovered.state == CalibrationState::Aborted,
          "terminal restart recovery is terminal");
    check(productionReserve(dep, QStringLiteral("probe-restart-aborted")) == 0,
          "terminal restart opens no lease");

    QTemporaryDir directory;
    check(directory.isValid(), "temporary directory for file store");
    const QString path = QDir(directory.path()).filePath(QStringLiteral("calibration-sessions.json"));
    QUuid fileSession;
    {
        FileRobotCalibrationSessionStore fileStore(path);
        RobotCalibrationSessionService service(fileStore, provider, drain);
        Wire wire = wireFor(dep);
        fileSession = wire.session;
        acceptBegin(service.begin(identity(wire), beginFrame(wire)), wire);
    }
    check(QFile::exists(path), "file calibration session store was written");
    {
        FileRobotCalibrationSessionStore fileStore(path);
        const auto document = QJsonDocument::fromJson([&] {
            QFile file(path);
            check(file.open(QIODevice::ReadOnly), "file store readable");
            return file.readAll();
        }());
        check(document.isObject() && document.object().value("schema").toInt() == 1,
              "file store has the frozen schema marker");
        RobotCalibrationSessionService service(fileStore, provider, drain);
        auto record = service.session(fileSession);
        check(record.has_value() && record->recoveryRequired,
              "file store restart restores the durable session");
        check(service.recover(fileSession).accepted, "file store session recovers explicitly");
        auto wire = wireFor(dep);
        wire.session = fileSession;
        auto abort = abortFrame(wire, nextRequest++, fileSession);
        check(service.abort(identity(abort), abort).state == CalibrationState::Aborted,
              "file store session aborts");
    }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        validation();
        beginIdempotencyAndLease();
        dependencyRejects();
        beginPersistenceRollback();
        abortLifecycle();
        abortDrainFailureAndRetry();
        abortPersistenceFailure();
        restartRecovery();
        restartAbortedAndFileStore();
        QTextStream(stdout) << "PASS robot_calibration_session_contract checks=" << checks
                            << " fixture=synthetic lifecycle, no physical camera/robot\n";
        return 0;
    } catch (const std::exception& error) {
        QTextStream(stderr) << "FAIL robot_calibration_session_contract checks=" << checks
                            << " error=" << error.what() << "\n";
        return 1;
    }
}
