#include "hardware/PylonSession.hpp"
#include "application/CameraLeaseArbiter.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QLibrary>
#include <QTemporaryDir>
#include <QFile>
#include <iostream>
#include <thread>
using namespace visionflow;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> QString rejects(F&& fn){try{fn();}catch(const std::exception& e){return QString::fromUtf8(e.what());}throw std::runtime_error("Expected rejection");}
QJsonObject node(const QJsonObject& report,const QString& name){for(const auto v:report["nodes"].toArray())if(v.toObject()["name"]==name)return v.toObject();throw std::runtime_error("Missing inventory entry");}
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        const PylonSelector selector{"Basler","acA1920-40um","23102780"};
        if(argc==1){check(!PylonSession::available(),"SDK absent build");PylonSession session;
            check(rejects([&]{session.open("missing",{},selector);}).contains("unavailable"),"Explicit no SDK failure");return 0;}
        const QString path=QString::fromLocal8Bit(argv[1]);QLibrary fixture(path);check(fixture.load(),"Load fixture");
        auto mode=reinterpret_cast<void(*)(int)>(fixture.resolve("fixtureMode"));
        auto resources=reinterpret_cast<int(*)()>(fixture.resolve("fixtureResources"));
        auto selected=reinterpret_cast<int(*)()>(fixture.resolve("fixtureSelected"));check(mode && resources && selected,"Fixture inspection exports");
        check(PylonSession::available() && PylonSession::abi()["deviceInfoBytes"]==708,"Header ABI self check");
        PylonSession session;
        QTemporaryDir temporary(app.applicationDirPath()+"/pylon-XXXXXX");check(temporary.isValid(),"Temporary DLL fixture");
        const auto broken=temporary.filePath("broken.dll");
        {QFile file(broken);check(file.open(QIODevice::WriteOnly),qPrintable(file.errorString()));check(file.write("bad DLL")>0,qPrintable(file.errorString()));}
        check(!rejects([&]{session.open(broken,{},selector);}).isEmpty() && resources()==0,"Malformed DLL cleanup");
        check(rejects([&]{session.open(path,{}, {"Basler","acA1920-40um",{}});}).contains("Exact"),"Reject empty identity");
        check(!rejects([&]{session.open("not-absolute",{},selector);}).isEmpty() && resources()==0,"Bad DLL cleanup");
        check(rejects([&]{session.open(QString::fromLocal8Bit(argv[2]),{},selector);}).contains("export"),"Missing export cleanup");
        const auto report=session.open(path,{},selector);
        check(selected()==1 && resources()==3,"Exact match selects second device");
        check(report["identity"].toObject()["serial"]==selector.serial && report["exclusive"].toBool(),"Exact readback/exclusive");
        check(node(report,"Width")["max"]=="1936" && node(report,"Width")["increment"]=="2","Integer metadata");
        check(node(report,"ExposureTime")["unit"]=="us" && node(report,"ExposureTime")["type"]=="float","Float metadata");
        check(node(report,"PixelFormat")["entries"].toArray()[0].toObject()["symbol"]=="Mono8","Enum metadata");
        check(node(report,"SensorHeight")["access"]=="missing" && !node(report,"AcquisitionAbort").contains("value"),"No fabricated missing/command value");
        check(rejects([&]{session.open(path,{},selector);}).contains("active"),"Double open rejected");
        {PylonSession other;check(rejects([&]{other.open(path,{},selector);}).contains("busy"),"Init/enumeration lifetime serialized");}
        QString error;std::thread wrong([&]{error=rejects([&]{session.close();});});wrong.join();check(error.contains("thread") && resources()==3,"Wrong thread makes no SDK call");
        session.close();session.close();check(resources()==0,"Idempotent close releases all resources");
        for(const int failure:{1,2,3,4,5,6,7,11,12,13,14,15}){
            mode(failure);const auto reason=rejects([&]{session.open(path,{},selector);});
            check(resources()==0,"Failed open cleans handles/init");
            if(failure==3)check(reason.contains("0xc2000001") && reason.contains("fixture vendor error"),"Return code and immediate vendor detail");
            mode(0);session.open(path,{},selector);session.close();check(resources()==0,"Reopen after failure");
        }
        for(int failure:{8,9,10}){
            session.open(path,{},selector);mode(failure);rejects([&]{session.close();});check(resources()>0,"Cleanup error retains resources/library");
            mode(0);session.close();check(resources()==0,"Explicit owner retry completes cleanup");
        }
        {PylonSession scoped;scoped.open(path,{},selector);}check(resources()==0,"Destructor cleanup");
        const PhysicalDeviceIdentity p{selector.vendor,selector.model,selector.serial,DeviceIdentityConfidence::SerialReported};
        {auto lease=CameraLeaseArbiter::reserve("native",CameraLeaseUse::Preview,{{"native-id",p}});
            auto conflict=CameraLeaseArbiter::reserve("gentl",CameraLeaseUse::Editor,{{"gentl-id",p}});
            check(!lease.reason && conflict.reason==3,"Common physical alias exclusion");}
        check(CameraLeaseArbiter::diagnostics().isEmpty(),"No physical lease leaks");
        std::cout<<"pylon binding / ABI / exact identity / failures / cleanup passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
