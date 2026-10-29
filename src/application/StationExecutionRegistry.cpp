#include "StationExecutionRegistry.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QScopeGuard>
#include <stdexcept>

namespace visionflow {
void StationExecutionRegistry::Permit::release(){std::lock_guard lock(budget->mutex);if(!released){--budget->used;released=true;}}
StationExecutionRegistry::StationExecutionRegistry(int workers):limit_(workers){
    if(workers<1 || workers>32)throw std::invalid_argument("Station worker limit must be 1..32");
    pool_.setMaxThreadCount(workers);
}
StationExecutionRegistry::~StationExecutionRegistry(){
    // No queued callbacks capture this registry. Owner must stop/drain/poll before
    // normal destruction; an exceptional shutdown never invents terminal success.
    for(const auto& job:jobs_)if(job->cancel)job->cancel->store(true);
    pool_.waitForDone();
}
void StationExecutionRegistry::owner() const {if(QThread::currentThread()!=owner_)throw std::runtime_error("Station executor owner thread required");}
void StationExecutionRegistry::bind(const QUuid& id,RuntimeCoordinator& c,QString catalog,Completion completion,ExecutionService::SourceAdapterFactory factory){
    owner();if(id.isNull() || bindings_.contains(id) || bindings_.size()>=32 || !completion)throw std::invalid_argument("Invalid station executor binding");
    for(const auto& b:bindings_)if(b.coordinator==&c)throw std::invalid_argument("Station coordinator must be independent");
    bindings_.insert(id,{&c,std::move(catalog),std::move(completion),std::move(factory)});
}
StationAdmissionReservation StationExecutionRegistry::reserve(const StationAdmissionSnapshot& s){
    owner();if(!s.station || !bindings_.contains(s.station->station.id))return {20,{}};
    const auto id=s.station->station.id;
    if(jobs_.contains(id))return {2,{}};
    if(auto prior=permits_.value(id).lock();prior && !prior->released)return {2,{}};
    std::lock_guard lock(budget_->mutex);if(budget_->used>=limit_)return {23,{}};
    auto permit=std::make_shared<Permit>();permit->budget=budget_;++budget_->used;
    permits_[id]=permit;return {0,permit};
}
void StationExecutionRegistry::dispatch(const StationAdmissionIntent& intent,const ProductionCycle& cycle,const std::shared_ptr<void>& lease){
    owner();const auto id=intent.identity.station;auto permit=permits_.value(id).lock();
    if(!bindings_.contains(id) || jobs_.contains(id) || !permit || permit->released || permit.get()!=lease.get()
       || !intent.snapshot.station || intent.snapshot.station->station.id!=id || !cycle.snapshot || !cycle.ticket
       || cycle.snapshot->recipe().id!=intent.snapshot.station->selection.recipe.id
       || cycle.snapshot->recipe().definition!=intent.snapshot.station->selection.recipe.definition)
        throw std::runtime_error("Station execution lease mismatch");
    auto& binding=bindings_[id];auto job=std::make_shared<Job>();job->intent=intent;job->cycle=cycle;job->lease=lease;job->permit=permit;
    job->cancel=binding.coordinator->cancellation();if(!job->cancel)throw std::runtime_error("Missing Runtime cancellation lease");
    // Capture immutable values only, never a Coordinator, station map or UI pointer.
    const auto catalog=binding.catalog;const auto factory=binding.factory;const auto cancel=job->cancel;
    job->future=QtConcurrent::run(&pool_,[cycle,catalog,cancel,factory]{return RuntimeService::execute(cycle,catalog,cancel,factory);});
    jobs_[id]=job;errors_.remove(id);
}
void StationExecutionRegistry::poll(){
    owner();if(polling_)return;polling_=true;const auto guard=qScopeGuard([&]{polling_=false;});
    const auto ids=jobs_.keys();
    for(const auto& id:ids){
        auto job=jobs_[id];if(job->failed || !job->future.isFinished())continue;
        auto& binding=bindings_[id];auto& c=*binding.coordinator;
        try{
            if(!job->result){
                job->result=job->future.result();
                if(job->cancel->load() && job->result->state!=ExecutionState::Skipped)RuntimeService::suppressCancelled(job->cycle,*job->result);
            }
            if(!job->committed){
                if(c.recordingPending())c.retryRecording();else c.finish(job->cycle.ticket,*job->result);
                job->committed=true;
            }
            // Sink seals/delivers this exact station/request result. On failure it
            // may be retried, but Runtime finish and acquisition never repeat.
            binding.completion(job->intent,job->cycle,*job->result);
            job->permit->release();jobs_.remove(id);errors_.remove(id);
        }catch(const ProductionException& e){
            errors_[id]=QString::fromUtf8(e.what());
            if(!c.recordingPending() && !job->committed)job->failed=true;
        }catch(const std::exception& e){errors_[id]=QString::fromUtf8(e.what());if(!job->result)job->failed=true;}
        catch(...){errors_[id]="Worker/completion failure: recovery required";job->failed=true;}
    }
}
bool StationExecutionRegistry::busy(const QUuid& id) const {owner();return jobs_.contains(id);}
QString StationExecutionRegistry::error(const QUuid& id) const {owner();return errors_.value(id);}
}
