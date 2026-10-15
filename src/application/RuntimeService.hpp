#pragma once
#include "ProductionRecipeService.hpp"
#include "ProductionStore.hpp"
#include "CalibrationWorkflow.hpp"
#include "SystemCalibration.hpp"
namespace visionflow {
struct RuntimeIdentity {
    QString principal;
    ProductionRecipeService::Authorize authorize;
    static RuntimeIdentity localEngineer();
};
// Owner-thread controller; worker entry points capture only immutable values.
class RuntimeService {
public:
    using SystemProvider=std::function<std::optional<ActiveCalibrationSnapshot>()>;
    RuntimeService(QString directory,QString catalogPath,RuntimeIdentity identity,SystemProvider system={});
    RecipeValidation validate(const Recipe&,const QString& projectDirectory,const ImageFrame& acquired);
    // Configuration-only preparation; never acquires or asserts actual frame readiness.
    void checkConfiguration(const Recipe&,const QString& projectDirectory) const;
    void prepare(const Recipe&,const QString& projectDirectory);
    void activate(const Recipe&,const QString& projectDirectory);
    void rollback(const QString& validationId);
    ProductionCycle begin(const QJsonObject& communication={});
    void requireReady() const {production_.requireReady(identity_.principal);}
    void finish(quint64 ticket,const CycleResult&);
    void observeInstallation(const Recipe&);
    std::shared_ptr<const ProductionSnapshot> active() const {return production_.active();}
    bool busy() const {return production_.busy();}
    QString principal() const {return identity_.principal;}
    const std::optional<RecipeValidation>& validation() const {return validation_;}
    QVector<QJsonObject> records() const {return store_.recent("cycles");}
    static ImageFrame probe(const Recipe&,const QString& directory,const std::shared_ptr<std::atomic_bool>& cancel);
    // Late Stop may only suppress an unchanged, trusted completion for this lease.
    static void suppressCancelled(const ProductionCycle&,CycleResult&);
    static CycleResult execute(const ProductionCycle&,const QString& catalogPath,const std::shared_ptr<std::atomic_bool>& cancel);
    static MeasurementExecutionContext calibration(const ProductionSnapshot&,const ImageFrame&,const QString& catalogPath);
private:
    ProductionDependencies dependencies(const Recipe&) const;
    ProductionDependencies declaredDependencies(const Recipe&) const;
    std::optional<ProductionDependencies> systemDependencies(const Recipe&) const;
    Recipe systemRecipe(const Recipe&,const QString& directory) const;
    MeasurementExecutionContext completionCalibration(const ImageFrame&) const;
    bool declaredPreparation_=false;
    ProductionStore store_;
    QString catalogPath_;
    RuntimeIdentity identity_;
    SystemProvider system_;
    QMap<QString,ImageFrame> installations_;
    QMap<QString,QJsonObject> declaredInstallations_;
    ProductionRecipeService production_;
    std::optional<RecipeValidation> validation_;
};
}
