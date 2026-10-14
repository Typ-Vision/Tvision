#pragma once
#include <QJsonObject>
#include <QString>
namespace visionflow {
QJsonObject metrologyDefaults(const QString& kind);
void validateMetrology(const QString& kind,const QJsonObject&);
bool isMetrology(const QString& kind);
}
