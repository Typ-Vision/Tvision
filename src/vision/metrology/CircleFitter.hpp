#pragma once
#include "FitDiagnostic.hpp"
#include <QPointF>
#include <QString>
#include <QVector>
#include <atomic>
#include <memory>
#include <optional>

namespace visionflow {
struct CircleFitOptions {
    int minPoints=6;
    double minCoverage=.6,minAngularSpanDegrees=90,maxCondition=1000;
    double inlierDistance=1,maxRms=.35,maxResidual=1;
    bool ransac=true;
    int ransacIterations=256;
};
struct CircleFitPoint {
    std::optional<QPointF> point;
    std::optional<double> signedResidual; // distance to centre minus radius
    bool used=false;
    QString reason;
    ScanRejections rejections;
};
struct FittedCircle {
    QPointF centre;
    double radius=0,rms=0,maxResidual=0;
    double coverage=0,angularSpanDegrees=0,condition=0;
    int usedCount=0,rejectedCount=0,missingCount=0;
    QVector<CircleFitPoint> scans; // All observations retained, including outliers.
};
class CircleFitter {
public:
    // One slot per requested scan. All points and metric limits must be in the
    // same caller-owned plane (raw px or already mapped world mm). No mapping,
    // full-circle quality/defect metric, tolerance decision or provenance claim.
    static FittedCircle fit(const QVector<CircleFitPoint>&,const CircleFitOptions& = {},
                            const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
