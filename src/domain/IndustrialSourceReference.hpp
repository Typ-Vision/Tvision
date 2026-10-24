#pragma once
#include "MeasurementSetupContract.hpp"

namespace visionflow {
// Standalone preparation value, deliberately absent from ToolRegistry/project codecs.
// It is a requested binding, never actual acquisition evidence or permission.
struct IndustrialSourceReference {
    CameraId camera;
    SetupId setup;
    ProfileId profile;
    int deviceRevision=1,setupRevision=1,profileRevision=1;
    PhysicalDeviceIdentity physical;
    QString acquisitionId;
    int timeoutMs=5000;
    bool operator==(const IndustrialSourceReference&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static IndustrialSourceReference fromJson(const QJsonObject&);
    // Explicit user edit only: capture current reference without mutating environment.
    static IndustrialSourceReference bind(const MeasurementSetupContract&,const ProfileId&,int timeoutMs);
    void validateAgainst(const MeasurementSetupContract&) const;
};
}
