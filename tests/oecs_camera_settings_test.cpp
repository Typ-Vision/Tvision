#include "OecsExperimentalFixture.hpp"
#include "application/CameraSettingsService.hpp"
using namespace oecs_experimental_fixture;
namespace {
void complete(CameraSettingsSession& s){until([&]{s.poll();return s.state()==CameraState::Connected || s.state()==CameraState::Configured || s.state()==CameraState::Fault;});if(s.state()==CameraState::Fault)throw std::runtime_error(s.error().toStdString());}
void runService(DiscoveredDevice device,QString runtime,OecsCameraSettings::ApiFactory api,const QString& output={},bool update=false){
    QTemporaryDir dir(QDir::currentPath()+"/oecs-settings-XXXXXX");check(dir.isValid(),"temporary project");
    const auto path=dir.filePath("settings.vfproject");QFile project(path);check(project.open(QIODevice::WriteOnly),"saved project");project.write("{}");project.close();
    MeasurementEnvironmentRepository repo(path,"settings");MeasurementEnvironmentRegistry registry(repo);
    auto camera=request().camera;camera.physical=device.identity;const auto id=registry.registerCamera(camera,"OPTO",registry.snapshot().revision).camera;
    CameraLocalConfiguration local(dir.filePath("local.ini"));
    auto session=CameraSettingsService::open(repo,id,repo.load().revision,{{device},{},false},local,{},[]{return true;},[]{},MeasurementEnvironmentRepository::Fault::None,runtime,api);
    session->connect(15000);complete(*session);const auto before=session->snapshot();
    session->apply({before.actual.settings.exposureTimeUs,before.actual.settings.gain},15000);complete(*session);
    QJsonObject scalarUpdate;
    if(update){
        const auto reopen=[&]{
            session->stop();until([&]{return session->shutdownState()==ShutdownState::Ready;});session.reset();
            session=CameraSettingsService::open(repo,id,repo.load().revision,{{device},{},false},local,{},[]{return true;},[]{},MeasurementEnvironmentRepository::Fault::None,runtime,api);
            session->connect(15000);complete(*session);
        };
        reopen();
        const auto original=before.actual.settings;
        const double exposure=original.exposureTimeUs+1000<=before.exposure.maximum ? original.exposureTimeUs+1000 : original.exposureTimeUs-1000;
        const double gain=original.gain!=before.gain.maximum ? before.gain.maximum : before.gain.minimum;
        check(exposure>=before.exposure.minimum && gain>=before.gain.minimum,"bounded physical scalar update");
        session->apply({exposure,gain},15000);complete(*session);const auto changed=session->snapshot();
        // Restore before evaluating the observation, including on a failed comparison.
        reopen();
        session->apply({original.exposureTimeUs,original.gain},15000);complete(*session);
        const auto restored=session->snapshot();
        scalarUpdate={{"before",before.actual.toJson()},{"changed",changed.actual.toJson()},{"restored",restored.actual.toJson()}};
        check(changed.saved && changed.profileChanged && !changed.geometryChanged && changed.deviceChanged,"scalar update preserves physical geometry");
        check(changed.actual.settings.exposureTimeUs!=original.exposureTimeUs && changed.actual.settings.gain!=original.gain,"both physical scalar readbacks changed");
        check(restored.saved && restored.actual.settings.exposureTimeUs==original.exposureTimeUs && restored.actual.settings.gain==original.gain,"original physical scalars restored");
    }
    check(session->snapshot().saved && repo.load().current.setups.first().pending && !repo.load().current.profiles.first().pending,"real settings saved without fabricated optics");
    check(!CameraProviderFactory{}.capabilities(device).preview,"settings do not grant native acquisition");
    if(!output.isEmpty()){
        QJsonObject report{{"identity",device.identity.toJson()},{"readback",session->snapshot().actual.toJson()},{"profileSaved",true},{"opticsPending",true},{"nativeAdmission",false},
            {"exposureMin",before.exposure.minimum},{"exposureMax",before.exposure.maximum},{"exposureUnit",before.exposure.unit},{"gainMin",before.gain.minimum},{"gainMax",before.gain.maximum},{"gainUnit",before.gain.unit},{"scalarUpdate",scalarUpdate}};
        QFile file(output);check(file.open(QIODevice::WriteOnly|QIODevice::NewOnly),"new report path");file.write(QJsonDocument(report).toJson());
    }
    session->stop();until([&]{return session->shutdownState()==ShutdownState::Ready;});
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    if(app.arguments().size()==4 && (app.arguments()[1]=="--physical" || app.arguments()[1]=="--physical-update")){
        // Actual discovery, then exact known device identity; no index fallback.
        const auto inventory=CameraService{}.inventory();check(inventory.errors.isEmpty() && !inventory.cancelled,"complete physical discovery required");
        const auto groups=CameraService::groupIndustrial(inventory.industrial);bool found=false;
        for(const auto& group:groups)if(group.identity.vendor=="Opto Engineering Srl" && group.identity.model=="COE-050-M-USB-050-IR-C" && group.identity.serial=="02A2220000627"){
            check(!found && !group.ambiguous && group.routes.size()==1,"exact unique physical route");found=true;
            runService(group.routes.first(),app.arguments()[2],makeOecsCApi,app.arguments()[3],app.arguments()[1]=="--physical-update");
        }
        check(found,"physical Opto camera not discovered");QTextStream(stdout)<<"PASS physical settings read/apply/temporary project save; scalar update only with --physical-update, restored on successful run; no acquisition or mm acceptance\n";return 0;
    }
    const auto runtime=QDir::current().absoluteFilePath("settings-fixture.dll");QFile marker(runtime);check(marker.open(QIODevice::WriteOnly),"fixture marker");marker.write("fixture");marker.close();
    for(int scenario=0;scenario<6;++scenario){auto h=state();const auto d=request().device;
        if(scenario==1){h->dynamicAuto=true;h->nodes["TriggerMode"].value=QString("On");h->nodes["TriggerSource"].value=QString("Software");}
        if(scenario==2)for(const auto n:{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"}){h->nodes[n].access=FeatureAccess::Missing;h->nodes[n].type=FeatureType::Unsupported;}
        if(scenario==3)for(const auto n:{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"}){h->nodes[n].type=FeatureType::Enumeration;h->nodes[n].value=QString(n)+"1";h->nodes[n].entries={{QString(n)+"1",1}};}
        if(scenario==4)h->wrongIdentity=true;
        if(scenario==5){h->dynamicAuto=true;h->nodes["TriggerMode"].value=QString("On");h->failure="configure:TriggerMode";}
        auto backend=OecsCameraSettings::create(d,runtime,[h]{return std::make_unique<Api>(h);});
        CameraTicket ticket;ticket.operation=CameraOperation::Connect;ticket.cancel=std::make_shared<std::atomic_bool>(false);ticket.deadline=Deadline(5000);
        if(scenario==4){rejects([&]{backend->execute(ticket);});continue;}
        backend->execute(ticket);check(backend->actualIdentity()==d.identity,"exact settings identity");
        if(scenario==5){rejects([&]{backend->setting("ExposureAuto");});check(h->nodes["TriggerMode"].value==FeatureValue(QString("On")),"failed temporary trigger write restores mode");backend->shutdown();continue;}
        const auto before=backend->readback();backend->writeSetting("ExposureTime",6000.);check(backend->readback().settings.exposureTimeUs==6000.,"OECS actual exposure write/readback");
        backend->writeSetting("ExposureTime",before.settings.exposureTimeUs);check(backend->readback()==before,"readback restored");
        ticket.operation=CameraOperation::Grab;rejects([&]{backend->execute(ticket);});check(h->calls.isEmpty(),"settings never start or trigger stream");backend->shutdown();
        if(oecsSdkBindingCompiled())runService(d,runtime,[h]{return std::make_unique<Api>(h);});
    }
    QFile::remove(runtime);QTextStream(stdout)<<"PASS OECS settings: product service, scalar write/readback, pending defaults, dynamic auto/trigger restore, geometry variants, wrong identity, no acquisition\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
