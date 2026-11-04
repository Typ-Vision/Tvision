#include "IndustrialProviderFixture.hpp"
#include "application/BaslerCameraSettings.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include "hardware/PylonSession.hpp"
#include <QLibrary>
#include <QTemporaryDir>
#include <iostream>
using namespace industrial_provider_fixture;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    if(argc==1){check(!PylonSession::available(),"SDK-less stream unavailable");PylonSession s;reject([&]{s.start(true,[]{return 100;});});return 0;}
    const QString dll=app.arguments()[1];QLibrary lib(dll);check(lib.load(),"load fixture");
    const auto mode=reinterpret_cast<void(*)(int)>(lib.resolve("fixtureMode"));
    const auto resources=reinterpret_cast<int(*)()>(lib.resolve("fixtureResources"));
    const auto triggers=reinterpret_cast<int(*)()>(lib.resolve("fixtureTriggers"));check(mode && resources && triggers,"fixture controls");
    const auto open=[&](PylonSession& s){s.open(dll,{},{"Basler","acA1920-40um","23102780"});};
    for(bool software:{false,true}){
        mode(40);PylonSession s;open(s);s.start(software,[]{return 500;});
        const auto a=s.grab([]{return 500;});const auto b=s.grab([]{return 500;});
        check(a.blockId==0 && b.blockId==1 && a.pixels==b.pixels,"USB zero ID/equal pixels are distinct frames");
        check(triggers()==(software?2:0),"preview never issues software command");
        s.stop();check(a.pixels[0]==char(0x2a),"copy survives SDK buffer return");
        check(featureEquivalent(s.feature("TriggerMode"),QString("Off")) && featureEquivalent(s.feature("TriggerSource"),QString("Line1")),"restore acquisition settings");
        s.start(software,[]{return 500;});s.grab([]{return 500;});s.close();check(resources()==0,"Stop Start close drained");
    }
    for(int v:{41,42,43,44,45,46,47,48,49,50,52,53,54,55,56,57,58}){
        mode(v);PylonSession s;open(s);
        if(v==55)reject([&]{s.start(true,[]{return 100;});});
        else {s.start(true,[]{return 100;});
            if(v==41 || v==42)s.grab([]{return 100;});
            Deadline d(30);reject([&]{s.grab([&]{return d.remainingMs();});});
        }
        reject([&]{s.grab([]{return 100;});});s.close();check(resources()==0,"fault drains all resources");
    }
    for(int phase=0;phase<8;++phase){mode(40);PylonSession s;open(s);int checks=0;
        const auto budget=[&]{return ++checks>(phase?phase-1:0)?0:100;};
        if(phase==0)reject([&]{s.start(true,budget);});
        else {s.start(true,[]{return 100;});reject([&]{s.grab(budget);});}
        s.close();check(resources()==0,"cancel phase drain");
    }
    for(int v:{51,59}){mode(v);PylonSession s;open(s);s.start(true,[]{return 100;});s.grab([]{return 100;});
        reject([&]{s.close();});check(resources()>0,"cleanup fault retains resources");
        mode(40);s.close();check(resources()==0,"owner retries cleanup");}
    for(int v:{60,61}){mode(v);PylonSession s;open(s);check(s.feature("DecimationHorizontal").access==(v==60?FeatureAccess::Missing:FeatureAccess::Unavailable),"NI is not implemented; NA stays unknown");s.close();}
    mode(40);auto r=request();r.device.identity={"Basler","acA1920-40um","23102780",DeviceIdentityConfidence::SerialReported};r.camera.physical=r.device.identity;
    r.device.route={"gentl",QDir::currentPath()+"/basler.cti","usb","usb"};r.device.backend={"gentl","test-only",QString(64,'b'),1};r.setup.geometry.resolution={1920,1200};r.setup.geometry.sensorRoi=QRect(8,8,1920,1200);
    r.profile.requested.exposureTimeUs=3000;r.profile.requested.gain=0;
    for(int v:{63,64}){mode(v);auto changed=r;changed.profile.requested.gain=1;auto backend=BaslerAcquisition::create(changed,dll);const auto cancel=std::make_shared<std::atomic_bool>(false);
        backend->execute({1,1,CameraOperation::Connect,Deadline(500),cancel});reject([&]{backend->execute({2,1,CameraOperation::Configure,Deadline(500),cancel});});
        if(v==63)check(featureEquivalent(backend->setting("Gain"),0.) && featureEquivalent(backend->setting("TriggerMode"),QString("Off")),"partial configure write restored");
        reject([&]{backend->execute({3,1,CameraOperation::Start,Deadline(500),cancel});});backend->shutdown();check(resources()==0,"configuration fault drained");}
    mode(40);
    auto b=BaslerAcquisition::create(r,dll);auto cancel=std::make_shared<std::atomic_bool>(false);
    const auto execute=[&](CameraOperation op){return b->execute({1,1,op,Deadline(500),cancel});};
    execute(CameraOperation::Connect);execute(CameraOperation::Configure);execute(CameraOperation::Start);
    const auto result=execute(CameraOperation::Grab);check(result.frame && !result.frame->image.metadata().experimentalAcquisition.isEmpty(),"untrusted provenance mandatory");b->shutdown();check(resources()==0,"backend drained");
    CameraProviderFactory product;check(!product.capabilities(r.device).softwareTrigger,"product G1 remains closed");reject([&]{product.open(r,CameraLeaseUse::Runtime,FrameQueueMode::ProductionFifo);});
    QTemporaryDir dir(QDir::currentPath()+"/basler-worker-XXXXXX");check(dir.isValid(),"worker fixture directory");
    const auto oldConfig=qgetenv("VISIONFLOW_CAMERA_CONFIGURATION");const auto local=dir.filePath("camera.ini");CameraLocalConfiguration(local).setRuntime("basler",dll);qputenv("VISIONFLOW_CAMERA_CONFIGURATION",local.toUtf8());
    const auto caps=product.capabilities(r.device);
    check(caps.open && caps.configure && caps.preview && !caps.softwareTrigger && caps.unavailableReason.contains("G1"),"configured SDK exposes preview without production authority");
    const auto beforeRejected=triggers();
    for(const auto use:{CameraLeaseUse::Calibration,CameraLeaseUse::Editor,CameraLeaseUse::Runtime,CameraLeaseUse::Preview}){
        try{product.open(r,use,FrameQueueMode::ProductionFifo);throw std::runtime_error("production unexpectedly admitted");}
        catch(const std::exception& e){check(QString::fromUtf8(e.what()).contains("Basler production/G1 admission unavailable"),"configured product rejects G1 before opening hardware");}
    }
    check(resources()==0 && triggers()==beforeRejected,"rejected production opens no handles and triggers no capture");
    r.profile.requested.trigger={};mode(40);
    reject([&]{product.open(r,CameraLeaseUse::Runtime,FrameQueueMode::PreviewLatest);});
    {auto worker=product.open(r,CameraLeaseUse::Preview,FrameQueueMode::PreviewLatest,1);start(*worker);command(*worker,CameraOperation::Grab);const auto frame=worker->take();check(frame && frame->sourceId==r.sourceId && triggers()==0,"product native preview on common worker");
        command(*worker,CameraOperation::Grab);worker->requestStop();command(*worker,CameraOperation::Stop);check(!worker->take(),"Stop retires preview generation");worker->requestShutdown();until([&]{return worker->shutdownState()==ShutdownState::Ready;});check(worker->cleanShutdown()==true,"native owner cleanup");}
    qputenv("VISIONFLOW_CAMERA_CONFIGURATION",oldConfig);check(resources()==0,"worker resources released");
    std::cout<<"PASS: pylon owned stream, preview/trigger, stale/reset/wrap, malformed payload, cancellation, cleanup and admission\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
