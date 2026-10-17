#include "application/IndustrialFrameProbe.hpp"
#include <optional>
namespace visionflow {
IndustrialProbeResult IndustrialFrameProbe::capture(OecsSession& session,const OecsStreamLimits& limits){
    std::optional<IndustrialProbeResult> result;
    session.probeFrame(limits,[&](const OecsBuffer& buffer){
        FrameMetadata metadata;
        metadata.completedAtUtc=QDateTime::currentDateTimeUtc();
        metadata.deviceTimestampTicks=buffer.deviceTicks;
        // No UTC exposure, clock epoch or source/frame/cycle admission IDs.
        auto owned=ImageFrame::fromBuffer(buffer.data,std::size_t(buffer.bytes),int(buffer.width),int(buffer.height),
            std::size_t(buffer.stride),PixelFormat::Gray8,metadata,false);
        result=IndustrialProbeResult{std::move(owned),{buffer.width,buffer.height,buffer.pixelFormat,
            buffer.stride,buffer.bytes,buffer.hardwareFrameId,buffer.deviceTicks}};
    });
    return std::move(result.value());
}
}
