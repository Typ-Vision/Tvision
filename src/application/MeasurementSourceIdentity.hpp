#pragma once
#include "domain/MeasurementSetupContract.hpp"
#include "ExperimentalCalibrationEvidence.hpp"
namespace visionflow {
struct SetupReference {
    CameraId camera;
    SetupId setup;
    int revision=1;
    bool operator==(const SetupReference&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static SetupReference fromJson(const QJsonObject&);
};
// Value evidence, not a trusted acquisition ticket or permission.
// sourceId belongs to an actual recipe tool or calibration-session source.
enum class SourceIdentityKind { RecipeTool, CalibrationSession };
struct MeasurementSourceEvidence {
    SourceOrigin origin=SourceOrigin::TrialFile;
    QString sourceId, frameId, cycleId, coordinates, pixelHash, filePath;
    int generation=1;
    std::optional<SetupReference> target;
    SourceIdentityKind sourceKind=SourceIdentityKind::CalibrationSession;
    std::optional<ExperimentalCalibrationEvidence> experimental;
    bool operator==(const MeasurementSourceEvidence&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static MeasurementSourceEvidence fromJson(const QJsonObject&);
};
}
