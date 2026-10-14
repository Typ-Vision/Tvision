#pragma once
#include "Coordinates.hpp"
#include <QSizeF>
#include <QRectF>
#include <QLineF>
#include <variant>
namespace visionflow {
struct ScanSegment {QPointF start,end;double width=1;bool operator==(const ScanSegment&) const=default;};
struct RotatedRectangle {QPointF centre;QSizeF size;double angleDegrees=0;bool operator==(const RotatedRectangle&) const=default;};
struct Annulus {QPointF centre;double innerRadius=1,outerRadius=2;bool operator==(const Annulus&) const=default;};
struct ArcRoi {Annulus ring;double startDegrees=0,sweepDegrees=90;bool operator==(const ArcRoi&) const=default;};
struct PolygonRoi {QVector<QPointF> vertices;bool operator==(const PolygonRoi&) const=default;};
struct PathRoi {QVector<QPointF> vertices;bool operator==(const PathRoi&) const=default;};
using RoiGeometry=std::variant<ScanSegment,RotatedRectangle,Annulus,ArcRoi,PolygonRoi,PathRoi>;
struct ToolRoi {
    CoordinateFrame frame;
    RoiGeometry geometry;
    bool operator==(const ToolRoi&) const=default;
    // No clipping or AABB replacement. Pixel centres span [0,w-1] × [0,h-1].
    void validate() const;
    QRectF bounds() const;
    bool inside(const QSize& image) const;
    QJsonObject toJson() const;
    static ToolRoi fromJson(const QJsonObject&);
};
}
