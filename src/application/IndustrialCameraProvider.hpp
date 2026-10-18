#pragma once
#include "CameraService.hpp"
#include "CameraWorkerSession.hpp"

namespace visionflow {
// Immutable admission snapshot. Local discovery facts never enter the project codec.
struct IndustrialCameraRequest {
    MeasurementCamera camera;
    MeasurementSetupDescriptor setup;
    AcquisitionProfile profile;
    DiscoveredDevice device;
    QString sourceId;
    void validate() const;
};
struct IndustrialCameraCapabilities {
    bool open=false, configure=false, preview=false, softwareTrigger=false;
    bool exposure=false, gain=false;
    QString unavailableReason;
    // Runtime trust is deliberately absent: provider frames cannot mint terminal results.
};
class IndustrialCameraBackend : public CameraWorkerBackend {
public:
    virtual PhysicalDeviceIdentity actualIdentity() const=0;
    virtual AcquisitionReadback readback() const=0;
};
class IndustrialCameraProvider {
public:
    virtual ~IndustrialCameraProvider()=default;
    virtual IndustrialCameraCapabilities capabilities(const DiscoveredDevice&) const=0;
    virtual std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest&) const=0;
};
// Default product composition has no admitted native backend. Injection is an in-memory
// composition seam, never selected by config, producer path, recipe or caller evidence.
class CameraProviderFactory {
public:
    CameraProviderFactory()=default;
    std::unique_ptr<AcquisitionAdapter> webcam(const MeasurementCamera&,const QVector<DeviceBinding>&,QSize resolution={}) const;
    static CameraProviderFactory forTests(std::shared_ptr<const IndustrialCameraProvider>);
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice&) const;
    std::unique_ptr<CameraWorkerSession> open(const IndustrialCameraRequest&,CameraLeaseUse,
                                             FrameQueueMode,size_t capacity=2) const;
    std::unique_ptr<CameraWorkerSession> resolveOpen(IndustrialCameraRequest,const IndustrialDiscoverySnapshot&,
        const std::optional<TransportLocator>&,CameraLeaseUse,FrameQueueMode,size_t capacity=2) const;
private:
    std::shared_ptr<const IndustrialCameraProvider> injected_;
};
}
