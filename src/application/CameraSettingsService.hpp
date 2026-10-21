#pragma once
#include "IndustrialCameraProvider.hpp"
#include "CameraLocalConfiguration.hpp"
#include "MeasurementEnvironment.hpp"
#include <mutex>

namespace visionflow {
struct CameraSettingsValues {
    std::optional<double> exposureUs, gainDb;
};
struct CameraSettingsSnapshot {
    CameraSettingsValues requested;
    CameraFeature exposure, gain, exposureAuto, gainAuto;
    AcquisitionReadback actual;
    bool saved=false, profileChanged=false, geometryChanged=false, deviceChanged=false, unusable=false;
    QString error;
};
// Controller owner thread: Connect reads actual values; apply submits Configure.
// Destruction drains the persistent worker before releasing its exclusive lease.
// The workspace/gate must outlive this session; invalidate is owner-thread, non-throwing.
class CameraSettingsSession {
public:
    ~CameraSettingsSession();
    void connect(int timeoutMs=5000);
    void apply(CameraSettingsValues,int timeoutMs=5000);
    bool poll();
    CameraSettingsSnapshot snapshot() const;
    CameraState state() const;
    QString error() const;
    void stop();
private:
    friend class CameraSettingsService;
    struct Shared {
        mutable std::mutex mutex;
        CameraSettingsSnapshot snapshot;
        CameraSettingsValues desired;
    };
    CameraSettingsSession()=default;
    std::shared_ptr<Shared> shared_;
    std::unique_ptr<CameraWorkerSession> worker_;
    MeasurementEnvironment expected_;
    std::optional<MeasurementEnvironmentRepository> repository_;
    std::function<bool()> gate_;
    std::function<void()> invalidate_;
    bool applying_=false;
};
class CameraSettingsService {
public:
    static std::unique_ptr<CameraSettingsSession> open(
        MeasurementEnvironmentRepository,const CameraId&,int expectedRevision,
        const IndustrialDiscoverySnapshot&,const CameraLocalConfiguration&,
        const CameraProviderFactory&,std::function<bool()> gate,
        std::function<void()> invalidate,
        MeasurementEnvironmentRepository::Fault=MeasurementEnvironmentRepository::Fault::None);
};
}
