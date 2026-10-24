#pragma once
#include "RuntimeService.hpp"
namespace visionflow {
// Whole-set admission controller; an optional archive hook enables terminal7/9 persistence.
// One trusted provider must capture the entire required set consistently.
class MultiSourceRuntimeService {
public:
    using Provider=std::function<MultiSourceDependencies(const Recipe&)>;
    MultiSourceRuntimeService(RuntimeIdentity identity,Provider provider,ProductionRecipeService::Persist persist={});
    void prepare(const Recipe&,const QString& directory);
    ProductionCycle begin(const QJsonObject& communication={});
    void finish(quint64 ticket,const CycleResult&);
    void abandon(quint64 ticket,const QString& reason);
    QString failure() const;
    void requireReady() const;
    bool busy() const;
    std::shared_ptr<const ProductionSnapshot> active() const;
    const std::optional<CycleResult>& lastCompletion() const;
private:
    void owner() const;
    void permission(ProductionAction) const;
    void current() const;
    struct Prepared {std::shared_ptr<const ProductionSnapshot> snapshot;QMap<QString,std::shared_ptr<const ProductionSnapshot>> branches;};
    Prepared capture(const Recipe&,const QString&) const;
    RuntimeIdentity identity_;
    Provider provider_;
    ProductionRecipeService::Persist persist_;
    QJsonObject pending_;
    std::optional<CycleResult> pendingCompletion_;
    QJsonObject communication_;
    QString beginId_;
    QDateTime started_;
    void persist(const QString&,const QString&,const QJsonObject&) const;
    QThread* thread_=QThread::currentThread();
    Prepared prepared_;
    ProductionCycle cycle_;
    quint64 serial_=0;
    std::optional<CycleResult> last_;
    QString failure_;
};
}
