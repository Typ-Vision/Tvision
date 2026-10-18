#include "ToolDefinition.hpp"
#include "ToolGraph.hpp"
#include "ToolRegistry.hpp"
#include "MeasurementSetupContract.hpp"
#include "IndustrialSourceReference.hpp"
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
    require(version.isDouble() && version.toDouble()==version.toInt() && version.toInt()>=1 && version.toInt()<=9,"Desteklenmeyen editor şeması.");
    require(editor["tools"].isArray(),"Editor tools dizi olmalı.");const auto array=editor["tools"].toArray();require(array.size()<=256,"En fazla 256 tool desteklenir.");
    QVector<ToolDefinition> tools;QSet<QString> ids,names;int sourceCount=0;
    for(const auto value:array) {
        require(value.isObject(),"Tool nesne olmalı.");const auto o=value.toObject();
        if(version.toInt()>=7)keys(o,{"id","kind","name","description","enabled","parameters","dependencies"});else keys(o,{"id","kind","name","description","enabled","parameters"});
        for(const auto key:{"id","kind","name","description"})require(o[key].isString(),QString("Tool metin alanı eksik: %1").arg(key));
        require(o["enabled"].isBool() && o["parameters"].isObject(),"Tool enabled/parameters tipi hatalı.");
        ToolDefinition t{o["id"].toString(),o["kind"].toString(),o["name"].toString(),o["description"].toString(),o["enabled"].toBool(),o["parameters"].toObject()};
        if(version.toInt()>=7){require(o["dependencies"].isArray(),"Schema 7 dependencies array required");t.dependencies=decodeToolDependencies(o["dependencies"].toArray());}
        require(version.toInt()>=ToolRegistry::get(t.kind).minimumEditorSchema || ToolRegistry::get(t.kind).minimumEditorSchema==6,"Tool requires a newer editor schema");
        const auto id=QUuid(t.id);require(!id.isNull() && !ids.contains(id.toString()),"Tool ID geçersiz veya tekrar ediyor.");
        require(!t.name.isEmpty() && t.name.size()<=64 && t.name==t.name.trimmed(),"Tool adı 1–64 karakter olmalı.");
        for(auto c:t.name)require(c.isPrint(),"Tool adı yazdırılabilir karakterlerden oluşmalı.");
        require(!names.contains(t.name.toCaseFolded()),"Tool adı reçetede zaten var.");require(t.description.size()<=2048,"Tool açıklaması çok uzun.");
        const auto defaults=defaultParameters(t.kind);QSet<QString> allowed;for(auto it=defaults.begin();it!=defaults.end();++it)allowed.insert(it.key());
        if(t.kind=="circle_fit" && version.toInt()>=8)allowed.insert("worldCalibration");
        if(version.toInt()==1 && t.kind=="template_match") {keys(t.parameters,{"minScore"});require(t.parameters.size()==1,"Tool parametreleri eksik.");}
        if((version.toInt()==1 && t.kind=="template_match") || (version.toInt()<3 && (t.kind=="blob" || t.kind=="edge")))
            for(auto it=defaults.begin();it!=defaults.end();++it)if(!t.parameters.contains(it.key()))t.parameters[it.key()]=it.value();
        if(version.toInt()<4 && t.kind=="edge" && !t.parameters.contains("referenceOrigin"))t.parameters["referenceOrigin"]=QJsonObject{};
        if(version.toInt()<5 && t.kind=="edge" && !t.parameters.contains("referenceGeometry"))t.parameters["referenceGeometry"]="";
        if(version.toInt()<6 && t.kind=="blob")for(const auto key:{"polarity","connectivity","minArea","maxArea"})if(!t.parameters.contains(key))t.parameters[key]=defaults[key];
        require(!t.parameters.contains("referenceBinding") || version.toInt()>=8,"Reference binding requires editor schema 8");
        require(!t.parameters.contains("worldCalibration") || (t.kind=="circle_fit" && version.toInt()>=8),"World calibration requires Circle/editor schema 8");
        ToolRegistry::validate(t);
        if(ToolRegistry::get(t.kind).acquisitionSource)require(++sourceCount<=(version.toInt()==9 ? 256:1),"Bu aşamada reçetede tek edinim kaynağı desteklenir; mevcut Image File/Webcam kaynağını önce kaldırın.");
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
    const auto schema=requiredEditorSchema(tools,definition["editor"].toObject()["schemaVersion"].toInt(6));
    for(const auto& t:tools)array.append(encodeEditorTool(t,schema));
    definition["editor"]=QJsonObject{{"schemaVersion",schema},{"tools",array},{"connections",QJsonArray{}}};definition=withEditorConnections(definition,connections);
    validateEditorDefinition(definition);return definition;
}
int requiredEditorSchema(const QVector<ToolDefinition>& tools,int previousSchema) {
    require(previousSchema>=1 && previousSchema<=9,"Unsupported previous schema");int schema=std::max(6,previousSchema);
    for(const auto& t:tools){schema=std::max(schema,ToolRegistry::get(t.kind).minimumEditorSchema);if(!t.dependencies.isEmpty())schema=std::max(schema,7);}
    for(const auto& t:tools)if(t.parameters.contains("referenceBinding") || t.parameters.contains("worldCalibration"))schema=std::max(schema,8);
    return schema;
}
QJsonObject encodeEditorTool(const ToolDefinition& t,int schema) {
    QJsonObject o{{"id",t.id},{"kind",t.kind},{"name",t.name},{"description",t.description},{"enabled",t.enabled},{"parameters",t.parameters}};
    require(schema>=6 && schema<=9,"Unsupported editor write schema");
    if(schema>=7)o["dependencies"]=encodeToolDependencies(t.dependencies);else require(t.dependencies.isEmpty(),"Cannot downgrade dependencies");return o;
}
void validateEditorDefinition(const QJsonObject& definition) {
    const auto tools=editorTools(definition);editorConnections(definition);
    for(const auto& t:tools)if(t.kind=="industrial_camera" && t.parameters["sourceReference"].isObject()){
        const auto ref=IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject());
        auto binding=definition["sourceSetups"].toObject()[t.id].toObject();
        if(binding.isEmpty() && definition["sourceSetup"].toObject()["sourceToolId"]==t.id)binding=definition["sourceSetup"].toObject();
        if(!binding.isEmpty())require(binding["target"].toObject()==QJsonObject{{"schemaVersion",1},{"cameraId",ref.camera.value},{"setupId",ref.setup.value},{"revision",ref.setupRevision}},"Industrial source and calibration setup differ; explicit rebind required");
    }
    if(definition.contains("sourceSetups")) {
        require(definition["editor"].toObject()["schemaVersion"]==9 && definition["sourceSetups"].isObject(),"Source set requires editor9 object");
        const auto bindings=definition["sourceSetups"].toObject();require(bindings.size()<=256,"Too many source bindings");
        for(auto i=bindings.begin();i!=bindings.end();++i){require(i->isObject() && i->toObject()["sourceToolId"]==i.key(),"Source binding key mismatch");require(definition["sourceSetup"].toObject()["sourceToolId"]!=i.key(),"Duplicate legacy/source-set binding");auto single=definition;single.remove("sourceSetups");single["sourceSetup"]=*i;validateEditorDefinition(single);}
    }
    if(definition.contains("calibrationPolicy"))require(definition["calibrationPolicy"]=="scoped-source/1","Unknown calibration policy");
    if(definition.contains("sourceSetup")){require(definition["sourceSetup"].isObject(),"Source setup must be an object");const auto b=definition["sourceSetup"].toObject();keys(b,{"schemaVersion","sourceToolId","target"});require(b.size()==3 && b["schemaVersion"]==1 && b["sourceToolId"].isString() && b["target"].isObject(),"Invalid source setup schema");const auto target=b["target"].toObject();keys(target,{"schemaVersion","cameraId","setupId","revision"});require(target.size()==4 && target["schemaVersion"]==1 && target["cameraId"].isString() && target["setupId"].isString() && target["revision"].isDouble() && target["revision"].toDouble()==target["revision"].toInt() && target["revision"].toInt()>0 && target["revision"].toInt()<=1000000000,"Invalid source setup target");CameraId{target["cameraId"].toString()}.validate();SetupId{target["setupId"].toString()}.validate();bool found=false;for(const auto& t:tools)if(t.id==b["sourceToolId"].toString() && ToolRegistry::get(t.kind).acquisitionSource)found=true;require(found,"Source setup references a missing acquisition tool");}

    if(definition.contains("runtime")) {
        require(definition["runtime"].isObject(),"Runtime options must be an object.");const auto r=definition["runtime"].toObject();
        keys(r,{"trigger","measurementToolId"});
        require(r["measurementToolId"].toString().isEmpty() || !QUuid(r["measurementToolId"].toString()).isNull(),"Measurement mapping must be a tool UUID.");
        require(r.size()==2 && r["measurementToolId"].isString() && (r["trigger"]=="Manual" || r["trigger"]=="SoftwareTrigger"),"Invalid Runtime mode/mapping.");
    }
}
QJsonObject withMultiSourceSchema(QJsonObject definition) {
    validateEditorDefinition(definition);
    const auto tools=editorTools(definition);const auto connections=editorConnections(definition);
    auto editor=definition["editor"].toObject();QJsonArray array;
    for(const auto& t:tools)array.append(encodeEditorTool(t,9));
    editor["schemaVersion"]=9;editor["tools"]=array;definition["editor"]=editor;
    definition=withEditorConnections(definition,connections);validateEditorDefinition(definition);return definition;
}
}
