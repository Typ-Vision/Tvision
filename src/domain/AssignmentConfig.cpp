#include "AssignmentConfig.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
void keys(const QJsonObject& o,std::initializer_list<QString> fields){QSet<QString> k(fields.begin(),fields.end());require(o.size()==k.size(),"Assignment fields missing/extra");for(auto i=o.begin();i!=o.end();++i)require(k.contains(i.key()),"Unknown assignment field");}
QString text(const QJsonObject& o,const char* k){require(o[k].isString() && !o[k].toString().isEmpty() && o[k].toString().size()<=128,"Invalid assignment identity");return o[k].toString();}
void uuid(const QString& s){require(!QUuid(s).isNull(),"Assignment UUID required");}
int revision(const QJsonObject& o){const auto v=o["revision"];require(v.isDouble() && v.toDouble()==v.toInt() && v.toInt()>0 && v.toInt()<=1000000000,"Invalid assignment revision");return v.toInt();}
// The codec checks enum shape, not membership. The recipe catalog is authoritative
// at evaluation time; untrusted values never extend that catalog.
EnumDefinitions shapeEnums(const QJsonObject& o){EnumDefinitions e;if(o["type"]=="Enum"){auto v=o["value"].toObject();e[v["definitionId"].toString()].insert(v["code"].toInt());}return e;}
EnumDefinitions shapeEnums(const ValueEnvelope& v){EnumDefinitions e;if(v.value)if(auto p=std::get_if<EnumValue>(&*v.value))e[p->definitionId].insert(p->code);return e;}
}
QJsonObject encodeAssignmentValue(const AssignmentValue& v){
    if(auto p=std::get_if<ValueEnvelope>(&v))return {{"kind","scalar"},{"envelope",CommunicationValues::encode(*p,shapeEnums(*p))}};
    const auto& s=std::get<SpatialAssignmentValue>(v);require(s.unit==ScalarUnit::Millimetre && (s.validity==ValueValidity::Valid)==s.value.has_value(),"Spatial assignment requires mm transform");
    ValueEnvelope m;m.type=ScalarType::Boolean;if(s.value)m.value=true;m.validity=s.validity;m.reason=s.reason;m.producerId=s.producerId;m.producerPort=s.producerPort;m.cycleId=s.cycleId;m.frameId=s.frameId;m.configId=s.configId;m.dependencyIds=s.dependencyIds;
    return {{"kind","spatial"},{"metadata",CommunicationValues::encode(m)},{"transform",s.value ? QJsonValue(s.value->toJson()):QJsonValue(QJsonValue::Null)}};
}
AssignmentValue decodeAssignmentValue(const QJsonObject& o){
    if(o["kind"]=="scalar"){keys(o,{"kind","envelope"});require(o["envelope"].isObject(),"Scalar envelope required");auto e=o["envelope"].toObject();return CommunicationValues::decode(e,shapeEnums(e));}
    keys(o,{"kind","metadata","transform"});require(o["kind"]=="spatial" && o["metadata"].isObject() && (o["transform"].isObject() || o["transform"].isNull()),"Spatial assignment required");auto m=CommunicationValues::decode(o["metadata"].toObject());require(m.type==ScalarType::Boolean && (m.validity==ValueValidity::Valid ? (m.value && std::get<bool>(*m.value) && o["transform"].isObject()):(!m.value && o["transform"].isNull())),"Spatial metadata invalid");
    return SpatialAssignmentValue{o["transform"].isNull() ? std::optional<SpatialTransform>{}:std::optional<SpatialTransform>{SpatialTransform::fromJson(o["transform"].toObject())},m.validity,m.reason,ScalarUnit::Millimetre,m.producerId,m.producerPort,m.cycleId,m.frameId,m.configId,m.dependencyIds};
}
AssignmentConfig decodeAssignmentConfig(const QJsonObject& o,const QString& producer){
    keys(o,{"revision","rows"});uuid(producer);AssignmentConfig c{producer,revision(o),{}};
    require(o["rows"].isArray() && o["rows"].toArray().size()<=AssignmentConfig::maxRows,"Invalid assignment rows");QSet<QString> targets;
    for(auto v:o["rows"].toArray()){
        require(v.isObject(),"Assignment row required");auto r=v.toObject();keys(r,{"targetId","source"});auto target=text(r,"targetId");uuid(target);require(!targets.contains(target),"Duplicate assignment target");targets.insert(target);
        require(r["source"].isObject(),"Assignment source required");auto s=r["source"].toObject();AssignmentSource source;
        if(s["kind"]=="constant"){keys(s,{"kind","value"});require(s["value"].isObject(),"Assignment value required");auto value=decodeAssignmentValue(s["value"].toObject());std::visit([](const auto& e){require(e.validity==ValueValidity::Valid && e.value && e.producerId.isEmpty() && e.producerPort.isEmpty() && e.cycleId.isEmpty() && e.configId.isEmpty() && e.frameId.isEmpty() && e.dependencyIds.isEmpty(),"Constant cannot carry runtime provenance");},value);source=ConstantAssignmentSource{value};}
        else if(s["kind"]=="tool_output"){keys(s,{"kind","toolId","port","revision"});auto id=text(s,"toolId");uuid(id);source=ToolOutputAssignmentSource{id,text(s,"port"),revision(s)};}
        else {keys(s,{"kind","producerId","fieldId","revision"});require(s["kind"]=="bundle","Unknown assignment source");auto id=text(s,"producerId"),field=text(s,"fieldId");uuid(id);uuid(field);source=BundleAssignmentSource{id,field,revision(s)};}
        c.rows.append({target,source});
    }return c;
}
QJsonObject encodeAssignmentConfig(const AssignmentConfig& c){QJsonArray rows;for(const auto& r:c.rows){QJsonObject s;if(auto p=std::get_if<ConstantAssignmentSource>(&r.source))s={{"kind","constant"},{"value",encodeAssignmentValue(p->value)}};else if(auto p=std::get_if<ToolOutputAssignmentSource>(&r.source))s={{"kind","tool_output"},{"toolId",p->toolId},{"port",p->port},{"revision",p->revision}};else {const auto& b=std::get<BundleAssignmentSource>(r.source);s={{"kind","bundle"},{"producerId",b.producerId},{"fieldId",b.fieldId},{"revision",b.revision}};}rows.append(QJsonObject{{"targetId",r.targetId},{"source",s}});}QJsonObject o{{"revision",c.revision},{"rows",rows}};decodeAssignmentConfig(o,c.producerId);return o;}
QJsonObject encodeAssignmentBundle(const AssignmentBundle& b){
    uuid(b.producerId);require(b.committed && !b.cycleId.isEmpty() && !b.configId.isEmpty() && b.cycleId.size()<=128 && b.configId.size()<=128 && b.revision>0 && b.revision<=1000000000 && !b.fields.isEmpty() && b.fields.size()<=AssignmentConfig::maxRows,"Invalid assignment bundle");QJsonObject fields;
    for(auto i=b.fields.begin();i!=b.fields.end();++i){uuid(i.key());std::visit([&](const auto& e){require(e.validity==ValueValidity::Valid && e.value && e.producerId==b.producerId && e.producerPort==i.key() && e.cycleId==b.cycleId && e.configId==b.configId,"Bundle field provenance mismatch");},i.value());fields[i.key()]=encodeAssignmentValue(i.value());}
    QJsonObject o{{"schemaVersion",1},{"producerId",b.producerId},{"cycleId",b.cycleId},{"configId",b.configId},{"revision",b.revision},{"fields",fields}};require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=8*1024*1024,"Assignment bundle too large");return o;
}
AssignmentBundle decodeAssignmentBundle(const QJsonObject& o){require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=8*1024*1024,"Assignment bundle too large");keys(o,{"schemaVersion","producerId","cycleId","configId","revision","fields"});require(o["schemaVersion"]==1 && o["fields"].isObject(),"Invalid bundle schema");AssignmentBundle b{text(o,"producerId"),text(o,"cycleId"),text(o,"configId"),revision(o),true,{}};auto f=o["fields"].toObject();require(f.size()<=AssignmentConfig::maxRows,"Too many bundle fields");for(auto i=f.begin();i!=f.end();++i){require(i->isObject(),"Invalid bundle field");b.fields[i.key()]=decodeAssignmentValue(i->toObject());}encodeAssignmentBundle(b);return b;}
}
