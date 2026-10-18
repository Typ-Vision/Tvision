#include "domain/IndustrialSourceReference.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f){try{f();}catch(const std::exception&){return;}throw std::runtime_error("expected rejection");}
MeasurementSetupContract fixture(){
    MeasurementSetupContract c;
    c.cameras.append({{"camera"},"Camera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},PhysicalDeviceIdentity{"vendor","model","serial",DeviceIdentityConfidence::SerialReported}});
    c.setups.append({{"setup"},{"camera"},"Setup","lens","plane",1,1,{{2448,2048},QRect(0,0,2448,2048),QSize(1,1),true,QSize(1,1),GeometryMirror{}}});
    AcquisitionSettings settings;settings.pixelFormat="Mono8";settings.exposureTimeUs=10000;settings.trigger={true,"FrameBurstStart","Software"};
    c.profiles.append({{"profile"},{"camera"},{"setup"},"Profile",1,settings});c.validate();return c;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    auto c=fixture();const auto environmentBytes=QJsonDocument(c.toJson()).toJson(QJsonDocument::Compact);
    const auto r=IndustrialSourceReference::bind(c,{"profile"},5000);const auto json=r.toJson();
    const auto bytes=QJsonDocument(json).toJson(QJsonDocument::Compact);
    check(IndustrialSourceReference::fromJson(json)==r,"exact reference roundtrip");
    check(!bytes.contains("producer") && !bytes.contains("runtimeSha256"),"no local backend route in portable reference");
    check(environmentBytes==QJsonDocument(c.toJson()).toJson(QJsonDocument::Compact),"binding never writes environment");
    auto renamed=c;renamed.cameras[0].name="Renamed";renamed.setups[0].name="Renamed";renamed.profiles[0].name="Renamed";
    r.validateAgainst(renamed);check(IndustrialSourceReference::bind(renamed,{"profile"},5000)==r,"renames preserve reference/hash");
    for(const auto& key:json.keys()){auto malformed=json;malformed.remove(key);reject([&]{IndustrialSourceReference::fromJson(malformed);});}
    for(const QString key:{"backend","producer","permission"}){auto bad=json;bad[key]="untrusted";reject([&]{IndustrialSourceReference::fromJson(bad);});}
    for(const QString key:{"schemaVersion","deviceRevision","setupRevision","profileRevision","timeoutMs"})
        for(const QJsonValue value:{QJsonValue(0),QJsonValue(-1),QJsonValue(1.5),QJsonValue("1"),QJsonValue(true),QJsonValue(std::numeric_limits<double>::infinity())}){
            auto bad=json;bad[key]=value;reject([&]{IndustrialSourceReference::fromJson(bad);});
        }
    for(const QString key:{"format","kind","acquisitionId","profileId"}){auto bad=json;bad[key]="unknown";if(key=="profileId"){
        auto dangling=IndustrialSourceReference::fromJson(bad);reject([&]{dangling.validateAgainst(c);});
    }else reject([&]{IndustrialSourceReference::fromJson(bad);});}
    for(int scenario=0;scenario<10;++scenario){
        auto changed=c;
        switch(scenario){
        case 0:changed.profiles.clear();break;
        case 1:++changed.profiles[0].revision;break;
        case 2:++changed.setups[0].revision;break;
        case 3:++changed.cameras[0].device.revision;++changed.setups[0].deviceRevision;break;
        case 4:changed.cameras[0].physical->serial="other";break;
        case 5:changed.profiles[0].requested.exposureTimeUs+=1;break; // same revision overwrite still fails
        case 6:changed.setups[0].geometry.reverse->x=true;break;
        case 7:changed.profiles[0].requested.trigger.source="Line0";break;
        case 8:changed.profiles[0].requested.trigger={};break;
        case 9:changed.profiles[0].requested.trigger.selector="unknown";break;
        }
        reject([&]{r.validateAgainst(changed);});
        if(scenario>=7)reject([&]{IndustrialSourceReference::bind(changed,{"profile"},5000);});
    }
    reject([&]{IndustrialSourceReference::bind(c,{"profile"},10);}); // exposure alone exhausts deadline
    reject([&]{IndustrialSourceReference::bind(c,{"profile"},600001);});
    auto changed=c;changed.profiles[0].requested.trigger.selector="FrameStart";
    auto frameStart=IndustrialSourceReference::bind(changed,{"profile"},5000);
    check(frameStart.acquisitionId!=r.acquisitionId,"FrameBurstStart never aliases FrameStart");
    check(ToolRegistry::get("industrial_camera").acquisitionSource,"industrial source registered explicitly");
    changed=c;changed.profiles[0].requested.pixelFormat="Mono12";
    reject([&]{IndustrialSourceReference::bind(changed,{"profile"},5000);});
    changed=c;changed.profiles[0].requested.exposureAuto=AutoMode::Continuous;
    reject([&]{IndustrialSourceReference::bind(changed,{"profile"},5000);});
    QTextStream(stdout)<<"PASS: standalone reference, strict codec, revisions, physical identity, settings/geometry and trigger rejection\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
