#pragma once
#include "CircleFitter.hpp"
#include "RadialScanner.hpp"
#include <QStringList>

namespace visionflow {
enum class CircleMetricScope { Unavailable, ObservedArc, FullCircle };
struct CircleQualityOptions {
    int minFoundPoints=6;
    double minCoverage=.9,maxGapDegrees=15;
};
struct RadialDeviation {
    double minimumRadius=0,maximumRadius=0,peakToValley=0;
    double maxInward=0,maxOutward=0;
};
struct ClosedContourMetrics {
    double area=0,perimeter=0,circularityIndex=0; // 4*pi*A/P^2, actual polygon
};
struct ClosedContourResult {
    std::optional<ClosedContourMetrics> metrics;
    QString unavailableReason;
};
struct CircleMetricSample {
    int scanId=0;
    double angleDegrees=0;
    std::optional<QPointF> point;
    std::optional<double> radialDistance,signedResidual;
    bool fitUsed=false;
};
struct CircleMetricResult {
    QPointF centre;
    double radius=0,diameter=0,fitRms=0,fitMaxResidual=0;
    int fitUsedCount=0,fitRejectedCount=0,foundCount=0,missingCount=0;
    double coverage=0,maxGapDegrees=0;
    bool fullCircle=false;
    RadialDeviation observedDeviation; // Diagnostic, even if quality fails.
    CircleMetricScope scope=CircleMetricScope::Unavailable;
    std::optional<RadialDeviation> deviation; // Qualified sampled full ring/arc.
    QStringList qualityReasons;
    ClosedContourResult contour;
    QVector<CircleMetricSample> samples;
    FrameMetadata origin;
    QByteArray inputContentId,processingId;
};
class CircleMetrics {
public:
    // Exact same ordered nominal-plane points as the fit; raw/world mapping and
    // calibration acceptance belong to the caller. No tolerance/part decision.
    static CircleMetricResult measure(const RadialSearch&,const FittedCircle&,
                                     const CircleQualityOptions& = {},
                                     const std::shared_ptr<std::atomic_bool>& cancel={});
    // Explicit closed polygon, without a repeated first vertex. Does not fit or
    // infer closure across missing scans. Topology failure returns unavailable.
    static ClosedContourResult closedContour(const QVector<QPointF>&,
                                             const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
