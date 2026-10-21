#pragma once
#include "application/MeasurementWorkspace.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include <QWidget>
class QCheckBox;class QTableWidget;class QLabel;class QLineEdit;class QPushButton;class QComboBox;class QSpinBox;class QTimer;
namespace visionflow {
class CameraManager final:public QWidget {
public:
    using Scanner=std::function<CameraInventory(const QStringList&,const std::shared_ptr<std::atomic_bool>&)>;
    CameraManager(std::function<MeasurementWorkspace*()>,std::function<bool()>,CameraSession&,QString localSettings,
                  QWidget* parent=nullptr,Scanner scanner={},CameraProviderFactory factory={});
    ~CameraManager() override;
    void refresh();
    void scan();
    void checkConnection();
    void stop();
    const CameraInventory& inventory() const {return inventory_;}
    std::function<void()> onChanged;
private:
    void attempt(std::function<void()>);
    void select();
    void registerSelected();
    void editProfile();
    void start(bool preview);
    void tick();
    void controls();
    void saveFrame();
    IndustrialCameraRequest request() const;
    std::function<MeasurementWorkspace*()> context_;
    std::function<bool()> gate_;
    CameraSession& jobs_;
    CameraLocalConfiguration local_;
    Scanner scanner_;
    CameraProviderFactory factory_;
    CameraInventory inventory_;
    struct Row {std::optional<MeasurementCamera> camera;std::optional<CameraDiscoveryEntry> discovery;};
    QVector<Row> rows_;
    QTableWidget* table_;
    QLabel *status_,*detail_,*image_,*readback_;
    QLineEdit *producer_,*name_,*exposure_,*gain_;
    QComboBox *routes_,*profiles_;
    QSpinBox* timeout_;
    QPushButton *scan_,*register_,*apply_,*preview_,*stop_,*check_;
    QTimer* timer_;
    QCheckBox* experimental_;
    QLineEdit* runtime_;
    QLabel* experimentalNotice_;
    QPushButton* save_;
    bool experimentalSession_=false;
    std::optional<AcquiredFrame> lastFrame_;
    std::unique_ptr<CameraWorkerSession> worker_;
    quint64 job_=0;
    bool previewing_=false,closing_=false;
};
}
