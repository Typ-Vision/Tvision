#include "application/IndustrialCameraConfiguration.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <functional>
#include <thread>
#include <cmath>
using namespace visionflow;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void rejects(const std::function<void()>& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected explicit rejection");}
struct State {
    QMap<QString,CameraFeature> nodes;
    QStringList writes;
    QString fail,clamp,corrupt,finalMutation;
    bool failed=false,rollbackFail=false,dynamicLock=false;
    int live=0;bool unloaded=false;
};
CameraFeature integer(QString n,qint64 v,double low=0,double high=65536){CameraFeature f;f.name=n;f.type=FeatureType::Integer;f.access=FeatureAccess::ReadWrite;f.value=v;f.minimum=low;f.maximum=high;f.increment=1;return f;}
CameraFeature enumeration(QString n,QString v,QStringList entries){CameraFeature f;f.name=n;f.type=FeatureType::Enumeration;f.access=FeatureAccess::ReadWrite;f.value=v;for(int i=0;i<entries.size();++i)f.entries.insert(entries[i],i);return f;}
std::shared_ptr<State> state(){auto s=std::make_shared<State>();auto put=[&](CameraFeature f){s->nodes.insert(f.name,f);};
    for(auto n:{"Width","SensorWidth"})put(integer(n,640));for(auto n:{"Height","SensorHeight"})put(integer(n,480));
    for(auto n:{"OffsetX","OffsetY"})put(integer(n,0));
    for(auto n:{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"})put(integer(n,1,1,4));
    put(integer("PayloadSize",640*480,0,512*1024*1024));
    for(auto n:{"DeviceVendorName","DeviceModelName","DeviceSerialNumber"}){CameraFeature f;f.name=n;f.type=FeatureType::String;f.access=FeatureAccess::ReadOnly;f.value=QString(n==QString("DeviceVendorName")?"VF":n==QString("DeviceModelName")?"CAM":"SN");put(f);}
    for(auto n:{"ReverseX","ReverseY"}){CameraFeature f;f.name=n;f.type=FeatureType::Boolean;f.access=FeatureAccess::ReadWrite;f.value=false;put(f);}
    put(enumeration("PixelFormat","Mono8",{"Mono8","Mono12"}));put(enumeration("ExposureAuto","Continuous",{"Off","Continuous"}));put(enumeration("GainAuto","Continuous",{"Off","Continuous"}));
    put(enumeration("AcquisitionMode","Continuous",{"Continuous","SingleFrame"}));put(enumeration("TriggerSelector","FrameStart",{"FrameStart","AcquisitionStart"}));put(enumeration("TriggerSource","Line1",{"Line1","Software"}));put(enumeration("TriggerMode","Off",{"Off","On"}));
    for(auto n:{"ExposureTime","Gain"}){CameraFeature f;f.name=n;f.type=FeatureType::Float;f.access=FeatureAccess::ReadWrite;f.value=n==QString("Gain")?1.:100.;f.minimum=0;f.maximum=10000;f.unit=n==QString("Gain")?"dB":"us";put(f);}
    return s;
}
class Api:public OecsCApi {
    std::shared_ptr<State> s;
public:
    explicit Api(std::shared_ptr<State> s):s(std::move(s)){}
    bool load(const QString&,QString&) override{return true;}bool unload() override{s->unloaded=true;return s->live==0;}
    quint32 sdkVersion() override{return 1;}int enumerateUsb(QVector<OecsDeviceInfo>& d) override{d={{"VF","CAM","SN"}};return 0;}
    int createHandle(int,void** h) override{*h=this;++s->live;return 0;}int openExclusive(void*) override{return 0;}int closeDevice(void*) override{return 0;}int destroyHandle(void*) override{--s->live;return 0;}
    int readFeature(void*,const QString& n,CameraFeature& f) override{if(!s->nodes.contains(n)){f={};f.name=n;f.access=FeatureAccess::Missing;return 0;}f=s->nodes[n];if(s->dynamicLock&&n=="ExposureTime")f.access=std::get<QString>(s->nodes["ExposureAuto"].value)=="Off"?FeatureAccess::ReadWrite:FeatureAccess::ReadOnly;return 0;}
    int writeFeature(void*,const QString& n,const FeatureValue& v) override{
        s->writes<<n;auto& f=s->nodes[n];f.value=v;
        if(s->rollbackFail&&s->failed)return oecs::errorParameter;
        if(n==s->fail&&!s->failed){s->failed=true;if(!s->corrupt.isEmpty())s->nodes[s->corrupt].value=qint64(2);return oecs::errorParameter;}
        if(n==s->clamp&&!s->failed){f.value=std::get<double>(v)-1.;s->failed=true;}
        if(n=="TriggerMode"&&v==FeatureValue(QString("On"))&&!s->finalMutation.isEmpty())s->nodes[s->finalMutation].value=qint64(2);
        if(f.type==FeatureType::Float)f.value=double(float(std::get<double>(f.value)));
        return 0;
    }
};
struct Fixture {
    std::shared_ptr<State> s=state();
    OecsSession session{std::make_unique<Api>(s),QDir::temp().absoluteFilePath("fake-camera.dll")};
    PhysicalDeviceIdentity identity{"VF","CAM","SN",DeviceIdentityConfidence::SerialReported};
    MeasurementSetupDescriptor setup{{"setup"},{"camera"},"Setup","lens","plane",1,1,{{640,480},QRect(0,0,640,480),QSize(1,1),true,QSize(1,1),GeometryMirror{}}};
    AcquisitionProfile profile{{"profile"},{"camera"},{"setup"},"Manual",1,{"Mono8",{true,"FrameStart","Software"},AutoMode::Off,500.1,AutoMode::Off,2.1,"declared light"}};
    Fixture(){session.open({"VF","CAM","SN"});}
    auto apply(){return IndustrialCameraConfiguration::apply(session,identity,setup,profile);}
};
void originalRestored(const Fixture& f,const QMap<QString,CameraFeature>& original){for(auto it=original.cbegin();it!=original.cend();++it)check(f.s->nodes[it.key()].value==it->value,"fixture state equals every original value");check(f.session.stage()==OecsSessionStage::Open,"successful rollback keeps idle session");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    {Fixture f;const auto r=f.apply();check(r.payloadBytes==640*480&&r.acquisitionId==f.profile.acquisitionId(f.setup),"readback produces profile identity");check(r.actual.settings.exposureTimeUs==double(float(500.1))&&r.actual.settings.exposureTimeUs!=500.1,"actual float preserved");check(!f.s->writes.contains("UserSetSave"),"no EEPROM writes");}
    {Fixture f;f.profile.requested.trigger={false,{},{}};check(!f.apply().actual.settings.trigger.enabled,"free run readback");}
    {Fixture f;f.s->nodes["ExposureTime"]=integer("ExposureTime",100,1,10000);f.s->nodes["ExposureTime"].unit="us";f.profile.requested.exposureTimeUs=500;check(f.apply().actual.settings.exposureTimeUs==500,"integer exposure supported");}
    {Fixture f;f.s->dynamicLock=true;check(!f.apply().acquisitionId.isEmpty(),"access refreshed after auto is disabled");}
    {Fixture f;f.s->nodes["ExposureTime"]=integer("ExposureTime",100,0,10000);f.s->nodes["ExposureTime"].unit="us";f.s->nodes["ExposureTime"].increment=10;f.profile.requested.exposureTimeUs=505;const auto before=f.s->nodes;rejects([&]{f.apply();});originalRestored(f,before);}
    for(int scenario=0;scenario<11;++scenario){Fixture f;auto& n=f.s->nodes;
        if(scenario==0)n["Gain"].access=FeatureAccess::ReadOnly;
        if(scenario==1)n.remove("ReverseX");if(scenario==2)n["Gain"].access=FeatureAccess::Unavailable;
        if(scenario==3)f.profile.requested.exposureTimeUs=10001;
        if(scenario==4){n["ExposureTime"].type=FeatureType::Integer;n["ExposureTime"].value=qint64(100);n["ExposureTime"].increment=10;}
        if(scenario==5)n["ExposureTime"].unit="ms";
        if(scenario==6)n["Gain"].unit.clear();
        if(scenario==7)n["TriggerSelector"].value=QString("AcquisitionStart");
        if(scenario==8)n["DeviceSerialNumber"].value=QString("other");
        if(scenario==9)f.profile.requested.trigger.source="Line1";
        if(scenario==10)f.profile.requested.exposureTimeUs=std::nan("");
        const auto before=n;rejects([&]{f.apply();});originalRestored(f,before);
    }
    for(const auto& node:QStringList{"ExposureAuto","GainAuto","ExposureTime","Gain","TriggerSource","TriggerMode"}){
        Fixture f;const auto before=f.s->nodes;f.s->fail=node;rejects([&]{f.apply();});check(f.s->failed,"failure injected after attempted write");originalRestored(f,before);
    }
    {Fixture f;const auto before=f.s->nodes;f.s->nodes["Gain"].access=FeatureAccess::WriteOnly;rejects([&]{f.apply();});check(f.s->writes.isEmpty(),"write-only feature rejected before mutation");}
    {auto value=integer("OffsetX",0);value.access=FeatureAccess::Unavailable;check(!featureEquivalent(value,qint64(0)),"inaccessible zero is not readback evidence");}
    {Fixture f;CameraFeature value=f.s->nodes["ExposureTime"];value.value=double(std::nextafter(float(500.1),0.0f));check(!featureEquivalent(value,500.1),"adjacent float clamp is not representation rounding");}
    // Exercise every geometry transition and reverse restore from a cropped/binned state.
    for(const auto& node:QStringList{"OffsetX","OffsetY","BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical","ReverseX","ReverseY","PixelFormat","Width","Height","AcquisitionMode"}){
        Fixture f;auto& n=f.s->nodes;n["OffsetX"].value=qint64(8);n["OffsetY"].value=qint64(8);
        for(auto key:{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"})n[key].value=qint64(2);
        n["ReverseX"].value=true;n["ReverseY"].value=true;n["PixelFormat"].value=QString("Mono12");n["Width"].value=qint64(320);n["Height"].value=qint64(240);n["AcquisitionMode"].value=QString("SingleFrame");
        const auto before=n;f.s->fail=node;rejects([&]{f.apply();});check(f.s->failed,"geometry failure injection reached");originalRestored(f,before);
    }
    {Fixture f;const auto before=f.s->nodes;f.s->clamp="ExposureTime";rejects([&]{f.apply();});originalRestored(f,before);}
    {Fixture f;f.s->fail="Gain";f.s->rollbackFail=true;rejects([&]{f.apply();});check(f.session.stage()==OecsSessionStage::Faulted,"unverified rollback faults");rejects([&]{f.apply();});f.session.close();check(f.s->live==0&&f.s->unloaded,"fault still cleans up owner handle");}
    {Fixture f;const auto before=f.s->nodes;f.s->finalMutation="OffsetX";rejects([&]{f.apply();});originalRestored(f,before);}
    {Fixture f;f.s->nodes["PayloadSize"].value=qint64(1);rejects([&]{f.apply();});check(f.session.stage()==OecsSessionStage::Open,"invalid payload never produces result");}
    {Fixture f;bool rejected=false;const auto count=f.s->writes.size();std::thread t([&]{try{f.apply();}catch(const OecsSessionError& e){rejected=e.failure()==OecsFailure::WrongThread;}});t.join();check(rejected&&f.s->writes.size()==count,"foreign worker has no SDK writes");}
    QTextStream(stdout)<<"industrial_configuration_test passed: typed features, units, range/access, int/float, clamp, partial write, rollback/Fault, cleanup"<<Qt::endl;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<Qt::endl;return 1;}}
