#pragma once
#include "ExecutionService.hpp"
#include "domain/PoseRoi.hpp"
#include "domain/PoseTrackingConfig.hpp"
namespace visionflow {
struct PoseReferenceResolution {ExecutionState state;std::optional<Pose2D> pose;QString reason;};
struct PoseRoiResolution {
    ExecutionState state=ExecutionState::Skipped;
    std::optional<ToolRoi> roi;
    QString reason;
};
struct WorldTrackingMap {
    // Owns a validated snapshot; each call checks cancellation and mapping domain.
    std::function<QPointF(QPointF)> nominalToImage;
    std::function<QPointF(QPointF)> imageToWorld;
    std::function<void(const ImageFrame&)> requireImage;
    // Compatibility for UI callers; EFR-03b migrates these names.
    QPointF nominalToRaw(QPointF p) const {return nominalToImage(p);}
    QPointF rawToWorld(QPointF p) const {return imageToWorld(p);}
    CoordinateFrame world;
    CalibrationDependency calibration;
};
struct WorldTrackingResolution {
    ExecutionState state=ExecutionState::Skipped;
    std::optional<WorldTrackingMap> map;
    QString reason;
};
class PoseRoiService {
public:
    static WorldTrackingMap calibratedMap(const CalibrationDependency&,const Transform2D&,const ImageFrame&,
        const MeasurementExecutionContext&,const std::shared_ptr<std::atomic_bool>& cancel={});
    static PoseReferenceResolution currentReference(const ToolDefinition&,const ToolResult&,const ImageFrame&);
    static ToolDefinition teachTool(const ToolDefinition& consumer,const CoordinateFrame& local,
        const ToolDefinition& source,const ToolResult&,const ImageFrame&);
    static WorldTrackingResolution resolveWorld(const WorldPoseTracking&,const ToolDefinition&,const ToolResult&,const ImageFrame&,
        const MeasurementExecutionContext&,const std::shared_ptr<std::atomic_bool>& cancel={});
    static ToolDefinition teachWorldTool(const ToolDefinition& consumer,const ToolRoi& nominalWorld,double mmPerSample,
        const ToolDefinition& source,const ToolResult&,const ImageFrame&,const MeasurementExecutionContext&,
        const std::shared_ptr<std::atomic_bool>& cancel={});
    static WorldTrackingMap worldMap(const WorldPoseTracking&,const ToolDefinition&,const ToolResult&,const ImageFrame&,
        const MeasurementExecutionContext&,const std::shared_ptr<std::atomic_bool>& cancel={});
    // Texture and shape have distinct method/geometry identities; no legacy Point conversion.
    static QString geometryIdentity(const ToolDefinition& source);
    static PoseRoi teach(const ToolRoi& raw,const CoordinateFrame& local,const ToolDefinition& source,
                         const ToolResult&,const ImageFrame&);
    static PoseRoiResolution resolve(const PoseRoi&,const ToolDefinition& source,const ToolResult&,const ImageFrame&);
    static WorldTrackingMap worldMap(const PoseRoi&,const ToolDefinition&,const ToolResult&,const ImageFrame&,
        const MeasurementExecutionContext&,const std::shared_ptr<std::atomic_bool>& cancel={});
    // Explicit perspective chain: H^-1(nominalWorld) -> taught raw -> current raw -> H(currentWorld).
    // Returns points, never pretends a projective image of a circle is a circle.
    static QVector<Point2D> worldPoints(const PoseRoi&,const ToolDefinition&,const ToolResult&,const ImageFrame&,
        const MeasurementExecutionContext&,const QVector<Point2D>& nominalWorld,
        const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
