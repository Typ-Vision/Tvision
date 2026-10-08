#pragma once
#include "ExecutionService.hpp"
#include "domain/Coordinates.hpp"
#include "PlaneMappingService.hpp"
namespace visionflow {
struct CalibrationSample {
    QString id;
    Point2D first,second;
    double knownMillimetres=0;
};
struct ManualScaleProvenance {
    QString origin; // acquired or file_test; declaration, not physical acceptance
    QString sourceId,frameId;
    QSize resolution;
};
// Typed planar calibration; taught/manual scale and homography share validation.
// Constructors/state are private: a caller cannot mark a candidate validated.
class PlanarCalibration {
public:
    const CalibrationDependency& dependency() const {return dependency_;}
    double millimetresPerPixel() const;
    double pixelsPerMillimetre() const;
    const std::optional<ManualScaleProvenance>& manualProvenance() const {return manualProvenance_;}
    const std::optional<PlaneMapping>& mapping() const {return mapping_;}
    double maxValidationErrorMillimetres() const {return maxResidual_;}
    const QVector<double>& measuredHoldoutMillimetres() const {return measured_;}
    const QVector<double>& residualHoldoutMillimetres() const {return residuals_;}
    bool validated() const {return validated_;}
    double validationRmsMillimetres() const {return rms_;}
private:
    friend class MeasurementService;
    PlanarCalibration()=default;
    CalibrationDependency dependency_;
    double scale_=0,rms_=0,maxResidual_=0;
    double pixelsPerMillimetre_=0;
    std::optional<ManualScaleProvenance> manualProvenance_;
    std::optional<PlaneMapping> mapping_;
    QVector<double> measured_,residuals_;
    bool validated_=false;
    CalibrationSample teaching_;
    QVector<CalibrationSample> holdout_;
    double maxError_=0;
};
// Compatibility name for callers of the legacy scale API.
using PlanarScaleCalibration=PlanarCalibration;
struct MeasurementResult {
    ExecutionState state=ExecutionState::Error;
    PartDecision decision=PartDecision::NotEvaluated;
    std::optional<double> pixels,millimetres;
    QString errorCode,error;
    FrameMetadata origin;
    CalibrationDependency calibration;
};
class MeasurementService {
public:
    // contentId is generated from setup, scale and evidence, never trusted input.
    PlanarCalibration teach(CalibrationDependency setup,const CalibrationSample&) const;
    PlanarCalibration manualScale(CalibrationDependency setup,double pixelsPerMillimetre,const ManualScaleProvenance&) const;
    // Candidate-only preview: no decision, validation or production authorization.
    double trialDistance(const PlanarCalibration&,const Point2D&,const Point2D&) const;
    PlanarCalibration teachHomography(CalibrationDependency setup,const PlaneMapping&) const;
    PlanarCalibration validate(const PlanarCalibration&,const QVector<CalibrationSample>& holdout,double maxErrorMillimetres) const;
    QJsonObject encode(const PlanarCalibration&) const;
    // Recompute scale, evidence acceptance and identity; never trust disk VALID.
    PlanarCalibration decode(const QJsonObject&) const;
    // Call with the two actual DONE upstream results and independently resolved
    // current setup/dependency; errors never carry a value or PASS/FAIL.
    MeasurementResult distance(const ToolResult& first,const QString& firstPort,const ToolResult& second,const QString& secondPort,
        const PlanarCalibration&,const CalibrationDependency& current,const QSize& rawSize,double lowerMillimetres,double upperMillimetres) const;
private:
    static void identify(PlanarCalibration&);
};
}
