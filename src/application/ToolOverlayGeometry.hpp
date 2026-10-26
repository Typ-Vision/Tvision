#pragma once
#include "ExecutionService.hpp"
#include "CircleToolService.hpp"
#include "MetrologyService.hpp"
#include "PoseRoiService.hpp"
#include "domain/ReferenceConfig.hpp"
#include <cmath>
#include <numbers>
namespace visionflow {
// Ephemeral execution evidence: actual resolved ROI, mapped once to raw pixels.
// It is never a recipe edit or terminal/production authority.
inline ToolOverlayGeometry toolOverlayGeometry(const ToolDefinition& original,const ToolDefinition& effective,
    const ImageFrame& frame,const MeasurementExecutionContext& context,const std::optional<ToolRoi>& resolved,
    const std::optional<WorldTrackingMap>& world,const ToolResult& result) {
    ToolOverlayGeometry out;out.origin=frame.metadata();out.rawSize=frame.size();
    try {
        std::optional<ToolRoi> roi=resolved;
        if(world)roi=original.parameters.contains("referenceBinding") ? toolReferenceBinding(original).nominal:ToolRoi::fromJson(original.parameters["scanRoi"].toObject());
        else if(!roi && !effective.parameters["scanRoi"].toObject().isEmpty())roi=ToolRoi::fromJson(effective.parameters["scanRoi"].toObject());
        auto map=[&](QPointF p){
            if(world)p=world->nominalToRaw(p);
            else if(roi && roi->frame.unit==LengthUnit::Millimetre)p=CircleToolService::mapPoint(frame,context,p,true);
            if(!std::isfinite(p.x()) || !std::isfinite(p.y()) || p.x()<0 || p.y()<0 || p.x()>frame.size().width()-1 || p.y()>frame.size().height()-1)throw std::runtime_error("Tarama geometrisi raw alanın dışında");
            return p;
        };
        auto polygon=[&](QVector<QPointF> points,bool closed=true){if(closed && !points.isEmpty())points<<points.first();for(auto& p:points)p=map(p);out.outlines<<points;};
        if(roi){
            auto polar=[](QPointF c,double r,double a){a*=std::numbers::pi/180;return c+QPointF(r*std::cos(a),r*std::sin(a));};
            std::visit([&](const auto& g){using T=std::decay_t<decltype(g)>;
                if constexpr(std::is_same_v<T,ScanSegment>){auto d=g.end-g.start;auto n=QPointF(-d.y(),d.x())*(g.width/(2*QLineF(g.start,g.end).length()));polygon({g.start+n,g.end+n,g.end-n,g.start-n});}
                else if constexpr(std::is_same_v<T,RotatedRectangle>){QVector<QPointF> p;const double a=g.angleDegrees*std::numbers::pi/180;for(auto v:QVector<QPointF>{{-g.size.width()/2,-g.size.height()/2},{g.size.width()/2,-g.size.height()/2},{g.size.width()/2,g.size.height()/2},{-g.size.width()/2,g.size.height()/2}})p<<g.centre+QPointF(v.x()*std::cos(a)-v.y()*std::sin(a),v.x()*std::sin(a)+v.y()*std::cos(a));polygon(p);}
                else if constexpr(std::is_same_v<T,Annulus> || std::is_same_v<T,ArcRoi>){
                    const auto arc=[&](){if constexpr(std::is_same_v<T,Annulus>)return ArcRoi{g,0,360};else return g;}();
                    for(double r:{arc.ring.innerRadius,arc.ring.outerRadius}){QVector<QPointF> p;for(int i=0;i<=180;++i)p<<polar(arc.ring.centre,r,arc.startDegrees+arc.sweepDegrees*i/180);polygon(p,false);}
                    if(std::abs(arc.sweepDegrees)<360)for(double a:{arc.startDegrees,arc.startDegrees+arc.sweepDegrees})polygon({polar(arc.ring.centre,arc.ring.innerRadius,a),polar(arc.ring.centre,arc.ring.outerRadius,a)},false);
                }else polygon(g.vertices,std::is_same_v<T,PolygonRoi>);
            },roi->geometry);
            for(const auto& line:MetrologyService::scanGuides(original.kind,original.parameters,*roi))out.guides<<QLineF(map(line.p1()),map(line.p2()));
        }else if(!result.searchRegion.isEmpty()){
            const auto r=result.searchRegion;polygon({r.topLeft(),r.topRight(),r.bottomRight(),r.bottomLeft()});
        }
    }catch(const std::exception& e){out.outlines.clear();out.guides.clear();out.reason=QString::fromUtf8(e.what());}
    if(world && result.lines.isEmpty())try {
        auto toRaw=[&](QPointF p){return CircleToolService::mapPoint(frame,context,p,true);};
        const auto line=result.outputs.value("lineMm");if(line.valid)if(const auto* l=std::get_if<Line2D>(&line.data))out.resultLines<<QLineF(toRaw(l->start),toRaw(l->end));
        const auto circle=result.outputs.value("circleMm"),arc=result.outputs.value("arcMm");
        if(circle.valid)if(const auto* c=std::get_if<Circle2D>(&circle.data)){
            double start=0,sweep=360;if(arc.valid)if(const auto* a=std::get_if<Arc2D>(&arc.data)){start=a->startDegrees;sweep=a->sweepDegrees;}
            auto at=[&](int i){const auto angle=(start+sweep*i/180)*std::numbers::pi/180;return toRaw(c->centre+QPointF(c->radius*std::cos(angle),c->radius*std::sin(angle)));};
            auto previous=at(0);for(int i=1;i<=180;++i){const auto next=at(i);out.resultLines<<QLineF(previous,next);previous=next;}
        }
    }catch(const std::exception& e){out.resultLines.clear();out.reason+="; Sonuç şekli raw dönüşümü: "+QString::fromUtf8(e.what());}
    return out;
}
}
