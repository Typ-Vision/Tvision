#include "BaslerCameraSettings.hpp"
#include "hardware/PylonSession.hpp"
#include <QFileInfo>
#include <cmath>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool supported(const DiscoveredDevice& d){return d.identity.usable() && d.identity.vendor=="Basler" && d.identity.model=="acA1920-40um" && d.route.backend=="gentl";}
QString text(const CameraFeature& f){require(f.readable() && (f.type==FeatureType::String || f.type==FeatureType::Enumeration),"Basler text readback unavailable");return std::get<QString>(f.value);}
class Backend final:public IndustrialCameraBackend {
    DiscoveredDevice device_;
    QString runtime_;
    QStringList dependencies_;
    PylonSession session_;
    int integer(const char* name) const {const auto f=setting(name);require(f.readable() && f.type==FeatureType::Integer,"Basler integer readback unavailable");const auto n=std::get<qint64>(f.value);require(n>=0 && n<=65536,"Basler geometry out of bounds");return int(n);}
    double scalar(const char* name,const QStringList& units) const {const auto f=setting(name);require(f.readable() && f.type==FeatureType::Float && units.contains(f.unit),"Unproven Basler scalar/unit");const auto n=std::get<double>(f.value);require(std::isfinite(n) && std::isfinite(f.minimum) && std::isfinite(f.maximum) && n>=f.minimum && n<=f.maximum,"Invalid Basler scalar readback");return n;}
public:
    Backend(DiscoveredDevice d,QString runtime,QStringList dependencies):device_(std::move(d)),runtime_(std::move(runtime)),dependencies_(std::move(dependencies)){}
    CameraWorkerResult execute(const CameraTicket& t) override {
        require(t.operation==CameraOperation::Connect,"Basler settings cannot acquire frames");
        require(!t.cancel->load() && !t.deadline.expired(),"Settings cancelled/expired");
        const auto& id=device_.identity;session_.open(runtime_,dependencies_,{id.vendor,id.model,id.serial});return {true,{},{}};
    }
    PhysicalDeviceIdentity actualIdentity() const override {auto id=device_.identity;
        id.vendor=text(setting("DeviceVendorName"));id.model=text(setting("DeviceModelName"));
        auto serial=setting("DeviceSerialNumber");if(!serial.readable())serial=setting("DeviceID");id.serial=text(serial);return id;}
    CameraFeature setting(const QString& name) const override {return session_.feature(name);}
    void writeSetting(const QString& name,const FeatureValue& value) override {session_.writeFeature(name,value);}
    QSize sensorSize() const override {return {integer("SensorWidth"),integer("SensorHeight")};}
    AcquisitionReadback readback() const override {
        AcquisitionReadback r;auto& g=r.geometry;auto& s=r.settings;
        g.resolution={integer("Width"),integer("Height")};
        const auto evidence=[&](const char* x,const char* y){const auto a=setting(x),b=setting(y);
            if(a.readable() && b.readable())return GeometryEvidence::Reported;
            if(a.access==FeatureAccess::Missing && b.access==FeatureAccess::Missing)return GeometryEvidence::NotApplicable;
            return GeometryEvidence::Unknown;};
        g.offsetEvidence=evidence("OffsetX","OffsetY");if(g.offsetEvidence==GeometryEvidence::Reported)g.offset={integer("OffsetX"),integer("OffsetY")};
        const auto pair=[&](const char* x,const char* y,QSize& size,GeometryEvidence& e){e=evidence(x,y);if(e==GeometryEvidence::Reported)size={integer(x),integer(y)};};
        pair("BinningHorizontal","BinningVertical",g.binning,g.binningEvidence);pair("DecimationHorizontal","DecimationVertical",g.decimation,g.decimationEvidence);
        g.reverseEvidence=evidence("ReverseX","ReverseY");if(g.reverseEvidence==GeometryEvidence::Reported){const auto x=setting("ReverseX"),y=setting("ReverseY");require(x.type==FeatureType::Boolean && y.type==FeatureType::Boolean,"Basler reverse type mismatch");g.reverse={std::get<bool>(x.value),std::get<bool>(y.value)};}
        s.pixelFormat=text(setting("PixelFormat"));s.exposureTimeUs=scalar("ExposureTime",{"us",QString::fromUtf8("µs")});s.gain=scalar("Gain",{"dB"});
        const auto automatic=[&](const char* n){const auto v=text(setting(n));require(v=="Off" || v=="Continuous" || v=="Once","Unknown Basler auto mode");return v=="Off"?AutoMode::Off:v=="Once"?AutoMode::Once:AutoMode::Continuous;};
        s.exposureAuto=automatic("ExposureAuto");s.gainAuto=automatic("GainAuto");
        const auto mode=text(setting("TriggerMode"));require(mode=="Off" || mode=="On","Unknown Basler trigger mode");s.trigger.enabled=mode=="On";
        if(s.trigger.enabled){s.trigger.selector=text(setting("TriggerSelector"));s.trigger.source=text(setting("TriggerSource"));}
        r.validate();return r;
    }
    void shutdown() override {session_.close();}
};
}
QString BaslerCameraSettings::unavailableReason(const DiscoveredDevice& d,const QString& runtime){
    if(!supported(d))return "Unsupported Basler identity/route (acA1920-40um USB required)";
    if(!PylonSession::available())return "pylon C binding unavailable; VISIONFLOW_PYLON_INCLUDE_DIR required";
    if(!QFileInfo(runtime).isAbsolute())return "Basler runtime DLL path must be absolute";
    if(!QFileInfo(runtime).isFile())return "Basler runtime DLL not found";
    return {};
}
std::unique_ptr<IndustrialCameraBackend> BaslerCameraSettings::create(const DiscoveredDevice& d,const QString& runtime,const QStringList& dependencies){
    d.validate();require(unavailableReason(d,runtime).isEmpty(),"Basler settings backend unavailable / unsupported");return std::make_unique<Backend>(d,runtime,dependencies);
}
}
