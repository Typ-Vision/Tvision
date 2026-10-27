#include "TriggerRouter.hpp"
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QJsonArray>
namespace visionflow {
namespace {
QByteArray key(const QStringList& parts){return QJsonDocument(QJsonArray::fromStringList(parts)).toJson(QJsonDocument::Compact);}
QByteArray snapshotKey(const InputSnapshot& s){return key({s.endpoint,s.peer,s.id,QString::number(s.revision)});}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void identity(const QString& s){require(!s.isEmpty() && s.size()<=256,"Invalid input/request identity");}
bool sameType(const AssignmentValue& a,const AssignmentValue& b){
    if(a.index()!=b.index())return false;
    if(auto x=std::get_if<ValueEnvelope>(&a)){auto y=std::get<ValueEnvelope>(b);if(x->type!=y.type || x->unit!=y.unit)return false;
        if(x->type==ScalarType::Enum){auto xv=x->value?std::get_if<EnumValue>(&*x->value):nullptr;auto yv=y.value?std::get_if<EnumValue>(&*y.value):nullptr;return xv && yv && xv->definitionId==yv->definitionId;}return true;}
    const auto& x=std::get<SpatialAssignmentValue>(a);const auto& y=std::get<SpatialAssignmentValue>(b);
    return x.value && y.value && x.unit==y.unit && x.value->source==y.value->source && x.value->target==y.value->target;
}
QJsonObject encoded(const InputSnapshot& s){QJsonObject fields;for(auto i=s.fields.begin();i!=s.fields.end();++i)fields[i.key()]=encodeAssignmentValue(i.value());return {{"endpoint",s.endpoint},{"peer",s.peer},{"id",s.id},{"revision",QString::number(s.revision)},{"recipe",s.recipeId},{"config",s.configId},{"sourceTime",s.sourceTime},{"clockId",s.clockId},{"fields",fields}};}
}
InputSnapshotStore::InputSnapshotStore(Clock clock):clock_(std::move(clock)) {if(!clock_)clock_=[] {return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();};}
void InputSnapshotStore::owner()const {if(thread_!=QThread::currentThread())throw ProductionException(ProductionError::WrongThread,"Input store owner thread required");}
void InputSnapshotStore::put(const InputSnapshot& s){owner();for(const auto& id:{s.endpoint,s.peer,s.id,s.recipeId,s.configId})identity(id);require(s.revision>0 && s.fields.size()<=64 && s.sourceTime.size()<=256 && s.clockId.size()<=256,"Invalid input snapshot");for(auto i=s.fields.begin();i!=s.fields.end();++i)identity(i.key());const auto bytes=QJsonDocument(encoded(s)).toJson(QJsonDocument::Compact);require(bytes.size()<=65536,"Input snapshot too large");const auto k=snapshotKey(s);auto it=entries_.constFind(k);if(it!=entries_.cend()){require(encoded(*it->snapshot)==encoded(s),"INPUT_CONFLICT");return;}require(entries_.size()<128,"INPUT_STORE_FULL");entries_.insert(k,{std::make_shared<const InputSnapshot>(s),clock_()});}
std::shared_ptr<const InputSnapshot> InputSnapshotStore::get(const QString& endpoint,const QString& peer,const QString& id,quint64 revision)const{owner();auto it=entries_.constFind(key({endpoint,peer,id,QString::number(revision)}));return it==entries_.cend()?nullptr:it->snapshot;}
bool InputSnapshotStore::fresh(const InputSnapshot& s,qint64 age)const {owner();auto it=entries_.constFind(snapshotKey(s));if(it==entries_.cend() || age<=0)return false;const auto elapsed=clock_()-it->received;return elapsed>=0 && elapsed<=age;}
DeliveryRequest TriggerRouter::deliveryRequest(const TriggerRequest& r){
    for(const auto& id:{r.workcell,r.project,r.endpoint,r.peer,r.requestId,r.recipeId,r.configId,r.inputSnapshotId})identity(id);
    require(r.inputRevision>0,"Input revision required");
    auto bytes=key({r.recipeId,r.configId,r.inputSnapshotId,QString::number(r.inputRevision),QString::number(r.sequence)});
    return {r.workcell,r.project,r.endpoint,r.peer,r.requestId,QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())};
}
AdmissionResult TriggerRouter::admit(const TriggerRequest& r,const QMap<QString,InputRequirement>& required,std::function<void()> beforeBegin,const QString& protocol){
    if(thread_!=QThread::currentThread())throw ProductionException(ProductionError::WrongThread,"Trigger router owner thread required");
    try {
        const auto d=deliveryRequest(r);const auto k=key({d.workcell,d.project,d.endpoint,d.peer,d.requestId});
        const auto durable=ledger_.inspect(d);if(durable.state!="ABSENT")return {"DUPLICATE",durable.state};
        auto prior=admitted_.constFind(k);if(prior!=admitted_.cend())return {*prior==d.fingerprint?"DUPLICATE":"CONFLICT",*prior==d.fingerprint?"ADMITTED":""};
        if(coordinator_.state()!=RuntimeState::Running)return {"NOT_RUNNING"};
        if(coordinator_.mode()!=RuntimeTrigger::SoftwareTrigger)return {"TRIGGER_SOURCE_DISABLED"};
        if(coordinator_.recordingPending())return {"RECORDING_PENDING"};
        if(coordinator_.deliveryPending())return {"DELIVERY_PENDING"};
        if(runtime_.busy())return {"BUSY"};
        const auto unavailable=unavailable_();if(!unavailable.isEmpty())return {unavailable};
        const auto active=runtime_.active();if(!active || active->recipe().id!=r.recipeId || QString::fromLatin1(active->validation().contentId)!=r.configId)return {"SNAPSHOT_MISMATCH"};
        const auto input=inputs_.get(r.endpoint,r.peer,r.inputSnapshotId,r.inputRevision);
        if(!input || input->recipeId!=r.recipeId || input->configId!=r.configId)return {"INPUT_MISMATCH"};
        require(required.size()<=64,"Invalid input profile");
        for(auto i=required.begin();i!=required.end();++i){auto value=input->fields.constFind(i.key());
            if(value==input->fields.cend())return {"INPUT_PARTIAL"};
            if(!std::visit([](const auto& v){return v.validity==ValueValidity::Valid && v.value.has_value();},*value))return {"INPUT_QUALITY"};
            if(!sameType(*value,i->shape))return {"INPUT_TYPE"};
            if(!inputs_.fresh(*input,i->maxAgeMs))return {"INPUT_STALE"};
        }
        if(admitted_.size()>=128)return {"QUEUE_FULL"};
        // The existing begin path is the sole cycle/permission/recording/camera gate.
        if(beforeBegin)beforeBegin();
        const QJsonObject communication{{"protocol",protocol},{"endpoint",r.endpoint},{"peer",r.peer},{"requestId",r.requestId},{"sequence",QString::number(r.sequence)},{"inputSnapshotId",r.inputSnapshotId},{"inputRevision",QString::number(r.inputRevision)}};
        auto cycle=coordinator_.begin(RuntimeTrigger::SoftwareTrigger,communication);
        admitted_.insert(k,d.fingerprint);return {{},{},cycle,input};
    }catch(const ProductionException& e){return {e.code()==ProductionError::Busy?"BUSY":QString::fromUtf8(e.what())};}
    catch(const std::exception& e){return {QString::fromUtf8(e.what())};}
}
}
