#pragma once
#include "ToolDefinition.hpp"
#include <QStringList>
#include <QMap>
namespace visionflow {
enum class PortType { Image, Region, Point, Number, Boolean, Text, CodeResults, PointSet, Profile, Line2D, Circle2D, Arc2D, Pose2D, InspectionResults, AssignmentBundle };
struct PortDefinition { QString id; PortType type; QString unit; QString frame; bool required=true; };
struct ToolConnection { QString sourceTool, sourcePort, targetTool, targetPort; bool operator==(const ToolConnection&) const = default; };
QVector<PortDefinition> inputPorts(const QString& kind);
QVector<PortDefinition> outputPorts(const QString& kind);
QString portTypeName(PortType type);
QVector<ToolConnection> editorConnections(const QJsonObject& definition);
// Includes explicit UUID/port/revision assignment sources as dependency edges.
QVector<ToolConnection> assignmentConnections(const QVector<ToolDefinition>&);
QVector<ToolConnection> executionConnections(const QJsonObject&);
QJsonObject withEditorConnections(QJsonObject definition, const QVector<ToolConnection>& connections);
// Origins use recipe acquisition tool IDs, never display names or physical IDs.
struct GraphValidation { QStringList errors; QVector<QString> order; QStringList requiredSources;
    QMap<QString,QString> toolSources; QMap<QString,QMap<QString,QString>> portSources;
    bool valid() const { return errors.isEmpty(); } };
GraphValidation validateGraph(const QVector<ToolDefinition>& tools, const QVector<ToolConnection>& connections);
// Strict multi-source contract additionally rejects disabled required upstreams.
// Legacy validateGraph keeps disabled dependency -> SKIPPED execution semantics.
GraphValidation validateSourceGraph(const QVector<ToolDefinition>& tools, const QVector<ToolConnection>& connections);
}
