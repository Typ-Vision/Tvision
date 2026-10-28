#include "ToolResultCodec.hpp"
#include <QJsonDocument>
#include <QSet>
#include <cmath>
#include <stdexcept>
#include <type_traits>
namespace visionflow {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void keys(const QJsonObject& o,std::initializer_list<QString> names) {
    const QSet<QString> allowed(names.begin(),names.end());require(o.size()==allowed.size(),"Port fields missing/extra");
    for(auto it=o.begin();it!=o.end();++it)require(allowed.contains(it.key()),"Unknown port field");
}
QString str(const QJsonObject& o,const char* key,int max=256){require(o[key].isString() && o[key].toString().size()<=max,"Invalid port text");return o[key].toString();}
double num(const QJsonObject& o,const char* key){require(o[key].isDouble() && std::isfinite(o[key].toDouble()),"Invalid port number");return o[key].toDouble();}
bool finite(QPointF p){return std::isfinite(p.x()) && std::isfinite(p.y());}
}
QJsonValue ToolResultCodec::payload(const PortValue& v) {
    return std::visit([](const auto& p)->QJsonValue {using T=std::decay_t<decltype(p)>;
        if constexpr(std::is_same_v<T,double>){require(std::isfinite(p),"Nonfinite Number");return p;}
        else if constexpr(std::is_same_v<T,bool>)return p;
        else if constexpr(std::is_same_v<T,QPointF>){require(finite(p),"Nonfinite Point");return QJsonObject{{"x",p.x()},{"y",p.y()}};}
        else if constexpr(std::is_same_v<T,QRect>){require(p.width()>0 && p.height()>0,"Invalid Region");return QJsonObject{{"x",p.x()},{"y",p.y()},{"width",p.width()},{"height",p.height()}};}
        else if constexpr(std::is_same_v<T,ImageFrame>){require(!p.size().isEmpty(),"Empty Image");QJsonObject o{{"width",p.size().width()},{"height",p.size().height()},{"pixelFormat",p.pixelFormat()}};if(!p.processingId().isEmpty())o["processingId"]=QString::fromLatin1(p.processingId().toHex());return o;}
        else if constexpr(std::is_same_v<T,AssignmentBundle>)return encodeAssignmentBundle(p);
        else return ToolValues::encode(ToolValueData(p));
    },v.data);
}
void ToolResultCodec::validate(const PortValue& v,const PortDefinition& expected,const QString& frameId,const QString& cycleId) {
    require(v.port.id==expected.id && v.port.type==expected.type && v.port.unit==expected.unit && v.port.frame==expected.frame,"Port declaration mismatch");
    require(!portTypeName(v.port.type).isEmpty() && !v.port.id.isEmpty() && v.port.id.size()<=128 && v.port.unit.size()<=32 && !v.port.frame.isEmpty() && v.port.frame.size()<=128,"Invalid port declaration");
    if(!v.valid){if(int(v.port.type)>int(PortType::Boolean))require(!v.invalidReason.isEmpty() && v.invalidReason.size()<=2048,"Invalid typed port requires reason");return;}
    if(int(v.port.type)>int(PortType::Boolean))require(!v.evaluationId.isEmpty() && v.evaluationId.size()<=256 && !v.method.isEmpty() && v.method.size()<=128 && v.methodVersion>=1 && v.methodVersion<=1000000 && v.invalidReason.isEmpty(),"Typed value method/evaluation provenance missing");
    if(v.port.type==PortType::AssignmentBundle){
        require(v.port.unit.isEmpty() && v.port.frame=="cycle" && v.frameId.isEmpty() && v.cycleId==cycleId && !cycleId.isEmpty() && std::holds_alternative<AssignmentBundle>(v.data),"Bundle port identity mismatch");
        const auto& b=std::get<AssignmentBundle>(v.data);require(b.cycleId==cycleId,"Bundle cycle mismatch");encodeAssignmentBundle(b);return;
    }
    if(v.port.frame=="cycle" && (v.port.type==PortType::Text || v.port.type==PortType::Boolean)) {
        require(v.port.unit.isEmpty() && v.frameId.isEmpty() && !cycleId.isEmpty() && v.cycleId==cycleId && v.origin.sourceId.isEmpty() && v.origin.frameId.isEmpty() && v.origin.coordinateSystemId.isEmpty(),"Cycle scalar identity mismatch");
        require((v.port.type==PortType::Text && std::holds_alternative<QString>(v.data)) || (v.port.type==PortType::Boolean && std::holds_alternative<bool>(v.data)),"Cycle scalar type mismatch");return;
    }
    require(!frameId.isEmpty() && !cycleId.isEmpty() && v.frameId==frameId && v.cycleId==cycleId && v.origin.frameId==frameId && v.origin.cycleId==cycleId && !v.origin.sourceId.isEmpty() && !v.origin.coordinateSystemId.isEmpty(),"Port frame/cycle/source mismatch");
    if(v.imageGeometry)require(v.imageGeometry->coordinateSystemId()==v.origin.coordinateSystemId && !v.imageGeometry->identity().isEmpty(),"Port image geometry/origin mismatch");
    if(const auto* image=std::get_if<ImageFrame>(&v.data))if(v.imageGeometry)require(*v.imageGeometry==image->geometry(),"Image geometry scope mismatch");
    const auto name=portTypeName(v.port.type);
    const auto encoded=payload(v);
    const auto actual=std::visit([](const auto& p)->QString {using T=std::decay_t<decltype(p)>;
        if constexpr(std::is_same_v<T,double>)return "Number";
        else if constexpr(std::is_same_v<T,bool>)return "Boolean";
        else if constexpr(std::is_same_v<T,QPointF>)return "Point";
        else if constexpr(std::is_same_v<T,QRect>)return "Region";
        else if constexpr(std::is_same_v<T,ImageFrame>)return "Image";
        else if constexpr(std::is_same_v<T,AssignmentBundle>)return "AssignmentBundle";
        else return ToolValues::encode(ToolValueData(p))["type"].toString();
    },v.data);
    require(!name.isEmpty() && name==actual,"Port union/type mismatch");
    if(v.port.type==PortType::Boolean || v.port.type==PortType::Text || v.port.type==PortType::InspectionResults)require(v.port.unit.isEmpty(),"Non-metric port unit must be empty");
    if(v.port.type==PortType::CodeResults)require(v.port.unit=="px" && v.port.frame=="image","Codes use raw pixel corners");
    if(v.port.type==PortType::Profile)require(v.port.unit=="gray8" && v.port.frame=="image","Profile unit/frame mismatch");
    std::visit([&](const auto& p){using T=std::decay_t<decltype(p)>;
        if constexpr(std::is_same_v<T,ImageFrame>)require(p.metadata().frameId==frameId && p.metadata().cycleId==cycleId && p.metadata().sourceId==v.origin.sourceId && p.metadata().coordinateSystemId==v.origin.coordinateSystemId,"Image origin mismatch");
        else if constexpr(std::is_same_v<T,PointSet> || std::is_same_v<T,Line2D> || std::is_same_v<T,Circle2D> || std::is_same_v<T,Pose2D> || std::is_same_v<T,Arc2D>) {
            const auto& f=[&]()->const CoordinateFrame& {if constexpr(std::is_same_v<T,Arc2D>)return p.circle.frame;else return p.frame;}();
            require(v.port.unit==(f.unit==LengthUnit::Pixel ? "px":"mm"),"Geometry unit mismatch");
            require((f.unit==LengthUnit::Pixel && v.port.frame=="image" && f.id==v.origin.coordinateSystemId && f.axes==AxisConvention::XRightYDown) || (f.unit==LengthUnit::Millimetre && v.port.frame=="plane"),"Geometry coordinate frame/axes mismatch");
        }
    },v.data);
}
QJsonObject ToolResultCodec::encode(const PortValue& v) {
    require(v.port.type!=PortType::Image,"Image bytes are not port JSON");validate(v,v.port,v.frameId,v.cycleId);
    return {{"schemaVersion",1},{"id",v.port.id},{"type",portTypeName(v.port.type)},{"unit",v.port.unit},{"coordinateFrame",v.port.frame},{"required",v.port.required},{"valid",v.valid},
        {"frameId",v.frameId},{"cycleId",v.cycleId},{"sourceId",v.origin.sourceId},{"sourceCoordinates",v.origin.coordinateSystemId},
        {"evaluationId",v.evaluationId},{"method",v.method},{"methodVersion",v.methodVersion},{"diagnostic",v.diagnostic},{"invalidReason",v.invalidReason},
        {"value",v.valid ? payload(v):QJsonValue(QJsonValue::Null)}};
}
PortValue ToolResultCodec::decode(const QJsonObject& o) {
    require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=(o["type"]=="AssignmentBundle" ? 8*1024*1024:ToolValues::maxJsonBytes)+4096,"Port envelope too big");
    keys(o,{"schemaVersion","id","type","unit","coordinateFrame","required","valid","frameId","cycleId","sourceId","sourceCoordinates","value","evaluationId","method","methodVersion","diagnostic","invalidReason"});
    require(o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1 && o["valid"].isBool() && o["required"].isBool(),"Invalid port schema/flags");
    const auto type=str(o,"type");PortType pt=PortType::Image;
    for(int n=int(PortType::Region);n<=int(PortType::AssignmentBundle);++n)if(portTypeName(PortType(n))==type)pt=PortType(n);
    require(pt!=PortType::Image,"Unknown/non-serializable port type");
    PortValue v;v.port={str(o,"id"),pt,str(o,"unit"),str(o,"coordinateFrame"),o["required"].toBool()};v.valid=o["valid"].toBool();
    v.frameId=str(o,"frameId");v.cycleId=str(o,"cycleId");v.origin={str(o,"sourceId"),v.frameId,v.cycleId,str(o,"sourceCoordinates")};
    v.evaluationId=str(o,"evaluationId");v.method=str(o,"method",128);v.invalidReason=str(o,"invalidReason",2048);const auto mv=num(o,"methodVersion");require(mv>=0 && mv<=1000000 && mv==std::floor(mv) && o["diagnostic"].isBool(),"Invalid method version/diagnostic");v.methodVersion=int(mv);v.diagnostic=o["diagnostic"].toBool();
    if(!v.valid){require(o["value"].isNull(),"Invalid port cannot carry payload");validate(v,v.port,v.frameId,v.cycleId);return v;}
    const auto value=o["value"];
    if(pt==PortType::AssignmentBundle){require(value.isObject(),"Bundle payload required");v.data=decodeAssignmentBundle(value.toObject());}
    else if(pt==PortType::Number){require(value.isDouble() && std::isfinite(value.toDouble()),"Invalid number");v.data=value.toDouble();}
    else if(pt==PortType::Boolean){require(value.isBool(),"Invalid bool");v.data=value.toBool();}
    else if(pt==PortType::Point || pt==PortType::Region) {
        require(value.isObject(),"Invalid geometry");auto p=value.toObject();
        if(pt==PortType::Point){keys(p,{"x","y"});v.data=QPointF(num(p,"x"),num(p,"y"));}
        else {keys(p,{"x","y","width","height"});for(auto key:{"x","y","width","height"}){auto n=num(p,key);require(n>=-100000000 && n<=100000000 && std::floor(n)==n,"Invalid Region integer");}v.data=QRect(int(num(p,"x")),int(num(p,"y")),int(num(p,"width")),int(num(p,"height")));}
    } else {require(value.isObject(),"Typed payload must be object");std::visit([&](const auto& p){
        if constexpr(std::is_same_v<std::decay_t<decltype(p)>,SpatialTransform>)throw std::runtime_error("Spatial payload requires an explicit spatial port contract");
        else v.data=p;
    },ToolValues::decode(value.toObject()));}
    validate(v,v.port,v.frameId,v.cycleId);return v;
}
QString ToolResultCodec::display(const PortValue& v,int precision) {
    if(!v.valid)return v.invalidReason.isEmpty() ? QStringLiteral("geçersiz"):v.invalidReason;
    return std::visit([&](const auto& p)->QString {using T=std::decay_t<decltype(p)>;
        if constexpr(std::is_same_v<T,double>)return QString::number(p,'f',precision);
        else if constexpr(std::is_same_v<T,bool>)return p ? "true":"false";
        else if constexpr(std::is_same_v<T,QPointF>)return QString("(%1, %2)").arg(p.x()).arg(p.y());
        else if constexpr(std::is_same_v<T,QRect>)return QString("(%1, %2) %3×%4").arg(p.x()).arg(p.y()).arg(p.width()).arg(p.height());
        else if constexpr(std::is_same_v<T,ImageFrame>)return QString("%1×%2").arg(p.size().width()).arg(p.size().height());
        else if constexpr(std::is_same_v<T,QString>)return p;
        else if constexpr(std::is_same_v<T,CodeResults>){
            if(p.items.isEmpty())return "Okunamadı (NO_READ)";
            QStringList rows;
            for(int i=0;i<p.items.size();++i){const auto& code=p.items[i];QJsonArray text;text.append(code.text);auto escaped=QString::fromUtf8(QJsonDocument(text).toJson(QJsonDocument::Compact));escaped=escaped.mid(1,escaped.size()-2);rows<<QString("#%1 %2 | %3 | %4 byte").arg(i).arg(code.symbology,escaped).arg(code.bytes.size());}
            return rows.join("\n");
        }
        else if constexpr(std::is_same_v<T,InspectionResults>){QStringList rows;for(const auto& check:p.items)rows<<check.name+": "+(check.accepted ? "Uygun":"Uygun değil")+(check.reason.isEmpty() ? "":" — "+check.reason);return rows.join(" | ");}
        else if constexpr(std::is_same_v<T,AssignmentBundle>)return QString("%1 assigned variables (revision %2)").arg(p.fields.size()).arg(p.revision);
        else return QString::fromUtf8(QJsonDocument(ToolValues::encode(ToolValueData(p))).toJson(QJsonDocument::Compact));
    },v.data);
}
std::optional<CoordinateFrame> ToolResultCodec::coordinates(const PortValue& v) {
    return std::visit([](const auto& p)->std::optional<CoordinateFrame>{using T=std::decay_t<decltype(p)>;
        if constexpr(std::is_same_v<T,PointSet> || std::is_same_v<T,Line2D> || std::is_same_v<T,Circle2D> || std::is_same_v<T,Pose2D>)return p.frame;
        else if constexpr(std::is_same_v<T,Arc2D>)return p.circle.frame;
        else return {};
    },v.data);
}
}
