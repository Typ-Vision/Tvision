#pragma once
#include "EdgeFinder.hpp"
namespace visionflow {
enum class RadialDirection { Outward, Inward };
struct RadialScanOptions {
    int scanCount=72;
    double scanWidth=1; // ROI coordinate units
    double unitsPerSample=1; // ROI units per synthetic pixel; raw mode requires 1
    RadialDirection direction=RadialDirection::Outward;
    EdgeOptions edge;
};
struct RadialScanPoint {
    int scanId=0;
    double angleDegrees=0; // unwrapped start + signed sweep fraction
    ScanSegment nominalScan;
    QPointF rawStart,rawEnd;
    std::optional<QPointF> point,rawPoint; // ROI units and raw px respectively
    std::optional<double> strength,signedStrength;
    QString reason,refinement;
};
struct RadialSearch {
    QVector<RadialScanPoint> scans;
    int foundCount=0,missingCount=0;
    bool fullCircle=false;
    double coverage=0,maxGapDegrees=0;
    FrameMetadata origin;
    QByteArray inputContentId,processingId;
};
class RadialScanner {
public:
    static RadialSearch find(const ImageFrame&,const ToolRoi&,const RadialScanOptions& = {},
                            const std::shared_ptr<std::atomic_bool>& cancel={});
    // Geometric sampling only: caller supplies domain-checked ROI-unit -> raw map.
    // Does not claim calibration acceptance, fit, residual or part decision.
    static RadialSearch findMapped(const ImageFrame&,const ArcRoi&,const ProfileRawMap&,const RadialScanOptions&,
                                  const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
