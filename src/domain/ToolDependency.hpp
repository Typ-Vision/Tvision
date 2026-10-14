#pragma once
#include <QJsonArray>
#include <QString>
#include <QVector>
namespace visionflow {
enum class ToolDependencyKind { Backend, Model, Asset };
struct ToolDependency {
    ToolDependencyKind kind=ToolDependencyKind::Backend;
    QString id,version,contentId,path;
    bool operator==(const ToolDependency&) const = default;
};
QJsonArray encodeToolDependencies(const QVector<ToolDependency>&);
QVector<ToolDependency> decodeToolDependencies(const QJsonArray&);
}
