#include "domain/SpatialTransform.hpp"
#include "domain/ToolValues.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
void close(double a,double b){check(std::abs(a-b)<1e-10,"known numeric fixture");}
void translation(const SpatialTransform& t,std::array<double,3> expected){for(int i=0;i<3;++i)close(t.translationMm[i],expected[i]);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    FrameRef a{"","robot","cell","00000000-0000-0000-0000-000000000001",1};
    auto b=a,c=a;b.frameUuid="00000000-0000-0000-0000-000000000002";c.frameUuid="00000000-0000-0000-0000-000000000003";
    check(FrameRef::fromJson(a.toJson())==a,"frame roundtrip");
    auto bad=a;bad.revision=0;reject([&]{bad.validate();},"zero revision");bad=a;bad.frameUuid="image";reject([&]{bad.validate();},"frame UUID required");bad=a;bad.robotId.clear();bad.groupId.clear();reject([&]{bad.validate();},"frame owner required");
    for(double v:{0.0,0.1,2.0,1.0+2e-6,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})reject([&]{Rotation3D{v,0,0,0}.normalized();},"bad quaternion norm");
    for(int i=0;i<4;++i){Rotation3D q;auto* component=i==0?&q.w:i==1?&q.x:i==2?&q.y:&q.z;*component=std::numeric_limits<double>::infinity();reject([&]{q.toJson();},"each nonfinite quaternion component rejected");}
    close(Rotation3D{1+0.5e-6,0,0,0}.normalized().w,1);
    const double h=std::sqrt(0.5);Rotation3D rz{h,0,0,h},rx{h,h,0,0},ry{h,0,h,0};
    check(rz.equivalent({-h,0,0,-h}),"q/-q equivalent");check(rz.toJson()==Rotation3D{-h,0,0,-h}.toJson(),"canonical quaternion encoding");check(Rotation3D{0,1,0,0}.toJson()==Rotation3D{0,-1,0,0}.toJson(),"canonical half turn");
    check(!rz.equivalent(rx),"distinct rotation");reject([&]{rz.equivalent(rx,-1);},"invalid comparison tolerance");
    SpatialTransform first{c,b,{10,20,30},rz},second{b,a,{1,2,3},rx};
    auto chain=first.then(second);check(chain.source==c && chain.target==a,"T_A_B * T_B_C direction");translation(chain,{11,-28,23});check(chain.rotation.equivalent({0.5,0.5,-0.5,0.5}),"known X90*Z90 rotation");
    SpatialTransform third{c,b,{2,3,4},ry},fourth{b,a,{5,6,7},rz};auto chain2=third.then(fourth);translation(chain2,{2,8,11});check(chain2.rotation.equivalent({0.5,-0.5,0.5,0.5}),"known Z90*Y90 rotation");
    for(auto t:{first,chain,chain2}){
        auto identity=t.then(t.inverse());translation(identity,{0,0,0});check(identity.rotation.equivalent({}),"right inverse identity");check(identity.source==t.source && identity.target==t.source,"inverse frame direction");
        auto left=t.inverse().then(t);translation(left,{0,0,0});check(left.rotation.equivalent({}),"left inverse identity");
        auto decoded=SpatialTransform::fromJson(t.toJson());translation(decoded,t.translationMm);check(decoded.rotation.equivalent(t.rotation),"transform roundtrip");
        auto tagged=ToolValues::encode(t);check(tagged["type"]=="SpatialTransform","explicit spatial tag");check(std::holds_alternative<SpatialTransform>(ToolValues::decode(tagged)),"typed spatial roundtrip");
    }
    SpatialTransform id{c,c,{},{} };translation(id.then(first),first.translationMm);check(id.then(first).rotation.equivalent(first.rotation),"identity composition");
    for(int i=0;i<5;++i){auto wrong=second;if(i==0)++wrong.source.revision;if(i==1)wrong.source.frameUuid=a.frameUuid;if(i==2)wrong.source.deviceId="foreign";if(i==3)wrong.source.robotId="foreign";if(i==4)wrong.source.groupId="foreign";reject([&]{first.then(wrong);},"exact frame and owner/revision mismatch");}
    auto overflow=first;overflow.translationMm={std::numeric_limits<double>::max(),0,0};auto more=second;more.rotation={};more.translationMm=overflow.translationMm;reject([&]{overflow.then(more);},"compose overflow rejected");
    for(int i=0;i<3;++i){auto t=first;t.translationMm[i]=std::numeric_limits<double>::infinity();reject([&]{t.toJson();},"nonfinite translation encode");}
    auto json=first.toJson();for(auto key:json.keys()){auto missing=json;missing.remove(key);reject([&]{SpatialTransform::fromJson(missing);},"required spatial field");}
    auto wrong=json;wrong["extra"]=true;reject([&]{SpatialTransform::fromJson(wrong);},"unknown spatial field");
    for(const auto& unit:{"m","px","degree"}){wrong=json;wrong["translationUnit"]=unit;reject([&]{SpatialTransform::fromJson(wrong);},"explicit mm only");}
    wrong=json;wrong["convention"]="left-handed-row-vector";reject([&]{SpatialTransform::fromJson(wrong);},"wrong convention");wrong=json;wrong["schemaVersion"]=2;reject([&]{SpatialTransform::fromJson(wrong);},"future schema");
    wrong=json;wrong["translation"]=QJsonArray{1,2,3,4,5,6,0};reject([&]{SpatialTransform::fromJson(wrong);},"HALCON tuple not translation");
    for(const auto& format:{"halcon-pose","euler-xyz","quaternion-xyzw"}){auto r=rz.toJson();r["format"]=format;reject([&]{Rotation3D::fromJson(r);},"no implicit rotation format conversion");}
    auto r=rz.toJson();r["value"]=QJsonArray{1,2,3,4,5,6,0};reject([&]{Rotation3D::fromJson(r);},"7 tuple not quaternion");r=rz.toJson();r["value"]=QJsonArray{0,0,0,0};reject([&]{Rotation3D::fromJson(r);},"zero quaternion decode");
    auto fj=a.toJson();fj["revision"]=1.5;reject([&]{FrameRef::fromJson(fj);},"fractional revision");fj=a.toJson();fj["revision"]=2147483648.0;reject([&]{FrameRef::fromJson(fj);},"revision overflow");
    for(const auto& field:{"source","target","rotation"}){wrong=json;wrong[field]=QJsonValue(QJsonValue::Null);reject([&]{SpatialTransform::fromJson(wrong);},"nested object required");}
    wrong=json;wrong["translation"]=QJsonArray{1,"2",3};reject([&]{SpatialTransform::fromJson(wrong);},"numeric string not Float64");
    r=rz.toJson();r["value"]=QJsonArray{1,QJsonValue(QJsonValue::Null),0,0};reject([&]{Rotation3D::fromJson(r);},"nonfinite JSON null rejected");
    r=rz.toJson();r["extra"]=0;reject([&]{Rotation3D::fromJson(r);},"unknown rotation field");
    fj=a.toJson();fj["extra"]=0;reject([&]{FrameRef::fromJson(fj);},"unknown frame field");
    bad=a;bad.deviceId=QString(257,'x');reject([&]{bad.validate();},"bounded owner identity");
    const QJsonObject legacy{{"schemaVersion",1},{"type","Pose2D"},{"data",QJsonObject{{"position",QJsonObject{{"x",4},{"y",5}}},{"angleRadians",0.5},{"frame",QJsonObject{{"id","source/image"},{"unit","px"},{"axes","down"}}}}}};
    auto old=ToolValues::decode(legacy);check(std::holds_alternative<Pose2D>(old),"legacy Pose2D never spatial");check(ToolValues::encode(old)==legacy,"legacy Pose2D exact JSON fixture");
    auto tagged=ToolValues::encode(first);tagged["type"]="Pose2D";reject([&]{ToolValues::decode(tagged);},"spatial cannot be relabeled Pose2D");
    tagged=ToolValues::encode(first);tagged["data"]=QJsonArray{1,2,3,4,5,6,0};reject([&]{ToolValues::decode(tagged);},"untagged HALCON seven tuple rejected");
    QTextStream(stdout)<<checks<<" checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<"\n";return 1;}}
