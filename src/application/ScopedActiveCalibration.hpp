#pragma once
#include "MeasurementEnvironment.hpp"
#include "IndependentCalibrationCapture.hpp"

namespace visionflow {
struct ScopedCalibrationSnapshot {
    SetupReference target;
    DeviceBinding device;
    ActiveCalibrationSnapshot calibration;
};
enum class CalibrationAvailability { None, Remembered, Valid, RecalibrationRequired };
struct CalibrationAvailabilityStatus {
    CalibrationAvailability state=CalibrationAvailability::None;
    std::optional<RememberedCalibration> remembered;
    bool mathematicsCompatible=false;
    bool sessionPermitted=false;
    QString reason;
};
// Application owned. Legacy ActiveCalibrationService remains the single-source
// compatibility path until explicit Editor/Runtime migration (KMC-7/8).
class ScopedActiveCalibrationService {
public:
    ScopedActiveCalibrationService(MeasurementEnvironmentRepository, ActiveCalibrationService::ChangeGate);
    MeasurementEnvironment environment() const;
    QVector<CalibrationModelRecord> records(const SetupReference&) const;
    QVector<CalibrationModelRecord> historyRecords(const SetupId&) const;
    MeasurementSetup setup(const SetupReference& ref) const {return installation(environment(),ref);}
    ActiveCalibrationSnapshot candidate(const SetupReference&,const QString& contentId) const;
    int migrateLegacy(const SetupReference&,const QVector<CalibrationModelRecord>&,int failAfter=-1);
    void save(const SetupReference&,const CalibrationModelRecord&);
    void activate(const SetupReference&,const QString& contentId,const IndependentCalibrationSession& probe,
                  MeasurementEnvironmentRepository::Fault=MeasurementEnvironmentRepository::Fault::None);
    void deactivate(const SetupId&);
    void setOperational(const CameraId&,const SetupId&);
    void disconnect(const CameraId&);
    CalibrationAvailabilityStatus status(const SetupReference&) const;
    // Explicit recheck of the remembered choice, not a new fit or production start.
    void prepareRemembered(const SetupReference&,const IndependentCalibrationSession&);
    void notifyOpticalChange(const CameraId&,int expectedRevision);
    void refreshAvailability(const QVector<DeviceBinding>&,const IndustrialDiscoverySnapshot& = {});
    QVector<ScopedCalibrationSnapshot> snapshots(const QVector<SetupReference>&,std::function<void()> beforeVerify={}) const;
private:
    void owner() const;
    MeasurementSetup installation(const MeasurementEnvironment&,const SetupReference&) const;
    MeasurementEnvironmentRepository repository_;
    ActiveCalibrationService::ChangeGate gate_;
    QThread* thread_;
    struct Permission { RememberedCalibration selection; int revision; std::optional<AcquisitionProfile> profile{}; std::optional<DiscoveredDevice> device{}; QJsonObject acquisition; bool operator==(const Permission&) const = default; };
    QMap<QString,Permission> permissions_;
    int revision_=0;
    QSet<QString> usedProbes_;
};
}
