#include "WorldTrackingService.hpp"
#include <cmath>
#include "domain/ToolRegistry.hpp"
#include "domain/ReferenceConfig.hpp"
#include <numbers>
#include <stdexcept>
#include <algorithm>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void validate(const ImageFrame& image,const ToolRoi& roi,const WorldTrackingMap& map,double unit){
    require(bool(map.requireImage),"World tracking image snapshot missing");map.requireImage(image);
    roi.validate();require(roi.frame==map.world && roi.frame.unit==LengthUnit::Millimetre,"World tracking ROI frame mismatch");
    require(bool(map.nominalToRaw) && bool(map.rawToWorld),"World tracking mapping missing");
    require(std::isfinite(unit) && unit>0,"World tracking sample unit must be positive finite");
}
ScanSegment scan(const ToolRoi& roi,double unit){
    require(std::holds_alternative<ScanSegment>(roi.geometry),"World tracking requires scan segment");
    auto s=std::get<ScanSegment>(roi.geometry);return {s.start/unit,s.end/unit,s.width/unit};
}
TrackedWorldPoint point(const WorldTrackingMap& map,QPointF nominal){
    const auto raw=map.nominalToRaw(nominal);return {raw,map.rawToWorld(raw)};
}
}
std::optional<TrackedWorldPoint> WorldTrackingService::edge(const ImageFrame& image,const ToolRoi& roi,double unit,const WorldTrackingMap& map,const EdgeOptions& o,const std::shared_ptr<std::atomic_bool>& cancel){
    validate(image,roi,map,unit);
    const auto found=EdgeFinder::findMapped(image,scan(roi,unit),[&](QPointF p){return map.nominalToRaw(p*unit);},o,cancel);
    if(!found.selected)return {};
    return point(map,found.candidates[*found.selected].point*unit);
}
std::optional<TrackedWorldPair> WorldTrackingService::pair(const ImageFrame& image,const ToolRoi& roi,double unit,const WorldTrackingMap& map,const EdgePairOptions& o,const std::shared_ptr<std::atomic_bool>& cancel){
    validate(image,roi,map,unit);
    const auto found=EdgePairFinder::findMapped(image,scan(roi,unit),[&](QPointF p){return map.nominalToRaw(p*unit);},o,cancel);
    if(!found.selected)return {};
    const auto p=found.candidates[*found.selected];
    const auto a=point(map,found.edges.candidates[p.firstEdge].point*unit),b=point(map,found.edges.candidates[p.secondEdge].point*unit);
    return TrackedWorldPair{a,b,QLineF(a.world,b.world).length()};
}
TrackedWorldLine WorldTrackingService::line(const ImageFrame& image,const ToolRoi& roi,double unit,const WorldTrackingMap& map,const LineScanOptions& o,const std::shared_ptr<std::atomic_bool>& cancel){
    validate(image,roi,map,unit);require(std::holds_alternative<RotatedRectangle>(roi.geometry),"World tracking line requires rectangle");
    const auto box=std::get<RotatedRectangle>(roi.geometry);
    const RotatedRectangle synthetic{box.centre/unit,box.size/unit,box.angleDegrees};
    auto world=LineFitter::observationsMapped(image,synthetic,[&](QPointF p){return map.nominalToRaw(p*unit);},o,cancel),raw=world;
    for(int i=0;i<world.size();++i)if(world[i].point){const auto p=point(map,*world[i].point*unit);raw[i].point=p.raw;world[i].point=p.world;}
    const double a=box.angleDegrees*std::numbers::pi/180;const QPointF u(std::cos(a),std::sin(a));
    const auto start=point(map,box.centre-u*(box.size.width()/2)),end=point(map,box.centre+u*(box.size.width()/2));
    return {raw,LineFitter::fit(world,end.world-start.world,o.fit,cancel)};
}
TrackedWorldCircle WorldTrackingService::circle(const ImageFrame& image,const ToolRoi& roi,const WorldTrackingMap& map,const RadialScanOptions& o,const CircleFitOptions& fit,const std::shared_ptr<std::atomic_bool>& cancel){
    validate(image,roi,map,o.unitsPerSample);
    require(std::holds_alternative<Annulus>(roi.geometry) || std::holds_alternative<ArcRoi>(roi.geometry),"World tracking circle requires annulus or arc");
    const auto arc=std::holds_alternative<Annulus>(roi.geometry) ? ArcRoi{std::get<Annulus>(roi.geometry),0,360}:std::get<ArcRoi>(roi.geometry);
    auto search=RadialScanner::findMapped(image,arc,map.nominalToRaw,o,cancel);
    QVector<CircleFitPoint> observations;
    for(auto& s:search.scans){if(s.rawPoint)s.point=map.rawToWorld(*s.rawPoint);observations.append({s.point});}
    // A projective image of a nominal circle need not remain circular. Existing
    // fit quality limits reject it; no radius/angle conversion or scale fallback.
    return {search,CircleFitter::fit(observations,fit,cancel)};
}
TrackedWorldCircleMetrics WorldTrackingService::metrics(const TrackedWorldCircle& hit,const CoordinateFrame& frame,const CircleQualityOptions& quality,const std::shared_ptr<std::atomic_bool>& cancel){
    require(frame.unit==LengthUnit::Millimetre,"World circle metrics require mm frame");
    // Reuse radial residual/contour math, then replace nominal-angle quality by
    // actual ordered world observations. Missing arc endpoints cannot define an arc.
    auto m=CircleMetrics::measure(hit.observations,hit.world,quality,cancel);
    m.qualityReasons.removeAll("excessive_nominal_gap");
    QVector<double> angles;
    for(const auto& s:hit.observations.scans)if(s.point){
        require(!cancel || !cancel->load(),"World circle metrics cancelled");
        const auto delta=*s.point-hit.world.centre;
        require(QLineF({},delta).length()>1e-12,"World circle observation at centre");
        angles.append(std::atan2(delta.y(),delta.x())*180/std::numbers::pi);
    }
    double sweep=0,gap=0,direction=0;bool ordered=true;
    for(int i=1;i<angles.size();++i){const double d=std::remainder(angles[i]-angles[i-1],360.);
        if(i==1)direction=d>0 ? 1:-1;
        if(std::abs(d)<1e-9 || std::abs(d)>=180-1e-9 || d*direction<=0)ordered=false;
        sweep+=d;gap=std::max(gap,std::abs(d));
    }
    if(hit.observations.fullCircle){const double closure=std::remainder(angles.first()-angles.last(),360.);
        if(closure*direction<=0 || std::abs(closure)>=180-1e-9 || std::abs(sweep+closure-direction*360)>1e-7)ordered=false;
        gap=std::max(gap,std::abs(closure));
    }else {
        if(!hit.observations.scans.first().point || !hit.observations.scans.last().point)m.qualityReasons.append("missing_current_arc_endpoint");
        if(std::abs(sweep)>=360-1e-9)ordered=false;
    }
    if(!ordered)m.qualityReasons.append("unordered_current_world_angles");
    if(gap>quality.maxGapDegrees)m.qualityReasons.append("excessive_current_world_gap");
    m.maxGapDegrees=gap;m.deviation.reset();m.scope=CircleMetricScope::Unavailable;
    if(m.qualityReasons.isEmpty()){m.deviation=m.observedDeviation;m.scope=m.fullCircle ? CircleMetricScope::FullCircle:CircleMetricScope::ObservedArc;}
    // CircleMetrics may have rejected a nominal gap even though the actual gap
    // is acceptable; topology still requires every real scan and a full ring.
    if(m.fullCircle && m.deviation && !m.missingCount){QVector<QPointF> points;for(const auto& s:hit.observations.scans)points.append(*s.point);m.contour=CircleMetrics::closedContour(points,cancel);}
    else if(m.fullCircle)m.contour={{},m.missingCount ? "missing_contour_samples":"quality_not_evaluated"};
    std::optional<Arc2D> arc;
    if(!m.fullCircle && ordered && hit.observations.scans.first().point && hit.observations.scans.last().point)
        arc=Arc2D{{m.centre,m.radius,frame},angles.first(),sweep};
    return {m,arc};
}
namespace {
EdgeOptions edgeOptions(const QJsonObject& p){
    EdgeOptions o;o.profile={p["stepPx"].toDouble(),p["widthStepPx"].toDouble(),p["sigmaPx"].toDouble()};
    o.minContrast=p["minContrast"].toDouble();o.subpixel=p["subpixel"].toBool();
    o.selection=p["selection"]=="first" ? EdgeSelection::First:p["selection"]=="last" ? EdgeSelection::Last:EdgeSelection::Strongest;
    o.polarity=p["polarity"]=="rising" ? EdgePolarity::Rising:p["polarity"]=="falling" ? EdgePolarity::Falling:EdgePolarity::Any;return o;
}
LineScanOptions lineOptions(const QJsonObject& p){
    LineScanOptions o;o.edge=edgeOptions(p);o.scanCount=p["scanCount"].toInt();o.scanWidthPx=p["scanWidthPx"].toDouble();
    o.fit.minPoints=p["minPoints"].toInt();o.fit.minCoverage=p["minCoverage"].toDouble();o.fit.minSpanCoverage=p["minSpanCoverage"].toDouble();o.fit.ransac=p["ransac"].toBool();
    o.fit.minSpan=p["minSpanMm"].toDouble();o.fit.inlierDistance=p["inlierDistanceMm"].toDouble();o.fit.maxRms=p["maxRmsMm"].toDouble();o.fit.maxResidual=p["maxResidualMm"].toDouble();return o;
}
RadialScanOptions radialOptions(const QJsonObject& p){
    RadialScanOptions o;o.edge=edgeOptions(p);o.scanCount=p["scanCount"].toInt();o.scanWidth=p["scanWidth"].toDouble();o.unitsPerSample=p["unitsPerSample"].toDouble();o.direction=p["direction"]=="inward" ? RadialDirection::Inward:RadialDirection::Outward;return o;
}
ArcRoi radialRoi(const ToolRoi& roi){return std::holds_alternative<Annulus>(roi.geometry) ? ArcRoi{std::get<Annulus>(roi.geometry),0,360}:std::get<ArcRoi>(roi.geometry);}
}
void WorldTrackingService::preflight(const ToolDefinition& t,const ImageFrame& image,const WorldTrackingMap& map,const std::shared_ptr<std::atomic_bool>& cancel){
    const auto b=t.parameters.contains("referenceBinding") ? toolReferenceBinding(t):ReferenceBinding{};
    require(!t.parameters.contains("referenceBinding") || b.world,"World executor requires world reference binding");
    const double unit=t.parameters.contains("referenceBinding") ? b.mmPerSample:worldPoseTracking(t).mmPerSample;const auto roi=ToolRoi::fromJson(t.parameters["scanRoi"].toObject());validate(image,roi,map,unit);
    if(t.kind=="line_fit"){
        auto box=std::get<RotatedRectangle>(roi.geometry);box.centre/=unit;box.size/=unit;
        LineFitter::observationsMapped(image,box,[&](QPointF q){return map.nominalToRaw(q*unit);},lineOptions(t.parameters),cancel);
    }else if(t.kind=="circle_fit")RadialScanner::findMapped(image,radialRoi(roi),map.nominalToRaw,radialOptions(t.parameters),cancel);
    else EdgeFinder::findMapped(image,scan(roi,unit),[&](QPointF q){return map.nominalToRaw(q*unit);},edgeOptions(t.parameters),cancel);
}
ToolResult WorldTrackingService::execute(const ToolDefinition& t,const ImageFrame& image,const WorldTrackingMap& map,const std::shared_ptr<std::atomic_bool>& cancel){
    ToolRegistry::validate(t);const auto b=t.parameters.contains("referenceBinding") ? toolReferenceBinding(t):ReferenceBinding{};
    require(!t.parameters.contains("referenceBinding") || b.world,"World executor requires world reference binding");
    const double unit=t.parameters.contains("referenceBinding") ? b.mmPerSample:worldPoseTracking(t).mmPerSample;if(t.parameters.contains("referenceBinding"))b.calibration->requireCurrent(map.calibration);else worldPoseTracking(t).calibration.requireCurrent(map.calibration);
    const auto roi=ToolRoi::fromJson(t.parameters["scanRoi"].toObject());const auto p=t.parameters;const auto& m=image.metadata();
    ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;r.calibration=map.calibration;
    // Raw fits and synthetic derivative units remain unavailable. Tool tolerance
    // is separate from the unevaluated part decision and production permission.
    for(const auto& port:outputPorts(t.kind)){PortValue v{port,false,false,m.frameId,m.cycleId,m};v.invalidReason="WORLD_TRACKING_UNAVAILABLE";v.method="world-raw-rigid-refit";v.methodVersion=2;r.outputs[port.id]=v;}
    auto put=[&](QString id,PortData data,bool valid=true,QString reason="NOT_EVALUATED"){require(r.outputs.contains(id),"Unknown world tracking output");auto& v=r.outputs[id];v.data=std::move(data);v.valid=valid;v.invalidReason=valid ? QString{}:reason;};
    if(t.kind=="edge_point"){
        const auto hit=edge(image,roi,unit,map,edgeOptions(p),cancel);put("found",bool(hit));if(hit){put("point",hit->raw);r.points.append(hit->raw);}
    }else if(t.kind=="edge_pair"){
        EdgePairOptions o;o.profile=edgeOptions(p).profile;o.selection=edgeOptions(p).selection;o.minContrast=p["minContrast"].toDouble();o.subpixel=p["subpixel"].toBool();o.minWidthPx=p["minWidthPx"].toDouble();o.maxWidthPx=p["maxWidthPx"].toDouble();o.polarity=p["polarity"]=="bright" ? StripePolarity::Bright:p["polarity"]=="dark" ? StripePolarity::Dark:StripePolarity::Any;
        const auto hit=pair(image,roi,unit,map,o,cancel);put("found",bool(hit));if(hit){put("first",hit->first.raw);put("second",hit->second.raw);put("centre",(hit->first.raw+hit->second.raw)/2);put("widthPx",QLineF(hit->first.raw,hit->second.raw).length());put("widthMm",hit->widthMm);put("accepted",hit->widthMm>=p["lower"].toDouble() && hit->widthMm<=p["upper"].toDouble());r.points={hit->first.raw,hit->second.raw};r.lines={QLineF(hit->first.raw,hit->second.raw)};}
    }else if(t.kind=="line_fit"){
        const auto hit=line(image,roi,unit,map,lineOptions(p),cancel);const auto& f=hit.world;
        const CoordinateFrame raw{m.coordinateSystemId};PointSet obs{{},raw};for(const auto& s:hit.raw)if(s.point)obs.points.append(*s.point);put("observationsPx",obs);
        put("found",true);put("lineMm",Line2D{f.supportStart,f.supportEnd,map.world});put("originMm",f.origin);put("tangentMm",f.tangent);put("angleMm",f.angleDegrees);put("spanMm",f.span);put("rmsMm",f.rms);put("maxResidualMm",f.maxResidual);
        put("accepted",f.span>=p["lower"].toDouble() && f.span<=p["upper"].toDouble() && f.angleDegrees>=p["angleLower"].toDouble() && f.angleDegrees<=p["angleUpper"].toDouble());
        put("coverage",f.coverage);put("spanCoverage",f.spanCoverage);put("usedCount",double(f.usedCount));put("outlierCount",double(f.rejectedCount));put("missingCount",double(f.missingCount));
        InspectionResults scanRows;for(int i=0;i<f.scans.size();++i){const auto& s=f.scans[i];scanRows.items.append({QString("scan %1").arg(i),s.point ? QString("%1,%2; residual=%3 mm").arg(s.point->x(),0,'g',17).arg(s.point->y(),0,'g',17).arg(*s.signedResidual,0,'g',17):"not_found",s.used});if(hit.raw[i].point)(s.used ? r.points:r.rejectedPoints).append(*hit.raw[i].point);}put("scans",scanRows);
    }else {
        CircleFitOptions f;f.minPoints=p["minPoints"].toInt();f.minCoverage=p["minFitCoverage"].toDouble();f.minAngularSpanDegrees=p["minAngularSpan"].toDouble();f.maxCondition=p["maxCondition"].toDouble();f.ransac=p["ransac"].toBool();f.inlierDistance=p["inlierDistance"].toDouble();f.maxRms=p["maxRms"].toDouble();f.maxResidual=p["maxResidual"].toDouble();
        const auto hit=circle(image,roi,map,radialOptions(p),f,cancel);const auto& fit=hit.world;
        const auto measured=metrics(hit,map.world,{p["minFoundPoints"].toInt(),p["minCoverage"].toDouble(),p["maxGapDegrees"].toDouble()},cancel);const auto& metric=measured.metrics;
        put("found",true);put("circleMm",Circle2D{fit.centre,fit.radius,map.world});put("centreMm",fit.centre);put("radiusMm",fit.radius);put("diameterMm",2*fit.radius);put("fitRmsMm",fit.rms);put("fitMaxResidualMm",fit.maxResidual);
        PointSet raw{{},{m.coordinateSystemId}},world{{},map.world};InspectionResults scanRows;
        for(int i=0;i<hit.observations.scans.size();++i){const auto& s=hit.observations.scans[i];if(s.point){world.points.append(*s.point);raw.points.append(*s.rawPoint);(fit.scans[i].used ? r.points:r.rejectedPoints).append(*s.rawPoint);}scanRows.items.append({QString("scan %1").arg(i),s.point ? QString("nominal-angle=%1; current=%2,%3 mm").arg(s.angleDegrees,0,'g',17).arg(s.point->x(),0,'g',17).arg(s.point->y(),0,'g',17):"not_found",fit.scans[i].used});}
        put("observationsPx",raw);put("observationsMm",world);put("slots",scanRows);
        put("arcMm",measured.arc.value_or(Arc2D{}),measured.arc.has_value(),metric.fullCircle ? "FULL_CIRCLE":"CURRENT_ARC_UNAVAILABLE");
        const auto d=metric.observedDeviation;put("radialMinMm",d.minimumRadius);put("radialMaxMm",d.maximumRadius);put("maxInwardMm",d.maxInward);put("maxOutwardMm",d.maxOutward);put("observedDeviationMm",d.peakToValley);
        const bool qualified=metric.deviation.has_value();put("qualified",qualified);put("deviationMm",qualified ? metric.deviation->peakToValley:0.,qualified);
        const bool diameterOk=metric.diameter>=p["diameterLower"].toDouble() && metric.diameter<=p["diameterUpper"].toDouble();
        const bool deviationOk=qualified && metric.deviation->peakToValley>=p["deviationLower"].toDouble() && metric.deviation->peakToValley<=p["deviationUpper"].toDouble();
        put("diameterAccepted",diameterOk,qualified);put("deviationAccepted",deviationOk,qualified);put("accepted",diameterOk && deviationOk,qualified);
        put("coverage",metric.coverage);put("maxGapDegrees",metric.maxGapDegrees);put("scope",QString(qualified ? (metric.fullCircle ? "full-circle":"observed-arc"):"unavailable"));
        put("circularityIndex",metric.contour.metrics ? metric.contour.metrics->circularityIndex:0.,metric.contour.metrics.has_value(),metric.contour.unavailableReason);
        InspectionResults reasons;for(const auto& reason:metric.qualityReasons)reasons.items.append({reason,reason,false});reasons.items.append({"contour",metric.contour.unavailableReason,metric.contour.metrics.has_value()});put("quality",reasons);
        InspectionResults diagnostics;for(const auto& s:metric.samples){
            const auto delta=s.point ? *s.point-metric.centre:QPointF{};
            diagnostics.items.append({QString("scan %1").arg(s.scanId),s.point ? QString("current-angle=%1 deg; radial=%2 mm; residual=%3 mm; %4").arg(std::atan2(delta.y(),delta.x())*180/std::numbers::pi,0,'g',17).arg(*s.radialDistance,0,'g',17).arg(*s.signedResidual,0,'g',17).arg(s.fitUsed ? "used":"outlier"):"not_found",s.fitUsed});
        }put("scans",diagnostics);
    }
    require(!cancel || !cancel->load(),"World tracking cancelled");r.message="World tracking mm metrics / tool tolerance; part NOT_EVALUATED";return r;
}

}

