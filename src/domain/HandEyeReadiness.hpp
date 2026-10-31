#pragma once

#include <QString>
#include <QStringList>

#include <optional>
#include <vector>

namespace visionflow {
// COMM4-18 pure readiness gate. No solver, capture, SDK, activation or hardware code
// belongs here; activation permission is never produced by this report.
enum class HandEyeMode { EyeInHand, EyeToHand };
enum class HandEyeProjection { PinholeBrown, Fisheye, Telecentric, Unknown };
// Synthetic fixture evidence may satisfy mathematical fixture work only. It never
// becomes physical evidence merely because every field is present.
enum class EvidenceState { Missing, Synthetic, PhysicalVerified };
enum class PoseSourceKind { Missing, Commanded, Actual };
enum class PoseFrameKind { Missing, Flange, Tcp };

enum class ReadinessCode {
    MissingDependency,
    InvalidPolicy,
    UnsupportedProjection,
    UnverifiedController,
    ActualPoseRequired,
    TcpOffsetRequired,
    UnverifiedOptics,
    UnverifiedPlate,
    UnverifiedCapture,
    PhysicalPolicyPending,
    IndependentReferenceRequired,
    SyntheticEvidenceOnly
};

QString readinessCodeName(ReadinessCode code);

struct ReadinessIssue {
    ReadinessCode code = ReadinessCode::MissingDependency;
    QString field;
    bool operator==(const ReadinessIssue&) const = default;
};

// Portable identity of a camera/lens/intrinsic/plate/adapter/frame/policy record.
// UUID, positive revision and digest are all mandatory; a display name is never identity.
struct RevisionedIdentity {
    QString uuid;
    int revision = 0;
    QString digest;
    bool complete() const;
    bool operator==(const RevisionedIdentity&) const = default;
};

// Identity and revision of one piece of evidence, preserving its origin.
struct EvidenceRef {
    EvidenceState state = EvidenceState::Missing;
    QString id;
    QString revision;
    bool complete() const;
    bool operator==(const EvidenceRef&) const = default;
};

// Research section 5 fixture policy. The only constructor is create(), so the
// numeric limits are a single immutable project value rather than copied defaults.
struct FixturePolicyV1 {
    QString revision;
    QString digest;
    double minRotationSpreadDeg = 5.0;
    double minAxisSeparationDeg = 15.0;
    double minRotationConditionRatio = 0.1;
    double minTranslationCondition = 1e-6;
    double minTranslationRatio = 1e-3;
    double minTranslationCoverage = 0.25;
    double maxPlateRmsPx = 0.5;
    double maxPlateMaxPx = 1.5;
    double minAmbiguityMarginPx = 0.2;
    int minSolveSamples = 12;
    int minHoldoutSamples = 5;
    int maxRasterDimension = 100000;
    static FixturePolicyV1 create();
    QString computeDigest() const;
    bool valid() const;
    bool operator==(const FixturePolicyV1&) const = default;

private:
    FixturePolicyV1() = default;
};

// Separate physical tolerance approval. It is never defaulted from fixture-policy-v1
// and cannot be eligible without its approval identity.
struct PhysicalTolerancePolicy {
    QString approvalId;
    QString revision;
    QString digest;
    double maxTranslationMm = 0;
    double maxRotationRad = 0;
    double rmsTranslationMm = 0;
    double rmsRotationRad = 0;
    double p95TranslationMm = 0;
    double p95RotationRad = 0;
    static PhysicalTolerancePolicy create(QString approvalId,
                                          QString revision,
                                          double maxTranslationMm,
                                          double maxRotationRad,
                                          double rmsTranslationMm,
                                          double rmsRotationRad,
                                          double p95TranslationMm,
                                          double p95RotationRad);
    QString computeDigest() const;
    bool valid() const;
    bool operator==(const PhysicalTolerancePolicy&) const = default;
};

struct RobotReadinessInput {
    QString brand;
    QString controller;
    QString firmware;
    PoseSourceKind poseSource = PoseSourceKind::Missing;
    PoseFrameKind poseFrame = PoseFrameKind::Missing;
    std::optional<int> tcpOffsetRevision;
    EvidenceRef controllerEvidence;
    EvidenceRef adapterEvidence;
    bool operator==(const RobotReadinessInput&) const = default;
};

struct OpticsReadinessInput {
    HandEyeProjection projection = HandEyeProjection::Unknown;
    int rasterWidth = 0;
    int rasterHeight = 0;
    RevisionedIdentity camera;
    RevisionedIdentity lens;
    RevisionedIdentity intrinsic;
    RevisionedIdentity plate;
    EvidenceRef opticalEvidence;
    EvidenceRef plateEvidence;
    double plateDiagonalMm = 0;
    double plateUncertaintyMm = 0;
    bool operator==(const OpticsReadinessInput&) const = default;
};

struct DatasetReadinessInput {
    EvidenceRef evidence;
    QStringList solveSampleIds;
    QStringList holdoutSampleIds;
    bool operator==(const DatasetReadinessInput&) const = default;
};

struct HandEyeReadinessInput {
    HandEyeMode mode = HandEyeMode::EyeInHand;
    RobotReadinessInput robot;
    OpticsReadinessInput optics;
    RevisionedIdentity adapter;
    RevisionedIdentity frame;
    DatasetReadinessInput dataset;
    EvidenceRef capture;
    std::optional<PhysicalTolerancePolicy> physicalPolicy;
    EvidenceRef independentReference;
    EvidenceRef toleranceApproval;
    FixturePolicyV1 policy = FixturePolicyV1::create();
    bool operator==(const HandEyeReadinessInput&) const = default;
};

// Separate eligibility values and issue collections keep fixture, physical solve
// preflight and physical validation from being conflated. No activation flag exists.
struct HandEyeReadinessReport {
    bool fixtureEligible = false;
    bool physicalSolveEligible = false;
    bool physicalValidationEligible = false;
    std::vector<ReadinessIssue> fixtureIssues;
    std::vector<ReadinessIssue> physicalSolveIssues;
    std::vector<ReadinessIssue> physicalValidationIssues;
    bool operator==(const HandEyeReadinessReport&) const = default;
};
} // namespace visionflow
