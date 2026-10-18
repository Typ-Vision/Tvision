#include "IndustrialRuntime.hpp"
#include <QJsonDocument>
#include <QSet>
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
}
