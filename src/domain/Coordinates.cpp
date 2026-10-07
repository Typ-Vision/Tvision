#include "Coordinates.hpp"
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <QSet>
namespace visionflow {
namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void finite(const QPointF& p) {require(std::isfinite(p.x()) && std::isfinite(p.y()),"Koordinat sonlu olmalı.");}
double angle(double radians) {
    require(std::isfinite(radians),"Açı sonlu olmalı.");return std::remainder(radians,2*std::numbers::pi);
}
QPointF rotate(const QPointF& p,double radians) {
    const auto a=angle(radians),c=std::cos(a),s=std::sin(a);
    const QPointF result(c*p.x()-s*p.y(),s*p.x()+c*p.y());finite(result);return result;
}
void same(const CoordinateFrame& a,const CoordinateFrame& b) {
    a.validate();b.validate();require(a==b,"Koordinat frame/birim/eksen uyumsuz.");
}
void validateCorners(const QVector<QPointF>& corners) {
    require(corners.size()==4,"Tracking ROI dört sıralı köşe gerektirir.");
    for(const auto& p:corners)finite(p);
    double sign=0;
    for(int i=0;i<4;++i) {
        const auto a=corners[(i+1)%4]-corners[i],b=corners[(i+2)%4]-corners[(i+1)%4];
        const auto cross=a.x()*b.y()-a.y()*b.x();
        require(std::isfinite(cross) && cross!=0 && (sign==0 || (cross>0)==(sign>0)),"Tracking ROI dejenere veya köşe sırası geçersiz.");sign=cross;
    }
}
void keys(const QJsonObject& o,const QSet<QString>& expected) {
    require(o.size()==expected.size(),"Koordinat/kalibrasyon kayıt alanları eksik.");
    for(auto it=o.begin();it!=o.end();++it)require(expected.contains(it.key()),"Koordinat/kalibrasyon kayıt alanı bilinmiyor.");
}
QString string(const QJsonObject& o,const QString& key) {
    require(o[key].isString(),"Koordinat/kalibrasyon kayıt metni eksik.");return o[key].toString();
}
int integer(const QJsonObject& o,const QString& key) {
    const auto v=o[key];require(v.isDouble() && v.toDouble()==v.toInt() && v.toInt()>0,"Koordinat/kalibrasyon kayıt tamsayısı geçersiz.");return v.toInt();
}
QJsonObject frameJson(const CoordinateFrame& f) {
    f.validate();return {{"id",f.id},{"unit",f.unit==LengthUnit::Pixel ? "px" : "mm"},{"axes",f.axes==AxisConvention::XRightYDown ? "x-right-y-down" : "x-right-y-up"}};
}
CoordinateFrame readFrame(const QJsonValue& v) {
    require(v.isObject(),"Koordinat frame kaydı nesne olmalı.");const auto o=v.toObject();keys(o,{"id","unit","axes"});
    const auto u=string(o,"unit"),a=string(o,"axes");
    require(u=="px" || u=="mm","Koordinat kayıt birimi bilinmiyor.");
    require(a=="x-right-y-down" || a=="x-right-y-up","Koordinat kayıt ekseni bilinmiyor.");
    CoordinateFrame f{string(o,"id"),u=="px" ? LengthUnit::Pixel : LengthUnit::Millimetre,a=="x-right-y-down" ? AxisConvention::XRightYDown : AxisConvention::XRightYUp};f.validate();return f;
}
}
void CoordinateFrame::validate() const {
    require(!id.trimmed().isEmpty() && id==id.trimmed(),"Koordinat sistemi ID eksik/geçersiz.");
    require(unit==LengthUnit::Pixel || unit==LengthUnit::Millimetre,"Koordinat birimi geçersiz.");
    require(axes==AxisConvention::XRightYDown || axes==AxisConvention::XRightYUp,"Eksen konvansiyonu geçersiz.");
}
void Pose2D::validate() const {frame.validate();finite(position);angle(angleRadians);}
void Transform2D::validate() const {
    source.validate();target.validate();finite(translation);angle(angleRadians);
    require(source.unit==target.unit && source.axes==target.axes,"Rigid dönüşüm birim/eksen dönüşümü yapamaz.");
}
Point2D Transform2D::map(const Point2D& p) const {
    validate();same(source,p.frame);finite(p.position);
    const auto result=rotate(p.position,angleRadians)+translation;finite(result);return {result,target};
}
Pose2D Transform2D::map(const Pose2D& p) const {
    p.validate();const auto result=map(Point2D{p.position,p.frame});
    return {result.position,angle(angle(angleRadians)+angle(p.angleRadians)),target};
}
Transform2D Transform2D::inverse() const {
    validate();return {target,source,rotate(-translation,-angle(angleRadians)),-angle(angleRadians)};
}
Transform2D Transform2D::then(const Transform2D& next) const {
    validate();next.validate();same(target,next.source);
    const auto offset=next.map(Point2D{translation,target}).position;
    return {source,next.target,offset,angle(angle(angleRadians)+angle(next.angleRadians))};
}
Transform2D poseTransform(const Pose2D& pose,const CoordinateFrame& local) {
    pose.validate();require(local.id!=pose.frame.id,"Lokal ve hedef koordinat sistemi ayrı olmalı.");
    Transform2D result{local,pose.frame,pose.position,pose.angleRadians};result.validate();return result;
}
TrackingRoi TrackingRoi::teach(const QVector<Point2D>& corners,const Pose2D& reference,const CoordinateFrame& local) {
    require(corners.size()==4,"Tracking ROI dört sıralı köşe gerektirir.");
    const auto inverse=poseTransform(reference,local).inverse();TrackingRoi result{local,reference.frame,{}};
    for(const auto& corner:corners)result.localCorners.append(inverse.map(corner).position);
    validateCorners(result.localCorners);
    return result;
}
QVector<Point2D> TrackingRoi::resolve(const Pose2D& reference) const {
    same(referenceFrame,reference.frame);
    const auto transform=poseTransform(reference,localFrame);
    validateCorners(localCorners);
    QVector<Point2D> result;for(const auto& p:localCorners)result.append(transform.map(Point2D{p,localFrame}));return result;
}
QRect translationTrackingRoi(const QRect& taught,const Point2D& origin,const Point2D& current,const QSize& imageSize) {
    same(origin.frame,current.frame);finite(origin.position);finite(current.position);
    require(origin.frame.unit==LengthUnit::Pixel && origin.frame.axes==AxisConvention::XRightYDown,"Image ROI px/Y-down gerektirir.");
    require(!taught.isEmpty() && imageSize.width()>0 && imageSize.height()>0,"Tracking ROI/boyut eksik.");
    const auto local=CoordinateFrame{origin.frame.id+"/tracking-local",LengthUnit::Pixel,AxisConvention::XRightYDown};
    const auto delta=poseTransform({origin.position,0,origin.frame},local).inverse()
        .then(poseTransform({current.position,0,current.frame},local)).translation;
    const auto x=double(taught.x())+std::round(delta.x()),y=double(taught.y())+std::round(delta.y());
    require(std::isfinite(x) && std::isfinite(y) && x>=0 && y>=0 && x+taught.width()<=imageSize.width() && y+taught.height()<=imageSize.height(),"ROI görüntü sınırı dışında.");
    return {int(x),int(y),taught.width(),taught.height()};
}
void CalibrationDependency::validate() const {
    source.validate();target.validate();
    require(!calibrationId.trimmed().isEmpty() && version>0 && !contentId.trimmed().isEmpty() && !cameraId.trimmed().isEmpty() && !lensId.trimmed().isEmpty() && resolution.width()>0 && resolution.height()>0 && !workingPlaneId.trimmed().isEmpty(),"Kalibrasyon bağımlılığı eksik/geçersiz.");
}
void CalibrationDependency::requireCurrent(const CalibrationDependency& current) const {
    validate();current.validate();require(*this==current,"Kalibrasyon kimliği/sürümü/kurulum bağımlılığı değişti.");
}
QJsonObject CalibrationDependency::toJson() const {
    validate();return {{"schemaVersion",1},{"calibrationId",calibrationId},{"version",version},{"contentId",contentId},{"cameraId",cameraId},{"lensId",lensId},{"width",resolution.width()},{"height",resolution.height()},{"workingPlaneId",workingPlaneId},{"source",frameJson(source)},{"target",frameJson(target)}};
}
CalibrationDependency CalibrationDependency::fromJson(const QJsonObject& o) {
    keys(o,{"schemaVersion","calibrationId","version","contentId","cameraId","lensId","width","height","workingPlaneId","source","target"});
    require(integer(o,"schemaVersion")==1,"Desteklenmeyen kalibrasyon bağımlılık şeması.");
    CalibrationDependency result{string(o,"calibrationId"),integer(o,"version"),string(o,"contentId"),string(o,"cameraId"),string(o,"lensId"),{integer(o,"width"),integer(o,"height")},string(o,"workingPlaneId"),readFrame(o["source"]),readFrame(o["target"])};
    result.validate();return result;
}
}
