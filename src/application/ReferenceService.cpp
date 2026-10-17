#include "ReferenceService.hpp"
#include "ToolResultCodec.hpp"
#include "WorldTrackingService.hpp"
#include "MeasurementService.hpp"
#include "domain/ToolRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QString method(const ToolDefinition& t){
    if(t.kind=="template_match")return "template-ncc";
    if(t.kind=="blob")return "connected-components";
    if(t.kind=="edge")return t.parameters.contains("referenceBinding") ? "reference-local-difference":"legacy-axis-difference";
    if(t.kind=="edge_point")return "profile-edge";
    if(t.kind=="edge_pair")return "profile-pair";
    if(t.kind=="line_fit")return "robust-line";
    return "radial-circle";
}
}
ReferenceResolution ReferenceService::current(const ToolDefinition& source,const QString& port,const ToolResult& result,const ImageFrame& image){
    try {
        ToolRegistry::validate(source);require(source.enabled && source.id==result.toolId,"Reference source/result identity invalid");
        require(referenceSourcePort(source.kind,port),"Unsupported reference source port");
        if(port=="pose") {
            const auto pose=PoseRoiService::currentReference(source,result,image);
            if(!pose.pose)return {pose.state,{},{},pose.reason};
            return {ExecutionState::Done,ReferenceAnchor(*pose.pose),{}, {}};
        }
        if(result.state!=ExecutionState::Done)return {ExecutionState::Skipped,{},{},"Reference source not DONE"};
        const auto m=image.metadata();require(m.completedAtUtc.isValid(),"Reference acquisition time missing");
        QString evaluation;
        for(const auto& id:{port,QString("found")}) {
            const auto ports=outputPorts(source.kind);auto d=std::find_if(ports.begin(),ports.end(),[&](const auto& p){return p.id==id;});
            require(d!=ports.end() && result.outputs.contains(id),"Reference output missing");const auto v=result.outputs[id];
            ToolResultCodec::validate(v,*d,m.frameId,m.cycleId);
            require(v.origin.sourceId==m.sourceId && v.origin.coordinateSystemId==m.coordinateSystemId && v.origin.completedAtUtc==m.completedAtUtc &&
                v.origin.exposureAtUtc==m.exposureAtUtc && v.origin.deviceClockId==m.deviceClockId && v.origin.deviceTimestampTicks==m.deviceTimestampTicks,"Reference acquisition differs");
            // Raw metrology ports are diagnostic for metric measurement but remain real raw reference points.
            const bool world=source.parameters.contains("referenceBinding") ? toolReferenceBinding(source).world:isWorldPoseTracking(source);
            require(!v.evaluationId.isEmpty() && v.method==(world ? "world-raw-rigid-refit":method(source)) && v.methodVersion==(world ? 2:1),"Reference method/evaluation invalid");
            const bool rawDiagnostic=(source.kind=="edge_pair" || source.kind=="line_fit" || source.kind=="circle_fit") && source.parameters["mode"]!="calibrated";
            require(v.diagnostic==rawDiagnostic,"Reference producer diagnostic contract differs");
            if(evaluation.isEmpty())evaluation=v.evaluationId;require(evaluation==v.evaluationId,"Reference evaluation differs");
        }
        const auto found=result.outputs["found"],v=result.outputs[port];
        require(found.valid && std::holds_alternative<bool>(found.data),"Reference found diagnostic invalid");
        if(!std::get<bool>(found.data) || !v.valid)return {ExecutionState::Skipped,{},{},"Reference point unavailable/not found"};
        require(std::holds_alternative<QPointF>(v.data),"Reference point type invalid");const auto p=std::get<QPointF>(v.data);
        require(std::isfinite(p.x()) && std::isfinite(p.y()) && p.x()>=0 && p.y()>=0 && p.x()<=image.size().width()-1 && p.y()<=image.size().height()-1,"Reference point outside image");
        return {ExecutionState::Done,ReferenceAnchor(Point2D{p,{m.coordinateSystemId}}),{}, {}};
    }catch(const std::exception& e){return {ExecutionState::Error,{},{},QString::fromUtf8(e.what())};}
}
ToolDefinition ReferenceService::teach(const ToolDefinition& consumer,const ToolDefinition& source,const QString& port,const ToolResult& result,const ImageFrame& image,QPointF offset,double angle){
    const auto currentResult=current(source,port,result,image);require(currentResult.anchor.has_value(),currentResult.reason.toUtf8().constData());
    auto tool=consumer;tool.parameters.remove("poseTracking");tool.parameters.remove("referenceBinding");ToolRegistry::validate(tool);
    ReferenceBinding b;b.mode=port=="pose" ? ReferenceMode::Rigid:ReferenceMode::Translation;b.sourceTool=source.id;b.sourcePort=port;
    b.geometryId=referenceGeometryIdentity(source,port);b.imageSource=image.metadata().sourceId;b.rawSize=image.size();b.taught=*currentResult.anchor;
    b.nominal=referenceNominalRoi(tool,{image.metadata().coordinateSystemId});b.offsetPx=offset;b.angleOffsetRadians=angle;
    b.resolve(b.taught,1,image.size());tool.parameters["referenceBinding"]=b.toJson();ToolRegistry::validate(tool);return tool;
}
ReferenceResolution ReferenceService::resolve(const ReferenceBinding& b,const ToolDefinition& source,const ToolResult& result,const ImageFrame& image){
    try {
        b.validate();require(b.sourceTool==source.id && b.geometryId==referenceGeometryIdentity(source,b.sourcePort),"Reference geometry/port changed; reteach");
        require(b.imageSource==image.metadata().sourceId && b.rawSize==image.size() && b.nominal.frame.id==image.metadata().coordinateSystemId,"Reference source/frame/resolution changed");
        require(!b.world,"World reference requires calibrated mapper");
        auto r=current(source,b.sourcePort,result,image);if(!r.anchor)return r;
        r.roi=b.resolve(*r.anchor,1,image.size());return r;
    }catch(const std::exception& e){return {ExecutionState::Error,{},{},QString::fromUtf8(e.what())};}
}
WorldTrackingResolution ReferenceService::resolveWorld(const ReferenceBinding& b,const ToolDefinition& source,const ToolResult& result,const ImageFrame& image,
    const MeasurementExecutionContext& ctx,const std::shared_ptr<std::atomic_bool>& cancel){
    try{
        b.validate();require(b.world && b.calibration.has_value(),"World reference binding missing");
        require(b.sourceTool==source.id && b.geometryId==referenceGeometryIdentity(source,b.sourcePort),"Reference geometry/port changed; reteach");
        require(b.imageSource==image.metadata().sourceId && b.rawSize==image.size() && b.calibration->source.id==image.metadata().coordinateSystemId,"Reference source/frame/resolution changed");
        const auto r=current(source,b.sourcePort,result,image);if(!r.anchor)return {r.state,{},r.reason};
        return {ExecutionState::Done,PoseRoiService::calibratedMap(*b.calibration,b.rawTransform(*r.anchor,1,image.size()),image,ctx,cancel),{}};
    }catch(const std::exception& e){return {ExecutionState::Error,{},QString::fromUtf8(e.what())};}
}
ToolDefinition ReferenceService::teachWorld(const ToolDefinition& consumer,const ToolRoi& nominal,double unit,const ToolDefinition& source,const QString& port,
    const ToolResult& result,const ImageFrame& image,const MeasurementExecutionContext& ctx,QPointF offset,double angle,const std::shared_ptr<std::atomic_bool>& cancel){
    require(ctx.current && ctx.calibration && ctx.calibration->measurementReady(),"World teaching requires current active or accepted legacy calibration");
    const auto r=current(source,port,result,image);require(r.anchor.has_value(),r.reason.toUtf8().constData());
    auto tool=consumer;tool.parameters.remove("poseTracking");tool.parameters.remove("referenceBinding");tool.parameters["scanRoi"]=nominal.toJson();
    if(tool.kind!="edge_point")tool.parameters["mode"]="calibrated";if(tool.kind=="circle_fit")tool.parameters["unitsPerSample"]=unit;
    ReferenceBinding b;b.world=true;b.mmPerSample=unit;b.calibration=ctx.calibration->dependency();b.nominal=nominal;
    b.mode=port=="pose" ? ReferenceMode::Rigid:ReferenceMode::Translation;b.sourceTool=source.id;b.sourcePort=port;
    b.geometryId=referenceGeometryIdentity(source,port);b.imageSource=image.metadata().sourceId;b.rawSize=image.size();b.taught=*r.anchor;b.offsetPx=offset;b.angleOffsetRadians=angle;
    tool.parameters["referenceBinding"]=b.toJson();ToolRegistry::validate(tool);
    const auto mapped=resolveWorld(b,source,result,image,ctx,cancel);require(mapped.map.has_value(),mapped.reason.toUtf8().constData());
    WorldTrackingService::preflight(tool,image,*mapped.map,cancel);return tool;
}
}
