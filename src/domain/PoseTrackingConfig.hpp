#pragma once
#include "ToolDefinition.hpp"
#include "PoseRoi.hpp"
namespace visionflow {
// Separate world teaching record: never stores world geometry in raw local ROI.
struct WorldPoseTracking {
    Pose2D taughtPose;
    QString sourceTool,geometryId,imageSource,roiIdentity;
    QSize rawSize;
    CalibrationDependency calibration;
    double mmPerSample=1;
    void validate() const;
    QJsonObject toJson() const;
    static WorldPoseTracking fromJson(const QJsonObject&);
};
bool isWorldPoseTracking(const ToolDefinition&);
WorldPoseTracking worldPoseTracking(const ToolDefinition&);
QString trackingSourceTool(const ToolDefinition&);
bool supportsPoseTracking(const QString& kind);
QString scanRoiIdentity(const ToolDefinition& tool);
ToolRoi nominalTrackingRoi(const ToolDefinition& tool,const CoordinateFrame& frame);
// Optional opt-in parameter; absent on legacy recipes and defaults.
PoseRoi poseTrackingBinding(const ToolDefinition& tool);
}
