#pragma once
#include "CommunicationPeerSessionService.hpp"
#include "domain/StationDefinition.hpp"
#include <QMap>
#include <QSet>
#include <memory>

namespace visionflow {
struct InputSnapshot;
// Created by the application from the active immutable manifest, never from a
// network request. Input values/quality/freshness are frozen by that provider.
struct StationAdmissionSnapshot {
    std::shared_ptr<const StationSnapshot> station;
    quint32 templateKey=0;
    QUuid cameraSetRevision, inputSnapshot, inputRevision;
    std::shared_ptr<const InputSnapshot> inputValues;
};
struct StationAdmissionBarriers {
    StationMode mode=StationMode::Stopped;
    bool ready=false, recordingPending=false, deliveryPending=false;
    quint32 resourceReason=0; // frozen W7 reason, including native physical gate
};
struct StationAdmissionRecord {
    QByteArray fingerprint;
    CommunicationV4Frame replay; // durable ACCEPTED or sealed TERMINAL
};
struct StationAdmissionIntent {
    V4RequestIdentity identity;
    QByteArray fingerprint;
    CommunicationV4Frame accepted;
    StationAdmissionSnapshot snapshot;
    CommunicationV4Frame request; // original body survives terminal/reconnect
};
class StationAdmissionStore {
public:
    virtual ~StationAdmissionStore()=default;
    virtual std::optional<StationAdmissionRecord> find(const V4RequestIdentity&) const=0;
    // Must durably reserve intent AND terminal headroom before returning. Never
    // pump events/call control ports. Throw is ambiguous: admission stays fenced.
    // Restart recovery must prohibit execution/re-run of unresolved intents.
    virtual void commit(const StationAdmissionIntent&)=0;
};
struct StationAdmissionReservation {
    quint32 reason=0;
    std::shared_ptr<void> lease;
};
class StationAdmissionBoundary {
public:
    virtual ~StationAdmissionBoundary()=default;
    virtual std::optional<StationAdmissionSnapshot> snapshot(const QUuid&) const=0;
    virtual StationAdmissionBarriers barriers(const QUuid&) const=0;
    // All-or-none resource/headroom reservation; lease releases on rejection.
    virtual StationAdmissionReservation reserve(const StationAdmissionSnapshot&)=0;
    // Existing Coordinator/worker adapter consumes the frozen intent. No capture
    // may precede durable commit. Exceptions require explicit durable recovery.
    virtual void begin(const StationAdmissionIntent&,const std::shared_ptr<void>& lease)=0;
    virtual bool drained(const QUuid&) const=0;
};
enum class StationTriggerOrigin { Plc, Manual };
class StationAdmissionService final {
public:
    StationAdmissionService(StationAdmissionStore&,StationAdmissionBoundary&);
    StationAdmissionService(const StationAdmissionService&)=delete;
    StationAdmissionService& operator=(const StationAdmissionService&)=delete;
    CommunicationV4Frame trigger(const V4RequestIdentity&,const CommunicationV4Frame&,StationTriggerOrigin=StationTriggerOrigin::Plc);
    // Execution completion only: exact durable terminal and real drain required.
    bool release(const V4RequestIdentity&);
private:
    void owner() const;
    struct Active {V4RequestIdentity identity;QByteArray fingerprint;CommunicationV4Frame accepted;std::shared_ptr<void> lease;quint32 blockedReason=0;};
    StationAdmissionStore& store_;
    StationAdmissionBoundary& boundary_;
    QMap<QUuid,Active> active_;
    QSet<QUuid> admitting_;
    QThread* owner_=QThread::currentThread();
};
}
