#include "CameraAcquisitionFixture.hpp"
#include "OecsExperimentalFixture.hpp"
#include "application/RuntimeCoordinator.hpp"
#include "application/RuntimeCameraSessions.hpp"
#include <QLockFile>
using namespace visionflow;
namespace runtime_fixture {
using persistent_fixture::check;using persistent_fixture::rejects;using persistent_fixture::Fixture;
struct Gate {std::promise<void> promise;bool released=false;void release(){if(!released){released=true;promise.set_value();}}~Gate(){release();}};
template<class F>void until(F condition){Deadline timeout(10000);while(!condition()){check(!timeout.expired(),"Runtime fixture timeout");QThread::msleep(1);}}
void settle(RuntimeCoordinator& c,RuntimeService& r){until([&]{c.poll();return c.state()!=RuntimeState::Starting && c.state()!=RuntimeState::Stopping && !r.camerasPending();});}
RuntimeIdentity identity(){return {"engineer",[](const QString& p,ProductionAction){return p=="engineer";}};}
struct Setup {
    QTemporaryDir directory{QDir(QCoreApplication::applicationDirPath()).filePath("runtime-cameras-XXXXXX")};
    ProjectService project;
    MultiSourceDependencies set;
    QStringList sources;
    Setup(Fixture& f,ApplicationType type=ApplicationType::Measurement){
        check(directory.isValid(),"Runtime fixture directory");project.createProject("Runtime cameras","",directory.filePath("p.vfproject"),"file");project.createRecipe("Camera run","",type,"file");
        for(const auto& r:f.requests){const auto id=project.addTool("industrial_camera");project.bindIndustrialSource(id,f.inputs.contract,r.profile.id,3000);sources<<id;set.sources[id]={};}
        project.save();set.industrial=std::make_shared<const IndustrialExecutionInputs>(f.inputs);
    }
    Recipe recipe() const{return *project.activeRecipe();}
    QString archive() const{return directory.filePath("archive");}
};
void matrix(QJsonArray& rows){
    for(int count:{1,5})for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        Fixture f("runtime-"+QString::number(count)+typeKey(type),count);Setup s(f,type);RuntimeService runtime(s.archive(),s.project.path()+".calibrations.json",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(runtime);
        c.load(s.recipe(),s.directory.path());check(*f.discoveries==0 && !runtime.active(),"Load is offline with zero open");c.start();check(c.state()==RuntimeState::Starting,"Industrial Run remains Starting until async preparation");settle(c,runtime);
        check(c.state()==RuntimeState::Running && runtime.records().isEmpty(),"Run opens complete set without result or capture");for(const auto& p:f.probes)check(p->opens==1 && p->configs==1 && p->grabs==0,"Run prepares once");
        for(int i=0;i<20;++i){const auto cycle=c.begin(RuntimeTrigger::Manual);const auto result=RuntimeService::execute(cycle,"",c.cancellation());check(result.state==ExecutionState::Done && result.frames.size()==count && result.industrialEvidence.size()==count,"actual native immutable cycle");c.finish(cycle.ticket,result);}
        check(*f.discoveries==count,"Cycles/current checks never discover again");for(const auto& p:f.probes)check(p->opens==1 && p->configs==1 && p->grabs==20 && p->closes==0,"twenty cycles reuse connection");
        c.stop();settle(c,runtime);check(c.state()==RuntimeState::LoadedStopped && runtime.records().size()==20,"Idle Stop closes all cameras, keeps terminal history");for(const auto& p:f.probes)check(p->closes==1 && !p->wrongThread,"Stop closes once with thread affinity");
        c.start();settle(c,runtime);check(c.state()==RuntimeState::Running && f.first()->opens==2,"Manual Run opens new session");c.stop();settle(c,runtime);
        rows.append(QJsonObject{{"cameras",count},{"type",typeKey(type)},{"cycles",20},{"openPerCameraBeforeStop",1},{"configurePerCameraBeforeStop",1},{"grabsPerCamera",20},{"discoveries",count}});
    }
}
void partialAndLate(){
    {Fixture f("rollback",5);f.probes[f.requests[2].device.identity.serial]->failConfigure=true;Setup s(f);RuntimeService r(s.archive(),"",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(r);c.load(s.recipe(),s.directory.path());c.start();settle(c,r);
        check(c.state()==RuntimeState::Fault && r.records().isEmpty(),"Partial prepare cannot enter Running or create a result");for(const auto& p:f.probes)check(p->opens==1 && p->closes==1,"Partial failure rolls back every camera");
        f.probes[f.requests[2].device.identity.serial]->failConfigure=false;c.start();settle(c,r);check(c.state()==RuntimeState::Running && f.first()->opens==2,"Explicit Run retries whole set");c.stop();settle(c,r);}
    {Fixture f("late-open-runtime");Setup s(f);RuntimeService r(s.archive(),"",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(r);c.load(s.recipe(),s.directory.path());Gate gate;f.first()->openGate=gate.promise.get_future().share();
        c.start();until([&]{return f.first()->openEntered.load();});c.stop();check(c.state()==RuntimeState::Stopping && r.camerasPending(),"Starting Stop retains pending lease");rejects([&]{c.start();});gate.release();settle(c,r);
        check(c.state()==RuntimeState::LoadedStopped && f.first()->opens==1 && f.first()->closes==1 && f.first()->grabs==0,"Late open closes without Running/capture");f.first()->openGate={};c.start();settle(c,r);check(c.state()==RuntimeState::Running,"Run after actual late-open drain");c.stop();settle(c,r);}
    {Fixture f("late-frame-runtime");Setup s(f);RuntimeService r(s.archive(),"",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(r);c.load(s.recipe(),s.directory.path());c.start();settle(c,r);Gate gate;f.first()->grabGate=gate.promise.get_future().share();
        const auto cycle=c.begin(RuntimeTrigger::Manual);const auto cancel=c.cancellation();auto work=std::async(std::launch::async,[cycle,cancel]{return RuntimeService::execute(cycle,"",cancel);});until([&]{return f.first()->grabEntered.load();});c.stop();
        const auto result=work.get();check(result.state==ExecutionState::Skipped && result.frames.isEmpty(),"Late Stop suppresses output before physical SDK returns");c.finish(cycle.ticket,result);check(c.state()==RuntimeState::Stopping,"Stopping includes SDK drain after persistence");gate.release();settle(c,r);check(c.state()==RuntimeState::LoadedStopped && f.first()->closes==1,"In-flight Stop drains then allows Run");}
}
void dependencyDisconnectAndRetry(){
    {Fixture f("runtime-close-failure");Setup s(f);RuntimeService r(s.archive(),"",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(r);c.load(s.recipe(),s.directory.path());c.start();settle(c,r);f.first()->failClose=true;c.stop();settle(c,r);
        check(c.state()==RuntimeState::Fault && r.cameraCloseFailed(),"Uncertain Runtime cleanup is Fault with quarantined lease");c.start();settle(c,r);check(c.state()==RuntimeState::Fault && f.first()->opens==1,"Uncertain lease is never reopened");}
    {Fixture f("changed-start");Setup s(f);RuntimeService r(s.archive(),"",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(r);c.load(s.recipe(),s.directory.path());Gate gate;f.first()->openGate=gate.promise.get_future().share();c.start();until([&]{return f.first()->openEntered.load();});s.set.sources[s.sources.first()].engineVersion="changed";gate.release();settle(c,r);
        check(c.state()==RuntimeState::Fault && f.first()->closes==1 && r.records().isEmpty(),"Changed start dependencies reject acceptance and drain");c.stop();rejects([&]{c.start();});check(f.first()->opens==1,"Changed snapshot requires explicit reload before reopening");s.set.sources[s.sources.first()].engineVersion=ExecutionService::version();c.load(s.recipe(),s.directory.path());c.start();settle(c,r);check(c.state()==RuntimeState::Running,"Explicit reload accepts compatible frozen dependency set");c.stop();settle(c,r);}
    {Fixture f("disconnect");Setup s(f);RuntimeService r(s.archive(),"",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(r);c.load(s.recipe(),s.directory.path());c.start();settle(c,r);f.first()->failGrab=true;
        auto cycle=c.begin(RuntimeTrigger::Manual);auto result=RuntimeService::execute(cycle,"",c.cancellation());check(result.state==ExecutionState::Error,"SDK disconnect gives an error");c.finish(cycle.ticket,result);settle(c,r);check(c.state()==RuntimeState::Fault && f.first()->closes==1,"Disconnect closes set and stops admission");
        f.first()->failGrab=false;c.poll();check(c.state()==RuntimeState::Fault && f.first()->opens==1,"Replug never auto Run");c.start();settle(c,r);check(c.state()==RuntimeState::Running && f.first()->opens==2,"Explicit Run after reconnect");
        s.set.sources[s.sources.first()].assetHashes["changed"]=QString(64,'a');rejects([&]{c.begin(RuntimeTrigger::Manual);});settle(c,r);check(c.state()==RuntimeState::Fault && f.first()->closes==2,"Changed running snapshot closes admission and cameras");c.stop();rejects([&]{c.start();});check(f.first()->opens==2,"Stop cannot silently accept changed running dependencies");}
    {Fixture f("record-retry");Setup s(f);RuntimeService r(s.archive(),"",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(r);c.load(s.recipe(),s.directory.path());c.start();settle(c,r);
        const auto cycle=c.begin(RuntimeTrigger::Manual);const auto result=RuntimeService::execute(cycle,"",c.cancellation());QLockFile lock(s.archive()+"/archive.lock");check(lock.tryLock(0),"Record fixture lock");rejects([&]{c.finish(cycle.ticket,result);});check(c.recordingPending() && r.busy(),"Trusted recording retry remains pending");c.stop();until([&]{c.poll();return !r.camerasPending();});
        check(c.state()==RuntimeState::Stopping && f.first()->closes==1,"Disk retry cannot retain camera after Stop");auto other=f.inputs.factory.open(f.requests.first(),CameraLeaseUse::Editor,FrameQueueMode::ProductionFifo,1);other.reset();lock.unlock();c.finish(cycle.ticket,result);
        check(c.state()==RuntimeState::LoadedStopped && r.records().size()==1 && f.first()->opens==1 && f.first()->grabs==1,"Record retry persists same terminal without reacquisition or reopening");}
}
void sharedCamera(){
    Fixture f("shared-runtime");Setup s(f);const auto alias=s.project.addTool("industrial_camera");s.project.bindIndustrialSource(alias,f.inputs.contract,f.requests.first().profile.id,3000);s.project.save();s.set.sources[alias]={};
    RuntimeService r(s.archive(),"",identity(),{},{},{},[&](const Recipe&){return s.set;});RuntimeCoordinator c(r);c.load(s.recipe(),s.directory.path());c.start();settle(c,r);check(c.state()==RuntimeState::Running && f.first()->opens==1,"Same physical/profile sources share prepared Runtime connection");
    const auto cycle=c.begin(RuntimeTrigger::Manual);const auto result=RuntimeService::execute(cycle,"",c.cancellation());check(result.state==ExecutionState::Done && result.frames.size()==2 && result.frames[0].frameId!=result.frames[1].frameId && f.first()->grabs==2,"Shared connection has distinct sequential source captures");c.finish(cycle.ticket,result);c.stop();settle(c,r);
}
void experimental(){
    namespace exp=oecs_experimental_fixture;auto state=exp::state();auto request=exp::request();request.profile.requested.exposureTimeUs=6000;QTemporaryDir dir;
    ProjectService p;p.createProject("OECS","",dir.filePath("p.vfproject"),"file");p.createRecipe("OECS","",ApplicationType::Measurement,"file");const auto source=p.addTool("industrial_camera");p.bindIndustrialSource(source,{{request.camera},{request.setup},{request.profile}},request.profile.id,3000);
    MultiSourceDependencies set;set.sources[source]={};IndustrialExecutionInputs in;in.contract={{request.camera},{request.setup},{request.profile}};in.discover=[request](int,const auto&){return IndustrialDiscoverySnapshot{{request.device},{},false};};set.industrial=std::make_shared<const IndustrialExecutionInputs>(in);
    auto opens=std::make_shared<std::atomic_int>(0),closes=std::make_shared<std::atomic_int>(0);
    class Api final:public exp::Api {std::shared_ptr<std::atomic_int> opens_,closes_;public:Api(std::shared_ptr<exp::State> s,std::shared_ptr<std::atomic_int> o,std::shared_ptr<std::atomic_int> c):exp::Api(s),opens_(o),closes_(c){}int openExclusive(void* h) override{++*opens_;return exp::Api::openExclusive(h);}int closeDevice(void* h) override{++*closes_;return exp::Api::closeDevice(h);}};
    auto backend=std::make_shared<AcquisitionOecsBackend>();backend->runtime=dir.filePath("fixture.dll");backend->api=[state,opens,closes]{return std::make_unique<Api>(state,opens,closes);};RuntimeCameraSessions cameras;cameras.start(*p.activeRecipe(),set,true,backend);
    until([&]{cameras.poll();return !cameras.pending();});check(cameras.ready(),"Experimental Runtime prepare");const auto prepared=cameras.compose(set);auto r=industrialExecutionRequest(*prepared.industrial,editorTools(p.activeRecipe()->definition).last());
    for(int i=0;i<20;++i){const auto frame=acquireIndustrial(*prepared.industrial,r,"cycle-"+QString::number(i),"frame-"+QString::number(i),3000,{},true);check(!frame.evidence && frame.image.metadata().experimentalAcquisition["productionAdmission"]==false,"Prepared Runtime does not promote OECS admission");}
    check(*opens==1 && state->calls.count("start")==20 && state->calls.count("stop")==20,"OECS open reused; stream cleanup stays per frame");cameras.stop();until([&]{cameras.poll();return !cameras.pending();});check(*closes==1,"Experimental idle Stop closes once");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{QJsonArray rows;runtime_fixture::matrix(rows);runtime_fixture::partialAndLate();runtime_fixture::dependencyDisconnectAndRetry();runtime_fixture::sharedCamera();runtime_fixture::experimental();
    QJsonObject report{{"status","PASS"},{"physicalAcceptance",false},{"productionAdmissionOpened",false},{"matrix",rows},{"cases",QJsonArray{"offlineLoad","runPrepare","repeat20","idleStop","reRun","partialRollback5","startingStopLateOpen","inFlightStopLateFrame","changedStartSnapshot","disconnectManualReRun","changedRunningSnapshot","diskRetryAfterDrain","sharedCamera","experimental20"}}};
    if(argc>1){QSaveFile file(QString::fromLocal8Bit(argv[1]));runtime_fixture::check(file.open(QIODevice::WriteOnly),"report open");file.write(QJsonDocument(report).toJson());runtime_fixture::check(file.commit(),"report save");}QTextStream(stdout)<<QJsonDocument(report).toJson();return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
