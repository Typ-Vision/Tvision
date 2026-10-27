#include "MeasurementDiagnostic.hpp"
#include "ProductionResultCodec.hpp"
#include "IndustrialRuntime.hpp"
#include "SourceStatusPresenter.hpp"
#include "domain/ReferenceBinding.hpp"
#include "SystemCalibration.hpp"
#include "SourceFrameIndex.hpp"
#include "domain/ToolRegistry.hpp"
#include <QSet>
#include <QUuid>
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
namespace {
QString bundleHash(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
QJsonObject legacyRecord(QJsonObject r){r["schemaVersion"]=r.take("baseSchemaVersion");r.remove("resultBundle");return r;}
}
QJsonObject ProductionResultCodec::withBundle(QJsonObject r,const QJsonObject& definition,const CycleResult* result,bool force){
    if(r["schemaVersion"]==10)return r;
    QSet<QString> active;for(const auto& t:editorTools(definition))if(t.enabled && t.kind=="publish_data")for(const auto& id:decodePublishConfig(t.parameters).groups)active.insert(id);
    const auto configured=editorTools(definition);if(!force && active.isEmpty() && std::none_of(configured.begin(),configured.end(),[](const auto& t){return t.enabled && t.kind=="result";}))return r;
    QJsonArray groups;
    for(const auto& g:recipeOutputGroups(definition))if(active.contains(g.id)){
        QJsonObject fields;bool valid=false;QString intentId,profileDependencyId;
        if(result)for(const auto& intent:result->publicationIntents)if(intent.groupId==g.id){
            require(intent.cycleId==r["cycleId"].toString() && intent.configId==QString::fromLatin1(r["configId"].toString().toLatin1().toHex()),"Bundle intent identity differs");
            intentId=intent.id;profileDependencyId=QString::fromLatin1(intent.profileDependencyId);valid=result->state==ExecutionState::Done;
            for(auto f=intent.fields.begin();f!=intent.fields.end();++f){fields[f.key()]=encodeAssignmentValue(f.value());std::visit([&](const auto& v){valid=valid && v.validity==ValueValidity::Valid;},f.value());}
        }
        if(fields.isEmpty())for(const auto& field:g.fields)for(const auto& variable:recipeVariables(definition))if(variable.id==field.fieldId){
            if(variable.type==VariableType::SpatialTransform){SpatialAssignmentValue v;v.reason="PUBLICATION_UNAVAILABLE";v.producerId=g.producerId;v.producerPort=field.fieldId;v.cycleId=r["cycleId"].toString();v.configId=QString::fromLatin1(r["configId"].toString().toLatin1().toHex());fields[field.name]=encodeAssignmentValue(v);}
            else {ValueEnvelope v;v.type=static_cast<ScalarType>(variable.type);v.unit=variable.unit;v.reason="PUBLICATION_UNAVAILABLE";v.producerId=g.producerId;v.producerPort=field.fieldId;v.cycleId=r["cycleId"].toString();v.configId=QString::fromLatin1(r["configId"].toString().toLatin1().toHex());fields[field.name]=encodeAssignmentValue(v);}
        }
        groups.append(QJsonObject{{"groupId",g.id},{"profileId",g.profileId},{"contractRevision",g.contractRevision},{"required",g.required},{"payloadSchema",g.payloadSchema},{"intentId",intentId},{"profileDependencyId",profileDependencyId},{"resultValid",valid},{"reason",valid ? "":"PUBLICATION_UNAVAILABLE"},{"fields",fields}});
    }
    QJsonObject b{{"schemaVersion",1},{"cycleId",r["cycleId"]},{"configHash",r["configId"]},{"dependencyId",r["dependencyId"]},{"decision",result ? resultDecisionKey(result->decision()):QString("NOT_EVALUATED")},{"groups",groups}};
    b["bundleId"]=bundleHash(b);r["baseSchemaVersion"]=r["schemaVersion"].toInt();r["schemaVersion"]=10;r["resultBundle"]=b;return r;
}
QJsonObject ProductionResultCodec::output(const PortValue& v) {
    QJsonObject o{{"type",portTypeName(v.port.type)},{"unit",v.port.unit},{"coordinateFrame",v.port.frame},{"frameId",v.frameId},{"cycleId",v.cycleId},{"sourceId",v.origin.sourceId}};
    o[v.port.type==PortType::Image ? "imageInfo":"value"]=v.valid ? ToolResultCodec::payload(v):QJsonValue(QJsonValue::Null);
    if(v.port.type!=PortType::Image)o["typedPort"]=ToolResultCodec::encode(v);
    return o;
}
namespace {
void validateMulti(const QJsonObject& r){
    const auto deps=r["dependencies"].toObject(),set=deps["sourceSet"].toObject(),branches=r["branches"].toObject();
    require(!r.contains("systemCalibration") && !r.contains("measurementSource") && !r.contains("referenceBindings"),"Multi-source evidence must stay scoped");
    require((deps.size()==3 || (deps.size()==4 && deps.contains("publication"))) && (deps["scope"]=="multi_source_in_memory/1" || deps["scope"]=="multi_source_runtime/1") && deps["engine"].isString(),"Invalid source-set envelope");
    require(r["dependencyId"].toString()==QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(deps).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex()),"Source-set dependency hash differs");
    const auto config=r["configuration"].toObject();require(r["configId"].toString()==QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(config).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex()) && config["recipeId"]==r["recipeId"] && config["definition"]==r["definition"] && config["recipeVersion"]==r["recipeVersion"] && config["name"]==r["recipeName"] && config["type"]==r["applicationType"],"Multi-source configuration hash differs");
    require(!r["validationId"].toString().isEmpty() && !r["cycleId"].toString().isEmpty(),"Missing multi-source terminal identity");
    const auto definition=r["definition"].toObject();require(definition["editor"].toObject()["schemaVersion"]==9,"Multi-source terminal definition schema");validateEditorDefinition(definition);
    const auto tools=editorTools(definition);const auto graph=validateSourceGraph(tools,editorConnections(definition));require(graph.valid() && !graph.requiredSources.isEmpty() && set.size()==graph.requiredSources.size() && branches.size()==set.size(),"Terminal required source set differs");
    require(r["frames"].isArray() && r["tools"].isArray() && r["tools"].toArray().size()<=256 && QStringList{"DONE","ERROR","SKIPPED"}.contains(r["state"].toString()),"Invalid multi-source terminal state/arrays");
    QVector<FrameMetadata> frames;QStringList acquired;QMap<QString,QJsonObject> bySource,byTool;
    for(const auto& v:r["frames"].toArray()){require(v.isObject(),"Invalid terminal frame");const auto f=v.toObject();const auto source=f["sourceId"].toString();require(graph.requiredSources.contains(source) && !bySource.contains(source),"Duplicate/foreign terminal source");FrameMetadata m;m.sourceId=source;m.frameId=f["frameId"].toString();m.cycleId=f["cycleId"].toString();m.coordinateSystemId=f["coordinateSystemId"].toString();m.completedAtUtc=QDateTime::fromString(f["completedAtUtc"].toString(),Qt::ISODateWithMs);require(m.completedAtUtc.isValid(),"Missing frame timestamp");frames.append(m);acquired.append(source);bySource[source]=f;}
    if(r["state"]=="DONE")require(acquired.size()==set.size(),"DONE terminal missing source");
    std::optional<SourceFrameIndex> index;if(!frames.isEmpty())index.emplace(r["cycleId"].toString(),acquired,frames);
    for(const auto& v:r["tools"].toArray()){require(v.isObject(),"Invalid terminal tool");const auto t=v.toObject();const auto id=t["toolId"].toString();auto configured=std::find_if(tools.cbegin(),tools.cend(),[&](const auto& c){return c.id==id;});require(configured!=tools.cend() && !byTool.contains(id) && t["kind"]==configured->kind && t["outputs"].isObject(),"Foreign/duplicate terminal tool");byTool[id]=t;
        if(configured->kind=="set_variables"){
            require(QStringList{"DONE","ERROR","SKIPPED"}.contains(t["state"].toString()) && t["decision"]=="NOT_EVALUATED" && t["calibration"].isNull(),"Invalid cycle data tool state/decision");
            if(t["state"]=="DONE")require(t["outputs"].toObject().size()==1 && t["outputs"].toObject().contains("bundle"),"Completed assignment bundle missing");
            if(r["state"]=="SKIPPED" || t["state"]!="DONE")require(t["outputs"].toObject().isEmpty(),"Failed/cancelled assignment carried output");
        }
        validateMeasurementDiagnostic(t,r);if(t.contains("measurementDiagnostic"))require(t["measurementDiagnostic"].toObject()["sourceId"]==graph.toolSources.value(id),"Diagnostic graph source differs");
        if(!graph.requiredSources.contains(graph.toolSources.value(id)) && !(configured->enabled && (configured->kind=="set_variables" || configured->kind=="result" || configured->kind=="publish_data")))require(t["state"]=="SKIPPED" && t["outputs"].toObject().isEmpty() && t["decision"]=="NOT_EVALUATED" && t["calibration"].isNull(),"Disabled branch carried evidence");
        const auto outputs=t["outputs"].toObject();for(auto p=outputs.begin();p!=outputs.end();++p){const auto o=p->toObject();if(o.contains("typedPort")){auto value=ToolResultCodec::decode(o["typedPort"].toObject());if(value.valid){const auto ports=outputPorts(configured->kind);const auto port=std::find_if(ports.cbegin(),ports.cend(),[&](const auto& x){return x.id==p.key();});require(port!=ports.cend(),"Undeclared terminal port");if(value.port.type==PortType::AssignmentBundle){require(configured->kind=="set_variables" && value.cycleId==r["cycleId"].toString() && value.evaluationId==r["evaluationId"].toString(),"Assignment terminal identity differs");const auto& b=std::get<AssignmentBundle>(value.data);const auto c=decodeAssignmentConfig(configured->parameters,id);require(b.producerId==id && b.revision==c.revision && b.configId==QString::fromLatin1(r["configId"].toString().toLatin1().toHex()),"Assignment terminal configuration differs");require(b.fields.size()==c.rows.size(),"Assignment terminal field count differs");for(const auto& row:c.rows)require(b.fields.contains(row.targetId),"Assignment terminal target differs");}else if(value.port.frame=="cycle" && configured->kind=="result") {require(value.cycleId==r["cycleId"].toString() && value.evaluationId==r["evaluationId"].toString(),"Result terminal identity differs");}else {require(index.has_value(),"Output without frame");index->validatePort(graph.toolSources.value(id),value,*port);}}}else{require(o["type"]=="Image" && o["imageInfo"].isObject() && index.has_value(),"Missing image evidence");const auto& f=index->frame(graph.toolSources.value(id));require(o["sourceId"]==f.sourceId && o["frameId"]==f.frameId && o["cycleId"]==f.cycleId,"Terminal image source/frame swap");}}
    }
    if(r["state"]=="DONE")require(byTool.size()==tools.size(),"DONE terminal missing tool");
    if(r["state"]=="SKIPPED")require(frames.isEmpty(),"Stop retained frame");
    for(const auto& source:graph.requiredSources){require(set.contains(source) && branches[source].isObject(),"Missing source snapshot");const auto branch=branches[source].toObject();require((branch["schemaVersion"].toInt()>=3 && branch["schemaVersion"].toInt()<=6) || (r["schemaVersion"]==9 && branch["schemaVersion"]==8),"Invalid branch schema");ProductionResultCodec::validate(branch);
        const auto configured=std::find_if(tools.cbegin(),tools.cend(),[&](const auto& t){return t.id==source;});require(configured!=tools.cend(),"Missing configured source");
        require((configured->kind=="industrial_camera")== (branch["schemaVersion"]==8),"Source branch schema differs");if(branch["schemaVersion"]==8)decodeIndustrialRuntimeDependency(*configured,branch["industrialSource"].toObject());
        require(branch["dependencies"]==set[source] && branch["cycleId"]==r["cycleId"] && branch["evaluationId"]==r["evaluationId"] && branch["state"]==r["state"] && branch["recipeId"]==r["recipeId"],"Branch immutable identity differs");
        const auto fs=branch["frames"].toArray();require(fs.size()==(bySource.contains(source) ? 1:0) && (fs.isEmpty() || fs.first().toObject()==bySource[source]),"Branch frame missing/swapped");
        QSet<QString> seen;for(const auto& v:branch["tools"].toArray()){const auto t=v.toObject();const auto id=t["toolId"].toString();require(!seen.contains(id) && graph.toolSources.value(id)==source && byTool.contains(id) && byTool[id]==t,"Branch tool missing/swapped");seen.insert(id);}
        for(auto t=byTool.begin();t!=byTool.end();++t)if(graph.toolSources.value(t.key())==source)require(seen.contains(t.key()),"Branch tool evidence missing");
    }
}
}
void ProductionResultCodec::validate(const QJsonObject& record) {
    require(QJsonDocument(record).toJson(QJsonDocument::Compact).size()<=maxRecordBytes,"Terminal result exceeds 16 MiB");
    if(record["schemaVersion"]==10){
        const int base=record["baseSchemaVersion"].toInt();require(base>=3 && base<=9 && record["baseSchemaVersion"]==base,"Invalid terminal10 base");
        const auto b=record["resultBundle"].toObject();auto content=b;content.remove("bundleId");
        require(b.size()==7 && b["schemaVersion"]==1 && b["bundleId"]==bundleHash(content) && b["cycleId"]==record["cycleId"] && b["configHash"]==record["configId"] && b["dependencyId"]==record["dependencyId"],"Terminal bundle identity/hash differs");
        require(QStringList{"OK","NOK","NOT_EVALUATED"}.contains(b["decision"].toString()) && (record["state"]=="DONE" || b["decision"]=="NOT_EVALUATED"),"Invalid bundle decision");
        CycleResult decisionEvidence;decisionEvidence.cycleId=record["cycleId"].toString();decisionEvidence.state=record["state"]=="DONE" ? ExecutionState::Done:ExecutionState::Error;
        for(const auto& item:record["tools"].toArray()){const auto t=item.toObject();ToolResult tool;tool.state=t["state"]=="DONE" ? ExecutionState::Done:ExecutionState::Error;const auto outputs=t["outputs"].toObject();for(auto o=outputs.begin();o!=outputs.end();++o)if(o->toObject().contains("typedPort"))tool.outputs[o.key()]=ToolResultCodec::decode(o->toObject()["typedPort"].toObject());decisionEvidence.tools.append(tool);}
        require(b["decision"]==resultDecisionKey(decisionEvidence.decision()),"Bundle decision differs from terminal evidence");
        require(b["groups"].isArray() && b["groups"].toArray().size()<=maxOutputGroups,"Invalid bundle groups");QSet<QString> seen;
        for(const auto& item:b["groups"].toArray()){
            const auto g=item.toObject();const auto id=g["groupId"].toString();require(g.size()==10 && !id.isEmpty() && !seen.contains(id) && g["required"].isBool() && g["resultValid"].isBool() && g["fields"].isObject() && g["fields"].toObject().size()<=maxPublicationFields,"Invalid bundle group");seen.insert(id);
            const QSet<QString> keys{"groupId","profileId","contractRevision","required","payloadSchema","intentId","profileDependencyId","resultValid","reason","fields"};for(auto i=g.begin();i!=g.end();++i)require(keys.contains(i.key()),"Unknown bundle group field");
            require(!QUuid(g["profileId"].toString()).isNull() && g["contractRevision"].toInt()>0 && !g["payloadSchema"].toString().isEmpty(),"Invalid group profile contract");
            if(!g["intentId"].toString().isEmpty())require(g["profileDependencyId"].toString().size()==64 && g["intentId"]==bundleHash({{"cycleId",record["cycleId"]},{"configId",QString::fromLatin1(record["configId"].toString().toLatin1().toHex())},{"groupId",id},{"dependencyId",g["profileDependencyId"]}}),"Intent dependency identity differs");
            if(record["definition"].isObject()){
                const auto definitions=recipeOutputGroups(record["definition"].toObject());const auto expected=std::find_if(definitions.begin(),definitions.end(),[&](const auto& d){return d.id==id;});require(expected!=definitions.end() && expected->profileId==g["profileId"].toString() && expected->contractRevision==g["contractRevision"].toInt() && expected->required==g["required"].toBool() && expected->payloadSchema==g["payloadSchema"].toString(),"Bundle group differs from recipe");
                require(g["fields"].toObject().size()==expected->fields.size(),"Bundle field set differs");for(const auto& field:expected->fields){require(g["fields"].toObject().contains(field.name),"Bundle field missing");const auto value=decodeAssignmentValue(g["fields"].toObject()[field.name].toObject());std::visit([&](const auto& v){require(v.producerId==expected->producerId && v.producerPort==field.fieldId,"Bundle producer differs");},value);}
            }
            bool valid=record["state"]=="DONE" && !g["intentId"].toString().isEmpty() && !g["fields"].toObject().isEmpty();
            const auto fields=g["fields"].toObject();for(auto f=fields.begin();f!=fields.end();++f){const auto value=decodeAssignmentValue(f->toObject());std::visit([&](const auto& v){require(v.cycleId==record["cycleId"].toString() && v.configId==QString::fromLatin1(record["configId"].toString().toLatin1().toHex()) && !v.producerId.isEmpty() && !v.producerPort.isEmpty(),"Terminal field origin differs");valid=valid && v.validity==ValueValidity::Valid;},value);}
            require(g["resultValid"].toBool()==valid && (valid ? g["reason"].toString().isEmpty():!g["reason"].toString().isEmpty()),"Invalid group cannot be normal payload");
            require(QJsonDocument(g).toJson(QJsonDocument::Compact).size()<=maxPublicationBytes,"Bundle group exceeds 64 KiB");
        }
        if(record["definition"].isObject()){QSet<QString> expected;for(const auto& t:editorTools(record["definition"].toObject()))if(t.enabled && t.kind=="publish_data")for(const auto& id:decodePublishConfig(t.parameters).groups)expected.insert(id);require(seen==expected,"Terminal bundle group set differs");}
        validate(legacyRecord(record));return;
    }
    require(!record.contains("resultBundle") && !record.contains("baseSchemaVersion"),"Bundle requires terminal10");
    const auto schema=record["schemaVersion"];
    if(schema==7 || schema==9){
        bool industrial=false;const auto set=record["dependencies"].toObject()["sourceSet"].toObject();for(const auto& v:set)industrial=industrial || v.toObject().contains("industrialSource");
        if(schema==9){const QSet<QString> keys{"configuration","schemaVersion","recordId","beginEventId","ticket","principal","recipeId","recipeName","recipeVersion","applicationType","validationId","configId","dependencyId","dependencies","definition","branches","cycleId","evaluationId","state","error","milliseconds","startedAtUtc","completedAtUtc","frames","tools","communicationState","communication"};require(record.size()==keys.size(),"Industrial multi-source fields missing/extra");for(auto i=record.begin();i!=record.end();++i)require(keys.contains(i.key()),"Unknown industrial multi-source field");}
        require(industrial==(schema==9),"Industrial source set requires terminal9");validateMulti(record);return;
    }
    if(schema==8)validateIndustrialTerminal(record);
    else require(!record.contains("industrialSource") && !record.contains("industrialAcquisition") && !record["dependencies"].toObject().contains("industrialSource"),"Industrial evidence requires terminal8");
    require(!record.contains("branches") && !record["dependencies"].toObject().contains("sourceSet"),"Source set requires terminal schema7");
    require(schema==1 || schema==2 || schema==3 || schema==4 || schema==5 || schema==6 || schema==8,"Unknown terminal result schema");
    const bool systemEnvelope=record.contains("systemCalibration");
    if(schema!=5 && schema!=6 && schema!=8)require(!record.contains("systemCalibration") && !record["dependencies"].toObject().contains("systemCalibration"),"System calibration requires terminal schema5");
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
        require(item.isObject(),"Invalid terminal tool");const auto tool=item.toObject();validateMeasurementDiagnostic(tool,record);
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
                if(v.port.type==PortType::AssignmentBundle){const auto& b=std::get<AssignmentBundle>(v.data);require(tool["kind"]=="set_variables" && b.producerId==tool["toolId"].toString() && b.configId==QString::fromLatin1(record["configId"].toString().toLatin1().toHex()),"Terminal assignment producer/configuration differs");}
                if(schema==6 && v.port.unit=="mm")require(systemEnvelope,"Physical mm result requires exact calibration evidence");
                if(schema==6 && v.port.type!=PortType::AssignmentBundle && v.port.frame!="cycle")require(v.origin.sourceId==record["measurementSource"].toObject()["binding"].toObject()["sourceToolId"].toString() && std::any_of(sourceFrames.cbegin(),sourceFrames.cend(),[&](const auto& f){return f.toObject()["frameId"]==v.frameId && f.toObject()["cycleId"]==v.cycleId;}),"Terminal source/frame differs from physical source snapshot");
                if(systemEnvelope && v.port.unit=="mm")require(tool["calibration"]==record["systemCalibration"].toObject()["executionDependency"],"Terminal millimetre output lacks system binding");
                require(v.cycleId==record["cycleId"].toString(),"Terminal port cycle mismatch");
                if(schema.toInt()>=3 && int(v.port.type)>int(PortType::Boolean))require(v.evaluationId==record["evaluationId"].toString(),"Terminal evaluation mismatch");
            }
            require(!v.diagnostic || tool["decision"]=="NOT_EVALUATED","Diagnostic cannot establish a terminal decision");
        }
    }
}
QString ProductionResultCodec::summary(const QJsonObject& record) {
    validate(record);if(record["schemaVersion"]==10)return "Bundle "+record["resultBundle"].toObject()["bundleId"].toString()+" / "+record["resultBundle"].toObject()["decision"].toString()+"\n"+summary(legacyRecord(record));QStringList lines;
    if((record["schemaVersion"]==7 || record["schemaVersion"]==9)){lines<<"Çok kaynaklı tarihsel kanıt • çalışma izni değildir";const auto branches=record["branches"].toObject();for(auto i=branches.begin();i!=branches.end();++i)lines<<"Kaynak: "+i.key()+"\n"+summary(i->toObject());return lines.join('\n');}
    if(record["schemaVersion"]==8){const auto a=record["industrialAcquisition"].toObject();lines<<acquisitionStatus(a["status"]=="ACQUIRED" ? "DONE":record["state"].toString(),a["reason"].toString()+" "+a["errorCode"].toString());lines<<presentIndustrialSource({},record["industrialSource"].toObject(),record["industrialAcquisition"].toObject()).detail;lines<<(record.contains("systemCalibration") ? "Kaynağa bağlı ölçüm modeli • karar araç bazında":"Raw / px • ölçüm doğrulanmadı");}
    for(const auto& item:record["tools"].toArray()) {
        const auto t=item.toObject();lines<<t["name"].toString(t["toolId"].toString())+" • "+t["state"].toString()+" • "+t["decision"].toString();
        if(t.contains("measurementDiagnostic"))lines<<"  "+presentMeasurementDiagnostic(decodeMeasurementDiagnostic(t["measurementDiagnostic"].toObject()));
        else if(!t["message"].toString().isEmpty())lines<<"  "+t["message"].toString();
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
    if(std::any_of(records.begin(),records.end(),[](const auto& r){return r["schemaVersion"]==10;})){QVector<QJsonObject> legacy;for(const auto& r:records){validate(r);legacy.append(r["schemaVersion"]==10 ? legacyRecord(r):r);}return csv(legacy);}
    const bool system=std::any_of(records.cbegin(),records.cend(),[](const auto& r){return r.contains("systemCalibration");});
    const bool physical=std::any_of(records.cbegin(),records.cend(),[](const auto& r){return r["schemaVersion"]==6;});
    const bool multi=std::any_of(records.cbegin(),records.cend(),[](const auto& r){return (r["schemaVersion"]==7 || r["schemaVersion"]==9);});
    const bool industrial=std::any_of(records.cbegin(),records.cend(),[](const auto& r){return r["schemaVersion"]==9;});
    QString data="cycleId,recipeId,toolId,port,type,unit,valid,diagnostic,frameId,evaluationId,method,methodVersion,value,reason,configId,validationId,dependencyId,referenceBinding"+QString(system ? ",systemCalibration":"")+QString(physical ? ",measurementSource":"")+QString(multi ? ",executionState,sourceId,sourceSet,sourceFrames":"")+QString(industrial ? ",industrialEvidence":"")+"\r\n";
    for(const auto& record:records) {
        validate(record);bool emitted=false;QJsonObject industrialEvidence;const auto branches=record["branches"].toObject();for(auto i=branches.begin();i!=branches.end();++i)if(i->toObject()["schemaVersion"]==8)industrialEvidence[i.key()]=QJsonObject{{"admission",i->toObject()["industrialSource"]},{"acquisition",i->toObject()["industrialAcquisition"]}};
        const auto industrialCell=cell(QString::fromUtf8(QJsonDocument(industrialEvidence).toJson(QJsonDocument::Compact)));
        for(const auto& item:record["tools"].toArray()) {
            const auto t=item.toObject();const auto outputs=t["outputs"].toObject();
            if(t.contains("measurementDiagnostic")){
                QStringList row;for(int j=0;j<18;++j)row<<QString{};
                row[0]=record["cycleId"].toString();row[1]=record["recipeId"].toString();row[2]=t["toolId"].toString();row[3]="measurementDiagnostic";row[4]="diagnostic/1";const auto evidence=decodeMeasurementDiagnostic(t["measurementDiagnostic"].toObject());row[8]=evidence.frameId;row[6]="false";row[7]="true";row[9]=record["evaluationId"].toString();row[13]=QString::fromUtf8(QJsonDocument(t["measurementDiagnostic"].toObject()).toJson(QJsonDocument::Compact));row[14]=record["configId"].toString();row[15]=record["validationId"].toString();row[16]=record["dependencyId"].toString();
                QStringList quoted;for(const auto& value:row)quoted<<cell(value);if(system)quoted<<cell({});if(physical)quoted<<cell({});if(multi)quoted<<cell(record["state"].toString())<<cell(evidence.sourceId)<<cell(QString::fromUtf8(QJsonDocument(record["dependencies"].toObject()["sourceSet"].toObject()).toJson(QJsonDocument::Compact)))<<cell(QString::fromUtf8(QJsonDocument(record["frames"].toArray()).toJson(QJsonDocument::Compact)));if(industrial)quoted<<industrialCell;
                data+=quoted.join(',')+"\r\n";emitted=true;
            }
            for(auto it=outputs.begin();it!=outputs.end();++it) {
                const auto o=it->toObject();const auto p=o["typedPort"].toObject();
                const QStringList row{record["cycleId"].toString(),record["recipeId"].toString(),t["toolId"].toString(),it.key(),o["type"].toString(),o["unit"].toString(),p.isEmpty() ? "legacy":p["valid"].toBool() ? "true":"false",p["diagnostic"].toBool() ? "true":"false",o["frameId"].toString(),p["evaluationId"].toString(),p["method"].toString(),QString::number(p["methodVersion"].toInt()),QString::fromUtf8(QJsonDocument(QJsonArray{o.contains("value") ? o["value"]:o["imageInfo"]}).toJson(QJsonDocument::Compact)),p["invalidReason"].toString(),record["configId"].toString(),record["validationId"].toString(),record["dependencyId"].toString(),QString::fromUtf8(QJsonDocument(record["referenceBindings"].toObject()[t["toolId"].toString()].toObject()).toJson(QJsonDocument::Compact))};
                QStringList quoted;for(const auto& s:row)quoted<<cell(s);if(system)quoted<<cell(QString::fromUtf8(QJsonDocument(record["systemCalibration"].toObject()).toJson(QJsonDocument::Compact)));if(physical)quoted<<cell(QString::fromUtf8(QJsonDocument(record["measurementSource"].toObject()).toJson(QJsonDocument::Compact)));if(multi){quoted<<cell(record["state"].toString())<<cell(o["sourceId"].toString())<<cell(QString::fromUtf8(QJsonDocument(record["dependencies"].toObject()["sourceSet"].toObject()).toJson(QJsonDocument::Compact)))<<cell(QString::fromUtf8(QJsonDocument(record["frames"].toArray()).toJson(QJsonDocument::Compact)));}if(industrial)quoted<<industrialCell;data+=quoted.join(',')+"\r\n";emitted=true;
                require(data.size()<=32*1024*1024,"CSV output exceeds limit");
            }
        }
        if(record["schemaVersion"]==9)for(auto i=industrialEvidence.begin();i!=industrialEvidence.end();++i)if(i->toObject()["acquisition"].toObject()["status"]=="NOT_ACQUIRED"){
            QStringList row;for(int j=0;j<18;++j)row<<QString{};row[0]=record["cycleId"].toString();row[1]=record["recipeId"].toString();row[2]=i.key();row[13]=i->toObject()["acquisition"].toObject()["reason"].toString();row[14]=record["configId"].toString();row[15]=record["validationId"].toString();row[16]=record["dependencyId"].toString();
            QStringList quoted;for(const auto& v:row)quoted<<cell(v);if(system)quoted<<cell({});if(physical)quoted<<cell({});
            quoted<<cell(record["state"].toString())<<cell(i.key())<<cell(QString::fromUtf8(QJsonDocument(record["dependencies"].toObject()["sourceSet"].toObject()).toJson(QJsonDocument::Compact)))<<cell(QString::fromUtf8(QJsonDocument(record["frames"].toArray()).toJson(QJsonDocument::Compact)))<<industrialCell;data+=quoted.join(',')+"\r\n";emitted=true;
        }
        // Preserve terminal7 failure/Stop evidence even when there is no output row.
        if((record["schemaVersion"]==7 || record["schemaVersion"]==9) && !emitted){
            QStringList row;for(int i=0;i<18;++i)row<<QString{};row[0]=record["cycleId"].toString();row[1]=record["recipeId"].toString();row[13]=record["error"].toString();row[14]=record["configId"].toString();row[15]=record["validationId"].toString();row[16]=record["dependencyId"].toString();
            QStringList quoted;for(const auto& value:row)quoted<<cell(value);if(system)quoted<<cell(QString{});if(physical)quoted<<cell(QString{});
            quoted<<cell(record["state"].toString())<<cell(QString{})<<cell(QString::fromUtf8(QJsonDocument(record["dependencies"].toObject()["sourceSet"].toObject()).toJson(QJsonDocument::Compact)))<<cell(QString::fromUtf8(QJsonDocument(record["frames"].toArray()).toJson(QJsonDocument::Compact)));if(industrial)quoted<<industrialCell;data+=quoted.join(',')+"\r\n";require(data.size()<=32*1024*1024,"CSV output exceeds limit");
        }
        require(data.size()<=32*1024*1024,"CSV output exceeds limit");
    }return data.toUtf8();
}
}
