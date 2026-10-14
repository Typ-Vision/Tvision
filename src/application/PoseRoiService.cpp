#include "PoseRoiService.hpp"
#include "CircleToolService.hpp"
#include "MeasurementService.hpp"
#include "ToolResultCodec.hpp"
#include "WorldTrackingService.hpp"
#include "domain/ToolRegistry.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
struct Unavailable:std::runtime_error {using std::runtime_error::runtime_error;};
QString referenceMethod(const ToolDefinition& source){
    ToolRegistry::validate(source);
    if(source.kind=="pattern_pose")return "masked-ccoeff-normed-pyramid";
    if(source.kind=="shape_pose")return "unsigned-oriented-chamfer-pyramid";
    throw std::runtime_error("Tracking needs pattern_pose or shape_pose source");
}
Pose2D reference(const ToolDefinition& source,const ToolResult& result,const ImageFrame& image){
    const auto method=referenceMethod(source);require(source.enabled,"Tracking needs enabled pose source");
    require(source.id==result.toolId,"Tracking source/result tool mismatch");
    if(result.state!=ExecutionState::Done)throw Unavailable("Tracking reference not DONE");
    const auto m=image.metadata();require(m.completedAtUtc.isValid(),"Tracking acquisition time missing");
    QString evaluation;
    for(const auto& d:outputPorts(source.kind))if(d.id=="pose" || d.id=="scale" || d.id=="found" || d.id=="ambiguous" || d.id=="truncated"){
        require(result.outputs.contains(d.id),"Tracking reference port missing");const auto& v=result.outputs[d.id];
        ToolResultCodec::validate(v,d,m.frameId,m.cycleId);
        require(v.frameId==m.frameId && v.cycleId==m.cycleId && v.origin.sourceId==m.sourceId && v.origin.coordinateSystemId==m.coordinateSystemId &&
            v.origin.frameId==m.frameId && v.origin.cycleId==m.cycleId && v.origin.completedAtUtc==m.completedAtUtc &&
            v.origin.exposureAtUtc==m.exposureAtUtc && v.origin.deviceTimestampTicks==m.deviceTimestampTicks && v.origin.deviceClockId==m.deviceClockId,"Tracking acquisition mismatch");
        require(!v.evaluationId.isEmpty() && !v.diagnostic && v.method==method && v.methodVersion==1,"Tracking reference method/evaluation invalid");
        if(evaluation.isEmpty())evaluation=v.evaluationId;require(v.evaluationId==evaluation,"Tracking pose/scale evaluation mismatch");
    }
    const auto& outputs=result.outputs;
    for(const auto& id:{"found","ambiguous","truncated"})require(outputs[id].valid,"Tracking selection diagnostic unavailable");
    if(!std::get<bool>(outputs["found"].data))throw Unavailable("Tracking reference not found");
    if((std::get<bool>(outputs["ambiguous"].data) || std::get<bool>(outputs["truncated"].data)) && source.parameters["selection"]!="best")throw Unavailable("Tracking ambiguous reference rejected");
    if(!outputs["pose"].valid || !outputs["scale"].valid)throw Unavailable("Tracking selected pose/scale unavailable");
    require(std::get<double>(outputs["scale"].data)==1.0,"Rigid tracking requires scale exactly 1");
    auto pose=std::get<Pose2D>(outputs["pose"].data);pose.validate();
    require(pose.position.x()>=0 && pose.position.y()>=0 && pose.position.x()<=image.size().width()-1 && pose.position.y()<=image.size().height()-1,"Tracking anchor outside image");return pose;
}
}
QString PoseRoiService::geometryIdentity(const ToolDefinition& source){
    referenceMethod(source);
    QJsonObject o{{"kind",source.kind},{"modelPng",source.parameters["modelPng"]},{"modelRoi",source.parameters["modelRoi"]},{"anchor",source.parameters["anchor"]}};
    require(!o["modelPng"].toString().isEmpty() && !o["anchor"].toObject().isEmpty(),"Tracking model not taught");
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
PoseReferenceResolution PoseRoiService::currentReference(const ToolDefinition& source,const ToolResult& result,const ImageFrame& image){
    try{return {ExecutionState::Done,reference(source,result,image),{}};}
    catch(const Unavailable& e){return {ExecutionState::Skipped,{},QString::fromUtf8(e.what())};}
    catch(const std::exception& e){return {ExecutionState::Error,{},QString::fromUtf8(e.what())};}
}
ToolDefinition PoseRoiService::teachTool(const ToolDefinition& consumer,const CoordinateFrame& local,
    const ToolDefinition& source,const ToolResult& result,const ImageFrame& image){
    auto taught=consumer;taught.parameters.remove("poseTracking");ToolRegistry::validate(taught);
    require(supportsPoseTracking(taught.kind),"Tool does not support pose tracking");
    require(!taught.parameters.contains("mode") || taught.parameters["mode"]=="pixel","Raw tracking requires pixel mode; use teachWorldTool");
    const auto binding=teach(nominalTrackingRoi(taught,{image.metadata().coordinateSystemId}),local,source,result,image);
    taught.parameters["poseTracking"]=QJsonObject{{"binding",binding.toJson()},{"roiIdentity",scanRoiIdentity(taught)}};
    ToolRegistry::validate(taught);return taught;
}
ToolDefinition PoseRoiService::teachWorldTool(const ToolDefinition& consumer,const ToolRoi& nominal,double unit,
    const ToolDefinition& source,const ToolResult& result,const ImageFrame& image,const MeasurementExecutionContext& ctx,
    const std::shared_ptr<std::atomic_bool>& cancel){
    require(ctx.calibration && ctx.current && ctx.calibration->validated(),"World teaching requires validated current calibration");
    auto taught=consumer;taught.parameters.remove("poseTracking");
    taught.parameters["scanRoi"]=nominal.toJson();
    if(taught.kind!="edge_point")taught.parameters["mode"]="calibrated";
    if(taught.kind=="circle_fit")taught.parameters["unitsPerSample"]=unit;
    WorldPoseTracking b{reference(source,result,image),source.id,geometryIdentity(source),image.metadata().sourceId,scanRoiIdentity(taught),image.size(),ctx.calibration->dependency(),unit};
    taught.parameters["poseTracking"]=b.toJson();ToolRegistry::validate(taught);
    // Preflight the actual configured sampling footprint, including transverse
    // taps and budgets. Missing/poor feature fits do not invalidate teaching.
    WorldTrackingService::preflight(taught,image,worldMap(b,source,result,image,ctx,cancel),cancel);
    return taught;
}
PoseRoi PoseRoiService::teach(const ToolRoi& raw,const CoordinateFrame& local,const ToolDefinition& source,const ToolResult& result,const ImageFrame& image){
    require(raw.inside(image.size()),"Tracking teaching footprint outside image");return PoseRoi::teach(raw,reference(source,result,image),local,source.id,geometryIdentity(source),image.size(),image.metadata().sourceId);
}
PoseRoiResolution PoseRoiService::resolve(const PoseRoi& binding,const ToolDefinition& source,const ToolResult& result,const ImageFrame& image){
    try{binding.validate();require(binding.sourceTool==source.id && binding.geometryId==geometryIdentity(source),"Tracking teaching stale; model/anchor changed");
        require(binding.imageSource==image.metadata().sourceId,"Tracking acquisition source changed; reteach required");
        auto pose=reference(source,result,image);return {ExecutionState::Done,binding.resolve(pose,1.,image.size()),{}};
    }catch(const Unavailable& e){return {ExecutionState::Skipped,{},QString::fromUtf8(e.what())};}
    catch(const std::exception& e){return {ExecutionState::Error,{},QString::fromUtf8(e.what())};}
}
WorldTrackingMap PoseRoiService::worldMap(const PoseRoi& binding,const ToolDefinition& source,const ToolResult& result,const ImageFrame& image,
    const MeasurementExecutionContext& ctx,const std::shared_ptr<std::atomic_bool>& cancel){
    const auto resolved=resolve(binding,source,result,image);require(resolved.state==ExecutionState::Done,"World tracking reference unavailable");
    require(binding.calibration.has_value(),"World tracking requires teaching calibration");
    WorldPoseTracking world{binding.taughtPose,binding.sourceTool,binding.geometryId,binding.imageSource,binding.geometryId,binding.rawSize,*binding.calibration,1.};
    return worldMap(world,source,result,image,ctx,cancel);
}
WorldTrackingResolution PoseRoiService::resolveWorld(const WorldPoseTracking& binding,const ToolDefinition& source,const ToolResult& result,const ImageFrame& image,
    const MeasurementExecutionContext& ctx,const std::shared_ptr<std::atomic_bool>& cancel){
    try{return {ExecutionState::Done,worldMap(binding,source,result,image,ctx,cancel),{}};}
    catch(const Unavailable& e){return {ExecutionState::Skipped,{},QString::fromUtf8(e.what())};}
    catch(const std::exception& e){return {ExecutionState::Error,{},QString::fromUtf8(e.what())};}
}
WorldTrackingMap PoseRoiService::worldMap(const WorldPoseTracking& binding,const ToolDefinition& source,const ToolResult& result,const ImageFrame& image,
    const MeasurementExecutionContext& ctx,const std::shared_ptr<std::atomic_bool>& cancel){
    require(!cancel || !cancel->load(),"World tracking cancelled");
    binding.validate();
    require(binding.sourceTool==source.id && binding.geometryId==geometryIdentity(source),"World tracking teaching stale; model/anchor changed");
    require(binding.imageSource==image.metadata().sourceId && binding.rawSize==image.size(),"World tracking acquisition source/resolution changed");
    require(ctx.current && ctx.calibration && ctx.calibration->validated(),"World tracking requires bound validated calibration");
    binding.calibration.requireCurrent(*ctx.current);binding.calibration.requireCurrent(ctx.calibration->dependency());
    require(ctx.rawSize==image.size() && binding.calibration.resolution==image.size(),"World tracking actual resolution mismatch");
    require(!binding.calibration.cameraId.startsWith("calibration-test:"),"File-test calibration cannot track production ROI");
    const auto current=reference(source,result,image);
    const auto delta=poseTransform(binding.taughtPose,CoordinateFrame{"tracking/delta"}).inverse().then(poseTransform(current,CoordinateFrame{"tracking/delta"}));
    return calibratedMap(binding.calibration,delta,image,ctx,cancel);
}
WorldTrackingMap PoseRoiService::calibratedMap(const CalibrationDependency& dependency,const Transform2D& delta,const ImageFrame& image,
    const MeasurementExecutionContext& ctx,const std::shared_ptr<std::atomic_bool>& cancel){
    require(!cancel || !cancel->load(),"World tracking cancelled");delta.validate();
    require(ctx.current && ctx.calibration && ctx.calibration->validated(),"World tracking requires validated calibration");
    dependency.requireCurrent(*ctx.current);dependency.requireCurrent(ctx.calibration->dependency());
    require(ctx.rawSize==image.size() && dependency.resolution==image.size(),"World tracking actual resolution mismatch");
    require(!dependency.cameraId.startsWith("calibration-test:"),"File-test calibration cannot track production ROI");
    require(delta.source==dependency.source && delta.target==dependency.source,"World transform raw frame mismatch");
    // Copy the resolver value and immutable image/calibration ownership. Neither
    // caller edits nor stack lifetimes may change a sampling job halfway through.
    const auto rawFrame=delta.source;
    auto forward=[image,ctx,cancel](QPointF raw){
        require(!cancel || !cancel->load(),"World tracking cancelled");
        return CircleToolService::mapPoint(image,ctx,raw,false);
    };
    auto inverse=[image,ctx,cancel,delta,rawFrame,forward](QPointF world){
        require(!cancel || !cancel->load(),"World tracking cancelled");
        const auto taughtRaw=CircleToolService::mapPoint(image,ctx,world,true);
        const auto raw=delta.map(Point2D{taughtRaw,rawFrame}).position;
        forward(raw); // Every actual transverse/longitudinal tap checks current hull.
        return raw;
    };
    const auto content=image.contentId(),processing=image.processingId();
    auto sameImage=[image,content,processing,cancel](const ImageFrame& actual){
        require(!cancel || !cancel->load(),"World tracking cancelled");
        const auto& a=actual.metadata();const auto& b=image.metadata();
        require(actual.size()==image.size() && actual.format()==image.format() && a.sourceId==b.sourceId && a.frameId==b.frameId && a.cycleId==b.cycleId && a.coordinateSystemId==b.coordinateSystemId && a.completedAtUtc==b.completedAtUtc && a.exposureAtUtc==b.exposureAtUtc && a.deviceTimestampTicks==b.deviceTimestampTicks && a.deviceClockId==b.deviceClockId && actual.contentId()==content && actual.processingId()==processing,"World tracking sampling image differs from reference snapshot");
    };
    return {inverse,forward,sameImage,dependency.target,dependency};
}
QVector<Point2D> PoseRoiService::worldPoints(const PoseRoi& binding,const ToolDefinition& source,const ToolResult& result,const ImageFrame& image,
    const MeasurementExecutionContext& ctx,const QVector<Point2D>& nominal,const std::shared_ptr<std::atomic_bool>& cancel){
    require(nominal.size()>0 && nominal.size()<=8192,"World tracking point count 1..8192");
    const auto map=worldMap(binding,source,result,image,ctx,cancel);
    QVector<Point2D> points;points.reserve(nominal.size());
    for(const auto& p:nominal){require(p.frame==map.world,"World tracking point frame mismatch");
        points.append({map.rawToWorld(map.nominalToRaw(p.position)),map.world});
    }return points;
}
}
