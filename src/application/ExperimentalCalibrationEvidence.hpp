#pragma once
#include "domain/MeasurementSetupContract.hpp"
#include "vision/ImageFrame.hpp"

namespace visionflow {
// Value provenance only: never a trusted acquisition ticket or production admission.
struct ExperimentalCalibrationEvidence {
    SourceOrigin origin=SourceOrigin::SetupFile;
    MeasurementCamera camera;
    MeasurementSetupDescriptor setup;
    AcquisitionProfile profile;
    QJsonObject acquisition;
    bool operator==(const ExperimentalCalibrationEvidence&) const = default;
    void validate() const;
    void verify(const ImageFrame&) const;
    QJsonObject toJson() const;
    static ExperimentalCalibrationEvidence fromJson(const QJsonObject&);
};
}
