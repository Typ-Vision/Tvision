#pragma once

#include "SpatialTransform.hpp"

#include <QUuid>

#include <array>
#include <optional>
#include <stdexcept>
#include <vector>

namespace visionflow {

// COMM4-19 robot actual-pose contract. S7 is authoritative:
// T_A_B maps B coordinates into A, right-handed column vectors, Float64 mm and
// quaternion w,x,y,z. A commanded pose is never promoted to actual here, and
// generic quaternion support is not a controller-compatibility claim.
enum class RobotPoseSourceKind { Commanded, Actual };
enum class RobotPosePayloadKind { BaseFlange, BaseTcp };

enum class RobotPoseRejectCode {
    InvalidContract,
    CommandedPoseRejected,
    PoseSourceRejected,
    FrameBindingMissing,
    FrameIdentityMismatch,
    FrameRevisionMismatch,
    TcpOffsetRequired,
    TcpOffsetUnexpected,
    TcpOffsetRevisionMismatch,
    TcpOffsetDirectionInvalid,
    UnsupportedRotationEncoding,
    EulerConventionRequired,
    EulerConventionMismatch,
    EulerHandednessUnsupported,
    EulerDirectionUnsupported,
    AmbiguousRotation,
    SyntheticCorrectionRejected,
    InvalidPose
};

QString robotPoseRejectCodeName(RobotPoseRejectCode code);

class RobotPoseRejected : public std::runtime_error {
public:
    RobotPoseRejected(RobotPoseRejectCode code, const QString& message);
    RobotPoseRejectCode code() const noexcept { return code_; }

private:
    RobotPoseRejectCode code_;
};

QString genericQuaternionConventionId();

// Explicit bridge between the domain FrameRef (canonical frame UUID plus a
// positive integer revision) and the two immutable UUIDs carried by W4.
// wireFrameUuid MUST equal frame.frameUuid; renaming or deriving one identity
// from the other is rejected. wireRevisionUuid is supplied as an opaque
// catalogue identity and is never a cast of the domain revision integer.
struct FrameRevisionBinding {
    FrameRef frame;
    QUuid wireFrameUuid;
    QUuid wireRevisionUuid;
    bool operator==(const FrameRevisionBinding&) const = default;
};

// Installation-scoped lookup. Exact FrameRef equality (including owner IDs and
// revision) is required; a UUID-only match is not sufficient.
class FrameRevisionMap {
public:
    FrameRevisionMap() = default;
    explicit FrameRevisionMap(std::vector<FrameRevisionBinding> bindings);

    void add(FrameRevisionBinding binding);
    const FrameRevisionBinding& require(const FrameRef& frame) const;
    const FrameRevisionBinding& requireWire(const QUuid& wireFrameUuid,
                                            const QUuid& wireRevisionUuid) const;
    bool containsFrameUuid(const QString& frameUuid) const;
    const std::vector<FrameRevisionBinding>& bindings() const { return bindings_; }

private:
    std::vector<FrameRevisionBinding> bindings_;
};

enum class EulerAxisOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };
enum class EulerAngleUnit { Radian, Degree };
enum class EulerRotationType { Intrinsic, Extrinsic };
enum class RotationHandedness { RightHanded, LeftHanded };
// The adapter converts controller readings into the internal representation.
// The inverse projection is a separate, unimplemented controller concern.
enum class EulerDirection { ControllerToInternal, InternalToController };

// Every field is part of the convention identity. No brand implies a convention
// and no default order/unit/type is inferred from angle names.
struct EulerConvention {
    QString conventionId;
    EulerAxisOrder axisOrder = EulerAxisOrder::XYZ;
    EulerAngleUnit unit = EulerAngleUnit::Radian;
    EulerRotationType rotationType = EulerRotationType::Intrinsic;
    RotationHandedness handedness = RotationHandedness::RightHanded;
    EulerDirection direction = EulerDirection::ControllerToInternal;
    int revision = 0;
    QUuid revisionUuid;
    void validate() const;
    bool operator==(const EulerConvention&) const = default;
};

struct EulerReading {
    QString conventionId;
    double a = 0, b = 0, c = 0;
    bool operator==(const EulerReading&) const = default;
};

// Converts one explicit controller Euler reading to the internal quaternion.
// Angle a applies to the first listed axis, b to the second and c to the third.
// Intrinsic: R = q(axis1,a) * q(axis2,b) * q(axis3,c).
// Extrinsic: fixed axes, R = q(axis3,c) * q(axis2,b) * q(axis1,a).
Rotation3D rotationFromEuler(const EulerConvention& convention, const EulerReading& reading);

// flange_T_tcp: source is the TCP frame, target is the flange frame.
// revision/revisionUuid identify the frozen offset record; both must match the
// contract before any composition is allowed.
struct RobotTcpOffset {
    SpatialTransform flangeTcp;
    int revision = 0;
    QUuid revisionUuid;
    void validate() const;
    bool operator==(const RobotTcpOffset&) const = default;
};

// Source/record identity for one accepted actual-pose stream. adapterRevision
// is the W6 adapterRevision catalogue identity, not a controller firmware claim.
struct RobotPoseContract {
    FrameRevisionBinding baseFrame;
    FrameRevisionBinding flangeFrame;
    QString rotationConventionId;
    int adapterRevision = 0;
    QUuid adapterRevisionUuid;
    std::optional<EulerConvention> euler;
    std::optional<int> tcpOffsetRevision;
    std::optional<QUuid> tcpOffsetRevisionUuid;
    void validate() const;
};

struct RobotActualPoseInput {
    // Safe default: missing source metadata must not become an actual sample.
    RobotPoseSourceKind source = RobotPoseSourceKind::Commanded;
    RobotPosePayloadKind payload = RobotPosePayloadKind::BaseFlange;
    // Frames and translation are always used. rotation is the quaternion
    // representation unless an explicit euler reading is supplied.
    SpatialTransform pose;
    std::optional<EulerReading> euler;
    std::optional<RobotTcpOffset> tcpOffset;
    // Any hidden correction, including a pure synthetic Z rotation used to make
    // a fixture agree, is outside the contract and is rejected.
    std::optional<SpatialTransform> correction;
};

struct RobotActualPose {
    SpatialTransform baseTFlange;
    RobotPosePayloadKind sourcePayload = RobotPosePayloadKind::BaseFlange;
    QString appliedRotationConventionId;
    int adapterRevision = 0;
    QUuid adapterRevisionUuid;
    bool operator==(const RobotActualPose&) const = default;
};

// Pure adapter: no robot client, network, SDK, capture, solver or motion code.
class RobotActualPoseAdapter {
public:
    RobotActualPoseAdapter(RobotPoseContract contract, FrameRevisionMap frames);
    RobotActualPose adapt(const RobotActualPoseInput& input) const;
    const RobotPoseContract& contract() const { return contract_; }
    const FrameRevisionMap& frames() const { return frames_; }

private:
    RobotPoseContract contract_;
    FrameRevisionMap frames_;
};

} // namespace visionflow
