#include "SpatialTransform.hpp"
#include <QJsonArray>
#include <QSet>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void keys(const QJsonObject& o,std::initializer_list<QString> names){QSet<QString> allowed(names.begin(),names.end());require(o.size()==allowed.size(),"Spatial fields missing/extra");for(auto i=o.begin();i!=o.end();++i)require(allowed.contains(i.key()),"Unknown spatial field");}
void identifier(const QString& s){require(s.toUtf8().size()<=256 && s==s.trimmed(),"Invalid spatial identity");for(auto c:s)require(c.isPrint() && !c.isSurrogate(),"Invalid spatial identity character");}
QString text(const QJsonObject& o,const char* key){require(o[key].isString(),"Spatial identity must be string");auto s=o[key].toString();identifier(s);return s;}
double number(QJsonValue v){require(v.isDouble() && std::isfinite(v.toDouble()),"Spatial number must be finite Float64");return v.toDouble();}
QJsonObject object(QJsonValue v){require(v.isObject(),"Spatial object required");return v.toObject();}
Rotation3D multiply(Rotation3D a,Rotation3D b){return Rotation3D{a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w}.normalized();}
std::array<double,3> rotate(Rotation3D q,std::array<double,3> p){
    const auto tx=2*(q.y*p[2]-q.z*p[1]),ty=2*(q.z*p[0]-q.x*p[2]),tz=2*(q.x*p[1]-q.y*p[0]);
    return {p[0]+q.w*tx+q.y*tz-q.z*ty,p[1]+q.w*ty+q.z*tx-q.x*tz,p[2]+q.w*tz+q.x*ty-q.y*tx};
}
}
void FrameRef::validate() const {
    for(const auto& s:{deviceId,robotId,groupId,frameUuid})identifier(s);
    require(!deviceId.isEmpty() || !robotId.isEmpty() || !groupId.isEmpty(),"Spatial frame owner required");
    const QUuid uuid(frameUuid);require(!uuid.isNull() && uuid.toString(QUuid::WithoutBraces)==frameUuid && revision>0,"Canonical frame UUID and positive revision required");
}
QJsonObject FrameRef::toJson() const {validate();return {{"deviceId",deviceId},{"robotId",robotId},{"groupId",groupId},{"frameUuid",frameUuid},{"revision",revision}};}
FrameRef FrameRef::fromJson(const QJsonObject& o){keys(o,{"deviceId","robotId","groupId","frameUuid","revision"});auto rev=number(o["revision"]);require(rev>=1 && rev<=2147483647 && std::floor(rev)==rev,"Invalid spatial frame revision");FrameRef f{text(o,"deviceId"),text(o,"robotId"),text(o,"groupId"),text(o,"frameUuid"),int(rev)};f.validate();return f;}
Rotation3D Rotation3D::normalized() const {
    require(std::isfinite(w)&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z),"Nonfinite quaternion");
    auto norm=std::hypot(std::hypot(w,x),std::hypot(y,z));require(std::isfinite(norm) && norm>0 && std::abs(norm-1)<=normTolerance,"Quaternion norm outside tolerance");
    Rotation3D q{w/norm,x/norm,y/norm,z/norm};
    // Canonical hemisphere including exact half-turns: q and -q encode identically.
    for(auto c:{q.w,q.x,q.y,q.z})if(c!=0){if(c<0){q.w=-q.w;q.x=-q.x;q.y=-q.y;q.z=-q.z;}break;}
    return q;
}
bool Rotation3D::equivalent(const Rotation3D& other,double tolerance) const {require(std::isfinite(tolerance)&&tolerance>=0,"Invalid rotation tolerance");auto a=normalized(),b=other.normalized();return std::hypot(std::hypot(a.w-b.w,a.x-b.x),std::hypot(a.y-b.y,a.z-b.z))<=tolerance || std::hypot(std::hypot(a.w+b.w,a.x+b.x),std::hypot(a.y+b.y,a.z+b.z))<=tolerance;}
QJsonObject Rotation3D::toJson() const {auto q=normalized();return {{"format","quaternion-wxyz"},{"value",QJsonArray{q.w,q.x,q.y,q.z}}};}
Rotation3D Rotation3D::fromJson(const QJsonObject& o){keys(o,{"format","value"});require(o["format"]=="quaternion-wxyz" && o["value"].isArray(),"Explicit wxyz quaternion required");auto a=o["value"].toArray();require(a.size()==4,"Quaternion requires four components");return Rotation3D{number(a[0]),number(a[1]),number(a[2]),number(a[3])}.normalized();}
void SpatialTransform::validate() const {source.validate();target.validate();rotation.normalized();for(auto t:translationMm)require(std::isfinite(t),"Nonfinite translation");}
SpatialTransform SpatialTransform::inverse() const {validate();auto q=rotation.normalized();q={q.w,-q.x,-q.y,-q.z};auto t=rotate(q,translationMm);for(auto& v:t)v=-v;SpatialTransform result{target,source,t,q.normalized()};result.validate();return result;}
SpatialTransform SpatialTransform::then(const SpatialTransform& next) const {validate();next.validate();require(target==next.source,"Spatial compose frame/revision mismatch");auto t=rotate(next.rotation.normalized(),translationMm);for(int i=0;i<3;++i)t[i]+=next.translationMm[i];SpatialTransform result{source,next.target,t,multiply(next.rotation.normalized(),rotation.normalized())};result.validate();return result;}
QJsonObject SpatialTransform::toJson() const {validate();return {{"schemaVersion",1},{"convention","right-handed-column-vector"},{"source",source.toJson()},{"target",target.toJson()},{"translationUnit","mm"},{"translation",QJsonArray{translationMm[0],translationMm[1],translationMm[2]}},{"rotation",rotation.toJson()}};}
SpatialTransform SpatialTransform::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","convention","source","target","translationUnit","translation","rotation"});require(o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1 && o["convention"]=="right-handed-column-vector" && o["translationUnit"]=="mm" && o["translation"].isArray(),"Unsupported spatial schema/convention/unit");auto a=o["translation"].toArray();require(a.size()==3,"Translation requires three components");SpatialTransform t{FrameRef::fromJson(object(o["source"])),FrameRef::fromJson(object(o["target"])),{number(a[0]),number(a[1]),number(a[2])},Rotation3D::fromJson(object(o["rotation"]))};t.validate();return t;}
}
