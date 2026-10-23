#include "application/CameraAcquisitionSessions.hpp"
#include "application/ProjectService.hpp"
#include "OecsExperimentalFixture.hpp"
#include <QJsonDocument>
#include <QTextStream>
#include <QSaveFile>
#include <set>
using namespace visionflow;
namespace persistent_fixture {
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
template<class F> void until(F f){Deadline d(5000);while(!f()){check(!d.expired(),"fixture timeout");QThread::msleep(1);}}
template<class T> T take(std::future<T> future){check(future.wait_for(std::chrono::seconds(6))==std::future_status::ready,"future timeout");return future.get();}
struct Probe {
    std::atomic_int created=0,opens=0,configs=0,grabs=0,closes=0,destroyed=0;
    std::atomic_bool wrongThread=false,openEntered=false,grabEntered=false,closeEntered=false;
    bool failConfigure=false,failGrab=false,failClose=false;
    std::shared_future<void> openGate,grabGate,closeGate;
};
IndustrialCameraRequest request(QString serial="persistent"){
    IndustrialCameraRequest r;
    r.device={{"VF","SESSION",serial,DeviceIdentityConfidence::SerialReported},{"oecs",{},"fixture","exact"},{"oecs","fixture",QString(64,'a'),1}};
    r.camera={{serial+"/camera"},"Synthetic camera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},r.device.identity};
    GeometryDescriptor g;g.resolution={3,2};g.sensorRoi=QRect(0,0,3,2);g.binning=QSize(1,1);g.decimation=QSize(1,1);g.reverse=GeometryMirror{};g.complete=true;
    r.setup={{serial+"/setup"},r.camera.id,"Synthetic setup","lens","plane",1,1,g};
    r.profile={{serial+"/profile"},r.camera.id,r.setup.id,"Synthetic profile",1,{"Mono8",{true,"FrameStart","Software"},AutoMode::Off,100,AutoMode::Off,1,{}}};
    r.sourceId="source-A";r.validate();return r;
}
class Backend final:public IndustrialCameraBackend {
    IndustrialCameraRequest r_;
    std::shared_ptr<Probe> p_;
    std::thread::id thread_=std::this_thread::get_id();
    bool opened_=false;
    void thread() const {if(thread_!=std::this_thread::get_id())p_->wrongThread=true;}
public:
    Backend(IndustrialCameraRequest r,std::shared_ptr<Probe> p):r_(std::move(r)),p_(std::move(p)){++p_->created;}
    ~Backend() override {thread();++p_->destroyed;}
    PhysicalDeviceIdentity actualIdentity() const override {thread();return r_.device.identity;}
    AcquisitionReadback readback() const override {
        thread();GeometryReadback g;g.resolution=r_.setup.geometry.resolution;g.offset=r_.setup.geometry.sensorRoi->topLeft();g.binning=*r_.setup.geometry.binning;g.decimation=*r_.setup.geometry.decimation;g.reverse=*r_.setup.geometry.reverse;
        g.offsetEvidence=g.binningEvidence=g.decimationEvidence=g.reverseEvidence=GeometryEvidence::Reported;return {g,r_.profile.requested};
    }
    CameraWorkerResult execute(const CameraTicket& t) override {
        thread();if(t.operation==CameraOperation::Connect){p_->openEntered=true;if(p_->openGate.valid())p_->openGate.wait();opened_=true;++p_->opens;}
        if(t.operation==CameraOperation::Configure){++p_->configs;if(p_->failConfigure)throw std::runtime_error("fixture configuration failure");}
        if(t.operation==CameraOperation::Grab){p_->grabEntered=true;if(p_->grabGate.valid())p_->grabGate.wait();if(p_->failGrab)throw std::runtime_error("fixture capture failure");
            const int n=++p_->grabs;const unsigned char pixels[6]={static_cast<unsigned char>(n),2,3,4,5,6};FrameMetadata m;
            m.sourceId=r_.sourceId;m.frameId=r_.sourceId+"/hardware-"+QString::number(n);m.cycleId="backend-cycle-"+QString::number(n);m.coordinateSystemId=r_.sourceId+"/image";m.completedAtUtc=QDateTime::currentDateTimeUtc();
            auto image=ImageFrame::fromBuffer(pixels,6,3,2,3,PixelFormat::Gray8,m);return {true,{},AcquiredFrame{image,m.sourceId,m.cycleId,m.frameId,m.completedAtUtc}};
        }return {true,{},{}};
    }
    void shutdown() override {thread();p_->closeEntered=true;if(p_->closeGate.valid())p_->closeGate.wait();if(opened_){++p_->closes;opened_=false;}if(p_->failClose)throw std::runtime_error("fixture uncertain close");}
};
class Provider final:public IndustrialCameraProvider {
    QMap<QString,std::shared_ptr<Probe>> probes_;
public:
    explicit Provider(QMap<QString,std::shared_ptr<Probe>> p):probes_(std::move(p)){}
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice& d) const override {return {probes_.contains(d.identity.serial),true,true,true,true,true,{}};}
    std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest& r) const override {check(probes_.contains(r.device.identity.serial),"unknown fixture serial");return std::make_unique<Backend>(r,probes_[r.device.identity.serial]);}
};
struct Fixture {
    QVector<IndustrialCameraRequest> requests;
    QMap<QString,std::shared_ptr<Probe>> probes;
    IndustrialExecutionInputs inputs;
    std::shared_ptr<std::atomic_int> discoveries=std::make_shared<std::atomic_int>(0);
    explicit Fixture(QString serial,int cameras=1){
        QVector<DiscoveredDevice> devices;
        for(int i=0;i<cameras;++i){auto r=request(serial+QString::number(i));r.sourceId="source-"+QString::number(i);requests.append(r);devices.append(r.device);probes[r.device.identity.serial]=std::make_shared<Probe>();
            inputs.contract.cameras.append(r.camera);inputs.contract.setups.append(r.setup);inputs.contract.profiles.append(r.profile);}
        inputs.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(probes));inputs.discover=[devices,count=discoveries](int,const auto&){++*count;return IndustrialDiscoverySnapshot{devices,{},false};};
    }
    std::shared_ptr<Probe> first(){return probes[requests.first().device.identity.serial];}
};
void repeatedAndSharing(QJsonObject& report){
    Fixture f("repeat");auto service=std::make_shared<CameraAcquisitionSessions>();check(service->status("editor").drained && *f.discoveries==0,"construction opens nothing");
    auto prepared=take(service->prepare("editor",f.inputs,f.requests,false));check(prepared.allReady() && f.first()->opens==1 && f.first()->configs==1 && f.first()->grabs==0,"prepare does not capture");
    auto inputs=f.inputs;inputs.sessions=service;inputs.sessionContext="editor";inputs.sessionGeneration=prepared.generation;
    std::set<QString> ids;
    for(int i=0;i<20;++i){auto result=take(std::async(std::launch::async,[inputs,r=f.requests.first(),i]{return acquireIndustrial(inputs,r,"cycle-"+QString::number(i),"frame-"+QString::number(i),3000,{});}));
        check(result.image.metadata().sourceId==f.requests.first().sourceId && result.image.metadata().cycleId=="cycle-"+QString::number(i) && result.evidence,"request identity");ids.insert(result.evidence->acquired.frameId);}
    check(ids.size()==20 && f.first()->opens==1 && f.first()->configs==1 && f.first()->grabs==20 && f.first()->closes==0 && *f.discoveries==1,"1 open/configure + 20 distinct grabs without discovery/close");
    auto b=f.requests.first();b.sourceId="source-B";auto aliases=f.requests;aliases.append(b);
    auto shared=take(service->prepare("editor",f.inputs,aliases,false));check(shared.allReady() && f.first()->opens==1,"same camera sources reuse connection");
    auto aFrame=take(service->acquire("editor",shared.generation,f.requests.first(),"cycle-A","frame-A",3000));
    auto bFrame=take(service->acquire("editor",shared.generation,b,"cycle-B","frame-B",3000));
    check(aFrame.image.metadata().sourceId!=bFrame.image.metadata().sourceId && aFrame.evidence->acquired.frameId!=bFrame.evidence->acquired.frameId &&
        bFrame.evidence->acquired.sourceId==b.sourceId && bFrame.evidence->workerAcquired->sourceId==f.requests.first().sourceId,"separate capture and bound metadata per source");
    auto onlyB=take(service->prepare("editor",f.inputs,{b},false));check(onlyB.allReady() && f.first()->closes==0,"removing an alias keeps last source lease");
    auto last=take(service->acquire("editor",onlyB.generation,b,"last","last",3000));check(last.image.metadata().sourceId==b.sourceId,"removed source not reused as output");
    auto none=take(service->prepare("editor",f.inputs,{},false));check(none.drained && f.first()->closes==1,"last source removal releases connection");
    auto reopened=take(service->prepare("editor",f.inputs,f.requests,false));check(reopened.allReady() && f.first()->opens==2,"new context opens again");
    check(take(service->close("editor")).clean && f.first()->closes==2 && f.first()->destroyed==2 && !f.first()->wrongThread,"idle close drains and correct SDK thread");
    check(service->shutdown().get().clean,"shutdown clean");
    report["repeat"] = QJsonObject{{"firstSessionOpen",1},{"firstSessionConfigure",1},{"firstSessionGrabs",20},{"firstSessionCloseBeforeExit",0},{"discoveriesBeforeReopen",1},{"distinctFrames",int(ids.size())}};
}
void reconciliationAndExecution(){
    Fixture f("reconcile");auto service=std::make_shared<CameraAcquisitionSessions>();auto r=f.requests.first();auto prepared=take(service->prepare("editor",f.inputs,{r},false));
    auto moved=r;moved.device.route.deviceId="different-route";rejects([&]{service->acquire("editor",prepared.generation,moved,"wrong-route","wrong-route",3000);});
    moved=r;moved.device.backend.runtimeSha256=QString(64,'b');rejects([&]{service->acquire("editor",prepared.generation,moved,"wrong-backend","wrong-backend",3000);});
    auto conflicting=r;conflicting.sourceId="other";conflicting.profile.id={"conflict"};conflicting.profile.requested.exposureTimeUs=200;auto conflictInputs=f.inputs;conflictInputs.contract.profiles.append(conflicting.profile);
    rejects([&]{service->prepare("editor",conflictInputs,{r,conflicting},false);});check(f.first()->opens==1 && service->status("editor").generation==prepared.generation,"conflict rejected before mutating healthy session");
    r.camera.name="Renamed";r.setup.name="Renamed setup";r.profile.name="Renamed profile";auto renamed=f.inputs;renamed.contract={{r.camera},{r.setup},{r.profile}};
    auto same=take(service->prepare("editor",renamed,{r},false));check(same.allReady() && f.first()->configs==1 && *f.discoveries==1,"rename does not reconfigure");
    ++r.profile.revision;r.profile.requested.exposureTimeUs=200;auto changed=renamed;changed.contract.profiles={r.profile};
    auto revised=take(service->prepare("editor",changed,{r},false));check(revised.allReady() && f.first()->opens==2 && f.first()->configs==2 && f.first()->closes==1,"revision closes and prepares new session");
    rejects([&]{service->acquire("editor",same.generation,r,"old","old",3000);});
    auto old=r;--old.profile.revision;old.profile.requested.exposureTimeUs=100;rejects([&]{service->acquire("editor",revised.generation,old,"stale","stale",3000);});
    // Exercise the common ExecutionService seam, including duplicate-source DAG.
    QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("ccux22b-XXXXXX"));check(dir.isValid(),"execution fixture directory");
    ProjectService project;project.createProject("Persistent","",dir.filePath("p.vfproject"),"file");project.createRecipe("Two","",ApplicationType::Measurement,"file");
    const auto first=project.addTool("industrial_camera");project.bindIndustrialSource(first,changed.contract,r.profile.id,3000);const auto second=project.addTool("industrial_camera");project.bindIndustrialSource(second,changed.contract,r.profile.id,3000);
    auto a=r,b=r;a.sourceId=first;b.sourceId=second;auto ready=take(service->prepare("editor",changed,{a,b},false));changed.sessions=service;changed.sessionContext="editor";changed.sessionGeneration=ready.generation;
    const auto result=ExecutionService(changed).run(project.activeRecipe()->definition,dir.path(),project.activeRecipe()->id,{},{});
    check(result.state==ExecutionState::Done && result.frames.size()==2 && result.industrialEvidence.size()==2 && result.frames[0].frameId!=result.frames[1].frameId,"shared physical camera is two actual source acquisitions");
    auto detached=changed;detached.sessions.reset();check(ExecutionService(detached).run(project.activeRecipe()->definition,dir.path(),project.activeRecipe()->id,{},{}).state==ExecutionState::Error,"legacy path keeps duplicate preflight closed");
    check(take(service->close("editor")).clean && service->shutdown().get().clean,"execution close");
}
void partialAndLease(){
    Fixture f("multi",5);auto service=std::make_shared<CameraAcquisitionSessions>();f.probes[f.requests[2].device.identity.serial]->failConfigure=true;
    auto status=take(service->prepare("runtime",f.inputs,f.requests,false));check(!status.allReady() && status.sources[f.requests[2].sourceId].state==AcquisitionSessionState::Fault,"partial preparation reports source failure");
    for(int i=0;i<5;++i)if(i!=2)check(status.sources[f.requests[i].sourceId].state==AcquisitionSessionState::Ready,"other cameras remain individually ready");
    rejects([&]{service->acquire("runtime",status.generation,f.requests[2],"bad","bad",3000);});
    auto other=take(service->prepare("other-context",f.inputs,{f.requests[0]},false));check(!other.allReady() && f.first()->opens==1,"another context cannot steal lease");
    check(take(service->close("other-context")).clean && take(service->close("runtime")).clean,"rollback all prepared cameras");
    for(const auto& probe:f.probes)check(probe->opens==1 && probe->closes==1 && !probe->wrongThread,"five camera rollback and affinity");
    auto outside=f.inputs.factory.open(f.requests[0],CameraLeaseUse::Calibration,FrameQueueMode::ProductionFifo,1);
    auto busy=take(service->prepare("editor",f.inputs,{f.requests[0]},false));check(!busy.allReady(),"external settings/preview/calibration worker lease blocks service");outside.reset();
    auto retry=take(service->prepare("editor",f.inputs,{f.requests[0]},false));check(retry.allReady(),"explicit retry after external lease release");check(take(service->close("editor")).clean && service->shutdown().get().clean,"lease shutdown");
}
void lateOpenAndDiscovery(){
    for(bool discoveryGate:{false,true}){
        Fixture f(discoveryGate?"late-discovery":"late-open");auto service=std::make_shared<CameraAcquisitionSessions>();std::promise<void> gate;auto released=gate.get_future().share();std::atomic_bool discovered=false;
        if(discoveryGate){const auto scan=f.inputs.discover;f.inputs.discover=[scan,released,&discovered](int ms,const auto& cancel){discovered=true;released.wait();return scan(ms,cancel);};}else f.first()->openGate=released;
        auto preparing=service->prepare("editor",f.inputs,f.requests,false);until([&]{return discoveryGate?discovered.load():f.first()->openEntered.load();});
        auto closing=service->close("editor",1);until([&]{return service->status("editor").closeBudgetExceeded;});
        check(closing.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready && !service->status("editor").drained,"late open still owns context until actual cleanup");
        rejects([&]{service->prepare("editor",f.inputs,f.requests,false);});gate.set_value();rejects([&]{take(std::move(preparing));});check(take(std::move(closing)).clean,"late open is closed after cancellation");
        check(f.first()->grabs==0 && f.first()->opens==(discoveryGate?0:1) && f.first()->closes==f.first()->opens,"cancelled preparation never captures or publishes Ready");
        f.first()->openGate={};auto ready=take(service->prepare("editor",f.inputs,f.requests,false));check(ready.allReady(),"manual reopen after late open drain");check(take(service->close("editor")).clean && service->shutdown().get().clean,"late open shutdown");
    }
}
void cancelLateFrameAndDeadline(){
    {Fixture f("cancel-before-dispatch");CameraAcquisitionSessions service;auto ready=take(service.prepare("editor",f.inputs,f.requests,false));
        auto cancelled=std::make_shared<std::atomic_bool>(true);rejects([&]{take(service.acquire("editor",ready.generation,f.requests.first(),"cancelled","cancelled",3000,cancelled));});
        until([&]{return f.first()->closes==1;});check(f.first()->grabs==0 && service.status("editor").sources[f.requests.first().sourceId].state==AcquisitionSessionState::Fault,"cancel before dispatch drains without grabbing");
        check(take(service.close("editor")).clean && service.shutdown().get().clean,"cancelled idle capture drain");}
    for(bool deadline:{false,true}){
        Fixture f(deadline?"deadline":"late-frame");auto service=std::make_shared<CameraAcquisitionSessions>();auto ready=take(service->prepare("editor",f.inputs,f.requests,false));
        std::promise<void> gate;f.first()->grabGate=gate.get_future().share();auto frame=service->acquire("editor",ready.generation,f.requests.first(),"old-cycle","old-frame",deadline?100:3000);
        until([&]{return f.first()->grabEntered.load();});rejects([&]{service->acquire("editor",ready.generation,f.requests.first(),"overlap","overlap",3000);});
        if(deadline)rejects([&]{take(std::move(frame));});
        auto closing=service->close("editor",1);until([&]{return service->status("editor").closeBudgetExceeded;});check(!service->status("editor").drained,"pending late frame not released by budget");
        gate.set_value();if(!deadline)rejects([&]{take(std::move(frame));});check(take(std::move(closing)).clean,"late frame drained");f.first()->grabGate={};
        auto target=f.requests.first();target.sourceId="new-source";auto reopened=take(service->prepare("editor",f.inputs,{target},false));
        auto current=take(service->acquire("editor",reopened.generation,target,"new-cycle","new-frame",3000));check(current.image.metadata().sourceId=="new-source" && current.image.metadata().frameId=="new-frame","old completion never enters new generation");
        check(take(service->close("editor")).clean && service->shutdown().get().clean,"late frame shutdown");
    }
}
void quarantineAndDestruction(){
    {Fixture f("quarantined");auto service=std::make_shared<CameraAcquisitionSessions>();check(take(service->prepare("editor",f.inputs,f.requests,false)).allReady(),"quarantine preparation");f.first()->failClose=true;
        check(!take(service->close("editor")).clean && service->status("editor").drained && !service->status("editor").clean,"uncertain cleanup is Fault, not released");
        check(!take(service->prepare("retry",f.inputs,f.requests,false)).allReady() && f.first()->opens==1,"uncertain lease stays quarantined");check(!service->shutdown().get().clean,"shutdown preserves quarantine failure");}
    {Fixture f("destruction");auto service=std::make_shared<CameraAcquisitionSessions>();check(take(service->prepare("editor",f.inputs,f.requests,false)).allReady(),"destruction preparation");std::promise<void> gate;f.first()->closeGate=gate.get_future().share();
        auto shutdown=service->shutdown(1);until([&]{return f.first()->closeEntered.load();});until([&]{return service->status("editor").closeBudgetExceeded;});
        auto destruction=std::async(std::launch::async,[service=std::move(service)]() mutable {service.reset();});
        const bool nonblocking=destruction.wait_for(std::chrono::milliseconds(100))==std::future_status::ready;
        rejects([&]{f.inputs.factory.open(f.requests.first(),CameraLeaseUse::Editor,FrameQueueMode::ProductionFifo,1);});
        gate.set_value();destruction.get();check(nonblocking && shutdown.get().clean && f.first()->destroyed==1 && !f.first()->wrongThread,"facade destruction keeps self-owned coordinator until actual worker drain");}
}
void experimentalReuse(){
    namespace exp=oecs_experimental_fixture;
    auto state=exp::state();auto r=exp::request();r.profile.requested.exposureTimeUs=6000;auto opens=std::make_shared<std::atomic_int>(0),closes=std::make_shared<std::atomic_int>(0),loads=std::make_shared<std::atomic_int>(0);
    class CountApi final:public exp::Api {
        std::shared_ptr<std::atomic_int> opens_,closes_,loads_;
    public:
        CountApi(std::shared_ptr<exp::State> s,std::shared_ptr<std::atomic_int> o,std::shared_ptr<std::atomic_int> c,std::shared_ptr<std::atomic_int> l):exp::Api(s),opens_(o),closes_(c),loads_(l){}
        bool load(const QString& p,QString& why) override {++*loads_;return exp::Api::load(p,why);}
        int openExclusive(void* h) override {++*opens_;return exp::Api::openExclusive(h);}
        int closeDevice(void* h) override {++*closes_;return exp::Api::closeDevice(h);}
    };
    AcquisitionOecsBackend options;options.runtime=QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("fixture-oecs.dll");options.api=[state,opens,closes,loads]{return std::make_unique<CountApi>(state,opens,closes,loads);};auto backend=std::make_shared<const AcquisitionOecsBackend>(options);
    IndustrialExecutionInputs inputs;inputs.contract={{r.camera},{r.setup},{r.profile}};auto discoveries=std::make_shared<std::atomic_int>(0);inputs.discover=[r,discoveries](int,const auto&){++*discoveries;return IndustrialDiscoverySnapshot{{r.device},{},false};};
    CameraAcquisitionSessions service;auto prepared=take(service.prepare("oecs-editor",inputs,{r},true,5000,backend));check(prepared.allReady(),"OECS persistent preparation");
    auto applied=prepared.sources[r.sourceId].readback;check(applied && applied->settings.exposureTimeUs==6000,"OECS actual readback retained");
    for(int i=0;i<20;++i){auto frame=take(service.acquire("oecs-editor",prepared.generation,r,"cycle-"+QString::number(i),"frame-"+QString::number(i),3000,{},true));
        check(!frame.evidence && frame.image.metadata().experimentalAcquisition["productionAdmission"]==false && frame.image.metadata().experimentalAcquisition["triggerAssociationVerified"]==false,"ready lease cannot promote unverified policy");}
    check(*opens==1 && *loads==1 && *closes==0 && *discoveries==1 && state->calls.count("start")==20 && state->calls.count("stop")==20 && state->calls.count("command")==20,"OECS connection reused, stream start/stop still per frame");
    check(take(service.close("oecs-editor")).clean && *closes==1 && service.shutdown().get().clean,"OECS close once");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QJsonObject report{{"physicalAcceptance",false},{"productionAdmission",false}};
    persistent_fixture::repeatedAndSharing(report);persistent_fixture::reconciliationAndExecution();persistent_fixture::partialAndLease();persistent_fixture::lateOpenAndDiscovery();persistent_fixture::cancelLateFrameAndDeadline();persistent_fixture::quarantineAndDestruction();persistent_fixture::experimentalReuse();
    report["status"]="PASS";report["cases"]=QJsonArray{"repeat20","sourceSharing","reconcile","executionComposition","fiveCameraPartialRollback","leaseRival","lateDiscovery","lateOpen","cancelLateFrame","deadline","quarantine","nonblockingDestruction","oecsReuse20"};
    if(argc>1){QSaveFile file(QString::fromLocal8Bit(argv[1]));persistent_fixture::check(file.open(QIODevice::WriteOnly),"report open");file.write(QJsonDocument(report).toJson());persistent_fixture::check(file.commit(),"report commit");}
    QTextStream(stdout)<<QJsonDocument(report).toJson();return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
