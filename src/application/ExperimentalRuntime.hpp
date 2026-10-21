#pragma once
#include "RuntimeService.hpp"

namespace visionflow {
// Session-only admission. Uses the common worker and viewer, but never issues a
// production completion capability or accepts communication requests.
class ExperimentalRuntime {
public:
    ExperimentalRuntime(RuntimeIdentity,RuntimeService::MultiProvider,ProductionRecipeService::Persist);
    void prepare(const Recipe&,const QString&);
    void requireReady() const;
    ProductionCycle begin(const QJsonObject&);
    void finish(quint64,const CycleResult&);
    bool busy() const { return cycle_.ticket!=0; }
    std::shared_ptr<const ProductionSnapshot> active() const { return snapshot_; }
    static bool accepts(const ProductionCycle&);
    static CycleResult execute(const ProductionCycle&,const std::shared_ptr<std::atomic_bool>&);
    static void suppressCancelled(const ProductionCycle&,CycleResult&);
    static void validateRecord(const QJsonObject&);
    static QString summary(const QJsonObject&);
    static QByteArray csv(const QVector<QJsonObject>&);
private:
    std::shared_ptr<const ProductionSnapshot> capture(const Recipe&,const QString&) const;
    void permission() const;
    static QByteArray digest(const CycleResult&);
    static void completion(const ProductionCycle&,const CycleResult&);
    RuntimeIdentity identity_;
    RuntimeService::MultiProvider provider_;
    ProductionRecipeService::Persist persist_;
    QThread* thread_=QThread::currentThread();
    std::shared_ptr<const ProductionSnapshot> snapshot_;
    ProductionCycle cycle_;
    quint64 serial_=0;
    QDateTime started_;
    QJsonObject pending_;
    QByteArray pendingDigest_;
    bool faulted_=false;
};
}
