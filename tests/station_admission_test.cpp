#include "application/StationRuntimeAdmissionBoundary.hpp"
#include "application/ProjectService.hpp"
#include "application/TriggerRouter.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QTextStream>
#include <future>
#include <limits>
using namespace visionflow;
namespace {
using C=CommunicationV4Codec;int checks=0;
void check(bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);}
void putUuid(QByteArray& b,int o,const QUuid& id){b.replace(o,16,id.toRfc4122());}
quint32 reason(const CommunicationV4Frame& f){C::encode(f,V4Direction::Response);return C::uint32(f.body,76);}
QString detail(const CommunicationV4Frame& f){return QString::fromUtf8(f.body.sliced(620,C::uint32(f.body,616)));}
struct Store:StationAdmissionStore {
    struct Entry {V4RequestIdentity id;StationAdmissionRecord record;};
    QVector<Entry> records;bool failRead=false,failWrite=false,writeThenFail=false;int writes=0;
    std::function<void()> duringCommit;
    std::optional<StationAdmissionRecord> find(const V4RequestIdentity& key) const override {
        if(failRead)throw std::runtime_error("disk read");for(const auto& e:records)if(e.id==key)return e.record;return {};
    }
    void commit(const StationAdmissionIntent& intent) override {
        ++writes;if(duringCommit)duringCommit();if(failWrite)throw std::runtime_error("disk full");
        check(!find(intent.identity),"Intent never overwrites an accepted key");records.append({intent.identity,{intent.fingerprint,intent.accepted}});
        if(writeThenFail)throw std::runtime_error("ambiguous durable write");
    }
};
struct Boundary:StationAdmissionBoundary {
    QMap<QUuid,StationAdmissionSnapshot> snapshots;QMap<QUuid,StationAdmissionBarriers> states;
    int begins=0,reserves=0,leases=0;quint32 reserveReason=0;bool throwBegin=false,isDrained=true;
    std::function<void()> duringReserve;Store* store=nullptr;
    std::optional<StationAdmissionSnapshot> snapshot(const QUuid& id) const override {if(!snapshots.contains(id))return {};return snapshots[id];}
    StationAdmissionBarriers barriers(const QUuid& id) const override {return states[id];}
    StationAdmissionReservation reserve(const StationAdmissionSnapshot&) override {
        ++reserves;if(duringReserve)duringReserve();if(reserveReason)return {reserveReason,{}};
        ++leases;return {0,std::shared_ptr<void>(new int(1),[this](void* p){delete static_cast<int*>(p);--leases;})};
    }
    void begin(const StationAdmissionIntent& intent,const std::shared_ptr<void>& lease) override {
        check(bool(lease) && store && store->find(intent.identity).has_value(),"Durable intent and resource lease precede acquisition");
        ++begins;if(throwBegin)throw std::runtime_error("uncertain begin");
    }
    bool drained(const QUuid&) const override {return isDrained;}
};
CommunicationV4Frame trigger(const StationAdmissionSnapshot& s,const QUuid& peer,const QUuid& epoch,const QUuid& session,quint64 id){
    CommunicationV4Frame f;f.header={10,s.station->station.key,id,session,peer,epoch,s.station->selection.recipeKey,s.templateKey,s.station->activation};f.body=QByteArray(384,0);
    putUuid(f.body,0,s.cameraSetRevision);putUuid(f.body,16,s.inputSnapshot);putUuid(f.body,32,s.inputRevision);return f;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir root(app.applicationDirPath()+"/admission-XXXXXX");check(root.isValid(),"Fixture root");
    JsonStationIdentityStore identities(root.filePath("registry.json"));StationIdentityRegistry registry(identities);
    const auto installation=QUuid::createUuid(),peer=QUuid::createUuid(),epoch=QUuid::createUuid();auto cat=registry.initialize(installation);registry.enrollEpoch(cat.revision,peer,epoch);
    Store store;Boundary boundary;boundary.store=&store;StationAdmissionService admission(store,boundary);
    QVector<QUuid> ids;
    for(int n=0;n<3;++n){
        const auto id=QUuid::createUuid();ids.append(id);cat=identities.load();const auto entry=registry.bind(cat.revision,{CatalogueCategory::Station,id,{},{ }},QString("S%1").arg(n+1));
        auto s=std::make_shared<StationSnapshot>();s->station={id,entry.key,entry.name};s->activation=QUuid::createUuid();s->selection.recipeKey=100+n;s->selection.revision=QUuid::createUuid();
        boundary.snapshots[id]={s,quint32(200+n),QUuid::createUuid(),{},{} };boundary.states[id]={StationMode::ArmedAuto,true,false,false,0};
    }
    CommunicationPeerSessionService sessions(identities);sessions.setTriggerHandler([&](const auto& key,const auto& f){return admission.trigger(key,f);});
    const auto connection=QUuid::createUuid();sessions.open(connection);
    CommunicationV4Frame hello;hello.header.kind=1;hello.header.requestId=1;hello.header.peer=peer;hello.header.epoch=epoch;hello.body=QByteArray(384,0);putUuid(hello.body,0,CommunicationPeerSessionService::contractRevision());
    check(reason(sessions.handle(connection,hello))==0,"HELLO prerequisite");
    auto request=[&](int n,quint64 id){return trigger(boundary.snapshots[ids[n]],peer,epoch,sessions.session(),id);};
    auto send=[&](const CommunicationV4Frame& f){return sessions.handle(connection,f);};
    auto stale=request(0,9007199254740993ULL);stale.header.activation=QUuid::createUuid();
    check(reason(send(stale))==4 && boundary.begins==0,"S1 stale activation rejects without capture");
    auto s2=request(1,9007199254740993ULL);check(send(s2).header.kind==110 && boundary.begins==1,"Independent S2 accepted while S1 stale");
    check(reason(send(request(1,s2.header.requestId+1)))==2 && boundary.begins==1,"Same station admits only one trigger");
    auto s1=request(0,1);check(send(s1).header.kind==110 && boundary.begins==2,"Distinct stations own independent active slots");
    auto duplicate=send(s1);check(duplicate.header.kind==110 && boundary.begins==2 && store.writes==2,"Exact duplicate replays durable accepted");
    auto conflict=s1;putUuid(conflict.body,0,QUuid::createUuid());check(reason(send(conflict))==6,"Accepted key changed content conflicts");
    boundary.states[ids[0]].mode=StationMode::Stopped;
    check(send(s1).header.kind==110 && boundary.begins==2,"Duplicate remains replayable after Stop");
    auto& state=boundary.states[ids[2]];auto s3=request(2,std::numeric_limits<quint64>::max());
    state.mode=StationMode::ArmedManual;check(reason(send(s3))==1,"NotAuto distinct");
    state.mode=StationMode::Preparing;check(reason(send(s3))==20,"NotReady distinct");
    state.mode=StationMode::ArmedAuto;state.recordingPending=true;auto pending=send(s3);
    check(reason(pending)==8 && detail(pending)=="PENDING_STORAGE" && boundary.begins==2,"PendingStorage prevents acquisition");state.recordingPending=false;
    state.deliveryPending=true;check(reason(send(s3))==2 && detail(send(s3))=="PENDING_DELIVERY","Pending ACK barrier");state.deliveryPending=false;
    state.resourceReason=17;check(reason(send(s3))==17,"Native physical gate preserved");state.resourceReason=0;
    for(int field=0;field<5;++field){auto f=s3;if(field==0)++f.header.recipeRevisionKey;if(field==1)++f.header.templateRevisionKey;if(field==2)putUuid(f.body,0,QUuid::createUuid());if(field==3)putUuid(f.body,16,QUuid::createUuid());if(field==4)putUuid(f.body,32,QUuid::createUuid());check(reason(send(f))==4,"Every exact recipe/template/camera/input revision fenced");}
    boundary.reserveReason=3;check(reason(send(s3))==3,"Camera busy is transient");boundary.reserveReason=23;check(reason(send(s3))==23,"Shared budget explicit resource limit");boundary.reserveReason=0;
    boundary.duringReserve=[&]{state.mode=StationMode::Stopped;};check(reason(send(s3))==1 && boundary.leases==2,"Local Stop during reserve rejects and rolls back lease");state.mode=StationMode::ArmedAuto;
    boundary.duringReserve=[&]{boundary.snapshots[ids[2]].cameraSetRevision=QUuid::createUuid();};check(reason(send(s3))==4 && boundary.leases==2,"Revision race after reserve rejects all-or-none");
    boundary.duringReserve={};s3=request(2,s3.header.requestId);
    quint32 reentrantReason=0;store.duringCommit=[&]{reentrantReason=reason(send(request(2,42)));};
    check(send(s3).header.kind==110 && reentrantReason==2 && boundary.begins==3,"Same key retried after transient barriers; reentrant competitor loses");store.duringCommit={};
    const auto s3key=sessions.requestIdentity(connection,s3);
    check(boundary.leases==3 && !admission.release(s3key),"Accepted intent cannot release before durable terminal");
    // A new transport session retains the durable namespace/fingerprint.
    sessions.close(connection);const auto reconnect=QUuid::createUuid();sessions.open(reconnect);sessions.handle(reconnect,hello);s3.header.session=sessions.session();
    check(sessions.handle(reconnect,s3).header.kind==110 && boundary.begins==3,"Reconnect duplicate never dispatches again");
    auto wrong=s3;wrong.header.session=QUuid::createUuid();check(reason(sessions.handle(reconnect,wrong))==5 && boundary.begins==3,"Session gate precedes admission");
    auto unknown=s3;unknown.header.stationKey=999999;check(reason(sessions.handle(reconnect,unknown))==10,"Unknown station key never reaches resource port");
    check(std::async(std::launch::async,[&]{try{admission.trigger({},s3);return false;}catch(...){return true;}}).get(),"Cross-thread admission rejected");
    for(auto& e:store.records)if(e.id==s3key){
        auto bytes=C::encode(e.record.replay,V4Direction::Response);C::putUInt32(bytes,8,111);putUuid(bytes,128,QUuid::createUuid());putUuid(bytes,144,QUuid::createUuid());C::putUInt32(bytes,192,3);C::putUInt32(bytes,228,3);bytes.replace(160,32,C::terminalDigest(bytes));e.record.replay=C::decode(bytes,V4Direction::Response);
    }
    boundary.isDrained=false;check(!admission.release(s3key) && boundary.leases==3,"Durable terminal still requires actual drain");boundary.isDrained=true;
    auto foreign=s3key;++foreign.request;check(!admission.release(foreign),"Foreign completion cannot clear station ownership");
    const auto terminal=sessions.handle(reconnect,s3);check(terminal.header.kind==111 && boundary.begins==3,"Exact sealed terminal replay without acquisition");
    check(admission.release(s3key) && boundary.leases==2,"Exact terminal and drain release resources");
    auto next=request(2,4242);state.mode=StationMode::ArmedManual;
    check(admission.trigger({installation,peer,epoch,ids[2],4242},next,StationTriggerOrigin::Manual).header.kind==110 && boundary.begins==4,"Manual cycle uses same atomic durable admission");
    {
        Store inputStore;Boundary b;b.store=&inputStore;b.snapshots=boundary.snapshots;b.states=boundary.states;b.states[ids[2]]={StationMode::ArmedAuto,true,false,false,0};StationAdmissionService a(inputStore,b);
        auto& snapshot=b.snapshots[ids[2]];snapshot.inputSnapshot=QUuid::createUuid();snapshot.inputRevision=QUuid::createUuid();
        auto f=trigger(snapshot,peer,epoch,sessions.session(),7);const V4RequestIdentity key{installation,peer,epoch,ids[2],7};
        check(reason(a.trigger(key,f))==20 && b.begins==0,"Input UUIDs without frozen values cannot grant readiness");
        snapshot.inputValues=std::make_shared<const InputSnapshot>();
        check(a.trigger(key,f).header.kind==110 && b.begins==1,"Exact nonzero input revisions retain immutable values");
    }
    // Isolated failures: fake store models durable port outcomes, not a v4 ledger.
    for(int scenario=0;scenario<4;++scenario){
        Store failure;Boundary b;b.store=&failure;b.snapshots=boundary.snapshots;b.states=boundary.states;b.states[ids[2]]={StationMode::ArmedAuto,true,false,false,0};StationAdmissionService a(failure,b);
        auto f=request(2,123);const V4RequestIdentity key{installation,peer,epoch,ids[2],123};
        failure.failRead=scenario==0;failure.failWrite=scenario==1;failure.writeThenFail=scenario==2;b.throwBegin=scenario==3;
        check(reason(a.trigger(key,f))==(scenario==3?9:8),"Read/write/ambiguous/begin failure reason");
        check(b.begins==(scenario==3?1:0),"Recording errors have zero acquisition calls");
        failure.failRead=false;failure.failWrite=false;failure.writeThenFail=false;b.throwBegin=false;
        const auto retry=a.trigger(key,f);
        if(scenario==0)check(retry.header.kind==110 && b.begins==1,"Read failure before intent remains retryable");
        else if(scenario==1)check(reason(retry)==8 && b.begins==0,"Ambiguous write fences station with PendingStorage until recovery");
        else check(retry.header.kind==110 && b.begins==(scenario==3?1:0),"Persisted intent replays; never automatically re-runs uncertain capture");
    }
    // Concrete existing Runtime/Coordinator adapter, independent of worker topology.
    ProjectService project;project.createProject("Admission fixture","",root.filePath("admission.vfproject"),"file");project.createRecipe("Recipe","",ApplicationType(0),"file");QImage image(24,24,QImage::Format_Grayscale8);image.fill(120);const auto imagePath=root.filePath("input.png");check(image.save(imagePath),"Runtime fixture image");project.setImageFile(imagePath);project.save();
    RuntimeService runtime(root.filePath("runtime"),root.filePath("catalog.json"),RuntimeIdentity::localEngineer());RuntimeCoordinator coordinator(runtime);StationRuntimeBoundary runtimeBoundary;
    auto real=std::make_shared<StationSnapshot>(*boundary.snapshots[ids[0]].station);real->selection.recipe=*project.activeRecipe();real->selection.projectDirectory=root.path();runtimeBoundary.bind(ids[0],runtime,coordinator);StationRunController controller(runtimeBoundary,ids[0]);
    auto manifest=boundary.snapshots[ids[0]];manifest.station=real;Store realStore;int dispatches=0;std::optional<ProductionCycle> cycle;
    StationRuntimeAdmissionBoundary adapter(runtimeBoundary,[&](const QUuid& id)->std::optional<StationAdmissionSnapshot>{return id==ids[0]?std::optional(manifest):std::nullopt;},
        [](const auto&){return StationAdmissionReservation{0,std::make_shared<int>(1)};},
        [&](const auto& intent,const auto& c,const auto&){check(realStore.find(intent.identity).has_value(),"Concrete Runtime begins only after intent commit");cycle=c;++dispatches;});adapter.bind(controller);
    StationAdmissionService actual(realStore,adapter);const V4RequestIdentity realKey{installation,peer,epoch,ids[0],77};auto realRequest=trigger(manifest,peer,epoch,sessions.session(),77);
    check(reason(actual.trigger(realKey,realRequest))==1 && !runtime.busy(),"Stopped real Runtime reports NotAuto before preparation");
    controller.run(real);controller.poll();check(controller.state().mode==StationMode::ArmedManual,"Real fixture preparation armed");
    check(reason(actual.trigger(realKey,realRequest))==1 && !runtime.busy(),"Real Run does not imply Auto");controller.setAuto(true);
    const auto activeActivation=real->activation;real->activation=QUuid::createUuid();realRequest.header.activation=real->activation;
    check(reason(actual.trigger(realKey,realRequest))==20 && !runtime.busy(),"Manifest cannot grant readiness to a different Runtime activation");real->activation=activeActivation;realRequest.header.activation=activeActivation;
    check(actual.trigger(realKey,realRequest).header.kind==110 && runtime.busy() && dispatches==1,"Station-aware adapter uses existing Coordinator begin");
    check(actual.trigger(realKey,realRequest).header.kind==110 && dispatches==1,"Concrete Runtime duplicate no second begin");
    controller.stop();auto result=RuntimeService::execute(*cycle,root.filePath("catalog.json"),coordinator.cancellation());coordinator.finish(cycle->ticket,result);controller.poll();check(controller.drained(),"Existing Runtime stop/drain retained");
    QTextStream(stdout)<<"PASS "<<checks<<" station admission assertions\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
