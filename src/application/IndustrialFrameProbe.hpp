#pragma once
#include "hardware/OecsSession.hpp"
#include "vision/ImageFrame.hpp"
namespace visionflow {
struct IndustrialFrameEvidence {
    quint32 width=0,height=0,pixelFormat=0;
    quint64 stride=0,receivedBytes=0,hardwareFrameId=0,deviceTicks=0;
};
struct IndustrialProbeResult {
    ImageFrame frame;
    IndustrialFrameEvidence actual;
};
// Worker diagnostic only: no production cycle admission or freshness guarantee.
class IndustrialFrameProbe {
public:
    static IndustrialProbeResult capture(OecsSession&,const OecsStreamLimits& limits={});
};
}
