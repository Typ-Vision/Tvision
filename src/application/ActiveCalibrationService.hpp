#pragma once
#include "MeasurementService.hpp"
#include "ExperimentalCalibrationEvidence.hpp"
#include <functional>

namespace visionflow {
// Shared installation identity, deliberately independent of acquisition tool IDs.
// configId is the SHA256 of the adapter's relevant acquisition configuration.
struct MeasurementSetup {
    QString id,adapter,configId,lensId,planeId;
    QSize resolution;
    CoordinateFrame source,target;
    bool operator==(const MeasurementSetup&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static MeasurementSetup fromJson(const QJsonObject&);
};
struct CalibrationImageEvidence {
    // acquired / setup_file (explicit same-installation declaration) / trial_file
    QString origin,sourceToolId,frameId,pixelHash;
    std::optional<ExperimentalCalibrationEvidence> experimental;
    bool operator==(const CalibrationImageEvidence&) const = default;
};
struct CalibrationControl {
    QVector<CalibrationSample> samples;
    std::optional<double> limitMillimetres;
};
// Mathematical readiness is replayed from the legacy math, never a writable VALID.
// Controls are separate evidence; absence is not zero error or acceptance.
class CalibrationModelRecord {
public:
    static CalibrationModelRecord create(const PlanarCalibration&,const MeasurementSetup&,
        const CalibrationImageEvidence&,const CalibrationControl& = {});
    static CalibrationModelRecord mapLegacy(const CalibrationModelRecord&,const MeasurementSetup&,const QString& name,QDateTime at);
    static CalibrationModelRecord associate(const CalibrationModelRecord&,const MeasurementSetup&,const QString& name,QDateTime at=QDateTime::currentDateTimeUtc());
    QString originalContentId() const {return original_.value("contentId").toString();}
    static CalibrationModelRecord decode(const QJsonObject&);
    // Explicit legacy adoption: preserves accepted legacy evidence and records
    // that a decoded raster hash was not available. Never invents image proof.
    static CalibrationModelRecord adoptLegacy(const PlanarCalibration&,const MeasurementSetup&,const QString& name,QDateTime at=QDateTime::currentDateTimeUtc());
    QJsonObject encode() const;
    CalibrationModelRecord named(const QString&,QDateTime recordedAt=QDateTime::currentDateTimeUtc()) const;
    QString name() const {return name_.isEmpty() ? model_.dependency().calibrationId : name_;}
    const QDateTime& recordedAt() const {return recordedAt_;}
    const PlanarCalibration& model() const {return model_;}
    const MeasurementSetup& setup() const {return setup_;}
    const CalibrationImageEvidence& image() const {return image_;}
    const QVector<double>& residualsMillimetres() const {return residuals_;}
    const std::optional<bool>& controlAccepted() const {return accepted_;}
    const QString& contentId() const {return contentId_;}
    bool activatable() const;
private:
    CalibrationModelRecord(PlanarCalibration,MeasurementSetup,CalibrationImageEvidence,CalibrationControl);
    PlanarCalibration model_;
    MeasurementSetup setup_;
    CalibrationImageEvidence image_;
    CalibrationControl control_;
    QVector<double> residuals_;
    std::optional<bool> accepted_;
    QString contentId_;
    QString name_;
    QDateTime recordedAt_;
    bool legacy_=false;
    QJsonObject original_,derived_;
    bool migration_=false;
};
struct ActiveCalibrationSelection {
    QString id,contentId;
    int version=0,revision=0;
    bool operator==(const ActiveCalibrationSelection&) const = default;
};
struct ActiveCalibrationSnapshot {
    CalibrationModelRecord record;
    ActiveCalibrationSelection selection;
    QJsonObject scope; // schema1 exact physical setup; empty for legacy compatibility
    QJsonObject acquisition; // CCUX-06 fresh session evidence; never rewrites model/scope2
};
// One application-owned instance per open project. Owner-thread commands only.
// The required gate checks Runtime/worker/pending record/ACK before any change.
// Persistence remembers a choice; construction never restores active permission.
class ActiveCalibrationService {
public:
    using ChangeGate=std::function<bool()>;
    ActiveCalibrationService(QString absolutePath,QString projectId,ChangeGate);
    QVector<CalibrationModelRecord> records() const;
    void save(const CalibrationModelRecord&) const;
    void activate(const QString& id,int version,const MeasurementSetup& current);
    void deactivate();
    ActiveCalibrationSnapshot resolve(const MeasurementSetup& current) const;
    std::optional<ActiveCalibrationSelection> active() const {return active_;}
    std::optional<ActiveCalibrationSelection> remembered() const;
    bool systemPolicy() const {return read()["revision"].toInt()>0;}
    ActiveCalibrationSnapshot snapshot() const;
private:
    QJsonObject read() const;
    void write(const QJsonObject&) const;
    QString path_,projectId_;
    ChangeGate gate_;
    std::optional<ActiveCalibrationSelection> active_;
};
}
