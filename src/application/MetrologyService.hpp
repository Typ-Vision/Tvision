#pragma once
#include "ExecutionService.hpp"
#include "domain/ToolRoi.hpp"
namespace visionflow {
class MetrologyService {
public:
    // Configuration evidence only; never a prediction of a successful fit.
    static QStringList configurationWarnings(const QString& kind,const QJsonObject&);
    static QVector<QLineF> scanGuides(const QString& kind,const QJsonObject&,const ToolRoi&);
    static ToolResult execute(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,
        const std::shared_ptr<std::atomic_bool>&,const MeasurementExecutionContext&);
};
}
