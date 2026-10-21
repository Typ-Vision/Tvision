#include "IndustrialProviderFixture.hpp"
#include "application/CameraSettingsService.hpp"
#include <QTemporaryDir>
#include <QFile>
#include <cmath>
#include <future>
using namespace industrial_provider_fixture;
namespace {
struct SettingsHardware {
    AcquisitionReadback actual;
    QMap<QString,CameraFeature> nodes;
    QString failure,clamp;
    bool rollbackFailure=false,failed=false,disconnect=false;
    bool rounded=false,dynamicAuto=false;
    std::function<void()> afterWrite;
    int writes=0;
    std::thread::id worker;
};
class SettingsBackend:public IndustrialCameraBackend {
    IndustrialCameraRequest r_;
    std::shared_ptr<SettingsHardware> h_;
    void worker() const {check(std::this_thread::get_id()==h_->worker,"settings stay on one worker");}
public:
    SettingsBackend(IndustrialCameraRequest r,std::shared_ptr<SettingsHardware> h):r_(std::move(r)),h_(std::move(h)){h_->worker=std::this_thread::get_id();}
    PhysicalDeviceIdentity actualIdentity() const override {worker();return r_.device.identity;}
    AcquisitionReadback readback() const override {worker();if(h_->disconnect)throw std::runtime_error("disconnected");return h_->actual;}
    CameraFeature setting(const QString& n) const override {worker();if(h_->disconnect)throw std::runtime_error("disconnected");auto f=h_->nodes.value(n);
        if(h_->dynamicAuto && n=="ExposureTime" && h_->actual.settings.exposureAuto!=AutoMode::Off)f.access=FeatureAccess::ReadOnly;return f;}
    void writeSetting(const QString& n,const FeatureValue& value) override {
        worker();++h_->writes;if(h_->rollbackFailure && h_->failed)throw std::runtime_error("restore failed");
        h_->nodes[n].value=value;
        if(n=="ExposureTime"){const double v=h_->rounded?double(float(std::get<double>(value))):std::get<double>(value);h_->nodes[n].value=v;h_->actual.settings.exposureTimeUs=v;}
        if(n=="Gain")h_->actual.settings.gain=std::get<double>(value);
        if(n=="ExposureAuto")h_->actual.settings.exposureAuto=std::get<QString>(value)=="Off"?AutoMode::Off:AutoMode::Continuous;
        if(n=="GainAuto")h_->actual.settings.gainAuto=std::get<QString>(value)=="Off"?AutoMode::Off:AutoMode::Continuous;
        if(n==h_->clamp && !h_->failed){h_->failed=true;h_->nodes[n].value=std::get<double>(value)-1.;h_->actual.settings.exposureTimeUs=std::get<double>(value)-1.;}
        if(n==h_->failure && !h_->failed){h_->failed=true;throw std::runtime_error("SDK write failure after mutation");}
        if(h_->afterWrite){auto callback=std::move(h_->afterWrite);h_->afterWrite={};callback();}
    }
    CameraWorkerResult execute(const CameraTicket&) override {worker();return {true,{},{}};}
    void shutdown() override {worker();}
};
class SettingsProvider:public IndustrialCameraProvider {
    std::shared_ptr<SettingsHardware> h_;
public:
    explicit SettingsProvider(std::shared_ptr<SettingsHardware> h):h_(std::move(h)){}
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice&) const override {return {true,true,false,false,true,true,{}};}
    std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest& r) const override {return std::make_unique<SettingsBackend>(r,h_);}
    std::unique_ptr<IndustrialCameraBackend> createSettings(const MeasurementCamera& camera,const DiscoveredDevice& device) const override {
        IndustrialCameraRequest r;r.camera=camera;r.device=device;return std::make_unique<SettingsBackend>(r,h_);
    }
};
struct Fixture {
    QTemporaryDir dir;
    IndustrialCameraRequest r=request();
    MeasurementEnvironmentRepository repo{dir.filePath("project.json"),"project"};
    CameraLocalConfiguration local{dir.filePath("local.ini")};
    std::shared_ptr<SettingsHardware> h=std::make_shared<SettingsHardware>();
    CameraProviderFactory factory=CameraProviderFactory::forTests(std::make_shared<SettingsProvider>(h));
    bool busy=false;int invalidations=0;
    Fixture(){
        QFile project(repo.projectPath());if(!project.open(QIODevice::WriteOnly))throw std::runtime_error(("saved fixture project: "+project.fileName()+": "+project.errorString()).toStdString());project.write("{}");project.close();
        MeasurementEnvironmentRegistry registry(repo);MeasurementSetupContract c{{r.camera},{r.setup},{r.profile}};registry.replace(c);
        h->actual.settings=r.profile.requested;auto& g=h->actual.geometry;g.resolution=r.setup.geometry.resolution;
        g.offsetEvidence=g.binningEvidence=g.decimationEvidence=g.reverseEvidence=GeometryEvidence::Reported;
        for(const auto& n:QStringList{"ExposureTime","Gain"}){CameraFeature f;f.name=n;f.type=FeatureType::Float;f.access=FeatureAccess::ReadWrite;
            f.value=n=="Gain"?1.:100.;f.minimum=n=="Gain"?0.:10.;f.maximum=n=="Gain"?20.:1000.;f.unit=n=="Gain"?"dB":"us";h->nodes[n]=f;}
        for(const auto& n:QStringList{"ExposureAuto","GainAuto"}){CameraFeature f;f.name=n;f.type=FeatureType::Enumeration;f.access=FeatureAccess::ReadWrite;
            f.value=QString("Off");f.entries={{"Off",0},{"Continuous",1}};h->nodes[n]=f;}
    }
    auto open(std::optional<IndustrialDiscoverySnapshot> devices={},MeasurementEnvironmentRepository::Fault fault=MeasurementEnvironmentRepository::Fault::None){
        return CameraSettingsService::open(repo,r.camera.id,repo.load().revision,devices.value_or(IndustrialDiscoverySnapshot{{r.device},{},false}),local,factory,
            [this]{return !busy;},[this]{++invalidations;},fault);
    }
};
void complete(CameraSettingsSession& s){until([&]{s.poll();return s.state()==CameraState::Connected || s.state()==CameraState::Configured || s.state()==CameraState::Fault;});}
void connect(CameraSettingsSession& s){s.connect();complete(s);check(s.state()==CameraState::Connected,"settings connected");}
void apply(CameraSettingsSession& s,CameraSettingsValues v){s.apply(v);complete(s);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    {Fixture f;const auto original=f.repo.load();auto s=f.open();connect(*s);const auto dto=s->snapshot();
        check(dto.exposure.minimum==10 && dto.exposure.maximum==1000 && dto.exposure.unit=="us" && std::get<double>(dto.exposure.value)==100,"device ranges/units/initial values");
        apply(*s,{100,1});check(s->snapshot().saved && !s->snapshot().profileChanged && f.h->writes==0 && f.invalidations==0 && f.repo.load().encode()==original.encode(),"same settings no writes/revision/invalidation");}
    {Fixture f;auto before=f.repo.load();auto other=f.r.camera;other.id={"other"};other.physical->serial="other";
        MeasurementEnvironmentRegistry reg(f.repo);reg.registerCamera(other,"CAM_02",before.revision);before=f.repo.load();
        auto s=f.open();connect(*s);apply(*s,{250,2});const auto after=f.repo.load();
        check(s->snapshot().saved && s->snapshot().profileChanged && !s->snapshot().geometryChanged && after.current.profiles.first().requested.exposureTimeUs==250,"apply readback then persist");
        check(after.current.cameras==before.current.cameras && after.current.profiles.last()==before.current.profiles.last() && after.current.setups==before.current.setups,"other camera/setup untouched");}
    for(int scenario=0;scenario<8;++scenario){Fixture f;const auto before=f.repo.load().encode();auto s=f.open();connect(*s);
        CameraSettingsValues v{250,2};if(scenario==0)v.exposureUs=1001;if(scenario==1)v.exposureUs=std::nan("");
        if(scenario==2)f.h->nodes["Gain"].access=FeatureAccess::Missing;
        if(scenario==3)f.h->clamp="ExposureTime";if(scenario==4)f.h->failure="Gain";
        if(scenario==5){f.h->failure="Gain";f.h->rollbackFailure=true;}
        if(scenario==6)f.h->disconnect=true;
        if(scenario==7)f.h->nodes["ExposureTime"].unit="ms";
        apply(*s,v);check(s->state()==CameraState::Fault && f.repo.load().encode()==before && !s->snapshot().saved,"negative leaves registry unchanged");
        if(scenario==3 || scenario==4)check(f.h->actual.settings==f.r.profile.requested && !s->snapshot().unusable,"readback/partial failure rollback verified");
        if(scenario==5)check(s->snapshot().unusable && s->error().contains("rollback unverified"),"rollback failure explicit unusable");
    }
    {Fixture f;auto s=f.open();connect(*s);f.busy=true;reject([&]{s->apply({250,2});});check(f.h->writes==0,"Runtime busy before write");}
    {Fixture f;f.h->dynamicAuto=true;f.h->nodes["ExposureAuto"].value=QString("Continuous");f.h->actual.settings.exposureAuto=AutoMode::Continuous;
        auto s=f.open();connect(*s);apply(*s,{250,2});check(s->snapshot().saved && s->snapshot().actual.settings.exposureAuto==AutoMode::Off,"manual disables supported auto before dynamic numeric write");}
    {Fixture f;f.h->rounded=true;auto s=f.open();connect(*s);apply(*s,{250.1,2});const auto result=s->snapshot();
        check(result.saved && result.requested.exposureUs==250.1 && result.actual.settings.exposureTimeUs==double(float(250.1)) && f.repo.load().current.profiles.first().requested.exposureTimeUs==250.1,"requested/readback float distinction");}
    {Fixture f;auto s=f.open();connect(*s);f.h->afterWrite=[&]{auto e=f.repo.load();auto next=e;next.current.cameras.first().name="Concurrent rename";f.repo.commit(e,next);};
        apply(*s,{250,2});check(s->state()==CameraState::Fault && f.repo.load().current.cameras.first().name=="Concurrent rename" && f.h->actual.settings==f.r.profile.requested,"CAS conflict after hardware write restores hardware, preserves concurrent registry");}
    {Fixture f;f.busy=true;reject([&]{f.open();});}
    {Fixture f;auto s=f.open();connect(*s);MeasurementEnvironmentRegistry reg(f.repo);reg.renameCamera(f.r.camera.id,"CAM_01","Renamed",f.repo.load().revision);
        reject([&]{s->apply({250,2});});check(f.h->writes==0,"stale registry before worker dispatch");}
    {Fixture f;auto s=f.open({},MeasurementEnvironmentRepository::Fault::BeforeCommit);connect(*s);auto before=f.repo.load().encode();apply(*s,{250,2});
        check(s->state()==CameraState::Fault && f.repo.load().encode()==before && f.h->actual.settings==f.r.profile.requested,"disk failure restores hardware, no atomicity claim");}
    {Fixture f;auto lease=f.factory.configure(f.r,CameraLeaseUse::Runtime);reject([&]{f.open();});}
    {Fixture f;auto s=f.open();connect(*s);s->stop();reject([&]{s->apply({250,2});});}
    {Fixture f;auto s=f.open();connect(*s);std::promise<void> entered,release;auto released=release.get_future().share();
        f.h->afterWrite=[&]{entered.set_value();released.wait();};auto started=entered.get_future();
        s->apply({250,2});const bool ready=started.wait_for(std::chrono::seconds(2))==std::future_status::ready;
        if(!ready)release.set_value();check(ready,"settings write started");
        s->stop();release.set_value();until([&]{s->poll();return s->state()==CameraState::Fault;});
        check(!s->snapshot().saved && f.h->actual.settings==f.r.profile.requested && f.invalidations==1,"Stop after write rolls back, invalidates before lease release");}
    {Fixture f;f.factory=CameraProviderFactory{};reject([&]{f.open();});}
    {Fixture f;f.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(std::make_shared<State>()));
        auto s=f.open();s->connect();complete(*s);check(s->state()==CameraState::Fault && s->error().contains("unsupported"),"legacy backend without feature readback fails explicitly");}
    {Fixture f;f.h->nodes["GainAuto"].value=QString("Continuous");f.h->nodes["GainAuto"].access=FeatureAccess::ReadOnly;
        f.h->actual.settings.gainAuto=AutoMode::Continuous;auto s=f.open();connect(*s);apply(*s,{250,2});
        check(s->state()==CameraState::Fault && f.h->writes==0,"unsupported manual transition rejected before mutation");}
    {Fixture f;QFile::remove(f.repo.projectPath());reject([&]{f.open();});}
    {Fixture f;auto second=f.r.device;second.route={"gentl",f.dir.filePath("fixture.cti"),"second","second"};second.backend.adapter="gentl";
        IndustrialDiscoverySnapshot d{{f.r.device,second},{},false};reject([&]{f.open(d);});
        f.local.setRoute(f.r.device.identity,second.route);{auto s=f.open(d);connect(*s);}
        f.local.setRoute(f.r.device.identity,TransportLocator{"oecs",{},"stale","stale"});
        reject([&]{f.open(d);});{auto s=f.open();connect(*s);}
        d.cancelled=true;reject([&]{f.open(d);});}
    {Fixture f;auto e=f.repo.load();e.current.setups.first().geometry.resolution={2,3};e.current.setups.first().geometry.sensorRoi=QRect(0,0,2,3);
        MeasurementEnvironmentRegistry reg(f.repo);e.current.setups.first()=f.r.setup.revised(e.current.setups.first());reg.replace(e.current);
        auto s=f.open();connect(*s);apply(*s,{250,2});check(s->snapshot().geometryChanged && f.repo.load().current.setups.first().revision==3,"critical geometry reported separately");}
    {Fixture f;MeasurementEnvironmentRegistry reg(f.repo);auto camera=f.r.camera;camera.id={"pending"};camera.physical->serial="pending";
        const auto id=reg.registerCamera(camera,"CAM_02",reg.snapshot().revision).camera;auto d=f.r.device;d.identity=*camera.physical;
        auto s=CameraSettingsService::open(f.repo,id,f.repo.load().revision,{{d},{},false},f.local,f.factory,[]{return true;},[]{});connect(*s);apply(*s,{250,2});
        const auto after=f.repo.load();check(s->snapshot().saved && after.defaults(id).status==CameraDefaultsStatus::Pending && after.current.setups.last().pending && !after.current.profiles.last().pending,"pending optics not fabricated, profile initialized from device");}
    QTextStream(stdout)<<"PASS camera_settings: worker DTO, readback/persistence, no-op, isolation, limits/NaN/unsupported, mismatch/disconnect/apply/rollback/disk failure, busy/lease/stale/Stop, routes, geometry, pending defaults\n";
    return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
