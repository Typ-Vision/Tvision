#pragma once
#include "PoseMatchService.hpp"
namespace visionflow {
class ShapeMatchService {
public:
    static QJsonObject teach(const ImageFrame&,const QRect&,QPointF,const QJsonObject&);
    static ToolResult execute(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&);
};
}
