#include "HandEyeReadinessService.hpp"

#include <QSet>

#include <algorithm>
#include <cmath>
#include <vector>

namespace visionflow {
namespace {
void addIssue(std::vector<ReadinessIssue>& issues, ReadinessCode code, const QString& field) {
    issues.push_back({code, field});
}

void addIdentityIssues(std::vector<ReadinessIssue>& issues,
                       const RevisionedIdentity& identity,
                       const QString& prefix) {
    if (identity.uuid.trimmed().isEmpty())
        addIssue(issues, ReadinessCode::MissingDependency, prefix + QStringLiteral(".uuid"));
    if (identity.revision <= 0)
        addIssue(issues, ReadinessCode::MissingDependency, prefix + QStringLiteral(".revision"));
    if (identity.digest.trimmed().isEmpty())
        addIssue(issues, ReadinessCode::MissingDependency, prefix + QStringLiteral(".digest"));
}

void addEvidenceIssues(std::vector<ReadinessIssue>& issues,
                       const EvidenceRef& evidence,
                       const QString& field,
                       ReadinessCode missingCode,
                       bool syntheticAllowed) {
    if (evidence.state == EvidenceState::Missing) {
        addIssue(issues, missingCode, field);
        return;
    }
    if (evidence.state != EvidenceState::Synthetic &&
        evidence.state != EvidenceState::PhysicalVerified) {
        addIssue(issues, missingCode, field + QStringLiteral(".state"));
        return;
    }
    bool complete = true;
    if (evidence.id.trimmed().isEmpty()) {
        addIssue(issues, missingCode, field + QStringLiteral(".id"));
        complete = false;
    }
    if (evidence.revision.trimmed().isEmpty()) {
        addIssue(issues, missingCode, field + QStringLiteral(".revision"));
        complete = false;
    }
    if (complete && evidence.state == EvidenceState::Synthetic && !syntheticAllowed)
        addIssue(issues, ReadinessCode::SyntheticEvidenceOnly, field);
}

void addSampleListIssues(std::vector<ReadinessIssue>& issues,
                         const QStringList& samples,
                         const QString& field) {
    QSet<QString> seen;
    for (const QString& sample : samples) {
        const QString trimmed = sample.trimmed();
        if (trimmed.isEmpty() || seen.contains(trimmed))
            addIssue(issues, ReadinessCode::MissingDependency, field);
        seen.insert(trimmed);
    }
}

std::vector<ReadinessIssue> fixtureIssues(const HandEyeReadinessInput& input) {
    std::vector<ReadinessIssue> issues;

    if (input.mode != HandEyeMode::EyeInHand && input.mode != HandEyeMode::EyeToHand)
        addIssue(issues, ReadinessCode::MissingDependency, QStringLiteral("mode"));

    if (!input.policy.valid())
        addIssue(issues, ReadinessCode::InvalidPolicy, QStringLiteral("policy"));

    switch (input.optics.projection) {
    case HandEyeProjection::PinholeBrown:
        break;
    case HandEyeProjection::Fisheye:
    case HandEyeProjection::Telecentric:
        addIssue(issues, ReadinessCode::UnsupportedProjection,
                 QStringLiteral("optics.projection"));
        break;
    case HandEyeProjection::Unknown:
    default:
        addIssue(issues, ReadinessCode::MissingDependency,
                 QStringLiteral("optics.projection"));
        break;
    }

    if (input.optics.rasterWidth <= 0 ||
        input.optics.rasterWidth > input.policy.maxRasterDimension)
        addIssue(issues, ReadinessCode::InvalidPolicy,
                 QStringLiteral("optics.raster.width"));
    if (input.optics.rasterHeight <= 0 ||
        input.optics.rasterHeight > input.policy.maxRasterDimension)
        addIssue(issues, ReadinessCode::InvalidPolicy,
                 QStringLiteral("optics.raster.height"));

    addIdentityIssues(issues, input.optics.camera, QStringLiteral("optics.camera"));
    addIdentityIssues(issues, input.optics.lens, QStringLiteral("optics.lens"));
    addIdentityIssues(issues, input.optics.intrinsic, QStringLiteral("optics.intrinsic"));
    addIdentityIssues(issues, input.optics.plate, QStringLiteral("optics.plate"));
    addIdentityIssues(issues, input.adapter, QStringLiteral("adapter"));
    addIdentityIssues(issues, input.frame, QStringLiteral("frame"));

    const double diagonal = input.optics.plateDiagonalMm;
    const double uncertainty = input.optics.plateUncertaintyMm;
    if (!std::isfinite(diagonal) || diagonal <= 0)
        addIssue(issues, ReadinessCode::InvalidPolicy,
                 QStringLiteral("optics.plateDiagonalMm"));
    if (!std::isfinite(uncertainty) || uncertainty <= 0)
        addIssue(issues, ReadinessCode::InvalidPolicy,
                 QStringLiteral("optics.plateUncertaintyMm"));
    else if (std::isfinite(diagonal) && diagonal > 0 && uncertainty >= diagonal)
        addIssue(issues, ReadinessCode::InvalidPolicy,
                 QStringLiteral("optics.plateUncertaintyMm"));

    addEvidenceIssues(issues, input.dataset.evidence, QStringLiteral("dataset.evidence"),
                      ReadinessCode::MissingDependency, true);

    addSampleListIssues(issues, input.dataset.solveSampleIds,
                        QStringLiteral("dataset.solveSampleIds"));
    addSampleListIssues(issues, input.dataset.holdoutSampleIds,
                        QStringLiteral("dataset.holdoutSampleIds"));

    QSet<QString> solveIds;
    for (const QString& sample : input.dataset.solveSampleIds) {
        if (!sample.trimmed().isEmpty()) solveIds.insert(sample.trimmed());
    }
    for (const QString& sample : input.dataset.holdoutSampleIds) {
        if (!sample.trimmed().isEmpty() && solveIds.contains(sample.trimmed())) {
            addIssue(issues, ReadinessCode::MissingDependency,
                     QStringLiteral("dataset.sampleIds"));
            break;
        }
    }

    if (input.dataset.solveSampleIds.size() < input.policy.minSolveSamples)
        addIssue(issues, ReadinessCode::MissingDependency,
                 QStringLiteral("dataset.solveSamples"));
    if (input.dataset.holdoutSampleIds.size() < input.policy.minHoldoutSamples)
        addIssue(issues, ReadinessCode::MissingDependency,
                 QStringLiteral("dataset.holdoutSamples"));

    return issues;
}

void addPhysicalSolveIssues(std::vector<ReadinessIssue>& issues,
                            const HandEyeReadinessInput& input) {
    if (input.robot.brand.trimmed().isEmpty())
        addIssue(issues, ReadinessCode::MissingDependency, QStringLiteral("robot.brand"));
    if (input.robot.controller.trimmed().isEmpty())
        addIssue(issues, ReadinessCode::MissingDependency,
                 QStringLiteral("robot.controller"));
    if (input.robot.firmware.trimmed().isEmpty())
        addIssue(issues, ReadinessCode::MissingDependency,
                 QStringLiteral("robot.firmware"));

    addEvidenceIssues(issues, input.robot.controllerEvidence,
                      QStringLiteral("robot.controllerEvidence"),
                      ReadinessCode::UnverifiedController, false);
    addEvidenceIssues(issues, input.robot.adapterEvidence,
                      QStringLiteral("robot.adapterEvidence"),
                      ReadinessCode::MissingDependency, false);

    if (input.robot.poseSource != PoseSourceKind::Actual)
        addIssue(issues, ReadinessCode::ActualPoseRequired,
                 QStringLiteral("robot.poseSource"));

    if (input.robot.poseFrame == PoseFrameKind::Tcp) {
        if (!input.robot.tcpOffsetRevision.has_value() ||
            *input.robot.tcpOffsetRevision <= 0)
            addIssue(issues, ReadinessCode::TcpOffsetRequired,
                     QStringLiteral("robot.tcpOffsetRevision"));
    } else if (input.robot.poseFrame != PoseFrameKind::Flange) {
        addIssue(issues, ReadinessCode::MissingDependency,
                 QStringLiteral("robot.poseFrame"));
    }

    addEvidenceIssues(issues, input.optics.opticalEvidence,
                      QStringLiteral("optics.opticalEvidence"),
                      ReadinessCode::UnverifiedOptics, false);
    addEvidenceIssues(issues, input.optics.plateEvidence,
                      QStringLiteral("optics.plateEvidence"),
                      ReadinessCode::UnverifiedPlate, false);
    addEvidenceIssues(issues, input.capture, QStringLiteral("capture"),
                      ReadinessCode::UnverifiedCapture, false);

    if (!input.physicalPolicy.has_value() || !input.physicalPolicy->valid())
        addIssue(issues, ReadinessCode::PhysicalPolicyPending,
                 QStringLiteral("physicalPolicy"));

    addEvidenceIssues(issues, input.dataset.evidence, QStringLiteral("dataset.evidence"),
                      ReadinessCode::MissingDependency, false);
}

void addPhysicalValidationIssues(std::vector<ReadinessIssue>& issues,
                                 const HandEyeReadinessInput& input) {
    if (input.physicalPolicy.has_value() && input.physicalPolicy->valid() &&
        input.toleranceApproval.complete() &&
        input.toleranceApproval.id != input.physicalPolicy->approvalId)
        addIssue(issues, ReadinessCode::PhysicalPolicyPending,
                 QStringLiteral("physicalPolicy.approvalId"));

    addEvidenceIssues(issues, input.independentReference,
                      QStringLiteral("independentReference"),
                      ReadinessCode::IndependentReferenceRequired, false);
    addEvidenceIssues(issues, input.toleranceApproval,
                      QStringLiteral("toleranceApproval"),
                      ReadinessCode::IndependentReferenceRequired, false);
}

void normalize(std::vector<ReadinessIssue>& issues) {
    std::sort(issues.begin(), issues.end(),
              [](const ReadinessIssue& left, const ReadinessIssue& right) {
                  if (left.code != right.code)
                      return static_cast<int>(left.code) < static_cast<int>(right.code);
                  return left.field < right.field;
              });
    issues.erase(std::unique(issues.begin(), issues.end()), issues.end());
}
} // namespace

HandEyeReadinessReport HandEyeReadinessService::evaluate(const HandEyeReadinessInput& input) {
    const std::vector<ReadinessIssue> mathematical = fixtureIssues(input);

    HandEyeReadinessReport report;
    report.fixtureIssues = mathematical;
    normalize(report.fixtureIssues);
    report.fixtureEligible = report.fixtureIssues.empty();

    report.physicalSolveIssues = mathematical;
    addPhysicalSolveIssues(report.physicalSolveIssues, input);
    normalize(report.physicalSolveIssues);
    report.physicalSolveEligible = report.physicalSolveIssues.empty();

    report.physicalValidationIssues = report.physicalSolveIssues;
    addPhysicalValidationIssues(report.physicalValidationIssues, input);
    normalize(report.physicalValidationIssues);
    report.physicalValidationEligible = report.physicalValidationIssues.empty();

    return report;
}
} // namespace visionflow
