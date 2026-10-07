#pragma once
#include "ExecutionService.hpp"
#include <functional>
namespace visionflow {
using ToolExecutor=std::function<ToolResult(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&)>;
class ToolExecutors {
public:
    static void registerExecutor(const QString& kind,ToolExecutor executor);
    static ToolExecutor get(const QString& kind,const MeasurementExecutionContext& context={});
private:
    static ToolResult webcam(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&);
    static ToolResult imageFile(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&);
    static ToolResult calculate(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&);
    static ToolResult match(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&);
    static ToolResult blob(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&);
    static ToolResult edge(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&);
};
}
