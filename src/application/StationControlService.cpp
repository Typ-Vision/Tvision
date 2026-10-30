#include "StationControlService.hpp"
#include <QScopeGuard>
#include <algorithm>
#include <stdexcept>

namespace visionflow {
namespace {
using C=CommunicationV4Codec;
CommunicationV4Frame response(const CommunicationV4Frame& f,quint32 kind,quint32 reason){
    CommunicationV4Frame r{f.header,QByteArray(896,0)};r.header.kind=kind;
    C::putUInt32(r.body,76,reason);C::putUInt32(r.body,100,kind==110?1:0);C::putUInt32(r.body,824,f.header.kind);return r;
}
bool headerEqual(const CommunicationV4Header& a,const CommunicationV4Header& b){
    return a.stationKey==b.stationKey && a.requestId==b.requestId && a.peer==b.peer && a.epoch==b.epoch
        && a.recipeRevisionKey==b.recipeRevisionKey && a.templateRevisionKey==b.templateRevisionKey && a.activation==b.activation;
}
bool valid(const StationControlRecord& r,const CommunicationV4Frame& request){
    try{
        if(C::requestFingerprint(C::encode(r.request,V4Direction::Request))!=r.fingerprint)return false;
        if(r.fingerprint!=C::requestFingerprint(C::encode(request,V4Direction::Request)))return false;
        C::encode(r.replay,V4Direction::Response);
        const auto& a=r.request.header;const auto& b=r.replay.header;
        if((b.kind!=110 && b.kind!=111 && !(b.kind==190 && C::uint32(r.replay.body,76)==9)) || b.stationKey!=a.stationKey || b.requestId!=a.requestId || b.peer!=a.peer || b.epoch!=a.epoch
            || C::uint32(r.replay.body,824)!=a.kind)return false;
        // SELECT terminal intentionally has a NEW activation/header. Its old
        // request is retained separately for dedup rather than compared to it.
        if(a.kind==4 && b.kind==111 && !C::uint32(r.replay.body,76))return !b.activation.isNull() && b.activation!=a.activation
            && b.recipeRevisionKey==C::uint32(r.request.body,0) && b.templateRevisionKey==C::uint32(r.request.body,4);
        return headerEqual(a,b);
    }catch(...){return false;}
}
}
StationControlService::StationControlService(StationActivationService& a,StationIdentityStore& i,StationControlStore* s,SavedRecipes recipes)
    :activations_(a),identities_(i),store_(s),recipes_(std::move(recipes)){}
void StationControlService::owner() const {if(QThread::currentThread()!=owner_)throw std::runtime_error("Control owner thread required");}
StationControlService::Binding& StationControlService::binding(const QUuid& id){
    auto b=bindings_.find(id);if(b==bindings_.end())throw std::runtime_error("Unknown station control binding");return b.value();
}
void StationControlService::bind(const StationDefinition& s,StationRunController& run){
    owner();if(s.id!=run.station() || !s.key || bindings_.contains(s.id) || bindings_.size()>=32)throw std::runtime_error("Invalid control binding");
    for(const auto& b:bindings_)if(b.station.key==s.key)throw std::runtime_error("Duplicate station key");
    bindings_.insert(s.id,{s,&run});
}
quint32 StationControlService::templateKey(const StationSnapshot& s) const {
    const auto saved=recipes_?recipes_(s.selection.recipeKey):std::nullopt;
    if(!saved || !saved->templateKey || saved->selection.revision!=s.selection.revision || stationRecipeBytes(saved->selection)!=stationRecipeBytes(s.selection))throw std::runtime_error("Missing saved manifest");
    return saved->templateKey;
}
bool StationControlService::exact(const QUuid& id,const CommunicationV4Header& h) const {
    const auto s=activations_.snapshot(id);
    return s?(s->station.key==h.stationKey && s->activation==h.activation && s->selection.recipeKey==h.recipeRevisionKey && templateKey(*s)==h.templateRevisionKey)
        :(!h.recipeRevisionKey && !h.templateRevisionKey && h.activation.isNull());
}
quint32 StationControlService::peerReason(const V4RequestIdentity& key) const {
    const auto c=identities_.load();validateIdentityCatalogue(c);
    if(c.installation!=key.installation)return 24;
    if(std::count_if(c.epochs.begin(),c.epochs.end(),[](const auto& e){return !e.retired;})!=1)return 24;
    for(const auto& e:c.epochs)if(e.peer==key.peer && e.epoch==key.epoch)return e.retired?19:0;
    return 24;
}
void StationControlService::setRemoteEnabled(bool enabled){
    owner();remoteEnabled_=enabled;
    if(!enabled)for(auto& b:bindings_){
        // Disable fences every late preparation callback through the SAME Stop
        // generation used by local operators; pending terminals remain durable.
        b.run->stop();
    }
}
std::shared_ptr<const StationSnapshot> StationControlService::select(const QUuid& id,const QUuid& expected,quint32 recipeKey,quint32 wantedTemplate){
    owner();auto& b=binding(id);
    if(!store_ || blocked_.contains(id))throw std::runtime_error("Durable control unavailable");
    const auto saved=recipes_?recipes_(recipeKey):std::nullopt;
    if(!saved || !wantedTemplate || saved->templateKey!=wantedTemplate)throw std::runtime_error("Unknown immutable saved recipe/template");
    const auto result=activations_.activate(b.station,expected,saved->selection);
    b.run->setAuto(false);return result;
}
void StationControlService::setAuto(const QUuid& id,const QUuid& expected,bool value){
    owner();auto& b=binding(id);const auto s=activations_.snapshot(id);
    if(!s || s->activation!=expected)throw std::runtime_error("STALE_ACTIVATION");
    if(!value){b.run->setAuto(false);localAuto_.remove(id);return;}
    prepareAuto(id,expected);localAuto_[id]=b.run->controlGeneration();
}
void StationControlService::prepareAuto(const QUuid& id,const QUuid& expected){
    auto& b=binding(id);const auto s=activations_.snapshot(id);
    if(!s || s->activation!=expected)throw std::runtime_error("STALE_ACTIVATION");
    if(!store_ || blocked_.contains(id))throw std::runtime_error("Durable control unavailable");
    templateKey(*s);
    const auto mode=b.run->state().mode;
    if((mode!=StationMode::ArmedManual && mode!=StationMode::ArmedAuto) || b.run->activation()!=expected)b.run->run(s);
}
void StationControlService::stop(const QUuid& id){owner();binding(id).run->stop();}
void StationControlService::finish(const Pending& p,quint32 reason,bool canceled){
    auto r=response(p.request,110,reason);auto raw=C::encode(r,V4Direction::Response);
    r.header.kind=111;
    if(p.selected && !reason){r.header.activation=p.selected->activation;r.header.recipeRevisionKey=p.selected->recipeRevisionKey;r.header.templateRevisionKey=p.selected->templateRevisionKey;}
    r.body.replace(0,16,QUuid::createUuid().toRfc4122());C::putUInt32(r.body,64,canceled?3:reason?2:1);C::putUInt32(r.body,72,reason?0:1);C::putUInt32(r.body,100,3);
    const auto mode=binding(p.key.station).run->state().mode;
    C::putUInt32(r.body,756,!reason && (mode==StationMode::ArmedManual || mode==StationMode::ArmedAuto));
    C::putUInt32(raw,8,111);C::putUInt32(raw,80,r.header.recipeRevisionKey);C::putUInt32(raw,84,r.header.templateRevisionKey);raw.replace(88,16,r.header.activation.toRfc4122());raw.replace(128,896,r.body);
    r.body.replace(32,32,C::terminalDigest(raw));C::encode(r,V4Direction::Response);
    store_->seal(p.key,r);
}
std::optional<CommunicationV4Frame> StationControlService::handle(const V4RequestIdentity& key,const CommunicationV4Frame& f){
    owner();C::encode(f,V4Direction::Request);const auto kind=f.header.kind;
    if(kind!=4 && kind!=5 && kind!=6 && kind!=11)return {};
    auto reject=[&](quint32 reason){return response(f,190,reason);};
    if(key.station.isNull() || key.request!=f.header.requestId || key.peer!=f.header.peer || key.epoch!=f.header.epoch
        || !bindings_.contains(key.station) || bindings_[key.station].station.key!=f.header.stationKey)return reject(10);
    try{if(auto reason=peerReason(key))return reject(reason);}catch(...){return reject(8);}
    if(!store_){if(kind==6 && remoteEnabled_ && exact(key.station,f.header))stop(key.station);return kind==11?std::nullopt:std::optional(reject(8));}
    const auto fingerprint=C::requestFingerprint(C::encode(f,V4Direction::Request));
    try{
        if(const auto old=store_->find(key)){
            if(kind==11){
                if(old->request.header.kind!=4 && old->request.header.kind!=5 && old->request.header.kind!=6)return {};
                const auto& t=old->replay;
                if(t.header.kind!=111 || !headerEqual(t.header,f.header) || f.body.first(16)!=t.body.first(16) || f.body.sliced(16,32)!=t.body.sliced(32,32))return reject(14);
                C::encode(t,V4Direction::Response);store_->acknowledge(key,f);
                auto r=response(f,112,0);r.body.replace(0,16,t.body.first(16));r.body.replace(32,32,t.body.sliced(32,32));C::putUInt32(r.body,100,4);C::putUInt32(r.body,824,C::uint32(t.body,824));return r;
            }
            if(old->fingerprint!=fingerprint)return reject(6);
            if(!valid(*old,f)){blocked_.insert(key.station);stop(key.station);return reject(8);}
            auto r=old->replay;r.header.session=f.header.session;return r;
        }
    }catch(...){blocked_.insert(key.station);stop(key.station);return reject(8);}
    if(kind==11)return {}; // another ledger owns this ACK
    if(!remoteEnabled_)return reject(25);
    if(dispatching_.contains(key.station))return reject(2);
    dispatching_.insert(key.station);const auto guard=qScopeGuard([&]{dispatching_.remove(key.station);});
    try{
        if(!exact(key.station,f.header))return reject(4);
        auto& run=*binding(key.station).run;
        // STOP closes admission before checking the bounded reserve, including
        // a second distinct STOP while the first control terminal awaits ACK.
        if(kind==6)stop(key.station);
        if(kind==5 && !C::uint32(f.body,0))setAuto(key.station,f.header.activation,false);
        if(blocked_.contains(key.station))return reject(8);
        if(kind==4 && !run.drained())return reject(2);
        if(kind==5 && C::uint32(f.body,0) && (run.state().mode==StationMode::Running || run.state().mode==StationMode::Draining || run.state().mode==StationMode::Preparing))return reject(2);
        if(kind==4){const auto saved=recipes_?recipes_(C::uint32(f.body,0)):std::nullopt;if(!saved || !C::uint32(f.body,4) || saved->templateKey!=C::uint32(f.body,4))return reject(4);}
        StationControlIntent intent{key,{f,response(f,110,0),fingerprint,false},kind==6};
        try{if(auto reason=store_->reserve(intent))return reject(reason);}catch(...){blocked_.insert(key.station);stop(key.station);return reject(8);}
        Pending p{key,f,run.controlGeneration(),kind==5 && bool(C::uint32(f.body,0))};
        try{
            if(kind==4){const auto s=select(key.station,f.header.activation,C::uint32(f.body,0),C::uint32(f.body,4));p.selected=f.header;p.selected->activation=s->activation;p.selected->recipeRevisionKey=s->selection.recipeKey;p.selected->templateRevisionKey=templateKey(*s);}
            else if(kind==5 && p.automatic){
                // Do not enable Auto until a durable READY terminal is sealed.
                prepareAuto(key.station,f.header.activation);
            }
            p.generation=run.controlGeneration();pending_.append(p);
        }catch(...){stop(key.station);try{finish(p,20);}catch(...){blocked_.insert(key.station);}}
        return intent.record.replay;
    }catch(...){return reject(20);}
}
void StationControlService::poll(){
    owner();for(auto& b:bindings_)b.run->poll();
    for(auto it=localAuto_.begin();it!=localAuto_.end();){
        auto& run=*binding(it.key()).run;
        if(run.controlGeneration()!=it.value() || blocked_.contains(it.key())){it=localAuto_.erase(it);continue;}
        const auto mode=run.state().mode;
        if(mode==StationMode::Preparing){++it;continue;}
        if(mode==StationMode::ArmedManual || mode==StationMode::ArmedAuto)run.setAuto(true);
        it=localAuto_.erase(it);
    }
    for(qsizetype n=0;n<pending_.size();){
        const auto p=pending_[n];auto& run=*binding(p.key.station).run;
        if(blocked_.contains(p.key.station)){++n;continue;}
        const bool canceled=p.automatic && run.controlGeneration()!=p.generation;
        const auto mode=run.state().mode;
        if(p.request.header.kind==6 && !run.drained()){++n;continue;}
        if(p.automatic && !canceled && mode==StationMode::Preparing){++n;continue;}
        const quint32 reason=canceled?20:p.automatic && mode!=StationMode::ArmedManual && mode!=StationMode::ArmedAuto?20:0;
        try{
            finish(p,reason,canceled);
            if(p.automatic && !reason && run.controlGeneration()==p.generation)run.setAuto(true);
        }catch(...){blocked_.insert(p.key.station);run.stop();}
        pending_.removeAt(n);
    }
}
}
