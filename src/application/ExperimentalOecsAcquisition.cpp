#include "ExperimentalOecsAcquisition.hpp"
#include "IndustrialCameraConfiguration.hpp"
#include "vision/ImageAccess.hpp"
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QUuid>

namespace visionflow {
namespace {
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
bool supported(const PhysicalDeviceIdentity& id){return id.vendor=="Opto Engineering Srl" && id.model=="COE-050-M-USB-050-IR-C" && id.usable();}
// Only the all-disabled metadata layout is admitted in EXP-01. No pixel erasure.
void verifyCleanRaster(OecsSession& session){
    const auto selector=session.feature("FrameSpecInfoSelector");
    require(selector.readable() && selector.type==FeatureType::Enumeration && !selector.entries.isEmpty(),"Embedded metadata bank inventory required");
    try {
        for(const auto& bank:selector.entries.keys()){
            session.writeFeature("FrameSpecInfoSelector",bank);
            require(featureEquivalent(session.feature("FrameSpecInfo"),false),"Embedded metadata must be disabled in every bank");
        }
    }catch(...){
        const auto error=std::current_exception();
        try{session.writeFeature("FrameSpecInfoSelector",selector.value);}catch(...){session.configurationFault();throw;}
        std::rethrow_exception(error);
    }
    session.writeFeature("FrameSpecInfoSelector",selector.value);
}
class Backend final:public CameraWorkerBackend {
    IndustrialCameraRequest request_;
    OecsSession session_;
    std::optional<IndustrialConfigurationResult> configured_;
    bool connected_=false,started_=false,fault_=false;
    QJsonObject evidence_;
    QString runtimeHash_;
public:
    Backend(IndustrialCameraRequest r,QString dll,ExperimentalOecsAcquisition::ApiFactory api)
        :request_(std::move(r)),session_(api(),dll){
        AcquisitionTimingSpan timing("backend.dll-hash");
        QFile file(dll);if(file.open(QIODevice::ReadOnly)){QCryptographicHash hash(QCryptographicHash::Sha256);
            require(hash.addData(&file),"Cannot hash OECS runtime");runtimeHash_=QString::fromLatin1(hash.result().toHex());}
    }
    CameraWorkerResult execute(const CameraTicket& t) override {
        const auto budget=[&]{require(!t.cancel->load(),"Camera cancelled");const int ms=t.deadline.remainingMs();require(ms>0,"Camera deadline exceeded");return ms;};
        try{
            budget();require(!fault_,"Camera fault; explicit close/reopen required");
            switch(t.operation){
            case CameraOperation::Connect:{
                require(!connected_,"Camera already open");const auto& id=request_.device.identity;
                session_.open({id.vendor,id.model,id.serial});connected_=true;break;
            }
            case CameraOperation::Configure:{
                AcquisitionTimingSpan timing("configure.readback");
                require(connected_ && !started_,"Camera not idle");verifyCleanRaster(session_);budget();
                configured_=IndustrialCameraConfiguration::applyExperimental(session_,request_.device.identity,request_.setup,request_.profile);
                require(configured_->actual.geometry.resolution==QSize(2448,2048) && configured_->payloadBytes==2448ULL*2048,"Exact clean full sensor raster required");
                verifyCleanRaster(session_);
                evidence_={{"schema","visionflow.oecs-experimental1"},{"policy","UnverifiedNextArrival"},
                    {"notice",ExperimentalOecsAcquisition::notice()},{"triggerAssociationVerified",false},{"productionAdmission",false},
                    {"vendor",request_.device.identity.vendor},{"model",request_.device.identity.model},{"serial",request_.device.identity.serial},
                    {"backend","oecs-c-api"},{"runtimeSha256",runtimeHash_},{"sdkVersion",QString::number(session_.sdkVersion(),16)},
                    {"profileId",request_.profile.id.value},{"profileRevision",double(request_.profile.revision)},
                    {"profileSnapshot",request_.profile.toJson()},
                    {"acquisitionId",configured_->acquisitionId},{"readback",configured_->actual.toJson()},{"settings",configured_->actual.settings.toJson()},
                    {"burstCount",1},{"triggerCache",false},{"triggerDelay",0},{"triggerDelayUnit","unreported"},{"embeddedMetadataEnabled",false},
                    {"autoModeReadbackPhase","Off verified before TriggerMode On; auto controls may become unavailable in triggered mode"},
                    {"width",2448},{"height",2048},{"originX",0},{"originY",0},{"pixelFormat","Mono8"}};
                break;
            }
            case CameraOperation::Start:require(configured_.has_value(),"Camera not configured");started_=true;break;
            case CameraOperation::Grab:{
                require(started_ && configured_.has_value(),"Camera not started");
                verifyCleanRaster(session_);budget();std::optional<AcquiredFrame> frame;
                OecsStreamLimits limits;limits.timeoutMs=budget();
                session_.experimentalFrame(limits,budget,[&](const OecsBuffer& b){
                    require(b.width==2448 && b.height==2048 && b.stride==2448 && b.bytes==2448ULL*2048,
                            "Frame differs from configured clean full sensor geometry");
                    FrameMetadata m;m.sourceId=request_.sourceId;m.frameId=QUuid::createUuid().toString(QUuid::WithoutBraces);
                    m.cycleId="experimental-preview";m.coordinateSystemId=request_.sourceId+"/image";
                    m.completedAtUtc=QDateTime::currentDateTimeUtc();m.deviceTimestampTicks=b.deviceTicks;
                    m.experimentalAcquisition=evidence_;
                    m.experimentalAcquisition["hardwareFrameId"]=QString::number(b.hardwareFrameId);
                    m.experimentalAcquisition["deviceTicks"]=QString::number(b.deviceTicks);
                    m.experimentalAcquisition["stride"]=QString::number(b.stride);
                    m.experimentalAcquisition["receivedBytes"]=QString::number(b.bytes);
                    auto image=ImageFrame::fromBuffer(b.data,size_t(b.bytes),int(b.width),int(b.height),size_t(b.stride),PixelFormat::Gray8,m);
                    frame=AcquiredFrame{std::move(image),m.sourceId,m.cycleId,m.frameId,m.completedAtUtc};
                });
                budget();return {true,{},std::move(frame),configured_->actual};
            }
            case CameraOperation::Stop:started_=false;break;
            case CameraOperation::Disconnect:session_.close();connected_=started_=false;configured_.reset();break;
            }
            budget();return {true,{},{},configured_?std::optional(configured_->actual):std::nullopt};
        }catch(...){fault_=true;throw;}
    }
    void shutdown() override {started_=false;session_.close();}
};
}
QString ExperimentalOecsAcquisition::notice(){return QString::fromUtf8("Deneysel kamera — tetik eşleşmesi doğrulanmadı");}
QString ExperimentalOecsAcquisition::installedRuntime(){return "C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvCameraControl.dll";}
bool ExperimentalOecsAcquisition::available(const DiscoveredDevice& device,const QString& dll){
    return supported(device.identity) && oecsSdkBindingCompiled() && QFileInfo(dll).isAbsolute() && QFileInfo(dll).isFile();
}
std::unique_ptr<CameraWorkerSession> ExperimentalOecsAcquisition::open(const IndustrialCameraRequest& request,const QString& dll,ApiFactory api){
    request.validate();require(supported(request.device.identity),"OECS device unsupported");
    require(QFileInfo(dll).isAbsolute() && bool(api),"Absolute OECS runtime and API required");
    require(request.profile.requested.trigger==TriggerSettings{true,"FrameBurstStart","Software"},"FrameBurstStart profile required");
    return std::make_unique<CameraWorkerSession>(request.device.identity,CameraLeaseUse::Editor,request.sourceId,FrameQueueMode::PreviewLatest,
        [request,dll,api=std::move(api)]{return std::make_unique<Backend>(request,dll,api);},1);
}
std::unique_ptr<CameraWorkerSession> ExperimentalOecsAcquisition::openCalibration(const IndustrialCameraRequest& request,const QString& dll,ApiFactory api){
    request.validate();require(supported(request.device.identity),"Calibration device unsupported");
    require(QFileInfo(dll).isAbsolute() && bool(api),"Absolute OECS runtime and API required");
    require(request.profile.requested.trigger==TriggerSettings{true,"FrameBurstStart","Software"},"FrameBurstStart profile required");
    return std::make_unique<CameraWorkerSession>(request.device.identity,CameraLeaseUse::Calibration,request.sourceId,FrameQueueMode::PreviewLatest,
        [request,dll,api=std::move(api)]{return std::make_unique<Backend>(request,dll,api);},1);
}
void ExperimentalOecsAcquisition::save(const AcquiredFrame& frame,const QString& path){
    const auto& image=frame.image;auto evidence=image.metadata().experimentalAcquisition;
    require(evidence.value("schema")=="visionflow.oecs-experimental1" && evidence.value("policy")=="UnverifiedNextArrival" &&
            evidence.value("triggerAssociationVerified")==false && evidence.value("productionAdmission")==false,"Frame provenance required");
    require(image.format()==PixelFormat::Gray8 && QFileInfo(path).isAbsolute() && path.endsWith(".pgm",Qt::CaseInsensitive),"Absolute .pgm path and Mono8 required");
    require(!QFileInfo::exists(path) && !QFileInfo::exists(path+".json"),"Choose a new capture filename");
    evidence["contentId"]=QString::fromLatin1(image.contentId().toHex());evidence["frameId"]=frame.frameId;
    evidence["sourceId"]=frame.sourceId;evidence["completedAtUtc"]=frame.acquiredAtUtc.toString(Qt::ISODateWithMs);
    evidence["rasterFile"]=QFileInfo(path).fileName();
    QSaveFile sidecar(path+".json");const auto json=QJsonDocument(evidence).toJson();
    require(sidecar.open(QIODevice::WriteOnly) && sidecar.write(json)==json.size() && sidecar.commit(),"Frame evidence save failed");
    QSaveFile raster(path);require(raster.open(QIODevice::WriteOnly),"Raster save failed");
    // The in-band marker survives renaming or losing the sidecar. EXP-02 must
    // explicitly admit file replay with its experimental provenance and UI notice.
    const auto header=QString("P5\n# VisionFlow-OECS-Experimental-Unverified\n%1 %2\n255\n").arg(image.size().width()).arg(image.size().height()).toLatin1();
    require(raster.write(header)==header.size(),"Raster header save failed");ImageAccess access(image);
    for(int y=0;y<image.size().height();++y){const auto row=access.row(y);require(raster.write(reinterpret_cast<const char*>(row.data()),qint64(row.size()))==qint64(row.size()),"Raster row save failed");}
    require(raster.commit(),"Raster commit failed");
}
}
