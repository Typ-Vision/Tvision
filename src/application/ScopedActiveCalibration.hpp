#pragma once
#include "MeasurementEnvironment.hpp"
#include "IndependentCalibrationCapture.hpp"

namespace visionflow {
struct ScopedCalibrationSnapshot {
    SetupReference target;
    DeviceBinding device;
    ActiveCalibrationSnapshot calibration;
};
// Deliberately distinct from the trusted Runtime/SystemCalibration snapshot API.
struct ExperimentalScopedCalibrationSnapshot { ScopedCalibrationSnapshot snapshot; };
enum class CalibrationSaveUseState { Failed, CandidateSaved, Activated };
struct CalibrationSaveUseRequest {
    SetupReference target;
    int environmentRevision=0, generation=0;
    CalibrationCapturePolicy policy=CalibrationCapturePolicy::Automatic;
};
struct CalibrationSaveUseResult {
    CalibrationSaveUseState state=CalibrationSaveUseState::Failed;
    QString contentId, error;
    bool duplicate=false, experimental=false;
    std::optional<bool> independentControlAccepted;
};
enum class CalibrationAvailability { None, Remembered, Valid, RecalibrationRequired };
struct CalibrationAvailabilityStatus {
    CalibrationAvailability state=CalibrationAvailability::None;
    std::optional<RememberedCalibration> remembered;
    bool mathematicsCompatible=false;
    bool sessionPermitted=false;
    bool experimental=false;
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
    // currentTarget also covers the caller's project/view lifetime and Stop.
    // Checked again after the immutable write, immediately before selection commit.
    CalibrationSaveUseResult saveAndUse(const CalibrationSaveUseRequest&,const CalibrationModelRecord&,
        const IndependentCalibrationSession&,std::function<bool()> currentTarget,
        MeasurementEnvironmentRepository::Fault=MeasurementEnvironmentRepository::Fault::None);
    void setExperimental(bool enabled);
    QVector<ExperimentalScopedCalibrationSnapshot> experimentalSnapshots(const QVector<SetupReference>&) const;
    void activate(const SetupReference&,const QString& contentId,const IndependentCalibrationSession& probe,
                  MeasurementEnvironmentRepository::Fault=MeasurementEnvironmentRepository::Fault::None);
    void deactivate(const SetupId&);
    void setOperational(const CameraId&,const SetupId&);
    void disconnect(const CameraId&,QJsonObject context={});
    CalibrationAvailabilityStatus status(const SetupReference&) const;
    // Explicit recheck of the remembered choice, not a new fit or production start.
    void prepareRemembered(const SetupReference&,const IndependentCalibrationSession&);
    void notifyOpticalChange(const CameraId&,int expectedRevision);
    void refreshAvailability(const QVector<DeviceBinding>&,const IndustrialDiscoverySnapshot& = {});
    QVector<ScopedCalibrationSnapshot> snapshots(const QVector<SetupReference>&,std::function<void()> beforeVerify={}) const;
private:
    void owner() const;
    MeasurementSetup installation(const MeasurementEnvironment&,const SetupReference&) const;
    void activateImpl(const SetupReference&,const QString&,const IndependentCalibrationSession&,
        CalibrationCapturePolicy,MeasurementEnvironmentRepository::Fault,std::function<void()> beforeCommit={},bool selectDefault=false);
    QVector<ScopedCalibrationSnapshot> snapshotsImpl(const QVector<SetupReference>&,bool,std::function<void()>) const;
    MeasurementEnvironmentRepository repository_;
    ActiveCalibrationService::ChangeGate gate_;
    QThread* thread_;
    struct Permission { RememberedCalibration selection; int revision; std::optional<AcquisitionProfile> profile{}; std::optional<DiscoveredDevice> device{}; QJsonObject acquisition; bool experimental=false; int commandBase=-1,commandCommitted=-1; bool operator==(const Permission&) const = default; };
    QMap<QString,Permission> permissions_;
    int revision_=0;
    QSet<QString> usedProbes_;
    bool experimental_=false;
    int permissionEpoch_=0;
    QMap<QString,int> cameraEpochs_;
};
}
