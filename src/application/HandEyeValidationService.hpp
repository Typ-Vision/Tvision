#pragma once
#include "HandEyeCandidateSolver.hpp"
#include <functional>

namespace visionflow {
// Explicit fixture adapter: neither this API nor remembered ACTIVE evidence is
// a production permission. No normal wire/main composition is installed.
struct HandEyeIndependentReference {
    QUuid sample;
    SpatialTransform expectedPlate; // independently measured base/flange_T_plate
};
struct HandEyeValidationTolerance {
    QUuid revision;
    double maxMm=0, rmsMm=0, p95Mm=0, maxRad=0, rmsRad=0, p95Rad=0;
};
struct HandEyeValidationRequest {
    QUuid session, candidate, candidateRevision, referenceRevision;
    QString referenceSource; // independent oracle/metrology identity, never training closure
    EvidenceState evidence=EvidenceState::Missing;
    std::vector<HandEyeFixtureSample> holdout;
    std::vector<HandEyeIndependentReference> references;
    HandEyeValidationTolerance tolerance;
};
struct HandEyeValidationResult {
    bool accepted=false;
    QString reason;
    QJsonObject report;
};
class HandEyeValidationService {
public:
    explicit HandEyeValidationService(QString archiveDirectory);
    HandEyeValidationResult validateFixture(const HandEyeValidationRequest&) const;
private:
    QString directory_;
};

// The provider must hold station admission, camera drain and configuration
// mutation locks for the ENTIRE callback, including the disk commit. Returning
// a snapshot without that exclusion is not an implementation of this port.
struct HandEyeActivationSnapshot {
    QUuid station, session, candidate, candidateRevision;
    CalibrationDependencySnapshot dependencies;
    bool idle=false, cameraDrained=false;
    QUuid referenceRevision, toleranceRevision;
};
class HandEyeActivationBoundary {
public:
    virtual ~HandEyeActivationBoundary()=default;
    virtual void withExclusiveSnapshot(const std::function<void(const HandEyeActivationSnapshot&)>&)=0;
};
struct HandEyeActivationRequest {
    QUuid station, session, candidate, candidateRevision, validationReport, expectedActiveRevision;
    QString dependenciesDigest;
};
struct HandEyeActivationResult {
    bool accepted=false;
    QString reason;
    QJsonObject active;
};
class HandEyeActivationService {
public:
    // Test-only crash seam: called after open, after partial/complete write, before
    // commit and after commit. A child process may terminate at each boundary.
    using Fault=std::function<void(const QString&)>;
    explicit HandEyeActivationService(QString archiveDirectory);
    HandEyeActivationResult activateFixture(const HandEyeActivationRequest&,HandEyeActivationBoundary&,Fault={}) const;
    // Physical policy/reference/native gates are still closed by COMM4-18.
    HandEyeActivationResult activateProduction(const HandEyeActivationRequest&) const;
    QJsonObject remembered(const QUuid& station) const;
private:
    QString directory_;
};
// Shared exact serialization, also used by the candidate solver. Display names
// are deliberately absent; every geometric change needs a new revision.
QJsonObject handEyeDependencyJson(const CalibrationDependencySnapshot&);
QString handEyeDigest(const QJsonObject&);
}
