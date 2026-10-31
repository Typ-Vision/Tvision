#include "HandEyeReadiness.hpp"

#include <QCryptographicHash>

#include <cmath>
#include <utility>

namespace visionflow {
namespace {
QString number(double value) {
    if (!std::isfinite(value)) return QStringLiteral("nonfinite");
    return QString::number(value, 'g', 17);
}

QString sha256(const QString& text) {
    return QString::fromLatin1(
        QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex());
}
} // namespace

QString readinessCodeName(ReadinessCode code) {
    switch (code) {
    case ReadinessCode::MissingDependency: return QStringLiteral("MISSING_DEPENDENCY");
    case ReadinessCode::InvalidPolicy: return QStringLiteral("INVALID_POLICY");
    case ReadinessCode::UnsupportedProjection: return QStringLiteral("UNSUPPORTED_PROJECTION");
    case ReadinessCode::UnverifiedController: return QStringLiteral("UNVERIFIED_CONTROLLER");
    case ReadinessCode::ActualPoseRequired: return QStringLiteral("ACTUAL_POSE_REQUIRED");
    case ReadinessCode::TcpOffsetRequired: return QStringLiteral("TCP_OFFSET_REQUIRED");
    case ReadinessCode::UnverifiedOptics: return QStringLiteral("UNVERIFIED_OPTICS");
    case ReadinessCode::UnverifiedPlate: return QStringLiteral("UNVERIFIED_PLATE");
    case ReadinessCode::UnverifiedCapture: return QStringLiteral("UNVERIFIED_CAPTURE");
    case ReadinessCode::PhysicalPolicyPending: return QStringLiteral("PHYSICAL_POLICY_PENDING");
    case ReadinessCode::IndependentReferenceRequired:
        return QStringLiteral("INDEPENDENT_REFERENCE_REQUIRED");
    case ReadinessCode::SyntheticEvidenceOnly: return QStringLiteral("SYNTHETIC_EVIDENCE_ONLY");
    }
    return QStringLiteral("UNKNOWN");
}

bool RevisionedIdentity::complete() const {
    return !uuid.trimmed().isEmpty() && revision > 0 && !digest.trimmed().isEmpty();
}

bool EvidenceRef::complete() const {
    return state != EvidenceState::Missing && !id.trimmed().isEmpty() &&
           !revision.trimmed().isEmpty();
}

FixturePolicyV1 FixturePolicyV1::create() {
    FixturePolicyV1 policy;
    policy.revision = QStringLiteral("fixture-policy-v1");
    policy.minRotationSpreadDeg = 5.0;
    policy.minAxisSeparationDeg = 15.0;
    policy.minRotationConditionRatio = 0.1;
    policy.minTranslationCondition = 1e-6;
    policy.minTranslationRatio = 1e-3;
    policy.minTranslationCoverage = 0.25;
    policy.maxPlateRmsPx = 0.5;
    policy.maxPlateMaxPx = 1.5;
    policy.minAmbiguityMarginPx = 0.2;
    policy.minSolveSamples = 12;
    policy.minHoldoutSamples = 5;
    policy.maxRasterDimension = 100000;
    policy.digest = policy.computeDigest();
    return policy;
}

QString FixturePolicyV1::computeDigest() const {
    const QStringList canonical{
        revision,
        number(minRotationSpreadDeg),
        number(minAxisSeparationDeg),
        number(minRotationConditionRatio),
        number(minTranslationCondition),
        number(minTranslationRatio),
        number(minTranslationCoverage),
        number(maxPlateRmsPx),
        number(maxPlateMaxPx),
        number(minAmbiguityMarginPx),
        QString::number(minSolveSamples),
        QString::number(minHoldoutSamples),
        QString::number(maxRasterDimension)};
    return sha256(canonical.join(QLatin1Char('|')));
}

bool FixturePolicyV1::valid() const {
    const FixturePolicyV1 canonical = create();
    return revision == canonical.revision && digest == canonical.digest &&
           minRotationSpreadDeg == canonical.minRotationSpreadDeg &&
           minAxisSeparationDeg == canonical.minAxisSeparationDeg &&
           minRotationConditionRatio == canonical.minRotationConditionRatio &&
           minTranslationCondition == canonical.minTranslationCondition &&
           minTranslationRatio == canonical.minTranslationRatio &&
           minTranslationCoverage == canonical.minTranslationCoverage &&
           maxPlateRmsPx == canonical.maxPlateRmsPx &&
           maxPlateMaxPx == canonical.maxPlateMaxPx &&
           minAmbiguityMarginPx == canonical.minAmbiguityMarginPx &&
           minSolveSamples == canonical.minSolveSamples &&
           minHoldoutSamples == canonical.minHoldoutSamples &&
           maxRasterDimension == canonical.maxRasterDimension;
}

PhysicalTolerancePolicy PhysicalTolerancePolicy::create(QString approvalId,
                                                       QString revision,
                                                       double maxTranslationMm,
                                                       double maxRotationRad,
                                                       double rmsTranslationMm,
                                                       double rmsRotationRad,
                                                       double p95TranslationMm,
                                                       double p95RotationRad) {
    PhysicalTolerancePolicy policy;
    policy.approvalId = std::move(approvalId);
    policy.revision = std::move(revision);
    policy.maxTranslationMm = maxTranslationMm;
    policy.maxRotationRad = maxRotationRad;
    policy.rmsTranslationMm = rmsTranslationMm;
    policy.rmsRotationRad = rmsRotationRad;
    policy.p95TranslationMm = p95TranslationMm;
    policy.p95RotationRad = p95RotationRad;
    policy.digest = policy.computeDigest();
    return policy;
}

QString PhysicalTolerancePolicy::computeDigest() const {
    const QStringList canonical{
        approvalId,
        revision,
        number(maxTranslationMm),
        number(maxRotationRad),
        number(rmsTranslationMm),
        number(rmsRotationRad),
        number(p95TranslationMm),
        number(p95RotationRad)};
    return sha256(canonical.join(QLatin1Char('|')));
}

bool PhysicalTolerancePolicy::valid() const {
    if (approvalId.trimmed().isEmpty() || revision.trimmed().isEmpty() ||
        digest.trimmed().isEmpty())
        return false;
    if (revision == FixturePolicyV1::create().revision) return false;
    const double values[]{maxTranslationMm, maxRotationRad, rmsTranslationMm,
                          rmsRotationRad, p95TranslationMm, p95RotationRad};
    for (const double value : values) {
        if (!std::isfinite(value) || value <= 0) return false;
    }
    if (digest != computeDigest()) return false;
    if (digest == FixturePolicyV1::create().digest) return false;
    return true;
}
} // namespace visionflow
