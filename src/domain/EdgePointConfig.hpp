#pragma once
#include <QJsonObject>
namespace visionflow {
QJsonObject edgePointDefaults();
void validateEdgePoint(const QJsonObject&);
}
