#pragma once
#include "MeasurementService.hpp"
namespace visionflow {
class CalibrationReportService {
public:
    // Exports actual computed evidence with model/provenance/unit labels.
    // This report does not attest to physical reference origin or production approval.
    static void saveCsv(const QString& absolutePath,const PlanarCalibration&);
};
}
