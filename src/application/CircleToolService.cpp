#include "CircleToolService.hpp"
#include "RadialScanService.hpp"
#include "domain/ToolRegistry.hpp"
#include "vision/metrology/CircleMetrics.hpp"
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
QPointF polar(QPointF c,double r,double a){a*=std::numbers::pi/180.;return c+QPointF(r*std::cos(a),r*std::sin(a));}
}
QPointF CircleToolService::mapPoint(const ImageFrame& image,const MeasurementExecutionContext& ctx,QPointF p,bool toRaw){
    require(ctx.calibration && ctx.current,"Circle requires current validated calibration");const auto& c=*ctx.calibration;
    QPointF raw=p;
    if(toRaw){if(c.mapping())raw=PlaneMappingService::inverseMap(*c.mapping(),p);else {raw=p/c.millimetresPerPixel();if(c.dependency().target.axes==AxisConvention::XRightYUp)raw.setY(-raw.y());}}
    ToolResult t;t.state=ExecutionState::Done;const auto m=image.metadata();t.outputs["p"]={{"p",PortType::Point,"px","image"},raw,true,m.frameId,m.cycleId,m};
    const auto check=MeasurementService{}.distance(t,"p",t,"p",c,*ctx.current,image.size(),0,std::numeric_limits<double>::max());require(check.state==ExecutionState::Done,check.error.toUtf8().constData());
    if(toRaw)return raw;if(c.mapping())return PlaneMappingService::map(*c.mapping(),raw);
    auto q=raw*c.millimetresPerPixel();if(c.dependency().target.axes==AxisConvention::XRightYUp)q.setY(-q.y());return q;
}
ToolResult CircleToolService::execute(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel,const MeasurementExecutionContext& ctx){
    ToolRegistry::validate(t);const auto p=t.parameters;const bool world=p["mode"]=="calibrated";
    if(p.contains("poseTracking"))throw std::runtime_error("Pose tracking requires DAG resolution");
    auto stop=[&]{require(!cancel || !cancel->load(),"Circle cancelled");};stop();
    const auto image=std::get<ImageFrame>(inputs.value("image").data);const auto m=image.metadata();
    require(!p["scanRoi"].toObject().isEmpty(),"Circle requires taught ring/arc");const auto roi=ToolRoi::fromJson(p["scanRoi"].toObject());
    if(world){require(ctx.calibration && ctx.current,"Circle requires current calibration");require(roi.frame==ctx.calibration->dependency().target,"Circle ROI target frame differs from calibration");}
    else require(roi.frame.id==m.coordinateSystemId,"Circle ROI raw frame differs from image");
    ArcRoi arc=std::holds_alternative<Annulus>(roi.geometry) ? ArcRoi{std::get<Annulus>(roi.geometry),0,360}:std::get<ArcRoi>(roi.geometry);
    RadialScanOptions o;o.scanCount=p["scanCount"].toInt();o.scanWidth=p["scanWidth"].toDouble();o.unitsPerSample=p["unitsPerSample"].toDouble();o.direction=p["direction"]=="inward" ? RadialDirection::Inward:RadialDirection::Outward;
    o.edge.profile={p["stepPx"].toDouble(),p["widthStepPx"].toDouble(),p["sigmaPx"].toDouble()};o.edge.minContrast=p["minContrast"].toDouble();o.edge.subpixel=p["subpixel"].toBool();o.edge.selection=p["selection"]=="first" ? EdgeSelection::First:p["selection"]=="last" ? EdgeSelection::Last:EdgeSelection::Strongest;o.edge.polarity=p["polarity"]=="rising" ? EdgePolarity::Rising:p["polarity"]=="falling" ? EdgePolarity::Falling:EdgePolarity::Any;
    const auto search=world ? RadialScanService::scan(image,arc,o,{*ctx.calibration,*ctx.current},cancel).search:RadialScanner::find(image,roi,o,cancel);
    ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](QString id,PortData data,bool valid=true,QString reason={}){for(const auto& port:outputPorts(t.kind))if(port.id==id){PortValue v{port,std::move(data),valid,frame,cycle,m};v.method="radial-circle";v.methodVersion=1;v.diagnostic=!world;if(!valid)v.invalidReason=reason.isEmpty() ? "NOT_EVALUATED":reason;r.outputs[id]=std::move(v);}};
    // Missing fit is an execution error; qualified metric failure below remains DONE.
    QVector<CircleFitPoint> points;for(const auto& s:search.scans)points.append({s.point});
    CircleFitOptions f;f.minPoints=p["minPoints"].toInt();f.minCoverage=p["minFitCoverage"].toDouble();f.minAngularSpanDegrees=p["minAngularSpan"].toDouble();f.maxCondition=p["maxCondition"].toDouble();f.ransac=p["ransac"].toBool();f.inlierDistance=p["inlierDistance"].toDouble();f.maxRms=p["maxRms"].toDouble();f.maxResidual=p["maxResidual"].toDouble();
    const auto fitted=CircleFitter::fit(points,f,cancel);const auto metrics=CircleMetrics::measure(search,fitted,{p["minFoundPoints"].toInt(),p["minCoverage"].toDouble(),p["maxGapDegrees"].toDouble()},cancel);
    auto raw=[&](QPointF q){stop();return world ? mapPoint(image,ctx,q,true):q;};
    r.points.append(raw(metrics.centre)); // actual fit centre must be within calibrated domain
    if(world)r.calibration=ctx.calibration->dependency();
    const QString suffix=world ? "Mm":"Px",other=world ? "Px":"Mm",unit=world ? "mm":"px";
    const Circle2D circle{metrics.centre,metrics.radius,roi.frame};
    put("circle"+suffix,circle);put("arc"+suffix,Arc2D{circle,arc.startDegrees,arc.sweepDegrees},!metrics.fullCircle,"FULL_CIRCLE");put("centre"+suffix,metrics.centre);
    put("diameter"+suffix,metrics.diameter);put("radius"+suffix,metrics.radius);put("fitRms"+suffix,metrics.fitRms);put("fitMaxResidual"+suffix,metrics.fitMaxResidual);
    const auto d=metrics.observedDeviation;put("radialMin"+suffix,d.minimumRadius);put("radialMax"+suffix,d.maximumRadius);put("maxInward"+suffix,d.maxInward);put("maxOutward"+suffix,d.maxOutward);put("observedDeviation"+suffix,d.peakToValley);
    const bool qualified=metrics.deviation.has_value();put("deviation"+suffix,qualified ? metrics.deviation->peakToValley:0.,qualified);
    put("circle"+other,Circle2D{},false,"OTHER_PLANE");put("arc"+other,Arc2D{},false,"OTHER_PLANE");put("centre"+other,QPointF{},false,"OTHER_PLANE");
    for(auto k:{"diameter","radius","fitRms","fitMaxResidual","radialMin","radialMax","maxInward","maxOutward","observedDeviation","deviation"})put(QString(k)+other,0.,false,"OTHER_PLANE");
    const bool diameterOk=metrics.diameter>=p["diameterLower"].toDouble() && metrics.diameter<=p["diameterUpper"].toDouble();
    const bool deviationOk=qualified && metrics.deviation->peakToValley>=p["deviationLower"].toDouble() && metrics.deviation->peakToValley<=p["deviationUpper"].toDouble();
    put("found",true);put("qualified",qualified);put("diameterAccepted",diameterOk,qualified);put("deviationAccepted",deviationOk,qualified);put("accepted",diameterOk && deviationOk,qualified);
    const QString scope=metrics.scope==CircleMetricScope::FullCircle ? "full-circle":metrics.scope==CircleMetricScope::ObservedArc ? "observed-arc":"unavailable";put("scope",scope);put("coverage",metrics.coverage);put("maxGapDegrees",metrics.maxGapDegrees);
    put("circularityIndex",metrics.contour.metrics ? metrics.contour.metrics->circularityIndex:0.,metrics.contour.metrics.has_value(),metrics.contour.unavailableReason);
    InspectionResults quality;for(const auto& reason:metrics.qualityReasons)quality.items.append({reason,reason,false});quality.items.append({"contour",metrics.contour.unavailableReason,metrics.contour.metrics.has_value()});put("quality",quality);
    PointSet observations{{},roi.frame};InspectionResults scanSlots;
    for(int i=0;i<search.scans.size();++i){const auto& s=search.scans[i];if(s.point)observations.points.append(*s.point);scanSlots.items.append({QString::number(i),s.point ? "found":"missing",fitted.scans[i].used});}
    put("observations"+suffix,observations);put("observations"+other,PointSet{},false,"OTHER_PLANE");put("slots",scanSlots);
    InspectionResults scans;for(int i=0;i<metrics.samples.size();++i){const auto& s=metrics.samples[i];QString text="missing";if(s.point){text=QString("x=%1 y=%2 residual=%3 %4 %5").arg(s.point->x(),0,'g',17).arg(s.point->y(),0,'g',17).arg(*s.signedResidual,0,'g',17).arg(unit,s.fitUsed ? "used":"outlier");if(s.fitUsed)r.points.append(*search.scans[i].rawPoint);else r.rejectedPoints.append(*search.scans[i].rawPoint);}scans.items.append({QString("scan %1 angle %2").arg(s.scanId).arg(s.angleDegrees,0,'g',17),text,s.fitUsed});}put("scans",scans);
    // Sample the fitted circle/arc in its own plane, inverse-map each point.
    // Never map a raw fitted centre or multiply a pixel radius by scale.
    QPointF previous=raw(polar(metrics.centre,metrics.radius,arc.startDegrees));
    for(int i=1;i<=180;++i){const auto next=raw(polar(metrics.centre,metrics.radius,arc.startDegrees+arc.sweepDegrees*i/180.));r.lines.append(QLineF(previous,next));previous=next;}
    r.message=QString("Diameter %1 %2 • deviation %3 • %4 • coverage %5 • %6").arg(metrics.diameter,0,'f',6).arg(unit).arg(qualified ? QString::number(metrics.deviation->peakToValley,'f',6):"NOT_EVALUATED").arg(scope).arg(metrics.coverage,0,'f',3).arg(qualified ? (diameterOk && deviationOk ? "accepted=true":"accepted=false"):"NOT_EVALUATED");
    stop();return r;
}
void CircleToolService::validateCompletion(const ToolDefinition& t,const ToolResult& r,const std::optional<CalibrationDependency>& binding){
    auto boolean=[&](const char* id){const auto v=r.outputs.value(id);require(v.valid && std::holds_alternative<bool>(v.data),"Circle Boolean missing");return std::get<bool>(v.data);};
    const bool world=t.parameters["mode"]=="calibrated",qualified=boolean("qualified");require(boolean("found"),"Circle DONE requires fitted points");
    require(!world || (r.calibration && binding && r.calibration==binding),"Circle calibration differs from snapshot");
    require(r.outputs.size()==outputPorts(t.kind).size(),"Circle completion ports missing");
    for(const auto& v:r.outputs)require(v.diagnostic==!world,"Circle diagnostic mode mismatch");
    const QString suffix=world ? "Mm":"Px",other=world ? "Px":"Mm";
    for(auto k:{"circle","arc","centre","observations","diameter","radius","fitRms","fitMaxResidual","radialMin","radialMax","maxInward","maxOutward","observedDeviation","deviation"})require(!r.outputs.value(QString(k)+other).valid,"Circle cannot publish the unmeasured plane");
    auto number=[&](QString id){const auto v=r.outputs.value(id);require(v.valid && std::holds_alternative<double>(v.data) && std::isfinite(std::get<double>(v.data)) && v.diagnostic==!world,"Circle metric/mode mismatch");return std::get<double>(v.data);};
    // Replay bounded nominal-plane observations against the immutable recipe.
    // Caller-supplied quality flags, metrics and tolerances are never trusted.
    const auto ov=r.outputs.value("observations"+suffix),sv=r.outputs.value("slots");
    require(ov.valid && std::holds_alternative<PointSet>(ov.data) && sv.valid && std::holds_alternative<InspectionResults>(sv.data),"Circle observation evidence missing");
    const auto observations=std::get<PointSet>(ov.data);const auto scanSlots=std::get<InspectionResults>(sv.data).items;
    const auto roi=ToolRoi::fromJson(t.parameters["scanRoi"].toObject());require(observations.frame==roi.frame && (!world || roi.frame==binding->target),"Circle observation frame mismatch");
    const auto arc=std::holds_alternative<Annulus>(roi.geometry) ? ArcRoi{std::get<Annulus>(roi.geometry),0,360}:std::get<ArcRoi>(roi.geometry);
    const auto p0=t.parameters;const int n=p0["scanCount"].toInt();require(scanSlots.size()==n,"Circle slot count mismatch");
    RadialSearch search;search.fullCircle=std::abs(arc.sweepDegrees)==360.;QVector<CircleFitPoint> points;int at=0;
    for(int i=0;i<n;++i){const auto slot=scanSlots[i];require(slot.name==QString::number(i) && (slot.reason=="found" || slot.reason=="missing"),"Circle scan slot malformed");std::optional<QPointF> q;if(slot.reason=="found"){require(at<observations.points.size(),"Circle point count mismatch");q=observations.points[at++];}RadialScanPoint s;s.scanId=i;s.angleDegrees=arc.startDegrees+i*(arc.sweepDegrees/(search.fullCircle ? n:n-1));s.point=q;search.scans.append(s);points.append({q});}
    require(at==observations.points.size(),"Circle extra points");
    CircleFitOptions fo;fo.minPoints=p0["minPoints"].toInt();fo.minCoverage=p0["minFitCoverage"].toDouble();fo.minAngularSpanDegrees=p0["minAngularSpan"].toDouble();fo.maxCondition=p0["maxCondition"].toDouble();fo.ransac=p0["ransac"].toBool();fo.inlierDistance=p0["inlierDistance"].toDouble();fo.maxRms=p0["maxRms"].toDouble();fo.maxResidual=p0["maxResidual"].toDouble();
    const auto fitted=CircleFitter::fit(points,fo);const auto metrics=CircleMetrics::measure(search,fitted,{p0["minFoundPoints"].toInt(),p0["minCoverage"].toDouble(),p0["maxGapDegrees"].toDouble()});
    for(int i=0;i<n;++i)require(scanSlots[i].accepted==fitted.scans[i].used,"Circle fit membership differs from observations");
    require(qualified==metrics.deviation.has_value(),"Circle qualification differs from observations");
    auto exact=[&](QString id,double expected){const auto got=number(id);if(got!=expected)throw std::runtime_error(QString("Circle %1 differs from observations: %2 vs %3").arg(id).arg(got,0,'g',17).arg(expected,0,'g',17).toStdString());};
    exact("diameter"+suffix,metrics.diameter);exact("radius"+suffix,metrics.radius);exact("fitRms"+suffix,metrics.fitRms);exact("fitMaxResidual"+suffix,metrics.fitMaxResidual);exact("observedDeviation"+suffix,metrics.observedDeviation.peakToValley);exact("radialMin"+suffix,metrics.observedDeviation.minimumRadius);exact("radialMax"+suffix,metrics.observedDeviation.maximumRadius);exact("maxInward"+suffix,metrics.observedDeviation.maxInward);exact("maxOutward"+suffix,metrics.observedDeviation.maxOutward);exact("coverage",metrics.coverage);exact("maxGapDegrees",metrics.maxGapDegrees);
    const auto cv=r.outputs.value("circle"+suffix);require(cv.valid && std::holds_alternative<Circle2D>(cv.data),"Circle geometry missing");const auto circle=std::get<Circle2D>(cv.data);require(circle.centre.x()==metrics.centre.x() && circle.centre.y()==metrics.centre.y() && circle.radius==metrics.radius && circle.frame==roi.frame,"Circle geometry differs from observations");
    const auto centre=r.outputs.value("centre"+suffix);require(centre.valid && std::holds_alternative<QPointF>(centre.data) && std::get<QPointF>(centre.data).x()==metrics.centre.x() && std::get<QPointF>(centre.data).y()==metrics.centre.y(),"Circle centre differs from observations");
    const auto av=r.outputs.value("arc"+suffix);require(av.valid==!search.fullCircle,"Circle arc validity mismatch");if(av.valid){require(std::holds_alternative<Arc2D>(av.data),"Circle arc missing");const auto a=std::get<Arc2D>(av.data);require(a.circle.centre.x()==circle.centre.x() && a.circle.centre.y()==circle.centre.y() && a.circle.radius==circle.radius && a.circle.frame==circle.frame && a.startDegrees==arc.startDegrees && a.sweepDegrees==arc.sweepDegrees,"Circle arc differs from recipe");}
    const auto ci=r.outputs.value("circularityIndex");require(ci.valid==metrics.contour.metrics.has_value(),"Circle contour validity mismatch");if(ci.valid){const auto index=number("circularityIndex");require(std::abs(index-metrics.contour.metrics->circularityIndex)<=16*std::numeric_limits<double>::epsilon(),"Circle polygon index differs from observations");}
    if(qualified)exact("deviation"+suffix,metrics.deviation->peakToValley);
    const auto diameter=number("diameter"+suffix);const auto deviation=r.outputs.value("deviation"+suffix);require(deviation.valid==qualified,"Circle quality/deviation mismatch");
    const auto scope=r.outputs.value("scope");require(scope.valid && std::holds_alternative<QString>(scope.data),"Circle scope missing");const auto s=std::get<QString>(scope.data);require(qualified ? s==(search.fullCircle ? "full-circle":"observed-arc"):s=="unavailable","Circle scope/quality mismatch");
    const auto coverage=number("coverage"),gap=number("maxGapDegrees");require(!qualified || (coverage>=t.parameters["minCoverage"].toDouble() && gap<=t.parameters["maxGapDegrees"].toDouble()),"Circle quality gate mismatch");
    if(!qualified){for(auto k:{"accepted","diameterAccepted","deviationAccepted"})require(!r.outputs.value(k).valid,"Unqualified Circle cannot carry tolerance");return;}
    const double pv=number("deviation"+suffix);const auto p=t.parameters;const bool a=diameter>=p["diameterLower"].toDouble() && diameter<=p["diameterUpper"].toDouble(),b=pv>=p["deviationLower"].toDouble() && pv<=p["deviationUpper"].toDouble();
    require(boolean("diameterAccepted")==a && boolean("deviationAccepted")==b && boolean("accepted")== (a && b),"Circle tolerance differs from snapshot");
}
}
