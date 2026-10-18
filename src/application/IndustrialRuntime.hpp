#pragma once
#include "IndustrialExecution.hpp"
namespace visionflow {
// Admission has no acquisition side effects. A runtime worker re-resolves the exact
// frozen device/backend and uses the Runtime lease before accepting actual readback.
struct IndustrialRuntimeAdmission {
    IndustrialExecutionInputs inputs;
    QMap<QString,QJsonObject> sources;
};
QJsonObject industrialCompletionEvidence(const IndustrialExecutionEvidence&);
void validateIndustrialTerminal(const QJsonObject&);
IndustrialRuntimeAdmission freezeIndustrialRuntime(const IndustrialExecutionInputs&,const QVector<ToolDefinition>&);
QJsonObject industrialRuntimeDependency(const IndustrialCameraRequest&,const IndustrialSourceReference&);
IndustrialCameraRequest decodeIndustrialRuntimeDependency(const ToolDefinition&,const QJsonObject&);
}
