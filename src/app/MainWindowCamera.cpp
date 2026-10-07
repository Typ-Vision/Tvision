#include "MainWindow.hpp"
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <QApplication>
#include <QElapsedTimer>
#include <QDir>
#include <QThread>
#include <QStatusBar>
#include <stdexcept>
namespace visionflow {
QWidget* MainWindow::makeCameraManager() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);
    auto* note=new QLabel("GenTL kamera keşfi ve salt okunur bağlantı kontrolü. Canlı görüntü, trigger, exposure/gain ve reçeteye kamera bağlama henüz uygulanmadı.");
    note->setWordWrap(true);note->setObjectName("muted");layout->addWidget(note);
    auto* row=new QHBoxLayout;
    cameraProducer_=new QLineEdit;cameraProducer_->setObjectName("cameraProducer");cameraProducer_->setPlaceholderText(".cti producer yolu; boşsa GENICAM_GENTL64_PATH");row->addWidget(cameraProducer_,1);
    auto* choose=new QPushButton("Producer seç");choose->setObjectName("cameraChoose");row->addWidget(choose);
    auto* scan=new QPushButton("Cihazları ara");scan->setObjectName("cameraScan");row->addWidget(scan);
    auto* stop=new QPushButton("Aramayı durdur");stop->setObjectName("cameraStop");row->addWidget(stop);layout->addLayout(row);
    cameraStatus_=new QLabel("Henüz arama yapılmadı. Bir üreticinin Windows x64 GenTL producer/sürücü kurulumu gerekir.");cameraStatus_->setObjectName("cameraStatus");cameraStatus_->setWordWrap(true);cameraStatus_->setTextFormat(Qt::PlainText);layout->addWidget(cameraStatus_);
    cameraTable_=new QTableWidget(0,6);cameraTable_->setObjectName("cameraDevices");cameraTable_->setHorizontalHeaderLabels({"Marka","Model","Seri no","Transport","Interface ID","Device ID"});
    cameraTable_->setSelectionBehavior(QAbstractItemView::SelectRows);cameraTable_->setSelectionMode(QAbstractItemView::SingleSelection);cameraTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);cameraTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);cameraTable_->horizontalHeader()->setStretchLastSection(true);layout->addWidget(cameraTable_,1);
    auto* check=new QPushButton("Seçili cihazı salt okunur aç / kapat");check->setObjectName("cameraCheck");layout->addWidget(check);
    connect(choose,&QPushButton::clicked,this,[this]{if(cameraSession_.busy())return;const auto file=QFileDialog::getOpenFileName(this,"Windows x64 GenTL producer",{},"GenTL producer (*.cti)");if(!file.isEmpty())cameraProducer_->setText(file);});
    connect(scan,&QPushButton::clicked,this,&MainWindow::discoverCameras);
    connect(stop,&QPushButton::clicked,this,[this]{if(cameraSession_.token(SessionJob::Discovery) && cameraSession_.busy()){cameraSession_.stop(SessionJob::Discovery);cameraStatus_->setText("İptal istendi; producer çağrısının bitmesi bekleniyor.");}});
    connect(check,&QPushButton::clicked,this,&MainWindow::checkCameraConnection);
    connect(cameraTable_,&QTableWidget::itemSelectionChanged,this,&MainWindow::updateCameraControls);
    updateCameraControls();
    return page;
}
void MainWindow::updateCameraControls() {
    // During page construction the buttons are not yet children of the main window.
    auto* page=cameraTable_->parentWidget();
    page->findChild<QPushButton*>("cameraScan")->setEnabled(!cameraSession_.busy());
    page->findChild<QPushButton*>("cameraChoose")->setEnabled(!cameraSession_.busy());
    page->findChild<QPushButton*>("cameraCheck")->setEnabled(!cameraSession_.busy() && cameraTable_->currentRow()>=0);
    page->findChild<QPushButton*>("cameraStop")->setEnabled(cameraSession_.busy() && bool(cameraSession_.token(SessionJob::Discovery)));
    cameraProducer_->setEnabled(!cameraSession_.busy());
}
void MainWindow::discoverCameras() {
    if(cameraSession_.busy())return;
    auto* watcher=cameraSession_.watch<CameraDiscovery>(this,SessionJob::Discovery);cameraDevices_.clear();cameraTable_->setRowCount(0);cameraStatus_->setText("GenTL cihazları aranıyor…");
    updateCameraControls();
    const auto file=cameraProducer_->text().trimmed();const auto cancel=cameraSession_.token(SessionJob::Discovery);
    connect(watcher,&QFutureWatcher<CameraDiscovery>::finished,this,[this,watcher]{
        const auto report=watcher->result();watcher->deleteLater();
        cameraDevices_=report.devices;cameraTable_->setRowCount(cameraDevices_.size());
        for(int row=0;row<cameraDevices_.size();++row){const auto& d=cameraDevices_[row];const QStringList values={d.vendor,d.model,d.serialNumber,d.transport,d.interfaceId,d.deviceId};
            for(int column=0;column<values.size();++column){auto* item=new QTableWidgetItem(values[column]);item->setToolTip(d.producerPath);cameraTable_->setItem(row,column,item);}}
        QString status=report.cancelled ? "Arama iptal edildi; cihaz listesi temizlendi." : report.producers.isEmpty() ? "GenTL producer bulunamadı. Producer seçin veya GENICAM_GENTL64_PATH tanımlayın." : QString("%1 producer yolu, %2 cihaz. Keşif bağlantı veya görüntü edinim kabulü değildir.").arg(report.producers.size()).arg(report.devices.size());
        if(!report.errors.isEmpty())status+="\nKeşif hatası: "+report.errors.join("\n");cameraStatus_->setText(status);updateCameraControls();
    });
    cameraSession_.launch(watcher,[file,cancel]{return CameraService{}.discover(file.isEmpty() ? QStringList{} : QStringList{file},1000,cancel);});
}
void MainWindow::checkCameraConnection() {
    if(cameraSession_.busy())return;
    const auto row=cameraTable_->currentRow();if(row<0 || row>=cameraDevices_.size()){cameraStatus_->setText("Bağlantı kontrolü için bir cihaz seçin.");return;}
    const auto device=cameraDevices_[row];auto* watcher=cameraSession_.watch<QString>(this,SessionJob::Connection);cameraStatus_->setText("Cihaz salt okunur açılıp kapatılıyor…");
    updateCameraControls();
    connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,device]{const auto error=watcher->result();watcher->deleteLater();
        cameraStatus_->setText(error.isEmpty() ? "Salt okunur aç/kapat tamamlandı: "+device.deviceId+". Cihaz kontrol sonunda kapatıldı; görüntü alınmadı." : "Bağlantı kontrolü başarısız: "+error);updateCameraControls();});
    cameraSession_.launch(watcher,[device]{try{CameraService{}.checkConnection(device);return QString{};}catch(const std::exception& e){return QString::fromUtf8(e.what());}});
}
bool MainWindow::verifyCamera(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto wait=[&]{QElapsedTimer timer;timer.start();while(cameraSession_.busy() && timer.elapsed()<10000){QApplication::processEvents();QThread::msleep(1);}check(!cameraSession_.busy(),"camera worker completed");};
    selectPage("camera");cameraProducer_->setText(QDir(artifacts).filePath("missing-producer.cti"));
    findChild<QPushButton*>("cameraScan")->click();check(cameraSession_.busy(),"camera scan background");check(!close(),"camera close guard");wait();check(cameraDevices_.isEmpty() && cameraStatus_->text().contains(".cti"),"missing producer error no devices");workspaceError_->hide();statusBar()->clearMessage();
    cameraProducer_->setText(QDir(artifacts).filePath("gentl_test_producer.cti"));findChild<QPushButton*>("cameraScan")->click();wait();check(cameraDevices_.size()==1 && cameraTable_->rowCount()==1,"UI synthetic producer discovery");
    cameraTable_->selectRow(0);findChild<QPushButton*>("cameraCheck")->click();check(cameraSession_.busy(),"camera check background");wait();check(cameraStatus_->text().contains("Cihaz kontrol sonunda kapatıldı"),"UI readonly check");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("step6-camera-test.png"));
    findChild<QPushButton*>("cameraScan")->click();findChild<QPushButton*>("cameraStop")->click();wait();check(cameraDevices_.isEmpty() && cameraStatus_->text().contains("iptal"),"UI cancelled scan clears devices");
    cameraProducer_->setText(QDir(artifacts).filePath("missing-producer.cti"));findChild<QPushButton*>("cameraScan")->click();wait();QApplication::processEvents();grab().save(QDir(artifacts).filePath("step6-camera-no-producer.png"));
    return true;
}
}
