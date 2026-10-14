#pragma once
#include "ToolValues.hpp"
#include <optional>

namespace visionflow {
enum class OffsetMode { Signed, Absolute };
enum class AngleMode { SignedDirected, UnsignedDirected, AcuteUndirected };
enum class CircleSize { Radius, Diameter };
enum class LineIntersectionKind { Unique, Parallel, Coincident };
struct LineIntersection {
    LineIntersectionKind kind;
    std::optional<Point2D> point; // Present only for Unique; infinite supporting lines.
};
// Pure geometry in one explicit frame; no calibration, acquisition or decisions.
// Invalid inputs / unrepresentable results throw std::runtime_error.
class GeometryMeasurements {
public:
    static constexpr double minimumLineLength=1e-12;
    static constexpr double parallelSineTolerance=1e-12;
    static double pointDistance(const Point2D&,const Point2D&);
    static double xOffset(const Point2D& from,const Point2D& to,OffsetMode);
    static double yOffset(const Point2D& from,const Point2D& to,OffsetMode);
    // Positive on the +90 degree side of start->end in the frame's axes.
    static double pointLineDistance(const Point2D&,const Line2D&,OffsetMode);
    // Unsigned distance to the closed segment, clamping projection to its ends.
    static double pointSegmentDistance(const Point2D&,const Line2D&);
    static double lineAngleDegrees(const Line2D& from,const Line2D& to,AngleMode);
    static LineIntersection lineIntersection(const Line2D&,const Line2D&);
    // Signed relative to the first line's direction; second direction irrelevant.
    static double parallelLineDistance(const Line2D&,const Line2D&,OffsetMode);
    static double circleCentreDistance(const Circle2D&,const Circle2D&);
    static double circleSize(const Circle2D&,CircleSize);
};
}
