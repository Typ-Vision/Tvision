#include "RobotActualPose.hpp"

#include <QSet>

#include <algorithm>
#include <cmath>

namespace visionflow {

namespace {

[[noreturn]] void rejected(RobotPoseRejectCode code, const QString& message) {
    throw RobotPoseRejected(code, message);
}

void requireBindingValid(const FrameRevisionBinding& binding) {
    try {
        binding.frame.validate();
    } catch (const std::exception&) {
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Invalid domain frame binding"));
    }
    if (binding.wireFrameUuid.isNull() || binding.wireRevisionUuid.isNull())
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Wire frame and revision UUIDs are required"));
    if (binding.wireFrameUuid.toString(QUuid::WithoutBraces) != binding.frame.frameUuid)
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Wire frame identity must equal the domain frame UUID"));
}

Rotation3D multiply(Rotation3D a, Rotation3D b) {
    return Rotation3D{a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
                      a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w}
        .normalized();
}

Rotation3D axisAngle(char axis, double radians) {
    const double half = radians / 2.0;
    const double c = std::cos(half), s = std::sin(half);
    return Rotation3D{c, axis == 'X' ? s : 0.0, axis == 'Y' ? s : 0.0,
                      axis == 'Z' ? s : 0.0}
        .normalized();
}

std::array<char, 3> axes(EulerAxisOrder order) {
    switch (order) {
    case EulerAxisOrder::XYZ:
        return {'X', 'Y', 'Z'};
    case EulerAxisOrder::XZY:
        return {'X', 'Z', 'Y'};
    case EulerAxisOrder::YXZ:
        return {'Y', 'X', 'Z'};
    case EulerAxisOrder::YZX:
        return {'Y', 'Z', 'X'};
    case EulerAxisOrder::ZXY:
        return {'Z', 'X', 'Y'};
    case EulerAxisOrder::ZYX:
        return {'Z', 'Y', 'X'};
    }
    rejected(RobotPoseRejectCode::InvalidContract, QStringLiteral("Unknown Euler axis order"));
}

void requireFrame(const FrameRevisionMap& frames, const FrameRef& actual,
                  const FrameRef& expected) {
    if (actual == expected) return;
    if (!frames.containsFrameUuid(actual.frameUuid))
        rejected(RobotPoseRejectCode::FrameBindingMissing,
                 QStringLiteral("Frame is not registered in the installation map"));
    if (actual.frameUuid == expected.frameUuid && actual.revision != expected.revision)
        rejected(RobotPoseRejectCode::FrameRevisionMismatch,
                 QStringLiteral("Exact frame revision is required"));
    rejected(RobotPoseRejectCode::FrameIdentityMismatch,
             QStringLiteral("Unexpected frame identity: %1@%2 expected %3@%4")
                 .arg(actual.frameUuid)
                 .arg(actual.revision)
                 .arg(expected.frameUuid)
                 .arg(expected.revision));
}

} // namespace

QString robotPoseRejectCodeName(RobotPoseRejectCode code) {
    switch (code) {
    case RobotPoseRejectCode::InvalidContract:
        return QStringLiteral("INVALID_CONTRACT");
    case RobotPoseRejectCode::CommandedPoseRejected:
        return QStringLiteral("COMMANDED_POSE_REJECTED");
    case RobotPoseRejectCode::PoseSourceRejected:
        return QStringLiteral("POSE_SOURCE_REJECTED");
    case RobotPoseRejectCode::FrameBindingMissing:
        return QStringLiteral("FRAME_BINDING_MISSING");
    case RobotPoseRejectCode::FrameIdentityMismatch:
        return QStringLiteral("FRAME_IDENTITY_MISMATCH");
    case RobotPoseRejectCode::FrameRevisionMismatch:
        return QStringLiteral("FRAME_REVISION_MISMATCH");
    case RobotPoseRejectCode::TcpOffsetRequired:
        return QStringLiteral("TCP_OFFSET_REQUIRED");
    case RobotPoseRejectCode::TcpOffsetUnexpected:
        return QStringLiteral("TCP_OFFSET_UNEXPECTED");
    case RobotPoseRejectCode::TcpOffsetRevisionMismatch:
        return QStringLiteral("TCP_OFFSET_REVISION_MISMATCH");
    case RobotPoseRejectCode::TcpOffsetDirectionInvalid:
        return QStringLiteral("TCP_OFFSET_DIRECTION_INVALID");
    case RobotPoseRejectCode::UnsupportedRotationEncoding:
        return QStringLiteral("UNSUPPORTED_ROTATION_ENCODING");
    case RobotPoseRejectCode::EulerConventionRequired:
        return QStringLiteral("EULER_CONVENTION_REQUIRED");
    case RobotPoseRejectCode::EulerConventionMismatch:
        return QStringLiteral("EULER_CONVENTION_MISMATCH");
    case RobotPoseRejectCode::EulerHandednessUnsupported:
        return QStringLiteral("EULER_HANDEDNESS_UNSUPPORTED");
    case RobotPoseRejectCode::EulerDirectionUnsupported:
        return QStringLiteral("EULER_DIRECTION_UNSUPPORTED");
    case RobotPoseRejectCode::AmbiguousRotation:
        return QStringLiteral("AMBIGUOUS_ROTATION");
    case RobotPoseRejectCode::SyntheticCorrectionRejected:
        return QStringLiteral("SYNTHETIC_CORRECTION_REJECTED");
    case RobotPoseRejectCode::InvalidPose:
        return QStringLiteral("INVALID_POSE");
    }
    return QStringLiteral("INVALID_CONTRACT");
}

RobotPoseRejected::RobotPoseRejected(RobotPoseRejectCode code, const QString& message)
    : std::runtime_error(message.toStdString()), code_(code) {}

QString genericQuaternionConventionId() {
    // W4 convention1: right-handed, column vectors, millimetres, quaternion-wxyz.
    return QStringLiteral("generic-quaternion-wxyz-mm-v1");
}

FrameRevisionMap::FrameRevisionMap(std::vector<FrameRevisionBinding> bindings) {
    for (auto& binding : bindings) add(std::move(binding));
}

void FrameRevisionMap::add(FrameRevisionBinding binding) {
    requireBindingValid(binding);
    for (const auto& existing : bindings_) {
        if (existing.frame == binding.frame || existing.wireRevisionUuid == binding.wireRevisionUuid)
            rejected(RobotPoseRejectCode::InvalidContract,
                     QStringLiteral("Duplicate frame/revision binding"));
    }
    bindings_.push_back(std::move(binding));
}

const FrameRevisionBinding& FrameRevisionMap::require(const FrameRef& frame) const {
    const auto found = std::find_if(bindings_.begin(), bindings_.end(),
                                    [&](const FrameRevisionBinding& binding) {
                                        return binding.frame == frame;
                                    });
    if (found == bindings_.end())
        rejected(RobotPoseRejectCode::FrameBindingMissing,
                 QStringLiteral("Exact domain frame/revision binding is missing: %1@%2")
                     .arg(frame.frameUuid)
                     .arg(frame.revision));
    return *found;
}

const FrameRevisionBinding& FrameRevisionMap::requireWire(const QUuid& wireFrameUuid,
                                                          const QUuid& wireRevisionUuid) const {
    const auto found = std::find_if(bindings_.begin(), bindings_.end(),
                                    [&](const FrameRevisionBinding& binding) {
                                        return binding.wireFrameUuid == wireFrameUuid &&
                                               binding.wireRevisionUuid == wireRevisionUuid;
                                    });
    if (found == bindings_.end())
        rejected(RobotPoseRejectCode::FrameBindingMissing,
                 QStringLiteral("Exact wire frame/revision binding is missing"));
    return *found;
}

bool FrameRevisionMap::containsFrameUuid(const QString& frameUuid) const {
    return std::any_of(bindings_.begin(), bindings_.end(),
                       [&](const FrameRevisionBinding& binding) {
                           return binding.frame.frameUuid == frameUuid;
                       });
}

void EulerConvention::validate() const {
    if (conventionId.trimmed().isEmpty() || revision <= 0 || revisionUuid.isNull())
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Explicit Euler convention identity and revision are required"));
    if (static_cast<int>(axisOrder) < static_cast<int>(EulerAxisOrder::XYZ) ||
        static_cast<int>(axisOrder) > static_cast<int>(EulerAxisOrder::ZYX))
        rejected(RobotPoseRejectCode::InvalidContract, QStringLiteral("Unknown Euler axis order"));
    if (static_cast<int>(unit) < static_cast<int>(EulerAngleUnit::Radian) ||
        static_cast<int>(unit) > static_cast<int>(EulerAngleUnit::Degree))
        rejected(RobotPoseRejectCode::InvalidContract, QStringLiteral("Unknown Euler angle unit"));
    if (static_cast<int>(rotationType) < static_cast<int>(EulerRotationType::Intrinsic) ||
        static_cast<int>(rotationType) > static_cast<int>(EulerRotationType::Extrinsic))
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Unknown Euler rotation type"));
    if (handedness != RotationHandedness::RightHanded)
        rejected(RobotPoseRejectCode::EulerHandednessUnsupported,
                 QStringLiteral("Only explicit right-handed Euler conventions are supported"));
    if (direction != EulerDirection::ControllerToInternal)
        rejected(RobotPoseRejectCode::EulerDirectionUnsupported,
                 QStringLiteral("Only controller-to-internal Euler projection is supported"));
}

Rotation3D rotationFromEuler(const EulerConvention& convention, const EulerReading& reading) {
    convention.validate();
    if (reading.conventionId != convention.conventionId)
        rejected(RobotPoseRejectCode::EulerConventionMismatch,
                 QStringLiteral("Euler reading does not carry the declared convention identity"));
    for (double angle : {reading.a, reading.b, reading.c}) {
        if (!std::isfinite(angle))
            rejected(RobotPoseRejectCode::InvalidPose,
                     QStringLiteral("Nonfinite Euler angle"));
    }
    const double scale = convention.unit == EulerAngleUnit::Degree ? std::acos(-1.0) / 180.0 : 1.0;
    const auto order = axes(convention.axisOrder);
    const Rotation3D first = axisAngle(order[0], reading.a * scale);
    const Rotation3D second = axisAngle(order[1], reading.b * scale);
    const Rotation3D third = axisAngle(order[2], reading.c * scale);
    if (convention.rotationType == EulerRotationType::Intrinsic)
        return multiply(multiply(first, second), third);
    // Extrinsic: fixed axes apply first-listed first, so the matrix product is
    // reversed relative to the intrinsic body-axis sequence.
    return multiply(multiply(third, second), first);
}

void RobotTcpOffset::validate() const {
    if (revision <= 0 || revisionUuid.isNull())
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Exact TCP offset revision identity is required"));
    try {
        flangeTcp.validate();
    } catch (const RobotPoseRejected&) {
        throw;
    } catch (const std::exception&) {
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Invalid flange_T_tcp offset transform"));
    }
}

void RobotPoseContract::validate() const {
    requireBindingValid(baseFrame);
    requireBindingValid(flangeFrame);
    if (baseFrame.frame == flangeFrame.frame)
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Base and flange frames must be distinct"));
    if (rotationConventionId.trimmed().isEmpty())
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Rotation convention identity is required"));
    if (adapterRevision <= 0 || adapterRevisionUuid.isNull())
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("Exact adapter revision identity is required"));
    if (euler) {
        euler->validate();
        if (euler->conventionId != rotationConventionId)
            rejected(RobotPoseRejectCode::EulerConventionMismatch,
                     QStringLiteral("Contract rotation convention and Euler convention differ"));
    }
    if (tcpOffsetRevision.has_value() != tcpOffsetRevisionUuid.has_value())
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("TCP offset revision integer and UUID are atomic"));
    if (tcpOffsetRevision && *tcpOffsetRevision <= 0)
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("TCP offset revision must be positive"));
    if (tcpOffsetRevisionUuid && tcpOffsetRevisionUuid->isNull())
        rejected(RobotPoseRejectCode::InvalidContract,
                 QStringLiteral("TCP offset revision UUID must be nonzero"));
}

RobotActualPoseAdapter::RobotActualPoseAdapter(RobotPoseContract contract, FrameRevisionMap frames)
    : contract_(std::move(contract)), frames_(std::move(frames)) {
    contract_.validate();
    const auto& base = frames_.require(contract_.baseFrame.frame);
    if (!(base == contract_.baseFrame))
        rejected(RobotPoseRejectCode::FrameIdentityMismatch,
                 QStringLiteral("Contract base frame does not match the frame map"));
    const auto& flange = frames_.require(contract_.flangeFrame.frame);
    if (!(flange == contract_.flangeFrame))
        rejected(RobotPoseRejectCode::FrameIdentityMismatch,
                 QStringLiteral("Contract flange frame does not match the frame map"));
}

RobotActualPose RobotActualPoseAdapter::adapt(const RobotActualPoseInput& input) const {
    if (input.correction)
        rejected(RobotPoseRejectCode::SyntheticCorrectionRejected,
                 QStringLiteral("Hidden/synthetic pose corrections are outside the contract"));
    if (input.source == RobotPoseSourceKind::Commanded)
        rejected(RobotPoseRejectCode::CommandedPoseRejected,
                 QStringLiteral("A commanded pose is not an actual pose"));
    if (input.source != RobotPoseSourceKind::Actual)
        rejected(RobotPoseRejectCode::PoseSourceRejected,
                 QStringLiteral("Only an explicit actual pose source is accepted"));

    SpatialTransform working = input.pose;
    QString appliedConvention = contract_.rotationConventionId;
    if (contract_.euler) {
        if (!input.euler)
            rejected(RobotPoseRejectCode::EulerConventionRequired,
                     QStringLiteral("Contract declares an Euler convention but no reading"));
        if (!working.rotation.equivalent(Rotation3D{}))
            rejected(RobotPoseRejectCode::AmbiguousRotation,
                     QStringLiteral("Euler reading cannot be combined with a quaternion"));
        working.rotation = rotationFromEuler(*contract_.euler, *input.euler);
        appliedConvention = contract_.euler->conventionId;
    } else if (input.euler) {
        rejected(RobotPoseRejectCode::EulerConventionRequired,
                 QStringLiteral("Euler reading requires an explicit adapter convention"));
    }
    try {
        working.validate();
    } catch (const RobotPoseRejected&) {
        throw;
    } catch (const std::exception&) {
        rejected(RobotPoseRejectCode::InvalidPose,
                 QStringLiteral("Invalid actual pose translation/frame/quaternion"));
    }
    // The accepted representation is normalized and canonical (q == -q).
    working.rotation = working.rotation.normalized();

    SpatialTransform result = working;
    if (input.payload == RobotPosePayloadKind::BaseFlange) {
        if (input.tcpOffset)
            rejected(RobotPoseRejectCode::TcpOffsetUnexpected,
                     QStringLiteral("TCP offset is ambiguous for a direct base_T_flange pose"));
        requireFrame(frames_, working.source, contract_.flangeFrame.frame);
        requireFrame(frames_, working.target, contract_.baseFrame.frame);
        result = working;
    } else if (input.payload == RobotPosePayloadKind::BaseTcp) {
        if (!contract_.tcpOffsetRevision || !contract_.tcpOffsetRevisionUuid)
            rejected(RobotPoseRejectCode::TcpOffsetRevisionMismatch,
                     QStringLiteral("Contract has no frozen TCP offset revision"));
        if (!input.tcpOffset)
            rejected(RobotPoseRejectCode::TcpOffsetRequired,
                     QStringLiteral("base_T_tcp requires flange_T_tcp with an exact revision"));
        input.tcpOffset->validate();
        if (input.tcpOffset->revision != *contract_.tcpOffsetRevision ||
            input.tcpOffset->revisionUuid != *contract_.tcpOffsetRevisionUuid)
            rejected(RobotPoseRejectCode::TcpOffsetRevisionMismatch,
                     QStringLiteral("TCP offset revision does not match the frozen contract"));
        const auto& offset = input.tcpOffset->flangeTcp;
        if (offset.target != contract_.flangeFrame.frame)
            rejected(RobotPoseRejectCode::TcpOffsetDirectionInvalid,
                     QStringLiteral("Offset target must be the contract flange frame"));
        requireFrame(frames_, working.source, offset.source);
        requireFrame(frames_, working.target, contract_.baseFrame.frame);
        // base_T_flange = base_T_tcp * inverse(flange_T_tcp).
        // then() composes this T_B_C with next T_A_B into T_A_C.
        result = offset.inverse().then(working);
    } else {
        rejected(RobotPoseRejectCode::InvalidPose, QStringLiteral("Unknown pose payload kind"));
    }

    RobotActualPose actual;
    actual.baseTFlange = result;
    actual.sourcePayload = input.payload;
    actual.appliedRotationConventionId = appliedConvention;
    actual.adapterRevision = contract_.adapterRevision;
    actual.adapterRevisionUuid = contract_.adapterRevisionUuid;
    return actual;
}

} // namespace visionflow
