#pragma once
#include "MeasurementService.hpp"
#include "ProjectService.hpp"
#include <functional>

namespace visionflow {
// Application-owned installation identity, resolved independently of saved evidence.
// cameraId is the acquisition source identity; physical device identity must be
// incorporated by the provider (a webcam index alone is insufficient).
struct CalibrationInstallation {
    QString cameraId,lensId,workingPlaneId;
    QSize resolution;
    CoordinateFrame source,target;
};
class CalibrationService {
public:
    using InstallationProvider=std::function<CalibrationInstallation(const QString& calibrationId)>;
    explicit CalibrationService(QString catalogPath,InstallationProvider provider={});
    // Append only, consecutive versions per ID; latest saved version is current.
    // Saving mathematical evidence does not authorize production activation.
    void save(const PlanarCalibration&) const;
    PlanarCalibration load(const QString& id,int version) const;
    CalibrationDependency current(const QString& id) const;
    PlanarCalibration resolve(const Recipe&) const;
    void bind(ProjectService&,const QString& id) const;
private:
    QVector<PlanarCalibration> read() const;
    CalibrationDependency currentFrom(const QVector<PlanarCalibration>&,const QString& id) const;
    QString path_;
    InstallationProvider provider_;
};
}
