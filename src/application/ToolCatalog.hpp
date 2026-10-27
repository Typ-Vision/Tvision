#pragma once
#include "domain/ToolRegistry.hpp"
#include <algorithm>

namespace visionflow {
// New-tool presentation policy is independent of persisted type compatibility.
struct ToolCatalog {
    static QString groupForKind(const QString& kind) {
        if(QStringList{"image_file","webcam","industrial_camera"}.contains(kind))return "acquisition";
        if(kind=="preprocess" || kind=="roi")return "processing";
        if(QStringList{"blob","code_read"}.contains(kind))return "detection";
        if(QStringList{"template_match","pattern_pose","shape_pose"}.contains(kind))return "matching";
        if(QStringList{"edge_point","edge_pair","line_fit","circle_fit","measurement","geometry_measurement"}.contains(kind))return "measurement";
        if(kind=="calculate" || kind=="set_variables" || kind=="result" || kind=="publish_data")return "calculation";
        return "other";
    }
    static QVector<ToolDescriptor> entries() {
        auto result=ToolRegistry::entries();
        result.erase(std::remove_if(result.begin(),result.end(),[](const auto& entry){return entry.kind=="edge";}),result.end());
        return result;
    }
    static bool canAdd(const QString& kind) {
        for(const auto& entry:entries())if(entry.kind==kind)return true;
        return false;
    }
};
}
