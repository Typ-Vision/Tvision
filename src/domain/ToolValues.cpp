#include "ToolValues.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <cmath>
#include <stdexcept>
#include <type_traits>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void keys(const QJsonObject& o,std::initializer_list<QString> names) {
    const QSet<QString> allowed(names.begin(),names.end());
    require(o.size()==allowed.size(),"Typed value fields missing/extra");
    for(auto it=o.begin();it!=o.end();++it)require(allowed.contains(it.key()),"Unknown typed field");
}
double number(const QJsonObject& o,const char* key){auto v=o[key];require(v.isDouble() && std::isfinite(v.toDouble()),"Expected finite number");return v.toDouble();}
QString text(const QJsonObject& o,const char* key,int max=ToolValues::maxText) {
    auto v=o[key];require(v.isString() && v.toString().size()<=max,"Invalid/big text");
    auto s=v.toString();
    for(qsizetype n=0;n<s.size();++n) {
        if(s[n].isHighSurrogate()){require(n+1<s.size() && s[n+1].isLowSurrogate(),"Unpaired Unicode surrogate");++n;}
        else require(!s[n].isLowSurrogate(),"Unpaired Unicode surrogate");
    }return s;
}
QJsonObject point(QPointF p){return {{"x",p.x()},{"y",p.y()}};}
QPointF point(const QJsonValue& v){require(v.isObject(),"Point must be object");auto o=v.toObject();keys(o,{"x","y"});return {number(o,"x"),number(o,"y")};}
QJsonArray points(const QVector<QPointF>& ps){QJsonArray a;for(auto p:ps)a.append(point(p));return a;}
QJsonArray array(const QJsonObject& o,const char* key,int limit=ToolValues::maxItems){require(o[key].isArray(),"Expected array");auto a=o[key].toArray();require(a.size()<=limit,"Collection exceeds limit");return a;}
QVector<QPointF> points(const QJsonArray& a){QVector<QPointF> ps;for(auto p:a)ps.append(point(p));return ps;}
QJsonObject frame(const CoordinateFrame& f){f.validate();require(f.id.size()<=128,"Frame ID too long");return {{"id",f.id},{"unit",f.unit==LengthUnit::Pixel ? "px":"mm"},{"axes",f.axes==AxisConvention::XRightYDown ? "down":"up"}};}
CoordinateFrame frame(const QJsonValue& v) {
    require(v.isObject(),"Frame must be object");auto o=v.toObject();keys(o,{"id","unit","axes"});
    require(o["unit"]=="px" || o["unit"]=="mm","Invalid frame unit");require(o["axes"]=="down" || o["axes"]=="up","Invalid axes");
    CoordinateFrame f{text(o,"id",128),o["unit"]=="px" ? LengthUnit::Pixel:LengthUnit::Millimetre,o["axes"]=="down" ? AxisConvention::XRightYDown:AxisConvention::XRightYUp};f.validate();return f;
}
QJsonObject circle(const Circle2D& c){return {{"centre",point(c.centre)},{"radius",c.radius},{"frame",frame(c.frame)}};}
Circle2D circle(const QJsonObject& o){keys(o,{"centre","radius","frame"});Circle2D c{point(o["centre"]),number(o,"radius"),frame(o["frame"])};require(c.radius>0,"Radius must be positive");return c;}
}
QJsonObject ToolValues::encode(const ToolValueData& value) {
    QString type;QJsonObject data;
    std::visit([&](const auto& v){using T=std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T,QString>)require(v.size()<=maxText,"Text too big");
        else if constexpr(std::is_same_v<T,CodeResults>){require(v.items.size()<=maxCodes,"Too many codes");for(const auto& c:v.items)require(c.text.size()<=maxText && c.bytes.size()<=maxText && c.corners.size()==4,"Code fields exceed limit");}
        else if constexpr(std::is_same_v<T,PointSet>)require(v.points.size()<=maxItems,"Too many points");
        else if constexpr(std::is_same_v<T,Profile>)require(v.samples.size()<=maxItems,"Too many samples");
        else if constexpr(std::is_same_v<T,InspectionResults>)require(v.items.size()<=128,"Too many inspections");
        if constexpr(std::is_same_v<T,QString>){type="Text";data={{"text",v}};}
        else if constexpr(std::is_same_v<T,CodeResults>){type="CodeResults";QJsonArray a;for(const auto& c:v.items)a.append(QJsonObject{{"text",c.text},{"bytes",QString::fromLatin1(c.bytes.toBase64())},{"symbology",c.symbology},{"corners",points(c.corners)}});data={{"items",a}};}
        else if constexpr(std::is_same_v<T,PointSet>){type="PointSet";data={{"points",points(v.points)},{"frame",frame(v.frame)}};}
        else if constexpr(std::is_same_v<T,Profile>){type="Profile";QJsonArray a;for(const auto& s:v.samples)a.append(QJsonObject{{"position",point(s.position)},{"value",s.value},{"derivative",s.derivative}});data={{"samples",a}};}
        else if constexpr(std::is_same_v<T,Line2D>){type="Line2D";data={{"start",point(v.start)},{"end",point(v.end)},{"frame",frame(v.frame)}};}
        else if constexpr(std::is_same_v<T,Circle2D>){type="Circle2D";data=circle(v);}
        else if constexpr(std::is_same_v<T,Arc2D>){type="Arc2D";data={{"circle",circle(v.circle)},{"startDegrees",v.startDegrees},{"sweepDegrees",v.sweepDegrees}};}
        else if constexpr(std::is_same_v<T,Pose2D>){type="Pose2D";data={{"position",point(v.position)},{"angleRadians",v.angleRadians},{"frame",frame(v.frame)}};}
        else if constexpr(std::is_same_v<T,SpatialTransform>){type="SpatialTransform";data=v.toJson();}
        else if constexpr(std::is_same_v<T,InspectionResults>){type="InspectionResults";QJsonArray a;for(const auto& i:v.items)a.append(QJsonObject{{"name",i.name},{"reason",i.reason},{"accepted",i.accepted}});data={{"items",a}};}
    },value);
    QJsonObject o{{"schemaVersion",1},{"type",type},{"data",data}};
    decode(o);return o;
}
ToolValueData ToolValues::decode(const QJsonObject& o) {
    require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=maxJsonBytes,"Typed payload exceeds 2 MiB");
    keys(o,{"schemaVersion","type","data"});require(o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1 && o["data"].isObject(),"Unsupported typed schema");
    auto type=text(o,"type",32);const auto d=o["data"].toObject();
    if(type=="Text"){keys(d,{"text"});return text(d,"text");}
    if(type=="CodeResults") {
        keys(d,{"items"});CodeResults r;
        for(auto item:array(d,"items",maxCodes)) {
            require(item.isObject(),"Code must be object");auto c=item.toObject();keys(c,{"text","bytes","symbology","corners"});
            const auto encoded=text(c,"bytes",maxText*2).toLatin1();
            const auto decoded=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
            require(bool(decoded) && decoded.decoded.size()<=maxText && decoded.decoded.toBase64()==encoded,"Invalid/noncanonical binary code payload");
            auto sym=text(c,"symbology",32);require(QSet<QString>{"QR","DataMatrix","Code128","Code39","EAN8","EAN13","UPCA","UPCE"}.contains(sym),"Unknown code symbology");
            auto ps=points(array(c,"corners",4));require(ps.size()==4,"Code requires four corners");
            r.items.append({text(c,"text"),sym,decoded.decoded,ps});
        }return r;
    }
    if(type=="PointSet"){keys(d,{"points","frame"});return PointSet{points(array(d,"points")),frame(d["frame"])};}
    if(type=="Profile") {
        keys(d,{"samples"});Profile r;for(auto item:array(d,"samples")){require(item.isObject(),"Sample must be object");auto s=item.toObject();keys(s,{"position","value","derivative"});r.samples.append({point(s["position"]),number(s,"value"),number(s,"derivative")});}return r;
    }
    if(type=="Line2D"){keys(d,{"start","end","frame"});Line2D l{point(d["start"]),point(d["end"]),frame(d["frame"])};require(l.start!=l.end,"Degenerate line");return l;}
    if(type=="Circle2D")return circle(d);
    if(type=="Arc2D"){keys(d,{"circle","startDegrees","sweepDegrees"});require(d["circle"].isObject(),"Arc circle missing");Arc2D a{circle(d["circle"].toObject()),number(d,"startDegrees"),number(d,"sweepDegrees")};require(a.startDegrees>=-360 && a.startDegrees<=360 && a.sweepDegrees!=0 && std::abs(a.sweepDegrees)<=360,"Invalid arc angle/sweep");return a;}
    if(type=="Pose2D"){keys(d,{"position","angleRadians","frame"});Pose2D p{point(d["position"]),number(d,"angleRadians"),frame(d["frame"])};p.validate();return p;}
    if(type=="SpatialTransform")return SpatialTransform::fromJson(d);
    if(type=="InspectionResults") {
        keys(d,{"items"});InspectionResults r;QSet<QString> names;for(auto item:array(d,"items",128)){require(item.isObject(),"Inspection must be object");auto i=item.toObject();keys(i,{"name","reason","accepted"});auto name=text(i,"name",128);require(!name.isEmpty() && !names.contains(name) && i["accepted"].isBool(),"Invalid/duplicate inspection");names.insert(name);r.items.append({name,text(i,"reason",2048),i["accepted"].toBool()});}return r;
    }
    throw std::runtime_error("Unknown typed result type");
}
}
