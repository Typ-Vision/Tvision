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
// ICM-03a industrial identity. PhysicalDeviceIdentity is portable project data;
// TransportLocator and BackendIdentity are local discovery facts, never project identity.
struct PhysicalDeviceIdentity {
    QString vendor, model, serial;
    // provider_reported: vendor/model without serial; serial_reported: all three actual values.
    DeviceIdentityConfidence confidence=DeviceIdentityConfidence::Unavailable;
    bool operator==(const PhysicalDeviceIdentity&) const = default;
    // Serial alone is not unique: only exact vendor+model+serial may select a device.
    bool usable() const {return confidence==DeviceIdentityConfidence::SerialReported;}
    void validate() const;
    QJsonObject toJson() const;
    static PhysicalDeviceIdentity fromJson(const QJsonObject&);
};
struct TransportLocator {
    QString backend; // oecs (direct C API, no producer) or gentl (canonical absolute producer path)
    QString producer, interfaceId, deviceId;
    bool operator==(const TransportLocator&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static TransportLocator fromJson(const QJsonObject&);
};
struct BackendIdentity {
    QString adapter, runtimeVersion, runtimeSha256;
    int adapterVersion=1;
    bool operator==(const BackendIdentity&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static BackendIdentity fromJson(const QJsonObject&);
    // Path independent: moving the same binary keeps it, replacing the binary changes it.
    QString fingerprint() const;
};
struct DiscoveredDevice {
    PhysicalDeviceIdentity identity;
    TransportLocator route;
    BackendIdentity backend;
    bool operator==(const DiscoveredDevice&) const = default;
    void validate() const;
};
// Every route of exactly one physical device, in discovery order. The same identity twice
// within one enumeration (backend+producer) is ambiguous. No index or next-device fallback.
QVector<int> resolveRoutes(const PhysicalDeviceIdentity&,const QVector<DiscoveredDevice>&);
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
    // Present only for industrial cameras (schema 2); device is then an inert gentl/unavailable
    // binding that carries the revision. Absent keeps the exact legacy schema 1 bytes.
    std::optional<PhysicalDeviceIdentity> physical{};
    bool operator==(const MeasurementCamera&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static MeasurementCamera fromJson(const QJsonObject&);
    // Explicit remap: rename keeps the device revision; device or physical change increments it.
    MeasurementCamera rebound(const MeasurementCamera&) const;
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
