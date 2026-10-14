#include "domain/GeometryMeasurements.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace visionflow;
using G=GeometryMeasurements;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double actual,double expected,double tolerance=1e-10){check(std::isfinite(actual) && std::abs(actual-expected)<=tolerance,"analytic geometry oracle");}
template<class F> void reject(F f){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}check(failed,"invalid geometry must reject");}
}
int main() {
    try {
        for(auto unit:{LengthUnit::Pixel,LengthUnit::Millimetre})
        for(auto axes:{AxisConvention::XRightYDown,AxisConvention::XRightYUp})
        for(double scale:{1e-8,1.0,1e8})
        for(double origin:{0.0,1000.0}) {
            CoordinateFrame f{"test/plane",unit,axes};
            auto p=[&](double x,double y){return Point2D{{origin+x*scale,origin+y*scale},f};};
            auto l=[&](double x1,double y1,double x2,double y2){return Line2D{p(x1,y1).position,p(x2,y2).position,f};};
            // Allow only input quantization at translated submicron coordinates.
            const double tol=1e-10*scale+5e-13;
            near(G::pointDistance(p(0,0),p(3,4)),5*scale,tol);
            near(G::pointDistance(p(3,4),p(0,0)),5*scale,tol);
            near(G::pointDistance(p(3,4),p(3,4)),0);
            near(G::xOffset(p(3,4),p(0,0),OffsetMode::Signed),-3*scale,tol);
            near(G::yOffset(p(3,4),p(0,0),OffsetMode::Absolute),4*scale,tol);
            near(G::yOffset(p(3,4),p(0,0),OffsetMode::Signed),-4*scale,tol);
            const auto horizontal=l(0,0,2,0),reverse=l(2,0,0,0),vertical=l(1,-2,1,2);
            near(G::pointLineDistance(p(3,4),horizontal,OffsetMode::Signed),4*scale,tol);
            near(G::pointLineDistance(p(3,4),reverse,OffsetMode::Signed),-4*scale,tol);
            near(G::pointLineDistance(p(3,4),reverse,OffsetMode::Absolute),4*scale,tol);
            near(G::pointSegmentDistance(p(5,4),horizontal),5*scale,tol);
            near(G::pointSegmentDistance(p(-3,4),horizontal),5*scale,tol);
            near(G::pointSegmentDistance(p(1,4),horizontal),4*scale,tol);
            near(G::pointSegmentDistance(p(2,0),horizontal),0);
            near(G::lineAngleDegrees(horizontal,vertical,AngleMode::SignedDirected),90);
            near(G::lineAngleDegrees(vertical,horizontal,AngleMode::SignedDirected),-90);
            near(G::lineAngleDegrees(horizontal,reverse,AngleMode::UnsignedDirected),180);
            near(G::lineAngleDegrees(horizontal,reverse,AngleMode::AcuteUndirected),0);
            near(G::lineAngleDegrees(horizontal,reverse,AngleMode::SignedDirected),180);
            auto intersection=G::lineIntersection(horizontal,vertical);
            check(intersection.kind==LineIntersectionKind::Unique && intersection.point.has_value(),"unique supporting lines");
            near(G::pointDistance(*intersection.point,p(1,0)),0,tol);
            // Intersection outside support segment still belongs to infinite line.
            intersection=G::lineIntersection(horizontal,l(4,2,4,3));
            near(G::pointDistance(*intersection.point,p(4,0)),0,tol);
            check(G::lineIntersection(horizontal,reverse).kind==LineIntersectionKind::Coincident,"antiparallel coincident");
            const auto parallel=l(9,3,4,3);
            intersection=G::lineIntersection(horizontal,parallel);
            check(intersection.kind==LineIntersectionKind::Parallel && !intersection.point,"parallel no fabricated point");
            near(G::parallelLineDistance(horizontal,parallel,OffsetMode::Signed),3*scale,tol);
            near(G::parallelLineDistance(reverse,parallel,OffsetMode::Signed),-3*scale,tol);
            near(G::parallelLineDistance(reverse,parallel,OffsetMode::Absolute),3*scale,tol);
            Circle2D a{p(0,0).position,2*scale,f},b{p(3,4).position,7*scale,f};
            near(G::circleCentreDistance(a,b),5*scale,tol);
            near(G::circleSize(a,CircleSize::Radius),2*scale);
            near(G::circleSize(a,CircleSize::Diameter),4*scale);
        }
        CoordinateFrame f{"plane",LengthUnit::Millimetre,AxisConvention::XRightYUp};
        Line2D x{{0,0},{1,0},f};Point2D p{{3,4},f};Circle2D c{{0,0},2,f};
        near(G::pointDistance({{1e9,1e9},f},{{1e9+3,1e9+4},f}),5);
        near(G::pointDistance({{0,0},f},{{3e200,4e200},f}),5e200,1e185); // hypot, no squared overflow
        near(G::pointLineDistance(p,{{0,0},{3,4},f},OffsetMode::Signed),0);
        near(G::pointLineDistance({{0,5},f},{{0,0},{3,4},f},OffsetMode::Signed),3);
        near(G::lineAngleDegrees(x,{{0,0},{-1,1},f},AngleMode::UnsignedDirected),135);
        near(G::lineAngleDegrees(x,{{0,0},{-1,1},f},AngleMode::AcuteUndirected),45);
        for(auto field:{0,1,2}) {
            auto wrong=f;if(field==0)wrong.id="foreign";if(field==1)wrong.unit=LengthUnit::Pixel;if(field==2)wrong.axes=AxisConvention::XRightYDown;
            Point2D q{p.position,wrong};Line2D y{x.start,x.end,wrong};Circle2D d{c.centre,c.radius,wrong};
            reject([&]{G::pointDistance(p,q);});reject([&]{G::xOffset(p,q,OffsetMode::Signed);});
            reject([&]{G::yOffset(p,q,OffsetMode::Signed);});reject([&]{G::pointLineDistance(q,x,OffsetMode::Signed);});
            reject([&]{G::pointSegmentDistance(q,x);});reject([&]{G::lineAngleDegrees(x,y,AngleMode::SignedDirected);});
            reject([&]{G::lineIntersection(x,y);});reject([&]{G::parallelLineDistance(x,y,OffsetMode::Signed);});
            reject([&]{G::circleCentreDistance(c,d);});
        }
        for(double bad:{0.0,1e-13,G::minimumLineLength}) {
            Line2D d{{0,0},{bad,0},f};
            reject([&]{G::pointLineDistance(p,d,OffsetMode::Signed);});reject([&]{G::pointSegmentDistance(p,d);});
            reject([&]{G::lineAngleDegrees(x,d,AngleMode::SignedDirected);});reject([&]{G::lineIntersection(x,d);});
            reject([&]{G::parallelLineDistance(x,d,OffsetMode::Signed);});
        }
        const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity(),max=std::numeric_limits<double>::max();
        for(double bad:{nan,inf,-inf}) {
            reject([&]{G::pointDistance(p,{{bad,0},f});});reject([&]{G::xOffset({{0,bad},f},p,OffsetMode::Signed);});
            reject([&]{G::lineIntersection(x,{{0,0},{1,bad},f});});reject([&]{G::circleSize({{0,0},bad,f},CircleSize::Radius);});
            reject([&]{G::circleCentreDistance(c,{{bad,0},2,f});});
        }
        reject([&]{G::pointDistance({{-max,0},f},{{max,0},f});});
        reject([&]{G::circleSize({{0,0},max,f},CircleSize::Diameter);});
        reject([&]{G::circleSize({{0,0},0,f},CircleSize::Radius);});
        reject([&]{G::circleSize({{0,0},-1,f},CircleSize::Radius);});
        reject([&]{G::parallelLineDistance(x,{{0,0},{0,1},f},OffsetMode::Absolute);});
        reject([&]{G::xOffset(p,p,static_cast<OffsetMode>(99));});
        reject([&]{G::lineAngleDegrees(x,x,static_cast<AngleMode>(99));});
        reject([&]{G::circleSize(c,static_cast<CircleSize>(99));});
        auto badFrame=f;badFrame.unit=static_cast<LengthUnit>(99);
        reject([&]{G::pointDistance({{0,0},badFrame},{{1,1},badFrame});});
        badFrame=f;badFrame.id.clear();reject([&]{G::circleSize({{0,0},1,badFrame},CircleSize::Radius);});
        const double epsilon=G::parallelSineTolerance;
        check(G::lineIntersection(x,{{1e12,1},{1e12+1,1},f}).kind==LineIntersectionKind::Parallel,"large tangent displacement must not hide unit gap");
        Line2D justLong{{0,0},{std::nextafter(G::minimumLineLength,inf),0},f};
        near(G::pointLineDistance(p,justLong,OffsetMode::Signed),4);
        for(double degrees:{-170.0,-100.0,-45.0,0.0,30.0,80.0,179.0}) {
            const double r=degrees*std::acos(-1.0)/180,co=std::cos(r),si=std::sin(r);
            Line2D rotated{{12,-7},{12+3*co,-7+3*si},f};
            near(G::lineAngleDegrees(x,rotated,AngleMode::SignedDirected),degrees);
            Point2D normal{{12-5*si,-7+5*co},f};
            near(G::pointLineDistance(normal,rotated,OffsetMode::Signed),5);
            near(G::pointSegmentDistance(normal,rotated),5);
            Line2D crossing{{12-2*si,-7+2*co},{12+2*si,-7-2*co},f};
            auto hit=G::lineIntersection(rotated,crossing);
            check(hit.kind==LineIntersectionKind::Unique && hit.point.has_value(),"rotated lines unique");
            near(G::pointDistance(*hit.point,{{12,-7},f}),0);
        }
        check(G::lineIntersection(x,{{0,1},{1,1+epsilon/2},f}).kind==LineIntersectionKind::Parallel,"near parallel conservative gate");
        check(G::lineIntersection(x,{{0,1},{1,1+epsilon*2},f}).kind==LineIntersectionKind::Unique,"above parallel gate");
        near(G::parallelLineDistance(x,{{0,epsilon/2},{1,epsilon/2},f},OffsetMode::Signed),epsilon/2,0);
        near(G::pointLineDistance({{0,-1e-14},f},x,OffsetMode::Signed),-1e-14,0); // No value snapping.
        std::cout<<"geometry measurements: "<<checks<<" checks passed\n";
    }catch(const std::exception& e){std::cerr<<"after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}
}
