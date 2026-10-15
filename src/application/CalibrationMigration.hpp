#pragma once
#include "CalibrationService.hpp"
#include "CalibrationCapture.hpp"
namespace visionflow {
class CalibrationMigration {
public:
    // Copies the complete ID history in order; existing exact copies are no-op.
    // The caller must obtain the explicit same-installation declaration first.
    static int adopt(const CalibrationService& legacy,ActiveCalibrationService& target,const QString& id,
        const ToolDefinition& currentSource,const QString& directory,const ImageFrame& actual,const QString& installation);
};
}
