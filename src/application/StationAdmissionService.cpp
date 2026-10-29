#include "StationAdmissionService.hpp"
#include <QScopeGuard>
#include <stdexcept>

namespace visionflow {
namespace {
using C=CommunicationV4Codec;
QUuid uuid(const QByteArray& b,int offset){return QUuid::fromRfc4122(b.sliced(offset,16));}
CommunicationV4Frame response(const CommunicationV4Frame& f,quint32 reason,const char* detail="",bool accepted=false){
    CommunicationV4Frame r{f.header,QByteArray(896,0)};r.header.kind=accepted?110:190;
    C::putUInt32(r.body,76,reason);C::putUInt32(r.body,100,accepted?1:0);C::putUInt32(r.body,824,f.header.kind);
    const QByteArray text(detail);C::putUInt32(r.body,616,text.size());r.body.replace(620,text.size(),text);return r;
}
bool same(const StationAdmissionSnapshot& a,const StationAdmissionSnapshot& b){
    return a.station && b.station && a.station->station==b.station->station
        && a.station->activation==b.station->activation && a.station->selection.recipeKey==b.station->selection.recipeKey
        && a.station->selection.revision==b.station->selection.revision && a.templateKey==b.templateKey
        && a.cameraSetRevision==b.cameraSetRevision && a.inputSnapshot==b.inputSnapshot && a.inputRevision==b.inputRevision && a.inputValues==b.inputValues;
}
quint32 gate(const StationAdmissionBarriers& b,StationTriggerOrigin origin){
    if(b.recordingPending)return 8;
    if(b.deliveryPending)return 2;
    if(b.mode==StationMode::Running)return 2;
    if(b.mode==StationMode::Stopped || (origin==StationTriggerOrigin::Plc && b.mode==StationMode::ArmedManual))return 1;
    if(!b.ready || (b.mode!=StationMode::ArmedAuto && b.mode!=StationMode::ArmedManual))return 20;
    return b.resourceReason;
}
const char* detail(const StationAdmissionBarriers& b){
    if(b.recordingPending)return "PENDING_STORAGE";
    if(b.deliveryPending)return "PENDING_DELIVERY";
    return "ADMISSION_BARRIER";
}
bool recordMatches(const StationAdmissionRecord& record,const CommunicationV4Frame& f){
    const auto& h=record.replay.header;
    if((h.kind!=110 && h.kind!=111) || h.stationKey!=f.header.stationKey || h.requestId!=f.header.requestId
       || h.peer!=f.header.peer || h.epoch!=f.header.epoch || h.recipeRevisionKey!=f.header.recipeRevisionKey
       || h.templateRevisionKey!=f.header.templateRevisionKey || h.activation!=f.header.activation)return false;
    try{C::encode(record.replay,V4Direction::Response);return true;}catch(...){return false;}
}
}
StationAdmissionService::StationAdmissionService(StationAdmissionStore& store,StationAdmissionBoundary& boundary):store_(store),boundary_(boundary){}
void StationAdmissionService::owner() const {if(QThread::currentThread()!=owner_)throw std::runtime_error("Admission owner thread required");}
CommunicationV4Frame StationAdmissionService::trigger(const V4RequestIdentity& key,const CommunicationV4Frame& f,StationTriggerOrigin origin){
    owner();C::encode(f,V4Direction::Request);
    if(f.header.kind!=10 || key.installation.isNull() || key.station.isNull() || key.peer!=f.header.peer
       || key.epoch!=f.header.epoch || key.request!=f.header.requestId)return response(f,10,"INVALID_REQUEST_IDENTITY");
    if(admitting_.contains(key.station))return response(f,2,"STATION_ADMITTING");
    admitting_.insert(key.station);const auto guard=qScopeGuard([&]{admitting_.remove(key.station);});
    const auto fingerprint=C::requestFingerprint(C::encode(f,V4Direction::Request));
    // Durable dedup precedes mutable mode/snapshot barriers, including Stop or
    // activation changes. Reconnect changes the session envelope only.
    try{
        if(const auto prior=store_.find(key)){
            if(prior->fingerprint!=fingerprint)return response(f,6,"REQUEST_CONFLICT");
            if(!recordMatches(*prior,f))return response(f,8,"INVALID_DURABLE_RECORD");
            auto replay=prior->replay;replay.header.session=f.header.session;return replay;
        }
    }catch(...){return response(f,8,"PENDING_STORAGE");}
    if(const auto active=active_.constFind(key.station);active!=active_.cend())return response(f,active->blockedReason?active->blockedReason:2,active->blockedReason==8?"PENDING_STORAGE":active->blockedReason==9?"RECOVERY_REQUIRED":"STATION_BUSY");
    if(active_.size()>=32)return response(f,23,"STATION_LIMIT");
    try{
        const auto snapshot=boundary_.snapshot(key.station);
        if(!snapshot || !snapshot->station || !snapshot->templateKey || snapshot->cameraSetRevision.isNull())return response(f,20,"MISSING_MANIFEST");
        const auto& s=*snapshot->station;
        if(s.station.id!=key.station || s.station.key!=f.header.stationKey)return response(f,10,"UNKNOWN_STATION");
        if(s.activation.isNull() || !s.selection.recipeKey || s.selection.revision.isNull())return response(f,20,"INVALID_ACTIVE_SNAPSHOT");
        if(s.activation!=f.header.activation || s.selection.recipeKey!=f.header.recipeRevisionKey || snapshot->templateKey!=f.header.templateRevisionKey
           || snapshot->cameraSetRevision!=uuid(f.body,0) || snapshot->inputSnapshot!=uuid(f.body,16) || snapshot->inputRevision!=uuid(f.body,32))return response(f,4,"STALE_SNAPSHOT");
        if(snapshot->inputSnapshot.isNull()!=snapshot->inputRevision.isNull() || snapshot->inputSnapshot.isNull()==bool(snapshot->inputValues))return response(f,20,"INVALID_INPUT_MANIFEST");
        auto barriers=boundary_.barriers(key.station);
        if(auto reason=gate(barriers,origin))return response(f,reason,detail(barriers));
        auto reservation=boundary_.reserve(*snapshot);
        if(reservation.reason)return response(f,reservation.reason,"RESOURCE_RESERVATION");
        if(!reservation.lease)return response(f,23,"MISSING_RESOURCE_LEASE");
        // A resource adapter can call local Stop or update a manifest; no stale
        // permission survives that callback. No partial reservation is retained.
        const auto current=boundary_.snapshot(key.station);
        if(!current || !same(*snapshot,*current))return response(f,4,"SNAPSHOT_CHANGED");
        barriers=boundary_.barriers(key.station);
        if(auto reason=gate(barriers,origin))return response(f,reason,detail(barriers));
        StationAdmissionIntent intent{key,fingerprint,response(f,0,"",true),*snapshot};
        active_.insert(key.station,{key,fingerprint,intent.accepted,std::move(reservation.lease)});
        try{store_.commit(intent);}catch(...){active_[key.station].lease.reset();active_[key.station].blockedReason=8;return response(f,8,"PENDING_STORAGE");}
        try{boundary_.begin(intent,active_[key.station].lease);}catch(...){active_[key.station].blockedReason=9;return response(f,9,"RECOVERY_REQUIRED");}
        return intent.accepted;
    }catch(...){return response(f,20,"APPLICATION_NOT_READY");}
}
bool StationAdmissionService::release(const V4RequestIdentity& key){
    owner();if(admitting_.contains(key.station))return false;
    const auto active=active_.constFind(key.station);if(active==active_.cend() || active->identity!=key)return false;
    try{const auto record=store_.find(key);if(!record || record->replay.header.kind!=111 || !boundary_.drained(key.station))return false;
        if(record->fingerprint!=active->fingerprint || !recordMatches(*record,active->accepted))return false;
        active_.remove(key.station);return true;
    }catch(...){return false;}
}
}
