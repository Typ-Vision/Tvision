#include "AcquisitionService.hpp"
#include <QElapsedTimer>
#include <exception>

namespace visionflow {
AcquisitionCapabilities ImageFileAdapter::capabilities() const {
    return {"image_file",true,false,false,false,false};
}
ImageFrame ImageFileAdapter::acquire(const AcquisitionRequest&) const {
    return ImageFileService{}.load(path_,experimentalEditorReplay_);
}
AcquisitionResult AcquisitionService::acquire(const AcquisitionAdapter& adapter,const AcquisitionRequest& request) const {
    AcquisitionResult result;
    QElapsedTimer timer;timer.start();
    auto cancelled=[&]{return request.cancel && request.cancel->load();};
    auto finish=[&]{result.milliseconds=timer.nsecsElapsed()/1e6;return result;};
    if(cancelled()) {result.state=AcquisitionState::Cancelled;result.error="Görüntü edinimi iptal edildi.";return finish();}
    if(request.sourceId.trimmed().isEmpty() || request.cycleId.trimmed().isEmpty() || request.frameId.trimmed().isEmpty() || request.timeoutMs<=0) {
        result.error="Edinim source/cycle/frame kimlikleri ve pozitif timeout gerektirir.";return finish();
    }
    auto effective=request;
    effective.deadline=request.deadline ? request.deadline->limitedTo(request.timeoutMs) : Deadline(request.timeoutMs);
    auto timedOut=[&]{return effective.deadline->expired();};
    if(timedOut()){result.state=AcquisitionState::Timeout;result.error="Edinim kuyruğunda deadline aşıldı.";return finish();}
    try {
        const auto caps=adapter.capabilities();
        const bool supported=request.trigger==TriggerMode::Software ? caps.softwareTrigger :
            request.trigger==TriggerMode::External ? caps.externalTrigger :
            request.trigger==TriggerMode::Continuous ? caps.continuous : false;
        if(!supported) {result.error="Kaynak istenen trigger modunu desteklemiyor.";return finish();}
        if(cancelled()){result.state=AcquisitionState::Cancelled;return finish();}
        if(timedOut()){result.state=AcquisitionState::Timeout;return finish();}
        auto image=adapter.acquire(effective);
        // File decode cannot be interrupted; late or cancelled frames must never escape.
        if(cancelled()) {result.state=AcquisitionState::Cancelled;result.error="Görüntü edinimi iptal edildi.";}
        else if(timedOut()) {result.state=AcquisitionState::Timeout;result.error="Görüntü edinimi zaman aşımı; geciken frame yayımlanmadı.";}
        else if(image.size().isEmpty()) {result.error="Adapter boş görüntü döndürdü.";}
        else {
            auto metadata=image.metadata();metadata.sourceId=request.sourceId;metadata.frameId=request.frameId;metadata.cycleId=request.cycleId;
            metadata.completedAtUtc=QDateTime::currentDateTimeUtc();
            if(metadata.coordinateSystemId.isEmpty())metadata.coordinateSystemId=request.sourceId+"/image";
            image=image.withMetadata(metadata);
            result.frame=AcquiredFrame{std::move(image),request.sourceId,request.cycleId,request.frameId,metadata.completedAtUtc};result.state=AcquisitionState::Done;
        }
    } catch(const std::exception& e) {
        if(cancelled()) {result.state=AcquisitionState::Cancelled;result.error="Görüntü edinimi iptal edildi.";}
        else if(timedOut()) {result.state=AcquisitionState::Timeout;result.error="Görüntü edinimi zaman aşımı.";}
        else result.error=QString::fromUtf8(e.what());
    }
    return finish();
}
}
