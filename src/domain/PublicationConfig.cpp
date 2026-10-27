#include "PublicationConfig.hpp"
#include "ToolDefinition.hpp"
#include <QJsonArray>
#include <QRegularExpression>
#include <QUuid>
#include <stdexcept>
namespace visionflow {
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void keys(const QJsonObject& o,const QSet<QString>& k){need(o.size()==k.size(),"Publication fields missing/extra");for(auto i=o.begin();i!=o.end();++i)need(k.contains(i.key()),"Unknown publication field");}
QString uuid(const QJsonObject& o,const char* k){need(o[k].isString(),"Publication UUID required");auto s=o[k].toString();need(!QUuid(s).isNull() && QUuid(s).toString(QUuid::WithoutBraces)==s,"Canonical publication UUID required");return s;}
int revision(const QJsonObject& o,const char* k){need(o[k].isDouble() && o[k].toDouble()==o[k].toInt() && o[k].toInt()>0 && o[k].toInt()<=1000000000,"Invalid publication revision");return o[k].toInt();}
}
QJsonObject encodeOutputGroup(const OutputGroup& g){QJsonArray fields;for(const auto& f:g.fields)fields.append(QJsonObject{{"name",f.name},{"fieldId",f.fieldId}});QJsonObject o{{"id",g.id},{"profileId",g.profileId},{"contractRevision",g.contractRevision},{"required",g.required},{"payloadSchema",g.payloadSchema},{"producerId",g.producerId},{"producerRevision",g.producerRevision},{"fields",fields}};decodeOutputGroup(o);return o;}
OutputGroup decodeOutputGroup(const QJsonObject& o){
    keys(o,{"id","profileId","contractRevision","required","payloadSchema","producerId","producerRevision","fields"});OutputGroup g;
    g.id=uuid(o,"id");g.profileId=uuid(o,"profileId");g.producerId=uuid(o,"producerId");g.contractRevision=revision(o,"contractRevision");g.producerRevision=revision(o,"producerRevision");
    need(o["required"].isBool() && o["payloadSchema"]=="vf.fields/1","Invalid publication requirement/schema");g.required=o["required"].toBool();
    need(o["fields"].isArray(),"Publication fields array required");const auto a=o["fields"].toArray();need(!a.isEmpty() && a.size()<=maxPublicationFields,"Publication field limit is 1..64");QSet<QString> names;
    static const QRegularExpression name("^[A-Za-z][A-Za-z0-9_.-]{0,63}$");
    for(const auto& v:a){need(v.isObject(),"Publication field object required");const auto f=v.toObject();keys(f,{"name","fieldId"});need(f["name"].isString() && name.match(f["name"].toString()).hasMatch() && !names.contains(f["name"].toString()),"Invalid/duplicate logical field");names.insert(f["name"].toString());g.fields.append({f["name"].toString(),uuid(f,"fieldId")});}return g;
}
QVector<OutputGroup> recipeOutputGroups(const QJsonObject& d){
    if(!d.contains("publication"))return {};need(d["publication"].isObject(),"Publication object required");const auto o=d["publication"].toObject();keys(o,{"schemaVersion","groups"});need(o["schemaVersion"]==1 && o["groups"].isArray(),"Invalid publication schema");
    const auto a=o["groups"].toArray();need(a.size()<=maxOutputGroups,"Publication group limit is 16");QVector<OutputGroup> groups;QSet<QString> ids;for(auto v:a){need(v.isObject(),"Output group object required");auto g=decodeOutputGroup(v.toObject());need(!ids.contains(g.id),"Duplicate output group");ids.insert(g.id);groups.append(g);}return groups;
}
QJsonObject withRecipeOutputGroups(QJsonObject d,const QVector<OutputGroup>& groups){QJsonArray a;for(const auto& g:groups)a.append(encodeOutputGroup(g));d["publication"]=QJsonObject{{"schemaVersion",1},{"groups",a}};validatePublicationBindings(d);return d;}
QJsonObject encodePublishConfig(const PublishConfig& c){QJsonArray a;for(const auto& id:c.groups)a.append(id);QJsonObject o{{"groups",a}};decodePublishConfig(o);return o;}
PublishConfig decodePublishConfig(const QJsonObject& o){keys(o,{"groups"});need(o["groups"].isArray() && o["groups"].toArray().size()<=maxOutputGroups,"Invalid Publish Data groups");PublishConfig c;for(auto v:o["groups"].toArray()){QJsonObject id{{"id",v}};auto s=uuid(id,"id");need(!c.groups.contains(s),"Duplicate Publish Data group");c.groups.append(s);}return c;}
void validatePublicationBindings(const QJsonObject& d){
    const auto groups=recipeOutputGroups(d);const auto tools=editorTools(d);QSet<QString> available,used;
    for(const auto& g:groups){available.insert(g.id);bool found=false;for(const auto& t:tools)if(t.id==g.producerId && t.kind=="set_variables"){
        const auto b=decodeAssignmentConfig(t.parameters,t.id);need(b.revision==g.producerRevision,"Publication bundle revision mismatch");QSet<QString> fields;for(const auto& r:b.rows)fields.insert(r.targetId);for(const auto& f:g.fields)need(fields.contains(f.fieldId),"Publication bundle field missing");found=true;
    }need(found,"Publication bundle producer unresolved");}
    for(const auto& t:tools)if(t.kind=="publish_data"){const auto c=decodePublishConfig(t.parameters);need(!t.enabled || !c.groups.isEmpty(),"Enabled Publish Data requires groups");for(const auto& id:c.groups){need(available.contains(id),"Publish Data group unresolved");if(t.enabled){need(!used.contains(id),"Output group published twice");used.insert(id);}}}
}
}
