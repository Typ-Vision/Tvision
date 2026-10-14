#pragma once
#include "ToolResultCodec.hpp"
namespace visionflow {
// Terminal evidence only; replay never grants execution permission.
class ProductionResultCodec {
public:
    static constexpr int maxRecordBytes=16*1024*1024;
    static QJsonObject output(const PortValue&);
    static void validate(const QJsonObject& record);
    static QString summary(const QJsonObject& record);
    static QByteArray csv(const QVector<QJsonObject>& records);
};
}
