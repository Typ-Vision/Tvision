#include "PoseMatchService.hpp"
#include "vision/matching/ShapeMatcher.hpp"
#include "MatchService.hpp"
#include "ToolResultCodec.hpp"
#include "domain/ToolRegistry.hpp"
#include "domain/PoseMatchConfig.hpp"
#include "vision/ImageAccess.hpp"
#include "vision/matching/PoseMatcher.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QJsonArray>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
QJsonObject PoseMatchService::teach(const ImageFrame& image,const QRect& roi,QPointF anchor,const QJsonObject& parameters){
    validatePoseMatch(parameters);
    require(image.format()==PixelFormat::Gray8,"Pose teaching requires Gray8; use Preprocess for colour/16 bit input");
    require(roi.width()>=16 && roi.height()>=16 && roi.width()<=256 && roi.height()<=256,"Pose model must be 16..256 px");
    require(std::isfinite(anchor.x()) && std::isfinite(anchor.y()) && anchor.x()>=0 && anchor.y()>=0 && anchor.x()<=roi.width()-1 && anchor.y()<=roi.height()-1,"Anchor outside taught model");
    auto p=parameters;p["modelPng"]=MatchService{}.teach(image,roi);p["modelRoi"]=MatchService::rectangle(roi);p["anchor"]=QJsonObject{{"x",anchor.x()},{"y",anchor.y()}};validatePoseMatch(p);return p;
}
ToolResult PoseMatchService::execute(const ToolDefinition& tool,const QMap<QString,PortValue>& inputs,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel){
    ToolRegistry::validate(tool);require(tool.kind=="pattern_pose" || tool.kind=="shape_pose","Unsupported pose method");require(inputs.size()==1 && inputs.contains("image"),"Pose requires only the image input");
    const auto input=inputs.value("image");ToolResultCodec::validate(input,inputPorts(tool.kind).first(),frameId,cycleId);
    require(input.valid && std::holds_alternative<ImageFrame>(input.data),"Pose image is unavailable");
    const auto image=std::get<ImageFrame>(input.data);const auto p=tool.parameters;
    require(!p["anchor"].toObject().isEmpty(),"Pose teaching anchor is missing");
    const auto size=MatchService::validateModel(MatchService::parameters(p));require(size.width()>=16 && size.height()>=16 && size.width()<=256 && size.height()<=256,"Pose model must be 16..256 px");
    auto bytes=QByteArray::fromBase64(p["modelPng"].toString().toLatin1());std::vector<uchar> buffer(bytes.begin(),bytes.end());auto model=cv::imdecode(buffer,cv::IMREAD_GRAYSCALE);
    require(image.format()==PixelFormat::Gray8,"pattern_pose requires Gray8; connect Preprocess for colour/16 bit input");
    const auto raw=ImageAccess(image).ownedCopy();auto roi=MatchService::rectangle(p["searchRoi"].toObject());if(roi.isEmpty())roi=QRect(QPoint(0,0),image.size());
    PoseSearch options;options.angleMin=p["angleMin"].toDouble();options.angleMax=p["angleMax"].toDouble();options.angleStep=p["angleStep"].toDouble();options.scaleMin=p["scaleMin"].toDouble();options.scaleMax=p["scaleMax"].toDouble();options.scaleStep=p["scaleStep"].toDouble();options.minScore=p["minScore"].toDouble();options.ambiguityGap=p["ambiguityGap"].toDouble();options.nmsDistance=p["nmsDistance"].toDouble();options.pyramidLevels=p["pyramidLevels"].toInt();options.maxCandidates=p["maxCandidates"].toInt();options.selectBest=p["selection"]=="best";
    const auto anchor=p["anchor"].toObject();QElapsedTimer timer;timer.start();
    const auto matcher=tool.kind=="shape_pose" ? ShapeMatcher::find : PoseMatcher::find;
    const auto result=matcher(raw,model,{anchor["x"].toDouble(),anchor["y"].toDouble()},{roi.x(),roi.y(),roi.width(),roi.height()},options,cancel);
    ToolResult r;r.toolId=tool.id;r.state=ExecutionState::Done;r.searchRegion=roi;r.milliseconds=timer.nsecsElapsed()/1e6;r.message=result.ambiguous ? "AMBIGUOUS":result.selected ? "FOUND":"NOT_FOUND";
    auto put=[&](const QString& id,PortData data,bool valid=true){for(const auto& port:outputPorts(tool.kind))if(port.id==id){PortValue v{port,std::move(data),valid,frameId,cycleId,image.metadata()};v.method=tool.kind=="shape_pose" ? "unsigned-oriented-chamfer-pyramid" : "masked-ccoeff-normed-pyramid";v.methodVersion=1;if(!valid)v.invalidReason=r.message;r.outputs[id]=std::move(v);}};
    put("found",!result.candidates.empty());put("ambiguous",result.ambiguous);put("truncated",result.truncated);
    QJsonArray candidates;for(const auto& c:result.candidates)candidates.append(QJsonObject{{"x",c.anchor.x},{"y",c.anchor.y},{"angleRadians",c.angleRadians},{"scale",c.scale},{"score",c.score}});
    put("candidates",QString::fromUtf8(QJsonDocument(candidates).toJson(QJsonDocument::Compact)));
    if(result.selected){const auto c=*result.selected;Pose2D pose{{c.anchor.x,c.anchor.y},c.angleRadians,{image.metadata().coordinateSystemId,LengthUnit::Pixel,AxisConvention::XRightYDown}};pose.validate();put("pose",pose);put("scale",c.scale);put("score",c.score);r.points={pose.position};}
    else {put("pose",false,false);put("scale",false,false);put("score",false,false);}
    return r;
}
}
