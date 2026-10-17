#pragma once
#include <QJsonObject>
#include <QPoint>
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
struct GeometryMirror {
    bool x=false, y=false;
    bool operator==(const GeometryMirror&) const = default;
};
struct GeometryDescriptor {
    QSize resolution;
    std::optional<QRect> sensorRoi;
    std::optional<QSize> binning;
    // Adapter must explicitly establish that no critical geometry is unknown.
    bool complete=false;
    // ICM-03b industrial geometry (schema 2): decimation and reverse are stated together with
    // sensorRoi/binning. Absent keeps the exact schema 1 bytes and geometry hash.
    std::optional<QSize> decimation{};
    std::optional<GeometryMirror> reverse{};
    bool critical() const {return decimation.has_value();}
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
// ICM-03b acquisition profile. Geometry belongs to the setup; the profile carries the
// non-geometric device settings. Requested and applied (readback) use the same value type.
struct ProfileId { QString value; bool operator==(const ProfileId&) const = default; void validate() const; };
enum class AutoMode { Off, Once, Continuous };
struct TriggerSettings {
    bool enabled=false;
    QString selector, source; // SFNC names; empty exactly while disabled
    bool operator==(const TriggerSettings&) const = default;
};
struct AcquisitionSettings {
    QString pixelFormat;
    TriggerSettings trigger;
    AutoMode exposureAuto=AutoMode::Off;
    double exposureTimeUs=0;
    AutoMode gainAuto=AutoMode::Off;
    double gain=0;
    QString illumination; // operator light declaration; may be empty
    bool operator==(const AcquisitionSettings&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static AcquisitionSettings fromJson(const QJsonObject&);
};
struct AcquisitionProfile {
    ProfileId id;
    CameraId camera;
    SetupId setup;
    QString name;
    int revision=1;
    AcquisitionSettings requested;
    bool operator==(const AcquisitionProfile&) const = default;
    // First product profile: Mono8 and manual exposure/gain only.
    void validate() const;
    QJsonObject toJson() const;
    static AcquisitionProfile fromJson(const QJsonObject&);
    // Rename keeps revision; any acquisition-relevant change increments it.
    AcquisitionProfile revised(const AcquisitionProfile&) const;
    // Portable identity of setup geometry + requested settings; name, ID and revision excluded.
    QString acquisitionId(const MeasurementSetupDescriptor&) const;
};
// Applied geometry as read back. NotApplicable = node absent and proven not applicable by
// standard/model evidence (identity value); Unknown is never guessed as 0/1 and never usable.
enum class GeometryEvidence { Reported, NotApplicable, Unknown };
struct GeometryReadback {
    QSize resolution;
    QPoint offset;
    GeometryEvidence offsetEvidence=GeometryEvidence::Unknown;
    QSize binning{1,1};
    GeometryEvidence binningEvidence=GeometryEvidence::Unknown;
    QSize decimation{1,1};
    GeometryEvidence decimationEvidence=GeometryEvidence::Unknown;
    GeometryMirror reverse;
    GeometryEvidence reverseEvidence=GeometryEvidence::Unknown;
    bool operator==(const GeometryReadback&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static GeometryReadback fromJson(const QJsonObject&);
    // Explicit schema 2 descriptor; any Unknown critical feature is rejected.
    GeometryDescriptor descriptor() const;
};
struct AcquisitionReadback {
    GeometryReadback geometry;
    AcquisitionSettings settings;
    bool operator==(const AcquisitionReadback&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static AcquisitionReadback fromJson(const QJsonObject&);
};
// Contract section 3 invalidation table as the single rule; each flag is one table row.
// Names and panel layout never take part.
struct AcquisitionBasis {
    MeasurementSetupDescriptor setup;
    std::optional<AcquisitionProfile> profile{};
    QString backend{}; // local BackendIdentity::fingerprint(); empty while unprepared
};
struct AcquisitionChange {
    bool setup=false;       // camera/device mapping, lens/focus, working plane
    bool geometry=false;    // resolution, offset/ROI, binning, decimation, reverse
    bool pixelFormat=false; // no equivalence proof exists in the first version
    bool acquisition=false; // exposure/gain/auto, trigger, illumination
    bool backend=false;     // producer/SDK: capability and preparation re-check only
    bool operator==(const AcquisitionChange&) const = default;
    bool calibrationCompatible() const {return !setup && !geometry && !pixelFormat;}
    bool cacheCompatible() const {return calibrationCompatible() && !acquisition;}
    bool preparationCompatible() const {return cacheCompatible() && !backend;}
};
AcquisitionChange acquisitionChange(const AcquisitionBasis& before,const AcquisitionBasis& after);
// Applied readback must reproduce the setup geometry and requested settings; returns the
// acquisition identity. Floating representation tolerance is ICM-04b readback evidence.
QString verifyApplied(const MeasurementSetupDescriptor&,const AcquisitionProfile&,const AcquisitionReadback&);
// Standalone value codec. Persistence, selections and activation belong to KMC-2/4.
struct MeasurementSetupContract {
    QVector<MeasurementCamera> cameras;
    QVector<MeasurementSetupDescriptor> setups;
    // Industrial cameras only; empty keeps the exact schema 1 contract bytes.
    QVector<AcquisitionProfile> profiles{};
    void validate(int capacity=128) const;
    QJsonObject toJson(int capacity=128) const;
    static MeasurementSetupContract fromJson(const QJsonObject&,int capacity=128);
};
enum class SourceOrigin { CameraAcquired, SetupFile, TrialFile, LegacyRecord };
QString sourceOriginName(SourceOrigin);
SourceOrigin sourceOriginFromName(const QString&);
}
