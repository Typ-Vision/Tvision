#pragma once
#include "ToolDefinition.hpp"
#include <QStringList>
namespace visionflow {
enum class PortType { Image, Region, Point, Number, Boolean, Text, CodeResults, PointSet, Profile, Line2D, Circle2D, Arc2D, Pose2D, InspectionResults };
struct PortDefinition { QString id; PortType type; QString unit; QString frame; bool required=true; };
struct ToolConnection { QString sourceTool, sourcePort, targetTool, targetPort; bool operator==(const ToolConnection&) const = default; };
QVector<PortDefinition> inputPorts(const QString& kind);
QVector<PortDefinition> outputPorts(const QString& kind);
QString portTypeName(PortType type);
QVector<ToolConnection> editorConnections(const QJsonObject& definition);
QJsonObject withEditorConnections(QJsonObject definition, const QVector<ToolConnection>& connections);
struct GraphValidation { QStringList errors; QVector<QString> order; bool valid() const { return errors.isEmpty(); } };
GraphValidation validateGraph(const QVector<ToolDefinition>& tools, const QVector<ToolConnection>& connections);
}
