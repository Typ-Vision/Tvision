#pragma once
#include "domain/MeasurementSetupContract.hpp"
#include "hardware/OecsSession.hpp"
namespace visionflow {
struct IndustrialConfigurationResult {
    AcquisitionReadback actual;
    quint64 payloadBytes=0;
    QString acquisitionId;
};
// Worker-only service: requires an exclusive, idle session; no project writes or activation.
class IndustrialCameraConfiguration {
public:
    static IndustrialConfigurationResult apply(OecsSession&,const PhysicalDeviceIdentity&,
        const MeasurementSetupDescriptor&,const AcquisitionProfile&);
    static IndustrialConfigurationResult applyExperimental(OecsSession&,const PhysicalDeviceIdentity&,
        const MeasurementSetupDescriptor&,const AcquisitionProfile&);
};
}
