#pragma once
#include <QJsonObject>
#include <QSize>
#include <QRect>
#include <QStringList>
#include <QVector>
#include <optional>

namespace visionflow {
// Project identities, never a display name, list index or recipe tool ID.
struct CameraId { QString value; bool operator==(const CameraId&) const = default; void validate() const; };
struct SetupId { QString value; bool operator==(const SetupId&) const = default; void validate() const; };
enum class DeviceIdentityConfidence { Unavailable, ProviderReported, SerialReported };
struct DeviceCapabilities {
    bool softwareCapture=false, resolution=false, sensorRoi=false, binning=false;
    bool operator==(const DeviceCapabilities&) const = default;
    QJsonObject toJson() const;
    static DeviceCapabilities fromJson(const QJsonObject&);
};
struct DeviceBinding {
    QString adapter, providerId;
    DeviceIdentityConfidence confidence=DeviceIdentityConfidence::Unavailable;
    int revision=1;
    DeviceCapabilities capabilities;
    bool operator==(const DeviceBinding&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static DeviceBinding fromJson(const QJsonObject&);
    // Exactly one actual discovery match is required. No index fallback.
    int resolve(const QVector<DeviceBinding>& discovered) const;
    DeviceBinding rebound(const DeviceBinding&) const;
};
struct GeometryDescriptor {
    QSize resolution;
    std::optional<QRect> sensorRoi;
    std::optional<QSize> binning;
    // Adapter must explicitly establish that no critical geometry is unknown.
    bool complete=false;
    bool operator==(const GeometryDescriptor&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static GeometryDescriptor fromJson(const QJsonObject&);
};
struct MeasurementCamera {
    CameraId id;
    QString name;
    DeviceBinding device;
    bool operator==(const MeasurementCamera&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static MeasurementCamera fromJson(const QJsonObject&);
};
struct MeasurementSetupDescriptor {
    SetupId id;
    CameraId camera;
    QString name, opticalProfile, workingPlane;
    int revision=1, deviceRevision=1;
    GeometryDescriptor geometry;
    bool operator==(const MeasurementSetupDescriptor&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static MeasurementSetupDescriptor fromJson(const QJsonObject&);
    // Rename preserves revision; physical/geometry changes increment it.
    MeasurementSetupDescriptor revised(const MeasurementSetupDescriptor&) const;
    QString geometryId() const;
    QString cacheId(const QJsonObject& nonGeometricSettings) const;
};
// Standalone value codec. Persistence, selections and activation belong to KMC-2/4.
struct MeasurementSetupContract {
    QVector<MeasurementCamera> cameras;
    QVector<MeasurementSetupDescriptor> setups;
    void validate(int capacity=128) const;
    QJsonObject toJson(int capacity=128) const;
    static MeasurementSetupContract fromJson(const QJsonObject&,int capacity=128);
};
enum class SourceOrigin { CameraAcquired, SetupFile, TrialFile, LegacyRecord };
QString sourceOriginName(SourceOrigin);
SourceOrigin sourceOriginFromName(const QString&);
}
