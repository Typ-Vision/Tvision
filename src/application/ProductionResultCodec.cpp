#include "ProductionResultCodec.hpp"
#include "domain/ReferenceBinding.hpp"
#include "SystemCalibration.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <stdexcept>
#include <algorithm>
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
    require(schema==1 || schema==2 || schema==3 || schema==4 || schema==5 || schema==6,"Unknown terminal result schema");
    const bool systemEnvelope=record.contains("systemCalibration");
    if(schema!=5 && schema!=6)require(!record.contains("systemCalibration") && !record["dependencies"].toObject().contains("systemCalibration"),"System calibration requires terminal schema5");
    if(schema!=6)require(!record.contains("measurementSource") && !record["dependencies"].toObject().contains("measurementSource"),"Physical source requires terminal schema6");
    if(schema==5)require(record["systemCalibration"].toObject()["schemaVersion"]==1,"Physical calibration requires terminal schema6");
    if(schema==6){
        const auto deps=record["dependencies"].toObject(),source=record["measurementSource"].toObject(),acquisition=source["acquisition"].toObject(),binding=source["binding"].toObject();
        require(source.size()==4 && source["schemaVersion"]==1 && binding.size()==3 && binding["schemaVersion"]==1 && binding["sourceToolId"].isString() && !binding["sourceToolId"].toString().isEmpty() && source==deps["measurementSource"].toObject() && acquisition.size()==3 && acquisition["target"]==binding["target"],"Terminal physical source mismatch");
        const auto target=SetupReference::fromJson(binding["target"].toObject());const auto device=DeviceBinding::fromJson(acquisition["device"].toObject());const auto geometry=GeometryDescriptor::fromJson(acquisition["geometry"].toObject());
        require(device.adapter=="webcam" && geometry.complete && !geometry.sensorRoi && !geometry.binning && (source["origin"]=="offline_replay" || source["origin"]=="camera_acquired"),"Unsupported terminal physical source");
        require(record["dependencyId"].toString()==QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(deps).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex()),"Terminal physical dependency hash mismatch");
        const auto kind=deps["tools"].toObject()[binding["sourceToolId"].toString()].toObject()["kind"].toString();require((kind=="image_file" && source["origin"]=="offline_replay") || (kind=="webcam" && source["origin"]=="camera_acquired"),"Terminal acquisition origin cannot be relabelled");
        require(record.contains("referenceBindings")==deps.contains("referenceBindings"),"Terminal physical reference evidence missing/extra");
        require(systemEnvelope==deps.contains("systemCalibration"),"Terminal physical calibration missing/extra");
        if(systemEnvelope){const auto s=SystemCalibration::decodeEvidence(record["systemCalibration"].toObject());require(!s.scope.isEmpty() && s.scope["target"]==target.toJson() && s.scope["device"]==acquisition["device"] && s.scope["setup"].toObject()["geometry"]==acquisition["geometry"],"Terminal physical calibration swap");}
    }
    require(record["tools"].isArray() && record["tools"].toArray().size()<=256,"Invalid terminal tools/limit");
    if(systemEnvelope){require(record["systemCalibration"].isObject() && record["systemCalibration"]==record["dependencies"].toObject()["systemCalibration"],"Terminal system calibration dependency mismatch");const auto s=SystemCalibration::decodeEvidence(record["systemCalibration"].toObject());
        require(!record["validationId"].toString().isEmpty() && record["configId"].toString().size()==64 && record["dependencyId"].toString().size()==64,"Missing system terminal snapshot identity");
        require(record["systemCalibration"].toObject()["executionDependency"]==record["dependencies"].toObject()["calibration"],"Terminal execution binding differs");}
    if(systemEnvelope){const auto deps=record["dependencies"].toObject();require(record.contains("referenceBindings")==deps.contains("referenceBindings"),"Terminal system reference evidence missing/extra");require(record["dependencyId"].toString()==QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(deps).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex()),"Terminal system dependency hash differs");}
    if(schema==4 || ((systemEnvelope || schema==6) && record.contains("referenceBindings"))) {
        require(record["referenceBindings"].isObject() && !record["referenceBindings"].toObject().isEmpty(),"Missing terminal references");
        require(record["dependencies"].toObject()["referenceBindings"]==record["referenceBindings"],"Terminal reference dependency mismatch");
        require(record["configId"].toString().size()==64 && record["dependencyId"].toString().size()==64 && !record["validationId"].toString().isEmpty(),"Missing terminal snapshot identity");
        const auto refs=record["referenceBindings"].toObject();
        for(auto it=refs.cbegin();it!=refs.cend();++it) {
            require(it->isObject(),"Invalid terminal reference");const auto binding=ReferenceBinding::fromJson(it->toObject());
            if(systemEnvelope && binding.world)require(binding.calibration && binding.calibration->toJson()==record["systemCalibration"].toObject()["executionDependency"].toObject(),"Terminal world reference differs from active system binding");
            bool present=false;for(const auto& t:record["tools"].toArray())if(t.toObject()["toolId"]==it.key())present=true;
            require(present || record["state"]!="DONE","Terminal reference consumer missing");
        }
    }
    const auto sourceFrames=record["frames"].toArray();
    if(schema==6){QString frame;const auto source=record["measurementSource"].toObject()["binding"].toObject()["sourceToolId"].toString();for(const auto& f:sourceFrames){const auto o=f.toObject();require(o["sourceId"]==source && o["cycleId"]==record["cycleId"] && !o["frameId"].toString().isEmpty() && (frame.isEmpty() || frame==o["frameId"].toString()),"Terminal physical frame provenance differs");frame=o["frameId"].toString();}}
    for(const auto& item:record["tools"].toArray()) {
        require(item.isObject(),"Invalid terminal tool");const auto tool=item.toObject();
        if(schema==6)require(tool["kind"]==record["dependencies"].toObject()["tools"].toObject()[tool["toolId"].toString()].toObject()["kind"],"Terminal tool kind differs from snapshot");
        require(tool["outputs"].isObject(),"Invalid terminal outputs");const auto outputs=tool["outputs"].toObject();
        require(outputs.size()<=128,"Terminal port limit");
        if(systemEnvelope && !tool["calibration"].isNull() && !tool["calibration"].isUndefined())require(tool["calibration"]==record["systemCalibration"].toObject()["executionDependency"],"Terminal metric tool calibration differs from system binding");
        for(auto it=outputs.begin();it!=outputs.end();++it) {
            require(it->isObject(),"Invalid terminal port");const auto o=it->toObject();
            if(!o.contains("typedPort")) {
                if(schema==6 && o["imageInfo"].isObject()){const auto source=record["measurementSource"].toObject(),image=o["imageInfo"].toObject();const auto geometry=GeometryDescriptor::fromJson(source["acquisition"].toObject()["geometry"].toObject());require(o["sourceId"]==source["binding"].toObject()["sourceToolId"] && image["width"]==geometry.resolution.width() && image["height"]==geometry.resolution.height() && o["cycleId"]==record["cycleId"] && std::any_of(sourceFrames.cbegin(),sourceFrames.cend(),[&](const auto& f){return f.toObject()["frameId"]==o["frameId"];}),"Terminal actual raster/physical source mismatch");}

                require(o["type"]=="Image" || (schema.toInt()<3 && QStringList{"Number","Boolean","Point","Region"}.contains(o["type"].toString())),"Missing typed terminal port");continue;
            }
            require(o["typedPort"].isObject(),"Invalid typed terminal envelope");
            const auto v=ToolResultCodec::decode(o["typedPort"].toObject());
            require(v.port.id==it.key() && o==output(v),"Terminal port metadata/payload mismatch");
            if(v.valid) {
                if(schema==6 && v.port.unit=="mm")require(systemEnvelope,"Physical mm result requires exact calibration evidence");
                if(schema==6)require(v.origin.sourceId==record["measurementSource"].toObject()["binding"].toObject()["sourceToolId"].toString() && std::any_of(sourceFrames.cbegin(),sourceFrames.cend(),[&](const auto& f){return f.toObject()["frameId"]==v.frameId && f.toObject()["cycleId"]==v.cycleId;}),"Terminal source/frame differs from physical source snapshot");
                if(systemEnvelope && v.port.unit=="mm")require(tool["calibration"]==record["systemCalibration"].toObject()["executionDependency"],"Terminal millimetre output lacks system binding");
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
    if(record.contains("systemCalibration")){const auto s=SystemCalibration::decodeEvidence(record["systemCalibration"].toObject());lines<<QString("Sistem kalibrasyonu: %1 v%2 • etkinleştirme %3 • %4").arg(s.record.name()).arg(s.selection.version).arg(s.selection.revision).arg(s.record.residualsMillimetres().isEmpty() ? "Bağımsız kontrol yapılmadı":"Kontrol kanıtı mevcut");}
    if(record["schemaVersion"]==6){const auto source=record["measurementSource"].toObject();const auto target=SetupReference::fromJson(source["binding"].toObject()["target"].toObject());lines<<QString("Kamera %1 • kurulum %2 r%3 • %4").arg(target.camera.value,target.setup.value).arg(target.revision).arg(source["origin"].toString());}
    for(auto it=refs.cbegin();it!=refs.cend();++it) {
        const auto b=ReferenceBinding::fromJson(it->toObject());
        lines<<QString("Referans %1 ← %2.%3 • X=%4 Y=%5 px • açı=%6 rad").arg(it.key(),b.sourceTool,b.sourcePort).arg(b.offsetPx.x(),0,'g',17).arg(b.offsetPx.y(),0,'g',17).arg(b.angleOffsetRadians,0,'g',17);
    }
    return lines.join('\n');
}
QByteArray ProductionResultCodec::csv(const QVector<QJsonObject>& records) {
    require(records.size()<=256,"CSV record limit");
    const bool system=std::any_of(records.cbegin(),records.cend(),[](const auto& r){return r.contains("systemCalibration");});
    const bool physical=std::any_of(records.cbegin(),records.cend(),[](const auto& r){return r["schemaVersion"]==6;});
    QString data="cycleId,recipeId,toolId,port,type,unit,valid,diagnostic,frameId,evaluationId,method,methodVersion,value,reason,configId,validationId,dependencyId,referenceBinding"+QString(system ? ",systemCalibration":"")+QString(physical ? ",measurementSource":"")+"\r\n";
    for(const auto& record:records) {
        validate(record);
        for(const auto& item:record["tools"].toArray()) {
            const auto t=item.toObject();const auto outputs=t["outputs"].toObject();
            for(auto it=outputs.begin();it!=outputs.end();++it) {
                const auto o=it->toObject();const auto p=o["typedPort"].toObject();
                const QStringList row{record["cycleId"].toString(),record["recipeId"].toString(),t["toolId"].toString(),it.key(),o["type"].toString(),o["unit"].toString(),p.isEmpty() ? "legacy":p["valid"].toBool() ? "true":"false",p["diagnostic"].toBool() ? "true":"false",o["frameId"].toString(),p["evaluationId"].toString(),p["method"].toString(),QString::number(p["methodVersion"].toInt()),QString::fromUtf8(QJsonDocument(QJsonArray{o.contains("value") ? o["value"]:o["imageInfo"]}).toJson(QJsonDocument::Compact)),p["invalidReason"].toString(),record["configId"].toString(),record["validationId"].toString(),record["dependencyId"].toString(),QString::fromUtf8(QJsonDocument(record["referenceBindings"].toObject()[t["toolId"].toString()].toObject()).toJson(QJsonDocument::Compact))};
                QStringList quoted;for(const auto& s:row)quoted<<cell(s);if(system)quoted<<cell(QString::fromUtf8(QJsonDocument(record["systemCalibration"].toObject()).toJson(QJsonDocument::Compact)));if(physical)quoted<<cell(QString::fromUtf8(QJsonDocument(record["measurementSource"].toObject()).toJson(QJsonDocument::Compact)));data+=quoted.join(',')+"\r\n";
                require(data.size()<=32*1024*1024,"CSV output exceeds limit");
            }
        }
    }return data.toUtf8();
}
}
