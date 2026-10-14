#include "CircleMetrics.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void stop(const std::shared_ptr<std::atomic_bool>& cancel){if(cancel && cancel->load())throw std::runtime_error("Circle metrics cancelled");}
bool finite(QPointF p){return std::isfinite(p.x()) && std::isfinite(p.y()) && std::abs(p.x())<=1e12 && std::abs(p.y())<=1e12;}
double norm(QPointF p){return std::hypot(p.x(),p.y());}
long double cross(QPointF a,QPointF b){return (long double)a.x()*b.y()-(long double)a.y()*b.x();}
}
ClosedContourResult CircleMetrics::closedContour(const QVector<QPointF>& points,const std::shared_ptr<std::atomic_bool>& cancel) {
    stop(cancel);require(points.size()<=4096,"Contour point budget exceeded");
    for(auto p:points){stop(cancel);require(finite(p),"Invalid contour point");}
    if(points.size()<3)return {{},"insufficient_contour_points"};
    const auto base=points.front();double scale=0;for(auto p:points)scale=std::max(scale,norm(p-base));
    if(scale<=1e-12)return {{},"degenerate_contour"};
    QVector<QPointF> q;for(auto p:points)q.append((p-base)/scale);
    constexpr long double epsilon=1e-12L;const int n=int(q.size());
    auto intersects=[&](QPointF a,QPointF b,QPointF c,QPointF d){
        if(std::max(a.x(),b.x())+epsilon<std::min(c.x(),d.x()) || std::max(c.x(),d.x())+epsilon<std::min(a.x(),b.x()) ||
           std::max(a.y(),b.y())+epsilon<std::min(c.y(),d.y()) || std::max(c.y(),d.y())+epsilon<std::min(a.y(),b.y()))return false;
        auto side=[=](long double v){return v>epsilon ? 1:(v<-epsilon ? -1:0);};
        const int abC=side(cross(b-a,c-a)),abD=side(cross(b-a,d-a)),cdA=side(cross(d-c,a-c)),cdB=side(cross(d-c,b-c));
        return abC*abD<=0 && cdA*cdB<=0;
    };
    long double twiceArea=0,perimeter=0;
    for(int i=0;i<n;++i){stop(cancel);const auto a=q[i],b=q[(i+1)%n],c=q[(i+2)%n];double length=norm(b-a);
        if(length<=epsilon)return {{},"degenerate_contour_edge"};
        if(std::abs(cross(b-a,c-b))<=epsilon && QPointF::dotProduct(b-a,c-b)<0)return {{},"overlapping_contour_edges"};
        twiceArea+=cross(a,b);perimeter+=length;
        // Adjacent edges share only their endpoint; all other intersections,
        // contacts and near contacts are conservatively rejected.
        for(int j=i+2;j<n;++j){stop(cancel);if(i==0 && j==n-1)continue;
            if(intersects(a,b,q[j],q[(j+1)%n]))return {{},"self_intersecting_contour"};}
    }
    const long double area=std::abs(twiceArea)/2;
    if(area<=epsilon || perimeter<=epsilon)return {{},"degenerate_contour_area"};
    ClosedContourMetrics r;r.area=double(area*scale*scale);r.perimeter=double(perimeter*scale);
    r.circularityIndex=double(4*std::numbers::pi_v<long double>*area/(perimeter*perimeter));
    require(std::isfinite(r.area) && std::isfinite(r.perimeter) && std::isfinite(r.circularityIndex),"Nonfinite contour metrics");
    stop(cancel);return {r,{}};
}
CircleMetricResult CircleMetrics::measure(const RadialSearch& search,const FittedCircle& fit,const CircleQualityOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    stop(cancel);require(o.minFoundPoints>=6 && o.minFoundPoints<=4096 && std::isfinite(o.minCoverage) && o.minCoverage>0 && o.minCoverage<=1 &&
        std::isfinite(o.maxGapDegrees) && o.maxGapDegrees>0 && o.maxGapDegrees<=360,"Invalid circle quality options");
    const int n=int(search.scans.size());require(n>=6 && n<=4096 && fit.scans.size()==n,"Circle metric support mismatch");
    require(finite(fit.centre) && std::isfinite(fit.radius) && fit.radius>0 && fit.radius<=1e12,"Invalid circle metric model");
    const double start=search.scans[0].angleDegrees,step=search.scans[1].angleDegrees-start;
    const double sweep=std::abs(step)*(search.fullCircle ? n:n-1);
    require(std::isfinite(start) && std::abs(start)<=360 && std::isfinite(step) && sweep>1e-9 && sweep<=360+1e-8 &&
        (search.fullCircle ? std::abs(sweep-360)<=1e-8:sweep<360),"Invalid nominal circle scan angles");
    CircleMetricResult r;r.centre=fit.centre;r.radius=fit.radius;r.diameter=2*fit.radius;r.fullCircle=search.fullCircle;
    r.origin=search.origin;r.inputContentId=search.inputContentId;r.processingId=search.processingId;
    double minimum=std::numeric_limits<double>::infinity(),maximum=0,fitSum=0;int first=-1,last=-1;QVector<QPointF> contour;
    for(int i=0;i<n;++i){stop(cancel);const auto& s=search.scans[i];const auto& f=fit.scans[i];
        require(s.scanId==i && std::isfinite(s.angleDegrees) && std::abs(s.angleDegrees-(start+i*step))<=1e-8,"Unordered/nonuniform circle scans");
        const bool samePoint=bool(s.point)==bool(f.point) && (!s.point ||
            (s.point->x()==f.point->x() && s.point->y()==f.point->y()));
        require(samePoint && (!f.used || bool(s.point)),"Circle fit/measurement point mismatch");
        CircleMetricSample sample;sample.scanId=i;sample.angleDegrees=s.angleDegrees;sample.point=s.point;sample.fitUsed=f.used;
        if(s.point){require(finite(*s.point),"Invalid circle measurement point");const double distance=norm(*s.point-fit.centre),residual=distance-fit.radius;
            sample.radialDistance=distance;sample.signedResidual=residual;minimum=std::min(minimum,distance);maximum=std::max(maximum,distance);
            if(f.used){++r.fitUsedCount;fitSum+=residual*residual;r.fitMaxResidual=std::max(r.fitMaxResidual,std::abs(residual));}else ++r.fitRejectedCount;
            ++r.foundCount;contour.append(*s.point);if(first<0)first=i;if(last>=0)r.maxGapDegrees=std::max(r.maxGapDegrees,(i-last)*std::abs(step));last=i;
        }else ++r.missingCount;
        r.samples.append(sample);
    }
    require(r.fitUsedCount>=6,"Insufficient circle fit membership");r.fitRms=std::sqrt(fitSum/r.fitUsedCount);
    r.coverage=double(r.foundCount)/n;
    if(search.fullCircle)r.maxGapDegrees=std::max(r.maxGapDegrees,(n-last+first)*std::abs(step));
    else r.maxGapDegrees=std::max({r.maxGapDegrees,first*std::abs(step),(n-1-last)*std::abs(step)});
    r.observedDeviation={minimum,maximum,maximum-minimum,std::max(0.,fit.radius-minimum),std::max(0.,maximum-fit.radius)};
    if(r.foundCount<o.minFoundPoints)r.qualityReasons.append("insufficient_found_points");
    if(r.coverage<o.minCoverage)r.qualityReasons.append("insufficient_scan_coverage");
    if(r.maxGapDegrees>o.maxGapDegrees)r.qualityReasons.append("excessive_nominal_gap");
    if(r.qualityReasons.isEmpty()){r.scope=search.fullCircle ? CircleMetricScope::FullCircle:CircleMetricScope::ObservedArc;r.deviation=r.observedDeviation;}
    if(!search.fullCircle)r.contour.unavailableReason="open_arc";
    else if(!r.deviation)r.contour.unavailableReason="quality_not_evaluated";
    else if(r.missingCount)r.contour.unavailableReason="missing_contour_samples";
    else r.contour=closedContour(contour,cancel);
    stop(cancel);return r;
}
}
