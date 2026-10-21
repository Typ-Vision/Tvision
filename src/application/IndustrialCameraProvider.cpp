#include "IndustrialCameraProvider.hpp"
#include "WebcamAdapter.hpp"
#include <stdexcept>
namespace visionflow {
std::unique_ptr<AcquisitionAdapter> CameraProviderFactory::webcam(const MeasurementCamera& camera,
    const QVector<DeviceBinding>& discovered,QSize resolution) const {
    camera.validate();if(camera.physical || camera.device.adapter!="webcam")throw std::runtime_error("Expected registered webcam.");
    const auto index=camera.device.resolve(discovered);
    WebcamSettings settings;settings.providerId=discovered[index].providerId;settings.resolution=resolution;
    return std::make_unique<WebcamAdapter>(settings);
}
namespace {
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
class CheckedBackend final : public CameraWorkerBackend {
public:
    CheckedBackend(std::shared_ptr<const IndustrialCameraProvider> provider,IndustrialCameraRequest request,bool configurationOnly=false)
        :request_(std::move(request)),backend_(provider->create(request_)),configurationOnly_(configurationOnly) {
        require(bool(backend_),"Industrial backend missing.");
    }
    CameraWorkerResult execute(const CameraTicket& ticket) override {
        require(!configurationOnly_ || (ticket.operation!=CameraOperation::Start && ticket.operation!=CameraOperation::Grab),
                "Configuration-only session cannot start acquisition.");
        if(ticket.operation==CameraOperation::Grab)checkApplied();
        auto result=backend_->execute(ticket);
        if(!result.success)return result;
        if(ticket.operation==CameraOperation::Connect)
            require(backend_->actualIdentity()==request_.device.identity,"Actual camera identity changed.");
        if(ticket.operation==CameraOperation::Configure || ticket.operation==CameraOperation::Grab){
            result.readback=checkApplied();
        }
        if(ticket.operation==CameraOperation::Grab){
            require(result.frame.has_value(),"Industrial grab has no owned frame.");
            const auto& f=*result.frame;
            require(f.image.metadata().experimentalAcquisition.isEmpty(),"Experimental frame cannot enter admitted acquisition.");
            require(f.sourceId==request_.sourceId && f.image.size()==request_.setup.geometry.resolution &&
                    f.image.format()==PixelFormat::Gray8,"Industrial frame source/geometry/format mismatch.");
        }
        return result;
    }
    void shutdown() override {backend_->shutdown();}
private:
    AcquisitionReadback checkApplied() const {
        require(backend_->actualIdentity()==request_.device.identity,"Actual camera identity changed.");
        const auto actual=backend_->readback();
        verifyApplied(request_.setup,request_.profile,actual);return actual;
    }
    IndustrialCameraRequest request_;
    std::unique_ptr<IndustrialCameraBackend> backend_;
    bool configurationOnly_=false;
};
}
void IndustrialCameraRequest::validate() const {
    camera.validate();setup.validate();profile.validate();device.validate();
    require(camera.physical && camera.physical->usable() && *camera.physical==device.identity,
            "Exact registered industrial identity required.");
    require(setup.camera==camera.id && setup.deviceRevision==camera.device.revision &&
            profile.camera==camera.id && profile.setup==setup.id,"Camera/setup/profile snapshot mismatch.");
    require(setup.geometry.complete && setup.geometry.critical() && !sourceId.trimmed().isEmpty(),
            "Complete industrial geometry/source required.");
}
CameraProviderFactory CameraProviderFactory::forTests(std::shared_ptr<const IndustrialCameraProvider> provider){
    require(bool(provider),"Test provider missing.");CameraProviderFactory result;result.injected_=std::move(provider);return result;
}
std::unique_ptr<CameraWorkerSession> CameraProviderFactory::resolveOpen(IndustrialCameraRequest request,
    const IndustrialDiscoverySnapshot& discovery,const std::optional<TransportLocator>& route,
    CameraLeaseUse use,FrameQueueMode mode,size_t capacity) const {
    request.device=CameraService::resolveIndustrial(request.camera,discovery,route);
    return open(request,use,mode,capacity);
}
IndustrialCameraCapabilities CameraProviderFactory::capabilities(const DiscoveredDevice& device) const {
    device.validate();if(injected_)return injected_->capabilities(device);
    return {false,false,false,false,false,false,"Native backend edinimi doğrulanmadı / SDK adapter'ı kullanılamıyor."};
}
std::unique_ptr<CameraWorkerSession> CameraProviderFactory::configure(const IndustrialCameraRequest& request,CameraLeaseUse use) const {
    request.validate();const auto caps=capabilities(request.device);
    require(injected_ && caps.open && caps.configure,"Industrial configuration backend unavailable.");
    return std::make_unique<CameraWorkerSession>(request.device.identity,use,request.sourceId,FrameQueueMode::ProductionFifo,
        [provider=injected_,request]{return std::make_unique<CheckedBackend>(provider,request,true);});
}
std::unique_ptr<CameraWorkerSession> CameraProviderFactory::open(const IndustrialCameraRequest& request,
    CameraLeaseUse use,FrameQueueMode mode,size_t capacity) const {
    request.validate();const auto caps=capabilities(request.device);
    require(injected_ && caps.open && caps.configure,"Industrial backend unavailable; no admitted native factory.");
    require(mode==FrameQueueMode::PreviewLatest ? caps.preview : caps.softwareTrigger,
            "Industrial preview/software trigger capability unavailable.");
    require(mode==FrameQueueMode::PreviewLatest ? !request.profile.requested.trigger.enabled :
            (request.profile.requested.trigger.enabled && request.profile.requested.trigger.source=="Software"),
            "Preview and software capture require separate profile/session admission.");
    return std::make_unique<CameraWorkerSession>(request.device.identity,use,request.sourceId,mode,
        [provider=injected_,request]{return std::make_unique<CheckedBackend>(provider,request);},capacity);
}
}
