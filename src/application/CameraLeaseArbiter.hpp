#pragma once
#include "domain/MeasurementSetupContract.hpp"
#include <memory>

namespace visionflow {
enum class CameraLeaseUse { Calibration, Editor, Runtime, Preview };
struct CameraLeaseTarget {QString cameraId;PhysicalDeviceIdentity physical;};
struct CameraLeaseDiagnostic {
    QString owner,cameraId,physicalKey;
    CameraLeaseUse use=CameraLeaseUse::Editor;
    quint64 generation=0;
    bool claimed=false,quarantined=false;
};
// One process registry for UUID ownership AND exact physical alias exclusion.
// Leases are ephemeral capabilities, never decoded from project/config/wire data.
class CameraLeaseArbiter final {
public:
    class Lease;
    using Ticket=std::shared_ptr<Lease>;
    struct Reservation {quint32 reason=0;QVector<Ticket> leases;};
    static Reservation reserve(QString owner,CameraLeaseUse,const QVector<CameraLeaseTarget>&,
                               const QVector<Ticket>& reusable={});
    static QVector<CameraLeaseDiagnostic> diagnostics();
    static QString physicalKey(const PhysicalDeviceIdentity&);
private:
    friend class CameraWorkerSession;
    static void claim(const Ticket&,const PhysicalDeviceIdentity&,CameraLeaseUse,const QString& cameraId);
    static void drained(const Ticket&,bool clean);
};
class CameraLeaseArbiter::Lease final {
    friend class CameraLeaseArbiter;
    CameraLeaseDiagnostic diagnostic_;
public:
    quint64 generation() const {return diagnostic_.generation;}
    QString physicalKey() const {return diagnostic_.physicalKey;}
};
}
