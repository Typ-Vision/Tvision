#include "ToolGraph.hpp"
#include "ToolRegistry.hpp"
#include "ReferenceConfig.hpp"
#include "PoseTrackingConfig.hpp"
#include "GeometryMeasurementConfig.hpp"
#include <QJsonArray>
#include <QSet>
#include <QMap>
#include <stdexcept>
namespace visionflow {
QString portTypeName(PortType type) {
    switch(type) {case PortType::Image:return "Image";case PortType::Region:return "Region";case PortType::Point:return "Point";case PortType::Number:return "Number";case PortType::Boolean:return "Boolean";
    case PortType::Text:return "Text";case PortType::CodeResults:return "CodeResults";case PortType::PointSet:return "PointSet";case PortType::Profile:return "Profile";case PortType::Line2D:return "Line2D";case PortType::Circle2D:return "Circle2D";case PortType::Arc2D:return "Arc2D";case PortType::Pose2D:return "Pose2D";case PortType::InspectionResults:return "InspectionResults";} return {};
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
    const auto tools=editorTools(definition);const auto schema=requiredEditorSchema(tools,e["schemaVersion"].toInt(6));
    QJsonArray normalized;for(const auto& t:tools)normalized.append(encodeEditorTool(t,schema));
    e["tools"]=normalized;e["schemaVersion"]=schema; e["connections"]=array; definition["editor"]=e; return definition;
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
    for(const auto& t:tools)if(supportsPoseTracking(t.kind) && !t.parameters.contains("referenceBinding")) {
        const bool tracking=t.parameters.contains("poseTracking");
        if(tracking!=bound.contains(t.id+"/poseReference"))r.errors<<t.name+": pose reference and teaching binding must be configured together";
        if(tracking)try {
            if(bound.contains(t.id+"/region") || bound.contains(t.id+"/reference"))r.errors<<t.name+": pose tracking cannot be combined with region/point reference";
            const auto sourceId=trackingSourceTool(t);
            for(const auto& c:connections)if(c.targetTool==t.id && c.targetPort=="poseReference") {
                const auto source=nodes.value(c.sourceTool);
                if(c.sourceTool!=sourceId || c.sourcePort!="pose" || (source.kind!="pattern_pose" && source.kind!="shape_pose"))
                    r.errors<<t.name+": pose reference producer differs from teaching";
            }
        }catch(const std::exception& e){r.errors<<t.name+": "+QString::fromUtf8(e.what());}
    }
    for(const auto& t:tools)if(t.parameters.contains("referenceBinding"))try {
        const auto b=toolReferenceBinding(t);const auto input=referenceInput(b);
        if(!bound.contains(t.id+"/"+input) || bound.contains(t.id+"/region") || bound.contains(t.id+"/"+(input=="reference" ? "poseReference":"reference")))
            r.errors<<t.name+": reference binding/connection conflict";
        for(const auto& c:connections)if(c.targetTool==t.id && c.targetPort==input) {
            const auto source=nodes.value(c.sourceTool);
            if(c.sourceTool!=b.sourceTool || c.sourcePort!=b.sourcePort || !source.enabled || !referenceSourcePort(source.kind,c.sourcePort) ||
               ((b.mode==ReferenceMode::Rigid)!=(c.sourcePort=="pose")))r.errors<<t.name+": reference source/port/mode invalid";
        }
    }catch(const std::exception& e){r.errors<<t.name+": "+QString::fromUtf8(e.what());}
    for(const auto& t:tools)if(supportsReference(t.kind) && !t.parameters.contains("referenceBinding")) {
        if(t.kind!="edge" && bound.contains(t.id+"/reference"))r.errors<<t.name+": point reference requires binding";
        if(t.kind=="edge" && bound.contains(t.id+"/poseReference"))r.errors<<t.name+": pose reference requires binding";
    }
    for(const auto& t:tools)if(t.kind=="edge" && bound.contains(t.id+"/reference") && bound.contains(t.id+"/region"))r.errors<<t.name+": region ve referans aynı anda bağlanamaz (uyumsuz arama modu).";
    for(const auto& t:tools)if(t.enabled && t.kind=="geometry_measurement") {
        try {
            const auto required=geometryMeasurementRequiredInputs(t.parameters);
            QMap<QString,ToolConnection> bindings;
            for(const auto& c:connections)if(c.targetTool==t.id){bindings[c.targetPort]=c;if(!required.contains(c.targetPort))r.errors<<t.name+": operation does not use input "+c.targetPort;}
            for(const auto& port:required)if(!bindings.contains(port))r.errors<<t.name+": operation requires input "+port;
            if(t.parameters["mode"]=="calibrated")for(const auto index:{QString("1"),QString("2")}) {
                const auto feature=required.contains("line"+index) ? "line"+index:"circle"+index;
                if(!required.contains(feature))continue;
                const auto sample="samples"+index;
                if(bindings.contains(feature) && bindings.contains(sample) && bindings[feature].sourceTool!=bindings[sample].sourceTool)
                    r.errors<<t.name+": feature and observations must come from the same tool";
            }
        }catch(const std::exception& e){r.errors<<t.name+": "+QString::fromUtf8(e.what());}
    }
    QStringList ready; for (const auto& t:tools) if (indegree[t.id]==0) ready<<t.id;
    while (!ready.isEmpty()) {const auto id=ready.takeFirst(); r.order<<id; for (const auto& target:next[id]) if (--indegree[target]==0) ready<<target;}
    if (r.order.size()!=tools.size()) r.errors<<"Döngüsel bağımlılık (DAG gerekli).";
    return r;
}
}
