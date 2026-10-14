#pragma once
#include <QJsonObject>
namespace visionflow {
QJsonObject poseMatchDefaults();
void validatePoseMatch(const QJsonObject&);
}
