#include "BaslerCameraSettings.hpp"
#include "hardware/PylonSession.hpp"
#include <QFileInfo>
#include <cmath>
#include <QUuid>
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
    std::optional<IndustrialCameraRequest> request_;
    bool configured_=false,started_=false,fault_=false;
    int integer(const char* name) const {const auto f=setting(name);require(f.readable() && f.type==FeatureType::Integer,"Basler integer readback unavailable");const auto n=std::get<qint64>(f.value);require(n>=0 && n<=65536,"Basler geometry out of bounds");return int(n);}
    double scalar(const char* name,const QStringList& units) const {const auto f=setting(name);require(f.readable() && f.type==FeatureType::Float && units.contains(f.unit),"Unproven Basler scalar/unit");const auto n=std::get<double>(f.value);require(std::isfinite(n) && std::isfinite(f.minimum) && std::isfinite(f.maximum) && n>=f.minimum && n<=f.maximum,"Invalid Basler scalar readback");return n;}
public:
    Backend(DiscoveredDevice d,QString runtime,QStringList dependencies):device_(std::move(d)),runtime_(std::move(runtime)),dependencies_(std::move(dependencies)){}
    Backend(IndustrialCameraRequest r,QString runtime,QStringList dependencies):Backend(r.device,std::move(runtime),std::move(dependencies)){request_=std::move(r);}
    CameraWorkerResult execute(const CameraTicket& t) override {
        const auto budget=[&]{require(!t.cancel->load() && !t.deadline.expired(),"Basler cancelled/expired");return t.deadline.remainingMs();};
        try{
            budget();require(!fault_,"Basler fault; close required");
            if(t.operation==CameraOperation::Connect){const auto& id=device_.identity;session_.open(runtime_,dependencies_,{id.vendor,id.model,id.serial});budget();return {true,{},{}};}
            require(request_.has_value(),"Basler settings cannot acquire frames");const auto& r=*request_;
            switch(t.operation){
            case CameraOperation::Configure:{
                require(!started_,"Basler configuration requires idle stream");
                // shortcut: active geometry must match; add ROI writes when another geometry is required.
                const auto actual=readback();require(actual.geometry.descriptor()==r.setup.geometry,"Basler active geometry differs from setup");
                require(r.profile.requested.pixelFormat=="Mono8" && r.profile.requested.exposureAuto==AutoMode::Off && r.profile.requested.gainAuto==AutoMode::Off,"Basler manual Mono8 profile required");
                const auto& trigger=r.profile.requested.trigger;require(!trigger.enabled || (trigger.selector=="FrameStart" && trigger.source=="Software"),"Basler supports FrameStart Software only");
                const auto selector=setting("TriggerSelector").value;
                QMap<QString,FeatureValue> before;
                try{
                    session_.writeFeature("TriggerSelector",QString("FrameStart"));
                    for(const auto& n:QStringList{"TriggerMode","TriggerSource","ExposureAuto","GainAuto","ExposureTime","Gain"}){const auto f=setting(n);require(f.readable(),"Basler configuration snapshot unavailable");before[n]=f.value;}
                    session_.writeFeature("TriggerMode",QString("Off"));
                    session_.writeFeature("ExposureAuto",QString("Off"));session_.writeFeature("GainAuto",QString("Off"));
                    session_.writeFeature("ExposureTime",r.profile.requested.exposureTimeUs);session_.writeFeature("Gain",r.profile.requested.gain);
                    session_.writeFeature("TriggerSource",QString("Software"));session_.writeFeature("TriggerMode",QString(trigger.enabled?"On":"Off"));
                    verifyApplied(r.setup,r.profile,readback());configured_=true;
                }catch(...){
                    const auto failure=std::current_exception();
                    if(before.size()==6){
                        session_.writeFeature("TriggerMode",QString("Off"));session_.writeFeature("ExposureAuto",QString("Off"));session_.writeFeature("GainAuto",QString("Off"));
                        for(const auto& n:QStringList{"ExposureTime","Gain","ExposureAuto","GainAuto","TriggerSource","TriggerMode"})session_.writeFeature(n,before[n]);
                    }
                    session_.writeFeature("TriggerSelector",selector);std::rethrow_exception(failure);
                }
                break;
            }
            case CameraOperation::Start:require(configured_ && !started_,"Basler not configured/idle");session_.start(r.profile.requested.trigger.enabled,budget);started_=true;break;
            case CameraOperation::Grab:{
                require(started_,"Basler not started");verifyApplied(r.setup,r.profile,readback());const auto f=session_.grab(budget);
                FrameMetadata m;m.sourceId=r.sourceId;m.frameId=QUuid::createUuid().toString(QUuid::WithoutBraces);m.cycleId="basler-unverified";m.coordinateSystemId=r.sourceId+"/image";
                m.completedAtUtc=QDateTime::currentDateTimeUtc();m.deviceTimestampTicks=f.timestamp;
                m.experimentalAcquisition={{"schema","visionflow.basler-unverified1"},{"triggerAssociationVerified",false},{"productionAdmission",false},{"backend","pylon-c-api"},{"blockId",QString::number(f.blockId)},{"readback",readback().toJson()}};
                auto image=ImageFrame::fromBuffer(f.pixels.constData(),size_t(f.pixels.size()),f.size.width(),f.size.height(),f.stride,PixelFormat::Gray8,m);
                budget();return {true,{},AcquiredFrame{image,m.sourceId,m.cycleId,m.frameId,m.completedAtUtc},readback()};
            }
            case CameraOperation::Stop:session_.stop();started_=false;break;
            case CameraOperation::Disconnect:session_.close();configured_=started_=false;return {true,{},{}};
            default:throw std::runtime_error("Unsupported Basler operation");
            }
            budget();return {true,{},{},readback()};
        }catch(...){fault_=true;throw;}
    }
    PhysicalDeviceIdentity actualIdentity() const override {auto id=device_.identity;
        id.vendor=text(setting("DeviceVendorName"));id.model=text(setting("DeviceModelName"));
        auto serial=setting("DeviceSerialNumber");if(!serial.readable())serial=setting("DeviceID");id.serial=text(serial);return id;}
    CameraFeature setting(const QString& name) const override {return session_.feature(name);}
    void writeSetting(const QString& name,const FeatureValue& value) override {
        require(QStringList{"ExposureTime","Gain","ExposureAuto","GainAuto"}.contains(name),"Unsupported Basler basic setting");session_.writeFeature(name,value);
    }
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
        if(request_)s.illumination=request_->profile.requested.illumination;
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
std::unique_ptr<IndustrialCameraBackend> BaslerAcquisition::create(const IndustrialCameraRequest& r,const QString& runtime,const QStringList& dependencies){
    r.validate();require(BaslerCameraSettings::unavailableReason(r.device,runtime).isEmpty(),"Basler acquisition unavailable");return std::make_unique<Backend>(r,runtime,dependencies);
}
}
