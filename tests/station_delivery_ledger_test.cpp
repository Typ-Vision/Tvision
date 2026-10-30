#include "application/StationDeliveryLedger.hpp"
#include "application/ProductionStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QTextStream>
#include <limits>
using namespace visionflow;
namespace {
using C=CommunicationV4Codec;int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
template<class F> void fails(F f,const char* why){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw,why);}
QUuid id(){return QUuid::createUuid();}
struct Fixture {
    QTemporaryDir root{QCoreApplication::applicationDirPath()+"/v4-ledger-XXXXXX"};
    V4RequestIdentity key{id(),id(),id(),id(),9007199254740993ULL};
    CommunicationV4Frame request{{10,7,key.request,id(),key.peer,key.epoch,31,41,id()},QByteArray(384,0)};
    StationControlIntent intent() const {
        auto a=request;a.header.kind=110;a.body=QByteArray(896,0);C::putUInt32(a.body,100,1);C::putUInt32(a.body,824,request.header.kind);
        return {key,{request,a,C::requestFingerprint(C::encode(request,V4Direction::Request)),false},request.header.kind==6 || request.header.kind==25};
    }
    CommunicationV4Frame terminal() const {
        auto t=intent().record.replay;t.header.kind=111;t.body.replace(0,16,id().toRfc4122());
        if(request.header.kind==10)t.body.replace(16,16,id().toRfc4122());
        C::putUInt32(t.body,64,1);C::putUInt32(t.body,72,1);C::putUInt32(t.body,100,3);
        // Valid zero/false/empty values are distinct from missing slots.
        C::putUInt32(t.body,80,1);C::putUInt32(t.body,84,1);C::putUInt32(t.body,92,1);
        return digest(t);
    }
    static CommunicationV4Frame digest(CommunicationV4Frame t){
        // encode checks digest; assemble canonical frame from valid ACCEPTED first.
        auto a=t;a.header.kind=110;a.body=QByteArray(896,0);auto raw=C::encode(a,V4Direction::Response);
        C::putUInt32(raw,8,111);raw.replace(128,896,t.body);t.body.replace(32,32,C::terminalDigest(raw));C::encode(t,V4Direction::Response);return t;
    }
};
CommunicationV4Frame ack(const CommunicationV4Frame& t){auto a=t;a.header.kind=11;a.body=QByteArray(384,0);a.body.replace(0,16,t.body.first(16));a.body.replace(16,32,t.body.sliced(32,32));return a;}
struct Boundary:StationAdmissionBoundary {
    StationAdmissionSnapshot value;int captures=0;
    std::optional<StationAdmissionSnapshot> snapshot(const QUuid&) const override{return value;}
    StationAdmissionBarriers barriers(const QUuid&) const override{return {StationMode::ArmedAuto,true,false,false,0};}
    StationAdmissionReservation reserve(const StationAdmissionSnapshot&) override{return {0,std::make_shared<int>(1)};}
    void begin(const StationAdmissionIntent&,const std::shared_ptr<void>&) override{++captures;}
    bool drained(const QUuid&) const override{return true;}
};
void admissionRecovery(){
    Fixture f;Boundary b;auto snapshot=std::make_shared<StationSnapshot>();snapshot->station={f.key.station,f.request.header.stationKey,"S1"};snapshot->activation=f.request.header.activation;
    snapshot->selection.recipeKey=f.request.header.recipeRevisionKey;snapshot->selection.revision=id();b.value={snapshot,f.request.header.templateRevisionKey,id(),{},{}};
    f.request.body.replace(0,16,b.value.cameraSetRevision.toRfc4122());
    {StationDeliveryLedger l(f.root.path(),f.key.installation);StationDeliveryLedger::Admission store(l);StationAdmissionService service(store,b);
        check(service.trigger(f.key,f.request).header.kind==110,"Disk intent precedes single capture");
        check(l.inspect(f.key)->value.replay.header.kind==110,"Stored live intent readable");
        for(int n=0;n<100;++n)check(service.trigger(f.key,f.request).header.kind==110,"Duplicate live accepted");
        check(b.captures==1,"100 retries acquire once");}
    {StationDeliveryLedger l(f.root.path(),f.key.installation);StationDeliveryLedger::Admission store(l);StationAdmissionService service(store,b);
        for(int n=0;n<100;++n)check(C::uint32(service.trigger(f.key,f.request).body,76)==9,"Restart unresolved returns recovery");
        check(b.captures==1,"Restart never repeats capture");
        auto conflict=f.request;conflict.body[0]^=1;check(C::uint32(service.trigger(f.key,conflict).body,76)==6,"Conflict before mutable state");}
}
void crashBoundaries(){
    for(const auto& phase:{"INTENT","BEGIN","TERMINAL","PENDING","ACK_COMMITTED"})for(const auto& edge:{"before:","after:"}){
        Fixture f;const QString point=QString(edge)+phase;bool fired=false;
        auto t=f.terminal();bool accepted=false;
        {StationDeliveryLedger l(f.root.path(),f.key.installation,[&](const QString& p){if(p==point){fired=true;throw std::runtime_error("simulated process loss/disk full");}});
            try{check(l.reserve(f.intent())==0,"Reserve succeeds before fault");accepted=true;l.seal(f.key,t);l.acknowledge(f.key,ack(t));}catch(const std::exception&){}
        }
        check(fired,"Every write boundary injected");
        StationDeliveryLedger restarted(f.root.path(),f.key.installation);const auto r=restarted.inspect(f.key);
        if(point=="before:INTENT"){check(!r,"Unaccepted write leaves no request");continue;}
        check(r.has_value(),"Accepted/ambiguous intent is never lost");
        if(r->value.replay.header.kind==111){
            check(C::encode(r->value.replay,V4Direction::Response)==C::encode(t,V4Direction::Response),"Crash replays exact sealed terminal");
            restarted.acknowledge(f.key,ack(t));restarted.acknowledge(f.key,ack(t));
            check(restarted.inspect(f.key)->value.acknowledged,"Late exact ACK after restart");
        }else {
            check(C::uint32(r->value.replay.body,76)==9,"Unresolved intent/begin requires recovery");
            fails([&]{restarted.seal(f.key,t);},"Restart cannot invent DONE for unresolved request");
        }
        if(accepted)check(r.has_value(),"No accepted work lost on seal/ACK fault");
    }
}
void exactAndIsolation(){
    Fixture f;StationDeliveryLedger l(f.root.path(),f.key.installation);check(l.reserve(f.intent())==0,"S1 reserve");auto t=f.terminal();l.seal(f.key,t);
    const auto stable=C::encode(t,V4Direction::Response);l.seal(f.key,t);
    for(int field=0;field<10;++field){auto a=ack(t);auto key=f.key;
        switch(field){case 0:++a.header.stationKey;break;case 1:++a.header.requestId;break;case 2:a.header.peer=id();break;case 3:a.header.epoch=id();break;case 4:++a.header.recipeRevisionKey;break;case 5:++a.header.templateRevisionKey;break;case 6:a.header.activation=id();break;case 7:a.body[0]^=1;break;case 8:a.body[16]^=1;break;case 9:key.station=id();break;}
        fails([&]{l.acknowledge(key,a);},"Wrong exact ACK rejected");check(l.pending(f.key.station),"Wrong ACK keeps barrier");
    }
    auto conflicting=f.intent();conflicting.record.request.body[0]^=1;conflicting.record.fingerprint=C::requestFingerprint(C::encode(conflicting.record.request,V4Direction::Request));
    fails([&]{l.reserve(conflicting);},"Same key different fingerprint conflicts");
    auto second=f.intent();++second.identity.request;++second.record.request.header.requestId;++second.record.replay.header.requestId;second.record.fingerprint=C::requestFingerprint(C::encode(second.record.request,V4Direction::Request));
    check(l.reserve(second)==2,"Unacked normal slot blocks next normal");
    Fixture s2;s2.key.installation=f.key.installation;s2.key.peer=f.key.peer;s2.key.epoch=f.key.epoch;s2.request.header.peer=f.key.peer;s2.request.header.epoch=f.key.epoch;s2.request.header.stationKey=8;
    check(l.reserve(s2.intent())==0,"S2 independent normal slot");const auto t2=s2.terminal();l.seal(s2.key,t2);
    Fixture stop;stop.key=f.key;stop.key.request+=10;stop.request=f.request;stop.request.header.kind=6;stop.request.header.requestId=stop.key.request;stop.request.body=QByteArray(384,0);
    check(l.reserve(stop.intent())==0,"STOP reserves independently of production");const auto ts=stop.terminal();l.seal(stop.key,ts);
    auto stop2=stop.intent();++stop2.identity.request;++stop2.record.request.header.requestId;++stop2.record.replay.header.requestId;stop2.record.fingerprint=C::requestFingerprint(C::encode(stop2.record.request,V4Direction::Request));
    check(l.reserve(stop2)==23,"Second distinct STOP bounded");
    l.acknowledge(stop.key,ack(ts));check(l.pending(f.key.station) && !l.pending(f.key.station,true),"STOP ACK releases only control slot");
    l.acknowledge(f.key,ack(t));check(!l.pending(f.key.station) && l.pending(s2.key.station),"S1 ACK cannot clear S2");
    auto wireAck=ack(t);check(l.handle(f.key,wireAck)->header.kind==112,"Wire ACK dispatch returns exact ACKED");
    wireAck.body[16]^=1;check(C::uint32(l.handle(f.key,wireAck)->body,76)==14,"Wire wrong ACK is mismatch");
    auto recover=f.request;recover.header.kind=12;recover.body=QByteArray(384,0);recover.header.session=id();
    check(l.handle(f.key,recover)->body==t.body && l.handle(f.key,recover)->header.session==recover.header.session,"RECOVER uses sealed bytes with new session");
    check(l.reserve(second)==0,"Exact ACK frees normal slot");
    auto pendingRecover=second.record.request;pendingRecover.header.kind=12;pendingRecover.body=QByteArray(384,0);
    check(l.handle(second.identity,pendingRecover)->header.kind==113 && C::uint32(l.handle(second.identity,pendingRecover)->body,76)==9,"RECOVER of unresolved begin never executes");
    StationDeliveryLedger restarted(f.root.path(),f.key.installation);
    for(int n=0;n<100;++n){restarted.acknowledge(f.key,ack(t));auto replay=restarted.replay(f.key,id());check(replay.header.session!=t.header.session && replay.body==t.body,"Reconnect alters only transport session");}
    check(C::encode(restarted.inspect(f.key)->value.replay,V4Direction::Response)==stable,"Persisted terminal immutable after duplicate ACK/restart");
    fails([&]{auto changed=t;changed.body[152]^=1;restarted.seal(f.key,Fixture::digest(changed));},"Reseal different bytes rejected");
    // CALIB_ABORT is kind25; ACTIVATE24 must use the normal reserve.
    Fixture abort;abort.key=f.key;abort.key.request+=3;abort.request=f.request;abort.request.header.kind=25;abort.request.header.requestId=abort.key.request;abort.request.body=QByteArray(384,0);
    check(restarted.reserve(abort.intent())==0,"ABORT control reserve alongside unresolved production");
    const auto ta=abort.terminal();restarted.seal(abort.key,ta);restarted.acknowledge(abort.key,ack(ta));
    Fixture select;select.request.header.kind=4;C::putUInt32(select.request.body,0,99);C::putUInt32(select.request.body,4,100);
    StationDeliveryLedger sl(select.root.path(),select.key.installation);StationDeliveryLedger::Control control(sl);
    check(control.reserve(select.intent())==0,"SELECT original fingerprint saved");auto selected=select.terminal();selected.header.activation=id();selected.header.recipeRevisionKey=99;selected.header.templateRevisionKey=100;selected=Fixture::digest(selected);control.seal(select.key,selected);
    StationDeliveryLedger sr(select.root.path(),select.key.installation);StationDeliveryLedger::Control rc(sr);
    check(rc.find(select.key)->fingerprint==select.intent().record.fingerprint && rc.find(select.key)->request.header.activation==select.request.header.activation,"SELECT retry retains original request and new terminal activation");
    auto old=ack(selected);old.header.activation=select.request.header.activation;fails([&]{sr.acknowledge(select.key,old);},"SELECT ACK requires new activation");sr.acknowledge(select.key,ack(selected));rc.acknowledge(select.key,ack(selected));
    check(rc.find(select.key)->acknowledged,"Exact control ACK port remains idempotent");
    fails([&]{rc.acknowledge(select.key);},"Key-only ACK cannot release durable state");
    Fixture maximum;maximum.key.request=std::numeric_limits<quint64>::max();maximum.request.header.requestId=maximum.key.request;StationDeliveryLedger ml(maximum.root.path(),maximum.key.installation);check(ml.reserve(maximum.intent())==0,"MAX uint64 is exact");
    auto zero=maximum.intent();zero.identity.request=0;zero.record.request.header.requestId=0;fails([&]{ml.reserve(zero);},"No request wrap to zero");
}
void corruptedAndDisk(){
    Fixture f;StationDeliveryLedger l(f.root.path(),f.key.installation);check(l.reserve(f.intent())==0,"Torn fixture intent");auto t=f.terminal();l.seal(f.key,t);
    const auto folder=f.root.filePath("station_delivery");const auto files=QDir(folder).entryInfoList({"*.json"},QDir::Files);
    for(const auto& file:files){QFile in(file.absoluteFilePath());check(in.open(QIODevice::ReadOnly),"Read journal fixture");const auto data=in.readAll();in.close();
        if(QJsonDocument::fromJson(data).object()["payload"].toObject()["state"]=="BEGIN"){
            check(in.open(QIODevice::WriteOnly|QIODevice::Truncate),"Inject torn journal");in.write(data.first(data.size()/2));in.close();break;
        }
    }
    StationDeliveryLedger torn(f.root.path(),f.key.installation);fails([&]{torn.recovery();},"Torn journal fences recovery");fails([&]{torn.reserve(f.intent());},"Corrupt store closes admission");
    // Real QSaveFile open failure after accepted BEGIN: replace the next target
    // with a directory. No permission emulation or invented successful DONE.
    Fixture disk;StationDeliveryLedger dl(disk.root.path(),disk.key.installation);check(dl.reserve(disk.intent())==0,"Disk error fixture accepted");
    auto key=QJsonObject{{"installation",disk.key.installation.toString(QUuid::WithoutBraces)},{"peer",disk.key.peer.toString(QUuid::WithoutBraces)},{"epoch",disk.key.epoch.toString(QUuid::WithoutBraces)},{"station",disk.key.station.toString(QUuid::WithoutBraces)},{"request",QString::number(disk.key.request)}};
    const auto identity=QCryptographicHash::hash(QJsonDocument(key).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();
    const auto eid=QUuid::fromRfc4122(QCryptographicHash::hash(identity+":2",QCryptographicHash::Sha256).first(16)).toString(QUuid::WithoutBraces);
    const auto obstacle=disk.root.filePath("station_delivery/"+eid+".json");check(QDir().mkpath(obstacle),"Create actual disk write obstacle");
    fails([&]{dl.seal(disk.key,disk.terminal());},"Atomic terminal write failure reported");
    StationDeliveryLedger dr(disk.root.path(),disk.key.installation);check(C::uint32(dr.inspect(disk.key)->value.replay.body,76)==9,"Failed terminal never becomes DONE");
    // Legacy immutable archive remains readable and included in shared budget.
    const auto legacy=id().toString(QUuid::WithoutBraces);ProductionStore(disk.root.path()).append("events",legacy,{{"legacy",true}});
    check(ProductionStore(disk.root.path()).get("events",legacy)["legacy"].toBool(),"Legacy archive preserved");
}
void terminalHeadroom(){
    Fixture f;const auto events=f.root.filePath("events");check(QDir().mkpath(events),"Budget fixture directory");
    const QJsonObject payload{{"fixture","archive-headroom"}};
    const auto sha=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(payload).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
    // Valid immutable legacy envelopes, written directly to avoid quadratic
    // fixture setup through append's per-write budget scan. No production API
    // limit is changed. Ten free records must hold normal5 AND control5.
    for(int n=0;n<9990;++n){const auto uuid=id().toString(QUuid::WithoutBraces);QFile file(events+"/"+uuid+".json");
        const auto data=QJsonDocument(QJsonObject{{"schemaVersion",1},{"format","visionflow.production"},{"kind","events"},{"id",uuid},{"sha256",sha},{"payload",payload}}).toJson(QJsonDocument::Compact);
        if(!file.open(QIODevice::WriteOnly) || file.write(data)!=data.size())throw std::runtime_error("Budget fixture write failed");
    }
    StationDeliveryLedger l(f.root.path(),f.key.installation);check(l.reserve(f.intent())==0,"Normal admission reserves two terminals at archive boundary");
    Fixture stop;stop.key=f.key;stop.key.request+=10;stop.request=f.request;stop.request.header.kind=6;stop.request.header.requestId=stop.key.request;stop.request.body=QByteArray(384,0);
    check(l.reserve(stop.intent())==0,"STOP consumes pre-reserved headroom without double counting");
    const auto terminal=f.terminal(),stopped=stop.terminal();l.seal(f.key,terminal);l.seal(stop.key,stopped);l.acknowledge(f.key,ack(terminal));l.acknowledge(stop.key,ack(stopped));
    check(!l.pending(f.key.station) && !l.pending(f.key.station,true),"Both terminals and exact ACKs fit full archive");
    auto next=f.intent();++next.identity.request;++next.record.request.header.requestId;++next.record.replay.header.requestId;next.record.fingerprint=C::requestFingerprint(C::encode(next.record.request,V4Direction::Request));
    check(l.reserve(next)==7 && !l.inspect(next.identity),"Full archive rejects before accepted intent");
    check(QDir(events).entryList({"*.json"},QDir::Files).size()==9990,"Legacy records never evicted");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    admissionRecovery();crashBoundaries();exactAndIsolation();corruptedAndDisk();terminalHeadroom();
    QTextStream(stdout)<<"COMM4-13 ledger fixture v1: "<<checks<<" assertions passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
