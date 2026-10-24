#pragma once
#include "MeasurementService.hpp"
#include "vision/metrology/EdgePairFinder.hpp"
namespace visionflow {
enum class EdgePairMode { PixelDebug, Calibrated };
struct EdgePairCalibration {
    PlanarCalibration calibration;
    CalibrationDependency current; // Independently resolved current setup.
};
struct EdgePairResult {
    ExecutionState state=ExecutionState::Error;
    bool found=false;
    std::optional<QPointF> first,second,centre;
    std::optional<double> widthPx,widthMm;
    FrameMetadata origin;
    std::optional<CalibrationDependency> calibration;
    QString error;
    std::optional<FitDiagnostic> diagnostic;
};
class EdgePairService {
public:
    // Service-only T5.1 API. Registry, recipe persistence and UI arrive in T5.3.
    static EdgePairResult measure(const ImageFrame&,const ToolRoi&,const EdgePairOptions&,
        EdgePairMode,const EdgePairCalibration* = nullptr,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
