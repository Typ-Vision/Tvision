#include "domain/RobotActualPose.hpp"

#include <QCoreApplication>
#include <QTextStream>

#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

using namespace visionflow;

namespace {

int checks = 0;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

void close(double actual, double expected, double tolerance, const char* why) {
    ++checks;
    if (!(std::isfinite(actual) && std::abs(actual - expected) <= tolerance))
        throw std::runtime_error(why);
}

QString uuidText(int value) {
    return QStringLiteral("00000000-0000-0000-0000-%1").arg(value, 12, 10, QLatin1Char('0'));
}

QUuid uuid(int value) { return QUuid(uuidText(value)); }

FrameRef frame(int value, int revision = 1) {
    return FrameRef{QString(), QStringLiteral("robot-1"), QStringLiteral("cell-1"),
                    uuidText(value), revision};
}

FrameRevisionBinding binding(const FrameRef& f, int revisionUuid) {
    return {f, QUuid(f.frameUuid), uuid(revisionUuid)};
}

RobotPoseContract baseContract() {
    RobotPoseContract contract;
    contract.baseFrame = binding(frame(1), 1001);
    contract.flangeFrame = binding(frame(2), 1002);
    contract.rotationConventionId = genericQuaternionConventionId();
    contract.adapterRevision = 4;
    contract.adapterRevisionUuid = uuid(9001);
    return contract;
}

FrameRevisionMap baseFrames() {
    return FrameRevisionMap({binding(frame(1), 1001), binding(frame(2), 1002),
                             binding(frame(3), 1003), binding(frame(4), 1004)});
}

SpatialTransform makeTransform(const FrameRef& source, const FrameRef& target,
                               std::array<double, 3> translation, Rotation3D rotation) {
    return SpatialTransform{source, target, translation, rotation};
}

EulerConvention eulerConvention(EulerAxisOrder order = EulerAxisOrder::XYZ,
                                EulerAngleUnit unit = EulerAngleUnit::Degree,
                                EulerRotationType type = EulerRotationType::Intrinsic) {
    EulerConvention convention;
    convention.conventionId = QStringLiteral("fixture-euler-convention-v1");
    convention.axisOrder = order;
    convention.unit = unit;
    convention.rotationType = type;
    convention.handedness = RotationHandedness::RightHanded;
    convention.direction = EulerDirection::ControllerToInternal;
    convention.revision = 2;
    convention.revisionUuid = uuid(2001);
    return convention;
}

using Matrix = std::array<std::array<double, 3>, 3>;

Matrix rotationMatrix(Rotation3D q) {
    q = q.normalized();
    const double w = q.w, x = q.x, y = q.y, z = q.z;
    return {{{1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)},
             {2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)},
             {2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)}}};
}

std::array<double, 3> multiply(const Matrix& m, std::array<double, 3> p) {
    return {m[0][0] * p[0] + m[0][1] * p[1] + m[0][2] * p[2],
            m[1][0] * p[0] + m[1][1] * p[1] + m[1][2] * p[2],
            m[2][0] * p[0] + m[2][1] * p[1] + m[2][2] * p[2]};
}

Matrix multiply(const Matrix& a, const Matrix& b) {
    Matrix result{};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            for (int k = 0; k < 3; ++k) result[r][c] += a[r][k] * b[k][c];
    return result;
}

Matrix transpose(const Matrix& m) {
    return {{{m[0][0], m[1][0], m[2][0]},
             {m[0][1], m[1][1], m[2][1]},
             {m[0][2], m[1][2], m[2][2]}}};
}

void checkTransform(const SpatialTransform& actual, const Matrix& expectedR,
                    std::array<double, 3> expectedT, const char* why) {
    const auto actualR = rotationMatrix(actual.rotation);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) close(actualR[r][c], expectedR[r][c], 1e-12, why);
    for (int i = 0; i < 3; ++i) close(actual.translationMm[i], expectedT[i], 1e-12, why);
}

Matrix axisRotation(char axis, double degrees) {
    const double radians = degrees * std::acos(-1.0) / 180.0;
    const double c = std::cos(radians), s = std::sin(radians);
    if (axis == 'X') return {{{1, 0, 0}, {0, c, -s}, {0, s, c}}};
    if (axis == 'Y') return {{{c, 0, s}, {0, 1, 0}, {-s, 0, c}}};
    return {{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
}

void reject(RobotPoseRejectCode code, const std::function<void()>& action, const char* why) {
    ++checks;
    try {
        action();
    } catch (const RobotPoseRejected& error) {
        if (error.code() != code) throw std::runtime_error(why);
        return;
    } catch (const std::exception&) {
        throw std::runtime_error(why);
    }
    throw std::runtime_error(why);
}

RobotActualPoseInput quaternionInput(const SpatialTransform& pose) {
    RobotActualPoseInput input;
    input.source = RobotPoseSourceKind::Actual;
    input.payload = RobotPosePayloadKind::BaseFlange;
    input.pose = pose;
    return input;
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        const auto base = frame(1), flange = frame(2), tcp = frame(3), other = frame(4);
        const double h = std::sqrt(0.5);

        // ------------------------------ frame/revision bridge
        FrameRevisionMap frames = baseFrames();
        check(frames.require(base) == binding(base, 1001), "exact domain frame lookup");
        check(frames.requireWire(uuid(1), uuid(1001)).frame == base, "exact wire lookup");
        reject(RobotPoseRejectCode::FrameBindingMissing,
               [&] { frames.require(frame(1, 2)); }, "domain revision absent from map");
        reject(RobotPoseRejectCode::FrameBindingMissing,
               [&] { frames.requireWire(uuid(1), uuid(1002)); }, "wire revision must be exact");
        reject(RobotPoseRejectCode::FrameBindingMissing,
               [&] { frames.requireWire(uuid(2), uuid(1001)); }, "wire frame/revision not interchangeable");
        {
            auto renamed = binding(base, 1001);
            renamed.wireFrameUuid = uuid(4);
            reject(RobotPoseRejectCode::InvalidContract,
                   [&] { FrameRevisionMap({renamed}); }, "wire frame identity cannot be renamed");
        }
        {
            auto missing = binding(base, 1001);
            missing.wireRevisionUuid = QUuid();
            reject(RobotPoseRejectCode::InvalidContract,
                   [&] { FrameRevisionMap({missing}); }, "null wire revision UUID rejected");
        }
        {
            auto duplicate = binding(frame(1, 1), 1001);
            reject(RobotPoseRejectCode::InvalidContract,
                   [&] { FrameRevisionMap({binding(base, 1001), duplicate}); },
                   "duplicate domain mapping rejected");
        }
        {
            auto duplicate = binding(other, 1001);
            reject(RobotPoseRejectCode::InvalidContract,
                   [&] { FrameRevisionMap({binding(base, 1001), duplicate}); },
                   "duplicate wire revision UUID rejected");
        }
        {
            auto invalid = binding(frame(1, 0), 1001);
            reject(RobotPoseRejectCode::InvalidContract,
                   [&] { FrameRevisionMap({invalid}); }, "zero domain revision rejected");
        }

        // ------------------------------ contract validation
        {
            auto contract = baseContract();
            contract.adapterRevision = 0;
            reject(RobotPoseRejectCode::InvalidContract, [&] { contract.validate(); },
                   "adapter revision required");
            contract = baseContract();
            contract.adapterRevisionUuid = QUuid();
            reject(RobotPoseRejectCode::InvalidContract, [&] { contract.validate(); },
                   "adapter revision UUID required");
            contract = baseContract();
            contract.rotationConventionId.clear();
            reject(RobotPoseRejectCode::InvalidContract, [&] { contract.validate(); },
                   "rotation convention identity required");
            contract = baseContract();
            contract.flangeFrame = contract.baseFrame;
            reject(RobotPoseRejectCode::InvalidContract, [&] { contract.validate(); },
                   "base and flange frames must differ");
            contract = baseContract();
            contract.tcpOffsetRevision = 3;
            reject(RobotPoseRejectCode::InvalidContract, [&] { contract.validate(); },
                   "TCP offset revision pair is atomic");
        }

        // ------------------------------ direct base_T_flange
        {
            const Rotation3D q{h, 0, 0, h}; // Rz(90)
            const SpatialTransform direct = makeTransform(flange, base, {12, -4, 30}, q);
            RobotActualPoseAdapter adapter(baseContract(), frames);
            const auto result = adapter.adapt(quaternionInput(direct));
            check(result.baseTFlange.source == flange && result.baseTFlange.target == base,
                  "direct pose keeps S7 direction");
            check(result.sourcePayload == RobotPosePayloadKind::BaseFlange,
                  "direct payload recorded");
            check(result.appliedRotationConventionId == genericQuaternionConventionId(),
                  "generic convention recorded");
            check(result.adapterRevision == 4 && result.adapterRevisionUuid == uuid(9001),
                  "adapter revision preserved");
            for (int i = 0; i < 3; ++i) close(result.baseTFlange.translationMm[i],
                                             direct.translationMm[i], 0, "direct translation exact");
            check(result.baseTFlange.rotation.equivalent(q), "direct rotation preserved");

            // q and -q are the same rotation and encode identically.
            auto negated = direct;
            negated.rotation = Rotation3D{-h, 0, 0, -h};
            const auto negatedResult = adapter.adapt(quaternionInput(negated));
            check(negatedResult.baseTFlange.rotation.toJson() ==
                      result.baseTFlange.rotation.toJson(),
                  "q/-q canonicalized identically");
            check(negatedResult.baseTFlange.translationMm == result.baseTFlange.translationMm,
                  "q/-q translation unchanged");

            // Norm tolerance boundary: 1e-6 accepted, larger rejected.
            auto drift = direct;
            drift.rotation = Rotation3D{1.0 + 0.5e-6, 0, 0, 0};
            const auto driftResult = adapter.adapt(quaternionInput(drift));
            close(driftResult.baseTFlange.rotation.w, 1.0, 1e-12, "in-tolerance drift normalized");
            drift.rotation = Rotation3D{1.0 + 1e-6, 0, 0, 0};
            check(adapter.adapt(quaternionInput(drift)).baseTFlange.rotation.w > 0.999999,
                  "exact tolerance boundary accepted");
            drift.rotation = Rotation3D{1.0 + 2e-6, 0, 0, 0};
            reject(RobotPoseRejectCode::InvalidPose,
                   [&] { adapter.adapt(quaternionInput(drift)); }, "out-of-tolerance norm rejected");
            drift.rotation = Rotation3D{0, 0, 0, 0};
            reject(RobotPoseRejectCode::InvalidPose,
                   [&] { adapter.adapt(quaternionInput(drift)); }, "zero norm rejected");
            for (int i = 0; i < 4; ++i) {
                Rotation3D bad;
                auto* component = i == 0 ? &bad.w : i == 1 ? &bad.x : i == 2 ? &bad.y : &bad.z;
                *component = std::numeric_limits<double>::quiet_NaN();
                drift.rotation = bad;
                reject(RobotPoseRejectCode::InvalidPose,
                       [&] { adapter.adapt(quaternionInput(drift)); }, "NaN quaternion rejected");
                *component = std::numeric_limits<double>::infinity();
                drift.rotation = bad;
                reject(RobotPoseRejectCode::InvalidPose,
                       [&] { adapter.adapt(quaternionInput(drift)); }, "Inf quaternion rejected");
            }
            drift = direct;
            drift.translationMm[1] = std::numeric_limits<double>::infinity();
            reject(RobotPoseRejectCode::InvalidPose,
                   [&] { adapter.adapt(quaternionInput(drift)); }, "nonfinite translation rejected");
        }

        // ------------------------------ noncommuting golden + TCP composition
        {
            const SpatialTransform flangeTcp =
                makeTransform(tcp, flange, {10, 0, 0}, Rotation3D{h, 0, 0, h}); // T_flange_tcp
            const SpatialTransform baseTcp =
                makeTransform(tcp, base, {100, 200, 300}, Rotation3D{h, h, 0, 0}); // T_base_tcp
            RobotTcpOffset offset{flangeTcp, 3, uuid(3001)};

            auto contract = baseContract();
            contract.tcpOffsetRevision = 3;
            contract.tcpOffsetRevisionUuid = uuid(3001);
            RobotActualPoseAdapter adapter(contract, frames);

            RobotActualPoseInput input = quaternionInput(baseTcp);
            input.payload = RobotPosePayloadKind::BaseTcp;
            input.tcpOffset = offset;
            const auto result = adapter.adapt(input);

            // Independent matrix oracle: base_T_flange = base_T_tcp * inv(flange_T_tcp).
            const Matrix rBase = rotationMatrix(baseTcp.rotation);
            const Matrix rOffset = rotationMatrix(flangeTcp.rotation);
            const Matrix expectedR = multiply(rBase, transpose(rOffset));
            const auto rotatedOffset = multiply(transpose(rOffset), flangeTcp.translationMm);
            const std::array<double, 3> expectedT{
                rBase[0][0] * -rotatedOffset[0] + rBase[0][1] * -rotatedOffset[1] +
                        rBase[0][2] * -rotatedOffset[2] + baseTcp.translationMm[0],
                rBase[1][0] * -rotatedOffset[0] + rBase[1][1] * -rotatedOffset[1] +
                        rBase[1][2] * -rotatedOffset[2] + baseTcp.translationMm[1],
                rBase[2][0] * -rotatedOffset[0] + rBase[2][1] * -rotatedOffset[1] +
                        rBase[2][2] * -rotatedOffset[2] + baseTcp.translationMm[2]};
            checkTransform(result.baseTFlange, expectedR, expectedT,
                           "independent noncommuting TCP golden vector");
            // Frozen numeric fixture for this exact input.
            close(result.baseTFlange.translationMm[0], 100, 1e-12, "golden tx");
            close(result.baseTFlange.translationMm[1], 200, 1e-12, "golden ty");
            close(result.baseTFlange.translationMm[2], 310, 1e-12, "golden tz");
            check(result.baseTFlange.rotation.equivalent(Rotation3D{0.5, 0.5, 0.5, -0.5}),
                  "golden Rx90*Rz-90 quaternion");
            check(result.sourcePayload == RobotPosePayloadKind::BaseTcp, "TCP payload recorded");
            check(result.baseTFlange.source == flange && result.baseTFlange.target == base,
                  "TCP composition frame direction");

            // Applying the offset forward is a different numeric result.
            const Matrix wrongR = multiply(rBase, rOffset);
            const auto wrongT0 = multiply(rBase, flangeTcp.translationMm);
            const std::array<double, 3> wrongT{wrongT0[0] + baseTcp.translationMm[0],
                                               wrongT0[1] + baseTcp.translationMm[1],
                                               wrongT0[2] + baseTcp.translationMm[2]};
            bool differs = false;
            const auto acceptedR = rotationMatrix(result.baseTFlange.rotation);
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c)
                    differs = differs || std::abs(acceptedR[r][c] - wrongR[r][c]) > 1e-9;
            differs = differs || result.baseTFlange.translationMm != wrongT;
            check(differs, "forward offset composition is not the accepted result");

            // The accepted flange result composes back to the original TCP pose.
            const auto back = flangeTcp.then(result.baseTFlange);
            check(back.source == tcp && back.target == base,
                  "accepted flange result composes back to TCP direction");
            for (int i = 0; i < 3; ++i)
                close(back.translationMm[i], baseTcp.translationMm[i], 1e-12,
                      "inverse roundtrip translation");
            check(back.rotation.equivalent(baseTcp.rotation), "inverse roundtrip rotation");

            // Missing offset.
            auto missing = input;
            missing.tcpOffset.reset();
            reject(RobotPoseRejectCode::TcpOffsetRequired,
                   [&] { adapter.adapt(missing); }, "TCP pose without offset rejected");
            // Offset revision mismatches, both halves.
            offset.revision = 4;
            auto badRevision = input;
            badRevision.tcpOffset = offset;
            reject(RobotPoseRejectCode::TcpOffsetRevisionMismatch,
                   [&] { adapter.adapt(badRevision); }, "wrong integer offset revision rejected");
            offset.revision = 3;
            offset.revisionUuid = uuid(3002);
            badRevision.tcpOffset = offset;
            reject(RobotPoseRejectCode::TcpOffsetRevisionMismatch,
                   [&] { adapter.adapt(badRevision); }, "wrong UUID offset revision rejected");
            offset.revisionUuid = uuid(3001);
            // Reversed offset direction.
            auto reversed = offset;
            reversed.flangeTcp = flangeTcp.inverse();
            auto badDirection = input;
            badDirection.tcpOffset = reversed;
            reject(RobotPoseRejectCode::TcpOffsetDirectionInvalid,
                   [&] { adapter.adapt(badDirection); }, "reversed TCP offset rejected");
            // Offset for a direct flange pose is ambiguous and rejected.
            auto unexpected = quaternionInput(
                makeTransform(flange, base, {1, 2, 3}, Rotation3D{h, h, 0, 0}));
            unexpected.tcpOffset = offset;
            reject(RobotPoseRejectCode::TcpOffsetUnexpected,
                   [&] { adapter.adapt(unexpected); }, "unexpected TCP offset rejected");
            // Contract without frozen offset identity cannot accept TCP poses.
            RobotActualPoseAdapter noOffsetContract(baseContract(), frames);
            auto noIdentity = input;
            noIdentity.tcpOffset = offset;
            reject(RobotPoseRejectCode::TcpOffsetRevisionMismatch,
                   [&] { noOffsetContract.adapt(noIdentity); },
                   "undeclared TCP offset identity rejected");
        }

        // ------------------------------ frames, revisions, source kind
        {
            RobotActualPoseAdapter adapter(baseContract(), frames);
            const SpatialTransform direct =
                makeTransform(flange, base, {1, 2, 3}, Rotation3D{h, 0, 0, h});

            auto input = quaternionInput(direct);
            input.source = RobotPoseSourceKind::Commanded;
            reject(RobotPoseRejectCode::CommandedPoseRejected,
                   [&] { adapter.adapt(input); }, "commanded pose rejected");

            auto revision = direct;
            revision.source.revision = 2;
            reject(RobotPoseRejectCode::FrameRevisionMismatch,
                   [&] { adapter.adapt(quaternionInput(revision)); },
                   "base source revision mismatch rejected");
            auto flangeRevision = direct;
            flangeRevision.target.revision = 7;
            reject(RobotPoseRejectCode::FrameRevisionMismatch,
                   [&] { adapter.adapt(quaternionInput(flangeRevision)); },
                   "flange target revision mismatch rejected");
            auto foreign = direct;
            foreign.source = other;
            reject(RobotPoseRejectCode::FrameIdentityMismatch,
                   [&] { adapter.adapt(quaternionInput(foreign)); },
                   "different registered frame is not base");
            auto unregistered = direct;
            unregistered.target = frame(5);
            reject(RobotPoseRejectCode::FrameBindingMissing,
                   [&] { adapter.adapt(quaternionInput(unregistered)); },
                   "unregistered frame rejected");
            auto owner = direct;
            owner.target.groupId = QStringLiteral("foreign-cell");
            reject(RobotPoseRejectCode::FrameIdentityMismatch,
                   [&] { adapter.adapt(quaternionInput(owner)); },
                   "frame owner identity mismatch rejected");

            // A binding with the same UUID but a new domain revision needs a new explicit mapping.
            auto next = baseContract();
            next.baseFrame = binding(frame(1, 2), 1005);
            reject(RobotPoseRejectCode::FrameBindingMissing,
                   [&] { (void)RobotActualPoseAdapter(next, frames); },
                   "unmapped domain revision is not derived or cast");
            auto extended = frames;
            extended.add(binding(frame(1, 2), 1005));
            RobotActualPoseAdapter nextAdapter(next, extended);
            auto nextPose = direct;
            nextPose.target = frame(1, 2);
            check(nextAdapter.adapt(quaternionInput(nextPose)).baseTFlange.target == frame(1, 2),
                  "explicit new revision mapping accepted");
            reject(RobotPoseRejectCode::FrameRevisionMismatch,
                   [&] { nextAdapter.adapt(quaternionInput(direct)); },
                   "old revision is not accepted by a new-revision contract");
        }

        // ------------------------------ synthetic corrections
        {
            RobotActualPoseAdapter adapter(baseContract(), frames);
            auto input = quaternionInput(
                makeTransform(flange, base, {1, 2, 3}, Rotation3D{h, 0, 0, h}));
            input.correction = makeTransform(flange, base, {0, 0, 0}, Rotation3D{h, 0, 0, h});
            reject(RobotPoseRejectCode::SyntheticCorrectionRejected,
                   [&] { adapter.adapt(input); }, "synthetic Z correction rejected");
            input.correction = makeTransform(flange, base, {0, 0, 0}, Rotation3D{});
            reject(RobotPoseRejectCode::SyntheticCorrectionRejected,
                   [&] { adapter.adapt(input); }, "even identity correction rejected");
        }

        // ------------------------------ Euler adapter
        {
            const EulerConvention xyzIntrinsic =
                eulerConvention(EulerAxisOrder::XYZ, EulerAngleUnit::Degree,
                                EulerRotationType::Intrinsic);
            auto contract = baseContract();
            contract.rotationConventionId = xyzIntrinsic.conventionId;
            contract.euler = xyzIntrinsic;
            RobotActualPoseAdapter adapter(contract, frames);

            RobotActualPoseInput input = quaternionInput(
                makeTransform(flange, base, {5, 6, 7}, Rotation3D{}));
            input.euler = EulerReading{xyzIntrinsic.conventionId, 90, 0, 0};
            auto result = adapter.adapt(input);
            check(result.appliedRotationConventionId == xyzIntrinsic.conventionId,
                  "Euler convention recorded on output");
            check(result.baseTFlange.rotation.equivalent(Rotation3D{h, h, 0, 0}),
                  "intrinsic X90 Euler golden");
            close(result.baseTFlange.translationMm[2], 7, 0, "Euler translation preserved");

            // Independent matrix oracle for a noncommuting intrinsic XYZ sequence.
            input.euler = EulerReading{xyzIntrinsic.conventionId, 90, 90, 30};
            result = adapter.adapt(input);
            const Matrix expectedIntrinsic =
                multiply(multiply(axisRotation('X', 90), axisRotation('Y', 90)),
                         axisRotation('Z', 30));
            checkTransform(result.baseTFlange, expectedIntrinsic, {5, 6, 7},
                           "intrinsic XYZ independent matrix fixture");

            // Extrinsic XYZ is the reverse order and is not the intrinsic result.
            const EulerConvention xyzExtrinsic =
                eulerConvention(EulerAxisOrder::XYZ, EulerAngleUnit::Degree,
                                EulerRotationType::Extrinsic);
            contract.euler = xyzExtrinsic;
            contract.rotationConventionId = xyzExtrinsic.conventionId;
            RobotActualPoseAdapter extrinsicAdapter(contract, frames);
            input.euler = EulerReading{xyzExtrinsic.conventionId, 90, 90, 30};
            const auto extrinsic = extrinsicAdapter.adapt(input);
            const Matrix expectedExtrinsic =
                multiply(multiply(axisRotation('Z', 30), axisRotation('Y', 90)),
                         axisRotation('X', 90));
            checkTransform(extrinsic.baseTFlange, expectedExtrinsic, {5, 6, 7},
                           "extrinsic XYZ independent matrix fixture");
            check(!extrinsic.baseTFlange.rotation.equivalent(result.baseTFlange.rotation),
                  "intrinsic and extrinsic directions differ");

            // Same numbers with a radian convention produce a different rotation.
            const EulerConvention xyzRadians =
                eulerConvention(EulerAxisOrder::XYZ, EulerAngleUnit::Radian,
                                EulerRotationType::Intrinsic);
            contract.euler = xyzRadians;
            contract.rotationConventionId = xyzRadians.conventionId;
            RobotActualPoseAdapter radianAdapter(contract, frames);
            input.euler = EulerReading{xyzRadians.conventionId, 90, 0, 0};
            const auto radianResult = radianAdapter.adapt(input);
            check(!radianResult.baseTFlange.rotation.equivalent(Rotation3D{h, h, 0, 0}),
                  "unit is explicit; degrees are not radians");
            input.euler = EulerReading{xyzRadians.conventionId,
                                       std::acos(-1.0) / 2, 0, 0};
            check(radianAdapter.adapt(input).baseTFlange.rotation.equivalent(
                      Rotation3D{h, h, 0, 0}),
                  "explicit radian golden");

            // Convention identity must match exactly; brand names are not inferred.
            input.euler = EulerReading{QStringLiteral("FANUC-WPR"), 0, 0, 0};
            reject(RobotPoseRejectCode::EulerConventionMismatch,
                   [&] { radianAdapter.adapt(input); }, "unknown brand convention rejected");
            input.euler = EulerReading{QString(), 0, 0, 0};
            reject(RobotPoseRejectCode::EulerConventionMismatch,
                   [&] { radianAdapter.adapt(input); }, "missing convention identity rejected");

            // Euler reading without a declared convention.
            RobotActualPoseAdapter quaternionOnly(baseContract(), frames);
            input = quaternionInput(makeTransform(flange, base, {0, 0, 0}, Rotation3D{}));
            input.euler = EulerReading{genericQuaternionConventionId(), 0, 0, 0};
            reject(RobotPoseRejectCode::EulerConventionRequired,
                   [&] { quaternionOnly.adapt(input); }, "Euler without convention rejected");

            // Euler plus a non-identity quaternion is ambiguous.
            contract.euler = xyzIntrinsic;
            contract.rotationConventionId = xyzIntrinsic.conventionId;
            RobotActualPoseAdapter strict(contract, frames);
            input = quaternionInput(
                makeTransform(flange, base, {0, 0, 0}, Rotation3D{h, h, 0, 0}));
            input.euler = EulerReading{xyzIntrinsic.conventionId, 90, 0, 0};
            reject(RobotPoseRejectCode::AmbiguousRotation,
                   [&] { strict.adapt(input); }, "Euler with extra quaternion rejected");

            // Left-handed and inverse-direction conventions are unsupported.
            auto left = xyzIntrinsic;
            left.handedness = RotationHandedness::LeftHanded;
            reject(RobotPoseRejectCode::EulerHandednessUnsupported,
                   [&] { rotationFromEuler(left, {left.conventionId, 0, 0, 0}); },
                   "left-handed Euler convention rejected");
            auto inverse = xyzIntrinsic;
            inverse.direction = EulerDirection::InternalToController;
            reject(RobotPoseRejectCode::EulerDirectionUnsupported,
                   [&] { rotationFromEuler(inverse, {inverse.conventionId, 0, 0, 0}); },
                   "inverse Euler direction rejected");
            auto missingIdentity = xyzIntrinsic;
            missingIdentity.conventionId.clear();
            reject(RobotPoseRejectCode::InvalidContract,
                   [&] { missingIdentity.validate(); }, "Euler convention identity required");
            auto missingRevision = xyzIntrinsic;
            missingRevision.revision = 0;
            reject(RobotPoseRejectCode::InvalidContract,
                   [&] { missingRevision.validate(); }, "Euler convention revision required");

            // NaN/Inf Euler components are rejected.
            for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::infinity()}) {
                reject(RobotPoseRejectCode::InvalidPose,
                       [&] {
                           rotationFromEuler(xyzIntrinsic,
                                             {xyzIntrinsic.conventionId, bad, 0, 0});
                       },
                       "nonfinite Euler angle rejected");
            }
        }

        // ------------------------------ constructor contract checks
        {
            auto contract = baseContract();
            contract.flangeFrame = binding(frame(9), 9009);
            reject(RobotPoseRejectCode::FrameBindingMissing,
                   [&] { RobotActualPoseAdapter(contract, frames); },
                   "contract frames must exist in the frame map");
            auto mismatched = baseContract();
            mismatched.baseFrame.wireRevisionUuid = uuid(7777);
            reject(RobotPoseRejectCode::FrameIdentityMismatch,
                   [&] { (void)RobotActualPoseAdapter(mismatched, frames); },
                   "contract wire revision must equal mapped revision");
        }

        QTextStream(stdout) << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        QTextStream(stderr) << "FAIL: " << error.what() << "\n";
        return 1;
    }
}
