#include "GeometryMeasurementService.hpp"
#include "CircleToolService.hpp"
#include "ToolResultCodec.hpp"
#include "MeasurementService.hpp"
#include "domain/GeometryMeasurements.hpp"
#include "domain/GeometryMeasurementConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "vision/metrology/LineFitter.hpp"
#include "vision/metrology/CircleFitter.hpp"
#include <cmath>
#include <algorithm>
#include <numbers>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
ToolResult GeometryMeasurementService::execute(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& frame,const QString& cycle,
    const std::shared_ptr<std::atomic_bool>& cancel,const MeasurementExecutionContext& ctx){
    ToolRegistry::validate(t);const auto p=t.parameters;const bool world=p["mode"]=="calibrated";
    auto stop=[&]{require(!cancel || !cancel->load(),"Geometry measurement cancelled");};stop();
    const auto required=geometryMeasurementRequiredInputs(p);
    require(inputs.size()==required.size(),"Geometry operation inputs missing/extra");
    const auto declarations=geometryMeasurementInputs();
    for(const auto& id:required){
        require(inputs.contains(id),"Geometry operation input missing");const auto v=inputs[id];
        auto expected=*std::find_if(declarations.begin(),declarations.end(),[&](const auto& d){return d.id==id;});
        expected.id=v.port.id;require(v.valid,"Geometry input invalid");ToolResultCodec::validate(v,expected,frame,cycle);
    }
    const auto image=std::get<ImageFrame>(inputs["image"].data);const auto m=image.metadata();
    require(m.completedAtUtc.isValid(),"Geometry acquisition time missing");
    for(const auto& v:inputs)require(v.origin.sourceId==m.sourceId && v.origin.coordinateSystemId==m.coordinateSystemId &&
        v.origin.frameId==m.frameId && v.origin.cycleId==m.cycleId && v.origin.completedAtUtc==m.completedAtUtc &&
        v.origin.exposureAtUtc==m.exposureAtUtc && v.origin.deviceTimestampTicks==m.deviceTimestampTicks && v.origin.deviceClockId==m.deviceClockId,"Geometry inputs differ from actual acquisition");
    const CoordinateFrame rawFrame{m.coordinateSystemId,LengthUnit::Pixel,AxisConvention::XRightYDown};
    if(world){
        if(!ctx.calibration || !ctx.current)throw std::runtime_error((QString("Geometry requires current active or accepted legacy calibration. ")+
            (ctx.error.isEmpty() ? QString("Current calibration unavailable"):ctx.error)+
            "; Runtime: Stop → Kaynağı doğrula. Model/kurulum eksikse Setup → Calibration üzerinden kayıtlı exact seçimi ve kaynak bağını denetleyin.").toUtf8().constData());
        require(ctx.rawSize==image.size(),"Geometry resolver actual resolution differs from image");
        require(!ctx.calibration->dependency().cameraId.startsWith("calibration-test:"),"File-test calibration cannot measure production geometry");
    }
    const auto targetFrame=world ? ctx.calibration->dependency().target:rawFrame;
    auto rawCheck=[&](QPointF q){require(std::isfinite(q.x()) && std::isfinite(q.y()) && q.x()>=0 && q.y()>=0 && q.x()<=image.size().width()-1 && q.y()<=image.size().height()-1,"Geometry point outside actual raw image");};
    auto map=[&](QPointF q){stop();rawCheck(q);return world ? CircleToolService::mapPoint(image,ctx,q,false):q;};
    auto inverse=[&](QPointF q){stop();const auto raw=world ? CircleToolService::mapPoint(image,ctx,q,true):q;rawCheck(raw);return raw;};
    auto point=[&](const QString& id){return Point2D{map(std::get<QPointF>(inputs[id].data)),targetFrame};};
    // Raw fits are type/direction seeds only. Every source observation is mapped;
    // membership and physical geometry are recomputed in the destination plane.
    auto observations=[&](const QString& feature,const QString& sample){
        const auto& v=inputs[feature];const auto& s=inputs[sample];
        require(v.evaluationId==s.evaluationId && v.method==s.method && v.methodVersion==s.methodVersion,"Geometry feature and samples have different evaluations/methods");
        const auto set=std::get<PointSet>(s.data);require(set.frame==rawFrame && set.points.size()>=p["fitMinPoints"].toInt() && set.points.size()<=128,"Geometry observations frame/count invalid");
        QVector<QPointF> out;for(auto q:set.points)out.append(map(q));return out;
    };
    auto line=[&](const QString& id,const QString& sample){
        const auto seed=std::get<Line2D>(inputs[id].data);require(seed.frame==rawFrame,"Geometry line raw frame mismatch");
        if(!world){rawCheck(seed.start);rawCheck(seed.end);return seed;}
        const auto mapped=observations(id,sample);const auto a=map(seed.start),b=map(seed.end);
        QVector<LineScanPoint> scans;for(auto q:mapped)scans.append({q});
        LineFitOptions o;o.minPoints=p["fitMinPoints"].toInt();o.minCoverage=.6;o.minSpanCoverage=.5;o.minSpan=p["lineMinSpan"].toDouble();
        o.inlierDistance=p["fitInlierDistance"].toDouble();o.maxRms=p["fitMaxRms"].toDouble();o.maxResidual=p["fitMaxResidual"].toDouble();
        o.ransac=p["fitRansac"].toBool();o.minVarianceRatio=p["lineMinVarianceRatio"].toDouble();
        const auto fit=[&]{try{return LineFitter::fit(scans,b-a,o,cancel);}catch(const FitDiagnosticError& e){auto d=e.evidence;d.stage="geometry_refit";throw FitDiagnosticError(e.what(),std::move(d));}}();inverse(fit.supportStart);inverse(fit.supportEnd);
        return Line2D{fit.supportStart,fit.supportEnd,targetFrame};
    };
    auto circle=[&](const QString& id,const QString& sample){
        const auto seed=std::get<Circle2D>(inputs[id].data);require(seed.frame==rawFrame,"Geometry circle raw frame mismatch");
        if(!world){rawCheck(seed.centre);return seed;}
        QVector<CircleFitPoint> scans;for(auto q:observations(id,sample))scans.append({q});
        CircleFitOptions o;o.minPoints=p["fitMinPoints"].toInt();o.minCoverage=.6;o.minAngularSpanDegrees=p["circleMinAngularSpan"].toDouble();
        o.maxCondition=p["circleMaxCondition"].toDouble();o.inlierDistance=p["fitInlierDistance"].toDouble();o.maxRms=p["fitMaxRms"].toDouble();
        o.maxResidual=p["fitMaxResidual"].toDouble();o.ransac=p["fitRansac"].toBool();
        const auto fit=[&]{try{return CircleFitter::fit(scans,o,cancel);}catch(const FitDiagnosticError& e){auto d=e.evidence;d.stage="geometry_refit";throw FitDiagnosticError(e.what(),std::move(d));}}();inverse(fit.centre);
        return Circle2D{fit.centre,fit.radius,targetFrame};
    };
    // Enforce currentness even for operations whose output is unavailable.
    // Feature points below still get their own hull/bounds checks.
    if(world){const auto& c=*ctx.calibration;require(c.measurementReady(),"Geometry calibration has no active readiness or accepted legacy evidence");c.dependency().requireCurrent(*ctx.current);}
    ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;
    const auto mode=p["offset"]=="signed" ? OffsetMode::Signed:OffsetMode::Absolute;
    const auto op=p["operation"].toString();std::optional<double> value,angle;std::optional<Point2D> intersection;QString status="MEASURED";
    using G=GeometryMeasurements;
    auto foot=[&](QPointF q,const Line2D& l){const auto d=l.end-l.start;const double length=std::hypot(d.x(),d.y());require(length>G::minimumLineLength,"Geometry support degenerate");const auto u=d/length;
        const auto v=q-l.start;const double along=v.x()*u.x()+v.y()*u.y();const auto f=l.start+along*u;
        if(world){require(along>=0 && along<=length,"Geometry perpendicular projection outside measured line support");inverse(f);}return f;};
    if(op=="point_distance" || op=="x_offset" || op=="y_offset"){
        const auto a=point("p1"),b=point("p2");value=op=="point_distance" ? G::pointDistance(a,b):op=="x_offset" ? G::xOffset(a,b,mode):G::yOffset(a,b,mode);r.points={inverse(a.position),inverse(b.position)};
    }else if(op=="point_line_distance" || op=="point_segment_distance"){
        const auto a=point("p1");const auto l=line("line1","samples1");
        value=op=="point_line_distance" ? G::pointLineDistance(a,l,mode):G::pointSegmentDistance(a,l);
        if(op=="point_line_distance")r.points.append(inverse(foot(a.position,l)));
        else {const auto d=l.end-l.start;const double length=std::hypot(d.x(),d.y());require(length>G::minimumLineLength,"Geometry support degenerate");const auto u=d/length,v=a.position-l.start;const double along=std::clamp(v.x()*u.x()+v.y()*u.y(),0.,length);r.points.append(inverse(l.start+along*u));}
        r.points.append(inverse(a.position));r.lines.append(QLineF(inverse(l.start),inverse(l.end)));
    }else if(op=="line_angle" || op=="line_intersection" || op=="parallel_distance"){
        const auto a=line("line1","samples1"),b=line("line2","samples2");
        if(op=="line_angle"){const auto am=p["angle"]=="signed_directed" ? AngleMode::SignedDirected:p["angle"]=="unsigned_directed" ? AngleMode::UnsignedDirected:AngleMode::AcuteUndirected;angle=G::lineAngleDegrees(a,b,am);}
        else if(op=="parallel_distance"){value=G::parallelLineDistance(a,b,mode);if(world)foot(b.start,a);}
        else {const auto hit=G::lineIntersection(a,b);intersection=hit.point;
            if(intersection){if(world){foot(intersection->position,a);foot(intersection->position,b);}r.points.append(inverse(intersection->position));}
            else status=hit.kind==LineIntersectionKind::Parallel ? "PARALLEL":"COINCIDENT";}
        r.lines={QLineF(inverse(a.start),inverse(a.end)),QLineF(inverse(b.start),inverse(b.end))};
    }else {const auto a=circle("circle1","samples1");r.points.append(inverse(a.centre));
        if(op=="circle_centre_distance"){const auto b=circle("circle2","samples2");value=G::circleCentreDistance(a,b);r.points.append(inverse(b.centre));}
        else value=G::circleSize(a,op=="circle_radius" ? CircleSize::Radius:CircleSize::Diameter);
    }
    for(const auto& port:geometryMeasurementOutputs()){
        PortValue v{port,false,false,frame,cycle,m};v.method="geometry-refit/"+op;v.methodVersion=2;v.diagnostic=!world;
        const QString suffix=world ? "Mm":"Px";
        if(port.id=="accepted" && p["toleranceEnabled"].toBool() && (value || angle)){const auto metric=value ? *value:*angle;v.data=metric>=p["lower"].toDouble() && metric<=p["upper"].toDouble();v.valid=true;}
        if(port.id=="status"){v.data=status;v.valid=true;}
        if(port.id=="value"+suffix && value){v.data=*value;v.valid=true;}
        if(port.id=="angle"+suffix && angle){v.data=*angle;v.valid=true;}
        if(port.id=="intersection"+suffix && intersection){v.data=intersection->position;v.valid=true;}
        if(!v.valid)v.invalidReason=status=="MEASURED" ? "NOT_APPLICABLE":status;
        r.outputs[port.id]=std::move(v);
    }
    if(world)r.calibration=ctx.calibration->dependency();
    // Helper segments join actual mapped features / perpendicular feet, never a fabricated result.
    if(r.points.size()==2)r.lines.append(QLineF(r.points[0],r.points[1]));
    r.message=op+": "+status;
    if(value || angle)r.message+=" • "+QString::number(value ? *value:*angle,'g',12)+" "+(angle ? "deg":world ? "mm":"px");
    r.message+=r.outputs["accepted"].valid ? (std::get<bool>(r.outputs["accepted"].data) ? " • accepted=true":" • accepted=false"):" • NOT_EVALUATED";stop();return r;
}
}
