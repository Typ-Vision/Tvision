#include "IndustrialProviderFixture.hpp"
#include "application/CameraSettingsService.hpp"
#include "application/BaslerCameraSettings.hpp"
#include "hardware/PylonSession.hpp"
#include <QTemporaryDir>
#include <QFile>
#include <QLibrary>
#include <QJsonDocument>
#include <cmath>
#include <iostream>
using namespace industrial_provider_fixture;
namespace {
void complete(CameraSettingsSession& s){until([&]{s.poll();return s.state()==CameraState::Connected || s.state()==CameraState::Configured || s.state()==CameraState::Fault;});}
struct Fixture {
    QTemporaryDir dir{QDir::currentPath()+"/basler-settings-XXXXXX"};
    MeasurementEnvironmentRepository repo{dir.filePath("project.json"),"basler-settings"};
    CameraLocalConfiguration local{dir.filePath("local.ini")};
    DiscoveredDevice device=request().device;
    CameraId id;
    QString runtime;
    int invalidations=0;
    Fixture(QString path, std::optional<DiscoveredDevice> actual={}):runtime(std::move(path)){
        check(dir.isValid(),"temp fixture");QFile file(repo.projectPath());check(file.open(QIODevice::WriteOnly),"saved project");file.write("{}");file.close();
        if(actual)device=*actual;
        else {device.identity={"Basler","acA1920-40um","23102780",DeviceIdentityConfidence::SerialReported};device.route={"gentl",dir.filePath("basler.cti"),"usb","usb"};device.backend={"gentl","test-only",QString(64,'b'),1};}
        auto c=request().camera;c.physical=device.identity;MeasurementEnvironmentRegistry registry(repo);id=registry.registerCamera(c,"BASLER",registry.snapshot().revision).camera;
        local.setRuntime("basler",runtime);
    }
    auto open(MeasurementEnvironmentRepository::Fault fault=MeasurementEnvironmentRepository::Fault::None){return CameraSettingsService::open(repo,id,repo.load().revision,{{device},{},false},local,{},[]{return true;},[this]{++invalidations;},fault);}
};
void connected(CameraSettingsSession& s){s.connect(15000);complete(s);check(s.state()==CameraState::Connected,qPrintable(s.error()));}
void apply(CameraSettingsSession& s,CameraSettingsValues v){s.apply(v,15000);complete(s);}
void saved(CameraSettingsSession& s){check(s.state()==CameraState::Configured && s.snapshot().saved,qPrintable(s.error()));}
void close(std::unique_ptr<CameraSettingsSession>& s){s->stop();until([&]{return s->shutdownState()==ShutdownState::Ready;});s.reset();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    if(app.arguments().size()==4 && app.arguments()[1]=="--physical-update"){
        const auto inventory=CameraService{}.inventory();std::optional<DiscoveredDevice> target;
        for(const auto& group:CameraService::groupIndustrial(inventory.industrial))if(group.identity.vendor=="Basler" && group.identity.model=="acA1920-40um" && group.identity.serial=="23102780"){
            check(!group.ambiguous && group.routes.size()==1 && !target,"unique exact Basler discovery");target=group.routes.first();
            check(CameraService::routeComplete(inventory.industrial,target->route),"complete selected producer");}
        check(target.has_value(),"exact physical Basler not discovered");Fixture f(app.arguments()[2],target);
        auto s=f.open();connected(*s);const auto before=s->snapshot();
        check(before.actual.settings.exposureAuto==AutoMode::Off && before.actual.settings.gainAuto==AutoMode::Off,"physical update requires manual original state");
        QJsonObject report{{"before",before.actual.toJson()},{"exposureUnit",before.exposure.unit},{"gainUnit",before.gain.unit},{"exposureMin",before.exposure.minimum},{"exposureMax",before.exposure.maximum},{"gainMin",before.gain.minimum},{"gainMax",before.gain.maximum}};
        const auto writeReport=[&]{QFile out(app.arguments()[3]);check(out.open(QIODevice::WriteOnly),"physical report");out.write(QJsonDocument(report).toJson());};writeReport();
        apply(*s,{before.actual.settings.exposureTimeUs,before.actual.settings.gain});saved(*s);check(!s->snapshot().deviceChanged,"physical no-op writes none");close(s);
        s=f.open();connected(*s);const auto unchanged=f.repo.load().encode();apply(*s,{});saved(*s);check(!s->snapshot().deviceChanged && !s->snapshot().profileChanged && unchanged==f.repo.load().encode(),"saved profile no-op");close(s);
        const double exposure=before.actual.settings.exposureTimeUs+(before.actual.settings.exposureTimeUs+100<=before.exposure.maximum?100:-100);
        const double gain=before.actual.settings.gain+(before.actual.settings.gain+1<=before.gain.maximum?1:-1);
        check(exposure>=before.exposure.minimum && gain>=before.gain.minimum,"bounded physical values");
        bool changedOK=false;QString error;
        try{s=f.open();connected(*s);apply(*s,{exposure,gain});saved(*s);report["changed"]=s->snapshot().actual.toJson();changedOK=true;}catch(const std::exception& e){error=e.what();}
        if(s)close(s);
        // Always restore before evaluating the change, even if SDK/readback failed.
        s=f.open();connected(*s);apply(*s,{before.actual.settings.exposureTimeUs,before.actual.settings.gain});saved(*s);
        report["restored"]=s->snapshot().actual.toJson();report["restoreVerified"]=s->snapshot().actual==before.actual;writeReport();
        check(s->snapshot().actual==before.actual,"physical restore exact readback");close(s);check(changedOK,qPrintable(error));
        check(!CameraProviderFactory{}.capabilities(*target).preview,"no acquisition admission");
        std::cout<<"PASS physical read/no-op/scalar update/restore; no grab\n";return 0;
    }
    if(argc==1){check(!PylonSession::available(),"SDK absent");std::cout<<"PASS Basler settings unavailable\n";return 0;}
    const QString runtime=QString::fromLocal8Bit(argv[1]);QLibrary sdk(runtime);check(sdk.load(),"fixture DLL");
    auto mode=reinterpret_cast<void(*)(int)>(sdk.resolve("fixtureMode"));auto writes=reinterpret_cast<int(*)()>(sdk.resolve("fixtureWrites"));auto resources=reinterpret_cast<int(*)()>(sdk.resolve("fixtureResources"));check(mode && writes && resources,"fixture exports");
    for(int scenario:{20,21,22,23,24,25,26,27,28,29,30,31}){
        mode(scenario);Fixture f(runtime);const auto original=f.repo.load().encode();auto s=f.open();s->connect();complete(*s);
        if(scenario==21 || scenario==23 || scenario==29){check(s->state()==CameraState::Fault && writes()==0 && original==f.repo.load().encode(),"missing/unproven unit fails without write");close(s);continue;}
        check(s->state()==CameraState::Connected,qPrintable(s->error()));const auto before=s->snapshot();
        check(before.actual.geometry.resolution==QSize(1920,1200) && before.sensorSize==QSize(1936,1216) && before.actual.geometry.offset==QPoint(8,8),"real geometry/sensor separate");
        check(before.actual.geometry.decimationEvidence==GeometryEvidence::NotApplicable,"missing pair evidence");
        if(scenario==27)check(before.actual.geometry.binningEvidence==GeometryEvidence::NotApplicable,"binning missing pair");
        if(scenario==28)check(before.actual.geometry.binningEvidence==GeometryEvidence::Unknown,"partial binning remains unknown");
        check(writes()==0 && original==f.repo.load().encode(),"read only does not save/write");
        apply(*s,{3500,1});
        if(scenario==22 || scenario==24 || scenario==25 || scenario==26 || scenario==31){check(s->state()==CameraState::Fault && original==f.repo.load().encode(),"negative leaves project unchanged");
            check(s->snapshot().unusable==(scenario==26),"rollback status");close(s);
            if(scenario!=26){PylonSession inspect;inspect.open(runtime,{}, {"Basler","acA1920-40um","23102780"});check(std::get<double>(inspect.feature("ExposureTime").value)==3000 && std::get<double>(inspect.feature("Gain").value)==0,"SDK rollback restored scalars");
                check(std::get<QString>(inspect.feature("ExposureAuto").value)==(scenario==31?"Continuous":"Off"),"rollback restores original auto");inspect.close();}
        }else{saved(*s);check(f.repo.load().current.setups.first().pending && !f.repo.load().current.profiles.first().pending,"pending optics preserved");close(s);
            s=f.open();connected(*s);const auto state=f.repo.load().encode();const auto count=writes();const auto invalidations=f.invalidations;apply(*s,{});saved(*s);
            check(writes()==count && state==f.repo.load().encode() && f.invalidations==invalidations,"true no-op");close(s);}
        check(resources()==0,"no SDK leaks");
    }
    for(int scenario=0;scenario<5;++scenario){mode(20);Fixture f(runtime);auto s=f.open(scenario==0?MeasurementEnvironmentRepository::Fault::BeforeCommit:MeasurementEnvironmentRepository::Fault::None);connected(*s);
        if(scenario==0){apply(*s,{3500,1});check(s->state()==CameraState::Fault && !s->snapshot().unusable,"disk failure rollback");}
        if(scenario==1 || scenario==4){apply(*s,{scenario==1?std::nan(""):100001.,1});check(s->state()==CameraState::Fault && writes()==0,"NaN/range before mutation");}
        if(scenario==2){auto e=f.repo.load(),next=e;next.current.cameras.first().name="Concurrent";f.repo.commit(e,next);reject([&]{s->apply({3500,1});});check(writes()==0,"stale before write");}
        if(scenario==3){close(s);auto wrong=f.device;wrong.identity.serial="wrong";reject([&]{auto b=BaslerCameraSettings::create(wrong,runtime);CameraTicket t;t.operation=CameraOperation::Connect;t.cancel=std::make_shared<std::atomic_bool>(false);t.deadline=Deadline(5000);b->execute(t);});}
        if(s)close(s);check(resources()==0,"negative cleanup");
    }
    std::cout<<"PASS Basler settings product composition, units, geometry, pending, no-op, auto, limits, SDK/disk/rollback failures, stale, identity\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
