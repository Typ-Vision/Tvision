#include "DeliveryStore.hpp"
#include "ProductionResultCodec.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLockFile>
#include <QUuid>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QString hash(const QByteArray& b){return QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex());}
QJsonObject key(const DeliveryRequest& r){require(!r.workcell.isEmpty()&&!r.project.isEmpty()&&!r.endpoint.isEmpty()&&!r.peer.isEmpty()&&!r.requestId.isEmpty()&&!r.fingerprint.isEmpty(),"Delivery identity/fingerprint missing");return {{"workcell",r.workcell},{"project",r.project},{"endpoint",r.endpoint},{"peer",r.peer},{"requestId",r.requestId}};}
QString identity(const DeliveryRequest& r){return hash(QJsonDocument(key(r)).toJson(QJsonDocument::Compact));}
QString eventId(const QString& key,int sequence){auto b=QCryptographicHash::hash((key+":"+QString::number(sequence)).toUtf8(),QCryptographicHash::Sha256).left(16);return QUuid::fromRfc4122(b).toString(QUuid::WithoutBraces);}
DeliveryStatus status(const QJsonObject& o){DeliveryStatus s;if(o.isEmpty())return s;s.state=o["state"].toString();if(s.state=="REQUEST_INTENT"||s.state=="CYCLE_BEGIN")s.state="RECOVERY_REQUIRED";s.cycleId=o["cycleId"].toString();s.payload=QByteArray::fromBase64(o["payload"].toString().toLatin1());s.payloadHash=o["payloadHash"].toString();s.attempts=o["attempts"].toInt();return s;}
struct Guard {QLockFile lock;explicit Guard(const QString& d):lock(d+"/delivery.lock"){require(!QFileInfo(d).isSymLink() && QDir().mkpath(d),"Delivery directory unavailable");lock.setStaleLockTime(0);require(lock.tryLock(0),"Delivery journal locked");}};
}
DeliveryStore::DeliveryStore(QString directory,Fault fault):directory_(QDir::cleanPath(directory)),fault_(std::move(fault)){require(QDir(directory_).isAbsolute(),"Delivery absolute directory required");}
QJsonObject DeliveryStore::load(const DeliveryRequest& r) const {
    const auto k=identity(r);QMap<QString,QMap<int,QJsonObject>> journals;
    for(const auto& o:ProductionStore(directory_).all("delivery")){
        require(o["format"]=="visionflow.delivery/1" && o["key"].isObject() && o["sequence"].isDouble(),"RECOVERY_REQUIRED: malformed delivery journal");
        const auto storedKey=o["key"].toObject();require(storedKey.size()==5 && o["identity"]==hash(QJsonDocument(storedKey).toJson(QJsonDocument::Compact)) && !o["fingerprint"].toString().isEmpty() && o["sequence"]==o["sequence"].toInt(-1) && o["sequence"].toInt(-1)>=0 && o["previous"].toString().size()==64,"RECOVERY_REQUIRED: malformed delivery identity");
        for(const QString field:{"workcell","project","endpoint","peer","requestId"})require(storedKey[field].isString()&&!storedKey[field].toString().isEmpty(),"RECOVERY_REQUIRED: malformed request key");
        const auto storedIdentity=o["identity"].toString();
        if(storedIdentity==k)require(o["key"].toObject()==key(r)&&o["fingerprint"]==r.fingerprint,"CONFLICT: request fingerprint differs");
        auto& events=journals[storedIdentity];
        const auto n=o["sequence"].toInt(-1);require(n>=0&&!events.contains(n),"RECOVERY_REQUIRED: duplicate sequence");events[n]=o;
    }
    QJsonObject found;
    for(auto journal=journals.cbegin();journal!=journals.cend();++journal){
    QJsonObject last;int n=0;
    for(const auto& o:journal.value()){require(o["sequence"].toInt()==n++ && o["previous"]==hash(QJsonDocument(last).toJson(QJsonDocument::Compact)),"RECOVERY_REQUIRED: incomplete delivery journal");
        const auto state=o["state"].toString(),prior=last.value("state").toString();
        require((last.isEmpty()&&state=="REQUEST_INTENT")||(prior=="REQUEST_INTENT"&&state=="CYCLE_BEGIN")||(prior=="CYCLE_BEGIN"&&state=="DELIVERY_PENDING")||((prior=="DELIVERY_PENDING"||prior=="ATTEMPT")&&(state=="ATTEMPT"||state=="ACK_RECEIVED"))||(prior=="ACK_RECEIVED"&&state=="ACK_COMMITTED"),"RECOVERY_REQUIRED: invalid delivery transition");
        require(state=="REQUEST_INTENT"||state=="CYCLE_BEGIN"||o.contains("payload"),"RECOVERY_REQUIRED: missing payload manifest");
        if(!last.isEmpty()){require(o["fingerprint"]==last["fingerprint"],"RECOVERY_REQUIRED: fingerprint changed");require(o["cycleId"]==last["cycleId"]||prior=="REQUEST_INTENT","RECOVERY_REQUIRED: cycle changed");}
        if(o.contains("payload")){const auto bytes=QByteArray::fromBase64(o["payload"].toString().toLatin1(),QByteArray::AbortOnBase64DecodingErrors);require(!bytes.isEmpty()&&bytes.size()<=65536&&hash(bytes)==o["payloadHash"],"RECOVERY_REQUIRED: invalid persisted payload");const auto terminal=ProductionStore(directory_).get("cycles",o["terminalId"].toString());require(hash(QJsonDocument(terminal).toJson(QJsonDocument::Compact))==o["terminalHash"] && terminal["cycleId"]==o["cycleId"] && terminal["resultBundle"].toObject()["bundleId"]==o["bundleId"],"RECOVERY_REQUIRED: terminal/bundle manifest mismatch");if(last.contains("payload"))require(o["payload"]==last["payload"]&&o["payloadHash"]==last["payloadHash"]&&o["terminalHash"]==last["terminalHash"],"RECOVERY_REQUIRED: payload changed");}
        last=o;
    }if(journal.key()==k)found=last;
    }return found;
}
void DeliveryStore::append(const DeliveryRequest& r,QJsonObject o,const QString& state) const {
    const auto previous=load(r);const int n=previous.isEmpty()?0:previous["sequence"].toInt()+1;
    o["format"]="visionflow.delivery/1";o["key"]=key(r);o["identity"]=identity(r);o["fingerprint"]=r.fingerprint;o["sequence"]=n;o["previous"]=hash(QJsonDocument(previous).toJson(QJsonDocument::Compact));o["state"]=state;
    if(fault_)fault_(state);ProductionStore(directory_).append("delivery",eventId(identity(r),n),o);
}
QVector<DeliveryRecovery> DeliveryStore::recovery() const {
    Guard g(directory_);QMap<QString,DeliveryRequest> requests;
    for(const auto& o:ProductionStore(directory_).all("delivery")){
        const auto k=o["key"].toObject();DeliveryRequest r{k["workcell"].toString(),k["project"].toString(),k["endpoint"].toString(),k["peer"].toString(),k["requestId"].toString(),o["fingerprint"].toString()};
        const auto id=identity(r);if(requests.contains(id))require(requests[id].fingerprint==r.fingerprint,"RECOVERY_REQUIRED: conflicting fingerprints");requests[id]=r;
    }
    QVector<DeliveryRecovery> result;for(const auto& r:requests)result.append({r,status(load(r))});return result;
}
QJsonObject DeliveryStore::recoveryTerminal(const DeliveryRequest& r) const {
    Guard g(directory_);const auto o=load(r);require(!o.isEmpty(),"Unknown request requires reconciliation");
    QJsonObject found;for(const auto& t:ProductionStore(directory_).all("cycles"))if(t["cycleId"]==o["cycleId"] && !o["cycleId"].toString().isEmpty()){
        require(found.isEmpty() && t["schemaVersion"]==10,"RECOVERY_REQUIRED: ambiguous/missing terminal bundle");found=t;
    }return found;
}
DeliveryStatus DeliveryStore::inspect(const DeliveryRequest& r) const {Guard g(directory_);return status(load(r));}
DeliveryStatus DeliveryStore::request(const DeliveryRequest& r) const {Guard g(directory_);auto o=load(r);if(o.isEmpty()){append(r,{},"REQUEST_INTENT");o=load(r);}return status(o);}
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
bool DeliveryStore::dispatch(const DeliveryRequest& r,const std::function<void(const QByteArray&)>& send) const {auto s=inspect(r);if(!s.dispatchable()||s.state=="ACK_RECEIVED")return false;attempt(r);send(s.payload);return true;}
}
