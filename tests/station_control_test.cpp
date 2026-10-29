#include "application/StationControlService.hpp"
#include "application/StationRuntimeAdmissionBoundary.hpp"
#include "application/ProjectService.hpp"
#include "persistence/StationActivationStore.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <future>
#include <vector>
using namespace visionflow;
namespace {
using C=CommunicationV4Codec;int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
QUuid id(){return QUuid::createUuid();}
quint32 reason(const CommunicationV4Frame& f){C::encode(f,V4Direction::Response);return C::uint32(f.body,76);}
CommunicationV4Frame ack(const CommunicationV4Frame& t){auto r=t;r.header.kind=11;r.body=QByteArray(384,0);r.body.replace(0,16,t.body.first(16));r.body.replace(16,32,t.body.sliced(32,32));return r;}
// Fake durable ledger shared by control AND production. No file persistence is
// claimed here: COMM4-13 must implement the same atomic slots, seal and ACK port.
struct Ledger:StationControlStore {
    struct Entry {V4RequestIdentity key;StationControlRecord record;bool control=false;};
    struct Production {V4RequestIdentity key;StationAdmissionRecord record;CommunicationV4Frame request;bool acknowledged=false;};
    QVector<Entry> entries;QVector<Production> production;int reserves=0,seals=0,ackWrites=0;
    bool failRead=false,failReserve=false,failSeal=false,failAck=false,ambiguous=false;
    std::optional<StationControlRecord> find(const V4RequestIdentity& key) const override {if(failRead)throw std::runtime_error("read");for(const auto& e:entries)if(e.key==key)return e.record;for(const auto& e:production)if(e.key==key)return StationControlRecord{e.request,e.record.replay,e.record.fingerprint,e.acknowledged};return {};}
    quint32 reserve(const StationControlIntent& intent) override {
        ++reserves;if(failReserve)throw std::runtime_error("full");
        for(const auto& e:entries)if(e.key==intent.identity)throw std::runtime_error("overwrite");
        for(const auto& e:entries)if(e.key.station==intent.identity.station && !e.record.acknowledged && e.control==intent.controlReserve)return 23;
        if(!intent.controlReserve)for(const auto& e:production)if(e.key.station==intent.identity.station && !e.acknowledged)return 2;
        entries.append({intent.identity,intent.record,intent.controlReserve});if(ambiguous)throw std::runtime_error("ambiguous");return 0;
    }
    void seal(const V4RequestIdentity& key,const CommunicationV4Frame& terminal) override {if(failSeal)throw std::runtime_error("seal");for(auto& e:entries)if(e.key==key){check(e.record.replay.header.kind==110,"Seal immutable once");e.record.replay=terminal;++seals;return;}throw std::runtime_error("missing intent");}
    void acknowledge(const V4RequestIdentity& key) override {if(failAck)throw std::runtime_error("ACK disk");for(auto& e:entries)if(e.key==key){if(!e.record.acknowledged)++ackWrites;e.record.acknowledged=true;return;}throw std::runtime_error("missing ACK");}
    bool productionPending(const QUuid& station) const {for(const auto& e:production)if(e.key.station==station && !e.acknowledged)return true;return false;}
    bool normalPending(const QUuid& station) const {if(productionPending(station))return true;for(const auto& e:entries)if(e.key.station==station && !e.control && !e.record.acknowledged)return true;return false;}
    bool ackProduction(const V4RequestIdentity& key,const CommunicationV4Frame& a){for(auto& e:production)if(e.key==key && e.record.replay.header.kind==111){const auto t=ack(e.record.replay);if(C::encode(t,V4Direction::Request)!=C::encode(a,V4Direction::Request))return false;e.acknowledged=true;return true;}return false;}
    struct Admission:StationAdmissionStore {
        Ledger& ledger;explicit Admission(Ledger& l):ledger(l){}
        std::optional<StationAdmissionRecord> find(const V4RequestIdentity& key) const override {for(const auto& e:ledger.production)if(e.key==key)return e.record;for(const auto& e:ledger.entries)if(e.key==key)return StationAdmissionRecord{e.record.fingerprint,e.record.replay};return {};}
        void commit(const StationAdmissionIntent& intent) override {
            check(!ledger.productionPending(intent.identity.station),"Production normal slot reserved once");
            for(const auto& e:ledger.entries)check(e.key.station!=intent.identity.station || e.control || e.record.acknowledged,"Control normal terminal bars production");
            auto request=intent.accepted;request.header.kind=10;request.body=QByteArray(384,0);request.body.replace(0,16,intent.snapshot.cameraSetRevision.toRfc4122());request.body.replace(16,16,intent.snapshot.inputSnapshot.toRfc4122());request.body.replace(32,16,intent.snapshot.inputRevision.toRfc4122());
            ledger.production.append({intent.identity,{intent.fingerprint,intent.accepted},request,false});
        }
    } admission{*this};
};
CommunicationV4Frame productionTerminal(const CommunicationV4Frame& accepted){
    auto raw=C::encode(accepted,V4Direction::Response);auto r=accepted;r.header.kind=111;r.body.replace(0,16,id().toRfc4122());r.body.replace(16,16,id().toRfc4122());C::putUInt32(r.body,64,1);C::putUInt32(r.body,72,1);C::putUInt32(r.body,100,3);C::putUInt32(raw,8,111);raw.replace(128,896,r.body);r.body.replace(32,32,C::terminalDigest(raw));C::encode(r,V4Direction::Response);return r;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir root(app.applicationDirPath()+"/control-XXXXXX");check(root.isValid(),"Fixture root");
    JsonStationIdentityStore identities(root.filePath("identities.json"));StationIdentityRegistry registry(identities);registry.initialize(id());
    const auto peer=id(),epoch=id(),connection=id();registry.enrollEpoch(identities.load().revision,peer,epoch);
    auto enroll=[&](CatalogueCategory category,const QUuid& uuid,const QUuid& revision=QUuid{}){return registry.bind(identities.load().revision,{category,uuid,revision,{}},"Saved fixture");};
    QImage image(32,24,QImage::Format_Grayscale8);image.fill(100);const auto imagePath=root.filePath("image.png");check(image.save(imagePath),"Image fixture");
    QMap<quint32,StationSavedRecipe> saved;QVector<quint32> recipes;
    for(int n=0;n<2;++n){ProjectService project;project.createProject("Saved","",root.filePath(QString("%1.vfproject").arg(n)),"file");project.createRecipe(n?"B":"A","",ApplicationType(0),"file");project.setImageFile(imagePath);project.save();
        auto recipe=*project.activeRecipe();auto revision=id(),camera=id();const auto re=enroll(CatalogueCategory::RecipeRevision,QUuid(recipe.id),revision),cam=enroll(CatalogueCategory::Camera,camera);
        saved[re.key]={{revision,re.key,recipe,root.path(),{{editorTools(recipe.definition).first().id,camera,cam.key}}},77};recipes.append(re.key);
    }
    StationRuntimeBoundary boundary;FileStationActivationStore activationStore(root.filePath("activation.bin"));StationActivationService activation(activationStore,identities,boundary);activation.initialize();Ledger ledger;
    auto resolver=[&](quint32 key)->std::optional<StationSavedRecipe>{if(saved.contains(key))return saved[key];return {};};
    StationControlService control(activation,identities,&ledger,resolver);
    std::vector<std::unique_ptr<RuntimeService>> runtimes;std::vector<std::unique_ptr<RuntimeCoordinator>> coordinators;std::vector<std::unique_ptr<StationRunController>> runs;QVector<StationDefinition> stations;
    for(int n=0;n<3;++n){const auto uuid=id();const auto entry=enroll(CatalogueCategory::Station,uuid);stations.append({uuid,entry.key,QString("S%1").arg(n+1)});
        runtimes.push_back(std::make_unique<RuntimeService>(root.filePath(QString("runtime%1").arg(n)),root.filePath("catalog.json"),RuntimeIdentity::localEngineer()));coordinators.push_back(std::make_unique<RuntimeCoordinator>(*runtimes.back()));boundary.bind(uuid,*runtimes.back(),*coordinators.back());runs.push_back(std::make_unique<StationRunController>(boundary,uuid));control.bind(stations.last(),*runs.back());
    }
    CommunicationPeerSessionService sessions(identities);sessions.open(connection);sessions.setControlHandler([&](const auto& key,const auto& f){return control.handle(key,f);});
    CommunicationV4Frame hello;hello.header.kind=1;hello.header.requestId=1;hello.header.peer=peer;hello.header.epoch=epoch;hello.body=QByteArray(384,0);hello.body.replace(0,16,CommunicationPeerSessionService::contractRevision().toRfc4122());check(reason(sessions.handle(connection,hello))==0,"Exact peer session");const auto session=sessions.session();
    quint64 sequence=10;
    auto request=[&](int n,quint32 kind){CommunicationV4Frame f;f.header={kind,stations[n].key,++sequence,session,peer,epoch,0,0,{}};f.body=QByteArray(384,0);if(const auto s=activation.snapshot(stations[n].id)){f.header.activation=s->activation;f.header.recipeRevisionKey=s->selection.recipeKey;f.header.templateRevisionKey=77;}return f;};
    auto send=[&](const CommunicationV4Frame& f){return sessions.handle(connection,f);};
    auto key=[&](int n,const CommunicationV4Frame& f){return V4RequestIdentity{identities.load().installation,peer,epoch,stations[n].id,f.header.requestId};};
    auto terminal=[&](const CommunicationV4Frame& f){control.poll();auto t=send(f);check(t.header.kind==111,"Terminal after actual poll");return t;};
    auto select=[&](int n,quint32 recipe){auto f=request(n,4);C::putUInt32(f.body,0,recipe);C::putUInt32(f.body,4,77);return f;};
    auto a=select(0,recipes[0]);check(reason(send(a))==25 && activation.snapshots().isEmpty(),"Remote disabled cannot select");control.setRemoteEnabled(true);
    auto wrong=a;wrong.header.peer=id();check(reason(send(wrong))==5,"Wrong peer session rejects");
    auto unknown=a;C::putUInt32(unknown.body,0,9999);check(reason(send(unknown))==4,"Unknown saved revision/draft rejects");
    check(send(a).header.kind==110,"A selection accepted");const auto firstActivation=activation.snapshot(stations[0].id)->activation;
    check(send(a).header.kind==110 && activationStore.load().history.size()==1,"Retry does not select twice");auto aTerminal=terminal(a);
    check(aTerminal.header.activation==firstActivation && aTerminal.header.activation!=a.header.activation && aTerminal.header.recipeRevisionKey==recipes[0],"SELECT terminal carries new activation");
    check(!runs[0]->automatic() && runs[0]->state().mode==StationMode::Stopped,"Selection remains Auto off");
    auto badAck=ack(aTerminal);badAck.header.activation=a.header.activation;check(reason(send(badAck))==14,"SELECT ACK old request header fails");
    badAck=ack(aTerminal);badAck.body[16]^=1;check(reason(send(badAck))==14,"Wrong digest ACK fails");check(send(ack(aTerminal)).header.kind==112 && send(ack(aTerminal)).header.kind==112 && ledger.ackWrites==1,"Exact new header ACK idempotent");
    auto automatic=request(0,5);C::putUInt32(automatic.body,0,1);check(send(automatic).header.kind==110 && !runs[0]->automatic(),"Auto accepted is not READY");const auto generation=runs[0]->controlGeneration();check(send(automatic).header.kind==110 && runs[0]->controlGeneration()==generation,"Auto retry never reloads/prepares");
    auto ready=terminal(automatic);check(!reason(ready) && C::uint32(ready.body,64)==1 && C::uint32(ready.body,72)==1 && C::uint32(ready.body,756)==1 && runs[0]->automatic(),"READY only after actual preparation");
    const auto cameraRevision=id();std::optional<ProductionCycle> cycle;int begins=0;
    StationRuntimeAdmissionBoundary admissionBoundary(boundary,[&](const QUuid& uuid)->std::optional<StationAdmissionSnapshot>{auto s=activation.snapshot(uuid);if(!s)return {};return StationAdmissionSnapshot{s,77,cameraRevision,{},{},{}};},
        [&](const auto& s){return ledger.normalPending(s.station->station.id)?StationAdmissionReservation{2,{}}:StationAdmissionReservation{0,std::make_shared<int>(1)};},[&](const auto&,const auto& c,const auto&){cycle=c;++begins;});for(auto& run:runs)admissionBoundary.bind(*run);
    StationAdmissionService admission(ledger.admission,admissionBoundary);sessions.setTriggerHandler([&](const auto& k,const auto& f){return admission.trigger(k,f);});
    auto trigger=request(0,10);trigger.body.replace(0,16,cameraRevision.toRfc4122());check(reason(send(trigger))==2 && begins==0,"Unacked control terminal shares normal production slot");send(ack(ready));auto accepted=send(trigger);check(accepted.header.kind==110 && begins==1 && runtimes[0]->busy(),"A actual Runtime trigger");
    auto stop=request(0,6);check(send(stop).header.kind==110 && !runs[0]->automatic() && runs[0]->state().mode==StationMode::Draining,"STOP during production closes admission immediately");control.poll();check(send(stop).header.kind==110,"STOP waits real drain");
    auto completed=RuntimeService::execute(*cycle,root.filePath("catalog.json"),coordinators[0]->cancellation());coordinators[0]->finish(cycle->ticket,completed);coordinators[0]->setDeliveryPending("production",true);
    auto production=productionTerminal(accepted);ledger.production.last().record.replay=production;
    const auto sealedProduction=C::encode(production,V4Direction::Response);auto stopped=terminal(stop);check(ledger.productionPending(stations[0].id) && C::encode(ledger.production.last().record.replay,V4Direction::Response)==sealedProduction,"STOP preserves pending production bytes");
    auto reused=request(0,4);reused.header.requestId=trigger.header.requestId;C::putUInt32(reused.body,0,recipes[1]);C::putUInt32(reused.body,4,77);check(reason(send(reused))==6,"Production/control request namespace cannot be reused");
    check(reason(send(ack(production)))==20,"Production ACK delegated to its ledger; no control mutation");
    check(send(ack(stopped)).header.kind==112 && ledger.productionPending(stations[0].id) && coordinators[0]->deliveryPending(),"Control ACK never clears production");
    // Exhausted control slot still executes the second STOP action.
    auto stop2=request(0,6);check(send(stop2).header.kind==110,"Extra control reserve while production pending");auto stop2Terminal=terminal(stop2);const auto beforeStop=runs[0]->controlGeneration();auto stop3=request(0,6);check(reason(send(stop3))==23 && runs[0]->controlGeneration()>beforeStop,"Full control reserve still closes admission; no fake accepted");
    for(int n=1;n<3;++n){auto s=select(n,recipes[0]);check(send(s).header.kind==110,"S2/S3 selection independent of S1 pending");auto t=terminal(s);send(ack(t));auto enable=request(n,5);C::putUInt32(enable.body,0,1);check(send(enable).header.kind==110,"S2/S3 prepare independently");auto r=terminal(enable);check(!reason(r) && runs[n]->automatic(),"S2/S3 continue while S1 pending");send(ack(r));}
    check(!ledger.ackProduction(key(0,trigger),ack(stopped)),"Control terminal cannot ACK production");check(ledger.ackProduction(key(0,trigger),ack(production)),"A terminal exact ACK");coordinators[0]->setDeliveryPending("production",false);check(admission.release(key(0,trigger)),"Durable terminal and drain release admission");send(ack(stop2Terminal));
    auto b=select(0,recipes[1]);check(send(b).header.kind==110,"B select after A ACK");auto bTerminal=terminal(b);check(bTerminal.header.activation!=firstActivation && sessions.session()==session,"B activation preserves socket session");send(ack(bTerminal));
    const auto back=select(0,recipes[0]);check(send(back).header.kind==110,"Return A");auto backTerminal=terminal(back);check(backTerminal.header.activation!=firstActivation && backTerminal.header.activation!=bTerminal.header.activation,"A B A never revives activation");send(ack(backTerminal));
    check(send(a).header.activation==firstActivation && activation.snapshot(stations[0].id)->activation==backTerminal.header.activation,"Old SELECT retry replays sealed terminal without reselect");
    auto conflict=a;C::putUInt32(conflict.body,0,recipes[1]);check(reason(send(conflict))==6,"Old fingerprint retains request content");
    auto stale=request(0,5);stale.header.activation=firstActivation;check(reason(send(stale))==4,"Stale activation rejects");
    auto race=request(0,5);C::putUInt32(race.body,0,1);check(send(race).header.kind==110,"Preparation before local Stop race");control.stop(stations[0].id);auto canceled=terminal(race);check(reason(canceled)==20 && C::uint32(canceled.body,64)==3 && !runs[0]->automatic(),"Late callback cannot reopen Auto after local Stop");send(ack(canceled));
    auto disableRace=request(0,5);C::putUInt32(disableRace.body,0,1);send(disableRace);control.setRemoteEnabled(false);auto disabled=terminal(disableRace);check(reason(disabled)==20 && !runs[0]->automatic(),"Remote disable fences preparation");send(ack(disabled));control.setRemoteEnabled(true);
    check(QFile::remove(imagePath),"Remove source for missing preparation");auto missing=request(0,5);C::putUInt32(missing.body,0,1);check(send(missing).header.kind==110,"Accepted immutable command records preparation failure");auto failed=send(missing);check(failed.header.kind==111 && reason(failed)==20 && !C::uint32(failed.body,756) && !runs[0]->automatic(),"Missing source cannot READY");send(ack(failed));check(image.save(imagePath),"Restore source");control.stop(stations[0].id);
    StationControlService noStore(activation,identities,nullptr,resolver);noStore.bind(stations[0],*runs[0]);bool rejected=false;try{noStore.setAuto(stations[0].id,activation.snapshot(stations[0].id)->activation,true);}catch(...){rejected=true;}check(rejected && !runs[0]->automatic(),"No durable port never opens Auto");
    // Durable seal failure must close Auto and preserve accepted recovery state.
    auto sealFailure=request(0,5);C::putUInt32(sealFailure.body,0,1);send(sealFailure);ledger.failSeal=true;control.poll();check(!runs[0]->automatic() && send(sealFailure).header.kind==110,"Seal failure fences Auto without fabricated READY");ledger.failSeal=false;
    auto fenced=request(0,5);C::putUInt32(fenced.body,0,1);check(reason(send(fenced))==8,"Unresolved write failure fences new work");
    check(std::async(std::launch::async,[&]{try{control.poll();return false;}catch(...){return true;}}).get(),"Owner thread enforced");
    for(int scenario=0;scenario<3;++scenario){
        Ledger failedLedger;StationControlService failedControl(activation,identities,&failedLedger,resolver);failedControl.bind(stations[0],*runs[0]);failedControl.setRemoteEnabled(true);
        auto f=request(0,5);C::putUInt32(f.body,0,1);auto k=key(0,f);failedLedger.failRead=scenario==0;failedLedger.failReserve=scenario==1;failedLedger.ambiguous=scenario==2;const auto before=runs[0]->controlGeneration();
        auto result=failedControl.handle(k,f);check(result && reason(*result)==8 && !runs[0]->automatic(),"Read/reserve/ambiguous failure never opens Auto");
        failedLedger.failRead=false;failedLedger.failReserve=false;failedLedger.ambiguous=false;result=failedControl.handle(k,f);
        check(result && (scenario==2?result->header.kind==110:reason(*result)==8) && !runs[0]->automatic() && runs[0]->controlGeneration()>=before,"Ambiguous accepted intent replays without preparation; failure stays fenced");
    }
    {
        Ledger localLedger;StationControlService local(activation,identities,&localLedger,resolver);local.bind(stations[0],*runs[0]);
        const auto selected=local.select(stations[0].id,activation.snapshot(stations[0].id)->activation,recipes[1],77);check(!runs[0]->automatic(),"Local select uses same CAS and Auto off");
        local.setAuto(stations[0].id,selected->activation,true);check(!runs[0]->automatic(),"Local Auto waits actual preparation poll");local.poll();check(runs[0]->automatic(),"Local Auto uses shared preparation application port");local.setAuto(stations[0].id,selected->activation,false);check(!runs[0]->automatic() && runs[0]->state().mode==StationMode::ArmedManual,"Local Auto off closes admission without drain");
        local.setAuto(stations[0].id,selected->activation,true);local.stop(stations[0].id);local.poll();check(!runs[0]->automatic(),"Local async Auto promotion fenced by Stop");
        local.setRemoteEnabled(true);auto f=request(0,6);check(local.handle(key(0,f),f)->header.kind==110,"Local ledger STOP accepted");local.poll();const auto t=local.handle(key(0,f),f).value();localLedger.failAck=true;check(reason(local.handle(key(0,f),ack(t)).value())==8 && !localLedger.entries.last().record.acknowledged,"ACK disk error preserves unresolved terminal");localLedger.failAck=false;check(local.handle(key(0,f),ack(t))->header.kind==112,"Late exact ACK allowed through storage fence");
    }
    // New service/reconnect replays durable records; neither invokes Run nor Auto.
    StationControlService restored(activation,identities,&ledger,resolver);restored.bind(stations[0],*runs[0]);const auto fence=runs[0]->controlGeneration();auto replayKey=key(0,a);auto reconnect=a;reconnect.header.session=id();auto replay=restored.handle(replayKey,reconnect);check(replay && replay->header.activation==firstActivation && replay->header.session==reconnect.header.session && runs[0]->controlGeneration()==fence,"Reconnect SELECT replay changes session envelope only");
    for(auto& run:runs)run->stop();
    QTextStream(stdout)<<"PASS "<<checks<<" station control assertions\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL "<<e.what()<<'\n';return 1;}}
