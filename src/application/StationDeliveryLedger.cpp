#include "StationDeliveryLedger.hpp"
#include "ProductionStore.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLockFile>
#include <stdexcept>

namespace visionflow {
namespace {
using C=CommunicationV4Codec;
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
QString hash(const QByteArray& b){return QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex());}
QByteArray json(const QJsonObject& o){return QJsonDocument(o).toJson(QJsonDocument::Compact);}
QString uuid(const QUuid& u){return u.toString(QUuid::WithoutBraces);}
QJsonObject keyJson(const V4RequestIdentity& k){return {{"installation",uuid(k.installation)},{"peer",uuid(k.peer)},{"epoch",uuid(k.epoch)},{"station",uuid(k.station)},{"request",QString::number(k.request)}};}
V4RequestIdentity readKey(const QJsonObject& o){
    bool ok=false;const auto n=o["request"].toString().toULongLong(&ok);
    V4RequestIdentity k{QUuid(o["installation"].toString()),QUuid(o["peer"].toString()),QUuid(o["epoch"].toString()),QUuid(o["station"].toString()),n};
    require(ok && !k.installation.isNull() && !k.peer.isNull() && !k.epoch.isNull() && !k.station.isNull() && n && keyJson(k)==o,"RECOVERY_REQUIRED: invalid v4 key");return k;
}
QString eventId(const QString& id,int n){return uuid(QUuid::fromRfc4122(QCryptographicHash::hash((id+":"+QString::number(n)).toUtf8(),QCryptographicHash::Sha256).first(16)));}
QByteArray unbase(const QJsonValue& v){require(v.isString(),"RECOVERY_REQUIRED: missing v4 bytes");const auto s=v.toString().toLatin1();const auto b=QByteArray::fromBase64(s,QByteArray::AbortOnBase64DecodingErrors);require(b.toBase64()==s,"RECOVERY_REQUIRED: malformed v4 bytes");return b;}
bool sameHeader(const CommunicationV4Header& a,const CommunicationV4Header& b){return a.stationKey==b.stationKey && a.requestId==b.requestId && a.peer==b.peer && a.epoch==b.epoch && a.recipeRevisionKey==b.recipeRevisionKey && a.templateRevisionKey==b.templateRevisionKey && a.activation==b.activation;}
bool controlKind(quint32 kind){return kind==6 || kind==25;}
void validatePair(const StationControlRecord& r){
    const auto& a=r.request;const auto& b=r.replay;
    require(C::requestFingerprint(C::encode(a,V4Direction::Request))==r.fingerprint,"CONFLICT: v4 fingerprint");
    C::encode(b,V4Direction::Response);
    if(b.header.kind==111)require(!QUuid::fromRfc4122(b.body.first(16)).isNull(),"V4 terminal result UUID required");
    require((b.header.kind==110 || b.header.kind==111) && a.header.stationKey==b.header.stationKey && a.header.requestId==b.header.requestId && a.header.peer==b.header.peer && a.header.epoch==b.header.epoch && C::uint32(b.body,824)==a.header.kind,"RECOVERY_REQUIRED: v4 response identity");
    if(a.header.kind==4 && b.header.kind==111 && C::uint32(b.body,76)==0)
        require(!b.header.activation.isNull() && b.header.activation!=a.header.activation && b.header.recipeRevisionKey==C::uint32(a.body,0) && b.header.templateRevisionKey==C::uint32(a.body,4),"RECOVERY_REQUIRED: v4 selection identity");
    else require(sameHeader(a.header,b.header),"RECOVERY_REQUIRED: v4 stable header");
}
struct Guard {
    QLockFile lock;
    explicit Guard(const QString& d):lock(d+"/station-delivery.lock"){
        require(!QFileInfo(d).isSymLink() && QDir().mkpath(d),"V4 ledger directory unavailable");lock.setStaleLockTime(0);require(lock.tryLock(0),"V4 ledger locked");
    }
};
}
StationDeliveryLedger::StationDeliveryLedger(QString dir,QUuid installation,Fault fault):directory_(QDir::cleanPath(dir)),installation_(installation),fault_(std::move(fault)){
    require(QDir(directory_).isAbsolute() && !installation_.isNull(),"V4 ledger absolute directory/installation required");
}
QString StationDeliveryLedger::identity(const V4RequestIdentity& k) const {readKey(keyJson(k));require(k.installation==installation_,"V4 installation mismatch");return hash(json(keyJson(k)));}
QMap<QString,StationDeliveryLedger::Entry> StationDeliveryLedger::load() const {
    QMap<QString,QMap<int,QJsonObject>> chains;
    for(const auto& o:ProductionStore(directory_).all("station_delivery")){
        require(o["format"]=="visionflow.station-delivery/1" && o.size()==10,"RECOVERY_REQUIRED: unsupported v4 journal");
        const auto k=readKey(o["key"].toObject());const auto id=identity(k);const auto n=o["sequence"].toInt(-1);
        require(o["identity"]==id && n>=0 && n<5 && o["sequence"]==n && !chains[id].contains(n),"RECOVERY_REQUIRED: v4 journal sequence");
        // Check physical deterministic path too: renamed/replaced records fail closed.
        require(ProductionStore(directory_).get("station_delivery",eventId(id,n))==o,"RECOVERY_REQUIRED: v4 journal path");chains[id][n]=o;
    }
    QMap<QString,Entry> result;
    for(auto it=chains.cbegin();it!=chains.cend();++it){
        Entry e;QJsonObject previous;int expected=0;QByteArray original,fingerprint,terminal;
        for(const auto& o:it.value()){
            const auto state=o["state"].toString(),prior=previous.value("state").toString();
            require(o["sequence"].toInt()==expected++ && o["previous"]==hash(json(previous)),"RECOVERY_REQUIRED: torn v4 chain");
            require((prior.isEmpty() && state=="INTENT") || (prior=="INTENT" && state=="BEGIN") || (prior=="BEGIN" && state=="TERMINAL") || (prior=="TERMINAL" && state=="PENDING") || ((prior=="PENDING" || prior=="TERMINAL") && state=="ACK_COMMITTED"),"RECOVERY_REQUIRED: invalid v4 transition");
            const auto request=unbase(o["request"]),response=unbase(o["response"]);const auto fp=QByteArray::fromHex(o["fingerprint"].toString().toLatin1());
            auto a=C::decode(request,V4Direction::Request);auto b=C::decode(response,V4Direction::Response);const auto k=readKey(o["key"].toObject());
            require(k.request==a.header.requestId && k.peer==a.header.peer && k.epoch==a.header.epoch && a.header.stationKey,"RECOVERY_REQUIRED: v4 key/header mismatch");
            require(o["control"].isBool() && o["control"].toBool()==controlKind(a.header.kind),"RECOVERY_REQUIRED: v4 reserve class");
            StationControlRecord r{a,b,fp,state=="ACK_COMMITTED"};validatePair(r);
            require((state=="INTENT" || state=="BEGIN")?b.header.kind==110:b.header.kind==111,"RECOVERY_REQUIRED: v4 state/response mismatch");
            if(previous.isEmpty()){original=request;fingerprint=fp;}
            else require(request==original && fp==fingerprint && o["control"]==previous["control"],"RECOVERY_REQUIRED: v4 request changed");
            if(!terminal.isEmpty())require(response==terminal,"RECOVERY_REQUIRED: sealed v4 bytes changed");
            if(b.header.kind==111)terminal=response;
            e.record={k,r,state,o["control"].toBool()};e.sequence=o["sequence"].toInt();e.event=o;previous=o;
        }
        result[it.key()]=e;
    }
    // Historical records never evict. Detect impossible overlapping reservations.
    QSet<QString> occupied;
    for(const auto& e:result)if(e.record.state!="ACK_COMMITTED"){
        const auto slot=uuid(e.record.key.station)+(e.record.control?":control":":normal");
        require(!occupied.contains(slot),"RECOVERY_REQUIRED: overlapping v4 reservations");occupied.insert(slot);
    }
    return result;
}
void StationDeliveryLedger::append(Entry& e,const QString& state){
    const auto& r=e.record;const auto id=identity(r.key);const int n=e.event.isEmpty()?0:e.sequence+1;
    QJsonObject o{{"format","visionflow.station-delivery/1"},{"key",keyJson(r.key)},{"identity",id},{"sequence",n},{"previous",hash(json(e.event))},{"state",state},{"request",QString::fromLatin1(C::encode(r.value.request,V4Direction::Request).toBase64())},{"response",QString::fromLatin1(C::encode(r.value.replay,V4Direction::Response).toBase64())},{"fingerprint",QString::fromLatin1(r.value.fingerprint.toHex())},{"control",r.control}};
    if(fault_)fault_("before:"+state);
    ProductionStore(directory_).append("station_delivery",eventId(id,n),o);
    e.event=o;e.sequence=n;e.record.state=state;e.record.value.acknowledged=state=="ACK_COMMITTED";
    if(fault_)fault_("after:"+state);
}
StationControlRecord StationDeliveryLedger::view(const Entry& e) const {
    auto r=e.record.value;
    if(r.replay.header.kind==110 && !live_.contains(identity(e.record.key))){
        r.replay.header.kind=190;r.replay.body=QByteArray(896,0);C::putUInt32(r.replay.body,76,9);C::putUInt32(r.replay.body,100,12);C::putUInt32(r.replay.body,824,r.request.header.kind);
    }
    return r;
}
std::optional<StationDeliveryLedger::Record> StationDeliveryLedger::inspect(const V4RequestIdentity& k) const {
    Guard guard(directory_);const auto id=identity(k);const auto all=load();const auto e=all.constFind(id);if(e==all.cend())return {};auto r=e->record;r.value=view(*e);return r;
}
QVector<StationDeliveryLedger::Record> StationDeliveryLedger::recovery() const {
    Guard guard(directory_);QVector<Record> records;for(const auto& e:load()){auto r=e.record;r.value=view(e);records.append(r);}return records;
}
bool StationDeliveryLedger::pending(const QUuid& station,bool control) const {
    for(const auto& r:recovery())if(r.key.station==station && r.control==control && !r.value.acknowledged)return true;return false;
}
quint32 StationDeliveryLedger::reserve(const StationControlIntent& intent){
    Guard guard(directory_);const auto id=identity(intent.identity);auto all=load();
    validatePair(intent.record);require(intent.record.replay.header.kind==110 && !intent.record.acknowledged,"V4 accepted intent required");
    const auto& h=intent.record.request.header;
    require(h.peer==intent.identity.peer && h.epoch==intent.identity.epoch && h.requestId==intent.identity.request && h.stationKey && intent.controlReserve==controlKind(h.kind),"V4 intent key/reserve mismatch");
    require(h.kind==4 || h.kind==5 || h.kind==6 || h.kind==10 || (h.kind>=20 && h.kind<=25),"V4 command does not create intent");
    if(all.contains(id)){require(all[id].record.value.fingerprint==intent.record.fingerprint,"CONFLICT: v4 request differs");return 0;}
    int remaining=0;QSet<QUuid> stations,normal,control;
    for(const auto& e:all)if(!e.record.value.acknowledged){
        stations.insert(e.record.key.station);remaining+=4-e.sequence;
        (e.record.control?control:normal).insert(e.record.key.station);
        if(e.record.key.station==intent.identity.station && e.record.control==intent.controlReserve)return intent.controlReserve?23:2;
    }
    if(!stations.contains(intent.identity.station) && stations.size()>=32)return 23;
    (intent.controlReserve?control:normal).insert(intent.identity.station);
    // Count the remaining events ONCE, including the requested slot. Reserve
    // one future control terminal only where normal has no control yet. A STOP
    // consumes that reserved headroom instead of reserving it a second time.
    int reserve=remaining+5;
    for(const auto& station:normal)if(!control.contains(station))reserve+=5;
    try{ProductionStore(directory_).requireCapacity(reserve,qint64(reserve)*4096);}catch(const std::exception& e){if(QByteArray(e.what())=="QUEUE_FULL")return 7;throw;}
    Entry e;e.record={intent.identity,intent.record,"",intent.controlReserve};
    append(e,"INTENT");append(e,"BEGIN");live_.insert(id);return 0;
}
void StationDeliveryLedger::seal(const V4RequestIdentity& k,const CommunicationV4Frame& terminal){
    Guard guard(directory_);auto all=load();const auto id=identity(k);require(all.contains(id),"Unknown v4 intent");auto e=all[id];
    auto r=e.record.value;r.replay=terminal;validatePair(r);require(terminal.header.kind==111,"V4 terminal required");
    if(e.record.value.replay.header.kind==111){require(C::encode(terminal,V4Direction::Response)==C::encode(e.record.value.replay,V4Direction::Response),"Immutable v4 terminal conflict");}
    else {
        require(e.record.state=="BEGIN" && live_.contains(id),"RECOVERY_REQUIRED: no live v4 begin");
        e.record.value=r;append(e,"TERMINAL");
    }
    if(e.record.state=="TERMINAL")append(e,"PENDING");
}
void StationDeliveryLedger::acknowledgeLocked(Entry& e,const CommunicationV4Frame& ack){
    C::encode(ack,V4Direction::Request);const auto& t=e.record.value.replay;
    require(ack.header.kind==11 && t.header.kind==111 && sameHeader(ack.header,t.header) && ack.body.first(16)==t.body.first(16) && ack.body.sliced(16,32)==t.body.sliced(32,32),"ACK_MISMATCH");
    if(e.record.state!="ACK_COMMITTED")append(e,"ACK_COMMITTED");
}
void StationDeliveryLedger::acknowledge(const V4RequestIdentity& k,const CommunicationV4Frame& ack){
    Guard guard(directory_);auto all=load();const auto id=identity(k);require(all.contains(id),"ACK_MISMATCH");auto e=all[id];acknowledgeLocked(e,ack);
}
CommunicationV4Frame StationDeliveryLedger::replay(const V4RequestIdentity& k,const QUuid& session) const {
    const auto r=inspect(k);require(r.has_value() && !session.isNull(),"Unknown v4 replay/session");auto f=r->value.replay;f.header.session=session;return f;
}
std::optional<CommunicationV4Frame> StationDeliveryLedger::handle(const V4RequestIdentity& k,const CommunicationV4Frame& f){
    if(f.header.kind!=11 && f.header.kind!=12)return {};
    C::encode(f,V4Direction::Request);
    CommunicationV4Frame response{f.header,QByteArray(896,0)};response.header.kind=190;
    auto reject=[&](quint32 reason){C::putUInt32(response.body,76,reason);return response;};
    if(k.peer!=f.header.peer || k.epoch!=f.header.epoch || k.request!=f.header.requestId)return reject(f.header.kind==11?14:10);
    try{
        if(f.header.kind==11){
            try{acknowledge(k,f);}catch(const std::runtime_error& e){if(QByteArray(e.what())=="ACK_MISMATCH")return reject(14);throw;}
            const auto record=inspect(k);const auto& t=record->value.replay;response.header.kind=112;
            response.body.replace(0,16,t.body.first(16));response.body.replace(32,32,t.body.sliced(32,32));C::putUInt32(response.body,100,4);C::putUInt32(response.body,824,C::uint32(t.body,824));return response;
        }
        const auto r=inspect(k);if(!r)return reject(10);
        // Recovery lookup may use the old SELECT request snapshot. Never expose
        // another numerical station under a forged resolved UUID.
        if(r->value.request.header.stationKey!=f.header.stationKey)return reject(10);
        auto result=r->value.replay;result.header.session=f.header.session;
        if(result.header.kind!=111){result.header.kind=113;C::putUInt32(result.body,76,9);C::putUInt32(result.body,100,12);}return result;
    }catch(...){return reject(8);}
}
std::optional<StationAdmissionRecord> StationDeliveryLedger::Admission::find(const V4RequestIdentity& k) const {
    const auto r=ledger_.inspect(k);if(!r)return {};return StationAdmissionRecord{r->value.fingerprint,r->value.replay};
}
void StationDeliveryLedger::Admission::commit(const StationAdmissionIntent& i){
    const auto reason=ledger_.reserve({i.identity,{i.request,i.accepted,i.fingerprint,false},false});require(!reason,"V4 admission reservation unavailable");
}
std::optional<StationControlRecord> StationDeliveryLedger::Control::find(const V4RequestIdentity& k) const {const auto r=ledger_.inspect(k);if(!r)return {};return r->value;}
void StationDeliveryLedger::Control::acknowledge(const V4RequestIdentity&){
    throw std::runtime_error("Exact v4 ACK frame required");
}
}
