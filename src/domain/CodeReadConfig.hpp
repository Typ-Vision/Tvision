#pragma once
#include <QJsonObject>
#include <QStringList>
namespace visionflow {
QStringList codeFormats();
QJsonObject codeReadDefaults();
void validateCodeRead(const QJsonObject&);
}
