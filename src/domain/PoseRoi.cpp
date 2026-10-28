#include "PoseRoi.hpp"
#include "ToolValues.hpp"
#include <QJsonDocument>
#include <QSet>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <type_traits>
namespace visionflow {
namespace {
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
}
ToolRoi mapRigidRoi(const ToolRoi& roi,const Transform2D& t){
    roi.validate();require(roi.frame==t.source,"Tracking ROI source frame mismatch");
    auto point=[&](QPointF p){return t.map(Point2D{p,roi.frame}).position;};
    const auto degrees=t.angleRadians*180/std::numbers::pi;
    ToolRoi result{t.target,std::visit([&](auto g)->RoiGeometry{
        using T=std::decay_t<decltype(g)>;
        if constexpr(std::is_same_v<T,ScanSegment>){g.start=point(g.start);g.end=point(g.end);}
        else if constexpr(std::is_same_v<T,RotatedRectangle>){g.centre=point(g.centre);g.angleDegrees=std::remainder(g.angleDegrees+degrees,360.);}
        else if constexpr(std::is_same_v<T,Annulus>)g.centre=point(g.centre);
        else if constexpr(std::is_same_v<T,ArcRoi>){g.ring.centre=point(g.ring.centre);g.startDegrees=std::remainder(g.startDegrees+degrees,360.);}
        else for(auto& p:g.vertices)p=point(p);
        return g;
    },roi.geometry)};result.validate();return result;
}
void PoseRoi::validate() const {
    if(imageGeometry)require(imageGeometry->cropDepth()>0 && imageGeometry->sourceId()==imageSource && imageGeometry->rasterSize()==rawSize && imageGeometry->coordinateSystemId()==taughtPose.frame.id,"Tracking crop scope differs");
    else require(!taughtPose.frame.id.startsWith("roi-crop/"),"Tracking child requires crop scope");
    local.validate();taughtPose.validate();
    require(local.frame.unit==LengthUnit::Pixel && taughtPose.frame.unit==LengthUnit::Pixel &&
            taughtPose.frame.axes==AxisConvention::XRightYDown,"Tracking requires raw pixel pose/local ROI");
    poseTransform(taughtPose,local.frame).validate();
    require(!sourceTool.trimmed().isEmpty() && sourceTool.size()<=128,"Tracking source tool missing/oversized");
    require(!imageSource.trimmed().isEmpty() && imageSource.size()<=256,"Tracking acquisition source missing/oversized");
    require(geometryId.size()==64 && QByteArray::fromHex(geometryId.toLatin1()).toHex()==geometryId.toLatin1(),"Tracking geometry SHA256 invalid");
    require(local.frame.id.size()<=256 && taughtPose.frame.id.size()<=256,"Tracking frame ID oversized");
    require(rawSize.width()>0 && rawSize.height()>0 && qint64(rawSize.width())*rawSize.height()<=16*1024*1024,"Tracking raw resolution invalid");
    require(taughtPose.position.x()>=0 && taughtPose.position.y()>=0 && taughtPose.position.x()<=rawSize.width()-1 && taughtPose.position.y()<=rawSize.height()-1,"Tracking teaching anchor outside image");
    if(calibration){calibration->validate();require(calibration->source==CoordinateFrame{imageGeometry ? imageGeometry->rootCoordinateSystemId():taughtPose.frame.id} && calibration->resolution==(imageGeometry ? imageGeometry->rootSize():rawSize),"Tracking calibration raw frame/resolution mismatch");}
}
PoseRoi PoseRoi::teach(const ToolRoi& raw,const Pose2D& pose,const CoordinateFrame& localFrame,const QString& source,const QString& identity,const QSize& size,const QString& acquisitionSource,const std::optional<ImageGeometry>& scope){
    require(raw.frame==pose.frame,"Tracking teaching ROI/pose frame mismatch");
    require(raw.inside(size),"Tracking teaching footprint outside image");
    PoseRoi r{mapRigidRoi(raw,poseTransform(pose,localFrame).inverse()),pose,source,identity,size,acquisitionSource,{},scope};r.validate();return r;
}
ToolRoi PoseRoi::resolve(const Pose2D& pose,double scale,const QSize& image) const {
    validate();pose.validate();require(pose.frame==taughtPose.frame,"Tracking current pose frame mismatch");
    require(image==rawSize,"Tracking resolution changed; reteach required");
    require(std::isfinite(scale) && scale==1.0,"Rigid tracking requires scale exactly 1");
    auto raw=mapRigidRoi(local,poseTransform(pose,local.frame));require(raw.inside(image),"Tracked ROI outside raw image; no clipping/fallback");return raw;
}
QJsonObject PoseRoi::toJson() const {
    validate();QJsonObject o{{"schemaVersion",1},{"mapping","raw-rigid-v1"},{"local",local.toJson()},
        {"taughtPose",ToolValues::encode(taughtPose)},{"sourceTool",sourceTool},{"imageSource",imageSource},{"geometryId",geometryId},{"width",rawSize.width()},{"height",rawSize.height()},
        {"calibration",calibration ? QJsonValue(calibration->toJson()):QJsonValue(QJsonValue::Null)}};if(imageGeometry)o["imageGeometry"]=imageGeometry->toJson();if(invalidated)o["invalidated"]=true;return o;
}
PoseRoi PoseRoi::fromJson(const QJsonObject& o){
    require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=160*1024,"Tracking JSON limit");
    QSet<QString> keys{"schemaVersion","mapping","local","taughtPose","sourceTool","imageSource","geometryId","calibration","width","height"};
    if(o.contains("invalidated"))keys.insert("invalidated");
    if(o.contains("imageGeometry"))keys.insert("imageGeometry");
    require(o.size()==keys.size(),"Tracking fields missing/extra");for(auto i=o.begin();i!=o.end();++i)require(keys.contains(i.key()),"Tracking unknown field");
    require(o["schemaVersion"]==1 && o["mapping"]=="raw-rigid-v1" && o["local"].isObject() && o["taughtPose"].isObject() &&
            o["sourceTool"].isString() && o["imageSource"].isString() && o["geometryId"].isString() && (o["calibration"].isNull() || o["calibration"].isObject()),"Tracking schema invalid");
    const auto value=ToolValues::decode(o["taughtPose"].toObject());require(std::holds_alternative<Pose2D>(value),"Tracking teaching value is not Pose2D");
    for(auto key:{"width","height"})require(o[key].isDouble() && o[key].toDouble()==o[key].toInt(),"Tracking resolution must be integer");
    PoseRoi r{ToolRoi::fromJson(o["local"].toObject()),std::get<Pose2D>(value),o["sourceTool"].toString(),o["geometryId"].toString(),{o["width"].toInt(),o["height"].toInt()},o["imageSource"].toString(),{}};
    if(o.contains("imageGeometry")){require(o["imageGeometry"].isObject(),"Tracking geometry object required");r.imageGeometry=ImageGeometry::fromJson(o["imageGeometry"].toObject());}
    if(o["calibration"].isObject())r.calibration=CalibrationDependency::fromJson(o["calibration"].toObject());if(o.contains("invalidated")){require(o["invalidated"].isBool() && o["invalidated"].toBool(),"Invalid tracking tombstone");r.invalidated=true;}r.validate();return r;
}
}
