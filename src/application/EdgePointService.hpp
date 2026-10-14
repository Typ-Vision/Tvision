#pragma once
#include "ExecutionService.hpp"
namespace visionflow {
class EdgePointService {
public:
    static ToolResult execute(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&);
};
}
