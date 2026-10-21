#include "MainWindow.hpp"
#include "CameraManager.hpp"
#include "CalibrationOverview.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDir>
#include <QThread>
#include <QStatusBar>
#include <QTableWidget>
#include <stdexcept>
namespace visionflow {
QWidget* MainWindow::makeCameraManager(){
    cameraManager_=new CameraManager([this]{return measurementWorkspace();},[this]{
        return !editorSession_.busy() && !runtimeJobs_.busy() && !(runtime_ && runtime_->busy()) &&
          !(runtimeCoordinator_ && (runtimeCoordinator_->state()==RuntimeState::Running || runtimeCoordinator_->state()==RuntimeState::Starting || runtimeCoordinator_->state()==RuntimeState::Stopping)) && !(plc_ && plc_->blocking());
    },cameraSession_,settingsFile_);
    cameraManager_->onProject=[this]{selectPage("projects");};
    cameraManager_->onChanged=[this]{if(calibrationOverview_)calibrationOverview_->refresh();};
    return cameraManager_;
}
void MainWindow::discoverCameras(){cameraManager_->scan();}
void MainWindow::checkCameraConnection(){cameraManager_->checkConnection();}
bool MainWindow::verifyCamera(const QString& artifacts){
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto wait=[&]{QElapsedTimer timer;timer.start();while(cameraSession_.busy() && timer.elapsed()<10000){QApplication::processEvents();QThread::msleep(1);}check(!cameraSession_.busy(),"camera worker completed");};
    selectPage("camera");auto* producer=cameraManager_->findChild<QLineEdit*>("cameraProducer");
    auto* status=cameraManager_->findChild<QLabel*>("cameraStatus");
    producer->setText(QDir(artifacts).filePath("missing-producer.cti"));cameraManager_->scan();
    check(cameraSession_.busy(),"camera scan background");check(!close(),"camera close guard");wait();
    check(cameraManager_->inventory().industrial.devices.isEmpty() && status->text().contains(".cti"),"missing producer error");workspaceError_->hide();statusBar()->clearMessage();
    producer->setText(QDir(artifacts).filePath("gentl_test_producer.cti"));cameraManager_->scan();wait();
    check(cameraManager_->inventory().industrial.devices.size()==1,"UI synthetic discovery");
    auto* table=cameraManager_->findChild<QTableWidget*>("cameraDevices");
    const auto device=cameraManager_->inventory().industrial.devices.front();
    for(int row=0;row<table->rowCount();++row)if(table->item(row,1)->text()==device.identity.model && table->item(row,2)->text()==device.identity.serial)table->selectRow(row);
    cameraManager_->checkConnection();check(cameraSession_.busy(),"UI read-only check runs on worker");wait();
    check(status->text().contains("Cihaz kontrol sonunda kapatıldı"),"UI read-only check closes device");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("step6-camera-test.png"));
    cameraManager_->scan();cameraManager_->stop();wait();check(cameraManager_->inventory().cancelled,"UI cancel clears discovery");
    producer->setText(QDir(artifacts).filePath("missing-producer.cti"));cameraManager_->scan();wait();
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("step6-camera-no-producer.png"));return true;
}
}
