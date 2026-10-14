#pragma once
#include "ToolGraph.hpp"
#include <QStringList>
namespace visionflow {
QJsonObject geometryMeasurementDefaults();
void validateGeometryMeasurement(const QJsonObject&);
QVector<PortDefinition> geometryMeasurementInputs();
QVector<PortDefinition> geometryMeasurementOutputs();
// Exact permitted/required input set for this operation and mode.
QStringList geometryMeasurementRequiredInputs(const QJsonObject&);
}
