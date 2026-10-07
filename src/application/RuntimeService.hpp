#pragma once
#include "ProductionRecipeService.hpp"
#include "ProductionStore.hpp"
#include "CalibrationWorkflow.hpp"
namespace visionflow {
struct RuntimeIdentity {
    QString principal;
    ProductionRecipeService::Authorize authorize;
    static RuntimeIdentity localEngineer();
};
// Owner-thread controller; worker entry points capture only immutable values.
class RuntimeService {
public:
    RuntimeService(QString directory,QString catalogPath,RuntimeIdentity identity);
    RecipeValidation validate(const Recipe&,const QString& projectDirectory,const ImageFrame& acquired);
    void activate(const Recipe&,const QString& projectDirectory);
    void rollback(const QString& validationId);
    ProductionCycle begin();
    void finish(quint64 ticket,const CycleResult&);
    void observeInstallation(const Recipe&);
    std::shared_ptr<const ProductionSnapshot> active() const {return production_.active();}
    bool busy() const {return production_.busy();}
    QString principal() const {return identity_.principal;}
    const std::optional<RecipeValidation>& validation() const {return validation_;}
    QVector<QJsonObject> records() const {return store_.recent("cycles");}
    static ImageFrame probe(const Recipe&,const QString& directory,const std::shared_ptr<std::atomic_bool>& cancel);
    static CycleResult execute(const ProductionCycle&,const QString& catalogPath,const std::shared_ptr<std::atomic_bool>& cancel);
private:
    ProductionDependencies dependencies(const Recipe&) const;
    ProductionStore store_;
    QString catalogPath_;
    RuntimeIdentity identity_;
    QMap<QString,ImageFrame> installations_;
    QMap<QString,QJsonObject> declaredInstallations_;
    ProductionRecipeService production_;
    std::optional<RecipeValidation> validation_;
};
}
