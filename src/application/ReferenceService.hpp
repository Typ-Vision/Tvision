#pragma once
#include "PoseRoiService.hpp"
#include "domain/ReferenceConfig.hpp"
namespace visionflow {
struct ReferenceResolution {
    ExecutionState state=ExecutionState::Skipped;
    std::optional<ReferenceAnchor> anchor;
    std::optional<ToolRoi> roi;
    QString reason;
};
class ReferenceService {
public:
    static ReferenceResolution current(const ToolDefinition&,const QString& port,const ToolResult&,const ImageFrame&);
    // nominal is the unoffset drawing. Reteaching preserves offsets exactly once.
    static ToolDefinition teach(const ToolDefinition& consumer,const ToolDefinition& source,const QString& port,
        const ToolResult&,const ImageFrame&,QPointF offset={},double angleOffsetRadians=0);
    static ReferenceResolution resolve(const ReferenceBinding&,const ToolDefinition&,const ToolResult&,const ImageFrame&);
    static ToolDefinition teachWorld(const ToolDefinition&,const ToolRoi&,double mmPerSample,const ToolDefinition&,const QString& port,
        const ToolResult&,const ImageFrame&,const MeasurementExecutionContext&,QPointF offset={},double angle=0,
        const std::shared_ptr<std::atomic_bool>& cancel={});
    static WorldTrackingResolution resolveWorld(const ReferenceBinding&,const ToolDefinition&,const ToolResult&,const ImageFrame&,
        const MeasurementExecutionContext&,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
