#include "StationDeliveryScheduler.hpp"
#include <stdexcept>

namespace visionflow {
QString StationDeliveryScheduler::identity(const V4RequestIdentity& k){
    return k.installation.toString()+k.peer.toString()+k.epoch.toString()+k.station.toString()+QString::number(k.request);
}
bool StationDeliveryScheduler::add(const CommunicationV4Frame& f,const QString& delivery){
    const auto raw=CommunicationV4Codec::encode(f,V4Direction::Response);
    if(count_>=maxFrames || bytes_+raw.size()>highWaterBytes)return false;
    auto& lane=lanes_[f.header.stationKey];
    if(lane.empty())ready_.push_back(f.header.stationKey);
    lane.push_back({raw,delivery});++count_;bytes_+=raw.size();return true;
}
bool StationDeliveryScheduler::enqueue(const CommunicationV4Frame& f){return add(f);}
bool StationDeliveryScheduler::canRead(qint64 socketBytes) const {
    // Reserve a complete four-request dispatch batch before admitting any work.
    return socketBytes>=0 && count_<=maxFrames-4 && bytes_+socketBytes+4096<=highWaterBytes;
}
QByteArrayView StationDeliveryScheduler::pendingBytes(qint64 socketBytes){
    if(socketBytes<0 || socketBytes>=highWaterBytes)return {};
    if(!active_ && !ready_.empty()){
        const auto station=ready_.front();ready_.pop_front();auto it=lanes_.find(station);
        active_=std::move(it.value().front());it.value().pop_front();
        if(it.value().empty())lanes_.erase(it);else ready_.push_back(station);
    }
    if(!active_)return {};
    return QByteArrayView(active_->bytes).sliced(offset_).first(qMin<qsizetype>(active_->bytes.size()-offset_,highWaterBytes-socketBytes));
}
void StationDeliveryScheduler::consumeWritten(qsizetype n,qint64 now){
    if(!active_ || n<0 || n>active_->bytes.size()-offset_)throw std::runtime_error("Invalid scheduler write progress");
    offset_+=n;bytes_-=n;
    if(offset_==active_->bytes.size()){
        if(auto it=attempts_.find(active_->delivery);it!=attempts_.end()){++it->sent;it->last=now;it->queued=false;}
        active_.reset();offset_=0;--count_;
    }
}
void StationDeliveryScheduler::tick(qint64 now,const QUuid& session,const Authorized& authorized,qint64 socketBytes){
    bindSession(session);
    if(!ledger_ || session.isNull())return;
    QSet<QString> pending;
    for(const auto& r:ledger_->recovery()){
        if(r.value.acknowledged || r.value.replay.header.kind!=111)continue;
        auto probe=r.value.request;probe.header.kind=12;probe.header.session=session;probe.body=QByteArray(384,0);
        if(!authorized(probe))continue;
        const auto id=identity(r.key);pending.insert(id);
        auto it=attempts_.find(id);
        if(it==attempts_.end()){
            if(attempts_.size()>=maxFrames)throw std::runtime_error("Delivery attempt limit");
            it=attempts_.insert(id,{r.key});
        }
        auto& a=it.value();
        if(a.queued || a.alarm || (a.sent && now-a.last<retryMs))continue;
        if(a.sent>maxRetries){a.alarm=true;continue;}
        auto frame=r.value.replay;frame.header.session=session;
        if(socketBytes>=0 && bytes_+socketBytes+1024<=highWaterBytes && add(frame,id))a.queued=true;
    }
    for(auto it=attempts_.begin();it!=attempts_.end();)if(!pending.contains(it.key()))it=attempts_.erase(it);else ++it;
}
bool StationDeliveryScheduler::timedOut(const V4RequestIdentity& k) const {const auto it=attempts_.constFind(identity(k));return it!=attempts_.cend() && it->alarm;}
void StationDeliveryScheduler::bindSession(const QUuid& session){
    if(session==session_)return;
    // A new HELLO cannot splice a new envelope into a partially written frame.
    if(offset_)throw std::runtime_error("Session changed during partial frame; reconnect required");
    reset();session_=session;
}
void StationDeliveryScheduler::reset(){lanes_.clear();ready_.clear();active_.reset();offset_=bytes_=count_=0;attempts_.clear();session_=QUuid{};}
}
