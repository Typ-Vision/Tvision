#pragma once
#include "MeasurementService.hpp"
#include "vision/metrology/RadialScanner.hpp"
namespace visionflow {
struct RadialScanCalibration {
    PlanarCalibration calibration;
    CalibrationDependency current; // independently resolved installation/version
};
struct CalibratedRadialSearch {
    RadialSearch search; // nominal points in target mm; rawPoint is raw px
    CalibrationDependency calibration;
};
class RadialScanService {
public:
    // T6.1 service API. World annulus/arc and width in mm; explicit mm per
    // synthetic sample unit. No registry, persistence, circle fit or decision.
    static CalibratedRadialSearch scan(const ImageFrame&,const ArcRoi& worldRoi,const RadialScanOptions&,
                                      const RadialScanCalibration&,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
