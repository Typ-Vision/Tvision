#include "ProductionResultCodec.hpp"
#include "domain/ReferenceBinding.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
QString cell(QString s){s.replace('"',"\"\"");return '"'+s+'"';}
}
QJsonObject ProductionResultCodec::output(const PortValue& v) {
    QJsonObject o{{"type",portTypeName(v.port.type)},{"unit",v.port.unit},{"coordinateFrame",v.port.frame},{"frameId",v.frameId},{"cycleId",v.cycleId},{"sourceId",v.origin.sourceId}};
    o[v.port.type==PortType::Image ? "imageInfo":"value"]=v.valid ? ToolResultCodec::payload(v):QJsonValue(QJsonValue::Null);
    if(v.port.type!=PortType::Image)o["typedPort"]=ToolResultCodec::encode(v);
    return o;
}
void ProductionResultCodec::validate(const QJsonObject& record) {
    require(QJsonDocument(record).toJson(QJsonDocument::Compact).size()<=maxRecordBytes,"Terminal result exceeds 16 MiB");
    const auto schema=record["schemaVersion"];
    require(schema==1 || schema==2 || schema==3 || schema==4,"Unknown terminal result schema");
    require(record["tools"].isArray() && record["tools"].toArray().size()<=256,"Invalid terminal tools/limit");
    if(schema==4) {
        require(record["referenceBindings"].isObject() && !record["referenceBindings"].toObject().isEmpty(),"Missing terminal references");
        require(record["dependencies"].toObject()["referenceBindings"]==record["referenceBindings"],"Terminal reference dependency mismatch");
        require(record["configId"].toString().size()==64 && record["dependencyId"].toString().size()==64 && !record["validationId"].toString().isEmpty(),"Missing terminal snapshot identity");
        const auto refs=record["referenceBindings"].toObject();
        for(auto it=refs.cbegin();it!=refs.cend();++it) {
            require(it->isObject(),"Invalid terminal reference");ReferenceBinding::fromJson(it->toObject());
            bool present=false;for(const auto& t:record["tools"].toArray())if(t.toObject()["toolId"]==it.key())present=true;
            require(present || record["state"]!="DONE","Terminal reference consumer missing");
        }
    }
    for(const auto& item:record["tools"].toArray()) {
        require(item.isObject(),"Invalid terminal tool");const auto tool=item.toObject();
        require(tool["outputs"].isObject(),"Invalid terminal outputs");const auto outputs=tool["outputs"].toObject();
        require(outputs.size()<=128,"Terminal port limit");
        for(auto it=outputs.begin();it!=outputs.end();++it) {
            require(it->isObject(),"Invalid terminal port");const auto o=it->toObject();
            if(!o.contains("typedPort")) {
                require(o["type"]=="Image" || (schema.toInt()<3 && QStringList{"Number","Boolean","Point","Region"}.contains(o["type"].toString())),"Missing typed terminal port");continue;
            }
            require(o["typedPort"].isObject(),"Invalid typed terminal envelope");
            const auto v=ToolResultCodec::decode(o["typedPort"].toObject());
            require(v.port.id==it.key() && o==output(v),"Terminal port metadata/payload mismatch");
            if(v.valid) {
                require(v.cycleId==record["cycleId"].toString(),"Terminal port cycle mismatch");
                if(schema.toInt()>=3 && int(v.port.type)>int(PortType::Boolean))require(v.evaluationId==record["evaluationId"].toString(),"Terminal evaluation mismatch");
            }
            require(!v.diagnostic || tool["decision"]=="NOT_EVALUATED","Diagnostic cannot establish a terminal decision");
        }
    }
}
QString ProductionResultCodec::summary(const QJsonObject& record) {
    validate(record);QStringList lines;
    for(const auto& item:record["tools"].toArray()) {
        const auto t=item.toObject();lines<<t["name"].toString(t["toolId"].toString())+" • "+t["state"].toString()+" • "+t["decision"].toString();
        const auto outputs=t["outputs"].toObject();
        for(auto it=outputs.begin();it!=outputs.end();++it)if(it->toObject().contains("typedPort")) {
            const auto v=ToolResultCodec::decode(it->toObject()["typedPort"].toObject());
            lines<<"  "+it.key()+" ["+(v.diagnostic ? "diagnostic":"result")+"]: "+ToolResultCodec::display(v)+" "+v.port.unit;
        }
    }
    const auto refs=record["referenceBindings"].toObject();
    for(auto it=refs.cbegin();it!=refs.cend();++it) {
        const auto b=ReferenceBinding::fromJson(it->toObject());
        lines<<QString("Referans %1 ← %2.%3 • X=%4 Y=%5 px • açı=%6 rad").arg(it.key(),b.sourceTool,b.sourcePort).arg(b.offsetPx.x(),0,'g',17).arg(b.offsetPx.y(),0,'g',17).arg(b.angleOffsetRadians,0,'g',17);
    }
    return lines.join('\n');
}
QByteArray ProductionResultCodec::csv(const QVector<QJsonObject>& records) {
    require(records.size()<=256,"CSV record limit");
    QString data="cycleId,recipeId,toolId,port,type,unit,valid,diagnostic,frameId,evaluationId,method,methodVersion,value,reason,configId,validationId,dependencyId,referenceBinding\r\n";
    for(const auto& record:records) {
        validate(record);
        for(const auto& item:record["tools"].toArray()) {
            const auto t=item.toObject();const auto outputs=t["outputs"].toObject();
            for(auto it=outputs.begin();it!=outputs.end();++it) {
                const auto o=it->toObject();const auto p=o["typedPort"].toObject();
                const QStringList row{record["cycleId"].toString(),record["recipeId"].toString(),t["toolId"].toString(),it.key(),o["type"].toString(),o["unit"].toString(),p.isEmpty() ? "legacy":p["valid"].toBool() ? "true":"false",p["diagnostic"].toBool() ? "true":"false",o["frameId"].toString(),p["evaluationId"].toString(),p["method"].toString(),QString::number(p["methodVersion"].toInt()),QString::fromUtf8(QJsonDocument(QJsonArray{o.contains("value") ? o["value"]:o["imageInfo"]}).toJson(QJsonDocument::Compact)),p["invalidReason"].toString(),record["configId"].toString(),record["validationId"].toString(),record["dependencyId"].toString(),QString::fromUtf8(QJsonDocument(record["referenceBindings"].toObject()[t["toolId"].toString()].toObject()).toJson(QJsonDocument::Compact))};
                QStringList quoted;for(const auto& s:row)quoted<<cell(s);data+=quoted.join(',')+"\r\n";
                require(data.size()<=32*1024*1024,"CSV output exceeds limit");
            }
        }
    }return data.toUtf8();
}
}
