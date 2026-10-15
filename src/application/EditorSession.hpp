#pragma once
#include "Deadline.hpp"
#include "ExecutionService.hpp"
#include "MatchService.hpp"
#include "ReferenceService.hpp"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QSet>
#include <functional>

namespace visionflow {
enum class SessionJob { Execution, Teaching, Image, Discovery, Connection };
enum class SessionState { Idle, Working, Cancelling };
enum class ShutdownState { NotRequested, Pending, BudgetExceeded, Ready };
// State is owned by the application session, accessed only from its owner thread.
// Worker functions capture immutable values; no worker accesses the session/UI.
class JobSession {
public:
    JobSession()=default;
    JobSession(const JobSession&)=delete;
    JobSession& operator=(const JobSession&)=delete;
    ~JobSession();
    bool busy(SessionJob job) const;
    bool busy() const {return !active_.isEmpty();}
    SessionState state() const;
    std::shared_ptr<std::atomic_bool> token(SessionJob job) const;
    void stop(SessionJob job);
    // Non-blocking owner-thread polling; budget exhaustion never detaches workers.
    void requestShutdown(int budgetMs=1000);
    ShutdownState shutdownState() const;
    template<class T> QFutureWatcher<T>* watch(QObject* owner,SessionJob job) {
        const auto ticket=begin(job);
        auto* watcher=new QFutureWatcher<T>(owner);
        QObject::connect(watcher,&QFutureWatcher<T>::finished,owner,[this,ticket]{finish(ticket);});
        return watcher;
    }
    template<class T,class Work> void launch(QFutureWatcher<T>* watcher,Work work) {
        auto future=QtConcurrent::run(std::move(work));
        futures_.append(QFuture<void>(future));watcher->setFuture(future);
    }
    quint64 begin(SessionJob job);
    void finish(quint64 ticket);
private:
    quint64 serial_=0;
    QMap<quint64,SessionJob> active_;
    QMap<SessionJob,std::shared_ptr<std::atomic_bool>> tokens_;
    QVector<QFuture<void>> futures_;
    std::optional<Deadline> shutdownDeadline_;
};
struct EditorContext {
    QString projectId,path,recipeId;
    QJsonObject definition;
    QJsonObject calibrationBinding;
    QJsonObject systemCalibration;
};
struct ImageLoad {std::optional<ImageFrame> frame;QString error;};
struct ModelTeaching {QString model,error;};
struct PoseTeaching {QJsonObject parameters;QString error;};
struct TrackingTeachingInput {
    ToolDefinition consumer,source;
    ToolResult reference;
    ImageFrame image;
    double mmPerSample;
    MeasurementExecutionContext calibration;
};
struct TrackingTeaching {std::optional<ToolDefinition> tool;QString error;};
class EditorSession : public JobSession {
public:
    std::optional<ImageFrame> frame;
    quint64 generation=0;
    QByteArray resultSignature;
    std::optional<MatchOutcome> matchOutcome;
    std::optional<CycleResult> cycle;
    QMap<QString,QPair<QByteArray,QPointF>> references;
    QByteArray signature(const EditorContext& context) const;
    QByteArray referenceSignature(const EditorContext& context,const QString& tool) const;
    std::optional<QPointF> referencePoint(const EditorContext& context,const QString& tool) const;
    QByteArray evaluationScope(const EditorContext&) const;
    void adoptFrame(const EditorContext&);
    TargetEvaluation evaluation(const EditorContext&) const;
    void reconcile(const EditorContext&);
    void invalidateTool(const EditorContext&,const QString& tool);
    bool acceptTool(CycleResult result,const QByteArray& captured,const EditorContext& current);
    void invalidateResults();
    void calibrationChanged(const EditorContext&);
    bool acceptCycle(CycleResult result,const QByteArray& captured,const EditorContext& current);
    bool accepts(const QByteArray& captured,const EditorContext& current) const {return captured==signature(current);}
    static ImageLoad load(const QString& path);
    static ModelTeaching teach(const ImageFrame& frame,const QRect& roi);
    static PoseTeaching teachPose(const ToolDefinition&,const ImageFrame&,const QRect&,QPointF anchor);
    ImageFrame toolImage(const EditorContext&,const QString& tool) const;
    ToolDefinition taughtReference(const EditorContext&,const ToolDefinition& consumer,const QString& producer,const QString& port,
        QPointF offset={},double angleOffsetRadians=0) const;
    ToolDefinition taughtTracking(const EditorContext&,const ToolDefinition& consumer,const QString& producer,
        double mmPerSample,const MeasurementExecutionContext& = {}) const;
    TrackingTeachingInput trackingTeachingInput(const EditorContext&,const ToolDefinition&,const QString& producer,double mmPerSample,const MeasurementExecutionContext& = {}) const;
    static TrackingTeaching teachTracking(const TrackingTeachingInput&,const std::shared_ptr<std::atomic_bool>& cancel={});
    static TrackingTeaching teachReference(const TrackingTeachingInput&,const QString& port,QPointF offset,double angle,
        const std::shared_ptr<std::atomic_bool>& cancel={});
    QJsonObject taughtSearch(const EditorContext& context,const ToolDefinition& tool,const QRect& roi) const;
    static void suppressCancelled(CycleResult& cycle);
private:
    QMap<QString,CachedToolResult> cache_;
    EditorContext cacheContext_;
    void cacheResult(const EditorContext&,const ToolResult&);

};
class CameraSession : public JobSession {};
}
