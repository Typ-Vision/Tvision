#include "OecsCameraSettings.hpp"
#include <QFileInfo>
#include <cmath>

namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool supported(const PhysicalDeviceIdentity& id){return id.usable() && id.vendor=="Opto Engineering Srl" && id.model=="COE-050-M-USB-050-IR-C";}
QString text(const CameraFeature& f){require(f.readable() && (f.type==FeatureType::Enumeration || f.type==FeatureType::String),"OECS text readback unavailable");return std::get<QString>(f.value);}
class Backend final:public IndustrialCameraBackend {
    DiscoveredDevice device_;
    mutable OecsSession session_;
    // OECS hides auto nodes while triggered. Read/write them with trigger disabled,
    // restoring and verifying the original mode even after a failed SDK write.
    template<class F> auto autoAccess(const QString& name,F action) const {
        const auto feature=session_.feature(name);
        if((name!="ExposureAuto" && name!="GainAuto") || feature.access!=FeatureAccess::Unavailable)return action();
        const auto mode=session_.feature("TriggerMode");require(text(mode)=="On","Auto controls unavailable outside triggered mode");
        try {
            session_.writeFeature("TriggerMode",QString("Off"));
            auto result=action();session_.writeFeature("TriggerMode",mode.value);return result;
        }catch(...){
            const auto original=std::current_exception();
            try{session_.writeFeature("TriggerMode",mode.value);}catch(...){session_.configurationFault();throw std::runtime_error("OECS trigger restore unverified; close/recheck required");}
            std::rethrow_exception(original);
        }
    }
public:
    Backend(DiscoveredDevice d,QString runtime,OecsCameraSettings::ApiFactory api):device_(std::move(d)),session_(api(),runtime){}
    CameraWorkerResult execute(const CameraTicket& t) override {
        require(t.operation==CameraOperation::Connect,"OECS settings cannot acquire frames");
        require(!t.cancel->load() && !t.deadline.expired(),"Settings cancelled/expired");
        const auto& id=device_.identity;session_.open({id.vendor,id.model,id.serial});return {true,{},{}};
    }
    PhysicalDeviceIdentity actualIdentity() const override {
        const auto d=session_.deviceInfo();auto id=device_.identity;id.vendor=d.vendor;id.model=d.model;id.serial=d.serial;
        require(text(session_.feature("DeviceVendorName"))==id.vendor && text(session_.feature("DeviceModelName"))==id.model && text(session_.feature("DeviceSerialNumber"))==id.serial,"OECS feature identity mismatch");return id;
    }
    CameraFeature setting(const QString& name) const override {return autoAccess(name,[&]{return session_.feature(name);});}
    void writeSetting(const QString& name,const FeatureValue& value) override {
        require(QStringList{"ExposureTime","Gain","ExposureAuto","GainAuto"}.contains(name),"Unsupported basic setting");
        autoAccess(name,[&]{session_.writeFeature(name,value);return true;});
    }
    AcquisitionReadback readback() const override {
        AcquisitionReadback result;auto& g=result.geometry;auto& s=result.settings;
        const auto integer=[&](const char* n){const auto f=session_.feature(n);require(f.readable() && f.type==FeatureType::Integer,"OECS integer readback unavailable");const auto v=std::get<qint64>(f.value);require(v>=0 && v<=65536,"OECS geometry out of bounds");return int(v);};
        const auto numeric=[&](const char* n){const auto f=session_.feature(n);require(f.readable() && (f.type==FeatureType::Float || f.type==FeatureType::Integer),"OECS numeric readback unavailable");return f.type==FeatureType::Float?std::get<double>(f.value):double(std::get<qint64>(f.value));};
        g.resolution={integer("Width"),integer("Height")};g.offset={integer("OffsetX"),integer("OffsetY")};g.offsetEvidence=GeometryEvidence::Reported;
        const auto pair=[&](const char* x,const char* y,QSize& size,GeometryEvidence& evidence){
            const auto a=session_.feature(x),b=session_.feature(y);
            if(a.access==FeatureAccess::Missing && b.access==FeatureAccess::Missing){size={1,1};evidence=GeometryEvidence::NotApplicable;return;}
            const auto factor=[&](const char* n,const CameraFeature& f){if(f.type==FeatureType::Integer)return integer(n);
                const auto symbol=QString(n)+"1";require(text(f)==symbol && f.entries.contains(symbol) && f.entries.value(symbol)==1,"Unproven OECS geometry factor");return 1;};
            size={factor(x,a),factor(y,b)};evidence=GeometryEvidence::Reported;
        };
        pair("BinningHorizontal","BinningVertical",g.binning,g.binningEvidence);pair("DecimationHorizontal","DecimationVertical",g.decimation,g.decimationEvidence);
        const auto boolean=[&](const char* n){const auto f=session_.feature(n);require(f.readable() && f.type==FeatureType::Boolean,"OECS reverse readback unavailable");return std::get<bool>(f.value);};
        g.reverse={boolean("ReverseX"),boolean("ReverseY")};g.reverseEvidence=GeometryEvidence::Reported;
        s.pixelFormat=text(session_.feature("PixelFormat"));s.exposureTimeUs=numeric("ExposureTime");s.gain=numeric("Gain");
        const auto automatic=[&](const char* n){const auto v=text(setting(n));require(v=="Off" || v=="Continuous" || v=="Once","Unknown auto mode");return v=="Off"?AutoMode::Off:v=="Once"?AutoMode::Once:AutoMode::Continuous;};
        s.exposureAuto=automatic("ExposureAuto");s.gainAuto=automatic("GainAuto");
        const auto mode=text(session_.feature("TriggerMode"));require(mode=="On" || mode=="Off","Unknown trigger mode");s.trigger.enabled=mode=="On";
        if(s.trigger.enabled){s.trigger.selector=text(session_.feature("TriggerSelector"));s.trigger.source=text(session_.feature("TriggerSource"));}
        result.validate();return result;
    }
    void shutdown() override {session_.close();}
};
}
QString OecsCameraSettings::installedRuntime(){return "C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvCameraControl.dll";}
bool OecsCameraSettings::available(const DiscoveredDevice& d,const QString& runtime){return supported(d.identity) && oecsSdkBindingCompiled() && QFileInfo(runtime).isAbsolute() && QFileInfo(runtime).isFile();}
std::unique_ptr<IndustrialCameraBackend> OecsCameraSettings::create(const DiscoveredDevice& d,const QString& runtime,ApiFactory api){
    d.validate();require(supported(d.identity) && QFileInfo(runtime).isAbsolute() && bool(api),"Supported OECS identity/runtime required");return std::make_unique<Backend>(d,runtime,std::move(api));
}
}
