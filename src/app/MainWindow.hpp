#pragma once
#include "domain/ToolRoi.hpp"
#include <QMainWindow>
#include <QStringList>
#include "application/ProjectService.hpp"
#include "application/ImageFileService.hpp"
#include "application/MatchService.hpp"
#include "application/ExecutionService.hpp"
#include "application/CameraService.hpp"
#include "application/EditorSession.hpp"
#include "application/EditorCameraSessions.hpp"
#include "application/CalibrationWorkflow.hpp"
#include "application/ChessboardService.hpp"
#include "application/PlaneMappingService.hpp"
#include "application/RuntimeCoordinator.hpp"
#include "application/PlcRuntimeSocket.hpp"
#include "application/MeasurementWorkspace.hpp"
class QVBoxLayout;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QTreeWidget;
class QAction;
class QListWidget;
class QStackedWidget;
class QSplitter;
class QLabel;
class QCloseEvent;
class QFormLayout;
namespace visionflow {
class ImageViewer;
class EdgeProfileView;
class CalibrationOverview;
class CameraManager;
class WorkspacePanel;
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(const QString& settingsFile);
    ~MainWindow() override;
    bool verifyMultiSource(const QString& artifacts);
    bool verifyMultiSourceRuntime(const QString& artifacts);
    bool verifyShell();
    bool verifyEditorResponsiveness(const QString& artifacts);
    bool verifyEditorPropertyCommits(const QString& artifacts);
    bool verifyWorkspace(const QString& artifactDirectory);
    bool verifyEditor(const QString& artifactDirectory);
    bool verifySession();
    bool verifyEditorCameraSessions(const QString& artifacts);
    bool verifyRuntimeCameraSessions(const QString& artifacts);
    bool verifyMatching(const QString& artifacts);
    bool verifyExecution(const QString& artifacts);
    bool verifyRoi(const QString& artifacts);
    bool verifyPreprocessing(const QString& artifacts);
    bool verifyEdgePoint(const QString& artifacts);
    bool verifyMetrology(const QString& artifacts);
    bool verifyGeometry(const QString& artifacts);
    bool verifyPoseTracking(const QString& artifacts);
    bool verifyEditorUsability(const QString& artifacts);
    bool verifyExperimentalEditor(const QString& artifacts);
    bool verifyCircle(const QString& artifacts);
    bool verifyCodeReading(const QString& artifacts);
    bool verifyCamera(const QString& artifacts);
    bool verifyWebcam(const QString& artifacts);
    bool verifyMeasurement(const QString& artifacts);
    bool verifyRestartRuntime(const QString& artifacts);
    bool verifyRuntimeFailures(const QString& artifacts);
    bool verifyRuntime(const QString& artifacts);
    bool verifyCommunication(const QString& artifacts);
    bool verifySystemCalibration(const QString& artifacts);
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    QWidget* makeDashboard();
    QWidget* makeCameraManager();
    MeasurementWorkspace* measurementWorkspace();
    void discoverCameras();
    void checkCameraConnection();
    QWidget* makeEditor();
    QWidget* makeCalibration();
    QWidget* makeRuntime();
    QWidget* makeCommunication();
    void communicationAction(const QString&);
    void refreshCommunication();
    QWidget* makeLogs();
    void refreshRuntime(bool notification=true);
    void updateOperatorMode(bool active);
    bool operatorMode_ = false;
    QByteArray operatorGeometry_;
    Qt::WindowStates operatorWindowState_{};
    void runtimeAction(const QString&);
    void showRuntimeResult();
    void completeRuntimeResult(CycleResult,const std::shared_ptr<std::atomic_bool>&);
    void refreshLogs();
    void exportLogCsv(const QString& path);
    void ensureRuntime();
    IndustrialExecutionInputs runtimeCameraInputs() const;
    void initializeCommunication();
    void refreshCalibration();
    QWidget* makeProjectManager();
    QWidget* makeRecipeManager();
    void showProjectDialog(bool edit = false);
    void openProjectDialog();
    void showRecipeDialog(bool edit = false);
    void openSelectedRecipe();
    void openEditorRecipe(const QString& id);
    void syncEditorCameras(const QString& retrySource={},bool activate=false,const MeasurementEnvironment* environment=nullptr);
    bool drainEditorCameras(std::function<void()> after);
    void pollEditorCameras();
    QString editorCameraStatus(const QString& source) const;
    void loadSelectedRecipe();
    void refreshWorkspace();
    void refreshWorkspaceHeader();
    void refreshRecipes();
    void saveProject();
    bool resolveUnsavedChanges();
    void displayError(const QString& message);
    void refreshEditor();
    quint64 editorRefreshEpoch_=0; // Supersedes an outer reentrant display operation.
    void selectTool(const QString& id);
    void showToolProperties();
    void showToolPropertiesView(const EditorContext&,const std::optional<MeasurementEnvironment>&);
    void addSelectedTool();
    enum class PropertyCommit { Unchanged, Changed, Failed };
    ToolDefinition readToolProperties(ToolDefinition tool) const;
    PropertyCommit commitToolProperties();
    bool applyToolProperties();
    std::optional<ToolDefinition> displayedProperties_;
    void openImageDialog();
    bool loadEditorImage(const QString& path);
    void beginRoi(bool model);
    void commitRoi(const QRect& roi);
    void addTrackingProperties(QFormLayout*,const ToolDefinition&);
    void teachPoseReference(const QString& producer,double mmPerSample,std::optional<ToolRoi> nominal={});
    void editWorldTrackingRoi(const QString& producer);
    std::optional<QPointF> referencePoint(const QString& source) const;
    void runMatch();
    QByteArray referenceSignature(const QString& toolId) const;
    void runFlow();
    void runExecution(const QString& targetTool);
    void refreshFlow();
    void refreshFlowView(const EditorContext&,const std::optional<MeasurementEnvironment>&);
    void showFlowMenu(const QPoint& position);
    void prepareNewToolDrawing();
    bool setDrawingImage();
    QString pendingDrawTool_,pendingDrawRecipe_;
    bool drawPreparationQueued_=false;
    std::optional<ImageFrame> drawingImage_;
    void showExecutionResult();
    void showExecutionResultView(const EditorContext&);
    QByteArray flowSignature() const;
    void loadImageAsync(const QString& path,bool commit);
    void refreshMatchOverlay();
    void refreshMatchOverlayView(const EditorContext&);
    void invalidateMatch();
    QByteArray matchSignature() const;
    bool propertiesEditing_ = false;
    QWidget* makePlaceholder(const QString& title, const QString& description);
    void selectPage(const QString& id);
    void saveLayout();
    void restoreLayout();
    void restoreSession();
    QString settingsFile_;
    QListWidget* navigation_{};
    WorkspacePanel* workspacePanel_{};
    QStackedWidget* pages_{};
    QLabel* pageTitle_{};
    QSplitter* horizontal_{};
    QSplitter* vertical_{};
    QStringList pageIds_;
    struct PageRefreshState {
        quint64 generation=0, renderedGeneration=0;
        QString reason;
        void invalidate(const QString& why){++generation;reason=why;}
        void rendered(){renderedGeneration=generation;reason.clear();}
    };
    PageRefreshState calibrationRefresh_,runtimeRefresh_;
    ProjectService service_;
    QLabel* context_{};
    QLabel* projectSummary_{};
    QLabel* editorContext_{};
    QLabel* workspaceError_{};
    QLabel* recipeCount_{};
    QLabel* dashboardRecipe_{};
    QTableWidget* recipeTable_{};
    QComboBox* typeFilter_{};
    QLineEdit* recipeSearch_{};
    QTreeWidget* projectTree_{};
    QAction* saveAction_{};
    bool smokeMode_ = false;
    EditorSession editorSession_;
    EditorCameraSessions editorCameras_;
    QString editorCameraRecipeKey_;
    QMap<QString,QString> editorCameraErrors_;
    std::optional<IndustrialExecutionInputs> editorCameraComposition_; // in-memory smoke composition only
    std::optional<QString> editorCameraPendingExecution_;
    bool editorCameraResume_=false,editorCameraExitAccepted_=false;
    std::function<void()> editorCameraAfterDrain_;
    CameraSession cameraSession_;
    EditorContext editorContext() const;
    std::optional<MeasurementEnvironment> editorDisplayEnvironment() const;
    std::optional<ActiveCalibrationSnapshot> systemCalibrationSnapshot(const Recipe* recipe=nullptr) const;
    CalibrationResolver calibrationResolver(const Recipe&) const;
    QJsonObject systemCalibrationIdentity() const;
    QString calibrationSummary(const EditorContext* context=nullptr) const;
    QString imageBinding_;
    QString editorRecipeId_;
    QString selectedToolId_;
    ImageViewer* imageViewer_{};
    EdgeProfileView *edgeProfile_{},*runtimeEdgeProfile_{};
    QListWidget* toolbox_{};
    QListWidget* flowTools_{};
    QWidget* propertiesBody_{};
    QLabel* imageInfo_{};
    QLabel* pixelInfo_{};
    QLabel* zoomInfo_{};
    QLabel* matchInfo_{};
    bool modelDrawing_=false;
    QString drawingTool_;
    QLabel* graphInfo_{};
    CameraManager* cameraManager_{};
    CalibrationOverview* calibrationOverview_{};
    std::shared_ptr<MeasurementWorkspace> measurementWorkspace_;
    QString measurementWorkspaceKey_;
    QJsonObject editorCalibrationIdentity_;
    JobSession runtimeJobs_;
    std::unique_ptr<RuntimeService> runtime_;
    std::optional<IndustrialExecutionInputs> runtimeCameraComposition_; // in-memory fixture composition only
    std::unique_ptr<RuntimeCoordinator> runtimeCoordinator_;
    std::unique_ptr<PlcRuntimeSocket> plc_;
    QLineEdit* plcAddress_{};
    QComboBox* plcMapping_{};
    QLabel* plcStatus_{};
    QLabel* hardwareStatus_{};
    bool runtimeSourcesReady(QString* reason=nullptr) const;
    QString runtimeVerificationError_,runtimeWorkspaceKey_;
    IndependentCaptureBackend runtimeVerificationBackend_;
    ExecutionService::SourceAdapterFactory runtimeSourceFactory_;
    QString runtimePath_;
    ProductionCycle runtimeCycle_;
    std::optional<CycleResult> runtimeResult_,runtimePending_;
    ImageViewer* runtimeViewer_{};
    QLabel* runtimeStatus_{};
    QLabel* runtimeSummary_{};
    QComboBox* runtimeTools_{};
    QComboBox* runtimeSources_{};
    QTableWidget* runtimePreparation_{};
    void refreshRuntimeSources();
    QTableWidget* logTable_{};
    QLabel* logStatus_{};
};
}
