#pragma once
#include "ExecutionService.hpp"
namespace visionflow {
ToolResult executeAssignments(const ToolDefinition&,const QJsonObject& definition,
    const QMap<QString,ToolResult>& completed,const QString& cycleId,const QString& configId);
}
