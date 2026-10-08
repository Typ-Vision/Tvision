#pragma once
#include "MeasurementService.hpp"
namespace visionflow {
struct CalibrationStudy {CalibrationTestImage image;PlanarCalibration calibration;};
// Offline study only, never a recipe binding. Referenced source image must remain
// available and reproduce the decoded content identity; no unbounded image embed.
class CalibrationStudyStore {
public:
    static void save(const QString& absolutePath,const CalibrationTestImage&,const PlanarCalibration&);
    static CalibrationStudy load(const QString& absolutePath);
};
}
