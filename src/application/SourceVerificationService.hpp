#pragma once
#include "ScopedActiveCalibration.hpp"
#include "ProductionRecipeService.hpp"
#include <QJsonDocument>
#include <QUuid>
#include <memory>
#include <vector>

namespace visionflow {
// Preparation evidence only: never fits a model, starts a cycle, records or decides.
// One Source is one actual probe; logical recipe sources sharing the exact camera/setup/
// profile/selection share it. The whole set is admitted only after every probe succeeded
// AND every camera context really drained; permission is renewed after the drain.
class SourceVerificationService {
public:
    struct Source {QStringList sourceIds; RememberedCalibration selection; IndependentCaptureRequest request; bool experimental=false;};
    // replayRefs: metric offline-replay refs whose current session permission must stay valid.
    struct Plan {QByteArray fingerprint,recipeIdentity; QVector<Source> probes; QVector<SetupReference> replayRefs; QVector<SetupReference> experimentalReplayRefs;};
    using Planner=std::function<Plan()>;
    enum class State {Waiting, Verifying, Ready, Failed, Cancelled};
    class Job {
        friend class SourceVerificationService;
        QString ticket_;
        QVector<IndependentCaptureJob> captures_;
    public:
        int probeCount() const {return captures_.size();}
    };
    class Result {
        friend class SourceVerificationService;
        QString ticket_;
        QVector<IndependentCaptureResult> captures_;
    };
    SourceVerificationService(ScopedActiveCalibrationService& active,std::function<bool()> gate)
        :active_(active),gate_(std::move(gate)){}
    ~SourceVerificationService(){for(auto& session:sessions_)session->cancel(true);}
    SourceVerificationService(const SourceVerificationService&)=delete;
    SourceVerificationService& operator=(const SourceVerificationService&)=delete;
    Job begin(Planner planner,const IndependentCaptureBackend& backend={}) {
        owner();require(!busy() && gate_(),"Runtime/preview/worker/pending record/ACK busy");
        auto plan=planner();require(!plan.fingerprint.isEmpty(),"Source fingerprint required");
        QSet<QString> cameras,sources;
        for(const auto& source:plan.probes){source.request.validate();
            require(source.request.origin==SourceOrigin::CameraAcquired && source.request.camera && source.request.setup,"Actual camera probe required");
            require(!source.sourceIds.isEmpty() && source.selection.target==SetupReference{source.request.camera->id,source.request.setup->id,source.request.setup->revision},"Probe selection target mismatch");
            require(!cameras.contains(source.request.camera->id.value),"Repeated source/camera ownership");
            cameras.insert(source.request.camera->id.value);
            for(const auto& id:source.sourceIds){require(!sources.contains(id),"Repeated source/camera ownership");sources.insert(id);}
        }
        QMap<QString,ScopedActiveCalibrationService::InvalidationToken> invalidations;
        auto watch=[&](const SetupReference& ref){invalidations[ref.camera.value]=active_.invalidationToken(ref.camera);};
        for(const auto& source:plan.probes)watch(source.selection.target);
        for(const auto& ref:plan.replayRefs)watch(ref);
        for(const auto& ref:plan.experimentalReplayRefs)watch(ref);
        for(auto& session:sessions_)session->cancel(true);
        sessions_.clear();Job job;job.ticket_=QUuid::createUuid().toString(QUuid::WithoutBraces);
        try {
            for(const auto& source:plan.probes){auto session=std::make_unique<IndependentCalibrationSession>();
                if(source.request.camera->physical)session->prepareCamera(source.request,backend);
                job.captures_.append(session->begin(source.request));sessions_.push_back(std::move(session));}
        }catch(...){for(auto& session:sessions_)session->cancel(true);sessions_.clear();throw;}
        planner_=std::move(planner);plan_=std::move(plan);invalidations_=std::move(invalidations);ticket_=job.ticket_;errors_.clear();probeErrors_.clear();probeOk_.clear();
        state_=State::Verifying;cancelled_=false;draining_=false;return job;
    }
    static Result run(const Job& job,const IndependentCaptureBackend& backend={}) {
        Result result;result.ticket_=job.ticket_;
        for(const auto& capture:job.captures_)result.captures_.append(IndependentCalibrationSession::run(capture,backend));
        return result;
    }
    // True = this completion was consumed (ownership is released only by poll()).
    bool accept(const Result& result) {
        owner();if(ticket_.isEmpty() || result.ticket_!=ticket_)return false;
        ticket_.clear();probeOk_.assign(plan_.probes.size(),false);probeErrors_=QVector<QString>(plan_.probes.size());
        if(result.captures_.size()!=plan_.probes.size())errors_["sourceSet"]="Incomplete probe result set";
        for(int i=0;i<plan_.probes.size() && i<result.captures_.size();++i){
            bool ok=false;try{ok=sessions_[i]->accept(result.captures_[i]);}catch(const std::exception& error){probeErrors_[i]=QString::fromUtf8(error.what());}
            probeOk_[i]=ok;if(!ok && probeErrors_[i].isEmpty())probeErrors_[i]=result.captures_[i].error().isEmpty() ? QString("Probe rejected"):result.captures_[i].error();
        }
        for(int i=result.captures_.size();i<plan_.probes.size();++i)probeErrors_[i]="Probe rejected";
        for(auto& session:sessions_)session->closeCamera();
        draining_=true;(void)poll();return true;
    }
    // Releases ownership only once every camera context really drained.
    bool poll() {
        owner();if(!draining_)return !busy();
        for(const auto& session:sessions_){const auto status=session->cameraStatus();if(status.preparing || !status.drained)return false;}
        draining_=false;
        if(cancelled_){state_=State::Cancelled;return true;}
        for(const auto& session:sessions_)if(!session->cameraStatus().clean){errors_["sourceSet"]="Camera close failed";state_=State::Failed;return true;}
        try {
            require(gate_(),"Runtime/preview/worker/pending record/ACK busy");
            requireInvalidationsCurrent();
            require(planner_().fingerprint==plan_.fingerprint,"Project/recipe/source/model/setup changed during verification");
            for(int i=0;i<plan_.probes.size();++i){const auto& source=plan_.probes[i];QString reason;
                if(!errors_.contains("sourceSet")){
                    if(!probeOk_[i])reason=probeErrors_.value(i);
                    else try {active_.reauthorizeRemembered(source.selection,*sessions_[i]);}catch(const std::exception& error){reason=QString::fromUtf8(error.what());}
                }
                if(!reason.isEmpty())for(const auto& id:source.sourceIds)errors_[id]=reason;
            }
            requireInvalidationsCurrent();
            require(planner_().fingerprint==plan_.fingerprint,"Dependencies changed during source completion");
        }catch(const std::exception& error){errors_["sourceSet"]=QString::fromUtf8(error.what());}
        state_=errors_.isEmpty() ? State::Ready:State::Failed;return true;
    }
    // Cooperative: ownership (busy) is kept until the worker Result is accepted and drained.
    void cancel(){owner();cancelled_=true;for(auto& session:sessions_)session->cancel(true);state_=State::Cancelled;}
    bool busy() const {owner();return !ticket_.isEmpty() || draining_;}
    // Existing same-session activation remains usable until this command is invoked.
    // After an attempted verification, a partial/failed/cancelled set cannot admit.
    bool admissionAllowed() const {owner();return !planner_ || state()==State::Ready;}
    bool admissionAllowedForIdentity(const QByteArray& identity) const {return admissionAllowed() && (!planner_ || plan_.recipeIdentity==identity);}
    bool admissionAllowedFor(const Recipe& recipe,const QString& directory) const {return admissionAllowedForIdentity(ProductionRecipeService::recipeContent(recipe,directory));}
    State state() const {
        owner();if(busy())return State::Verifying;
        if(state_==State::Ready){try{
            requireInvalidationsCurrent();
            if(planner_().fingerprint!=plan_.fingerprint)return State::Waiting;
            QVector<SetupReference> trusted,experimental;
            auto add=[](QVector<SetupReference>& to,const SetupReference& ref){if(!to.contains(ref))to.append(ref);};
            for(const auto& source:plan_.probes)add(source.experimental ? experimental:trusted,source.selection.target);
            for(const auto& ref:plan_.replayRefs)add(trusted,ref);
            for(const auto& ref:plan_.experimentalReplayRefs)add(experimental,ref);
            if(!trusted.isEmpty())(void)active_.snapshots(trusted);
            if(!experimental.isEmpty())(void)active_.experimentalSnapshots(experimental);
        }catch(...){return State::Waiting;}}
        return state_;
    }
    const QMap<QString,QString>& errors() const {owner();return errors_;}
    int probeCount() const {owner();return plan_.probes.size();}
private:
    void requireInvalidationsCurrent() const {
        for(auto i=invalidations_.cbegin();i!=invalidations_.cend();++i)
            require(active_.invalidationToken(CameraId{i.key()})==i.value(),"Camera/session invalidated after verification began; acquire a new probe");
    }
    static void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
    void owner() const {require(thread_==QThread::currentThread(),"Source verification owner thread required");}
    ScopedActiveCalibrationService& active_;
    std::function<bool()> gate_;
    QThread* thread_=QThread::currentThread();
    Planner planner_;Plan plan_;QString ticket_;
    QMap<QString,ScopedActiveCalibrationService::InvalidationToken> invalidations_;
    std::vector<std::unique_ptr<IndependentCalibrationSession>> sessions_;
    QMap<QString,QString> errors_;
    QVector<QString> probeErrors_;std::vector<bool> probeOk_;
    State state_=State::Waiting;
    bool cancelled_=false,draining_=false;
};
}
