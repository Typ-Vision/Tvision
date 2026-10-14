#include "ToolRoi.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <type_traits>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool finite(double n){return std::isfinite(n) && std::abs(n)<=1e8;}
void point(QPointF p){require(finite(p.x()) && finite(p.y()),"ROI nonfinite/oversized point");}
QPointF radial(QPointF c,double radius,double angle){const auto a=angle*std::numbers::pi/180;return c+QPointF(radius*std::cos(a),radius*std::sin(a));}
QRectF extent(const QVector<QPointF>& points){double l=points.first().x(),r=l,t=points.first().y(),b=t;for(auto p:points){l=std::min(l,p.x());r=std::max(r,p.x());t=std::min(t,p.y());b=std::max(b,p.y());}return QRectF(QPointF(l,t),QPointF(r,b));}
QVector<QPointF> corners(const RotatedRectangle& r){QVector<QPointF> points;const auto a=r.angleDegrees*std::numbers::pi/180;for(auto p:QVector<QPointF>{{-r.size.width()/2,-r.size.height()/2},{r.size.width()/2,-r.size.height()/2},{r.size.width()/2,r.size.height()/2},{-r.size.width()/2,r.size.height()/2}})points<<r.centre+QPointF(p.x()*std::cos(a)-p.y()*std::sin(a),p.x()*std::sin(a)+p.y()*std::cos(a));return points;}
double cross(QPointF a,QPointF b,QPointF c){return (b.x()-a.x())*(c.y()-a.y())-(b.y()-a.y())*(c.x()-a.x());}
bool on(QPointF a,QPointF b,QPointF p){return std::abs(cross(a,b,p))<1e-10 && p.x()>=std::min(a.x(),b.x()) && p.x()<=std::max(a.x(),b.x()) && p.y()>=std::min(a.y(),b.y()) && p.y()<=std::max(a.y(),b.y());}
bool intersects(QPointF a,QPointF b,QPointF c,QPointF d){return (cross(a,b,c)*cross(a,b,d)<0 && cross(c,d,a)*cross(c,d,b)<0) || on(a,b,c) || on(a,b,d) || on(c,d,a) || on(c,d,b);}
void keys(const QJsonObject& o,std::initializer_list<QString> list){QSet<QString> expected(list.begin(),list.end());require(o.size()==expected.size(),"ROI fields missing/extra");for(auto i=o.begin();i!=o.end();++i)require(expected.contains(i.key()),"Unknown ROI field");}
double number(const QJsonObject& o,const char* key){require(o[key].isDouble() && finite(o[key].toDouble()),"Invalid ROI number");return o[key].toDouble();}
QJsonArray xy(QPointF p){return {p.x(),p.y()};}
QPointF xy(const QJsonValue& v){require(v.isArray() && v.toArray().size()==2,"ROI point must be pair");const auto a=v.toArray();require(a[0].isDouble() && a[1].isDouble(),"Invalid ROI point");QPointF p(a[0].toDouble(),a[1].toDouble());point(p);return p;}
}
void ToolRoi::validate() const {
    frame.validate();require((frame.unit==LengthUnit::Pixel && frame.axes==AxisConvention::XRightYDown) || (frame.unit==LengthUnit::Millimetre && (std::holds_alternative<ScanSegment>(geometry) || std::holds_alternative<RotatedRectangle>(geometry) || std::holds_alternative<Annulus>(geometry) || std::holds_alternative<ArcRoi>(geometry))),"Unsupported ROI plane/geometry");
    std::visit([](const auto& g){using T=std::decay_t<decltype(g)>;
        if constexpr(std::is_same_v<T,ScanSegment>){point(g.start);point(g.end);require(QLineF(g.start,g.end).length()>1e-9 && finite(g.width) && g.width>0,"Invalid scan segment/width");}
        else if constexpr(std::is_same_v<T,RotatedRectangle>){point(g.centre);require(finite(g.size.width()) && finite(g.size.height()) && g.size.width()>0 && g.size.height()>0 && finite(g.angleDegrees) && std::abs(g.angleDegrees)<=360,"Invalid rotated rectangle");}
        else if constexpr(std::is_same_v<T,Annulus>){point(g.centre);require(finite(g.innerRadius) && finite(g.outerRadius) && g.innerRadius>0 && g.outerRadius>g.innerRadius,"Invalid annulus radii");}
        else if constexpr(std::is_same_v<T,ArcRoi>){point(g.ring.centre);require(finite(g.ring.innerRadius) && finite(g.ring.outerRadius) && g.ring.innerRadius>0 && g.ring.outerRadius>g.ring.innerRadius && finite(g.startDegrees) && std::abs(g.startDegrees)<=360 && finite(g.sweepDegrees) && std::abs(g.sweepDegrees)>1e-9 && std::abs(g.sweepDegrees)<=360,"Invalid arc ROI");}
        else {
            constexpr bool closed=std::is_same_v<T,PolygonRoi>;const auto& p=g.vertices;require(p.size()>=(closed ? 3:2) && p.size()<=1024,"ROI vertex count 2/3..1024");
            for(int i=0;i<p.size();++i){point(p[i]);if(i)require(QLineF(p[i-1],p[i]).length()>1e-9,"Duplicate consecutive ROI vertices");}
            if constexpr(closed) {
                require(p.first()!=p.last(),"Polygon closure is implicit");double area=0;for(int i=0;i<p.size();++i)area+=cross({0,0},p[i],p[(i+1)%p.size()]);require(std::abs(area)>1e-9,"Zero polygon area");
                for(int i=0;i<p.size();++i)for(int j=i+1;j<p.size();++j)if(j!=i+1 && !(i==0 && j==p.size()-1))require(!intersects(p[i],p[(i+1)%p.size()],p[j],p[(j+1)%p.size()]),"Self intersecting polygon");
            }
        }
    },geometry);
}
QRectF ToolRoi::bounds() const {
    validate();return std::visit([](const auto& g)->QRectF {using T=std::decay_t<decltype(g)>;
        if constexpr(std::is_same_v<T,ScanSegment>){const auto d=g.end-g.start;const double l=QLineF(g.start,g.end).length();const auto n=QPointF(-d.y(),d.x())*(g.width/(2*l));return extent({g.start+n,g.end+n,g.end-n,g.start-n});}
        else if constexpr(std::is_same_v<T,RotatedRectangle>)return extent(corners(g));
        else if constexpr(std::is_same_v<T,Annulus>)return {g.centre-QPointF(g.outerRadius,g.outerRadius),QSizeF(g.outerRadius*2,g.outerRadius*2)};
        else if constexpr(std::is_same_v<T,ArcRoi>){
            QVector<QPointF> points;for(auto r:{g.ring.innerRadius,g.ring.outerRadius})for(auto a:{g.startDegrees,g.startDegrees+g.sweepDegrees})points<<radial(g.ring.centre,r,a);
            for(int k=-8;k<=8;++k){const auto a=k*90.0;const auto d=(a-g.startDegrees)/g.sweepDegrees;if(d>=0 && d<=1)for(auto r:{g.ring.innerRadius,g.ring.outerRadius})points<<radial(g.ring.centre,r,a);}return extent(points);
        } else return extent(g.vertices);
    },geometry);
}
bool ToolRoi::inside(const QSize& image) const {require(frame.unit==LengthUnit::Pixel,"World ROI requires explicit raw mapping");const auto b=bounds();return image.width()>0 && image.height()>0 && b.left()>=0 && b.top()>=0 && b.right()<=image.width()-1 && b.bottom()<=image.height()-1;}
QJsonObject ToolRoi::toJson() const {
    validate();auto o=std::visit([](const auto& g)->QJsonObject{using T=std::decay_t<decltype(g)>;
        if constexpr(std::is_same_v<T,ScanSegment>)return {{"kind","scan"},{"start",xy(g.start)},{"end",xy(g.end)},{"width",g.width}};
        else if constexpr(std::is_same_v<T,RotatedRectangle>)return {{"kind","rectangle"},{"centre",xy(g.centre)},{"width",g.size.width()},{"height",g.size.height()},{"angle",g.angleDegrees}};
        else if constexpr(std::is_same_v<T,Annulus>)return {{"kind","annulus"},{"centre",xy(g.centre)},{"inner",g.innerRadius},{"outer",g.outerRadius}};
        else if constexpr(std::is_same_v<T,ArcRoi>)return {{"kind","arc"},{"centre",xy(g.ring.centre)},{"inner",g.ring.innerRadius},{"outer",g.ring.outerRadius},{"start",g.startDegrees},{"sweep",g.sweepDegrees}};
        else {QJsonArray points;for(auto p:g.vertices)points<<xy(p);return {{"kind",std::is_same_v<T,PolygonRoi> ? "polygon":"path"},{"vertices",points}};}
    },geometry);QJsonObject result{{"schemaVersion",1},{"frameId",frame.id},{"geometry",o}};if(frame.unit==LengthUnit::Millimetre){result["schemaVersion"]=2;result["unit"]="mm";result["axes"]=frame.axes==AxisConvention::XRightYUp ? "up":"down";}return result;
}
ToolRoi ToolRoi::fromJson(const QJsonObject& o) {
    require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=128*1024,"ROI JSON limit");if(o["schemaVersion"]==2)keys(o,{"schemaVersion","frameId","geometry","unit","axes"});else keys(o,{"schemaVersion","frameId","geometry"});require((o["schemaVersion"]==1 || o["schemaVersion"]==2) && o["frameId"].isString() && o["geometry"].isObject(),"Invalid ROI schema");
    ToolRoi roi;roi.frame.id=o["frameId"].toString();if(o["schemaVersion"]==2){require(o["unit"]=="mm" && (o["axes"]=="up" || o["axes"]=="down"),"Invalid world ROI frame");roi.frame.unit=LengthUnit::Millimetre;roi.frame.axes=o["axes"]=="up" ? AxisConvention::XRightYUp:AxisConvention::XRightYDown;}const auto g=o["geometry"].toObject();const auto kind=g["kind"].toString();
    if(kind=="scan"){keys(g,{"kind","start","end","width"});roi.geometry=ScanSegment{xy(g["start"]),xy(g["end"]),number(g,"width")};}
    else if(kind=="rectangle"){keys(g,{"kind","centre","width","height","angle"});roi.geometry=RotatedRectangle{xy(g["centre"]),{number(g,"width"),number(g,"height")},number(g,"angle")};}
    else if(kind=="annulus"){keys(g,{"kind","centre","inner","outer"});roi.geometry=Annulus{xy(g["centre"]),number(g,"inner"),number(g,"outer")};}
    else if(kind=="arc"){keys(g,{"kind","centre","inner","outer","start","sweep"});roi.geometry=ArcRoi{{xy(g["centre"]),number(g,"inner"),number(g,"outer")},number(g,"start"),number(g,"sweep")};}
    else if(kind=="polygon" || kind=="path"){keys(g,{"kind","vertices"});require(g["vertices"].isArray() && g["vertices"].toArray().size()<=1024,"Invalid ROI vertices");QVector<QPointF> points;for(auto p:g["vertices"].toArray())points<<xy(p);if(kind=="polygon")roi.geometry=PolygonRoi{points};else roi.geometry=PathRoi{points};}
    else require(false,"Unknown ROI kind");roi.validate();return roi;
}
}
