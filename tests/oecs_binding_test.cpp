#include "hardware/OecsCApi.hpp"
#include <QCoreApplication>
#include <QLibrary>
#include <QTextStream>
#include <atomic>
#include <barrier>
#include <stdexcept>
#include <thread>
using namespace visionflow;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2,"fixture DLL argument required");
    const QString path=QString::fromLocal8Bit(argv[1]);QLibrary fixture(path);
    check(fixture.load(),"load fixture inspection exports");
    const auto serial=reinterpret_cast<const char*(*)(void*)>(fixture.resolve("testHandleSerial"));
    const auto marker=reinterpret_cast<unsigned int(*)(void*)>(fixture.resolve("testHandleMarker"));
    const auto overlap=reinterpret_cast<int(*)()>(fixture.resolve("testEnumerationOverlap"));
    check(serial && marker && overlap,"inspection exports available");
    auto a=makeOecsCApi(),b=makeOecsCApi();QString reason;
    check(a->load(path,reason) && b->load(path,reason),"load two actual dynamic bindings");
    QVector<OecsDeviceInfo> selected,other;
    check(a->enumerateUsb(selected)==0 && selected.size()==1,"enumerate A with null entries filtered");
    check(b->enumerateUsb(other)==0 && other.size()==1 && other[0].serial!=selected[0].serial,"B overwrites SDK enumeration storage");
    void* handle=nullptr;
    check(a->createHandle(0,&handle)==0,"create A after B enumeration");
    check(QString::fromLatin1(serial(handle))==selected[0].serial && marker(handle)==0x12345678,"create uses complete owned A record");
    CameraFeature feature;
    check(a->readFeature(handle,"Width",feature)==0&&feature.type==FeatureType::Integer&&std::get<qint64>(feature.value)==640&&feature.increment==1,"int feature ABI");
    check(a->writeFeature(handle,"Width",qint64(641))==0,"int write ABI");
    check(a->readFeature(handle,"ExposureTime",feature)==0&&feature.type==FeatureType::Float&&feature.unit=="us"&&std::get<double>(feature.value)==123.5,"float metadata ABI");
    check(a->writeFeature(handle,"ExposureTime",124.5)==0,"float write ABI");
    check(a->readFeature(handle,"PixelFormat",feature)==0&&std::get<QString>(feature.value)=="Mono8"&&feature.entries.value("Mono8")==17301505,"enum XML symbol ABI");
    check(a->readFeature(handle,"ReverseX",feature)==0&&std::get<bool>(feature.value)==false&&a->writeFeature(handle,"ReverseX",true)==0,"boolean ABI");
    check(a->readFeature(handle,"DeviceModelName",feature)==0&&std::get<QString>(feature.value)=="CAM","string ABI");
    check(a->readFeature(handle,"Missing",feature)==0&&feature.access==FeatureAccess::Missing,"missing node ABI");
    check(a->writeFeature(handle,"UserSetSave",qint64(1))==oecs::errorParameter,"no EEPROM commands");
    check(!a->unload() && a->destroyHandle(handle)==0,"live handle pins binding until destroy");
    check(a->createHandle(1,&handle)==oecs::errorParameter,"filtered index cannot select absent entry");
    check(a->unload() && b->unload(),"unload both bindings");
    check(a->readFeature(nullptr,"Width",feature)==oecs::errorLoadLibrary,"unloaded feature access rejected");
    std::atomic<bool> passed{true};std::barrier start(2);
    const auto worker=[&]{
        auto api=makeOecsCApi();QString localReason;
        const bool loaded=api->load(path,localReason);start.arrive_and_wait();
        if(!loaded){passed=false;return;}
        for(int i=0;i<40;++i){
            QVector<OecsDeviceInfo> devices;void* h=nullptr;
            if(api->enumerateUsb(devices)!=0 || devices.size()!=1 || api->createHandle(0,&h)!=0){passed=false;break;}
            if(QString::fromLatin1(serial(h))!=devices[0].serial || marker(h)!=0x12345678)passed=false;
            if(api->destroyHandle(h)!=0)passed=false;
        }
        if(!api->unload())passed=false;
    };
    std::thread first(worker),second(worker);first.join();second.join();
    check(passed && overlap()==0,"concurrent instances serialize enumeration and retain owned identity");
    QTextStream(stdout)<<"oecs_binding_test passed: interleaved identity, full record, concurrent enumeration"<<Qt::endl;
    return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<Qt::endl;return 1;}}
