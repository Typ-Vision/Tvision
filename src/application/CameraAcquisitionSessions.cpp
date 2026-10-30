#include "CameraAcquisitionSessions.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <set>

namespace visionflow {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QString physicalKey(const IndustrialCameraRequest& r){
    require(r.camera.physical && r.camera.physical->usable(),"Exact physical camera required");const auto& p=*r.camera.physical;
    return QString::fromLatin1(QJsonDocument(QJsonArray{p.vendor,p.model,p.serial}).toJson(QJsonDocument::Compact).toHex());
}
QByteArray declaration(const IndustrialCameraRequest& r){
    auto c=r.camera.toJson(),s=r.setup.toJson(),p=r.profile.toJson();c.remove("name");s.remove("name");p.remove("name");
    return QJsonDocument(QJsonObject{{"camera",c},{"setup",s},{"profile",p}}).toJson(QJsonDocument::Compact);
}
std::optional<TransportLocator> routeFor(const IndustrialExecutionInputs& in,const IndustrialCameraRequest& r){
    if(in.routes.contains(r.camera.id.value))return in.routes[r.camera.id.value];return {};
}
void validateTargets(const IndustrialExecutionInputs& in,const QVector<IndustrialCameraRequest>& requests,bool experimental){
    in.contract.validate();
    require(in.leaseUse==CameraLeaseUse::Editor || in.leaseUse==CameraLeaseUse::Runtime || in.leaseUse==CameraLeaseUse::Calibration,"Invalid acquisition context use");
    require(!experimental || (in.leaseUse!=CameraLeaseUse::Runtime && in.admittedDevices.isEmpty()),"Experimental context cannot carry production admission");
    QSet<QString> sources;QMap<QString,QByteArray> identities;
    for(const auto& r:requests){
        r.camera.validate();r.setup.validate();r.profile.validate();
        require(!r.sourceId.isEmpty() && !sources.contains(r.sourceId),"Missing/duplicate session source");sources.insert(r.sourceId);
        require(in.contract.cameras.contains(r.camera) && in.contract.setups.contains(r.setup) && in.contract.profiles.contains(r.profile),"Session target differs from frozen contract");
        require(r.setup.geometry.complete && r.setup.geometry.critical() && r.profile.requested.trigger.enabled && r.profile.requested.trigger.source=="Software","Complete software acquisition target required");
        (void)r.profile.acquisitionId(r.setup);
        const auto key=physicalKey(r);const auto config=declaration(r);
        require(!identities.contains(key) || identities[key]==config,"Conflicting profiles for the same physical camera");identities[key]=config;
        if(const auto hint=routeFor(in,r))hint->validate();
    }
}
}
bool AcquisitionContextStatus::allReady() const {
    if(preparing || closing)return false;for(const auto& source:sources)if(source.state!=AcquisitionSessionState::Ready)return false;return true;
}
struct CameraAcquisitionSessions::Core {
    struct SharedContext {
        AcquisitionContextStatus status;
        CameraLeaseUse use=CameraLeaseUse::Editor;
        std::shared_ptr<std::atomic_bool> cancel=std::make_shared<std::atomic_bool>(false);
        std::set<QString> busy;
        std::optional<Deadline> closeDeadline;
    };
    struct Entry {
        IndustrialCameraRequest request;
        QByteArray config;
        std::optional<TransportLocator> hint;
        CameraProviderFactory factory;
        bool experimental=false,closing=false;
        std::shared_ptr<const AcquisitionOecsBackend> backend;
        std::unique_ptr<CameraWorkerSession> worker;
    };
    struct Scope {
        std::shared_ptr<SharedContext> shared;
        std::map<QString,std::unique_ptr<Entry>> entries;
        std::vector<std::shared_ptr<std::promise<AcquisitionContextClose>>> closeWaiters;
        bool closeStarted=false,clean=true;
        QString error;
    };
    mutable std::mutex mutex;
    std::condition_variable wake;
    std::deque<std::function<void()>> jobs;
    QMap<QString,std::shared_ptr<SharedContext>> contexts;
    std::map<QString,Scope> scopes; // coordinator only, including all worker access
    bool stopping=false;
    std::promise<AcquisitionContextClose> shutdownPromise;
    std::shared_future<AcquisitionContextClose> shutdownFuture=shutdownPromise.get_future().share();

    template<class T,class Work> std::future<T> postLocked(Work work){
        auto promise=std::make_shared<std::promise<T>>();auto future=promise->get_future();
        jobs.push_back([promise,work=std::move(work)]() mutable {try{promise->set_value(work());}catch(...){promise->set_exception(std::current_exception());}});
        wake.notify_one();return future;
    }
    std::shared_ptr<SharedContext> contextLocked(const QString& id){
        auto& shared=contexts[id];if(!shared){shared=std::make_shared<SharedContext>();shared->status.context=id;}return shared;
    }
    Scope& scope(const std::shared_ptr<SharedContext>& shared){auto& s=scopes[shared->status.context];s.shared=shared;return s;}
    AcquisitionContextStatus snapshotLocked(const SharedContext& c) const {
        auto out=c.status;
        out.closeBudgetExceeded=c.closeDeadline && !out.drained && c.closeDeadline->expired();
        if(out.closing)out.state=out.drained ? (out.clean?AcquisitionSessionState::Closed:AcquisitionSessionState::Fault):AcquisitionSessionState::Closing;
        else if(out.preparing)out.state=AcquisitionSessionState::Opening;
        else {out.state=out.sources.isEmpty()?AcquisitionSessionState::Closed:AcquisitionSessionState::Fault;
            for(const auto& source:out.sources)if(source.state==AcquisitionSessionState::Ready)out.state=AcquisitionSessionState::Ready;
            for(const auto& source:out.sources)if(source.state==AcquisitionSessionState::Capturing)out.state=AcquisitionSessionState::Capturing;}
        return out;
    }
    void live(const std::shared_ptr<SharedContext>& c,quint64 generation,const std::shared_ptr<std::atomic_bool>& token,
              Deadline deadline,const std::shared_ptr<std::atomic_bool>& cancel={}){
        require(!token->load() && !(cancel && cancel->load()) && !deadline.expired(),"Camera context cancelled/deadline exceeded");
        std::lock_guard lock(mutex);require(!stopping && c->status.generation==generation && !c->status.closing,"Retired camera context generation");
    }
    void phase(const std::shared_ptr<SharedContext>& c,quint64 generation,const QString& key,AcquisitionSessionState state,
               const QString& error={},const Entry* entry=nullptr){
        std::lock_guard lock(mutex);if(c->status.generation!=generation || c->status.closing)return;
        for(auto& source:c->status.sources)if(source.physicalKey==key){source.state=state;source.error=error;
            if(entry && entry->worker){source.request.device=entry->request.device;
                source.readback=entry->worker->readback();if(source.readback)source.acquisitionId=verifyApplied(source.request.setup,source.request.profile,*source.readback);}}
    }
    void retire(Entry& e,int budget=1000){if(!e.closing){e.closing=true;e.worker->requestShutdown(budget);}}
    void drainScope(Scope& s){
        for(auto it=s.entries.begin();it!=s.entries.end();){auto& e=*it->second;
            if(!e.closing || e.worker->shutdownState()!=ShutdownState::Ready){++it;continue;}
            if(e.worker->cleanShutdown()!=std::optional<bool>(true)){s.clean=false;s.error=e.worker->lifecycle().error();}
            e.worker.reset();it=s.entries.erase(it); // only after the hardware worker has exited/joined
        }
        if(s.closeStarted && s.entries.empty()){
            std::lock_guard lock(mutex);auto& out=s.shared->status;out.drained=true;out.clean=s.clean;s.shared->busy.clear();
            for(auto& source:out.sources){source.state=s.clean?AcquisitionSessionState::Closed:AcquisitionSessionState::Fault;source.error=s.error;}
            for(auto& waiter:s.closeWaiters)waiter->set_value({s.clean,s.error});s.closeWaiters.clear();s.closeStarted=false;
        }
    }
    void command(Entry& entry,CameraOperation op,CameraState expected,const std::shared_ptr<SharedContext>& c,
                 quint64 generation,const std::shared_ptr<std::atomic_bool>& token,Deadline deadline,
                 const std::shared_ptr<std::atomic_bool>& cancel={}){
        live(c,generation,token,deadline,cancel);entry.worker->submit(op,deadline.remainingMs());
        for(;;){live(c,generation,token,deadline,cancel);const auto accepted=entry.worker->poll();
            require(entry.worker->lifecycle().state()!=CameraState::Fault,entry.worker->lifecycle().error().toUtf8().constData());
            if(accepted && entry.worker->lifecycle().state()==expected)return;QThread::msleep(1);}
    }
    void waitRelease(Scope& s,const QString& key,const std::shared_ptr<SharedContext>& c,quint64 generation,
                     const std::shared_ptr<std::atomic_bool>& token,Deadline deadline){
        while(s.entries.contains(key)){drainScope(s);if(!s.entries.contains(key))break;live(c,generation,token,deadline);QThread::msleep(1);}
    }
    AcquisitionContextStatus prepare(const std::shared_ptr<SharedContext>& c,quint64 generation,
        const std::shared_ptr<std::atomic_bool>& token,const IndustrialExecutionInputs& in,
        const QVector<IndustrialCameraRequest>& requests,bool experimental,Deadline deadline,
        const std::shared_ptr<const AcquisitionOecsBackend>& backend){
        auto& s=scope(c);
        try {
            live(c,generation,token,deadline);
            QMap<QString,QVector<IndustrialCameraRequest>> groups;
            for(const auto& r:requests)groups[physicalKey(r)].append(r);
            QVector<CameraLeaseTarget> targets;QVector<CameraLeaseArbiter::Ticket> reusable;
            for(const auto& r:requests)targets.append({r.camera.id.value,*r.camera.physical});
            for(const auto& [key,e]:s.entries)if(!e->closing)reusable.append(e->worker->lease());
            auto reservation=CameraLeaseArbiter::reserve(c->status.context,in.leaseUse,targets,reusable);
            if(reservation.reason){
                for(const auto& key:groups.keys())phase(c,generation,key,AcquisitionSessionState::Fault,"CAMERA_BUSY");
                std::lock_guard lock(mutex);c->status.resourceReason=3;c->status.preparing=false;c->status.drained=s.entries.empty();return snapshotLocked(*c);
            }
            auto ticketFor=[&](const QString& key){for(const auto& ticket:reservation.leases)
                if(ticket->physicalKey()==key)return ticket;
                return CameraLeaseArbiter::Ticket{};};
            QStringList removed;
            for(auto& [key,e]:s.entries)if(!groups.contains(key)){retire(*e);removed.append(key);}
            for(const auto& key:removed)waitRelease(s,key,c,generation,token,deadline);
            for(auto group=groups.cbegin();group!=groups.cend();++group){
                const auto key=group.key();const auto target=group->first();
                try {
                    live(c,generation,token,deadline);const auto config=declaration(target);const auto hint=routeFor(in,target);
                    auto old=s.entries.find(key);
                    bool reuse=old!=s.entries.end() && !old->second->closing && old->second->config==config && old->second->hint==hint &&
                        old->second->experimental==experimental && old->second->factory.sameComposition(in.factory) && old->second->backend==backend;
                    if(reuse)for(const auto& r:*group){
                        if(r.device.identity.usable() && r.device!=old->second->request.device)reuse=false;
                        if(in.admittedDevices.contains(r.sourceId) && in.admittedDevices[r.sourceId]!=old->second->request.device)reuse=false;}
                    if(!reuse && old!=s.entries.end()){retire(*old->second);waitRelease(s,key,c,generation,token,deadline);}
                    if(!reuse){
                        auto r=target;AcquisitionTimingSpan discoveryTiming("discovery.resolve");
                        const auto discovery=in.discover?in.discover(deadline.remainingMs(),token):CameraService::industrialSnapshot(CameraService{}.discover(hint && !hint->producer.isEmpty()?QStringList{hint->producer}:QStringList{},deadline.remainingMs(),token));
                        live(c,generation,token,deadline);r.device=CameraService::resolveIndustrial(r.camera,discovery,hint);r.validate();
                        discoveryTiming.finish("success",{{"route",r.device.route.toJson()},{"backend",r.device.backend.toJson()}});
                        for(const auto& source:*group){
                            require(!source.device.identity.usable() || source.device==r.device,"Expected camera route/backend changed");
                            require(!in.admittedDevices.contains(source.sourceId) || in.admittedDevices[source.sourceId]==r.device,"Runtime camera/backend differs from admission");}
                        auto entry=std::make_unique<Entry>();entry->request=r;entry->config=config;entry->hint=hint;entry->factory=in.factory;entry->experimental=experimental;entry->backend=backend;
                        if(experimental){const auto dll=backend?backend->runtime:ExperimentalOecsAcquisition::installedRuntime();const auto api=backend?backend->api:ExperimentalOecsAcquisition::ApiFactory(makeOecsCApi);
                            entry->worker=in.leaseUse==CameraLeaseUse::Calibration?ExperimentalOecsAcquisition::openCalibration(r,dll,api,ticketFor(key)):ExperimentalOecsAcquisition::open(r,dll,api,ticketFor(key));
                        }else entry->worker=in.factory.open(r,in.leaseUse,FrameQueueMode::ProductionFifo,1,ticketFor(key));
                        s.entries[key]=std::move(entry);auto& e=*s.entries[key];
                        command(e,CameraOperation::Connect,CameraState::Connected,c,generation,token,deadline);
                        phase(c,generation,key,AcquisitionSessionState::Configuring);
                        command(e,CameraOperation::Configure,CameraState::Configured,c,generation,token,deadline);
                        command(e,CameraOperation::Start,CameraState::Streaming,c,generation,token,deadline);
                    }
                    live(c,generation,token,deadline);phase(c,generation,key,AcquisitionSessionState::Ready,{},s.entries[key].get());
                }catch(const std::exception& e){
                    for(auto& [held,entry]:s.entries)retire(*entry);
                    for(const auto& held:groups.keys())phase(c,generation,held,AcquisitionSessionState::Fault,QString::fromUtf8(e.what()));
                    break;
                }
            }
            live(c,generation,token,deadline);
            std::lock_guard lock(mutex);c->status.preparing=false;c->status.drained=s.entries.empty();c->status.clean=s.clean;return snapshotLocked(*c);
        }catch(...){for(auto& [key,e]:s.entries){retire(*e);phase(c,generation,key,AcquisitionSessionState::Fault,"Preparation cancelled or expired");}
            std::lock_guard lock(mutex);if(c->status.generation==generation)c->status.preparing=false;throw;}
    }
    IndustrialExecutionFrame acquire(const std::shared_ptr<SharedContext>& c,quint64 generation,
        const std::shared_ptr<std::atomic_bool>& token,IndustrialCameraRequest request,const QString& cycle,const QString& frame,
        Deadline deadline,const std::shared_ptr<std::atomic_bool>& cancel,bool experimental,const std::optional<DiscoveredDevice>& admitted){
        auto& s=scope(c);const auto key=physicalKey(request);Entry* entry=nullptr;
        try {
            require(s.entries.contains(key),"Prepared camera session missing");entry=s.entries[key].get();live(c,generation,token,deadline,cancel);
            require(!entry->closing && entry->config==declaration(request) && entry->experimental==experimental,"Prepared camera configuration/policy changed");
            require(!admitted || *admitted==entry->request.device,"Prepared camera differs from Runtime admission");
            command(*entry,CameraOperation::Grab,CameraState::Streaming,c,generation,token,deadline,cancel);
            auto captured=entry->worker->take();const auto actual=entry->worker->readback();require(captured && actual,"Prepared acquisition missing frame/readback");
            verifyApplied(request.setup,request.profile,*actual);
            const auto original=captured->image.metadata();
            require(original.sourceId==entry->request.sourceId && !original.frameId.isEmpty() && !original.cycleId.isEmpty() && original.completedAtUtc.isValid() && captured->image.processing().isEmpty(),"Invalid prepared raw frame");
            require(experimental==!original.experimentalAcquisition.isEmpty(),"Prepared acquisition policy/provenance mismatch");
            request.device=entry->request.device;
            // Source ID is an application binding, not a hardware identity. The
            // underlying worker envelope is retained separately for diagnostics.
            auto acquired=original;acquired.sourceId=request.sourceId;acquired.coordinateSystemId=request.sourceId+"/image";
            auto mapped=acquired;mapped.frameId=frame;mapped.cycleId=cycle;
            if(experimental){mapped.experimentalAcquisition["contentId"]=QString::fromLatin1(captured->image.contentId().toHex());mapped.experimentalAcquisition["frameId"]=original.frameId;mapped.experimentalAcquisition["sourceId"]=original.sourceId;mapped.experimentalAcquisition["completedAtUtc"]=original.completedAtUtc.toString(Qt::ISODateWithMs);}
            auto evidence=experimental?std::shared_ptr<const IndustrialExecutionEvidence>{}:std::make_shared<const IndustrialExecutionEvidence>(IndustrialExecutionEvidence{request,*actual,acquired,mapped,captured->image.contentId(),original});
            auto image=captured->image.withMetadata(mapped);live(c,generation,token,deadline,cancel);
            // Publication is serialized with close/generation retirement. Stream
            // cleanup has completed; the connection remains healthy and leased.
            std::lock_guard lock(mutex);require(c->status.generation==generation && !token->load() && !(cancel && cancel->load()),"Retired acquisition completion");
            c->busy.erase(key);for(auto& source:c->status.sources)if(source.physicalKey==key)source.state=AcquisitionSessionState::Ready;
            return {std::move(image),std::move(evidence)};
        }catch(const std::exception& e){for(auto& [held,e]:s.entries)retire(*e);phase(c,generation,key,AcquisitionSessionState::Fault,QString::fromUtf8(e.what()));
            std::lock_guard lock(mutex);if(c->status.generation==generation)c->busy.erase(key);throw;
        }catch(...){for(auto& [held,e]:s.entries)retire(*e);phase(c,generation,key,AcquisitionSessionState::Fault,"Unknown acquisition failure");
            std::lock_guard lock(mutex);if(c->status.generation==generation)c->busy.erase(key);throw;}
    }
    void beginClose(const std::shared_ptr<SharedContext>& c,int budget,const std::shared_ptr<std::promise<AcquisitionContextClose>>& waiter={}){
        auto& s=scope(c);if(waiter)s.closeWaiters.push_back(waiter);s.closeStarted=true;for(auto& [key,e]:s.entries)retire(*e,budget);drainScope(s);
    }
    void markCloseLocked(const std::shared_ptr<SharedContext>& c,int budget){
        if(!c->status.closing){++c->status.generation;c->cancel->store(true);c->status.closing=true;c->status.preparing=false;c->closeDeadline=Deadline(budget);c->status.drained=false;
            for(auto& source:c->status.sources)source.state=AcquisitionSessionState::Closing;}
    }
    std::shared_future<AcquisitionContextClose> shutdown(int budget){
        std::lock_guard lock(mutex);if(stopping)return shutdownFuture;stopping=true;
        for(const auto& c:contexts)markCloseLocked(c,budget);
        jobs.push_back([this,budget]{QVector<std::shared_ptr<SharedContext>> all;{std::lock_guard lock(mutex);for(const auto& c:contexts)all.append(c);}for(const auto& c:all)beginClose(c,budget);});wake.notify_one();return shutdownFuture;
    }
    void run(){
        for(;;){std::function<void()> job;
            {std::unique_lock lock(mutex);if(jobs.empty())wake.wait_for(lock,std::chrono::milliseconds(1));if(!jobs.empty()){job=std::move(jobs.front());jobs.pop_front();}}
            if(job)job();for(auto& [id,s]:scopes)drainScope(s);
            std::lock_guard lock(mutex);
            if(stopping && jobs.empty()){
                bool done=true,clean=true;QString error;
                for(const auto& [id,s]:scopes){done&=s.entries.empty() && !s.closeStarted;clean&=s.clean;if(!s.error.isEmpty())error=s.error;}
                if(done){shutdownPromise.set_value({clean,error});return;}
            }
        }
    }
};
CameraAcquisitionSessions::CameraAcquisitionSessions():core_(std::make_shared<Core>()){
    // This coordinator is self-owned, has no UI callbacks, and exits only after
    // joined hardware teardown. No hardware worker is detached or force-killed.
    std::thread([core=core_]{core->run();}).detach();
}
CameraAcquisitionSessions::~CameraAcquisitionSessions(){core_->shutdown(1000);}
std::future<AcquisitionContextStatus> CameraAcquisitionSessions::prepare(const QString& id,const IndustrialExecutionInputs& original,
    QVector<IndustrialCameraRequest> requests,bool experimental,int timeout,std::shared_ptr<const AcquisitionOecsBackend> backend){
    require(!id.trimmed().isEmpty() && timeout>0 && timeout<=60000,"Invalid preparation context/budget");validateTargets(original,requests,experimental);
    auto in=original;in.sessions.reset();in.sessionContext.clear();in.sessionGeneration=0;
    const auto timing=AcquisitionTiming::current();Deadline deadline(timeout);std::lock_guard lock(core_->mutex);require(!core_->stopping,"Camera session service shutting down");
    auto c=core_->contextLocked(id);require(c->busy.empty(),"Drain pending acquisition before preparing again");require(!c->status.closing || c->status.drained,"Drain previous context before preparing again");
    require(c->status.generation==0 || c->use==in.leaseUse,"Context lease use changed; choose a new context ID");c->use=in.leaseUse;
    c->cancel->store(true);c->cancel=std::make_shared<std::atomic_bool>(false);auto token=c->cancel;const auto generation=++c->status.generation;
    c->status.resourceReason=0;c->status.closing=false;c->status.preparing=true;c->status.drained=false;c->status.sources.clear();c->closeDeadline.reset();c->busy.clear();
    for(const auto& r:requests)c->status.sources[r.sourceId]={AcquisitionSessionState::Opening,r,physicalKey(r),{},{},{}};
    return core_->postLocked<AcquisitionContextStatus>([core=core_,c,generation,token,in,requests=std::move(requests),experimental,deadline,backend,timing]{
        AcquisitionTimingBinding binding(timing);AcquisitionTimingSpan span("sessions.prepare");return core->prepare(c,generation,token,in,requests,experimental,deadline,backend);});
}
std::future<IndustrialExecutionFrame> CameraAcquisitionSessions::acquire(const QString& id,quint64 generation,IndustrialCameraRequest request,
    const QString& cycle,const QString& frame,int timeout,std::shared_ptr<std::atomic_bool> cancel,bool experimental,std::optional<DiscoveredDevice> admitted){
    require(timeout>0 && !cycle.isEmpty() && !frame.isEmpty(),"Invalid prepared capture identity/budget");Deadline deadline(timeout);const auto timing=AcquisitionTiming::current();
    std::lock_guard lock(core_->mutex);require(!core_->stopping && core_->contexts.contains(id),"Unknown/shutting down camera context");auto c=core_->contexts[id];
    require(c->status.generation==generation && !c->status.preparing && !c->status.closing,"Stale/not prepared camera context");
    require(c->status.sources.contains(request.sourceId),"Source not prepared in this context");const auto& source=c->status.sources[request.sourceId];
    require(source.state==AcquisitionSessionState::Ready && declaration(source.request)==declaration(request),"Source not Ready or configuration changed");
    require(!request.device.identity.usable() || request.device==source.request.device,"Requested camera route/backend differs from preparation");
    const auto key=source.physicalKey;require(!c->busy.contains(key),"Camera acquisition already pending");c->busy.insert(key);auto token=c->cancel;
    for(auto& s:c->status.sources)if(s.physicalKey==key)s.state=AcquisitionSessionState::Capturing;
    return core_->postLocked<IndustrialExecutionFrame>([core=core_,c,generation,token,request=std::move(request),cycle,frame,deadline,cancel,experimental,admitted,timing]{
        AcquisitionTimingBinding binding(timing);AcquisitionTimingSpan span("sessions.acquire");return core->acquire(c,generation,token,request,cycle,frame,deadline,cancel,experimental,admitted);});
}
std::future<AcquisitionContextClose> CameraAcquisitionSessions::close(const QString& id,int budget){
    require(budget>0,"Invalid close budget");std::lock_guard lock(core_->mutex);require(!core_->stopping,"Camera session service shutting down");auto c=core_->contextLocked(id);core_->markCloseLocked(c,budget);
    auto promise=std::make_shared<std::promise<AcquisitionContextClose>>();auto future=promise->get_future();
    core_->jobs.push_back([core=core_,c,budget,promise]{core->beginClose(c,budget,promise);});core_->wake.notify_one();return future;
}
std::shared_future<AcquisitionContextClose> CameraAcquisitionSessions::shutdown(int budget){require(budget>0,"Invalid shutdown budget");return core_->shutdown(budget);}
AcquisitionContextStatus CameraAcquisitionSessions::status(const QString& id) const {
    std::lock_guard lock(core_->mutex);if(!core_->contexts.contains(id)){AcquisitionContextStatus empty;empty.context=id;return empty;}return core_->snapshotLocked(*core_->contexts[id]);
}
}
