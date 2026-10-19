#include "LineFitter.hpp"
#include "ScanLayout.hpp"
#include <opencv2/geometry.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
bool finite(QPointF p){return std::isfinite(p.x()) && std::isfinite(p.y());}
double norm(QPointF p){return std::hypot(p.x(),p.y());}
double dot(QPointF a,QPointF b){return a.x()*b.x()+a.y()*b.y();}
double cross(QPointF a,QPointF b){return a.x()*b.y()-a.y()*b.x();}
void validate(const LineFitOptions& o) {
    auto positive=[](double v){return std::isfinite(v) && v>0;};
    require(o.minPoints>=3 && o.minPoints<=4096 && positive(o.minCoverage) && o.minCoverage<=1 && positive(o.minSpanCoverage) && o.minSpanCoverage<=1 &&
        positive(o.minSpan) && positive(o.inlierDistance) && positive(o.maxRms) && positive(o.maxResidual) && positive(o.minVarianceRatio) && o.minVarianceRatio>1 &&
        o.ransacIterations>=1 && o.ransacIterations<=1024,"Invalid line fit options");
}
struct Model {QPointF centre,tangent;};
}
FittedLine LineFitter::fit(const QVector<LineScanPoint>& scans,QPointF taught,const LineFitOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Line fit cancelled");};stop();validate(o);
    require(scans.size()>=o.minPoints && scans.size()<=4096 && finite(taught) && std::isfinite(norm(taught)) && norm(taught)>1e-12,"Invalid line support/direction");taught/=norm(taught);
    QVector<QPointF> points;QVector<int> ids;std::set<std::pair<double,double>> unique;
    for(int i=0;i<scans.size();++i){stop();if(scans[i].point){const auto p=*scans[i].point;require(finite(p) && std::abs(p.x())<=1e12 && std::abs(p.y())<=1e12,"Invalid line point");require(unique.emplace(p.x(),p.y()).second,"Duplicate line observation");points.append(p);ids.append(i);}}
    requireFitSupport(points.size()>=o.minPoints,"Insufficient found line points","LINE_POINTS","edge_search",scans,o.minPoints,0);
    // Shift/scale before OpenCV's float line representation; preserve large offsets.
    QPointF offset=points.front(),mean;for(auto p:points)mean+=p-offset;mean/=points.size();offset+=mean;
    double scale=0;for(auto p:points)scale=std::max(scale,norm(p-offset));require(std::isfinite(scale) && scale>1e-12,"Degenerate line points");
    require(std::isfinite(o.inlierDistance/scale) && o.inlierDistance/scale>=1e-12,"Line threshold outside normalized solver range");
    QVector<QPointF> local;for(auto p:points)local.append((p-offset)/scale);
    auto orient=[&](Model m){require(finite(m.centre) && finite(m.tangent) && norm(m.tangent)>1e-12,"Invalid line model");m.tangent/=norm(m.tangent);if(dot(m.tangent,taught)<0)m.tangent=-m.tangent;return m;};
    auto solve=[&](const QVector<int>& indices,int distance){
        stop();require(indices.size()>=3,"Insufficient refit points");
        // Re-normalize each consensus independently: far-away rejected points
        // must not reduce the precision of the final float fitLine result.
        const auto base=points[indices.front()];QPointF shift;
        for(auto i:indices)shift+=points[i]-base;shift/=indices.size();const auto centre=base+shift;
        double spread=0;for(auto i:indices)spread=std::max(spread,norm(points[i]-centre));require(spread>1e-12,"Degenerate refit points");
        require(std::isfinite(o.inlierDistance/spread),"Line refit threshold overflow");
        std::vector<cv::Point2d> data;for(auto i:indices){const auto p=(points[i]-centre)/spread;data.emplace_back(p.x(),p.y());}
        cv::Vec4f v;cv::fitLine(data,v,distance,distance==cv::DIST_HUBER ? o.inlierDistance/spread:0,1e-8,1e-8);stop();
        return orient({(centre-offset)/scale+QPointF(v[2],v[3])*(spread/scale),{v[0],v[1]}});
    };
    auto members=[&](const Model& m){QVector<int> result;for(int i=0;i<local.size();++i){stop();if(std::abs(cross(local[i]-m.centre,m.tangent))*scale<=o.inlierDistance)result.append(i);}return result;};
    auto error=[&](const Model& m,const QVector<int>& list){double total=0;for(auto i:list){stop();const double e=cross(local[i]-m.centre,m.tangent);total+=e*e;}return total;};
    QVector<int> all;for(int i=0;i<local.size();++i)all.append(i);
    auto best=solve(all,cv::DIST_HUBER);auto inliers=members(best);double bestError=error(best,inliers);
    // Local PRNG only: reproducible pair hypotheses, no global OpenCV RNG writes.
    if(o.ransac){std::mt19937 rng(0x56464c31u);for(int iteration=0;iteration<o.ransacIterations;++iteration){stop();const int a=int(rng()%local.size()),b=int(rng()%local.size());if(a==b || norm(local[b]-local[a])<1e-9)continue;const auto candidate=orient({local[a],local[b]-local[a]});if(dot(candidate.tangent,taught)<.5)continue;auto list=members(candidate);const double e=error(candidate,list);if(list.size()>inliers.size() || (list.size()==inliers.size() && e<bestError)){best=candidate;inliers=std::move(list);bestError=e;}}}
    requireFitSupport(inliers.size()>=o.minPoints,"Insufficient line consensus","LINE_CONSENSUS","fit_consensus",scans,o.minPoints,int(inliers.size()));
    bool stable=false;for(int pass=0;pass<8;++pass){best=solve(inliers,cv::DIST_L2);auto next=members(best);requireFitSupport(next.size()>=o.minPoints,"Insufficient line refit consensus","LINE_CONSENSUS","fit_consensus",scans,o.minPoints,int(next.size()));if(next==inliers){stable=true;break;}inliers=std::move(next);}require(stable,"Unstable line consensus");
    require(dot(best.tangent,taught)>=.5,"Line direction inconsistent with taught axis");
    FittedLine r;r.tangent=best.tangent;r.origin=offset+best.centre*scale;r.scans=scans;
    double low=std::numeric_limits<double>::infinity(),high=-low,sum=0,maximum=0;int first=int(scans.size()),last=-1;
    QPointF centre;for(auto i:inliers)centre+=local[i];centre/=inliers.size();double xx=0,xy=0,yy=0;
    for(auto& s:r.scans){s.used=false;s.signedResidual.reset();s.reason=s.point ? "outlier":"not_found";}
    for(int i=0;i<points.size();++i)r.scans[ids[i]].signedResidual=cross(local[i]-best.centre,best.tangent)*scale;
    for(auto i:inliers){stop();auto& s=r.scans[ids[i]];s.used=true;s.reason.clear();first=std::min(first,ids[i]);last=std::max(last,ids[i]);const double t=dot(local[i]-best.centre,best.tangent)*scale;low=std::min(low,t);high=std::max(high,t);const double e=*s.signedResidual;sum+=e*e;maximum=std::max(maximum,std::abs(e));const auto q=local[i]-centre;xx+=q.x()*q.x();xy+=q.x()*q.y();yy+=q.y()*q.y();}
    const double trace=xx+yy,discriminant=std::hypot(xx-yy,2*xy),major=(trace+discriminant)/2,minor=std::max(0.,(trace-discriminant)/2);
    require(major>0 && minor<=major/o.minVarianceRatio,"Ill-conditioned line distribution");
    r.varianceRatio=major/std::max(minor,major*1e-15);r.span=high-low;r.rms=std::sqrt(sum/inliers.size());r.maxResidual=maximum;
    r.usedCount=inliers.size();r.missingCount=scans.size()-points.size();r.rejectedCount=points.size()-inliers.size();r.coverage=double(r.usedCount)/scans.size();r.spanCoverage=double(last-first)/(scans.size()-1);
    require(r.coverage>=o.minCoverage && r.spanCoverage>=o.minSpanCoverage,"Insufficient line scan coverage");
    require(r.span>=o.minSpan,"Insufficient line spatial span");require(std::isfinite(r.rms) && r.rms<=o.maxRms && r.maxResidual<=o.maxResidual,"Line residual limits exceeded");
    r.supportStart=r.origin+r.tangent*low;r.supportEnd=r.origin+r.tangent*high;r.angleDegrees=std::atan2(r.tangent.y(),r.tangent.x())*180/std::numbers::pi;if(r.angleDegrees>=180)r.angleDegrees-=360;
    stop();return r;
}
LineSearch LineFitter::find(const ImageFrame& image,const ToolRoi& roi,const LineScanOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    roi.validate();
    require(std::holds_alternative<RotatedRectangle>(roi.geometry) && roi.inside(image.size()) && roi.frame.id==image.metadata().coordinateSystemId,"Line requires an in-frame raw rotated rectangle");
    const auto box=std::get<RotatedRectangle>(roi.geometry);
    const auto scans=observationsMapped(image,box,[](QPointF p){return p;},o,cancel);
    const double angle=box.angleDegrees*std::numbers::pi/180;
    LineSearch r;r.line=fit(scans,{std::cos(angle),std::sin(angle)},o.fit,cancel);r.origin=image.metadata();r.inputContentId=image.contentId();r.processingId=image.processingId();return r;
}
QVector<LineScanPoint> LineFitter::observationsMapped(const ImageFrame& image,const RotatedRectangle& box,const ProfileRawMap& map,const LineScanOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    const ToolRoi roi{{"sampling"},box};
    auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Line scan cancelled");};stop();validate(o.fit);roi.validate();
    require(bool(map),"Line requires a sampling map");
    require(o.scanCount>=3 && o.scanCount<=4096 && o.scanCount>=o.fit.minPoints && std::isfinite(o.scanWidthPx) && o.scanWidthPx>0 && o.scanWidthPx<=1024 && o.scanWidthPx<box.size.width(),"Invalid line scans");
    const qint64 pixels=qint64(image.size().width())*image.size().height();
    require(pixels<=16*1024*1024 && pixels*o.scanCount<=512*1024*1024,"Line image/hash work budget exceeded");
    // Preflight aggregate work before allocating profiles or hashing/reading pixels.
    const auto p=o.edge.profile;require(std::isfinite(p.stepPx) && p.stepPx>=.05 && p.stepPx<=16 && std::isfinite(p.widthStepPx) && p.widthStepPx>=.25 && p.widthStepPx<=16 && std::isfinite(p.sigmaPx) && p.sigmaPx>=0 && p.sigmaPx<=16,"Invalid line sampling options");
    const auto layout=ScanLayout::line(box,o.scanCount,o.scanWidthPx);
    QVector<ToolRoi> scanRois;scanRois.reserve(o.scanCount);double totalTaps=0,totalConvolution=0;
    for(const auto& segment:layout){stop();ToolRoi scan{roi.frame,segment};
        // Use the actual endpoints, as the sampler does: rotated floating-point
        // lengths can cross a ceil boundary even when nominal height is integral.
        const double length=QLineF(segment.start,segment.end).length();
        const double count=std::ceil(length/p.stepPx)+1,across=std::ceil(o.scanWidthPx/p.widthStepPx)+1,step=length/(count-1),radius=p.sigmaPx==0 ? 0:std::ceil(3*p.sigmaPx/step);
        require(count<=65536 && across<=1024 && radius<=1024 && count>=2*radius+4 && count*across<=4*1024*1024,"Line profile budget/support exceeded");
        totalTaps+=count*across;totalConvolution+=(count-2*radius)*(2*radius+1);
        const auto d=(segment.end-segment.start)/length;const QPointF normal(-d.y(),d.x());
        for(auto q:{segment.start,segment.end})for(double side:{-.5,.5}){const auto raw=map(q+normal*(segment.width*side));require(finite(raw) && raw.x()>=0 && raw.y()>=0 && raw.x()<=image.size().width()-1 && raw.y()<=image.size().height()-1,"Line strip outside raw domain");}
        require(totalTaps<=16*1024*1024 && totalConvolution<=16*1024*1024,"Line aggregate sampling budget exceeded");scanRois.append(std::move(scan));
    }
    QVector<LineScanPoint> scans;scans.reserve(o.scanCount);
    for(const auto& scan:scanRois){stop();
        const auto edge=EdgeFinder::findMapped(image,std::get<ScanSegment>(scan.geometry),map,o.edge,cancel);LineScanPoint s;s.rejections=scanRejections(edge);if(edge.selected)s.point=edge.candidates[*edge.selected].point;scans.append(s);
    }
    stop();return scans;
}
}
