#include "ToolDefinition.hpp"
#include "ToolGraph.hpp"
#include "ToolRegistry.hpp"
#include <QJsonArray>
#include <QSet>
#include <QUuid>
#include <QStringList>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <stdexcept>
#include <cmath>
#include <algorithm>
namespace visionflow {
namespace {
void require(bool ok,const QString& message) {if (!ok) throw std::runtime_error(message.toUtf8().constData());}
void keys(const QJsonObject& o,const QSet<QString>& allowed) {for (auto it=o.begin();it!=o.end();++it) require(allowed.contains(it.key()),"Desteklenmeyen editor alanı: "+it.key());}
void roi(const QJsonValue& value) {
    require(value.isObject(),"ROI nesne olmalı.");const auto o=value.toObject(); keys(o,{"x","y","width","height"});
    if (o.isEmpty()) return; require(o.size()==4,"ROI alanları eksik.");
    for (const auto key:{"x","y","width","height"}) {auto v=o[key];require(v.isDouble() && v.toDouble()==v.toInt() && v.toDouble()>=0 && v.toDouble()<=100000,"ROI koordinatı geçersiz.");}
    require(o["width"].toInt()>0 && o["height"].toInt()>0,"ROI boyutu pozitif olmalı.");
}
}
QString toolLabel(const QString& kind) {return ToolRegistry::get(kind).label;}
QJsonObject defaultParameters(const QString& kind) {return ToolRegistry::get(kind).defaults;}
QVector<ToolDefinition> editorTools(const QJsonObject& definition) {
    if(!definition.contains("editor"))return {};
    require(definition["editor"].isObject(),"definition.editor nesne olmalı.");const auto editor=definition["editor"].toObject();
    keys(editor,{"schemaVersion","tools","connections"});const auto version=editor["schemaVersion"];
    require(version.isDouble() && version.toDouble()==version.toInt() && version.toInt()>=1 && version.toInt()<=6,"Desteklenmeyen editor şeması.");
    require(editor["tools"].isArray(),"Editor tools dizi olmalı.");const auto array=editor["tools"].toArray();require(array.size()<=256,"En fazla 256 tool desteklenir.");
    QVector<ToolDefinition> tools;QSet<QString> ids,names;int sourceCount=0;
    for(const auto value:array) {
        require(value.isObject(),"Tool nesne olmalı.");const auto o=value.toObject();keys(o,{"id","kind","name","description","enabled","parameters"});
        for(const auto key:{"id","kind","name","description"})require(o[key].isString(),QString("Tool metin alanı eksik: %1").arg(key));
        require(o["enabled"].isBool() && o["parameters"].isObject(),"Tool enabled/parameters tipi hatalı.");
        ToolDefinition t{o["id"].toString(),o["kind"].toString(),o["name"].toString(),o["description"].toString(),o["enabled"].toBool(),o["parameters"].toObject()};
        const auto id=QUuid(t.id);require(!id.isNull() && !ids.contains(id.toString()),"Tool ID geçersiz veya tekrar ediyor.");
        require(!t.name.isEmpty() && t.name.size()<=64 && t.name==t.name.trimmed(),"Tool adı 1–64 karakter olmalı.");
        for(auto c:t.name)require(c.isPrint(),"Tool adı yazdırılabilir karakterlerden oluşmalı.");
        require(!names.contains(t.name.toCaseFolded()),"Tool adı reçetede zaten var.");require(t.description.size()<=2048,"Tool açıklaması çok uzun.");
        const auto defaults=defaultParameters(t.kind);QSet<QString> allowed;for(auto it=defaults.begin();it!=defaults.end();++it)allowed.insert(it.key());
        if(version.toInt()==1 && t.kind=="template_match") {keys(t.parameters,{"minScore"});require(t.parameters.size()==1,"Tool parametreleri eksik.");}
        if((version.toInt()==1 && t.kind=="template_match") || (version.toInt()<3 && (t.kind=="blob" || t.kind=="edge")))
            for(auto it=defaults.begin();it!=defaults.end();++it)if(!t.parameters.contains(it.key()))t.parameters[it.key()]=it.value();
        if(version.toInt()<4 && t.kind=="edge" && !t.parameters.contains("referenceOrigin"))t.parameters["referenceOrigin"]=QJsonObject{};
        if(version.toInt()<5 && t.kind=="edge" && !t.parameters.contains("referenceGeometry"))t.parameters["referenceGeometry"]="";
        if(version.toInt()<6 && t.kind=="blob")for(const auto key:{"polarity","connectivity","minArea","maxArea"})if(!t.parameters.contains(key))t.parameters[key]=defaults[key];
        ToolRegistry::validate(t);
        if(ToolRegistry::get(t.kind).acquisitionSource)require(++sourceCount<=1,"Bu aşamada reçetede tek edinim kaynağı desteklenir; mevcut Image File/Webcam kaynağını önce kaldırın.");
        ids.insert(id.toString());names.insert(t.name.toCaseFolded());tools.append(t);
    }
    // Schema 3 used the match rectangle as an edge region. Preserve the drawn box
    // and establish its translation anchor at the taught model center.
    if(version.toInt()<4)for(const auto c:editor["connections"].toArray()) {
        const auto binding=c.toObject();if(binding["targetPort"]!="region" || binding["sourcePort"]!="region")continue;
        for(const auto& source:tools)if(source.id==binding["sourceTool"].toString() && source.kind=="template_match") {
            const auto model=source.parameters["modelRoi"].toObject();if(model.isEmpty())continue;
            for(auto& target:tools)if(target.id==binding["targetTool"].toString() && target.kind=="edge")target.parameters["referenceOrigin"]=QJsonObject{{"x",model["x"].toDouble()+(model["width"].toDouble()-1)/2},{"y",model["y"].toDouble()+(model["height"].toDouble()-1)/2}};
        }
    }
    // Legacy files cannot prove historical teaching identity. Preserve their
    // current relationship and establish a baseline once, without dirtying open.
    if(version.toInt()<5)for(const auto value:editor["connections"].toArray()) {
        const auto c=value.toObject();
        for(const auto& source:tools)if(source.id==c["sourceTool"].toString())
            for(auto& target:tools)if(target.id==c["targetTool"].toString() && target.kind=="edge" &&
                (c["targetPort"]=="reference" || (version.toInt()<4 && c["targetPort"]=="region" && source.kind=="template_match")) &&
                !target.parameters["referenceOrigin"].toObject().isEmpty())target.parameters["referenceGeometry"]=referenceGeometry(source);
    }
    return tools;
}
QString referenceGeometry(const ToolDefinition& tool) {
    const auto parameters=tool.kind=="template_match" ? QJsonObject{{"modelRoi",tool.parameters["modelRoi"]},{"modelPng",tool.parameters["modelPng"]}} : tool.parameters;
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(QJsonObject{{"id",tool.id},{"kind",tool.kind},{"geometry",parameters}}).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
bool referenceNeedsTeaching(const ToolDefinition& edge,const ToolDefinition& source) {
    return edge.parameters["referenceGeometry"].toString()!=referenceGeometry(source);
}
QJsonObject withEditorTools(QJsonObject definition,const QVector<ToolDefinition>& tools) {
    const auto connections=editorConnections(definition);QJsonArray array;
    for(const auto& t:tools)array.append(QJsonObject{{"id",t.id},{"kind",t.kind},{"name",t.name},{"description",t.description},{"enabled",t.enabled},{"parameters",t.parameters}});
    definition["editor"]=QJsonObject{{"schemaVersion",6},{"tools",array},{"connections",QJsonArray{}}};definition=withEditorConnections(definition,connections);
    validateEditorDefinition(definition);return definition;
}
void validateEditorDefinition(const QJsonObject& definition) {
    editorTools(definition);editorConnections(definition);
    if(definition.contains("runtime")) {
        require(definition["runtime"].isObject(),"Runtime options must be an object.");const auto r=definition["runtime"].toObject();
        keys(r,{"trigger","measurementToolId"});
        require(r["measurementToolId"].toString().isEmpty() || !QUuid(r["measurementToolId"].toString()).isNull(),"Measurement mapping must be a tool UUID.");
        require(r.size()==2 && r["measurementToolId"].isString() && (r["trigger"]=="Manual" || r["trigger"]=="SoftwareTrigger"),"Invalid Runtime mode/mapping.");
    }
}
}
