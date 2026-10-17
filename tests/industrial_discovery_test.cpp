#include "application/CameraService.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <stdexcept>
#include <algorithm>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void reject(F f){try{f();}catch(const std::exception&){return;}throw std::runtime_error("expected rejection");}
DiscoveredDevice device(QString serial="one",QString producer={}){
    const QString backend=producer.isEmpty()?"oecs":"gentl";
    return {{"vendor","model",serial,DeviceIdentityConfidence::SerialReported},
        {backend,producer,"interface",serial},{backend,"1.0",QString(64,'a'),1}};
}
MeasurementCamera camera(){return {{"camera"},"Camera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},device().identity};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    IndustrialDiscoverySnapshot snapshot;auto registered=camera();
    check(CameraService::groupIndustrial(snapshot).isEmpty(),"zero discovery");
    reject([&]{CameraService::resolveIndustrial(registered,snapshot);});
    snapshot.devices={device()};const auto bytes=registered.toJson();
    check(CameraService::resolveIndustrial(registered,snapshot)==device(),"exact one resolution");
    for(int i=0;i<10;++i)snapshot.devices.append(device(QString::number(i)));
    check(CameraService::groupIndustrial(snapshot).size()==11,"N physical devices");
    std::reverse(snapshot.devices.begin(),snapshot.devices.end());
    check(CameraService::resolveIndustrial(registered,snapshot).identity==*registered.physical,"reorder cannot change CameraId binding");
    snapshot.devices={device(),device("one","C:/producer/one.cti"),device("one","C:/producer/two.cti")};
    auto groups=CameraService::groupIndustrial(snapshot);
    check(groups.size()==1 && groups[0].routes.size()==3 && !groups[0].ambiguous,"three routes one physical device");
    reject([&]{CameraService::resolveIndustrial(registered,snapshot);});
    const auto preferred=snapshot.devices[1];
    check(CameraService::resolveIndustrial(registered,snapshot,preferred.route)==preferred,"explicit local route");
    snapshot.devices.removeAt(1);
    reject([&]{CameraService::resolveIndustrial(registered,snapshot,preferred.route);});
    snapshot.devices={device(),device()};
    check(CameraService::groupIndustrial(snapshot)[0].ambiguous,"duplicate enumeration ambiguous");
    reject([&]{CameraService::resolveIndustrial(registered,snapshot,device().route);});
    auto foreign=device();foreign.identity.model="different";snapshot.devices={foreign};
    reject([&]{CameraService::resolveIndustrial(registered,snapshot,foreign.route);});
    auto incomplete=device();incomplete.identity.serial.clear();incomplete.identity.confidence=DeviceIdentityConfidence::ProviderReported;
    snapshot.devices={incomplete,incomplete};check(CameraService::groupIndustrial(snapshot).size()==2,"unknown serial never deduplicated");
    reject([&]{CameraService::resolveIndustrial(registered,snapshot);});
    snapshot.devices={device()};snapshot.cancelled=true;reject([&]{CameraService::resolveIndustrial(registered,snapshot);});
    snapshot.cancelled=false;snapshot.errors={"SDK missing"};reject([&]{CameraService::resolveIndustrial(registered,snapshot);});
    snapshot.errors.clear();snapshot.devices[0].backend.adapter="gentl";
    reject([&]{CameraService::groupIndustrial(snapshot);});
    check(registered.toJson()==bytes,"resolution never mutates portable registration");
    QTextStream(stdout)<<"PASS: offline/0/1/N, grouping, exact resolve, ambiguous and partial discovery rejection\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
