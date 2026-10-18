#include "application/CameraService.hpp"
#include "application/MeasurementWorkspace.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
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
    snapshot.devices={device(),device("one","C:/producer/one.cti")};
    const DeviceBinding webcam{"webcam","71742d646576696365",DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}};
    const auto inventory=CameraService::combine({webcam},snapshot);
    check(inventory.devices.size()==2 && inventory.devices[1].industrial->routes.size()==2,"unified webcam/industrial inventory deduplicates routes");
    QTemporaryDir dir(QDir::currentPath()+"/industrial-registry-XXXXXX");check(dir.isValid(),"temporary registry");
    bool idle=true;const auto path=dir.path()+"/project.vfproject";
    MeasurementWorkspace workspace(path,"project",[&]{return idle;},[&]{return QVector<DeviceBinding>{webcam};});
    workspace.observeInventory(inventory);
    const auto cameraId=workspace.registerCamera(inventory.devices[1],"Industrial");
    workspace.registerCamera(inventory.devices[0],"Webcam");
    const auto e=workspace.environment();
    auto adapter=CameraProviderFactory{}.webcam(e.current.cameras[1],{webcam},QSize(640,480));
    check(adapter->capabilities().softwareTrigger,"shared factory preserves webcam capability");
    reject([&]{CameraProviderFactory{}.webcam(e.current.cameras[1],{});});
    check(e.current.cameras.size()==2 && e.current.cameras[0].id==cameraId,"shared persistent CameraId");
    reject([&]{workspace.registerCamera(inventory.devices[1],"Duplicate");});
    idle=false;reject([&]{workspace.registerCamera(inventory.devices[1],"Busy");});idle=true;
    MeasurementWorkspace restarted(path,"project",[]{return true;});
    check(restarted.environment().current.cameras[0].id==cameraId && restarted.availability(e.current.cameras[0]).contains("Keşif yapılmadı"),"restart keeps registration without availability permission");
    auto offline=CameraService::combine({},{});workspace.observeInventory(offline);
    check(workspace.availability(e.current.cameras[0]).contains("Çevrimdışı"),"unplug availability lost");
    reject([&]{workspace.registerCamera(inventory.devices[1],"Old discovery");});
    CameraLocalConfiguration local(dir.path()+"/local.ini");
    local.setRoute(device().identity,device().route);
    check(local.route(device().identity)==device().route && !local.route(device("other").identity),"local exact route persistence");
    check(!QJsonDocument(e.encode()).toJson().contains("C:/producer"),"transport never exported in project registry");
    const auto producer=dir.path()+"/fake.cti";QFile binary(producer);check(binary.open(QIODevice::WriteOnly),"fake producer file");binary.write("fixture");binary.close();
    local.setProducers({producer,producer});check(local.producers().size()==1,"remember local producer without repeated selection");
    CameraDiscovery raw;raw.devices={{producer,"if","dev","vendor","model","one","GigE"}};
    auto actual=CameraService::industrialSnapshot(raw);
    check(actual.errors.isEmpty() && actual.devices.size()==1 && actual.devices[0].identity==device().identity && actual.devices[0].backend.runtimeSha256.size()==64,"discovery converts actual identity and binary fingerprint");
    check(!CameraProviderFactory{}.capabilities(actual.devices[0]).softwareTrigger,"discovery cannot grant native capture");
    raw.devices[0].producerPath=dir.path()+"/missing.cti";
    actual=CameraService::industrialSnapshot(raw);check(!actual.errors.isEmpty() && actual.devices.isEmpty(),"missing SDK route explicit failure");
    QTextStream(stdout)<<"PASS: offline/0/1/N, grouping, exact resolve, ambiguous and partial discovery rejection\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
