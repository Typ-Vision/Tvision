#include "MainWindow.hpp"
#include "CalibrationOverview.hpp"
namespace visionflow {
QWidget* MainWindow::makeCalibration() {
    calibrationOverview_=new CalibrationOverview([this]{return measurementWorkspace();});
    calibrationOverview_->setLocalSettings(settingsFile_);
    calibrationOverview_->onCameraManager=[this]{selectPage("camera");};
    calibrationOverview_->setRecipes([this]{return service_.hasProject() ? service_.project().recipes:QVector<Recipe>{};});
    calibrationOverview_->onChanged=[this]{refreshFlow();refreshMatchOverlay();runtimeResult_.reset();showRuntimeResult();refreshRuntime();};
    return calibrationOverview_;
}
MeasurementWorkspace* MainWindow::measurementWorkspace(){
        const auto key=service_.hasProject() ? service_.path()+"\n"+service_.project().id:QString{};
        if(key!=measurementWorkspaceKey_){measurementWorkspace_.reset();measurementWorkspaceKey_.clear();
            if(service_.hasProject() && !service_.path().isEmpty()){
                measurementWorkspace_=std::make_shared<MeasurementWorkspace>(service_.path(),service_.project().id,[this]{return !editorSession_.busy() && !cameraSession_.busy() && !runtimeJobs_.busy() && !(runtime_ && runtime_->busy()) && !(runtimeCoordinator_ && (runtimeCoordinator_->state()==RuntimeState::Running || runtimeCoordinator_->state()==RuntimeState::Starting || runtimeCoordinator_->state()==RuntimeState::Stopping)) && !(plc_ && plc_->blocking());});
                measurementWorkspaceKey_=key;
            }
        }return measurementWorkspace_.get();
}
void MainWindow::refreshCalibration() {
    if(calibrationOverview_)calibrationOverview_->refresh();
}
}
