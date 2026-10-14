#include "GeometryMeasurements.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* why) {if(!ok)throw std::runtime_error(why);}
double finite(double v) {require(std::isfinite(v),"Geometry result/input must be finite");return v;}
QPointF finite(QPointF p) {finite(p.x());finite(p.y());return p;}
void frame(const CoordinateFrame& f) {
    f.validate();
    require((f.unit==LengthUnit::Pixel || f.unit==LengthUnit::Millimetre) &&
            (f.axes==AxisConvention::XRightYDown || f.axes==AxisConvention::XRightYUp),"Invalid geometry frame enum");
}
void same(const CoordinateFrame& a,const CoordinateFrame& b) {
    frame(a);frame(b);require(a==b,"Geometry frame/unit/axes mismatch");
}
void point(const Point2D& p) {frame(p.frame);finite(p.position);}
double norm(QPointF p) {return finite(std::hypot(p.x(),p.y()));}
QPointF delta(QPointF a,QPointF b) {return finite(b-a);}
QPointF direction(const Line2D& l) {
    frame(l.frame);finite(l.start);finite(l.end);
    const auto d=delta(l.start,l.end);const double length=norm(d);
    require(length>GeometryMeasurements::minimumLineLength,"Degenerate/too short line");
    return d/length;
}
double cross(QPointF a,QPointF b) {return finite(a.x()*b.y()-a.y()*b.x());}
double dot(QPointF a,QPointF b) {return finite(a.x()*b.x()+a.y()*b.y());}
double offset(double v,OffsetMode mode) {
    require(mode==OffsetMode::Signed || mode==OffsetMode::Absolute,"Invalid offset mode");
    v=finite(v);return v==0 ? 0 : mode==OffsetMode::Absolute ? std::abs(v) : v;
}
void circle(const Circle2D& c) {frame(c.frame);finite(c.centre);require(finite(c.radius)>0,"Circle radius must be positive");}
}
double GeometryMeasurements::pointDistance(const Point2D& a,const Point2D& b) {
    point(a);point(b);same(a.frame,b.frame);return norm(delta(a.position,b.position));
}
double GeometryMeasurements::xOffset(const Point2D& a,const Point2D& b,OffsetMode m) {
    point(a);point(b);same(a.frame,b.frame);return offset(finite(b.position.x()-a.position.x()),m);
}
double GeometryMeasurements::yOffset(const Point2D& a,const Point2D& b,OffsetMode m) {
    point(a);point(b);same(a.frame,b.frame);return offset(finite(b.position.y()-a.position.y()),m);
}
double GeometryMeasurements::pointLineDistance(const Point2D& p,const Line2D& l,OffsetMode m) {
    point(p);same(p.frame,l.frame);const auto u=direction(l);
    return offset(cross(u,delta(l.start,p.position)),m);
}
double GeometryMeasurements::pointSegmentDistance(const Point2D& p,const Line2D& l) {
    point(p);same(p.frame,l.frame);const auto u=direction(l),d=delta(l.start,p.position);
    const double projection=dot(d,u),length=norm(delta(l.start,l.end));
    if(projection<=0)return norm(d);
    if(projection>=length)return norm(delta(l.end,p.position));
    return std::abs(cross(u,d));
}
double GeometryMeasurements::lineAngleDegrees(const Line2D& a,const Line2D& b,AngleMode mode) {
    same(a.frame,b.frame);const auto u=direction(a),v=direction(b);
    double degrees=finite(std::atan2(cross(u,v),dot(u,v))*180/std::numbers::pi);
    if(degrees==-180)degrees=180; // Directed range (-180,180].
    if(mode==AngleMode::SignedDirected)return degrees==0 ? 0 : degrees;
    require(mode==AngleMode::UnsignedDirected || mode==AngleMode::AcuteUndirected,"Invalid angle mode");
    degrees=std::abs(degrees);
    return mode==AngleMode::UnsignedDirected ? degrees : std::min(degrees,180-degrees);
}
LineIntersection GeometryMeasurements::lineIntersection(const Line2D& a,const Line2D& b) {
    same(a.frame,b.frame);const auto u=direction(a),v=direction(b),d=delta(a.start,b.start);
    const double determinant=cross(u,v);
    if(std::abs(determinant)<=parallelSineTolerance) {
        // Scale-aware numerical coincidence, not a metrology acceptance tolerance.
        const double tolerance=minimumLineLength+16*std::numeric_limits<double>::epsilon()*norm(d);
        return {std::abs(cross(u,d))<=tolerance ? LineIntersectionKind::Coincident : LineIntersectionKind::Parallel,std::nullopt};
    }
    const double along=finite(cross(d,v)/determinant);
    return {LineIntersectionKind::Unique,Point2D{finite(a.start+u*along),a.frame}};
}
double GeometryMeasurements::parallelLineDistance(const Line2D& a,const Line2D& b,OffsetMode m) {
    same(a.frame,b.frame);const auto u=direction(a),v=direction(b);
    require(std::abs(cross(u,v))<=parallelSineTolerance,"Parallel spacing requires parallel lines");
    return offset(cross(u,delta(a.start,b.start)),m);
}
double GeometryMeasurements::circleCentreDistance(const Circle2D& a,const Circle2D& b) {
    circle(a);circle(b);same(a.frame,b.frame);return norm(delta(a.centre,b.centre));
}
double GeometryMeasurements::circleSize(const Circle2D& c,CircleSize size) {
    circle(c);require(size==CircleSize::Radius || size==CircleSize::Diameter,"Invalid circle size selector");
    return finite(size==CircleSize::Radius ? c.radius : 2*c.radius);
}
}
