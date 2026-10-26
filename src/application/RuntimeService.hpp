#pragma once
#include "ProductionRecipeService.hpp"
#include "ProductionStore.hpp"
#include "CalibrationWorkflow.hpp"
#include "SystemCalibration.hpp"
#include "RuntimeDiagnostics.hpp"
namespace visionflow {
class MultiSourceRuntimeService;
class ExperimentalRuntime;
class RuntimeCameraSessions;
class SourceVerificationService;
struct RuntimeIdentity {
    QString principal;
    ProductionRecipeService::Authorize authorize;
    static RuntimeIdentity localEngineer();
};
// Owner-thread controller; worker entry points capture only immutable values.
class RuntimeService {
public:
    using SystemProvider=std::function<std::optional<ActiveCalibrationSnapshot>()>;
    using ScopedProvider=std::function<std::optional<ActiveCalibrationSnapshot>(const Recipe&)>;
    using AcquisitionProvider=std::function<QJsonObject(const Recipe&)>;
    using MultiProvider=std::function<MultiSourceDependencies(const Recipe&)>;
    RuntimeService(QString directory,QString catalogPath,RuntimeIdentity identity,SystemProvider system={},ScopedProvider scoped={},AcquisitionProvider acquisition={},MultiProvider multi={});
    ~RuntimeService();
    RecipeValidation validate(const Recipe&,const QString& projectDirectory,const ImageFrame& acquired);
    // Configuration-only preparation; never acquires or asserts actual frame readiness.
    void checkConfiguration(const Recipe&,const QString& projectDirectory) const;
    void prepare(const Recipe&,const QString& projectDirectory);
    void requestCameraPreparation(const Recipe&,const QString& projectDirectory);
    // Optional whole-source verification; never invoked -> legacy admission unchanged.
    void setSourceVerification(std::shared_ptr<SourceVerificationService>);
    bool sourceVerificationBusy() const;
    void cancelSourceVerification();
    bool pollSourceVerification();
    bool pollCameras();
    void stopCameras();
    bool camerasPending() const;
    bool camerasDraining() const;
    QString cameraError() const;
    bool cameraCloseFailed() const;
    void activate(const Recipe&,const QString& projectDirectory);
    void rollback(const QString& validationId);
    ProductionCycle begin(const QJsonObject& communication={});
    void requireReady() const;
    void finish(quint64 ticket,const CycleResult&);
    static bool presentationMatches(const CycleResult&,const std::shared_ptr<const ProductionSnapshot>&);
    static bool sameCompletion(const CycleResult&,const CycleResult&);
    std::shared_ptr<RuntimeDiagnostics> diagnostics() const {return diagnostics_;}
    void observeInstallation(const Recipe&);
    std::shared_ptr<const ProductionSnapshot> active() const;
    bool busy() const;
    void setExperimental(bool enabled,MultiProvider provider={});
    // Configure backend dependencies only; normal Load/Run resolves required sources.
    void setAutomaticAcquisition(MultiProvider provider={});
    // Used only by offline Load; execution always consults operational providers.
    void setConfigurationProviders(ScopedProvider scoped,MultiProvider multi,MultiProvider experimental={});
    bool experimental() const {return bool(experimental_);}
    bool requiresManualCycles(const Recipe&,const QString& directory) const;
    QString principal() const {return identity_.principal;}
    const std::optional<RecipeValidation>& validation() const {return validation_;}
    QVector<QJsonObject> records() const {return store_.recent("cycles");}
    static ImageFrame probe(const Recipe&,const QString& directory,const std::shared_ptr<std::atomic_bool>& cancel);
    // Late Stop may only suppress an unchanged, trusted completion for this lease.
    static void suppressCancelled(const ProductionCycle&,CycleResult&);
    static CycleResult execute(const ProductionCycle&,const QString& catalogPath,const std::shared_ptr<std::atomic_bool>& cancel,ExecutionService::SourceAdapterFactory factory={});
    static MeasurementExecutionContext calibration(const ProductionSnapshot&,const ImageFrame&,const QString& catalogPath);
private:
    void requireVerificationIdle() const;
    void requireVerified(const Recipe&,const QString& directory) const;
    void requireActiveVerified() const;
    void rememberVerificationIdentity(const Recipe&,const QString& directory);
    std::optional<ActiveCalibrationSnapshot> systemSnapshot(const Recipe&) const;
    QJsonObject sourceDependencies(const Recipe&) const;
    ProductionDependencies calibrationDependencies(const Recipe&) const;
    ProductionDependencies declaredCalibrationDependencies(const Recipe&) const;
    ProductionDependencies dependencies(const Recipe&) const;
    ProductionDependencies declaredDependencies(const Recipe&) const;
    MultiSourceDependencies cameraComposition(const Recipe&,const MultiProvider&) const;
    std::optional<ProductionDependencies> systemDependencies(const Recipe&) const;
    Recipe systemRecipe(const Recipe&,const QString& directory) const;
    MeasurementExecutionContext completionCalibration(const ImageFrame&) const;
    bool declaredPreparation_=false;
    std::shared_ptr<RuntimeDiagnostics> diagnostics_=std::make_shared<RuntimeDiagnostics>();
    ProductionStore store_;
    QString catalogPath_;
    RuntimeIdentity identity_;
    SystemProvider system_;
    ScopedProvider scoped_;
    AcquisitionProvider acquisition_;
    QMap<QString,ImageFrame> installations_;
    QMap<QString,QJsonObject> declaredInstallations_;
    ProductionRecipeService production_;
    std::optional<RecipeValidation> validation_;
    ScopedProvider configurationScoped_;
    MultiProvider configurationMulti_,configurationExperimental_;
    MultiProvider multiProvider_;
    std::unique_ptr<MultiSourceRuntimeService> multi_;
    bool multiMode_=false;
    std::unique_ptr<ExperimentalRuntime> experimental_;
    bool policyReset_=false;
    bool automaticAcquisition_=true;
    MultiProvider automaticProvider_;
    std::unique_ptr<RuntimeCameraSessions> cameraSessions_;
    std::shared_ptr<SourceVerificationService> verification_;
    // Original recipe identity before systemRecipe adds derived calibration fields.
    // Entries follow the already bounded in-memory production validation history.
    QMap<QString,QByteArray> verificationRecipeIdentities_;
};
}
