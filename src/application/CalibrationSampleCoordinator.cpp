#include "CalibrationSampleCoordinator.hpp"
#include "ProductionStore.hpp"
#include "vision/ImageAccess.hpp"
#include <limits>
#include <stdexcept>

namespace visionflow {
namespace {
using C=CommunicationV4Codec;
QUuid uuid(QByteArrayView b,qsizetype n){return QUuid::fromRfc4122(b.sliced(n,16));}
bool sameSnapshot(const CommunicationV4Header& a,const CommunicationV4Header& b){
    return a.stationKey==b.stationKey && a.recipeRevisionKey==b.recipeRevisionKey
        && a.templateRevisionKey==b.templateRevisionKey && a.activation==b.activation;
}
}
CalibrationSampleCoordinator::CalibrationSampleCoordinator(RobotCalibrationSessionService& s,
    CalibrationDependencyProvider& d,StationDeliveryLedger& l,bool fixture)
    :sessions_(s),dependencies_(d),ledger_(l),softwareFixture_(fixture){
    clock_.start();
    // Archive records never evict. Counting every durable request gives a
    // strictly increasing local token across coordinator/process restarts.
    const auto records=ledger_.recovery();nextToken_=quint64(records.size())+1;
    for(const auto& r:records)if(r.value.request.header.kind==21 && r.value.replay.header.kind!=111){
        const auto session=uuid(r.value.request.body,0);fenced_.insert(session);
        Pending p;p.job.key_=r.key;p.job.session_=session;p.job.sample_=uuid(r.value.request.body,16);
        p.request=r.value.request;p.fenced=true;restored_.insert(session,p);
    }
}
void CalibrationSampleCoordinator::owner() const {
    if(QThread::currentThread()!=owner_)throw std::runtime_error("Sample coordinator owner thread required");
}
CommunicationV4Frame CalibrationSampleCoordinator::response(const CommunicationV4Frame& f,quint32 kind,quint32 reason) const {
    CommunicationV4Frame r{f.header,QByteArray(896,0)};r.header.kind=kind;
    C::putUInt32(r.body,76,reason);C::putUInt32(r.body,100,kind==110?1:0);
    if(f.body.size()==384){r.body.replace(104,16,f.body.first(16));r.body.replace(120,16,f.body.sliced(16,16));C::putUInt32(r.body,820,C::uint32(f.body,32));}
    C::putUInt32(r.body,824,f.header.kind);return r;
}
CalibrationSampleCoordinator::Admission CalibrationSampleCoordinator::sample(const V4RequestIdentity& key,
    const CommunicationV4Frame& f,IndependentCaptureRequest captureRequest){
    owner();auto reject=[&](quint32 n){return Admission{response(f,190,n),{}};};
    if(f.header.kind!=21 || key.installation.isNull() || key.station.isNull()
        || key.peer!=f.header.peer || key.epoch!=f.header.epoch || key.request!=f.header.requestId)return reject(11);
    QByteArray fingerprint;
    try{fingerprint=C::requestFingerprint(C::encode(f,V4Direction::Request));}catch(...){return reject(13);}
    const auto session=uuid(f.body,0),sampleId=uuid(f.body,16);
    try{
        if(auto prior=ledger_.inspect(key)){
            if(prior->value.fingerprint!=fingerprint)return reject(6);
            return {ledger_.replay(key,f.header.session),{}};
        }
        // A sample UUID cannot be rebound to a different command, pose, peer or
        // role, even after exact ACK has freed the station reservation.
        for(const auto& r:ledger_.recovery())if(r.value.request.header.kind==21
            && uuid(r.value.request.body,0)==session && uuid(r.value.request.body,16)==sampleId)return reject(6);
    }catch(...){return reject(8);}
    const auto record=sessions_.session(session);
    const auto generation=sessions_.admitCapture(session);
    if(!record || !generation || record->beginCommand.installation!=key.installation
        || record->beginCommand.station!=key.station || record->beginCommand.peer!=key.peer
        || record->beginCommand.epoch!=key.epoch)return reject(5);
    if(fenced_.contains(session))return reject(9);
    if(pending_.contains(session))return reject(2);
    const auto& d=record->dependencies;
    try{
        const auto current=dependencies_.current(key.station,d.cameraKey);
        if(!current || *current!=d)return reject(4);
        if(record->beginRequest.isEmpty())return reject(9);
        const auto begin=C::decode(record->beginRequest,V4Direction::Request);
        if(C::requestFingerprint(record->beginRequest)!=record->fingerprint
            || !sameSnapshot(begin.header,f.header))return reject(4);
        if(C::uint32(f.body,32)!=d.cameraKey || uuid(f.body,40)!=d.cameraRevision
            || C::uint32(f.body,208)!=d.acquisitionProfileRevisionKey
            || uuid(f.body,136)!=d.pose.targetFrame || uuid(f.body,152)!=d.pose.targetFrameRevision
            || uuid(f.body,168)!=d.pose.sourceFrame || uuid(f.body,184)!=d.pose.sourceFrameRevision)return reject(4);
        // Wire's generic base_T_flange is already the controller adapter's output.
        // TCP input requires the future authoritative offset resolver; no silent conversion.
        if(C::uint32(f.body,76)!=1)return reject(24);
        if(record->state==CalibrationState::Candidate && C::uint32(f.body,212)!=2)return reject(11);
        // SAMPLE cannot inherit the wizard's automatic experimental OECS route.
        if(captureRequest.policy==CalibrationCapturePolicy::ExperimentalOecs)return reject(17);
        captureRequest.policy=CalibrationCapturePolicy::Trusted;
        captureRequest.validate();
        if(captureRequest.origin!=SourceOrigin::CameraAcquired || !captureRequest.camera
            || captureRequest.camera->id.value!=d.cameraId)return reject(4);
        if(!softwareFixture_ && (!captureRequest.camera->physical || *captureRequest.camera->physical!=d.physical
            || captureRequest.policy==CalibrationCapturePolicy::ExperimentalOecs))return reject(17);
    }catch(...){return reject(11);}
    auto accepted=response(f,110,0);
    try{
        const auto reason=ledger_.reserve({key,{f,accepted,fingerprint,false},false});
        if(reason)return reject(reason);
    }catch(...){fenced_.insert(session);return reject(8);}
    Pending p;p.request=f;p.capture=std::make_shared<IndependentCalibrationSession>();
    p.job.key_=key;p.job.session_=session;p.job.sample_=sampleId;
    p.job.generation_=*generation;p.job.cameraSession_=QUuid::createUuid();
    if(nextToken_==std::numeric_limits<quint64>::max()){fenced_.insert(session);return reject(23);}
    p.job.token_=nextToken_++;
    try{
        ProductionStore(ledger_.directory()).append("calibration_sample",p.job.cameraSession_.toString(QUuid::WithoutBraces),
            {{"format","visionflow.calibration-sample/1"},{"token",QString::number(p.job.token_)},
             {"cameraSession",p.job.cameraSession_.toString(QUuid::WithoutBraces)},
             {"session",session.toString(QUuid::WithoutBraces)},{"sample",sampleId.toString(QUuid::WithoutBraces)},
             {"request",QString::fromLatin1(C::encode(f,V4Direction::Request).toBase64())}});
        // Session state commit follows durable intent and precedes any hardware.
        if(!sessions_.collect(session))throw std::runtime_error("Session cannot collect");
        p.job.capture_=p.capture->begin(std::move(captureRequest),sessions_.captureLease(session));
    }catch(...){
        p.fenced=true;pending_.insert(session,p);fenced_.insert(session);return reject(9);
    }
    pending_.insert(session,p);return {accepted,p.job};
}
IndependentCaptureResult CalibrationSampleCoordinator::run(const Job& j,const IndependentCaptureBackend& b){
    if(!j.capture_)throw std::runtime_error("Missing sample capture job");
    return IndependentCalibrationSession::run(*j.capture_,b);
}
CommunicationV4Frame CalibrationSampleCoordinator::finish(Pending& p,const QUuid& frame,quint32 reason){
    auto r=response(p.request,110,reason);auto raw=C::encode(r,V4Direction::Response);r.header.kind=111;
    r.body.replace(0,16,QUuid::createUuid().toRfc4122());r.body.replace(136,16,frame.toRfc4122());
    C::putUInt32(r.body,64,reason?2:1);C::putUInt32(r.body,72,reason?0:1);
    C::putUInt32(r.body,100,reason?7:6);
    C::putUInt64(r.body,748,frame.isNull()?0:quint64(clock_.nsecsElapsed()));
    C::putUInt32(raw,8,111);raw.replace(128,896,r.body);
    r.body.replace(32,32,C::terminalDigest(raw));C::encode(r,V4Direction::Response);
    if(restored_.contains(p.job.session_))ledger_.recoverSampleFailure(p.job.key_,r);
    else ledger_.seal(p.job.key_,r);
    return r;
}
std::optional<CommunicationV4Frame> CalibrationSampleCoordinator::complete(const Job& j,const IndependentCaptureResult& result){
    owner();auto it=pending_.find(j.session_);
    if(it==pending_.end() || it->fenced || fenced_.contains(j.session_)
        || it->job.token_!=j.token_ || it->job.cameraSession_!=j.cameraSession_
        || it->job.sample_!=j.sample_ || !sessions_.acceptCaptureCallback(j.session_,j.generation_))return {};
    // Independent session validates its own opaque ticket and generation. A
    // foreign result must not become this sample's failure or consume its ticket.
    if(!it->capture->matches(result))return {};
    const bool success=it->capture->accept(result);
    if(!success){timeout(j.session_);return {};}
    const auto frame=QUuid(it->capture->evidence()->frameId);
    if(frame.isNull()){timeout(j.session_);return {};}
    try{
        const auto image=*it->capture->math().frame();
        const auto pixels=ImageAccess(image).ownedCopy();
        const qsizetype size=qsizetype(pixels.total()*pixels.elemSize());
        if(size<=0 || size>12*1024*1024)throw std::runtime_error("Sample raster exceeds bounded archive");
        const auto raster=QByteArray(reinterpret_cast<const char*>(pixels.data),size).toBase64();
        ProductionStore(ledger_.directory()).append("calibration_sample",frame.toString(QUuid::WithoutBraces),
            {{"format","visionflow.calibration-sample-frame/1"},{"token",QString::number(j.token_)},
             {"cameraSession",j.cameraSession_.toString(QUuid::WithoutBraces)},
             {"session",j.session_.toString(QUuid::WithoutBraces)},{"sample",j.sample_.toString(QUuid::WithoutBraces)},
             {"frame",frame.toString(QUuid::WithoutBraces)},{"pixelHash",it->capture->evidence()->pixelHash},
             {"width",image.size().width()},{"height",image.size().height()},
             {"stride",int(pixels.step)},{"pixelFormat",int(image.format())},
             {"rasterBase64",QString::fromLatin1(raster)},
             {"role",int(C::uint32(it->request.body,212))}});
        auto terminal=finish(*it,frame,0);pending_.erase(it);return terminal;
    }
    catch(...){timeout(j.session_);return {};}
}
bool CalibrationSampleCoordinator::timeout(const QUuid& session){
    owner();auto it=pending_.find(session);if(it==pending_.end())return false;
    it->fenced=true;fenced_.insert(session);if(it->capture)it->capture->cancel();return true;
}
std::optional<CommunicationV4Frame> CalibrationSampleCoordinator::recover(const QUuid& session,CalibrationDrainPort& drain){
    owner();
    if(!pending_.contains(session) && restored_.contains(session))pending_.insert(session,restored_[session]);
    auto it=pending_.find(session);if(it==pending_.end() || !it->fenced)return {};
    const auto record=sessions_.session(session);if(!record)return {};
    try{
        if(!drain.drain(session,record->dependencies.cameraId))return {};
        // A crash/fault may have happened after the immutable terminal commit.
        // Preserve it byte-for-byte rather than replacing success with failure.
        const auto prior=ledger_.inspect(it->job.key_);
        if(prior && prior->value.replay.header.kind==111){
            auto r=prior->value.replay;pending_.erase(it);restored_.remove(session);fenced_.remove(session);return r;
        }
        auto r=finish(*it,{},21);pending_.erase(it);restored_.remove(session);fenced_.remove(session);return r;
    }catch(...){return {};}
}
}
