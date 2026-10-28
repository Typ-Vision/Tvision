#include "ReferenceBinding.hpp"
#include "ToolValues.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QSet>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void keys(const QJsonObject& o,const QSet<QString>& expected){
    require(o.size()==expected.size(),"Reference fields missing/extra");
    for(auto i=o.begin();i!=o.end();++i)require(expected.contains(i.key()),"Reference unknown field");
}
void identity(const QString& s,int limit){require(!s.isEmpty() && s==s.trimmed() && s.size()<=limit,"Reference identity invalid");}
double number(const QJsonObject& o,const char* k){require(o[k].isDouble() && std::isfinite(o[k].toDouble()),"Reference number must be finite");return o[k].toDouble();}
void anchor(const ReferenceAnchor& a,const CoordinateFrame& frame,const QSize& size){
    std::visit([&](const auto& v){
        v.frame.validate();require(v.frame==frame,"Reference anchor frame mismatch");
        require(std::isfinite(v.position.x()) && std::isfinite(v.position.y()) && v.position.x()>=0 && v.position.y()>=0 && v.position.x()<=size.width()-1 && v.position.y()<=size.height()-1,"Reference anchor outside image");
        if constexpr(std::is_same_v<std::decay_t<decltype(v)>,Pose2D>)v.validate();
    },a);
}
}
void ReferenceBinding::validate() const {
    require(mode==ReferenceMode::Translation || mode==ReferenceMode::Rigid,"Reference mode invalid");
    require((mode==ReferenceMode::Translation)==std::holds_alternative<Point2D>(taught),"Reference mode/teaching type mismatch");
    identity(sourceTool,128);identity(sourcePort,128);identity(imageSource,256);identity(nominal.frame.id,256);
    require(geometryId.size()==64 && QByteArray::fromHex(geometryId.toLatin1()).toHex()==geometryId.toLatin1(),"Reference source geometry SHA256 invalid");
    require(rawSize.width()>0 && rawSize.height()>0 && qint64(rawSize.width())*rawSize.height()<=16*1024*1024,"Reference resolution invalid");
    if(imageGeometry)require(imageGeometry->sourceId()==imageSource && imageGeometry->cropDepth()>0 && (world ? imageGeometry->rootSize():imageGeometry->rasterSize())==rawSize,"Reference image geometry mismatch");
    if(!world && nominal.frame.id.startsWith("roi-crop/"))require(imageGeometry && imageGeometry->coordinateSystemId()==nominal.frame.id,"Reference child requires matching scope");
    nominal.validate();
    if(world){require(calibration.has_value(),"World reference calibration missing");require(nominal.frame==calibration->target,"World reference nominal plane mismatch");require(std::isfinite(mmPerSample) && mmPerSample>=1e-9 && mmPerSample<=1e9,"World sample unit invalid");}
    else require(nominal.frame.unit==LengthUnit::Pixel && nominal.frame.axes==AxisConvention::XRightYDown && nominal.inside(rawSize),"Reference requires bounded raw nominal ROI");
    const auto raw=world ? calibration->source:nominal.frame;
    require(raw.unit==LengthUnit::Pixel && raw.axes==AxisConvention::XRightYDown,"Reference raw frame invalid");
    anchor(taught,raw,rawSize);
    require(std::isfinite(offsetPx.x()) && std::isfinite(offsetPx.y()) && std::abs(offsetPx.x())<=1e8 && std::abs(offsetPx.y())<=1e8 && std::isfinite(angleOffsetRadians),"Reference offset invalid/oversized");
    require(mode==ReferenceMode::Rigid || angleOffsetRadians==0,"Point reference cannot have angle offset");
    if(calibration){calibration->validate();require(calibration->source==raw && calibration->resolution==rawSize,"Reference calibration frame/resolution mismatch");}
}
QString ReferenceBinding::roiIdentity() const {
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(nominal.toJson()).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
Transform2D ReferenceBinding::rawTransform(const ReferenceAnchor& current,double scale,const QSize& size) const {
    validate();require(size==rawSize,"Reference resolution changed; reteach required");
    require(std::isfinite(scale) && scale==1,"Reference requires scale exactly one");
    const auto raw=world ? calibration->source:nominal.frame;
    require(current.index()==taught.index(),"Reference current type mismatch");anchor(current,raw,size);
    if(mode==ReferenceMode::Translation){
        const auto delta=std::get<Point2D>(current).position-std::get<Point2D>(taught).position+offsetPx;
        Transform2D t{raw,raw,delta,0};t.validate();return t;
    }
    const CoordinateFrame local{raw.id+"/reference-local"};
    const Transform2D correction{local,local,offsetPx,std::remainder(angleOffsetRadians,2*std::numbers::pi)};
    return poseTransform(std::get<Pose2D>(taught),local).inverse()
        .then(correction).then(poseTransform(std::get<Pose2D>(current),local));
}
ToolRoi ReferenceBinding::resolve(const ReferenceAnchor& current,double scale,const QSize& size) const {
    require(!world,"World reference requires per-tap mapper");
    auto roi=mapRigidRoi(nominal,rawTransform(current,scale,size));
    require(roi.inside(size),"Reference ROI outside image; no clipping/fallback");return roi;
}
QJsonObject ReferenceBinding::toJson() const {
    validate();QJsonObject teaching;
    if(mode==ReferenceMode::Translation){const auto& p=std::get<Point2D>(taught);teaching={{"x",p.position.x()},{"y",p.position.y()}};}
    else teaching=ToolValues::encode(std::get<Pose2D>(taught));
    QJsonObject o{{"schemaVersion",world ? 2:1},{"mapping",world ? "world-reference-v1":"raw-reference-v1"},{"mode",mode==ReferenceMode::Translation ? "translation":"rigid"},
        {"sourceTool",sourceTool},{"sourcePort",sourcePort},{"geometryId",geometryId},{"imageSource",imageSource},
        {"nominal",nominal.toJson()},{"roiIdentity",roiIdentity()},{"taught",teaching},{"width",rawSize.width()},{"height",rawSize.height()},
        {"offset",QJsonObject{{"x",offsetPx.x()},{"y",offsetPx.y()},{"angleRadians",angleOffsetRadians},{"unit","px"},{"axes",mode==ReferenceMode::Translation ? "image":"source-local"}}},
        {"calibration",calibration ? QJsonValue(calibration->toJson()):QJsonValue(QJsonValue::Null)}};
    if(world)o["mmPerSample"]=mmPerSample;if(imageGeometry)o["imageGeometry"]=imageGeometry->toJson();if(invalidated)o["invalidated"]=true;return o;
}
ReferenceBinding ReferenceBinding::fromJson(const QJsonObject& o){
    require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=160*1024,"Reference JSON limit");
    const bool world=o["schemaVersion"]==2;
    QSet<QString> expected{"schemaVersion","mapping","mode","sourceTool","sourcePort","geometryId","imageSource","nominal","roiIdentity","taught","width","height","offset","calibration"};if(world)expected.insert("mmPerSample");if(o.contains("imageGeometry"))expected.insert("imageGeometry");if(o.contains("invalidated"))expected.insert("invalidated");keys(o,expected);
    require((world ? o["mapping"]=="world-reference-v1":o["schemaVersion"]==1 && o["mapping"]=="raw-reference-v1") && (o["mode"]=="translation" || o["mode"]=="rigid"),"Reference schema/mode invalid");
    for(auto k:{"sourceTool","sourcePort","geometryId","imageSource","roiIdentity"})require(o[k].isString(),"Reference string required");
    for(auto k:{"nominal","taught","offset"})require(o[k].isObject(),"Reference object required");
    require(o["calibration"].isNull() || o["calibration"].isObject(),"Reference calibration invalid");
    ReferenceBinding b;if(o.contains("invalidated")){require(o["invalidated"].isBool() && o["invalidated"].toBool(),"Invalid reference tombstone");b.invalidated=true;}if(o.contains("imageGeometry")){require(o["imageGeometry"].isObject(),"Reference geometry object required");b.imageGeometry=ImageGeometry::fromJson(o["imageGeometry"].toObject());}b.mode=o["mode"]=="translation" ? ReferenceMode::Translation:ReferenceMode::Rigid;
    b.world=world;if(world)b.mmPerSample=number(o,"mmPerSample");
    if(o["calibration"].isObject())b.calibration=CalibrationDependency::fromJson(o["calibration"].toObject());
    b.sourceTool=o["sourceTool"].toString();b.sourcePort=o["sourcePort"].toString();b.geometryId=o["geometryId"].toString();b.imageSource=o["imageSource"].toString();
    b.nominal=ToolRoi::fromJson(o["nominal"].toObject());require(b.roiIdentity()==o["roiIdentity"],"Reference nominal identity mismatch");
    for(auto k:{"width","height"})require(number(o,k)==o[k].toInt() && o[k].toInt()>0,"Reference integer resolution required");
    b.rawSize={o["width"].toInt(),o["height"].toInt()};
    const auto t=o["taught"].toObject();
    if(b.mode==ReferenceMode::Translation){keys(t,{"x","y"});require(!world || b.calibration.has_value(),"World reference calibration missing");b.taught=Point2D{{number(t,"x"),number(t,"y")},world ? b.calibration->source:b.nominal.frame};}
    else {auto v=ToolValues::decode(t);require(std::holds_alternative<Pose2D>(v),"Rigid reference requires taught pose");b.taught=std::get<Pose2D>(v);}
    const auto d=o["offset"].toObject();keys(d,{"x","y","angleRadians","unit","axes"});
    require(d["unit"]=="px" && d["axes"]==(b.mode==ReferenceMode::Translation ? "image":"source-local"),"Reference offset unit/axes mismatch");
    b.offsetPx={number(d,"x"),number(d,"y")};b.angleOffsetRadians=number(d,"angleRadians");
    if(o["calibration"].isObject())b.calibration=CalibrationDependency::fromJson(o["calibration"].toObject());b.validate();return b;
}
}
