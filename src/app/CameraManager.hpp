#pragma once
#include "application/MeasurementWorkspace.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include "application/CameraPreviewSession.hpp"
#include "application/ExperimentalOecsAcquisition.hpp"
#include <QWidget>
class QCheckBox;class QTableWidget;class QLabel;class QLineEdit;class QPushButton;class QComboBox;class QSpinBox;class QTimer;
namespace visionflow {
class CameraManager final:public QWidget {
public:
    using Scanner=std::function<CameraInventory(const QStringList&,const std::shared_ptr<std::atomic_bool>&)>;
    CameraManager(std::function<MeasurementWorkspace*()>,std::function<bool()>,CameraSession&,QString localSettings,
                  QWidget* parent=nullptr,Scanner scanner={},CameraProviderFactory factory={},
                  ExperimentalOecsAcquisition::ApiFactory experimentalApi=makeOecsCApi,
                  std::function<QString(QWidget*)> framePathChooser={});
    ~CameraManager() override;
    void refresh();
    void scan();
    void checkConnection();
    void stop();
    // Latest exit intent runs on the UI timer only after the shared job and worker drain.
    void leave(std::function<void()>);
    bool hasPreview() const {return previewActive_ || lastFrame_.has_value();}
    const CameraInventory& inventory() const {return inventory_;}
    std::function<void()> onChanged, onProject;
private:
    void attempt(std::function<void()>);
    void select();
    void registerSelected();
    void editProfile();
    void settings(bool apply);
    void settingsReadback();
    void start(bool live);
    void tick();
    void pollJobs();
    void clearFrame();
    void controls();
    void saveFrame();
    IndustrialCameraRequest request() const;
    std::function<MeasurementWorkspace*()> context_;
    std::function<bool()> gate_;
    CameraSession& jobs_;
    CameraLocalConfiguration local_;
    Scanner scanner_;
    CameraProviderFactory factory_;
    ExperimentalOecsAcquisition::ApiFactory experimentalApi_;
    std::function<QString(QWidget*)> framePathChooser_;
    CameraInventory inventory_;
    struct Row {std::optional<MeasurementCamera> camera;std::optional<CameraDiscoveryEntry> discovery;};
    QVector<Row> rows_;
    QTableWidget* table_;
    QLabel *status_,*detail_,*image_,*readback_,*previewStatus_;
    QLineEdit *producer_,*shortId_,*name_,*exposure_,*gain_;
    QWidget *advancedBody_;
    QPushButton *settingsRead_;
    std::unique_ptr<CameraSettingsSession> settings_;
    CameraSettingsValues desired_;
    bool settingsApply_=false, settingsClosing_=false, settingsKnown_=false;
    bool exposureWritable_=false, gainWritable_=false;
    QSize settingsGeometry_;
    QString selectedKey_;
    QComboBox *routes_,*profiles_;
    QSpinBox* timeout_;
    QPushButton *scan_,*register_,*apply_,*preview_,*live_,*stop_,*check_;
    QTimer* timer_;
    QLineEdit* runtime_;
    QPushButton* save_;
    bool experimentalSession_=false;
    std::optional<AcquiredFrame> lastFrame_;
    CameraPreviewSession previewSession_;
    bool previewActive_=false, liveMode_=false;
    quint64 frameRevision_=0;
    bool frameDialogOpen_=false;
    std::function<void()> afterStop_;
    quint64 job_=0;
    bool closing_=false;
};
}
