#include "PoseTrackingConfig.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <stdexcept>
#include <cmath>
#include <QStringList>
#include "ToolValues.hpp"
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
void WorldPoseTracking::validate() const {
    if(imageGeometry)require(imageGeometry->cropDepth()>0 && imageGeometry->rootSize()==rawSize && imageGeometry->sourceId()==imageSource && imageGeometry->rootCoordinateSystemId()==taughtPose.frame.id,"World tracking crop scope differs");
    taughtPose.validate();calibration.validate();
    require(taughtPose.frame==calibration.source && taughtPose.frame.unit==LengthUnit::Pixel && taughtPose.frame.axes==AxisConvention::XRightYDown,"World tracking raw pose/frame mismatch");
    require(rawSize==calibration.resolution && rawSize.width()>0 && rawSize.height()>0 && qint64(rawSize.width())*rawSize.height()<=16*1024*1024,"World tracking resolution invalid");
    require(taughtPose.position.x()>=0 && taughtPose.position.y()>=0 && taughtPose.position.x()<=rawSize.width()-1 && taughtPose.position.y()<=rawSize.height()-1,"World tracking anchor outside image");
    require(!sourceTool.trimmed().isEmpty() && sourceTool.size()<=128 && !imageSource.trimmed().isEmpty() && imageSource.size()<=256 && taughtPose.frame.id.size()<=256,"World tracking identity invalid");
    for(const auto& id:{geometryId,roiIdentity})require(id.size()==64 && QByteArray::fromHex(id.toLatin1()).toHex()==id.toLatin1(),"World tracking SHA256 invalid");
    require(!calibration.cameraId.startsWith("calibration-test:"),"File-test calibration cannot teach world tracking");
    require(std::isfinite(mmPerSample) && mmPerSample>=1e-9 && mmPerSample<=1e9,"World tracking sample unit invalid");
}
QJsonObject WorldPoseTracking::toJson() const {
    validate();QJsonObject o{{"schemaVersion",2},{"mapping","world-raw-rigid-v1"},{"taughtPose",ToolValues::encode(taughtPose)},{"sourceTool",sourceTool},{"geometryId",geometryId},{"imageSource",imageSource},{"roiIdentity",roiIdentity},{"width",rawSize.width()},{"height",rawSize.height()},{"calibration",calibration.toJson()},{"mmPerSample",mmPerSample}};if(imageGeometry)o["imageGeometry"]=imageGeometry->toJson();if(invalidated)o["invalidated"]=true;return o;
}
WorldPoseTracking WorldPoseTracking::fromJson(const QJsonObject& o){
    require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=160*1024,"World tracking JSON limit");
    QStringList keys{"schemaVersion","mapping","taughtPose","sourceTool","geometryId","imageSource","roiIdentity","width","height","calibration","mmPerSample"};
    if(o.contains("invalidated"))keys.append("invalidated");
    if(o.contains("imageGeometry"))keys.append("imageGeometry");
    require(o.size()==keys.size(),"World tracking fields missing/extra");for(auto i=o.begin();i!=o.end();++i)require(keys.contains(i.key()),"World tracking unknown field");
    require(o["schemaVersion"]==2 && o["mapping"]=="world-raw-rigid-v1" && o["taughtPose"].isObject() && o["calibration"].isObject(),"World tracking schema invalid");
    for(auto key:{"sourceTool","geometryId","imageSource","roiIdentity"})require(o[key].isString(),"World tracking string field invalid");
    for(auto key:{"width","height"})require(o[key].isDouble() && o[key].toDouble()==o[key].toInt(),"World tracking integer resolution required");
    require(o["mmPerSample"].isDouble(),"World tracking sample unit required");
    auto value=ToolValues::decode(o["taughtPose"].toObject());require(std::holds_alternative<Pose2D>(value),"World tracking teaching requires Pose2D");
    WorldPoseTracking r{std::get<Pose2D>(value),o["sourceTool"].toString(),o["geometryId"].toString(),o["imageSource"].toString(),o["roiIdentity"].toString(),{o["width"].toInt(),o["height"].toInt()},CalibrationDependency::fromJson(o["calibration"].toObject()),o["mmPerSample"].toDouble()};if(o.contains("imageGeometry")){require(o["imageGeometry"].isObject(),"World tracking geometry object required");r.imageGeometry=ImageGeometry::fromJson(o["imageGeometry"].toObject());}if(o.contains("invalidated")){require(o["invalidated"].isBool() && o["invalidated"].toBool(),"Invalid tracking tombstone");r.invalidated=true;}r.validate();return r;
}
bool isWorldPoseTracking(const ToolDefinition& t){return t.parameters["poseTracking"].toObject()["schemaVersion"]==2;}
WorldPoseTracking worldPoseTracking(const ToolDefinition& t){
    require(t.kind=="edge_point" || t.kind=="edge_pair" || t.kind=="line_fit" || t.kind=="circle_fit","World tracking consumer unsupported");
    const auto b=WorldPoseTracking::fromJson(t.parameters["poseTracking"].toObject());
    require(b.roiIdentity==scanRoiIdentity(t),"World ROI changed; reteach required");
    const auto roi=ToolRoi::fromJson(t.parameters["scanRoi"].toObject());
    require(roi.frame==b.calibration.target,"World tracking nominal ROI plane mismatch");
    require(t.kind=="edge_point" || t.parameters["mode"]=="calibrated","World tracking requires calibrated mode");
    require((t.kind=="edge_point" || t.kind=="edge_pair") ? std::holds_alternative<ScanSegment>(roi.geometry):t.kind=="line_fit" ? std::holds_alternative<RotatedRectangle>(roi.geometry):(std::holds_alternative<Annulus>(roi.geometry) || std::holds_alternative<ArcRoi>(roi.geometry)),"World tracking nominal ROI type mismatch");
    if(t.kind=="circle_fit")require(t.parameters["unitsPerSample"]==b.mmPerSample,"World tracking radial sample unit changed; reteach required");
    return b;
}
QString trackingSourceTool(const ToolDefinition& t){return isWorldPoseTracking(t) ? worldPoseTracking(t).sourceTool:poseTrackingBinding(t).sourceTool;}
bool supportsPoseTracking(const QString& k){return k=="edge_point" || k=="edge_pair" || k=="line_fit" || k=="circle_fit" || k=="blob" || k=="code_read";}
ToolRoi nominalTrackingRoi(const ToolDefinition& t,const CoordinateFrame& frame){
    if(t.kind!="blob" && t.kind!="code_read")return ToolRoi::fromJson(t.parameters["scanRoi"].toObject());
    const auto o=t.parameters["searchRoi"].toObject();
    require(o.size()==4,"Pose tracking requires an explicit search rectangle");
    const double x=o["x"].toDouble(),y=o["y"].toDouble(),w=o["width"].toDouble()-1,h=o["height"].toDouble()-1;
    require(w>0 && h>0,"Pose tracking rectangle must span at least two pixel centres per axis");
    ToolRoi roi{frame,RotatedRectangle{{x+w/2,y+h/2},{w,h},0}};roi.validate();return roi;
}
QString scanRoiIdentity(const ToolDefinition& t){
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(t.parameters[t.kind=="blob" || t.kind=="code_read" ? "searchRoi":"scanRoi"].toObject()).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
PoseRoi poseTrackingBinding(const ToolDefinition& t){
    require(supportsPoseTracking(t.kind),"Tool does not support pose tracking");
    require(t.parameters["poseTracking"].isObject(),"Pose tracking must be an object");
    const auto o=t.parameters["poseTracking"].toObject();
    require(o.size()==2 && o["binding"].isObject() && o["roiIdentity"].isString(),"Invalid pose tracking envelope");
    require(o["roiIdentity"].toString()==scanRoiIdentity(t),"Scan ROI changed; pose tracking must be retaught");
    require(!t.parameters.contains("mode") || t.parameters["mode"]=="pixel","Raw tracking requires pixel mode; use explicit world teaching");
    auto binding=PoseRoi::fromJson(o["binding"].toObject());
    const auto roi=nominalTrackingRoi(t,binding.taughtPose.frame);
    require(!binding.calibration && roi.frame==binding.taughtPose.frame && roi.geometry.index()==binding.local.geometry.index(),"Tracking ROI plane/type mismatch");
    require(roi.inside(binding.rawSize),"Tracking nominal ROI outside teaching image");
    const auto expected=PoseRoi::teach(roi,binding.taughtPose,binding.local.frame,binding.sourceTool,binding.geometryId,binding.rawSize,binding.imageSource,binding.imageGeometry);
    require(expected.local.toJson()==binding.local.toJson(),"Tracking local geometry differs from nominal teaching ROI");
    return binding;
}
}
