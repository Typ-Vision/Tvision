#pragma once
#include <QMainWindow>
#include <QStringList>
#include "application/ProjectService.hpp"
#include "application/ImageFileService.hpp"
#include "application/MatchService.hpp"
#include "application/ExecutionService.hpp"
#include "application/CameraService.hpp"
#include "application/EditorSession.hpp"
#include "application/CalibrationWorkflow.hpp"
#include "application/RuntimeService.hpp"
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
namespace visionflow {
class ImageViewer;
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(const QString& settingsFile);
    bool verifyShell();
    bool verifyWorkspace(const QString& artifactDirectory);
    bool verifyEditor(const QString& artifactDirectory);
    bool verifySession();
    bool verifyMatching(const QString& artifacts);
    bool verifyExecution(const QString& artifacts);
    bool verifyCamera(const QString& artifacts);
    bool verifyWebcam(const QString& artifacts);
    bool verifyMeasurement(const QString& artifacts);
    bool verifyRuntime(const QString& artifacts);
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    QWidget* makeDashboard();
    QWidget* makeCameraManager();
    void discoverCameras();
    void checkCameraConnection();
    void updateCameraControls();
    QWidget* makeEditor();
    QWidget* makeCalibration();
    QWidget* makeRuntime();
    QWidget* makeLogs();
    void refreshRuntime();
    void runtimeAction(const QString&);
    void showRuntimeResult();
    void refreshLogs();
    void ensureRuntime();
    void refreshCalibration();
    void calibrationAction(const QString& action);
    void invalidateCalibrationDraft();
    QWidget* makeProjectManager();
    QWidget* makeRecipeManager();
    void showProjectDialog(bool edit = false);
    void openProjectDialog();
    void showRecipeDialog(bool edit = false);
    void openSelectedRecipe();
    void refreshWorkspace();
    void refreshRecipes();
    void saveProject();
    bool resolveUnsavedChanges();
    void displayError(const QString& message);
    void refreshEditor();
    void selectTool(const QString& id);
    void showToolProperties();
    void addSelectedTool();
    bool applyToolProperties();
    void openImageDialog();
    bool loadEditorImage(const QString& path);
    void beginRoi(bool model);
    void commitRoi(const QRect& roi);
    std::optional<QPointF> referencePoint(const QString& source) const;
    void runMatch();
    QByteArray referenceSignature(const QString& toolId) const;
    void runFlow();
    void runExecution(const QString& targetTool);
    void refreshFlow();
    void showExecutionResult();
    QByteArray flowSignature() const;
    void loadImageAsync(const QString& path,bool commit);
    void refreshMatchOverlay();
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
    QStackedWidget* pages_{};
    QLabel* pageTitle_{};
    QSplitter* horizontal_{};
    QSplitter* vertical_{};
    QStringList pageIds_;
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
    CameraSession cameraSession_;
    EditorContext editorContext() const;
    QString imageBinding_;
    QString editorRecipeId_;
    QString selectedToolId_;
    ImageViewer* imageViewer_{};
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
    QLineEdit* cameraProducer_{};
    QLabel* cameraStatus_{};
    QTableWidget* cameraTable_{};
    QVector<CameraDevice> cameraDevices_;
    ImageViewer* calibrationViewer_{};
    QLabel* calibrationFrameInfo_{};
    QLabel* calibrationPixelInfo_{};
    QWidget* calibrationControls_{};
    QLabel* calibrationStatus_{};
    QLineEdit* calibrationId_{};
    QLineEdit* calibrationLens_{};
    QLineEdit* calibrationPlane_{};
    QTableWidget* calibrationSamples_{};
    QString calibrationRecipeId_,calibrationFrameId_;
    quint64 calibrationGeneration_=0;
    std::optional<PlanarScaleCalibration> calibrationCandidate_;
    JobSession runtimeJobs_;
    std::unique_ptr<RuntimeService> runtime_;
    QString runtimePath_;
    ProductionCycle runtimeCycle_;
    std::optional<CycleResult> runtimeResult_,runtimePending_;
    ImageViewer* runtimeViewer_{};
    QLabel* runtimeStatus_{};
    QLabel* runtimeSummary_{};
    QComboBox* runtimeTools_{};
    QComboBox* runtimeHistory_{};
    QTableWidget* logTable_{};
    QLabel* logStatus_{};
};
}
