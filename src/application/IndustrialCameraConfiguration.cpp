#include "application/IndustrialCameraConfiguration.hpp"
#include <cmath>
#include <exception>
#include <stdexcept>
#include <algorithm>
namespace visionflow {
namespace {
void require(bool ok,const QString& why){if(!ok)throw std::runtime_error(why.toStdString());}
qint64 integer(const CameraFeature& f){require(f.readable()&&f.type==FeatureType::Integer,"Unreadable integer: "+f.name);return std::get<qint64>(f.value);}
QString text(const CameraFeature& f){require(f.readable()&&(f.type==FeatureType::String||f.type==FeatureType::Enumeration),"Unreadable text: "+f.name);return std::get<QString>(f.value);}
double numeric(const CameraFeature& f){require(f.readable(),"Unreadable numeric: "+f.name);if(f.type==FeatureType::Integer)return double(std::get<qint64>(f.value));require(f.type==FeatureType::Float,"Wrong numeric type: "+f.name);return std::get<double>(f.value);}
FeatureValue numberFor(const CameraFeature& f,double value){
    require(std::isfinite(value),"Non-finite request");if(f.type==FeatureType::Float)return value;
    require(f.type==FeatureType::Integer && std::trunc(value)==value && std::abs(value)<=9007199254740991.0,"Nonrepresentable integer: "+f.name);return qint64(value);
}
}
static IndustrialConfigurationResult applyConfiguration(OecsSession& session,const PhysicalDeviceIdentity& identity,const MeasurementSetupDescriptor& setup,const AcquisitionProfile& profile,bool experimental){
    identity.validate();require(identity.usable(),"Exact physical identity required");
    profile.validate();setup.validate();profile.acquisitionId(setup);
    const auto info=session.deviceInfo();require(info.vendor==identity.vendor&&info.model==identity.model&&info.serial==identity.serial,"Session identity mismatch");
    QMap<QString,CameraFeature> before;
    for(const auto& f:session.features())before.insert(f.name,f);
    if(experimental)for(const auto& n:QStringList{"TriggerDelay","AcquisitionBurstFrameCount","TriggerCacheEnable"})before.insert(n,session.feature(n));
    const auto initial=[&](const QString& n)->const CameraFeature& {const auto it=before.constFind(n);require(it!=before.cend(),"Missing feature: "+n);return it.value();};
    QStringList unimplemented;
    if(experimental)for(const auto& pair:{std::pair{"BinningHorizontal","BinningVertical"},std::pair{"DecimationHorizontal","DecimationVertical"}}){
        const bool x=initial(pair.first).access==FeatureAccess::Missing,y=initial(pair.second).access==FeatureAccess::Missing;
        require(x==y,"Mixed implemented/unimplemented geometry pair");
        if(x)unimplemented.append({pair.first,pair.second});
    }
    require(text(initial("DeviceVendorName"))==identity.vendor&&text(initial("DeviceModelName"))==identity.model&&text(initial("DeviceSerialNumber"))==identity.serial,"Device identity readback mismatch");
    const auto width=integer(initial("SensorWidth")),height=integer(initial("SensorHeight"));
    require(width>0&&height>0&&width<=65536&&height<=65536,"Invalid full sensor dimensions");
    // Until selector-bank transactions are implemented, a different active bank is rejected.
    const QString selector=experimental?"FrameBurstStart":"FrameStart";
    require(text(initial("TriggerSelector"))==selector,"Unsupported active trigger selector bank");
    const auto& r=profile.requested;
    require(r.exposureAuto==AutoMode::Off && r.gainAuto==AutoMode::Off,"Only manual exposure/gain supported");
    require(!r.trigger.enabled||(r.trigger.selector==selector&&r.trigger.source=="Software"),"Explicit supported selector/Software required");
    require(initial("ExposureTime").unit=="us"||initial("ExposureTime").unit==QString::fromUtf8("µs"),"Exposure unit not proven as microseconds");
    require(initial("Gain").unit=="dB","Gain unit not proven as dB");
    using Write=QPair<QString,FeatureValue>;
    QVector<Write> plan{{"TriggerMode",QString("Off")},{"ExposureAuto",QString("Off")},{"GainAuto",QString("Off")},
        {"OffsetX",qint64(0)},{"OffsetY",qint64(0)},{"BinningHorizontal",qint64(1)},{"BinningVertical",qint64(1)},
        {"DecimationHorizontal",qint64(1)},{"DecimationVertical",qint64(1)},{"ReverseX",false},{"ReverseY",false},
        {"PixelFormat",QString("Mono8")},{"Width",width},{"Height",height},{"AcquisitionMode",QString("Continuous")},
        {"ExposureTime",numberFor(initial("ExposureTime"),r.exposureTimeUs)},{"Gain",numberFor(initial("Gain"),r.gain)}};
    if(experimental){
        require(r.trigger.enabled,"Experimental software trigger required");
        plan.push_back({"AcquisitionBurstFrameCount",qint64(1)});
        plan.push_back({"TriggerCacheEnable",false});
        plan.push_back({"TriggerDelay",numberFor(initial("TriggerDelay"),0)});
    }
    // This COE XML exposes binning/decimation as enumerations whose documented
    // factor-one entry has raw value 1, rather than SFNC integer nodes.
    if(experimental)for(auto& w:plan){
        if(QStringList{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"}.contains(w.first) && initial(w.first).type==FeatureType::Enumeration){
            const auto symbol=w.first+"1";
            require(initial(w.first).entries.contains(symbol) && initial(w.first).entries.value(symbol)==1,"Unproven factor-one geometry entry");
            w.second=symbol;
        }
    }
    plan.erase(std::remove_if(plan.begin(),plan.end(),[&](const Write& w){return unimplemented.contains(w.first);}),plan.end());
    if(r.trigger.enabled)plan.push_back({"TriggerSource",QString("Software")});
    plan.push_back({"TriggerMode",QString(r.trigger.enabled?"On":"Off")});
    // Preflight readability/types, but re-evaluate dynamic range/access at each write.
    for(const auto& w:plan){const auto& f=initial(w.first);require(f.readable()&&f.value.index()==w.second.index(),"Missing/locked feature: "+w.first);}
    QVector<QString> touched;bool autoVerifiedBeforeTrigger=false;
    try {
        for(const auto& w:plan){
            if(experimental && w.first=="TriggerMode" && w.second==FeatureValue(QString("On")))continue;
            const auto f=session.feature(w.first);validateFeatureWrite(f,w.second);
            if(!featureEquivalent(f,w.second)){
                // An SDK error may still have changed the register. Include the attempted write.
                if(!touched.contains(w.first))touched.push_back(w.first);
                session.writeFeature(w.first,w.second);
            }
        }
        if(experimental){
            require(text(session.feature("ExposureAuto"))=="Off" && text(session.feature("GainAuto"))=="Off","Auto readback before trigger mismatch");
            autoVerifiedBeforeTrigger=true;
            if(!touched.contains("TriggerMode"))touched.push_back("TriggerMode");
            session.writeFeature("TriggerMode",QString("On"));
        }
        IndustrialConfigurationResult result;
        auto& a=result.actual;auto& g=a.geometry;
        const auto intNow=[&](const char* n){const auto v=integer(session.feature(n));require(v>=0&&v<=65536,"Geometry out of bounds");return int(v);};
        g.resolution={intNow("Width"),intNow("Height")};g.offset={intNow("OffsetX"),intNow("OffsetY")};g.offsetEvidence=GeometryEvidence::Reported;
        const auto factorNow=[&](const char* n){const auto f=session.feature(n);
            if(experimental && f.type==FeatureType::Enumeration){const auto symbol=QString(n)+"1";
                require(f.entries.contains(symbol) && f.entries.value(symbol)==1 && text(f)==symbol,"Geometry factor-one readback mismatch");return 1;}
            return intNow(n);};
        const auto missingPair=[&](const char* x,const char* y){if(!unimplemented.contains(x))return false;
            require(session.feature(x).access==FeatureAccess::Missing && session.feature(y).access==FeatureAccess::Missing,"Geometry implementation changed");return true;};
        if(missingPair("BinningHorizontal","BinningVertical")){g.binning={1,1};g.binningEvidence=GeometryEvidence::NotApplicable;}
        else {g.binning={factorNow("BinningHorizontal"),factorNow("BinningVertical")};g.binningEvidence=GeometryEvidence::Reported;}
        if(missingPair("DecimationHorizontal","DecimationVertical")){g.decimation={1,1};g.decimationEvidence=GeometryEvidence::NotApplicable;}
        else {g.decimation={factorNow("DecimationHorizontal"),factorNow("DecimationVertical")};g.decimationEvidence=GeometryEvidence::Reported;}
        const auto boolNow=[&](const char* n){const auto f=session.feature(n);require(f.readable()&&f.type==FeatureType::Boolean,"Unknown reverse");return std::get<bool>(f.value);};
        g.reverse={boolNow("ReverseX"),boolNow("ReverseY")};g.reverseEvidence=GeometryEvidence::Reported;
        a.settings=r; // illumination is an operator declaration, not a device node
        a.settings.pixelFormat=text(session.feature("PixelFormat"));
        const auto autoOff=[&](const char* n){const auto f=session.feature(n);
            if(experimental && autoVerifiedBeforeTrigger && f.access==FeatureAccess::Unavailable)return true;
            return text(f)=="Off";};
        require(autoOff("ExposureAuto")&&autoOff("GainAuto"),"Auto readback mismatch");
        a.settings.exposureTimeUs=numeric(session.feature("ExposureTime"));a.settings.gain=numeric(session.feature("Gain"));
        const auto mode=text(session.feature("TriggerMode"));require(mode=="On"||mode=="Off","Unknown trigger mode");
        a.settings.trigger.enabled=mode=="On";
        if(a.settings.trigger.enabled){a.settings.trigger.selector=text(session.feature("TriggerSelector"));a.settings.trigger.source=text(session.feature("TriggerSource"));}
        else {a.settings.trigger.selector.clear();a.settings.trigger.source.clear();}
        require(text(session.feature("AcquisitionMode"))=="Continuous","Acquisition mode readback mismatch");
        const auto payload=integer(session.feature("PayloadSize"));
        require(payload>=width*height&&payload<=512LL*1024*1024,"Invalid/bounded payload readback");result.payloadBytes=quint64(payload);
        // Verify every target again after the complete transaction (dependent writes can alter earlier nodes).
        for(const auto& w:plan){if(w.first=="TriggerMode"&&std::get<QString>(w.second)=="Off"&&r.trigger.enabled)continue;
            if(experimental && autoVerifiedBeforeTrigger && (w.first=="ExposureAuto" || w.first=="GainAuto") && session.feature(w.first).access==FeatureAccess::Unavailable)continue;
            require(featureEquivalent(session.feature(w.first),w.second),"Final readback differs: "+w.first);}
        require(text(session.feature("DeviceVendorName"))==identity.vendor&&text(session.feature("DeviceModelName"))==identity.model&&text(session.feature("DeviceSerialNumber"))==identity.serial,"Final identity readback mismatch");
        result.acquisitionId=verifyApplied(setup,profile,a);return result;
    }catch(...){
        const auto original=std::current_exception();bool restored=true;
        // Safe restore order: disable dependent modes, neutral offsets, restore geometry,
        // exposure/gain, then source and original modes. Verify the complete original inventory.
        const auto restore=[&](const QString& n,const FeatureValue& value){if(unimplemented.contains(n))return;try{session.writeFeature(n,value);}catch(...){restored=false;}};
        if(!touched.isEmpty()){
            restore("TriggerMode",QString("Off"));restore("ExposureAuto",QString("Off"));restore("GainAuto",QString("Off"));
            restore("OffsetX",qint64(0));restore("OffsetY",qint64(0));
            for(const auto& n:QStringList{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical","ReverseX","ReverseY","PixelFormat","Width","Height","OffsetX","OffsetY","AcquisitionMode","ExposureTime","Gain","TriggerSource","ExposureAuto","GainAuto","TriggerMode"})restore(n,initial(n).value);
            if(experimental)for(const auto& n:QStringList{"AcquisitionBurstFrameCount","TriggerCacheEnable","TriggerDelay"})restore(n,initial(n).value);
            for(auto it=before.cbegin();it!=before.cend();++it){if(!it->readable())continue;
                try{require(featureEquivalent(session.feature(it.key()),it->value),"Rollback readback differs");}catch(...){restored=false;}}
        }
        if(!restored){session.configurationFault();throw OecsSessionError(OecsFailure::ConfigurationFailed,"rollback",0,"Configuration failed; rollback unverified; session Fault");}
        std::rethrow_exception(original);
    }
}
IndustrialConfigurationResult IndustrialCameraConfiguration::apply(OecsSession& s,const PhysicalDeviceIdentity& i,const MeasurementSetupDescriptor& g,const AcquisitionProfile& p){return applyConfiguration(s,i,g,p,false);}
IndustrialConfigurationResult IndustrialCameraConfiguration::applyExperimental(OecsSession& s,const PhysicalDeviceIdentity& i,const MeasurementSetupDescriptor& g,const AcquisitionProfile& p){
    i.validate();g.validate();p.validate();p.acquisitionId(g);
    const auto info=s.deviceInfo();require(info.vendor==i.vendor && info.model==i.model && info.serial==i.serial,"Session identity mismatch");
    require(p.requested.trigger==TriggerSettings{true,"FrameBurstStart","Software"},"Explicit experimental trigger profile required");
    const auto mode=s.feature("TriggerMode");require(text(mode)=="Off" || text(mode)=="On","Unknown trigger mode");
    // Auto controls are dynamically unavailable while triggered on this camera.
    // Snapshot/verify their actual Off values while the trigger bank is disabled.
    s.writeFeature("TriggerMode",QString("Off"));
    try{return applyConfiguration(s,i,g,p,true);}catch(...){
        const auto failure=std::current_exception();
        if(s.stage()==OecsSessionStage::Open)try{s.writeFeature("TriggerMode",mode.value);}catch(...){s.configurationFault();throw;}
        std::rethrow_exception(failure);
    }
}
}
