#pragma once
#include "ExecutionService.hpp"
namespace visionflow {
class PoseMatchService {
public:
    // Anchor is explicitly model-local; no implicit centre/angle=0 migration.
    static QJsonObject teach(const ImageFrame&,const QRect&,QPointF modelLocalAnchor,const QJsonObject& parameters);
    static ToolResult execute(const ToolDefinition&,const QMap<QString,PortValue>&,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel);
};
}
