#include "IndependentCalibrationCapture.hpp"
#include "WebcamAdapter.hpp"
#include "MeasurementEnvironment.hpp"
#include <QFileInfo>
#include <QUuid>
#include <limits>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
}
IndependentCaptureRequest IndependentCaptureRequest::fromDefaults(const MeasurementEnvironment& e,const CameraId& id,CalibrationCapturePolicy policy,QString path){
    const auto defaults=e.defaults(id);require(defaults.status==CameraDefaultsStatus::Ready && defaults.setup.has_value(),"Ready unambiguous camera defaults required");
    IndependentCaptureRequest r;r.origin=path.isEmpty() ? SourceOrigin::CameraAcquired:SourceOrigin::SetupFile;r.filePath=std::move(path);r.policy=policy;
    for(const auto& c:e.current.cameras)if(c.id==id)r.camera=c;
    for(const auto& s:e.current.setups)if(s.id==*defaults.setup && s.camera==id)r.setup=s;
    if(r.camera && r.camera->physical && defaults.profile)for(const auto& p:e.current.profiles)if(p.id==*defaults.profile)r.profile=p;
    r.validate();return r;
}
void IndependentCaptureRequest::validate() const {
    require(timeoutMs>0 && timeoutMs<=60000,"Capture timeout must be 1–60000 ms");
    require(origin==SourceOrigin::CameraAcquired || origin==SourceOrigin::SetupFile || origin==SourceOrigin::TrialFile,"Unsupported acquisition origin");
    require(policy==CalibrationCapturePolicy::Trusted || policy==CalibrationCapturePolicy::ExperimentalOecs,"Unknown calibration capture policy");
    if(policy==CalibrationCapturePolicy::ExperimentalOecs)require(camera && camera->physical && setup && profile && origin!=SourceOrigin::TrialFile,"Experimental capture requires frozen camera/setup/profile target");
    if(origin==SourceOrigin::TrialFile)require(!camera && !setup && !profile && !localRoute,"Trial file cannot declare a physical target");
    else {
        require(camera.has_value() && setup.has_value(),"Capture requires exact camera/setup snapshot");MeasurementSetupContract{{*camera},{*setup},profile ? QVector<AcquisitionProfile>{*profile}:QVector<AcquisitionProfile>{}}.validate();require(setup->geometry.complete,"Critical geometry unknown");
        installation(*camera,*setup);
        if(camera->physical && origin==SourceOrigin::CameraAcquired)require(profile && profile->requested.trigger.enabled && profile->requested.trigger.source=="Software","Industrial capture requires exact software trigger profile");
        if(!camera->physical)require(!profile && !localRoute,"Webcam cannot carry industrial configuration");
    }
    if(origin==SourceOrigin::CameraAcquired)require(filePath.isEmpty() && (camera->physical || camera->device.capabilities.softwareCapture),"Camera capture cannot be file-backed");
    else require(QFileInfo(filePath).isAbsolute() && filePath.size()<=32768 && !filePath.contains(QChar::Null),"Absolute capture file path required");
}
MeasurementSetup IndependentCaptureRequest::installation(const MeasurementCamera& camera,const MeasurementSetupDescriptor& setup){
    MeasurementSetupContract{{camera},{setup}}.validate();
    require(setup.geometry.complete,"Critical geometry unknown");
    if(camera.physical)require(camera.physical->usable() && setup.geometry.critical(),"Exact industrial identity/geometry required");
    else require(camera.device.adapter=="webcam" && !setup.geometry.sensorRoi && !setup.geometry.binning && camera.device.confidence!=DeviceIdentityConfidence::Unavailable,"Unsupported capture adapter/geometry");
    QJsonObject config{{"geometryId",setup.geometryId()},{"device",camera.device.toJson()}};
    if(camera.physical)config["physical"]=camera.physical->toJson();
    return {setup.id.value+"/"+QString::number(setup.revision),camera.device.adapter,CalibrationWizardSession::configurationId(config),setup.opticalProfile,setup.workingPlane,setup.geometry.resolution,{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{setup.workingPlane+"/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
}
IndependentCalibrationSession::IndependentCalibrationSession():thread_(QThread::currentThread()),sourceId_("calibration-session:"+id()){}
IndependentCalibrationSession::~IndependentCalibrationSession(){if(cancel_)cancel_->store(true);}
void IndependentCalibrationSession::owner() const {require(thread_==QThread::currentThread(),"Calibration session owner thread required");}
void IndependentCalibrationSession::cancel(){owner();if(cancel_)cancel_->store(true);ticket_.clear();cancel_.reset();evidence_.reset();acceptedSetup_.reset();profile_.reset();device_.reset();math_.clear();}
bool IndependentCalibrationSession::busy() const {owner();return !ticket_.isEmpty();}
std::optional<MeasurementSourceEvidence> IndependentCalibrationSession::evidence() const {owner();return evidence_;}
std::optional<MeasurementSetup> IndependentCalibrationSession::acceptedSetup() const {owner();return acceptedSetup_;}
std::optional<AcquisitionProfile> IndependentCalibrationSession::acceptedProfile() const {owner();return profile_;}
std::optional<DiscoveredDevice> IndependentCalibrationSession::acceptedDevice() const {owner();return device_;}
CalibrationWizardSession& IndependentCalibrationSession::math(){owner();return math_;}
IndependentCaptureJob IndependentCalibrationSession::begin(IndependentCaptureRequest request){
    owner();cancel();request.validate();require(generation_<1000000000,"Capture generation exhausted");IndependentCaptureJob job;job.request_=std::move(request);job.sourceId_=sourceId_;job.ticket_=id();job.generation_=++generation_;job.cancel_=std::make_shared<std::atomic_bool>(false);job.deadline_=Deadline(job.request_.timeoutMs);ticket_=job.ticket_;cancel_=job.cancel_;return job;
}
IndependentCaptureResult IndependentCalibrationSession::run(const IndependentCaptureJob& job,const IndependentCaptureBackend& backend){
    IndependentCaptureResult result;result.ticket_=job.ticket_;result.generation_=job.generation_;
    try {
        const auto& r=job.request_;r.validate();require(!job.cancel_->load() && !job.deadline_.expired(),"Capture cancelled/expired before acquisition");
        std::unique_ptr<AcquisitionAdapter> adapter;
        if(r.origin==SourceOrigin::CameraAcquired && !r.camera->physical){
            auto discovered=backend.discover ? backend.discover():WebcamAdapter::discover();const auto index=r.camera->device.resolve(discovered);const auto actual=discovered[index];
            if(backend.create)adapter=backend.create(actual,r.setup->geometry);
            else adapter=std::make_unique<WebcamAdapter>(WebcamSettings{0,"qt",actual.providerId,r.setup->geometry.resolution});
            require(adapter && adapter->capabilities().sourceKind=="webcam","Unsupported actual camera adapter");
        }else if(r.origin!=SourceOrigin::CameraAcquired)adapter=std::make_unique<ImageFileAdapter>(r.filePath,r.policy==CalibrationCapturePolicy::ExperimentalOecs);
        const auto source=r.origin==SourceOrigin::CameraAcquired ? job.sourceId_:"calibration-test:"+job.sourceId_;
        auto image=[&]() -> ImageFrame {
            if(adapter){
                AcquisitionRequest request{source,id(),id(),TriggerMode::Software,r.timeoutMs,job.cancel_,job.deadline_};
                auto acquired=AcquisitionService{}.acquire(*adapter,request);
                require(acquired.state==AcquisitionState::Done && acquired.frame.has_value(),acquired.error.toUtf8().constData());
                return acquired.frame->image;
            }
            const auto discovery=backend.industrialDiscover ? backend.industrialDiscover():CameraService::industrialSnapshot(CameraService{}.discover(r.localRoute && !r.localRoute->producer.isEmpty() ? QStringList{r.localRoute->producer}:QStringList{},job.deadline_.remainingMs(),job.cancel_));
            require(!job.cancel_->load() && !job.deadline_.expired(),"Industrial discovery cancelled/expired");
            IndustrialCameraRequest request{*r.camera,*r.setup,*r.profile,{},source};
            request.device=CameraService::resolveIndustrial(*r.camera,discovery,r.localRoute);
            auto session=r.policy==CalibrationCapturePolicy::ExperimentalOecs ? ExperimentalOecsAcquisition::openCalibration(request,ExperimentalOecsAcquisition::installedRuntime(),backend.experimentalApi):backend.factory.open(request,CameraLeaseUse::Calibration,FrameQueueMode::ProductionFifo,1);
            auto command=[&](CameraOperation op,CameraState expected){
                require(!job.cancel_->load() && !job.deadline_.expired(),"Industrial capture cancelled/expired");
                session->submit(op,job.deadline_.remainingMs());
                for(;;){
                    session->poll();
                    require(!job.cancel_->load() && !job.deadline_.expired(),"Industrial capture cancelled/expired");
                    require(session->lifecycle().state()!=CameraState::Fault,session->lifecycle().error().toUtf8().constData());
                    if(session->lifecycle().state()==expected)break;
                    QThread::msleep(1);
                }
            };
            command(CameraOperation::Connect,CameraState::Connected);
            command(CameraOperation::Configure,CameraState::Configured);
            command(CameraOperation::Start,CameraState::Streaming);
            command(CameraOperation::Grab,CameraState::Streaming);
            auto frame=session->take();require(frame.has_value(),"Industrial capture has no accepted frame");
            require(frame->image.metadata().sourceId==source,"Industrial frame metadata source mismatch");
            result.profile_=r.profile;result.device_=request.device;
            return frame->image; // owned raster survives worker shutdown and lease release
        }();
        require(image.processing().isEmpty(),"Only raw acquisition accepted");
        auto meta=image.metadata();meta.coordinateSystemId="image";
        if(r.policy==CalibrationCapturePolicy::ExperimentalOecs && r.origin==SourceOrigin::CameraAcquired){
            meta.experimentalAcquisition["contentId"]=QString::fromLatin1(image.contentId().toHex());
            meta.experimentalAcquisition["frameId"]=meta.frameId;meta.experimentalAcquisition["sourceId"]=meta.sourceId;
            meta.experimentalAcquisition["completedAtUtc"]=meta.completedAtUtc.toString(Qt::ISODateWithMs);
        }
        image=image.withMetadata(meta);
        MeasurementSourceEvidence evidence{r.origin,meta.sourceId,meta.frameId,meta.cycleId,meta.coordinateSystemId,QString::fromLatin1(image.contentId().toHex()),r.filePath,job.generation_,{},SourceIdentityKind::CalibrationSession};
        MeasurementSetup mathSetup;
        if(r.policy==CalibrationCapturePolicy::ExperimentalOecs){
            ExperimentalCalibrationEvidence proof{r.origin,*r.camera,*r.setup,*r.profile,meta.experimentalAcquisition};proof.verify(image);evidence.experimental=proof;
            result.profile_=r.profile;
        }else require(meta.experimentalAcquisition.isEmpty(),"Experimental raster requires calibration opt-in");
        if(r.setup){
            require(image.size()==r.setup->geometry.resolution,"Actual frame/setup geometry mismatch");evidence.target=SetupReference{r.camera->id,r.setup->id,r.setup->revision};
            // Path is evidence only. Replacing a setup file never changes physical identity.
            mathSetup=IndependentCaptureRequest::installation(*r.camera,*r.setup);
        }else mathSetup={job.sourceId_,"image_file",CalibrationWizardSession::configurationId({{"trialSession",job.sourceId_}}),"trial-lens","trial-plane",image.size(),{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{job.sourceId_+"/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
        evidence.validate();mathSetup.validate();require(!job.cancel_->load() && !job.deadline_.expired(),"Capture cancelled/expired after acquisition");result.frame_=image;result.evidence_=evidence;result.mathSetup_=mathSetup;
    }catch(const std::exception& e){result.error_=QString::fromUtf8(e.what());}
    return result;
}
bool IndependentCalibrationSession::accept(const IndependentCaptureResult& result){
    owner();if(ticket_.isEmpty() || ticket_!=result.ticket_ || generation_!=result.generation_ || !cancel_)return false;
    const bool cancelled=cancel_->load();ticket_.clear();cancel_.reset();evidence_.reset();acceptedSetup_.reset();profile_.reset();device_.reset();math_.clear();if(cancelled || !result.error_.isEmpty() || !result.frame_ || !result.evidence_ || !result.mathSetup_)return false;
    const auto origin=result.evidence_->origin;math_.useFrame(*result.frame_,*result.mathSetup_,origin==SourceOrigin::CameraAcquired ? "acquired":sourceOriginName(origin),result.evidence_->experimental);evidence_=result.evidence_;acceptedSetup_=result.mathSetup_;profile_=result.profile_;device_=result.device_;return true;
}
}
