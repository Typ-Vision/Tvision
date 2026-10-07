#pragma once
#include <QJsonObject>
#include <QVector>
namespace visionflow {
struct ToolDefinition {
    QString id;
    QString kind;
    QString name;
    QString description;
    bool enabled = true;
    QJsonObject parameters;
};
QString toolLabel(const QString& kind);
QJsonObject defaultParameters(const QString& kind);
QString referenceGeometry(const ToolDefinition& tool);
bool referenceNeedsTeaching(const ToolDefinition& edge, const ToolDefinition& source);
QVector<ToolDefinition> editorTools(const QJsonObject& definition);
QJsonObject withEditorTools(QJsonObject definition, const QVector<ToolDefinition>& tools);
void validateEditorDefinition(const QJsonObject& definition);
}
