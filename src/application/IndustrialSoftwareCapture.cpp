#include "application/IndustrialSoftwareCapture.hpp"
#include <QUuid>
namespace visionflow {
namespace {
class TriggerAdapter final: public AcquisitionAdapter {
public:
    TriggerAdapter(OecsSession& session,OecsStreamLimits limits):session_(session),limits_(limits){}
    AcquisitionCapabilities capabilities() const override {
        return {"industrial",session_.softwareTriggerCorrelation()==OecsTriggerCorrelation::ActualCounterEpoch,
            false,false,false,false};
    }
    ImageFrame acquire(const AcquisitionRequest& request) const override {
        std::optional<ImageFrame> owned;
        const auto remaining=[&]{
            if(request.cancel&&request.cancel->load())throw std::runtime_error("Software capture cancelled");
            const int ms=request.deadline->remainingMs();
            if(ms<=0)throw std::runtime_error("Software capture deadline exceeded");
            return ms;
        };
        const auto arm=session_.softwareFrame(limits_,remaining,[&](const OecsBuffer& b){
            FrameMetadata metadata;
            metadata.deviceTimestampTicks=b.deviceTicks;
            owned=ImageFrame::fromBuffer(b.data,std::size_t(b.bytes),int(b.width),int(b.height),
                std::size_t(b.stride),PixelFormat::Gray8,metadata,false);
            evidence=IndustrialTriggerEvidence{{},{b.width,b.height,b.pixelFormat,b.stride,b.bytes,b.hardwareFrameId,b.deviceTicks},b.triggerCounter};
        });
        evidence->arm=arm;
        return std::move(owned.value());
    }
    mutable std::optional<IndustrialTriggerEvidence> evidence;
private:
    OecsSession& session_;
    OecsStreamLimits limits_;
};
}
IndustrialTriggerResult IndustrialSoftwareCapture::capture(OecsSession& session,const QString& sourceId,int timeoutMs,
    std::shared_ptr<std::atomic_bool> cancel,std::optional<Deadline> deadline,OecsStreamLimits limits){
    // Include admission/setup in the same monotonic budget, with the existing 30 s cap.
    AcquisitionRequest request;
    request.deadline=deadline?deadline->limitedTo(std::min(timeoutMs,30000)):Deadline(std::min(timeoutMs,30000));
    request.sourceId=sourceId;
    request.cycleId=QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.frameId=QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.timeoutMs=std::min(timeoutMs,30000);
    request.cancel=std::move(cancel);
    limits.timeoutMs=request.timeoutMs;
    TriggerAdapter adapter(session,limits);
    IndustrialTriggerResult result;
    result.acquisition=AcquisitionService{}.acquire(adapter,request);
    if(result.acquisition.state==AcquisitionState::Done)result.evidence=adapter.evidence;
    return result;
}
}
