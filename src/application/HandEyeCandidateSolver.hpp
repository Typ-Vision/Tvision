#pragma once
#include "domain/RobotCalibrationSession.hpp"
#include "domain/SpatialTransform.hpp"
#include <QJsonObject>
#include <vector>
namespace visionflow {
// Explicit synthetic adapter boundary. Physical collection/pose estimation is
// blocked by COMM4-18; no normal main or wire route constructs this input.
struct HandEyeFixtureSample {
    QUuid sample, frame;
    bool holdout = false;
    SpatialTransform baseTFlange, cameraTPlate;
    double rmsPx = 0, maxPx = 0, ambiguityMarginPx = 0;
};
struct HandEyeFixtureRequest {
    QUuid session;
    CalibrationDependencySnapshot dependencies;
    FrameRef base, flange, camera, plate;
    std::vector<HandEyeFixtureSample> samples;
    double plateDiagonalMm = 0;
    FixturePolicyV1 policy = FixturePolicyV1::create();
    EvidenceState evidence = EvidenceState::Missing;
};
struct HandEyeSolveResult {
    bool accepted = false;
    QString reason;
    // Contains immutable input snapshot, dependency/sample digests, all quality
    // metrics, rejected sample reasons, algorithm/version and synthetic origin.
    QJsonObject report;
};
class HandEyeCandidateSolver {
public:
    // Copies the request before solving; excludes holdout from fit and digest.
    // Success archives a candidate plus provenance through ProductionStore.
    // No active-model port, activation permission or physical solve API exists.
    HandEyeSolveResult solveFixture(const HandEyeFixtureRequest&, const QString& archiveDirectory) const;
};
}
