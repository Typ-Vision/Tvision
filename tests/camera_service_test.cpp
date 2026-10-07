#include "application/CameraService.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
#include <thread>
#include <chrono>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F action,const char* message){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,message);}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        check(app.arguments().size()==3,"fixture paths required");
        const QString file=app.arguments()[1],incomplete=app.arguments()[2];
        QLibrary fixture(file);check(fixture.load(),"load test fixture");
        const auto live=reinterpret_cast<int(*)()>(fixture.resolve("FixtureLiveHandles"));
        const auto mode=reinterpret_cast<void(*)(int)>(fixture.resolve("FixtureMode"));check(live && mode,"fixture controls");
        const auto original=qgetenv("GENICAM_GENTL64_PATH");qputenv("GENICAM_GENTL64_PATH","");
        QTemporaryDir directory(QDir(app.applicationDirPath()).filePath("camera-XXXXXX"));check(directory.isValid(),"test directory");
        const auto copy=directory.filePath("fixture.CTI");check(QFile::copy(file,copy),"copy uppercase producer");
        const auto ignored=directory.filePath("not-a-producer.txt");{QFile f(ignored);check(f.open(QIODevice::WriteOnly),"ignored file");f.write("text");}
        auto paths=genTLProducerPaths({directory.path(),directory.path()});check(paths.size()==1 && paths[0]==QFileInfo(copy).canonicalFilePath(),"path discovery deduplicates and filters");
        qputenv("GENICAM_GENTL64_PATH",directory.path().toUtf8());check(genTLProducerPaths()==paths,"environment path discovery");qputenv("GENICAM_GENTL64_PATH","");
        auto report=CameraService{}.discover();check(report.producers.isEmpty() && report.devices.isEmpty() && report.errors.isEmpty(),"no producer honest empty result");
        mode(0);report=CameraService{}.discover({file});check(report.errors.isEmpty() && report.devices.size()==1,"ABI discovery");
        const auto device=report.devices[0];check(device.interfaceId=="interface-1" && device.deviceId=="device-1" && device.serialNumber=="TEST-001" && device.vendor=="Synthetic test producer","device metadata");check(live()==0,"discovery cleanup");
        CameraService{}.checkConnection(device);check(live()==0,"open close cleanup");
        {GenTLTransport transport(file);transport.openReadOnly(device);check(transport.isDeviceOpen(),"opened device state");rejects([&]{transport.openReadOnly(device);},"duplicate open rejected");rejects([&]{transport.discover(1000);},"discovery while open rejected");transport.closeDevice();check(!transport.isDeviceOpen(),"closed state");transport.closeDevice();transport.openReadOnly(device);}
        {GenTLTransport transport(file);rejects([&]{GenTLTransport concurrent(file);},"concurrent session fails without deadlock");}
        check(live()==0,"destructor closes open device");
        mode(3);rejects([&]{CameraService{}.checkConnection(device);},"access denied");check(live()==0,"failed open cleanup");
        for(int failure:{1,2,6,7,9,12,13}){mode(failure);report=CameraService{}.discover({file});check(report.devices.isEmpty() && report.errors.size()==1,"failed discovery no devices");check(live()==0,"failed discovery releases handles");}
        mode(11);rejects([&]{CameraService{}.checkConnection(device);},"null device handle rejected");check(live()==0,"null device cleanup");
        mode(8);report=CameraService{}.discover({file},1);check(report.devices.isEmpty() && !report.errors.isEmpty() && live()==0,"late discovery suppressed");
        mode(4);report=CameraService{}.discover({file});check(report.devices.isEmpty() && report.errors.isEmpty() && live()==0,"zero interface producer");
        mode(5);report=CameraService{}.discover({file});check(report.devices.size()==1 && report.devices[0].serialNumber.isEmpty(),"optional unsupported metadata");
        mode(0);report=CameraService{}.discover({incomplete,file});check(report.errors.size()==1 && report.devices.size()==1 && live()==0,"producer failure isolation and missing export");
        report=CameraService{}.discover({directory.filePath("missing.cti")});check(report.devices.isEmpty() && report.errors.size()==1,"missing producer");
        const auto invalid=directory.filePath("invalid.cti");{QFile f(invalid);check(f.open(QIODevice::WriteOnly),"invalid producer");f.write("not a DLL");}
        report=CameraService{}.discover({invalid});check(report.errors.size()==1,"invalid binary");
        auto cancel=std::make_shared<std::atomic_bool>(true);report=CameraService{}.discover({file},1000,cancel);check(report.cancelled && report.devices.isEmpty() && live()==0,"pre-cancel discovery");
        mode(8);cancel->store(false);std::thread stopper([&]{std::this_thread::sleep_for(std::chrono::milliseconds(2));cancel->store(true);});report=CameraService{}.discover({file},1000,cancel);stopper.join();check(report.cancelled && report.devices.isEmpty() && live()==0,"cancel during producer call suppresses results");mode(0);
        report=CameraService{}.discover({file},0);check(report.errors.size()==1 && live()==0,"invalid timeout");
        {GenTLTransport transport(file);auto bad=device;bad.deviceId="missing";rejects([&]{transport.openReadOnly(bad);},"device disappeared");check(!transport.isDeviceOpen(),"failed open state");transport.openReadOnly(device);}
        check(live()==0,"recovery after disappeared device");qputenv("GENICAM_GENTL64_PATH",original);
        QTextStream(stdout)<<"PASS: synthetic GenTL C ABI discovery, open/close, cleanup, errors, deadline and recovery\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}
}
