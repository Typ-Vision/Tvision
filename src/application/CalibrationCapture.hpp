#pragma once
#include "CalibrationWizardSession.hpp"
#include "IndependentCalibrationCapture.hpp"
#include "domain/ToolDefinition.hpp"
namespace visionflow {
// Acquisition and setup derivation shared by the wizard and future resolver.
class CalibrationCapture {
public:
    static ImageFrame acquire(const ToolDefinition&,const QString& directory,const std::shared_ptr<std::atomic_bool>&);
    static QString configuration(const ToolDefinition&,const QString& directory);
    static MeasurementSetup setup(const ToolDefinition&,const QString& directory,const ImageFrame&,
        const QString& installation,const QString& lens,const QString& plane);
};
}
