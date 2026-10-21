#pragma once
#include "IndustrialProviderFixture.hpp"
namespace camera_settings_fixture {
struct SettingsHardware {
    AcquisitionReadback actual;
    QMap<QString,CameraFeature> nodes;
    QString failure,clamp;
    bool rollbackFailure=false,failed=false,disconnect=false;
    bool rounded=false,dynamicAuto=false;
    std::function<void()> afterWrite;
    int writes=0;
    std::thread::id worker;
};
class SettingsBackend:public IndustrialCameraBackend {
    IndustrialCameraRequest r_;
    std::shared_ptr<SettingsHardware> h_;
    void worker() const {check(std::this_thread::get_id()==h_->worker,"settings stay on one worker");}
public:
    SettingsBackend(IndustrialCameraRequest r,std::shared_ptr<SettingsHardware> h):r_(std::move(r)),h_(std::move(h)){h_->worker=std::this_thread::get_id();}
    PhysicalDeviceIdentity actualIdentity() const override {worker();return r_.device.identity;}
    AcquisitionReadback readback() const override {worker();if(h_->disconnect)throw std::runtime_error("disconnected");return h_->actual;}
    CameraFeature setting(const QString& n) const override {worker();if(h_->disconnect)throw std::runtime_error("disconnected");auto f=h_->nodes.value(n);
        if(h_->dynamicAuto && n=="ExposureTime" && h_->actual.settings.exposureAuto!=AutoMode::Off)f.access=FeatureAccess::ReadOnly;return f;}
    void writeSetting(const QString& n,const FeatureValue& value) override {
        worker();++h_->writes;if(h_->rollbackFailure && h_->failed)throw std::runtime_error("restore failed");
        h_->nodes[n].value=value;
        if(n=="ExposureTime"){const double v=h_->rounded?double(float(std::get<double>(value))):std::get<double>(value);h_->nodes[n].value=v;h_->actual.settings.exposureTimeUs=v;}
        if(n=="Gain")h_->actual.settings.gain=std::get<double>(value);
        if(n=="ExposureAuto")h_->actual.settings.exposureAuto=std::get<QString>(value)=="Off"?AutoMode::Off:AutoMode::Continuous;
        if(n=="GainAuto")h_->actual.settings.gainAuto=std::get<QString>(value)=="Off"?AutoMode::Off:AutoMode::Continuous;
        if(n==h_->clamp && !h_->failed){h_->failed=true;h_->nodes[n].value=std::get<double>(value)-1.;h_->actual.settings.exposureTimeUs=std::get<double>(value)-1.;}
        if(n==h_->failure && !h_->failed){h_->failed=true;throw std::runtime_error("SDK write failure after mutation");}
        if(h_->afterWrite){auto callback=std::move(h_->afterWrite);h_->afterWrite={};callback();}
    }
    CameraWorkerResult execute(const CameraTicket&) override {worker();return {true,{},{}};}
    void shutdown() override {worker();}
};
class SettingsProvider:public IndustrialCameraProvider {
    std::shared_ptr<SettingsHardware> h_;
public:
    explicit SettingsProvider(std::shared_ptr<SettingsHardware> h):h_(std::move(h)){}
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice&) const override {return {true,true,false,false,true,true,{}};}
    std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest& r) const override {return std::make_unique<SettingsBackend>(r,h_);}
    std::unique_ptr<IndustrialCameraBackend> createSettings(const MeasurementCamera& camera,const DiscoveredDevice& device) const override {
        IndustrialCameraRequest r;r.camera=camera;r.device=device;return std::make_unique<SettingsBackend>(r,h_);
    }
};
}
