#pragma once
#include <QJsonObject>
#include <QString>
namespace visionflow {
// Read-only presentation of frozen or historical facts; never resolves a device.
struct IndustrialSourcePresentation {QString device,configuration,detail;};
QString acquisitionStatus(const QString& state,const QString& diagnostic={});
IndustrialSourcePresentation presentIndustrialSource(const QJsonObject& reference,
    const QJsonObject& admission={},const QJsonObject& acquisition={});
}
