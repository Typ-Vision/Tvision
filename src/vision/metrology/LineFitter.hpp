#pragma once
#include "EdgeFinder.hpp"
namespace visionflow {
struct LineFitOptions {
    int minPoints=6;
    double minCoverage=.6,minSpanCoverage=.5,minSpan=10;
    double inlierDistance=1,maxRms=.35,maxResidual=1,minVarianceRatio=25;
    bool ransac=true;
    int ransacIterations=256;
};
struct LineScanPoint {
    std::optional<QPointF> point;
    std::optional<double> signedResidual;
    bool used=false;
    QString reason;
};
struct FittedLine {
    QPointF origin,tangent; // Infinite line; tangent follows the taught direction.
    QPointF supportStart,supportEnd; // Projections of the inlier extrema only.
    double angleDegrees=0,span=0,rms=0,maxResidual=0;
    double coverage=0,spanCoverage=0,varianceRatio=0;
    int usedCount=0,rejectedCount=0,missingCount=0;
    QVector<LineScanPoint> scans;
};
struct LineScanOptions {
    int scanCount=21;
    double scanWidthPx=2;
    EdgeOptions edge;
    LineFitOptions fit;
};
struct LineSearch {
    FittedLine line;
    FrameMetadata origin;
    QByteArray inputContentId,processingId;
};
class LineFitter {
public:
    // Points are in one caller-owned coordinate system; all metric options use
    // its units. One slot per requested scan, including missing observations.
    static FittedLine fit(const QVector<LineScanPoint>&,QPointF taughtTangent,const LineFitOptions& = {},
                          const std::shared_ptr<std::atomic_bool>& cancel={});
    // Raw pixel-only rectangular scan. World mapping belongs to T5.3.
    static LineSearch find(const ImageFrame&,const ToolRoi&,const LineScanOptions& = {},
                           const std::shared_ptr<std::atomic_bool>& cancel={});
    // Synthetic sampling plane; observations retain their nominal coordinates.
    static QVector<LineScanPoint> observationsMapped(const ImageFrame&,const RotatedRectangle&,const ProfileRawMap&,
        const LineScanOptions&,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
