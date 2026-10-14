#pragma once
#include "ExecutionService.hpp"
namespace visionflow {
class CircleToolService {
public:
    static ToolResult execute(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,
        const std::shared_ptr<std::atomic_bool>&,const MeasurementExecutionContext&);
    // Validated application mapping used by teaching/overlay, never by a UI algorithm.
    static QPointF mapPoint(const ImageFrame&,const MeasurementExecutionContext&,QPointF,bool toRaw);
    static void validateCompletion(const ToolDefinition&,const ToolResult&,const std::optional<CalibrationDependency>&);
};
}
