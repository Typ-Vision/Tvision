#include "RadialScanner.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
RadialSearch RadialScanner::find(const ImageFrame& image,const ToolRoi& roi,const RadialScanOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    roi.validate();require(roi.frame.id==image.metadata().coordinateSystemId && o.unitsPerSample==1,"Radial raw frame/sampling unit mismatch");
    ArcRoi arc;
    if(const auto* ring=std::get_if<Annulus>(&roi.geometry))arc={*ring,0,360};
    else {require(std::holds_alternative<ArcRoi>(roi.geometry),"Radial scan requires annulus/arc");arc=std::get<ArcRoi>(roi.geometry);}
    return findMapped(image,arc,[](QPointF p){return p;},o,cancel);
}
RadialSearch RadialScanner::findMapped(const ImageFrame& image,const ArcRoi& arc,const ProfileRawMap& map,const RadialScanOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Radial scan cancelled");};stop();
    ToolRoi{{"nominal"},arc}.validate();
    require(bool(map) && o.scanCount>=3 && o.scanCount<=4096 && std::isfinite(o.unitsPerSample) && o.unitsPerSample>0 &&
        std::isfinite(o.scanWidth) && o.scanWidth>0 && o.scanWidth/o.unitsPerSample<=1024 &&
        (o.direction==RadialDirection::Outward || o.direction==RadialDirection::Inward),"Invalid radial options");
    const qint64 pixels=qint64(image.size().width())*image.size().height();
    require(pixels<=16*1024*1024 && pixels*o.scanCount<=512*1024*1024,"Radial image/hash work budget exceeded");
    const auto p=o.edge.profile;
    require(std::isfinite(p.stepPx) && p.stepPx>=.05 && p.stepPx<=16 && std::isfinite(p.widthStepPx) && p.widthStepPx>=.25 && p.widthStepPx<=16 && std::isfinite(p.sigmaPx) && p.sigmaPx>=0 && p.sigmaPx<=16,"Invalid radial sampling options");
    RadialSearch result;result.fullCircle=std::abs(arc.sweepDegrees)==360;result.origin=image.metadata();
    QVector<ScanSegment> synthetic;double taps=0,convolution=0;
    const double spacing=arc.sweepDegrees/(result.fullCircle ? o.scanCount:o.scanCount-1);
    auto raw=[&](QPointF q){auto r=map(q);require(std::isfinite(r.x()) && std::isfinite(r.y()) && r.x()>=0 && r.y()>=0 && r.x()<=image.size().width()-1 && r.y()<=image.size().height()-1,"Radial footprint outside raw domain");return r;};
    for(int i=0;i<o.scanCount;++i){stop();RadialScanPoint s;s.scanId=i;s.angleDegrees=arc.startDegrees+i*spacing;
        const double a=s.angleDegrees*std::numbers::pi/180;const QPointF u(std::cos(a),std::sin(a));
        auto start=arc.ring.centre+u*arc.ring.innerRadius,end=arc.ring.centre+u*arc.ring.outerRadius;
        if(o.direction==RadialDirection::Inward)std::swap(start,end);
        s.nominalScan={start,end,o.scanWidth};ScanSegment scan{start/o.unitsPerSample,end/o.unitsPerSample,o.scanWidth/o.unitsPerSample};
        ToolRoi{{"sampling"},scan}.validate();
        const double length=QLineF(scan.start,scan.end).length(),count=std::ceil(length/p.stepPx)+1,across=std::ceil(scan.width/p.widthStepPx)+1;
        const double step=length/(count-1),radius=p.sigmaPx==0 ? 0:std::ceil(3*p.sigmaPx/step);
        require(count<=65536 && across<=1024 && radius<=1024 && count>=2*radius+4 && count*across<=4*1024*1024,"Radial profile budget/support exceeded");
        taps+=count*across;convolution+=(count-2*radius)*(2*radius+1);
        require(taps<=16*1024*1024 && convolution<=16*1024*1024,"Radial aggregate sampling budget exceeded");
        const auto d=(end-start)/QLineF(start,end).length();const QPointF n(-d.y(),d.x());
        // All strips are preflighted before the first image read. A projective
        // rectangle has straight sides; checked convex raw hulls contain it.
        for(auto q:{start,end})for(double side:{-.5,.5})raw(q+n*(o.scanWidth*side));
        s.rawStart=raw(start);s.rawEnd=raw(end);result.scans.append(s);synthetic.append(scan);
    }
    for(int i=0;i<o.scanCount;++i){stop();auto& s=result.scans[i];
        const auto edge=EdgeFinder::findMapped(image,synthetic[i],[&](QPointF p){return raw(p*o.unitsPerSample);},o.edge,cancel);
        if(edge.selected){const auto& c=edge.candidates[*edge.selected];s.point=c.point*o.unitsPerSample;s.rawPoint=raw(*s.point);s.strength=c.strength;s.signedStrength=c.signedStrength;s.refinement=c.refinement;++result.foundCount;}
        else s.reason="not_found";
    }
    result.missingCount=o.scanCount-result.foundCount;result.coverage=double(result.foundCount)/o.scanCount;
    const double sweep=std::abs(arc.sweepDegrees),delta=std::abs(spacing);int first=-1,last=-1;
    for(int i=0;i<o.scanCount;++i)if(result.scans[i].point){if(first<0)first=i;if(last>=0)result.maxGapDegrees=std::max(result.maxGapDegrees,(i-last)*delta);last=i;}
    if(first<0)result.maxGapDegrees=sweep;
    else if(result.fullCircle)result.maxGapDegrees=std::max(result.maxGapDegrees,(o.scanCount-last+first)*delta);
    else result.maxGapDegrees=std::max({result.maxGapDegrees,first*delta,(o.scanCount-1-last)*delta});
    stop();result.inputContentId=image.contentId();result.processingId=image.processingId();stop();return result;
}
}
