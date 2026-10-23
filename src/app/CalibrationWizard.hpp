#pragma once
#include "application/CalibrationCapture.hpp"
#include "application/IndependentCalibrationCapture.hpp"
#include "application/EditorSession.hpp"
#include "application/ScopedActiveCalibration.hpp"
#include <QDialog>
class QLabel; class QPushButton; class QStackedWidget; class QLineEdit; class QComboBox; class QListWidget; class QDoubleSpinBox; class QSpinBox; class QCheckBox;
namespace visionflow {
class ImageViewer;
class CalibrationWizard : public QDialog {
public:
    CalibrationWizard(bool plate,std::optional<ToolDefinition> source,QString directory,
        std::function<void(const CalibrationModelRecord&)> save,QWidget* parent=nullptr);
    CalibrationWizard(bool plate,IndependentCaptureRequest target,QString directory,
        std::function<void(const CalibrationModelRecord&)> save,QWidget* parent=nullptr,IndependentCaptureBackend backend={});
    ~CalibrationWizard();
    // Same commands used by the file chooser and UI acceptance tests.
    void openFile(const QString&);
    using SaveUse = std::function<CalibrationSaveUseResult(const CalibrationModelRecord&,const IndependentCalibrationSession&)>;
    void setSaveAndUse(SaveUse command);
private:
    void captureIndependent(QString path={});
    void saveAndUse(const CalibrationModelRecord&);
    void showSaveResult(const CalibrationSaveUseResult&);
    SaveUse saveUse_;
    IndependentCalibrationSession activationProbe_;
    bool savedAndUsed_=false;
    std::optional<CalibrationModelRecord> namedRecord_;
    QString namedBase_;
    std::optional<IndependentCaptureRequest> independentTarget_;
    IndependentCaptureBackend backend_;
    IndependentCalibrationSession independent_;
    std::optional<MeasurementSetup> independentSetup_;
    QString independentOrigin_;
    void attempt(std::function<void()>);
    void refresh();
    void presentFrame(ImageFrame,bool imported);
    void detect();
    void invalidatePlate();
    CalibrationGridTarget plateTarget(QString id) const;
    std::optional<QRect> searchRoi_;
    std::optional<FramedPlateGeometry> framedGeometry_;
    QCheckBox* spacingConfirmed_;
    QLabel* plateDiagnostics_;
    void displayPoints();
    void selectPair();
    void advance();
    void cancelWork(bool closeCamera=true);
    void pollCamera();
    void done(int) override;
    std::optional<int> pendingDone_;
    void reject() override;
    bool plateMode_,imported_=false;
    bool checking_=false;
    bool pendingControl_=false;
    std::optional<CalibrationModelRecord> resultRecord_;
    std::optional<WizardScanResult> lastResult_;
    int step_=0;
    QString directory_,lastScan_,origin_;
    std::optional<ToolDefinition> source_;
    std::optional<ImageFrame> pendingFrame_;
    std::optional<MeasurementSetup> setup_;
    std::function<void(const CalibrationModelRecord&)> save_;
    CalibrationWizardSession session_;
    JobSession jobs_;
    std::shared_ptr<std::atomic_bool> captureCancel_;
    CalibrationControl controls_;
    ImageViewer* viewer_;
    QStackedWidget* pages_;
    QLabel *heading_,*status_,*imageInfo_,*result_,*distance_;
    QPushButton *back_,*next_;
    QLineEdit *installation_,*lens_,*plane_,*name_;
    QComboBox *usage_,*method_,*pattern_,*candidates_,*polarity_;
    QListWidget* points_;
    QDoubleSpinBox *known_,*spacing_,*contrast_,*controlKnown_,*limit_;
    QSpinBox *columns_,*rows_;
    QCheckBox* limited_;
    QComboBox* selection_;
    QDoubleSpinBox *sampling_,*sigma_,*stripWidth_;
    QCheckBox* subpixel_;
};
}
