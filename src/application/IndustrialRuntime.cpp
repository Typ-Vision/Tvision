#include "IndustrialRuntime.hpp"
#include "SystemCalibration.hpp"
#include <QJsonDocument>
#include <QSet>
#include <QJsonArray>
#include <QCryptographicHash>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}}
QJsonObject industrialRuntimeDependency(const IndustrialCameraRequest& r,const IndustrialSourceReference& ref){
    r.validate();return {{"schemaVersion",1},{"sourceToolId",r.sourceId},{"reference",ref.toJson()},
        {"camera",r.camera.toJson()},{"setup",r.setup.toJson()},{"profile",r.profile.toJson()},
        {"physical",r.device.identity.toJson()},{"backend",r.device.backend.toJson()},{"route",r.device.route.toJson()}};
}
IndustrialCameraRequest decodeIndustrialRuntimeDependency(const ToolDefinition& t,const QJsonObject& o){
    require(t.kind=="industrial_camera" && o["reference"]==t.parameters["sourceReference"],"Industrial admission source reference mismatch");
    IndustrialCameraRequest r;r.sourceId=t.id;r.camera=MeasurementCamera::fromJson(o["camera"].toObject());
    r.setup=MeasurementSetupDescriptor::fromJson(o["setup"].toObject());r.profile=AcquisitionProfile::fromJson(o["profile"].toObject());
    r.device={PhysicalDeviceIdentity::fromJson(o["physical"].toObject()),TransportLocator::fromJson(o["route"].toObject()),BackendIdentity::fromJson(o["backend"].toObject())};
    const auto ref=IndustrialSourceReference::fromJson(o["reference"].toObject());
    ref.validateAgainst({{r.camera},{r.setup},{r.profile}});r.validate();
    require(o==industrialRuntimeDependency(r,ref),"Industrial admission missing/unknown fields");return r;
}
IndustrialRuntimeAdmission freezeIndustrialRuntime(const IndustrialExecutionInputs& original,const QVector<ToolDefinition>& tools){
    IndustrialRuntimeAdmission result;result.inputs=original;result.inputs.admittedDevices.clear();result.inputs.leaseUse=CameraLeaseUse::Runtime;
    QSet<QByteArray> physical;
    for(const auto& t:tools)if(t.enabled && t.kind=="industrial_camera"){
        auto r=industrialExecutionRequest(result.inputs,t);const auto ref=IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject());
        const auto key=QJsonDocument(r.camera.physical->toJson()).toJson(QJsonDocument::Compact);
        require(!physical.contains(key),"Repeated physical industrial source");physical.insert(key);
        std::optional<TransportLocator> route;if(original.routes.contains(r.camera.id.value))route=original.routes[r.camera.id.value];
        const auto scan=original.discover ? original.discover(ref.timeoutMs,{}) : CameraService::industrialSnapshot(CameraService{}.discover(route && !route->producer.isEmpty() ? QStringList{route->producer}:QStringList{},ref.timeoutMs));
        r.device=CameraService::resolveIndustrial(r.camera,scan,route);r.validate();
        const auto caps=original.factory.capabilities(r.device);
        require(caps.open && caps.configure && caps.softwareTrigger,"Industrial Runtime backend capability unavailable");
        result.inputs.admittedDevices[t.id]=r.device;result.inputs.routes[r.camera.id.value]=r.device.route;
        result.sources[t.id]=industrialRuntimeDependency(r,ref);
    }return result;
}
namespace {
QJsonObject frameEvidence(const FrameMetadata& f){return {{"sourceId",f.sourceId},{"frameId",f.frameId},{"cycleId",f.cycleId},{"coordinateSystemId",f.coordinateSystemId},{"completedAtUtc",f.completedAtUtc.toString(Qt::ISODateWithMs)},
    {"exposureAtUtc",f.exposureAtUtc ? QJsonValue(f.exposureAtUtc->toString(Qt::ISODateWithMs)):QJsonValue(QJsonValue::Null)},
    {"deviceTimestampTicks",f.deviceTimestampTicks ? QJsonValue(QString::number(*f.deviceTimestampTicks)):QJsonValue(QJsonValue::Null)},{"deviceClockId",f.deviceClockId}};}
}
QJsonObject industrialCompletionEvidence(const IndustrialExecutionEvidence& e){
    e.request.validate();return {{"schemaVersion",1},{"status","ACQUIRED"},{"sourceToolId",e.request.sourceId},
        {"camera",e.request.camera.toJson()},{"setup",e.request.setup.toJson()},{"profile",e.request.profile.toJson()},
        {"physical",e.request.device.identity.toJson()},{"backend",e.request.device.backend.toJson()},{"route",e.request.device.route.toJson()},
        {"actual",e.actual.toJson()},{"acquired",frameEvidence(e.acquired)},{"execution",frameEvidence(e.execution)},{"pixelHash",QString::fromLatin1(e.pixelHash.toHex())}};
}
void validateIndustrialTerminal(const QJsonObject& r){
    QSet<QString> keys{"schemaVersion","evaluationId","recordId","beginEventId","ticket","principal","recipeId","recipeName","applicationType","recipeVersion","validationId","configId","dependencyId","dependencies","startedAtUtc","completedAtUtc","cycleId","milliseconds","state","error","frames","tools","communicationState","communication","industrialSource","industrialAcquisition"};
    if(r["dependencies"].toObject().contains("systemCalibration"))keys.insert("systemCalibration");if(r["dependencies"].toObject().contains("referenceBindings"))keys.insert("referenceBindings");
    require(r.size()==keys.size(),"Industrial terminal fields missing/extra");for(auto i=r.begin();i!=r.end();++i)require(keys.contains(i.key()),"Unknown industrial terminal field");
    const auto deps=r["dependencies"].toObject(), admission=deps["industrialSource"].toObject(), proof=r["industrialAcquisition"].toObject();
    require(r["industrialSource"]==admission && !admission.isEmpty(),"Missing industrial admission");
    const auto source=admission["sourceToolId"].toString();ToolDefinition tool;tool.id=source;tool.kind="industrial_camera";tool.parameters={{"sourceReference",admission["reference"]}};
    const auto request=decodeIndustrialRuntimeDependency(tool,admission);
    require(r["dependencyId"].toString()==QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(deps).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex()),"Industrial dependency hash differs");
    require(proof["schemaVersion"]==1 && proof["sourceToolId"]==source,"Invalid industrial acquisition identity");
    const auto frames=r["frames"].toArray();require(frames.size()<=1,"Industrial frame count");
    if(proof["status"]=="ACQUIRED"){
        require(proof.size()==13 && r["state"]!="SKIPPED" && frames.size()==1,"Invalid acquired evidence envelope");
        for(const auto key:{"camera","setup","profile","physical","backend","route"})require(proof[key]==admission[key],"Actual industrial request differs from admission");
        const auto actual=AcquisitionReadback::fromJson(proof["actual"].toObject());require(verifyApplied(request.setup,request.profile,actual)==request.profile.acquisitionId(request.setup),"Actual industrial profile/geometry differs");
        const auto acquired=proof["acquired"].toObject(), execution=proof["execution"].toObject(), frame=frames.first().toObject();
        auto decodeFrame=[](const QJsonObject& o){FrameMetadata f;f.sourceId=o["sourceId"].toString();f.frameId=o["frameId"].toString();f.cycleId=o["cycleId"].toString();f.coordinateSystemId=o["coordinateSystemId"].toString();f.completedAtUtc=QDateTime::fromString(o["completedAtUtc"].toString(),Qt::ISODateWithMs);
            if(!o["exposureAtUtc"].isNull()){f.exposureAtUtc=QDateTime::fromString(o["exposureAtUtc"].toString(),Qt::ISODateWithMs);require(f.exposureAtUtc->isValid(),"Invalid exposure timestamp");}
            if(!o["deviceTimestampTicks"].isNull()){bool ok=false;f.deviceTimestampTicks=o["deviceTimestampTicks"].toString().toULongLong(&ok);require(ok,"Invalid device timestamp");}f.deviceClockId=o["deviceClockId"].toString();require(!f.coordinateSystemId.isEmpty() && f.completedAtUtc.isValid() && o==frameEvidence(f),"Invalid exact frame evidence");return f;};
        auto mapped=decodeFrame(acquired);decodeFrame(execution);mapped.frameId=execution["frameId"].toString();mapped.cycleId=execution["cycleId"].toString();mapped.coordinateSystemId=source+"/image";require(frameEvidence(mapped)==execution,"Backend metadata mapping differs");
        require(acquired.size()==8 && execution.size()==8 && acquired["sourceId"]==source && execution["sourceId"]==source && !acquired["frameId"].toString().isEmpty() && !acquired["cycleId"].toString().isEmpty() && QDateTime::fromString(acquired["completedAtUtc"].toString(),Qt::ISODateWithMs).isValid(),"Invalid backend frame identity");
        for(const auto key:{"frameId","cycleId","sourceId","coordinateSystemId","completedAtUtc"})require(execution[key]==frame[key],"Industrial execution frame swapped");
        require(execution["cycleId"]==r["cycleId"] && execution["completedAtUtc"]==acquired["completedAtUtc"],"Industrial cycle/timestamp differs");
        const auto pixel=proof["pixelHash"].toString();require(pixel.size()==64 && QString::fromLatin1(QByteArray::fromHex(pixel.toLatin1()).toHex())==pixel,"Invalid industrial pixel hash");
        bool found=false;for(const auto& item:r["tools"].toArray()){const auto t=item.toObject();if(t["toolId"]!=source)continue;
            require(t["kind"]=="industrial_camera" && t["state"]=="DONE","Acquired source not DONE");const auto output=t["outputs"].toObject()["image"].toObject(), image=output["imageInfo"].toObject();
            require(output["sourceId"]==source && output["frameId"]==execution["frameId"] && output["cycleId"]==r["cycleId"] && image["width"]==actual.geometry.resolution.width() && image["height"]==actual.geometry.resolution.height() && image["contentId"]==pixel && image.size()==4,"Industrial raster differs");found=true;}
        require(found,"Missing industrial source image");
    }else require(proof==QJsonObject{{"schemaVersion",1},{"sourceToolId",source},{"status","NOT_ACQUIRED"},{"reason",proof["reason"]},{"errorCode",proof["errorCode"]}} && proof["reason"].isString() && proof["errorCode"].isString() && frames.isEmpty() && r["state"]!="DONE","Failure invented industrial frame");
    if(proof["status"]=="NOT_ACQUIRED")for(const auto& v:r["tools"].toArray()){const auto t=v.toObject();require(t["outputs"].toObject().isEmpty() && t["decision"]=="NOT_EVALUATED" && t["calibration"].isNull(),"Unacquired branch carried result");}
    if(deps.contains("systemCalibration")){const auto model=SystemCalibration::decodeEvidence(deps["systemCalibration"].toObject());const auto scope=model.scope;
        require(scope["schemaVersion"]==2 && scope["physical"]==admission["physical"] && scope["device"]==request.camera.device.toJson() && scope["geometryId"]==request.setup.geometryId() && scope["target"]==SetupReference{request.camera.id,request.setup.id,request.setup.revision}.toJson() && scope["profile"]==admission["profile"],"Industrial terminal calibration scope differs");}
}


}
