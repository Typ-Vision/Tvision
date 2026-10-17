#pragma once
#include <QJsonObject>
#include <QVector>
#include "ToolDependency.hpp"
namespace visionflow {
struct ToolDefinition {
    QString id;
    QString kind;
    QString name;
    QString description;
    bool enabled = true;
    QJsonObject parameters;
    QVector<ToolDependency> dependencies;
};
QJsonObject encodeEditorTool(const ToolDefinition&,int editorSchema);
int requiredEditorSchema(const QVector<ToolDefinition>&,int previousSchema=6);
QString toolLabel(const QString& kind);
QJsonObject defaultParameters(const QString& kind);
QString referenceGeometry(const ToolDefinition& tool);
bool referenceNeedsTeaching(const ToolDefinition& edge, const ToolDefinition& source);
QVector<ToolDefinition> editorTools(const QJsonObject& definition);
QJsonObject withEditorTools(QJsonObject definition, const QVector<ToolDefinition>& tools);
void validateEditorDefinition(const QJsonObject& definition);
// Explicit opt-in; ordinary open/save and adding a second source never migrate.
QJsonObject withMultiSourceSchema(QJsonObject definition);
}
