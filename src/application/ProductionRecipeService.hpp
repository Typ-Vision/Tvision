#pragma once
#include "ExecutionService.hpp"
#include "domain/Project.hpp"
#include "domain/Coordinates.hpp"
#include <QThread>
#include <functional>
#include <stdexcept>

namespace visionflow {
enum class ProductionAction { Validate, Activate, Rollback, Run };
enum class ProductionError { Unauthorized, Busy, InvalidConfiguration, DependencyChanged,
                             UnknownValidation, HistoryFull, InvalidCompletion, WrongThread, PersistenceFailure };
class ProductionException : public std::runtime_error {
public:
    ProductionException(ProductionError code,const QString& message);
    ProductionError code() const { return code_; }
private:
    ProductionError code_;
};
// Trusted application providers, never a role/VALID flag supplied by the UI.
struct ProductionDependencies {
    QString engineVersion=ExecutionService::version();
    QMap<QString,QString> assetHashes; // SHA-256 identities from a trusted asset resolver
    std::optional<CalibrationDependency> calibration;
};
struct RecipeValidation {
    QString id,principal;
    QByteArray contentId,dependencyId;
    QDateTime checkedAtUtc;
    QString scope="execution_configuration"; // not camera/part/calibration acceptance
};
class ProductionSnapshot {
public:
    const Recipe& recipe() const { return recipe_; }
    const QString& projectDirectory() const { return directory_; }
    const RecipeValidation& validation() const { return validation_; }
    const QJsonObject& dependencies() const { return dependencies_; }
private:
    friend class ProductionRecipeService;
    Recipe recipe_;
    QString directory_;
    RecipeValidation validation_;
    QJsonObject dependencies_;
};
struct ProductionCycle {
    quint64 ticket=0;
    std::shared_ptr<const ProductionSnapshot> snapshot;
};
// Owner thread controller; snapshots may be captured by immutable worker jobs.
// Archive hooks persist evidence; restart never restores authorization as VALID.
class ProductionRecipeService {
public:
    using Authorize=std::function<bool(const QString&,ProductionAction)>;
    using Dependencies=std::function<ProductionDependencies(const Recipe&)>;
    using Persist=std::function<void(const QString& kind,const QString& id,const QJsonObject&)>;
    explicit ProductionRecipeService(Authorize authorize={},Dependencies dependencies={},int historyLimit=16,Persist persist={},CalibrationResolver completionResolver={});
    ProductionRecipeService(const ProductionRecipeService&)=delete;
    ProductionRecipeService& operator=(const ProductionRecipeService&)=delete;
    RecipeValidation validate(const QString& principal,const Recipe& draft,const QString& projectDirectory);
    void activate(const QString& principal,const QString& validationId,const Recipe& currentDraft,const QString& projectDirectory);
    void rollback(const QString& principal,const QString& validationId);
    std::shared_ptr<const ProductionSnapshot> active() const;
    ProductionCycle beginCycle(const QString& principal,const QJsonObject& communication={});
    void requireReady(const QString& principal) const;
    void finishCycle(quint64 ticket,const CycleResult& result);
    void abandonCycle(quint64 ticket,const QString& reason);
    QVector<QJsonObject> cycleRecords() const;
    bool busy() const;
private:
    friend class RuntimeService;
    static QByteArray completionDigest(const CycleResult&);
    void owner() const;
    void permission(const QString&,ProductionAction) const;
    QJsonObject dependencies(const Recipe&,const QString& directory) const;
    void requireCurrent(const ProductionSnapshot&) const;
    void idle() const;
    QThread* ownerThread_=QThread::currentThread();
    Authorize authorize_;
    Dependencies dependencies_;
    Persist persist_;
    CalibrationResolver completionResolver_;
    void persist(const QString&,const QString&,const QJsonObject&) const;
    void transition(const QString&,const QString&,const ProductionSnapshot&) const;
    int historyLimit_;
    QMap<QString,std::shared_ptr<const ProductionSnapshot>> validated_;
    QVector<QString> activated_;
    std::shared_ptr<const ProductionSnapshot> active_;
    ProductionCycle cycle_;
    QString cyclePrincipal_;
    QString cycleBeginId_;
    QJsonObject cycleCommunication_;
    QDateTime cycleStarted_;
    quint64 serial_=0;
    QVector<QJsonObject> records_;
    QJsonObject pendingRecord_;
    void record(const CycleResult*,const QString& error);
};
}
