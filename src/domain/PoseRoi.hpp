#pragma once
#include "ToolRoi.hpp"
#include <optional>
namespace visionflow {
// Shared rigid mapping preserves the actual geometry, including signed arcs.
ToolRoi mapRigidRoi(const ToolRoi&,const Transform2D&);
// Rigid, model-local geometry. Matching scale is deliberately excluded.
struct PoseRoi {
    ToolRoi local;
    Pose2D taughtPose;
    QString sourceTool,geometryId;
    QSize rawSize;
    QString imageSource;
    std::optional<CalibrationDependency> calibration;
    void validate() const;
    static PoseRoi teach(const ToolRoi& raw,const Pose2D&,const CoordinateFrame& local,
                         const QString& sourceTool,const QString& geometryId,const QSize& rawSize,const QString& imageSource);
    ToolRoi resolve(const Pose2D&,double scale,const QSize&) const;
    QJsonObject toJson() const;
    static PoseRoi fromJson(const QJsonObject&);
};
}
