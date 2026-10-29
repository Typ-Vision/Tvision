#include "application/StationExecutionRegistry.hpp"
#include "application/StationRuntimeAdmissionBoundary.hpp"
#include "application/ProjectService.hpp"
#include "application/RuntimeVariables.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QLockFile>
#include <QTextStream>
#include <QElapsedTimer>
#include <condition_variable>
#include <chrono>
#include <future>
using namespace visionflow;
namespace {
using C=CommunicationV4Codec;int checks=0;
void check(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(...){rejected=true;}check(rejected,"Expected rejection");}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
struct Gate {
    std::mutex mutex;std::condition_variable cv;bool entered[4]{},released[4]{};int fail=-1;
    qint64 start[4]{},end[4]{};Qt::HANDLE threads[4]{};QElapsedTimer timer;
    Gate(){timer.start();}
    void reset(){std::lock_guard lock(mutex);for(int n=0;n<4;++n){entered[n]=released[n]=false;start[n]=end[n]=0;}fail=-1;}
    void release(int n){std::lock_guard lock(mutex);released[n]=true;cv.notify_all();}
    void wait(int count){std::unique_lock lock(mutex);check(cv.wait_for(lock,std::chrono::seconds(10),[&]{for(int n=0;n<count;++n)if(!entered[n])return false;return true;}),"All fixture camera workers enter concurrently");}
};
class Camera final:public AcquisitionAdapter {
    std::shared_ptr<Gate> gate_;int n_;QString path_;
public:
    Camera(std::shared_ptr<Gate> gate,int n,QString path):gate_(std::move(gate)),n_(n),path_(std::move(path)){}
    AcquisitionCapabilities capabilities()const override{return {"image_file",true};}
    ImageFrame acquire(const AcquisitionRequest& r)const override{
        {std::unique_lock lock(gate_->mutex);gate_->start[n_]=gate_->timer.nsecsElapsed();gate_->threads[n_]=QThread::currentThreadId();gate_->entered[n_]=true;gate_->cv.notify_all();
        if(!gate_->cv.wait_for(lock,std::chrono::seconds(12),[&]{return gate_->released[n_];}))throw std::runtime_error("Fixture barrier timeout");
        gate_->end[n_]=gate_->timer.nsecsElapsed();if(gate_->fail==n_)throw std::runtime_error("Fixture S1 acquisition failure");}
        // Deliberately ignore cancellation in the barrier; a late real frame is
        // returned and must be suppressed by the production cancellation fence.
        auto late=r;late.cancel.reset();return ImageFileAdapter(path_).acquire(late);
    }
};
struct Store:StationAdmissionStore {
    struct Entry{V4RequestIdentity key;StationAdmissionRecord record;};QVector<Entry> entries;
    std::optional<StationAdmissionRecord> find(const V4RequestIdentity& key)const override{for(const auto& e:entries)if(e.key==key)return e.record;return {};}
    void commit(const StationAdmissionIntent& i)override{check(!find(i.identity),"Intent unique before worker");entries.append({i.identity,{i.fingerprint,i.accepted}});}
    void terminal(const StationAdmissionIntent& i){
        for(auto& e:entries)if(e.key==i.identity){auto r=i.accepted;r.header.kind=111;r.body.replace(0,16,QUuid::createUuid().toRfc4122());r.body.replace(16,16,QUuid::createUuid().toRfc4122());C::putUInt32(r.body,64,1);C::putUInt32(r.body,72,1);C::putUInt32(r.body,100,3);
            auto bytes=C::encode(i.accepted,V4Direction::Response);C::putUInt32(bytes,8,111);bytes.replace(128,896,r.body);r.body.replace(32,32,C::terminalDigest(bytes));C::encode(r,V4Direction::Response);e.record.replay=r;return;}
        throw std::runtime_error("No durable intent");
    }
};
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir root(app.applicationDirPath()+"/station-execution-XXXXXX");check(root.isValid(),"Fixture directory");
    const auto installation=QUuid::createUuid(),peer=QUuid::createUuid(),epoch=QUuid::createUuid();
    JsonStationIdentityStore identities(root.filePath("identities.json"));StationIdentityRegistry registry(identities);auto cat=registry.initialize(installation);registry.enrollEpoch(cat.revision,peer,epoch);
    StationRuntimeBoundary boundary;Store store;auto gate=std::make_shared<Gate>();
    QVector<QUuid> ids;QVector<StationAdmissionSnapshot> manifests;QStringList paths,archives,sources,variables;
    std::vector<std::unique_ptr<RuntimeService>> runtimes;std::vector<std::unique_ptr<RuntimeCoordinator>> coordinators;std::vector<std::unique_ptr<StationRunController>> controllers;
    QVector<QVector<CycleResult>> results(4);int sinkFailures=0;bool failSink=false;
    StationExecutionRegistry executor(3);
    for(int n=0;n<4;++n){
        auto id=QUuid::createUuid();ids.append(id);cat=identities.load();auto entry=registry.bind(cat.revision,{CatalogueCategory::Station,id,{},{}},QString("S%1").arg(n+1));
        paths.append(root.filePath(QString("camera-%1.png").arg(n)));QImage image(32+n,24,QImage::Format_Grayscale8);image.fill(255);check(image.save(paths[n]),"Independent fixture raster");
        ProjectService project;project.createProject("Executor","",root.filePath(QString("s%1.vfproject").arg(n)),"file");project.createRecipe(QString("Recipe%1").arg(n),"",ApplicationType(n%3),"file");sources.append(project.setImageFile(paths[n]));auto blob=project.addTool("blob");auto decision=project.addTool("result");
        for(auto t:project.tools())if(t.id==blob){t.parameters["minArea"]=n==1?100000:1;project.updateTool(t);}else if(t.id==decision){t.enabled=true;t.parameters=encodeResultConfig({ResultOperator::All,{{blob,"found",4}}});project.updateTool(t);}
        auto recipe=*project.activeRecipe();VariableDefinition v{uuid(),QString("S%1Value").arg(n),VariableType::Int64};variables.append(v.id);ValueEnvelope value;value.type=ScalarType::Int64;value.validity=ValueValidity::Valid;value.value=qint64(100+n);
        const auto setter=uuid();auto tools=editorTools(recipe.definition);tools.append({setter,"set_variables","Value",{},true,encodeAssignmentConfig({setter,1,{{v.id,ConstantAssignmentSource{value}}}})});recipe.definition=withRecipeVariables(withEditorTools(recipe.definition,tools),{v});
        auto s=std::make_shared<StationSnapshot>();s->station={id,entry.key,entry.name};s->activation=QUuid::createUuid();s->selection.recipeKey=100+n;s->selection.revision=QUuid::createUuid();s->selection.recipe=recipe;s->selection.projectDirectory=root.path();manifests.append({s,200u+quint32(n),QUuid::createUuid(),{},{} });
        archives.append(root.filePath(QString("runtime-%1").arg(n)));runtimes.push_back(std::make_unique<RuntimeService>(archives[n],root.filePath("catalog.json"),RuntimeIdentity::localEngineer()));coordinators.push_back(std::make_unique<RuntimeCoordinator>(*runtimes[n]));boundary.bind(id,*runtimes[n],*coordinators[n]);controllers.push_back(std::make_unique<StationRunController>(boundary,id));controllers[n]->run(s);controllers[n]->poll();controllers[n]->setAuto(true);
        executor.bind(id,*coordinators[n],root.filePath("catalog.json"),[&,n](const auto& intent,const auto& cycle,const auto& result){
            check(QThread::currentThread()==app.thread(),"Completion on owner thread");check(store.find(intent.identity).has_value(),"Committed intent before worker/terminal");check(cycle.snapshot->recipe().id==manifests[n].station->selection.recipe.id && result.recipeId==cycle.snapshot->recipe().id,"Immutable station recipe provenance");
            if(failSink && n==2 && sinkFailures++==0)throw std::runtime_error("Fixture terminal sink unavailable");
            results[n].append(result);store.terminal(intent);
        },[gate,n,path=paths[n]](const ToolDefinition& t)->std::unique_ptr<AcquisitionAdapter>{if(t.kind!="image_file")return {};return std::make_unique<Camera>(gate,n,path);});
    }
    StationRuntimeAdmissionBoundary adapter(boundary,[&](const QUuid& id)->std::optional<StationAdmissionSnapshot>{const auto n=ids.indexOf(id);return n<0?std::nullopt:std::optional(manifests[n]);},[&](const auto& s){return executor.reserve(s);},[&](const auto& i,const auto& c,const auto& lease){executor.dispatch(i,c,lease);});for(auto& c:controllers)adapter.bind(*c);
    StationAdmissionService admission(store,adapter);CommunicationPeerSessionService sessions(identities);sessions.setTriggerHandler([&](const auto& key,const auto& f){return admission.trigger(key,f);});const auto connection=QUuid::createUuid();sessions.open(connection);
    CommunicationV4Frame hello;hello.header.kind=1;hello.header.requestId=1;hello.header.peer=peer;hello.header.epoch=epoch;hello.body=QByteArray(384,0);hello.body.replace(0,16,CommunicationPeerSessionService::contractRevision().toRfc4122());check(C::uint32(sessions.handle(connection,hello).body,76)==0,"Shared peer stream HELLO");
    auto request=[&](int n,quint64 ticket){CommunicationV4Frame f;const auto& s=manifests[n];f.header={10,s.station->station.key,ticket,sessions.session(),peer,epoch,s.station->selection.recipeKey,s.templateKey,s.station->activation};f.body=QByteArray(384,0);f.body.replace(0,16,s.cameraSetRevision.toRfc4122());return f;};
    auto send=[&](int n,quint64 ticket){return sessions.handle(connection,request(n,ticket));};
    auto until=[&](auto predicate){QElapsedTimer timer;timer.start();while(!predicate() && timer.elapsed()<10000){executor.poll();for(auto& c:controllers)c->poll();QThread::msleep(1);}check(predicate(),"Bounded station completion/drain");};
    auto release=[&](int n,quint64 ticket){check(admission.release({installation,peer,epoch,ids[n],ticket}),"Exact terminal releases only its own station slot");};
    rejects([&]{executor.bind(ids[0],*coordinators[0],"",{});});
    rejects([&]{executor.bind(QUuid::createUuid(),*coordinators[0],"",[](const auto&,const auto&,const auto&){});});
    auto wrongThread=std::async(std::launch::async,[&]{try{executor.busy(ids[0]);return false;}catch(...){return true;}});check(wrongThread.get(),"Registry owner thread enforced");
    {auto permit=executor.reserve(manifests[3]);check(!permit.reason,"Reversible pre-intent reservation");check(executor.reserve(manifests[3]).reason==2,"Same station reservation BUSY");}check(store.entries.isEmpty(),"Reservation alone never executes/writes");
    for(int n=0;n<3;++n)check(send(n,1).header.kind==110,"Same peer admits independent stations");gate->wait(3);
    check(C::uint32(send(0,2).body,76)==2,"Same station second cycle BUSY");check(C::uint32(send(3,1).body,76)==23 && !runtimes[3]->busy(),"Fourth station explicit RESOURCE_LIMIT before begin");check(send(0,1).header.kind==110,"Duplicate accepted does not recapture");
    {std::lock_guard lock(gate->mutex);for(int n=0;n<3;++n){check(!gate->end[n] && gate->start[n]>0,"Three acquisitions overlap at common barrier");check(gate->threads[n]!=QThread::currentThreadId(),"Acquisition runs off owner thread");for(int j=0;j<n;++j)check(gate->threads[n]!=gate->threads[j],"Independent worker threads");}}
    // S1 remains in its uncooperative camera while S2/S3 finish independently.
    controllers[0]->stop();check(controllers[0]->state().mode==StationMode::Draining,"Stop waits for its actual worker");rejects([&]{controllers[0]->run(manifests[0].station);});
    QLockFile disk(archives[1]+"/archive.lock");check(disk.tryLock(),"Fixture recording failure lock");failSink=true;gate->release(1);gate->release(2);
    until([&]{return coordinators[1]->recordingPending() && sinkFailures>0;});check(executor.busy(ids[0]) && executor.busy(ids[1]) && executor.busy(ids[2]),"One stopped, one storage pending, one sink pending stay isolated");
    check(!coordinators[1]->variables(),"Pending storage cannot publish variables");disk.unlock();until([&]{return !executor.busy(ids[1]) && !executor.busy(ids[2]);});
    check(results[1].size()==1 && results[2].size()==1 && results[0].isEmpty(),"S2/S3 complete while S1 drains; sink retry no duplicate terminal");
    check(results[1][0].decision()==ResultDecision::Nok && results[2][0].decision()==ResultDecision::Ok,"Distinct station decisions isolated");
    for(int n=1;n<3;++n){const auto& r=results[n][0];check(r.state==ExecutionState::Done && r.variables() && r.variables()->values.size()>0,"Real Runtime completion has variables");check(std::get<qint64>(*std::get<ValueEnvelope>(r.variables()->values[variables[n]]).value)==100+n,"Station-local assignment value");check(!r.variables()->values.contains(variables[0]),"Other station variable absent");check(r.frames.size()==1 && r.frames[0].sourceId==sources[n] && r.frames[0].cycleId==r.cycleId,"Station frame source/cycle provenance");release(n,1);}
    check(!admission.release({installation,peer,epoch,ids[0],1}),"S1 cannot release before actual drain/terminal");gate->release(0);until([&]{return !executor.busy(ids[0]);});
    check(controllers[0]->drained() && !coordinators[0]->variables(),"Stopped late worker never revives variable presentation");check(results[0][0].state==ExecutionState::Skipped && results[0][0].decision()==ResultDecision::NotEvaluated,"Late Stop completion is cancelled without fake decision");release(0,1);
    for(int n=0;n<3;++n)QTextStream(stdout)<<"timeline round=1 station=S"<<n+1<<" cameraEnterNs="<<gate->start[n]<<" cameraLeaveNs="<<gate->end[n]<<'\n';
    const auto old=results[0][0];gate->reset();gate->fail=0;failSink=false;controllers[0]->run(manifests[0].station);controllers[0]->poll();for(int n=0;n<3;++n)controllers[n]->setAuto(true);
    for(int n=0;n<3;++n)check(send(n,2).header.kind==110,"Second independent cycles admitted");gate->wait(3);gate->release(0);until([&]{return !executor.busy(ids[0]);});check(results[0][1].state==ExecutionState::Error && executor.busy(ids[1]) && executor.busy(ids[2]),"S1 acquisition error leaves S2/S3 workers running");
    rejects([&]{coordinators[0]->finish(1,old);});check(executor.busy(ids[1]) && executor.busy(ids[2]),"Foreign/late old callback rejected without affecting peers");
    gate->release(1);gate->release(2);until([&]{return !executor.busy(ids[1]) && !executor.busy(ids[2]);});check(results[1][1].state==ExecutionState::Done && results[2][1].state==ExecutionState::Done,"Peers complete after S1 error");
    for(int n=1;n<3;++n){check(results[n][0].cycleId!=results[n][1].cycleId && results[n][0].frameId!=results[n][1].frameId,"New cycles never reuse prior frame/context");release(n,2);}
    // Dynamic fourth station uses the same code path once worker capacity returns.
    check(send(3,1).header.kind==110,"Fourth station retries transient capacity without code special case");gate->release(3);until([&]{return !executor.busy(ids[3]);});check(results[3].size()==1 && results[3][0].state==ExecutionState::Done,"Fourth station actual execution");release(3,1);
    for(auto& c:controllers){c->stop();c->poll();}executor.poll();check(results[0].size()==2 && results[1].size()==2 && results[2].size()==2,"No late registry callback after retirement");
    QTextStream(stdout)<<"PASS "<<checks<<" station execution assertions; fixture cameras, no physical acceptance\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
