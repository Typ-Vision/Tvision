#pragma once
#include "CalibrationService.hpp"
namespace visionflow {
// Commands shared by Calibration UI and editor workers. No UI/SDK dependency.
class CalibrationWorkflow {
public:
    static CalibrationInstallation installation(const Recipe&,const ImageFrame&,const QString& modelType={});
    static PlanarCalibration teach(const Recipe&,const ImageFrame&,const QString& id,int version,const CalibrationSample&);
    static PlanarCalibration manualScale(const Recipe&,const ImageFrame&,const QString& id,int version,double pixelsPerMillimetre);
    static MeasurementExecutionContext resolve(const Recipe&,const ImageFrame&,const QString& catalogPath);
    static void bind(ProjectService&,const ImageFrame&,const QString& catalogPath,const QString& id,const QString& modelType="uniform_planar_scale");
};
}
