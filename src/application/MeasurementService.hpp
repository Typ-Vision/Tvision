#pragma once
#include "ExecutionService.hpp"
#include "domain/Coordinates.hpp"
namespace visionflow {
struct CalibrationSample {
    QString id;
    Point2D first,second;
    double knownMillimetres=0;
};
// Uniform scale only: parallel plane, no perspective/lens correction.
// Constructors/state are private: a caller cannot mark a candidate validated.
class PlanarScaleCalibration {
public:
    const CalibrationDependency& dependency() const {return dependency_;}
    double millimetresPerPixel() const {return scale_;}
    bool validated() const {return validated_;}
    double validationRmsMillimetres() const {return rms_;}
private:
    friend class MeasurementService;
    PlanarScaleCalibration()=default;
    CalibrationDependency dependency_;
    double scale_=0,rms_=0;
    bool validated_=false;
    CalibrationSample teaching_;
    QVector<CalibrationSample> holdout_;
    double maxError_=0;
};
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
    PlanarScaleCalibration teach(CalibrationDependency setup,const CalibrationSample&) const;
    PlanarScaleCalibration validate(const PlanarScaleCalibration&,const QVector<CalibrationSample>& holdout,double maxErrorMillimetres) const;
    QJsonObject encode(const PlanarScaleCalibration&) const;
    // Recompute scale, evidence acceptance and identity; never trust disk VALID.
    PlanarScaleCalibration decode(const QJsonObject&) const;
    // Call with the two actual DONE upstream results and independently resolved
    // current setup/dependency; errors never carry a value or PASS/FAIL.
    MeasurementResult distance(const ToolResult& first,const QString& firstPort,const ToolResult& second,const QString& secondPort,
        const PlanarScaleCalibration&,const CalibrationDependency& current,const QSize& rawSize,double lowerMillimetres,double upperMillimetres) const;
private:
    static void identify(PlanarScaleCalibration&);
};
}
