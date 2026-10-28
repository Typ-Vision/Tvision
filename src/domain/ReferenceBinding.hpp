#pragma once
#include "vision/ImageGeometry.hpp"
#include "PoseRoi.hpp"
#include <variant>

namespace visionflow {
enum class ReferenceMode { Translation, Rigid };
using ReferenceAnchor=std::variant<Point2D,Pose2D>;

// Opt-in domain record; tool parameters require editor schema 8.
// Base ROI is always unoffset raw geometry; reusing it never accumulates offsets.
struct ReferenceBinding {
    ReferenceMode mode=ReferenceMode::Translation;
    QString sourceTool,sourcePort,geometryId,imageSource;
    ToolRoi nominal;
    ReferenceAnchor taught;
    QSize rawSize;
    QPointF offsetPx;
    double angleOffsetRadians=0;
    std::optional<CalibrationDependency> calibration;
    // Schema 2 world-reference-v1 keeps the exact nominal mm geometry.
    bool world=false;
    double mmPerSample=1;

    std::optional<ImageGeometry> imageGeometry;
    bool invalidated=false;
    void validate() const;
    QString roiIdentity() const;
    // Point input stays Point; mode/type mismatch never manufactures a pose.
    Transform2D rawTransform(const ReferenceAnchor& current,double scale,const QSize&) const;
    ToolRoi resolve(const ReferenceAnchor& current,double scale,const QSize&) const;
    QJsonObject toJson() const;
    static ReferenceBinding fromJson(const QJsonObject&);
};
}
