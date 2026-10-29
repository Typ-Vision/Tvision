#pragma once
#include "StationAdmissionService.hpp"
#include "RuntimeCoordinator.hpp"
#include <QFuture>
#include <QThreadPool>
#include <mutex>

namespace visionflow {
// Owner-thread composition of existing station Runtime contexts. reserve/dispatch
// plug into StationRuntimeAdmissionBoundary; no acquisition precedes durable intent.
class StationExecutionRegistry final {
public:
    using Completion=std::function<void(const StationAdmissionIntent&,const ProductionCycle&,const CycleResult&)>;
    explicit StationExecutionRegistry(int workers=3);
    ~StationExecutionRegistry();
    StationExecutionRegistry(const StationExecutionRegistry&)=delete;
    StationExecutionRegistry& operator=(const StationExecutionRegistry&)=delete;
    void bind(const QUuid&,RuntimeCoordinator&,QString catalog,Completion,ExecutionService::SourceAdapterFactory factory={});
    StationAdmissionReservation reserve(const StationAdmissionSnapshot&);
    void dispatch(const StationAdmissionIntent&,const ProductionCycle&,const std::shared_ptr<void>&);
    void poll();
    bool busy(const QUuid&) const;
    QString error(const QUuid&) const;
private:
    struct Budget {std::mutex mutex;int used=0;};
    struct Permit {
        std::shared_ptr<Budget> budget;bool released=false;
        void release();~Permit(){release();}
    };
    struct Job {
        StationAdmissionIntent intent;ProductionCycle cycle;std::shared_ptr<void> lease;
        std::shared_ptr<Permit> permit;std::shared_ptr<std::atomic_bool> cancel;
        QFuture<CycleResult> future;std::optional<CycleResult> result;
        bool committed=false,failed=false;
    };
    struct Binding {RuntimeCoordinator* coordinator;QString catalog;Completion completion;ExecutionService::SourceAdapterFactory factory;};
    void owner() const;
    QThread* owner_=QThread::currentThread();int limit_;
    std::shared_ptr<Budget> budget_=std::make_shared<Budget>();
    QThreadPool pool_;
    QMap<QUuid,Binding> bindings_;
    QMap<QUuid,std::weak_ptr<Permit>> permits_;
    QMap<QUuid,std::shared_ptr<Job>> jobs_;
    QMap<QUuid,QString> errors_;
    bool polling_=false;
};
}
