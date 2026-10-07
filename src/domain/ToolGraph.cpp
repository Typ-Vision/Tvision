#include "ToolGraph.hpp"
#include "ToolRegistry.hpp"
#include <QJsonArray>
#include <QSet>
#include <QMap>
#include <stdexcept>
namespace visionflow {
QString portTypeName(PortType type) {
    switch(type) {case PortType::Image:return "Image";case PortType::Region:return "Region";case PortType::Point:return "Point";case PortType::Number:return "Number";case PortType::Boolean:return "Boolean";} return {};
}
QVector<PortDefinition> inputPorts(const QString& kind) {return ToolRegistry::get(kind).inputs;}
QVector<PortDefinition> outputPorts(const QString& kind) {return ToolRegistry::get(kind).outputs;}
QVector<ToolConnection> editorConnections(const QJsonObject& definition) {
    const auto editor=definition["editor"].toObject(); QVector<ToolConnection> result;
    if (editor["schemaVersion"].toInt()<3) {
        const auto tools=editorTools(definition); QString source;
        for (const auto& t:tools) if (t.kind=="image_file") source=t.id;
        if (!source.isEmpty()) for (const auto& t:tools) if (t.kind!="image_file" && t.kind!="calculate") result.append({source,"image",t.id,"image"});
        return result;
    }
    if (!editor["connections"].isArray()) throw std::runtime_error("Connections dizi olmalı.");
    auto array=editor["connections"].toArray(); if (array.size()>2048) throw std::runtime_error("En fazla 2048 bağlantı.");
    for (const auto value:array) {
        auto o=value.toObject(); if (!value.isObject() || o.size()!=4) throw std::runtime_error("Bağlantı alanları hatalı.");
        for (const auto key:{"sourceTool","sourcePort","targetTool","targetPort"}) if (!o[key].isString() || o[key].toString().isEmpty() || o[key].toString().size()>128) throw std::runtime_error("Bağlantı referansı hatalı.");
        result.append({o["sourceTool"].toString(),o["sourcePort"].toString(),o["targetTool"].toString(),o["targetPort"].toString()});
    }
    if(editor["schemaVersion"].toInt()<4) {
        const auto tools=editorTools(definition);
        for(auto& c:result)if(c.targetPort=="region" && c.sourcePort=="region") {
            bool match=false,edge=false;for(const auto& t:tools){if(t.id==c.sourceTool && t.kind=="template_match")match=true;if(t.id==c.targetTool && t.kind=="edge")edge=true;}
            if(match && edge){c.targetPort="reference";c.sourcePort="point";}
        }
    }
    return result;
}
QJsonObject withEditorConnections(QJsonObject definition,const QVector<ToolConnection>& connections) {
    auto e=definition["editor"].toObject(); QJsonArray array;
    for (const auto& c:connections) array.append(QJsonObject{{"sourceTool",c.sourceTool},{"sourcePort",c.sourcePort},{"targetTool",c.targetTool},{"targetPort",c.targetPort}});
    e["schemaVersion"]=5; e["connections"]=array; definition["editor"]=e; return definition;
}
GraphValidation validateGraph(const QVector<ToolDefinition>& tools,const QVector<ToolConnection>& connections) {
    GraphValidation r; QMap<QString,ToolDefinition> nodes; QMap<QString,int> indegree; QMap<QString,QStringList> next; QSet<QString> bound;
    for (const auto& t:tools) {nodes[t.id]=t; indegree[t.id]=0;}
    for (const auto& c:connections) {
        if (!nodes.contains(c.sourceTool) || !nodes.contains(c.targetTool)) {r.errors<<"Eksik tool: "+c.sourceTool+" → "+c.targetTool;continue;}
        auto outputs=outputPorts(nodes[c.sourceTool].kind), inputs=inputPorts(nodes[c.targetTool].kind);
        auto out=std::find_if(outputs.begin(),outputs.end(),[&](const auto& p){return p.id==c.sourcePort;});
        auto in=std::find_if(inputs.begin(),inputs.end(),[&](const auto& p){return p.id==c.targetPort;});
        if (out==outputs.end() || in==inputs.end()) r.errors<<"Eksik port: "+c.targetPort;
        else if (out->type!=in->type || out->unit!=in->unit || out->frame!=in->frame) r.errors<<"Port tip/birim/frame uyumsuzluğu: "+c.sourcePort+" → "+c.targetPort;
        const auto key=c.targetTool+"/"+c.targetPort;
        if (bound.contains(key)) r.errors<<"Girişe birden fazla bağlantı: "+key;
        bound.insert(key); ++indegree[c.targetTool]; next[c.sourceTool]<<c.targetTool;
    }
    for (const auto& t:tools) if (t.enabled) for (const auto& p:inputPorts(t.kind)) if (p.required && !bound.contains(t.id+"/"+p.id)) r.errors<<t.name+": eksik giriş "+p.id;
    for(const auto& t:tools)if(t.kind=="edge" && bound.contains(t.id+"/reference") && bound.contains(t.id+"/region"))r.errors<<t.name+": region ve referans aynı anda bağlanamaz (uyumsuz arama modu).";
    QStringList ready; for (const auto& t:tools) if (indegree[t.id]==0) ready<<t.id;
    while (!ready.isEmpty()) {const auto id=ready.takeFirst(); r.order<<id; for (const auto& target:next[id]) if (--indegree[target]==0) ready<<target;}
    if (r.order.size()!=tools.size()) r.errors<<"Döngüsel bağımlılık (DAG gerekli).";
    return r;
}
}
