#include "application/CameraService.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include "hardware/RuntimeLibrary.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
#include <thread>
#include <chrono>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F action,const char* message){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,message);}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        check(app.arguments().size()>=3,"fixture paths required");
        const QString file=app.arguments()[1],incomplete=app.arguments()[2];
        QLibrary fixture(file);check(fixture.load(),"load test fixture");
        const auto live=reinterpret_cast<int(*)()>(fixture.resolve("FixtureLiveHandles"));
        const auto mode=reinterpret_cast<void(*)(int)>(fixture.resolve("FixtureMode"));check(live && mode,"fixture controls");
        const auto maxScanWait=reinterpret_cast<uint64_t(*)()>(fixture.resolve("FixtureMaxScanWait"));check(maxScanWait,"fixture scan wait control");
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
        mode(14);report=CameraService{}.discover({file},1000,{},100);check(report.errors.isEmpty() && report.devices.size()==2 && maxScanWait()==100 && live()==0,"full scan waits across interfaces leave enumeration budget");
        mode(14);report=CameraService{}.discover({file},1000,{},2000);check(report.errors.isEmpty() && report.devices.size()==2 && maxScanWait()<=500 && live()==0,"scan wait larger than total budget is safely capped");
        mode(15);report=CameraService{}.discover({file},200,{},20);check(report.devices.isEmpty() && report.errors.size()==1 && live()==0,"late metadata rejects entire producer result and cleans handles");
        mode(4);report=CameraService{}.discover({file});check(report.devices.isEmpty() && report.errors.isEmpty() && live()==0,"zero interface producer");
        mode(5);report=CameraService{}.discover({file});check(report.devices.size()==1 && report.devices[0].serialNumber.isEmpty(),"optional unsupported metadata");
        mode(0);report=CameraService{}.discover({incomplete,file});check(report.errors.size()==1 && report.devices.size()==1 && live()==0,"producer failure isolation and missing export");
        const auto partial=CameraService::industrialSnapshot(report);
        check(partial.producerResults.size()==2 && !partial.producerResults[0].complete && partial.producerResults[1].complete,"per-producer completion and error origin");
        MeasurementCamera registered{{"fixture"},"Fixture",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},partial.devices[0].identity};
        check(CameraService::resolveIndustrial(registered,partial)==partial.devices[0],"broken A does not block healthy B");
        auto broken=partial;broken.producerResults[1].complete=false;
        rejects([&]{CameraService::resolveIndustrial(registered,broken);},"selected B partial rejected");
        broken=partial;broken.errors.append("unattributed inventory error");
        rejects([&]{CameraService::resolveIndustrial(registered,broken);},"unknown error origin cannot grant route use");
        broken=partial;broken.cancelled=true;
        rejects([&]{CameraService::resolveIndustrial(registered,broken);},"cancelled inventory rejected despite completed B");
        CameraLocalConfiguration local(directory.filePath("local.ini"));
        local.setProducers({file});qputenv("GENICAM_GENTL64_PATH",directory.path().toUtf8());
        check(local.automaticProducers().size()==2,"remembered additions merge process environment");
        local.setProducers({file,file});check(local.automaticProducers().size()==2,"canonical repeated producer deduplicates");
        local.setProducerEnabled(copy,false);check(local.automaticProducers().size()==1,"environment producer disabled locally");
        local.setProducerEnabled(copy,true);local.removeProducer(file);check(local.automaticProducers().size()==1,"remove A keeps B");
        local.setProducers({file});local.setProducerEnabled(file,true);
        const auto config=qgetenv("VISIONFLOW_CAMERA_CONFIGURATION");qputenv("VISIONFLOW_CAMERA_CONFIGURATION",directory.filePath("local.ini").toUtf8());
        mode(0);report=CameraService{}.discover();check(report.producers.size()==2,"product automatic scan uses local and environment union");
        CameraService{}.discover({incomplete});check(local.producers().size()==1,"explicit diagnostic never mutates automatic list");
        local.setProducerEnabled(file,false);local.setProducerEnabled(copy,false);
        check(CameraService{}.discover().producers.isEmpty(),"all producers disabled cannot resurrect environment");
        qputenv("VISIONFLOW_CAMERA_CONFIGURATION",config);qputenv("GENICAM_GENTL64_PATH","");
        const auto runtime=directory.filePath("runtime.dll");check(QFile::copy(incomplete,runtime),"runtime PE fixture");
        local.setRuntime("oecs",runtime,{directory.path()});
        check(CameraLocalConfiguration(directory.filePath("local.ini")).runtime("oecs").path==QFileInfo(runtime).canonicalFilePath(),"runtime restart persistence");
        check(!local.runtime("oecs",directory.filePath("missing.dll")).error.isEmpty(),"broken explicit choice cannot fallback");
        check(QFile::remove(runtime),"remove saved runtime");check(!local.runtime("oecs").error.isEmpty() && local.runtime("oecs").path==runtime,"broken remembered selection cannot silently change SDK");
#ifdef Q_OS_WIN
        QByteArray x86(90,'\0');x86.replace(0,2,"MZ");x86[60]=64;x86.replace(64,4,QByteArray("PE\0\0",4));x86[68]=char(0x4c);x86[69]=1;x86[88]=char(0x0b);x86[89]=1;
        {QFile f(runtime);check(f.open(QIODevice::WriteOnly),"x86 fixture");f.write(x86);}
        check(runtimeBinaryError(runtime).contains("x64"),"wrong x86 architecture diagnosed before load");
        rejects([&]{local.setRuntime("basler",runtime);},"x86 cannot be saved as backend runtime");QFile::remove(runtime);
#endif
        check(QFile::copy(incomplete,runtime),"valid runtime PE copy");
        rejects([&]{local.setRuntime("oecs",runtime,{directory.filePath("absent")});},"missing dependency directory rejected");
        const auto vendor=qgetenv("PYLON_DEV_DIR");
        QDir().mkpath(directory.filePath("pylon/Development"));QDir().mkpath(directory.filePath("pylon/Runtime/x64"));
        check(QFile::copy(incomplete,directory.filePath("pylon/Runtime/x64/PylonC_v1.dll")),"vendor runtime fixture");
        qputenv("PYLON_DEV_DIR",directory.filePath("pylon/Development").toUtf8());
        check(!local.runtime("basler").path.isEmpty() && local.runtime("basler").error.isEmpty(),"vendor environment resolves same installation runtime");
        check(QFile::copy(incomplete,directory.filePath("pylon/Runtime/x64/PylonC_v2.dll")),"second runtime fixture");
        check(!local.runtime("basler").error.isEmpty() && local.runtime("basler").path.isEmpty(),"multiple runtime versions demand selection");
        local.setRuntime("basler",directory.filePath("pylon/Runtime/x64/PylonC_v1.dll"));
        check(local.runtime("basler").error.isEmpty(),"saved explicit runtime resolves multiple installations");
        qputenv("PYLON_DEV_DIR",vendor);
#ifdef Q_OS_WIN
        check(app.arguments().size()==4,"dependency fixture required");
        const auto dependent=directory.filePath("dependent.dll");check(QFile::copy(app.arguments()[3],dependent),"dependent runtime copy");
        RuntimeLibrary dependencyLoader;QString dependencyError;
        check(!dependencyLoader.load(dependent,{},dependencyError) && dependencyError.contains("bağımlılık"),"missing dependency diagnosed by actual Windows loader");
        local.setRuntime("basler",dependent,{QFileInfo(app.arguments()[3]).absolutePath()});
        qputenv("VISIONFLOW_CAMERA_CONFIGURATION",directory.filePath("local.ini").toUtf8());
        check(dependencyLoader.load(dependent,{},dependencyError),"saved backend process dependency directory resolves load");
        qputenv("VISIONFLOW_CAMERA_CONFIGURATION",config);
        QLibrary dependentLibrary(dependent);check(dependentLibrary.load(),"loaded dependent runtime");
        const auto dependentFunction=reinterpret_cast<int(*)()>(dependentLibrary.resolve("CameraRuntimeFixture"));
        check(dependentFunction && dependentFunction()==42,"dependency function executes from configured installation");
        dependentLibrary.unload();dependencyLoader.unload();
        RuntimeLibrary siblingLoader;
        check(siblingLoader.load(app.arguments()[3],{},dependencyError),"forward-slash Qt path resolves sibling DLLs without PATH changes");
        const auto shared=directory.filePath("shared");check(QDir().mkpath(shared),"shared system dependency fixture");
        const auto sharedRuntime=shared+"/runtime.dll";check(QFile::copy(incomplete,sharedRuntime),"shared runtime fixture");
        wchar_t systemDirectory[32768]{};const auto systemSize=GetSystemDirectoryW(systemDirectory,32768);
        check(systemSize>0 && systemSize<32768,"Windows system directory");
        const auto systemDll=QString::fromWCharArray(systemDirectory,int(systemSize))+"/kernel32.dll";
        check(QFile::copy(systemDll,shared+"/kernel32.dll"),"system DLL sibling copy");
        RuntimeLibrary sharedLoader;check(sharedLoader.load(sharedRuntime,{},dependencyError),"already loaded system dependency is not a foreign SDK conflict");sharedLoader.unload();
        check(!sharedLoader.load(shared+"/kernel32.dll",{},dependencyError),"explicit main runtime still requires its exact installation");
        check(QFile::copy(QFileInfo(app.arguments()[3]).absolutePath()+"/camera_runtime_dependency.dll",shared+"/camera_runtime_dependency.dll"),"foreign vendor dependency copy");
        check(!sharedLoader.load(sharedRuntime,{},dependencyError) && dependencyError.contains("camera_runtime_dependency.dll"),"foreign vendor dependency still rejected");
#endif

        report=CameraService{}.discover({directory.filePath("missing.cti")});check(report.devices.isEmpty() && report.errors.size()==1,"missing producer");
        const auto invalid=directory.filePath("invalid.cti");{QFile f(invalid);check(f.open(QIODevice::WriteOnly),"invalid producer");f.write("not a DLL");}
        report=CameraService{}.discover({invalid});check(report.errors.size()==1,"invalid binary");
        auto cancel=std::make_shared<std::atomic_bool>(true);report=CameraService{}.discover({file},1000,cancel);check(report.cancelled && report.devices.isEmpty() && live()==0,"pre-cancel discovery");
        mode(8);cancel->store(false);std::thread stopper([&]{std::this_thread::sleep_for(std::chrono::milliseconds(2));cancel->store(true);});report=CameraService{}.discover({file},1000,cancel);stopper.join();check(report.cancelled && report.devices.isEmpty() && live()==0,"cancel during producer call suppresses results");mode(0);
        report=CameraService{}.discover({file},0);check(report.errors.size()==1 && live()==0,"invalid timeout");
        report=CameraService{}.discover({file},1000,{},0);check(report.errors.size()==1 && live()==0,"invalid scan wait before producer init");
        {GenTLTransport transport(file);auto bad=device;bad.deviceId="missing";rejects([&]{transport.openReadOnly(bad);},"device disappeared");check(!transport.isDeviceOpen(),"failed open state");transport.openReadOnly(device);}
        check(live()==0,"recovery after disappeared device");qputenv("GENICAM_GENTL64_PATH",original);
        QTextStream(stdout)<<"PASS: synthetic GenTL C ABI discovery, open/close, cleanup, errors, deadline and recovery\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}
}
