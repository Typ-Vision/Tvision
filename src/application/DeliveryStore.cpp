#include "DeliveryStore.hpp"
#include "ProductionResultCodec.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLockFile>
#include <QUuid>
#include <QSet>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QString hash(const QByteArray& b){return QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex());}
QJsonObject key(const DeliveryRequest& r){require(!r.workcell.isEmpty()&&!r.project.isEmpty()&&!r.endpoint.isEmpty()&&!r.peer.isEmpty()&&!r.requestId.isEmpty()&&!r.fingerprint.isEmpty(),"Delivery identity/fingerprint missing");return {{"workcell",r.workcell},{"project",r.project},{"endpoint",r.endpoint},{"peer",r.peer},{"requestId",r.requestId}};}
QString identity(const DeliveryRequest& r){return hash(QJsonDocument(key(r)).toJson(QJsonDocument::Compact));}
QJsonObject ns(const DeliveryRequest& r){auto o=key(r);o.remove("requestId");return o;}
QString eventId(const QString& key,int sequence){auto b=QCryptographicHash::hash((key+":"+QString::number(sequence)).toUtf8(),QCryptographicHash::Sha256).left(16);return QUuid::fromRfc4122(b).toString(QUuid::WithoutBraces);}
DeliveryStatus status(const QJsonObject& o){DeliveryStatus s;if(o.isEmpty())return s;s.state=o["state"].toString();if(s.state=="REQUEST_INTENT"||s.state=="CYCLE_BEGIN")s.state="RECOVERY_REQUIRED";s.cycleId=o["cycleId"].toString();s.payload=QByteArray::fromBase64(o["payload"].toString().toLatin1());s.payloadHash=o["payloadHash"].toString();s.attempts=o["attempts"].toInt();return s;}
struct Guard {QLockFile lock;explicit Guard(const QString& d):lock(d+"/delivery.lock"){require(!QFileInfo(d).isSymLink() && QDir().mkpath(d),"Delivery directory unavailable");lock.setStaleLockTime(0);require(lock.tryLock(0),"Delivery journal locked");}};
}
DeliveryStore::DeliveryStore(QString directory,Fault fault):directory_(QDir::cleanPath(directory)),fault_(std::move(fault)){require(QDir(directory_).isAbsolute(),"Delivery absolute directory required");}
QJsonObject DeliveryStore::load(const DeliveryRequest& r,QMap<QString,QJsonObject>* all) const {
    const auto k=identity(r);QMap<QString,QMap<int,QJsonObject>> journals;
    QVector<QJsonObject> records;
    if(all)records=ProductionStore(directory_).all("delivery");
    else {
        // Event IDs are deterministic. Inspect only this immutable chain, while
        // startup/admission still validate the complete archive. Scan the bounded
        // ID space as well, so a removed middle/first event cannot truncate it.
        require(!QFileInfo(directory_).isSymLink() && !QFileInfo(directory_+"/delivery").isSymLink(),"Delivery symlink refused");
        const auto files=QDir(directory_+"/delivery").entryInfoList({"*.json"},QDir::Files);require(files.size()<=10000,"Delivery record limit exceeded");
        QSet<QString> names;for(const auto& f:files){require(!f.isSymLink(),"Delivery symlink refused");names.insert(f.completeBaseName());}
        if(lookupIdentity_!=k){lookupEventIds_.clear();lookupEventIds_.reserve(10000);for(int n=0;n<10000;++n)lookupEventIds_.append(eventId(k,n));lookupIdentity_=k;}
        for(const auto& id:lookupEventIds_)if(names.contains(id))records.append(ProductionStore(directory_).get("delivery",id));
    }
    for(const auto& o:records){
        require(o["format"]=="visionflow.delivery/1" && o["key"].isObject() && o["sequence"].isDouble(),"RECOVERY_REQUIRED: malformed delivery journal");
        const auto storedKey=o["key"].toObject();require(storedKey.size()==5 && o["identity"]==hash(QJsonDocument(storedKey).toJson(QJsonDocument::Compact)) && !o["fingerprint"].toString().isEmpty() && o["sequence"]==o["sequence"].toInt(-1) && o["sequence"].toInt(-1)>=0 && o["previous"].toString().size()==64,"RECOVERY_REQUIRED: malformed delivery identity");
        for(const QString field:{"workcell","project","endpoint","peer","requestId"})require(storedKey[field].isString()&&!storedKey[field].toString().isEmpty(),"RECOVERY_REQUIRED: malformed request key");
        const auto storedIdentity=o["identity"].toString();require(all || storedIdentity==k,"RECOVERY_REQUIRED: foreign journal event");
        if(storedIdentity==k)require(o["key"].toObject()==key(r)&&o["fingerprint"]==r.fingerprint,"CONFLICT: request fingerprint differs");
        auto& events=journals[storedIdentity];
        const auto n=o["sequence"].toInt(-1);require(n>=0&&!events.contains(n),"RECOVERY_REQUIRED: duplicate sequence");events[n]=o;
    }
    QJsonObject found;QMap<QString,QJsonObject> terminals;
    for(auto journal=journals.cbegin();journal!=journals.cend();++journal){
    QJsonObject last;int n=0;
    for(const auto& o:journal.value()){require(o["sequence"].toInt()==n++ && o["previous"]==hash(QJsonDocument(last).toJson(QJsonDocument::Compact)),"RECOVERY_REQUIRED: incomplete delivery journal");
        const auto state=o["state"].toString(),prior=last.value("state").toString();
        require((last.isEmpty()&&state=="REQUEST_INTENT")||(prior=="REQUEST_INTENT"&&state=="CYCLE_BEGIN")||(prior=="CYCLE_BEGIN"&&state=="DELIVERY_PENDING")||((prior=="DELIVERY_PENDING"||prior=="ATTEMPT")&&(state=="ATTEMPT"||state=="ACK_RECEIVED"))||(prior=="ACK_RECEIVED"&&state=="ACK_COMMITTED"),"RECOVERY_REQUIRED: invalid delivery transition");
        require(state=="REQUEST_INTENT"||state=="CYCLE_BEGIN"||o.contains("payload"),"RECOVERY_REQUIRED: missing payload manifest");
        if(!last.isEmpty()){require(o["fingerprint"]==last["fingerprint"],"RECOVERY_REQUIRED: fingerprint changed");require(o["cycleId"]==last["cycleId"]||prior=="REQUEST_INTENT","RECOVERY_REQUIRED: cycle changed");}
        if(o.contains("payload")){const auto bytes=QByteArray::fromBase64(o["payload"].toString().toLatin1(),QByteArray::AbortOnBase64DecodingErrors);require(!bytes.isEmpty()&&bytes.size()<=65536&&hash(bytes)==o["payloadHash"],"RECOVERY_REQUIRED: invalid persisted payload");const auto terminalId=o["terminalId"].toString();if(!terminals.contains(terminalId))terminals[terminalId]=ProductionStore(directory_).get("cycles",terminalId);const auto terminal=terminals.value(terminalId);require(hash(QJsonDocument(terminal).toJson(QJsonDocument::Compact))==o["terminalHash"] && terminal["cycleId"]==o["cycleId"] && terminal["resultBundle"].toObject()["bundleId"]==o["bundleId"],"RECOVERY_REQUIRED: terminal/bundle manifest mismatch");if(last.contains("payload"))require(o["payload"]==last["payload"]&&o["payloadHash"]==last["payloadHash"]&&o["terminalHash"]==last["terminalHash"],"RECOVERY_REQUIRED: payload changed");}
        last=o;
    }if(all)(*all)[journal.key()]=last;if(journal.key()==k)found=last;
    }return found;
}
void DeliveryStore::append(const DeliveryRequest& r,QJsonObject o,const QString& state) const {
    const auto previous=load(r);const int n=previous.isEmpty()?0:previous["sequence"].toInt()+1;
    o["format"]="visionflow.delivery/1";o["key"]=key(r);o["identity"]=identity(r);o["fingerprint"]=r.fingerprint;o["sequence"]=n;o["previous"]=hash(QJsonDocument(previous).toJson(QJsonDocument::Compact));o["state"]=state;
    if(fault_)fault_(state);ProductionStore(directory_).append("delivery",eventId(identity(r),n),o);
}
QVector<DeliveryRecovery> DeliveryStore::recovery() const {
    Guard g(directory_);return readRecovery();
}
QVector<DeliveryRecovery> DeliveryStore::readRecovery() const {
    QMap<QString,DeliveryRequest> requests;
    for(const auto& o:ProductionStore(directory_).all("delivery")){
        const auto k=o["key"].toObject();DeliveryRequest r{k["workcell"].toString(),k["project"].toString(),k["endpoint"].toString(),k["peer"].toString(),k["requestId"].toString(),o["fingerprint"].toString()};
        const auto id=identity(r);if(requests.contains(id))require(requests[id].fingerprint==r.fingerprint,"RECOVERY_REQUIRED: conflicting fingerprints");requests[id]=r;
    }
    QMap<QString,QJsonObject> latest;if(!requests.isEmpty())load(requests.first(),&latest);
    QVector<DeliveryRecovery> result;for(auto i=requests.cbegin();i!=requests.cend();++i)result.append({i.value(),status(latest.value(i.key()))});return result;
}
QJsonObject DeliveryStore::recoveryTerminal(const DeliveryRequest& r) const {
    Guard g(directory_);const auto o=load(r);require(!o.isEmpty(),"Unknown request requires reconciliation");
    QJsonObject found;for(const auto& t:ProductionStore(directory_).all("cycles"))if(t["cycleId"]==o["cycleId"] && !o["cycleId"].toString().isEmpty()){
        require(found.isEmpty() && t["schemaVersion"]==10,"RECOVERY_REQUIRED: ambiguous/missing terminal bundle");found=t;
    }return found;
}
void DeliveryStore::requireNamespace(const DeliveryRequest& r) const {
    for(const auto& o:ProductionStore(directory_).all("delivery_namespaces")){
        require(o["format"]=="visionflow.namespace-retirement/1" && o["from"].isObject() && o["to"].isObject() && !o["agreement"].toString().isEmpty(),"RECOVERY_REQUIRED: invalid namespace retirement");
        require(o["from"].toObject()!=ns(r),"NAMESPACE_RETIRED");
    }
}
QString DeliveryStore::admission(const DeliveryRequest& r,bool required,bool online) const {
    Guard g(directory_);return admissionLocked(r,required,online);
}
QString DeliveryStore::admissionLocked(const DeliveryRequest& r,bool required,bool online) const {
    try{requireNamespace(r);if(required && !online)return "ENDPOINT_OFFLINE";
        int pending=0;for(const auto& item:readRecovery())if(item.request.workcell==r.workcell && item.request.project==r.project && item.request.endpoint==r.endpoint && item.status.state!="ACK_COMMITTED")++pending;
        if(required && pending)return "DELIVERY_PENDING";if(pending>=128)return "QUEUE_FULL";
        // Leave room for terminal/snapshot and all unresolved delivery journals.
        ProductionStore(directory_).requireCapacity(32+pending*8,32LL*1024*1024+pending*1024LL*1024);return {};
    }catch(const std::exception& e){return QString::fromUtf8(e.what());}
}
void DeliveryStore::rolloverNamespace(const DeliveryRequest& from,const DeliveryRequest& to,const QString& agreement) const {
    Guard g(directory_);requireNamespace(from);requireNamespace(to);
    require(from.workcell==to.workcell && from.project==to.project && from.endpoint==to.endpoint && from.peer!=to.peer && !agreement.trimmed().isEmpty() && agreement.size()<=1024,"Namespace rollover requires new peer generation and peer agreement");
    // Validate all chains before writing a retirement. No pending work may vanish.
    for(const auto& item:readRecovery()){
        require(ns(item.request)!=ns(to),"Target namespace already used");if(ns(item.request)==ns(from))require(item.status.state=="ACK_COMMITTED","Namespace has unresolved requests");
    }
    for(const auto& o:ProductionStore(directory_).all("delivery_namespaces"))require(o["to"].toObject()!=ns(to),"Target namespace already armed");
    const auto digest=QCryptographicHash::hash(QJsonDocument(ns(from)).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).left(16);
    ProductionStore(directory_).append("delivery_namespaces",QUuid::fromRfc4122(digest).toString(QUuid::WithoutBraces),{{"format","visionflow.namespace-retirement/1"},{"from",ns(from)},{"to",ns(to)},{"agreement",agreement}});
}
DeliveryStatus DeliveryStore::inspect(const DeliveryRequest& r) const {Guard g(directory_);requireNamespace(r);return status(load(r));}
DeliveryStatus DeliveryStore::request(const DeliveryRequest& r,bool required,bool online) const {
    Guard g(directory_);requireNamespace(r);auto o=load(r);if(o.isEmpty()){const auto reason=admissionLocked(r,required,online);require(reason.isEmpty(),qPrintable(reason));append(r,{},"REQUEST_INTENT");o=load(r);}return status(o);
}
void DeliveryStore::begin(const DeliveryRequest& r,const QString& cycle) const {Guard g(directory_);auto o=load(r);require(!QUuid(cycle).isNull(),"Cycle UUID required");if(o["state"]!="REQUEST_INTENT"){require(o["cycleId"]==cycle,"Delivery cycle conflict");return;}o["cycleId"]=cycle;append(r,o,"CYCLE_BEGIN");}
void DeliveryStore::commit(const DeliveryRequest& r,const QJsonObject& terminal,const QByteArray& bytes) const {
    Guard g(directory_);auto o=load(r);ProductionResultCodec::validate(terminal);require(terminal["schemaVersion"]==10&&terminal["cycleId"]==o["cycleId"]&&!bytes.isEmpty()&&bytes.size()<=65536,"Delivery terminal/payload invalid");
    const auto terminalHash=hash(QJsonDocument(terminal).toJson(QJsonDocument::Compact));
    if(o["state"]!="CYCLE_BEGIN"){require(o.contains("payload")&&o["payloadHash"]==hash(bytes)&&o["terminalHash"]==terminalHash,"Immutable delivery conflict");return;}
    if(fault_)fault_("TERMINAL");ProductionStore(directory_).append("cycles",terminal["recordId"].toString(),terminal);
    o["terminalId"]=terminal["recordId"];o["terminalHash"]=terminalHash;o["bundleId"]=terminal["resultBundle"].toObject()["bundleId"];o["payload"]=QString::fromLatin1(bytes.toBase64());o["payloadHash"]=hash(bytes);o["attempts"]=0;
    append(r,o,"DELIVERY_PENDING");
}
void DeliveryStore::attempt(const DeliveryRequest& r) const {Guard g(directory_);auto o=load(r);require(o["state"]=="DELIVERY_PENDING"||o["state"]=="ATTEMPT","Delivery not pending");require(o["attempts"].toInt()<10000,"Attempt journal limit");o["attempts"]=o["attempts"].toInt()+1;append(r,o,"ATTEMPT");}
void DeliveryStore::acknowledge(const DeliveryRequest& r,const QString& digest) const {Guard g(directory_);auto o=load(r);require(o.contains("payload")&&o["payloadHash"]==digest,"ACK payload mismatch");if(o["state"]=="ACK_RECEIVED"||o["state"]=="ACK_COMMITTED")return;require(status(o).dispatchable(),"ACK not pending");append(r,o,"ACK_RECEIVED");}
void DeliveryStore::commitAcknowledgement(const DeliveryRequest& r) const {Guard g(directory_);auto o=load(r);if(o["state"]=="ACK_COMMITTED")return;require(o["state"]=="ACK_RECEIVED","ACK receipt missing");append(r,o,"ACK_COMMITTED");}
bool DeliveryStore::dispatch(const DeliveryRequest& r,const std::function<void(const QByteArray&)>& send) const {auto s=inspect(r);if(!s.dispatchable()||s.state=="ACK_RECEIVED")return false;
    for(const auto& item:recovery())if(item.request.workcell==r.workcell && item.request.project==r.project && item.request.endpoint==r.endpoint && identity(item.request)!=identity(r) && (item.status.state=="ATTEMPT"||item.status.state=="ACK_RECEIVED"))return false;
    attempt(r);send(s.payload);return true;}
}
