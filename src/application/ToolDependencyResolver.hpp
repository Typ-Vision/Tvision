#pragma once
#include "domain/ToolRegistry.hpp"
namespace visionflow {
class ToolDependencyResolver {
public:
    static QJsonArray resolve(const ToolDefinition&,const QString& projectDirectory);
};
}
