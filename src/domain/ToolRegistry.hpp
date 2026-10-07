#pragma once
#include "ToolGraph.hpp"
#include <functional>
namespace visionflow {
struct ToolDescriptor {
    QString kind,label; int version;
    QJsonObject defaults;
    QVector<PortDefinition> inputs,outputs;
    std::function<void(const ToolDefinition&)> validate;
    bool acquisitionSource=false;
};
class ToolRegistry {
public:
    static QVector<ToolDescriptor> entries();
    // Register compiled extensions during startup, before workers begin.
    static void registerType(ToolDescriptor descriptor);
    static ToolDescriptor get(const QString& kind);
    static void validate(const ToolDefinition& tool);
};
}
