#pragma once
#include "AcquisitionService.hpp"
#include "CalibrationWizardSession.hpp"
#include "MeasurementSourceIdentity.hpp"
#include "IndustrialCameraProvider.hpp"
#include <QThread>
#include <functional>

namespace visionflow {
// No recipe or ToolDefinition. Targeted files require an explicit setup declaration;
// trial files have no target and cannot acquire camera provenance.
struct IndependentCaptureRequest {
    SourceOrigin origin=SourceOrigin::TrialFile;
    QString filePath;
    std::optional<MeasurementCamera> camera;
    std::optional<MeasurementSetupDescriptor> setup;
    int timeoutMs=5000;
    std::optional<AcquisitionProfile> profile;
    std::optional<TransportLocator> localRoute;
    void validate() const;
    static MeasurementSetup installation(const MeasurementCamera&,const MeasurementSetupDescriptor&);
};
// Adapter seam for deterministic contract tests, not a user-editable source claim.
struct IndependentCaptureBackend {
    std::function<QVector<DeviceBinding>()> discover;
    std::function<std::unique_ptr<AcquisitionAdapter>(const DeviceBinding&,const GeometryDescriptor&)> create;
    CameraProviderFactory factory;
    std::function<IndustrialDiscoverySnapshot()> industrialDiscover;
};
class IndependentCaptureJob {
    friend class IndependentCalibrationSession;
    IndependentCaptureJob()=default;
    IndependentCaptureRequest request_;
    QString sourceId_,ticket_;
    int generation_=0;
    Deadline deadline_{1};
    std::shared_ptr<std::atomic_bool> cancel_;
public:
    void cancel() const {cancel_->store(true);}
};
class IndependentCaptureResult {
    friend class IndependentCalibrationSession;
    IndependentCaptureResult()=default;
    QString ticket_,error_;
    int generation_=0;
    std::optional<ImageFrame> frame_;
    std::optional<MeasurementSourceEvidence> evidence_;
    std::optional<MeasurementSetup> mathSetup_;
    std::optional<AcquisitionProfile> profile_;
    std::optional<DiscoveredDevice> device_;
public:
    const QString& error() const {return error_;}
};
// Owner-thread admission/accept; run(job) executes on a caller-owned worker and
// never retains this session or any QWidget. New target/Stop retires old drafts.
class IndependentCalibrationSession {
public:
    IndependentCalibrationSession();
    ~IndependentCalibrationSession();
    IndependentCalibrationSession(const IndependentCalibrationSession&)=delete;
    IndependentCalibrationSession& operator=(const IndependentCalibrationSession&)=delete;
    IndependentCaptureJob begin(IndependentCaptureRequest);
    static IndependentCaptureResult run(const IndependentCaptureJob&,const IndependentCaptureBackend& = {});
    bool accept(const IndependentCaptureResult&);
    void cancel();
    bool busy() const;
    std::optional<MeasurementSourceEvidence> evidence() const;
    std::optional<MeasurementSetup> acceptedSetup() const;
    std::optional<AcquisitionProfile> acceptedProfile() const;
    std::optional<DiscoveredDevice> acceptedDevice() const;
    CalibrationWizardSession& math(); // existing Edge/Caliper/plate implementation
private:
    void owner() const;
    QThread* thread_;
    QString sourceId_,ticket_;
    int generation_=0;
    std::shared_ptr<std::atomic_bool> cancel_;
    std::optional<MeasurementSourceEvidence> evidence_;
    std::optional<MeasurementSetup> acceptedSetup_;
    std::optional<AcquisitionProfile> profile_;
    std::optional<DiscoveredDevice> device_;
    CalibrationWizardSession math_;
};
}
