#include "application/HandEyeReadinessService.hpp"
#include "domain/HandEyeReadiness.hpp"

#include <QCoreApplication>
#include <QSet>
#include <QStringList>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace visionflow;

namespace {
int checks = 0;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

bool hasIssue(const std::vector<ReadinessIssue>& issues, ReadinessCode code, const QString& field) {
    return std::any_of(issues.begin(), issues.end(), [&](const ReadinessIssue& issue) {
        return issue.code == code && issue.field == field;
    });
}

bool hasCode(const std::vector<ReadinessIssue>& issues, ReadinessCode code) {
    return std::any_of(issues.begin(), issues.end(), [&](const ReadinessIssue& issue) {
        return issue.code == code;
    });
}

void checkSortedUnique(const std::vector<ReadinessIssue>& issues, const char* why) {
    auto sorted = issues;
    std::sort(sorted.begin(), sorted.end(), [](const ReadinessIssue& a, const ReadinessIssue& b) {
        if (static_cast<int>(a.code) != static_cast<int>(b.code))
            return static_cast<int>(a.code) < static_cast<int>(b.code);
        return a.field < b.field;
    });
    check(sorted == issues, why);
    QSet<QString> keys;
    for (const auto& issue : issues) {
        const QString key = readinessCodeName(issue.code) + QLatin1Char('|') + issue.field;
        check(!keys.contains(key), "duplicate readiness issue");
        keys.insert(key);
    }
}

RevisionedIdentity identity(const QString& id) {
    return {id, 1, id + QStringLiteral("-digest")};
}

EvidenceRef physical(const QString& id) {
    return {EvidenceState::PhysicalVerified, id, QStringLiteral("rev-1")};
}

EvidenceRef synthetic(const QString& id) {
    return {EvidenceState::Synthetic, id, QStringLiteral("rev-1")};
}

QStringList samples(const QString& prefix, int count) {
    QStringList result;
    for (int i = 0; i < count; ++i) result.append(prefix + QString::number(i + 1));
    return result;
}

HandEyeReadinessInput fixtureInput(HandEyeMode mode = HandEyeMode::EyeInHand) {
    HandEyeReadinessInput input;
    input.mode = mode;
    input.optics.projection = HandEyeProjection::PinholeBrown;
    input.optics.rasterWidth = 2448;
    input.optics.rasterHeight = 2048;
    input.optics.camera = identity(QStringLiteral("camera-1"));
    input.optics.lens = identity(QStringLiteral("lens-1"));
    input.optics.intrinsic = identity(QStringLiteral("intrinsic-1"));
    input.optics.plate = identity(QStringLiteral("plate-1"));
    input.adapter = identity(QStringLiteral("adapter-1"));
    input.frame = identity(QStringLiteral("frame-1"));
    input.optics.plateDiagonalMm = 100.0;
    input.optics.plateUncertaintyMm = 0.1;
    input.dataset.evidence = synthetic(QStringLiteral("dataset-synthetic-1"));
    input.dataset.solveSampleIds = samples(QStringLiteral("solve-"), 12);
    input.dataset.holdoutSampleIds = samples(QStringLiteral("holdout-"), 5);
    return input;
}

PhysicalTolerancePolicy tolerancePolicy() {
    return PhysicalTolerancePolicy::create(
        QStringLiteral("approval-1"), QStringLiteral("physical-tolerance-v1"),
        0.5, 0.01, 0.2, 0.004, 0.4, 0.008);
}

HandEyeReadinessInput physicalInput(HandEyeMode mode = HandEyeMode::EyeInHand) {
    HandEyeReadinessInput input = fixtureInput(mode);
    input.robot.brand = QStringLiteral("robot-brand");
    input.robot.controller = QStringLiteral("controller");
    input.robot.firmware = QStringLiteral("firmware-1");
    input.robot.poseSource = PoseSourceKind::Actual;
    input.robot.poseFrame = PoseFrameKind::Flange;
    input.robot.controllerEvidence = physical(QStringLiteral("controller-evidence-1"));
    input.robot.adapterEvidence = physical(QStringLiteral("adapter-evidence-1"));
    input.optics.opticalEvidence = physical(QStringLiteral("optical-evidence-1"));
    input.optics.plateEvidence = physical(QStringLiteral("plate-evidence-1"));
    input.dataset.evidence = physical(QStringLiteral("dataset-physical-1"));
    input.capture = physical(QStringLiteral("capture-evidence-1"));
    input.physicalPolicy = tolerancePolicy();
    input.independentReference = physical(QStringLiteral("independent-reference-1"));
    input.toleranceApproval = physical(QStringLiteral("approval-1"));
    return input;
}

void checkFixtureEligible(const HandEyeReadinessInput& input, const char* why) {
    const auto report = HandEyeReadinessService::evaluate(input);
    check(report.fixtureEligible, why);
    check(report.fixtureIssues.empty(), "eligible fixture has no issues");
}

void checkBlocked(const HandEyeReadinessInput& input, ReadinessCode code, const QString& field,
                  bool physicalPath, const char* why) {
    const auto report = HandEyeReadinessService::evaluate(input);
    check(!report.fixtureEligible, why);
    check(hasIssue(report.fixtureIssues, code, field), why);
    if (physicalPath) {
        check(!report.physicalSolveEligible, why);
        check(hasIssue(report.physicalSolveIssues, code, field), why);
    }
}

void checkPhysicalBlocked(const HandEyeReadinessInput& input, ReadinessCode code,
                          const QString& field, const char* why) {
    const auto report = HandEyeReadinessService::evaluate(input);
    check(report.fixtureEligible, "physical-only dependency leaves fixture eligible");
    check(!report.physicalSolveEligible, why);
    check(hasIssue(report.physicalSolveIssues, code, field), why);
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        // Both modes x every projection. Only verified pinhole/Brown-Conrady is eligible.
        for (const auto mode : {HandEyeMode::EyeInHand, HandEyeMode::EyeToHand}) {
            auto pinhole = fixtureInput(mode);
            checkFixtureEligible(pinhole, "fixture pinhole eligible for both modes");
            auto report = HandEyeReadinessService::evaluate(pinhole);
            check(report.fixtureEligible && !report.physicalSolveEligible &&
                      !report.physicalValidationEligible,
                  "synthetic fixture is never a physical preflight");

            auto fisheye = fixtureInput(mode);
            fisheye.optics.projection = HandEyeProjection::Fisheye;
            checkBlocked(fisheye, ReadinessCode::UnsupportedProjection, QStringLiteral("optics.projection"),
                         true, "fisheye unsupported for both paths");

            auto telecentric = fixtureInput(mode);
            telecentric.optics.projection = HandEyeProjection::Telecentric;
            checkBlocked(telecentric, ReadinessCode::UnsupportedProjection,
                         QStringLiteral("optics.projection"), true,
                         "telecentric unsupported for both paths");

            auto unknown = fixtureInput(mode);
            unknown.optics.projection = HandEyeProjection::Unknown;
            checkBlocked(unknown, ReadinessCode::MissingDependency, QStringLiteral("optics.projection"),
                         true, "unknown projection is missing, never guessed");

            auto invalid = fixtureInput(mode);
            invalid.optics.projection = static_cast<HandEyeProjection>(99);
            checkBlocked(invalid, ReadinessCode::MissingDependency, QStringLiteral("optics.projection"),
                         true, "invalid projection enum rejected");
        }

        // Fully synthetic mathematical fixture: fixture=true, physical=false, explicit reasons.
        {
            const auto input = fixtureInput();
            const auto report = HandEyeReadinessService::evaluate(input);
            check(report.fixtureEligible, "complete synthetic fixture eligible");
            check(!report.physicalSolveEligible, "synthetic fixture does not grant physical solve");
            check(!report.physicalValidationEligible, "synthetic fixture does not grant physical validation");
            check(hasIssue(report.physicalSolveIssues, ReadinessCode::UnverifiedController,
                           QStringLiteral("robot.controllerEvidence")),
                  "synthetic fixture reports missing controller evidence on physical path");
            check(hasIssue(report.physicalSolveIssues, ReadinessCode::UnverifiedOptics,
                           QStringLiteral("optics.opticalEvidence")),
                  "synthetic fixture reports missing optics evidence on physical path");
            check(hasIssue(report.physicalSolveIssues, ReadinessCode::UnverifiedPlate,
                           QStringLiteral("optics.plateEvidence")),
                  "synthetic fixture reports missing plate evidence on physical path");
            check(hasIssue(report.physicalSolveIssues, ReadinessCode::UnverifiedCapture,
                           QStringLiteral("capture")),
                  "synthetic fixture reports missing capture evidence on physical path");
            check(hasIssue(report.physicalSolveIssues, ReadinessCode::PhysicalPolicyPending,
                           QStringLiteral("physicalPolicy")),
                  "synthetic fixture has no physical tolerance approval");
            check(hasIssue(report.physicalSolveIssues, ReadinessCode::SyntheticEvidenceOnly,
                           QStringLiteral("dataset.evidence")),
                  "synthetic dataset cannot enter physical path");
            check(hasIssue(report.physicalValidationIssues,
                           ReadinessCode::IndependentReferenceRequired,
                           QStringLiteral("independentReference")),
                  "physical validation requires independent reference");
            checkSortedUnique(report.fixtureIssues, "fixture issues sorted and unique");
            checkSortedUnique(report.physicalSolveIssues, "physical solve issues sorted and unique");
            checkSortedUnique(report.physicalValidationIssues,
                              "physical validation issues sorted and unique");
        }

        // Empty/default input reports every dependency, never an identity/default success.
        {
            const HandEyeReadinessInput input;
            const auto report = HandEyeReadinessService::evaluate(input);
            check(!report.fixtureEligible && !report.physicalSolveEligible &&
                      !report.physicalValidationEligible,
                  "empty input never eligible");
            for (const auto* field : {"optics.camera.uuid", "optics.camera.revision", "optics.camera.digest",
                                      "optics.lens.uuid", "optics.intrinsic.uuid", "optics.plate.uuid",
                                      "adapter.uuid", "frame.uuid", "dataset.evidence",
                                      "optics.raster.width", "optics.raster.height"}) {
                check(hasIssue(report.fixtureIssues, ReadinessCode::MissingDependency, QString::fromLatin1(field)) ||
                          hasIssue(report.fixtureIssues, ReadinessCode::InvalidPolicy, QString::fromLatin1(field)),
                      "empty input reports every mathematical dependency");
            }
            for (const auto* field : {"robot.brand", "robot.controller", "robot.firmware",
                                      "robot.controllerEvidence", "robot.adapterEvidence",
                                      "robot.poseSource", "robot.poseFrame",
                                      "optics.opticalEvidence", "optics.plateEvidence", "capture",
                                      "physicalPolicy"}) {
                check(hasIssue(report.physicalSolveIssues, ReadinessCode::MissingDependency, QString::fromLatin1(field)) ||
                          hasIssue(report.physicalSolveIssues, ReadinessCode::UnverifiedController, QString::fromLatin1(field)) ||
                          hasIssue(report.physicalSolveIssues, ReadinessCode::UnverifiedOptics, QString::fromLatin1(field)) ||
                          hasIssue(report.physicalSolveIssues, ReadinessCode::UnverifiedPlate, QString::fromLatin1(field)) ||
                          hasIssue(report.physicalSolveIssues, ReadinessCode::UnverifiedCapture, QString::fromLatin1(field)) ||
                          hasIssue(report.physicalSolveIssues, ReadinessCode::ActualPoseRequired, QString::fromLatin1(field)) ||
                          hasIssue(report.physicalSolveIssues, ReadinessCode::PhysicalPolicyPending, QString::fromLatin1(field)),
                      "empty input reports every physical dependency");
            }
        }

        // Positive pure preflight fixture: explicit physical evidence for every dependency only.
        {
            const auto input = physicalInput();
            const auto report = HandEyeReadinessService::evaluate(input);
            check(report.fixtureEligible, "physical preflight fixture also passes mathematical gate");
            check(report.physicalSolveEligible, "complete explicit physical preflight is eligible");
            check(report.physicalValidationEligible, "complete explicit reference/tolerance is eligible");
            check(report.physicalSolveIssues.empty(), "positive preflight has no solve issues");
            check(report.physicalValidationIssues.empty(), "positive preflight has no validation issues");
            check(report.physicalValidationEligible && report.physicalSolveEligible,
                  "validation is not weaker than solve preflight");
        }

        // Every physical dependency removed one at a time.
        {
            auto input = physicalInput();
            input.robot.brand.clear();
            check(HandEyeReadinessService::evaluate(input).physicalSolveEligible == false,
                  "brand required for physical solve");
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::MissingDependency, QStringLiteral("robot.brand")),
                  "brand missing issue");

            input = physicalInput();
            input.robot.controller.clear();
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::MissingDependency, QStringLiteral("robot.controller")),
                  "controller missing issue");

            input = physicalInput();
            input.robot.firmware.clear();
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::MissingDependency, QStringLiteral("robot.firmware")),
                  "firmware missing issue");

            input = physicalInput();
            input.robot.controllerEvidence = {};
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::UnverifiedController,
                           QStringLiteral("robot.controllerEvidence")),
                  "controller evidence missing issue");

            input = physicalInput();
            input.robot.adapterEvidence = {};
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::MissingDependency,
                           QStringLiteral("robot.adapterEvidence")),
                  "actual adapter evidence required");

            input = physicalInput();
            input.optics.opticalEvidence = {};
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::UnverifiedOptics,
                           QStringLiteral("optics.opticalEvidence")),
                  "optics evidence missing issue");

            input = physicalInput();
            input.optics.plateEvidence = {};
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::UnverifiedPlate,
                           QStringLiteral("optics.plateEvidence")),
                  "plate evidence missing issue");

            input = physicalInput();
            input.capture = {};
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::UnverifiedCapture, QStringLiteral("capture")),
                  "capture correlation/drain evidence required");

            input = physicalInput();
            input.physicalPolicy.reset();
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::PhysicalPolicyPending,
                           QStringLiteral("physicalPolicy")),
                  "physical tolerance policy required");

            input = physicalInput();
            input.independentReference = {};
            auto report = HandEyeReadinessService::evaluate(input);
            check(report.physicalSolveEligible, "reference is validation-only");
            check(!report.physicalValidationEligible, "missing reference blocks validation");
            check(hasIssue(report.physicalValidationIssues,
                           ReadinessCode::IndependentReferenceRequired,
                           QStringLiteral("independentReference")),
                  "independent reference issue");

            input = physicalInput();
            input.toleranceApproval = {};
            report = HandEyeReadinessService::evaluate(input);
            check(report.physicalSolveEligible, "tolerance approval is validation-only");
            check(!report.physicalValidationEligible, "missing tolerance approval blocks validation");
            check(hasIssue(report.physicalValidationIssues,
                           ReadinessCode::IndependentReferenceRequired,
                           QStringLiteral("toleranceApproval")),
                  "tolerance approval issue");
        }

        // TCP/flange distinction and commanded pose rejection.
        {
            auto input = physicalInput();
            input.robot.poseFrame = PoseFrameKind::Tcp;
            input.robot.tcpOffsetRevision.reset();
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::TcpOffsetRequired,
                           QStringLiteral("robot.tcpOffsetRevision")),
                  "TCP pose without exact offset revision rejected");

            input.robot.tcpOffsetRevision = 0;
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::TcpOffsetRequired,
                           QStringLiteral("robot.tcpOffsetRevision")),
                  "nonpositive TCP offset revision rejected");

            input.robot.tcpOffsetRevision = 7;
            check(HandEyeReadinessService::evaluate(input).physicalSolveEligible,
                  "TCP pose with exact offset revision eligible");

            input = physicalInput();
            input.robot.poseFrame = PoseFrameKind::Missing;
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::MissingDependency, QStringLiteral("robot.poseFrame")),
                  "missing pose frame rejected");

            input = physicalInput();
            input.robot.poseFrame = PoseFrameKind::Flange;
            input.robot.tcpOffsetRevision = 3;
            check(HandEyeReadinessService::evaluate(input).physicalSolveEligible,
                  "flange pose does not require TCP offset");

            input = physicalInput();
            input.robot.poseSource = PoseSourceKind::Commanded;
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::ActualPoseRequired,
                           QStringLiteral("robot.poseSource")),
                  "commanded pose is never actual evidence");

            input.robot.poseSource = PoseSourceKind::Missing;
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::ActualPoseRequired,
                           QStringLiteral("robot.poseSource")),
                  "missing actual pose rejected");
        }

        // Synthetic evidence must never leak into either physical path.
        {
            auto input = physicalInput();
            input.robot.controllerEvidence = synthetic(QStringLiteral("controller-synthetic"));
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::SyntheticEvidenceOnly,
                           QStringLiteral("robot.controllerEvidence")),
                  "synthetic controller evidence blocked");

            input = physicalInput();
            input.robot.adapterEvidence = synthetic(QStringLiteral("adapter-synthetic"));
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::SyntheticEvidenceOnly,
                           QStringLiteral("robot.adapterEvidence")),
                  "synthetic adapter evidence blocked");

            input = physicalInput();
            input.optics.opticalEvidence = synthetic(QStringLiteral("optics-synthetic"));
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::SyntheticEvidenceOnly,
                           QStringLiteral("optics.opticalEvidence")),
                  "synthetic optics evidence blocked");

            input = physicalInput();
            input.optics.plateEvidence = synthetic(QStringLiteral("plate-synthetic"));
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::SyntheticEvidenceOnly,
                           QStringLiteral("optics.plateEvidence")),
                  "synthetic plate evidence blocked");

            input = physicalInput();
            input.capture = synthetic(QStringLiteral("capture-synthetic"));
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::SyntheticEvidenceOnly, QStringLiteral("capture")),
                  "synthetic capture evidence blocked");

            input = physicalInput();
            input.dataset.evidence = synthetic(QStringLiteral("dataset-synthetic"));
            check(hasIssue(HandEyeReadinessService::evaluate(input).physicalSolveIssues,
                           ReadinessCode::SyntheticEvidenceOnly,
                           QStringLiteral("dataset.evidence")),
                  "synthetic dataset blocked");

            input = physicalInput();
            input.independentReference = synthetic(QStringLiteral("reference-synthetic"));
            auto report = HandEyeReadinessService::evaluate(input);
            check(report.physicalSolveEligible, "reference does not affect solve preflight");
            check(hasIssue(report.physicalValidationIssues,
                           ReadinessCode::SyntheticEvidenceOnly,
                           QStringLiteral("independentReference")),
                  "synthetic reference blocked");

            input = physicalInput();
            input.toleranceApproval = synthetic(QStringLiteral("approval-1"));
            report = HandEyeReadinessService::evaluate(input);
            check(hasIssue(report.physicalValidationIssues,
                           ReadinessCode::SyntheticEvidenceOnly,
                           QStringLiteral("toleranceApproval")),
                  "synthetic tolerance approval blocked");
        }

        // Unknown evidence origins must never become verified physical evidence.
        for (const auto mode : {HandEyeMode::EyeInHand, HandEyeMode::EyeToHand}) {
            for (const int state : {-1, 3, 99, std::numeric_limits<int>::max()}) {
                for (int field = 0; field < 8; ++field) {
                    auto input = physicalInput(mode);
                    EvidenceRef* evidence[]{
                        &input.robot.controllerEvidence, &input.robot.adapterEvidence,
                        &input.optics.opticalEvidence, &input.optics.plateEvidence,
                        &input.dataset.evidence, &input.capture,
                        &input.independentReference, &input.toleranceApproval};
                    const QString fields[]{
                        QStringLiteral("robot.controllerEvidence"), QStringLiteral("robot.adapterEvidence"),
                        QStringLiteral("optics.opticalEvidence"), QStringLiteral("optics.plateEvidence"),
                        QStringLiteral("dataset.evidence"), QStringLiteral("capture"),
                        QStringLiteral("independentReference"), QStringLiteral("toleranceApproval")};
                    const ReadinessCode codes[]{
                        ReadinessCode::UnverifiedController, ReadinessCode::MissingDependency,
                        ReadinessCode::UnverifiedOptics, ReadinessCode::UnverifiedPlate,
                        ReadinessCode::MissingDependency, ReadinessCode::UnverifiedCapture,
                        ReadinessCode::IndependentReferenceRequired,
                        ReadinessCode::IndependentReferenceRequired};
                    evidence[field]->state = static_cast<EvidenceState>(state);
                    check(!evidence[field]->complete(), "unknown evidence origin is incomplete");
                    const auto before = input;
                    const auto report = HandEyeReadinessService::evaluate(input);
                    check(report.fixtureEligible == (field != 4),
                          "unknown dataset origin blocks fixture; physical-only origins do not");
                    check(report.physicalSolveEligible == (field >= 6),
                          "unknown solve evidence blocks solve; validation-only evidence does not");
                    check(!report.physicalValidationEligible, "unknown evidence blocks physical validation");
                    check(hasIssue(report.physicalValidationIssues, codes[field],
                                   fields[field] + QStringLiteral(".state")),
                          "unknown evidence reports exact state field");
                    check(report == HandEyeReadinessService::evaluate(input),
                          "unknown evidence report is deterministic");
                    check(input == before, "unknown evidence evaluation does not mutate input");
                }
            }
        }

        // Identity revision/digest and evidence identity are mandatory.
        {
            auto input = fixtureInput();
            input.optics.camera.uuid.clear();
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("optics.camera.uuid"), true,
                         "empty camera UUID rejected");
            input = fixtureInput();
            input.optics.camera.revision = 0;
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("optics.camera.revision"), true,
                         "zero camera revision rejected");
            input = fixtureInput();
            input.optics.camera.digest.clear();
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("optics.camera.digest"), true,
                         "empty camera digest rejected");
            input = fixtureInput();
            input.frame.digest.clear();
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("frame.digest"), true,
                         "empty frame digest rejected");
            input = fixtureInput();
            input.adapter.revision = -1;
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("adapter.revision"), true,
                         "negative adapter revision rejected");
            input = fixtureInput();
            input.dataset.evidence = {EvidenceState::Synthetic, QString(), QStringLiteral("rev-1")};
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("dataset.evidence.id"), true,
                         "evidence identity required");
            input = fixtureInput();
            input.dataset.evidence = {EvidenceState::Synthetic,
                                      QStringLiteral("dataset-1"), QString()};
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("dataset.evidence.revision"), true,
                         "evidence revision required");
        }

        // Repeated sample/frame UUIDs and count thresholds are policy boundaries.
        {
            auto input = fixtureInput();
            input.dataset.solveSampleIds.removeLast();
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("dataset.solveSamples"), false,
                         "below minimum solve samples rejected");

            input = fixtureInput();
            input.dataset.holdoutSampleIds.removeLast();
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("dataset.holdoutSamples"), false,
                         "below minimum holdout samples rejected");

            input = fixtureInput();
            input.dataset.holdoutSampleIds.append(input.dataset.solveSampleIds.first());
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("dataset.sampleIds"), false,
                         "solve/holdout UUID intersection rejected");

            input = fixtureInput();
            input.dataset.solveSampleIds[1] = input.dataset.solveSampleIds[0];
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("dataset.solveSampleIds"), false,
                         "duplicate solve sample rejected");

            input = fixtureInput();
            input.dataset.holdoutSampleIds[1] = QString();
            checkBlocked(input, ReadinessCode::MissingDependency,
                         QStringLiteral("dataset.holdoutSampleIds"), false,
                         "empty holdout sample identity rejected");

            checkFixtureEligible(fixtureInput(), "exact minimum dataset accepted");
        }

        // Raster and plate numeric boundaries.
        {
            auto input = fixtureInput();
            input.optics.rasterWidth = 0;
            checkBlocked(input, ReadinessCode::InvalidPolicy,
                         QStringLiteral("optics.raster.width"), false,
                         "zero raster width rejected");
            input = fixtureInput();
            input.optics.rasterHeight = -1;
            checkBlocked(input, ReadinessCode::InvalidPolicy,
                         QStringLiteral("optics.raster.height"), false,
                         "negative raster height rejected");
            input = fixtureInput();
            input.optics.rasterWidth = FixturePolicyV1::create().maxRasterDimension;
            checkFixtureEligible(input, "maximum raster dimension accepted");
            input.optics.rasterWidth = FixturePolicyV1::create().maxRasterDimension + 1;
            checkBlocked(input, ReadinessCode::InvalidPolicy,
                         QStringLiteral("optics.raster.width"), false,
                         "raster above maximum rejected");

            input = fixtureInput();
            input.optics.plateDiagonalMm = 0;
            checkBlocked(input, ReadinessCode::InvalidPolicy,
                         QStringLiteral("optics.plateDiagonalMm"), false,
                         "zero plate diagonal rejected");
            input = fixtureInput();
            input.optics.plateUncertaintyMm = 0;
            checkBlocked(input, ReadinessCode::InvalidPolicy,
                         QStringLiteral("optics.plateUncertaintyMm"), false,
                         "zero plate uncertainty rejected");
            input = fixtureInput();
            input.optics.plateUncertaintyMm = input.optics.plateDiagonalMm;
            checkBlocked(input, ReadinessCode::InvalidPolicy,
                         QStringLiteral("optics.plateUncertaintyMm"), false,
                         "uncertainty equal to diagonal rejected");
            input = fixtureInput();
            input.optics.plateUncertaintyMm =
                std::nextafter(input.optics.plateDiagonalMm, 0.0);
            checkFixtureEligible(input, "uncertainty just below diagonal accepted");

            for (const double value : {std::numeric_limits<double>::quiet_NaN(),
                                       std::numeric_limits<double>::infinity(),
                                       -std::numeric_limits<double>::infinity()}) {
                input = fixtureInput();
                input.optics.plateDiagonalMm = value;
                checkBlocked(input, ReadinessCode::InvalidPolicy,
                             QStringLiteral("optics.plateDiagonalMm"), false,
                             "nonfinite plate diagonal rejected");
                input = fixtureInput();
                input.optics.plateUncertaintyMm = value;
                checkBlocked(input, ReadinessCode::InvalidPolicy,
                             QStringLiteral("optics.plateUncertaintyMm"), false,
                             "nonfinite plate uncertainty rejected");
            }
        }

        // Fixture policy is a single immutable v1 value; any mutation or nonfinite value is invalid.
        {
            auto input = fixtureInput();
            input.policy.maxPlateRmsPx = -1;
            checkBlocked(input, ReadinessCode::InvalidPolicy, QStringLiteral("policy"), true,
                         "negative fixture policy rejected");
            input = fixtureInput();
            input.policy.minSolveSamples = 0;
            checkBlocked(input, ReadinessCode::InvalidPolicy, QStringLiteral("policy"), true,
                         "zero fixture sample minimum rejected");
            input = fixtureInput();
            input.policy.minTranslationCondition =
                std::numeric_limits<double>::quiet_NaN();
            checkBlocked(input, ReadinessCode::InvalidPolicy, QStringLiteral("policy"), true,
                         "nonfinite fixture policy rejected");
            input = fixtureInput();
            input.policy.digest.clear();
            checkBlocked(input, ReadinessCode::InvalidPolicy, QStringLiteral("policy"), true,
                         "missing fixture policy digest rejected");
            input = fixtureInput();
            input.policy.revision = QStringLiteral("fixture-policy-v2");
            checkBlocked(input, ReadinessCode::InvalidPolicy, QStringLiteral("policy"), true,
                         "foreign fixture policy revision rejected");
            check(FixturePolicyV1::create().valid(), "canonical fixture policy valid");
        }

        // Physical tolerance policy is separate, approved and valid only with physical identity.
        {
            auto input = physicalInput();
            input.physicalPolicy->maxTranslationMm = 0;
            checkPhysicalBlocked(input, ReadinessCode::PhysicalPolicyPending,
                         QStringLiteral("physicalPolicy"),
                         "zero physical tolerance rejected");
            input = physicalInput();
            input.physicalPolicy->rmsRotationRad =
                std::numeric_limits<double>::infinity();
            checkPhysicalBlocked(input, ReadinessCode::PhysicalPolicyPending,
                         QStringLiteral("physicalPolicy"),
                         "nonfinite physical tolerance rejected");
            input = physicalInput();
            input.physicalPolicy->approvalId.clear();
            checkPhysicalBlocked(input, ReadinessCode::PhysicalPolicyPending,
                         QStringLiteral("physicalPolicy"),
                         "physical policy without approval identity rejected");
            input = physicalInput();
            input.physicalPolicy->digest = input.policy.digest;
            checkPhysicalBlocked(input, ReadinessCode::PhysicalPolicyPending,
                         QStringLiteral("physicalPolicy"),
                         "fixture policy digest cannot act as physical tolerance");
            input = physicalInput();
            input.physicalPolicy->revision = FixturePolicyV1::create().revision;
            checkPhysicalBlocked(input, ReadinessCode::PhysicalPolicyPending,
                         QStringLiteral("physicalPolicy"),
                         "fixture policy revision cannot act as physical tolerance");
            input = physicalInput();
            input.physicalPolicy->digest.clear();
            checkPhysicalBlocked(input, ReadinessCode::PhysicalPolicyPending,
                         QStringLiteral("physicalPolicy"),
                         "physical policy digest required");

            input = physicalInput();
            input.toleranceApproval.id = QStringLiteral("approval-2");
            auto report = HandEyeReadinessService::evaluate(input);
            check(report.physicalSolveEligible, "approval linkage is validation-only");
            check(!report.physicalValidationEligible,
                  "physical policy approval identity must match tolerance evidence");
            check(hasIssue(report.physicalValidationIssues,
                           ReadinessCode::PhysicalPolicyPending,
                           QStringLiteral("physicalPolicy.approvalId")),
                  "physical policy approval mismatch reported");
        }

        // Invalid mode enum and deterministic, mutation-free, repeatable evaluation.
        {
            auto input = fixtureInput();
            input.mode = static_cast<HandEyeMode>(42);
            checkBlocked(input, ReadinessCode::MissingDependency, QStringLiteral("mode"), true,
                         "invalid mode enum rejected");

            const auto physical = physicalInput(HandEyeMode::EyeToHand);
            const auto before = physical;
            const auto first = HandEyeReadinessService::evaluate(physical);
            const auto second = HandEyeReadinessService::evaluate(physical);
            check(first == second, "report deterministic");
            check(physical == before, "input not mutated");
            check(first.fixtureEligible && first.physicalSolveEligible &&
                      first.physicalValidationEligible,
                  "eye-to-hand physical preflight eligible");
            checkSortedUnique(first.physicalValidationIssues,
                              "deterministic issue order retained");

            auto changed = physical;
            changed.optics.camera.revision = 0;
            const auto changedReport = HandEyeReadinessService::evaluate(changed);
            check(!changedReport.physicalSolveEligible,
                  "snapshot dependency change re-blocks eligibility");
            check(hasIssue(changedReport.physicalSolveIssues,
                           ReadinessCode::MissingDependency,
                           QStringLiteral("optics.camera.revision")),
                  "changed dependency reports exact field");
        }

        QTextStream(stdout) << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        QTextStream(stderr) << "FAIL: " << error.what() << "\n";
        return 1;
    }
}
