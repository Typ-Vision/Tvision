#pragma once
#include "ExecutionService.hpp"
namespace visionflow {
class GeometryMeasurementService {
public:
    static ToolResult execute(const ToolDefinition&,const QMap<QString,PortValue>&,const QString& frame,const QString& cycle,
        const std::shared_ptr<std::atomic_bool>&,const MeasurementExecutionContext&);
};
}
