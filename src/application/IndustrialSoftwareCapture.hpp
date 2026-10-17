#pragma once
#include "application/AcquisitionService.hpp"
#include "application/IndustrialFrameProbe.hpp"
namespace visionflow {
struct IndustrialTriggerEvidence {
    OecsTriggerArm arm;
    IndustrialFrameEvidence actual;
    quint32 triggerCounter=0;
};
struct IndustrialTriggerResult {
    AcquisitionResult acquisition;
    std::optional<IndustrialTriggerEvidence> evidence;
};
// Worker-only software admission. IDs minted privately; no Runtime trust/lease yet.
class IndustrialSoftwareCapture {
public:
    static IndustrialTriggerResult capture(OecsSession&,const QString& sourceId,int timeoutMs=1000,
        std::shared_ptr<std::atomic_bool> cancel={},std::optional<Deadline> deadline={},
        OecsStreamLimits limits={});
};
}
