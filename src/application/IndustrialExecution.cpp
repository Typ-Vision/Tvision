#include "IndustrialExecution.hpp"
#include "ExperimentalOecsAcquisition.hpp"
#include <QThread>
#include <stdexcept>

namespace visionflow {
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
ExecutionService::ExecutionService(const IndustrialExecutionInputs& inputs,SourceAdapterFactory factory)
    :sourceAdapterFactory_(std::move(factory)),industrial_(std::make_shared<const IndustrialExecutionInputs>(inputs)){}
IndustrialCameraRequest industrialExecutionRequest(const IndustrialExecutionInputs& inputs,const ToolDefinition& tool){
    const auto ref=IndustrialSourceReference::fromJson(tool.parameters["sourceReference"].toObject());
    ref.validateAgainst(inputs.contract);
    IndustrialCameraRequest request;request.sourceId=tool.id;
    for(const auto& c:inputs.contract.cameras)if(c.id==ref.camera)request.camera=c;
    for(const auto& s:inputs.contract.setups)if(s.id==ref.setup)request.setup=s;
    for(const auto& p:inputs.contract.profiles)if(p.id==ref.profile)request.profile=p;
    return request;
}
IndustrialExecutionFrame acquireIndustrial(const IndustrialExecutionInputs& inputs,IndustrialCameraRequest request,
    const QString& cycle,const QString& frame,int timeout,const std::shared_ptr<std::atomic_bool>& cancel,bool experimentalEditor){
    const auto inherited=AcquisitionTiming::current();
    AcquisitionTimingBinding timingBinding(inherited.request.isEmpty()?AcquisitionTiming::request(inputs.leaseUse==CameraLeaseUse::Runtime?"runtime":"editor"):inherited);
    AcquisitionTimingSpan timing("industrial.acquire");
    require(!experimentalEditor || (inputs.leaseUse==CameraLeaseUse::Editor && inputs.admittedDevices.isEmpty()),"Experimental acquisition requires Editor lease without production admission");
    Deadline deadline(timeout);
    auto live=[&]{require(!(cancel && cancel->load()) && !deadline.expired(),"Industrial execution cancelled/deadline exceeded");};
    live();
    std::optional<TransportLocator> route;
    if(inputs.routes.contains(request.camera.id.value))route=inputs.routes[request.camera.id.value];
    AcquisitionTimingSpan discoveryTiming("discovery.resolve");
    const auto discovery=inputs.discover ? inputs.discover(deadline.remainingMs(),cancel):
        CameraService::industrialSnapshot(CameraService{}.discover(route && !route->producer.isEmpty() ? QStringList{route->producer}:QStringList{},deadline.remainingMs(),cancel));
    live();request.device=CameraService::resolveIndustrial(request.camera,discovery,route);
    discoveryTiming.finish("success",{{"route",request.device.route.toJson()},{"backend",request.device.backend.toJson()}});
    if(inputs.admittedDevices.contains(request.sourceId))require(request.device==inputs.admittedDevices[request.sourceId],"Industrial Runtime device/backend changed; prepare again");
    auto session=experimentalEditor ? ExperimentalOecsAcquisition::open(request,ExperimentalOecsAcquisition::installedRuntime()):inputs.factory.open(request,inputs.leaseUse,FrameQueueMode::ProductionFifo,1);
    try {
        auto command=[&](CameraOperation op,CameraState expected){
            live();session->submit(op,deadline.remainingMs());
            for(;;){
                live();session->poll();
                require(session->lifecycle().state()!=CameraState::Fault,session->lifecycle().error().toUtf8().constData());
                if(session->lifecycle().state()==expected)break;
                QThread::msleep(1);
            }
        };
        command(CameraOperation::Connect,CameraState::Connected);
        command(CameraOperation::Configure,CameraState::Configured);
        command(CameraOperation::Start,CameraState::Streaming);
        command(CameraOperation::Grab,CameraState::Streaming);
        live();const auto captured=session->take();const auto applied=session->readback();
        require(captured && applied,"Industrial execution missing accepted frame/readback");
        const auto& image=captured->image;const auto original=image.metadata();
        require(original.sourceId==request.sourceId && !original.frameId.isEmpty() && !original.cycleId.isEmpty() &&
            original.completedAtUtc.isValid() && image.processing().isEmpty(),"Invalid industrial raw frame identity");
        auto mapped=original;mapped.frameId=frame;mapped.cycleId=cycle;mapped.coordinateSystemId=request.sourceId+"/image";
        if(experimentalEditor){mapped.experimentalAcquisition["contentId"]=QString::fromLatin1(image.contentId().toHex());mapped.experimentalAcquisition["frameId"]=original.frameId;mapped.experimentalAcquisition["sourceId"]=original.sourceId;mapped.experimentalAcquisition["completedAtUtc"]=original.completedAtUtc.toString(Qt::ISODateWithMs);}
        require(experimentalEditor==!original.experimentalAcquisition.isEmpty(),"Industrial acquisition policy/provenance mismatch");
        auto evidence=experimentalEditor ? std::shared_ptr<const IndustrialExecutionEvidence>{}:std::make_shared<const IndustrialExecutionEvidence>(IndustrialExecutionEvidence{request,*applied,original,mapped,image.contentId()});
        // Cleanup and lease release precede publication. A delayed SDK cannot publish after Stop.
        session.reset();live();timing.finish("success",{{"sourceId",request.sourceId}});return {image.withMetadata(mapped),std::move(evidence)};
    }catch(...){timing.finish(cancel && cancel->load()?"cancelled":deadline.expired()?"timeout":"error");if(session)session->requestStop();throw;}
}
}
