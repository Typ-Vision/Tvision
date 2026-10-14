#pragma once
#include "PoseRoiService.hpp"
#include "vision/metrology/EdgePairFinder.hpp"
#include "vision/metrology/LineFitter.hpp"
#include "vision/metrology/RadialScanner.hpp"
#include "vision/metrology/CircleFitter.hpp"
#include "vision/metrology/CircleMetrics.hpp"
namespace visionflow {
struct TrackedWorldPoint {QPointF raw,world;};
struct TrackedWorldPair {TrackedWorldPoint first,second;double widthMm=0;};
struct TrackedWorldLine {QVector<LineScanPoint> raw;FittedLine world;};
struct TrackedWorldCircle {RadialSearch observations;FittedCircle world;};
struct TrackedWorldCircleMetrics {CircleMetricResult metrics;std::optional<Arc2D> arc;};
// Programmatic sampling/refit, world metrics and tool tolerance. No production
// Runtime authorization or part decisions. Supply a validated worldMap snapshot.
class WorldTrackingService {
public:
    // Radial metrics use all actual observations, including fit outliers.
    // Arc endpoints and angular gaps use current world angles, never nominal rays.
    static TrackedWorldCircleMetrics metrics(const TrackedWorldCircle&,const CoordinateFrame&,
        const CircleQualityOptions&,const std::shared_ptr<std::atomic_bool>& cancel={});
    static void preflight(const ToolDefinition&,const ImageFrame&,const WorldTrackingMap&,const std::shared_ptr<std::atomic_bool>& cancel={});
    static ToolResult execute(const ToolDefinition&,const ImageFrame&,const WorldTrackingMap&,const std::shared_ptr<std::atomic_bool>& cancel={});
    static std::optional<TrackedWorldPoint> edge(const ImageFrame&,const ToolRoi& nominalWorld,double mmPerSample,
        const WorldTrackingMap&,const EdgeOptions& = {},const std::shared_ptr<std::atomic_bool>& cancel={});
    static std::optional<TrackedWorldPair> pair(const ImageFrame&,const ToolRoi& nominalWorld,double mmPerSample,
        const WorldTrackingMap&,const EdgePairOptions& = {},const std::shared_ptr<std::atomic_bool>& cancel={});
    // scanWidthPx and profile options use synthetic sample units; fit limits use mm.
    static TrackedWorldLine line(const ImageFrame&,const ToolRoi& nominalWorld,double mmPerSample,
        const WorldTrackingMap&,const LineScanOptions&,const std::shared_ptr<std::atomic_bool>& cancel={});
    // scanWidth/unitsPerSample use nominal mm, profile options synthetic units.
    // Observed points are current world mm; rawPoint remains raw px. Nominal
    // scan angles describe sampling coverage, never a current world arc angle.
    static TrackedWorldCircle circle(const ImageFrame&,const ToolRoi& nominalWorld,const WorldTrackingMap&,
        const RadialScanOptions&,const CircleFitOptions&,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
