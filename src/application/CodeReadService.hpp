#pragma once
#include "ToolExecutors.hpp"
namespace visionflow {
class CodeReadService {
public:
    static ToolResult execute(const ToolDefinition&,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&,const std::optional<ToolRoi>& resolvedRoi={});
    static InspectionResults inspect(const CodeResults&,const QJsonObject&);
};
}
