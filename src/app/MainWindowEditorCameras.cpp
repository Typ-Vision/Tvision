#include "MainWindow.hpp"
#include "application/AcquisitionPolicy.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include <QLabel>
#include <QStackedWidget>
#include <QTimer>

namespace visionflow {
void MainWindow::syncEditorCameras(const QString& retry,bool activate,const MeasurementEnvironment* displayEnvironment){
    if(editorCameraExitAccepted_)return;
    if(!service_.activeRecipe())return;
    const auto key=service_.path()+"\n"+service_.project().id+"\n"+service_.activeRecipe()->id;
    if(!activate && !editorCameras_.active())return;
    if(!activate && key!=editorCameraRecipeKey_){editorCameras_.stop();return;}
    if(!measurementWorkspace())return;
    editorCameraRecipeKey_=key;editorCameraErrors_.clear();
    IndustrialExecutionInputs in=editorCameraComposition_.value_or(IndustrialExecutionInputs{});
    const auto environment=displayEnvironment ? *displayEnvironment:measurementWorkspace_->environment();in.contract=environment.current;
    if(!editorSession_.busy())service_.refreshEditorCameraSources(environment);
    for(const auto& camera:in.contract.cameras)if(camera.physical)if(const auto route=CameraLocalConfiguration(settingsFile_).route(*camera.physical))in.routes[camera.id.value]=*route;
    QVector<EditorCameraTarget> targets;
    for(const auto& tool:service_.tools())if(tool.enabled && tool.kind=="industrial_camera")try{
        auto request=industrialExecutionRequest(in,tool);
        if(environment.archivedCameras.contains(request.camera.id.value) || environment.archivedSetups.contains(request.setup.id.value))throw std::runtime_error("Kamera veya kurulum arşivlenmiş.");
        targets.append({request,AcquisitionPolicy::camera(request.camera)});
    }catch(const std::exception& e){editorCameraErrors_[tool.id]=QString::fromUtf8(e.what());}
    try{editorCameras_.reconcile(key,in,std::move(targets),retry);}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
}
QString MainWindow::editorCameraStatus(const QString& source) const{
    if(editorCameraErrors_.contains(source))return "Kamera hazır değil — "+editorCameraErrors_[source];
    const auto state=editorCameras_.source(source);
    if(!state)return "Kamera hazır değil — Görüntü al ile hazırlayın.";
    switch(state->state){
    case AcquisitionSessionState::Ready:return "Kamera hazır";
    case AcquisitionSessionState::Capturing:return "Görüntü alınıyor…";
    case AcquisitionSessionState::Opening:case AcquisitionSessionState::Configuring:return "Kamera bağlantısı hazırlanıyor…";
    case AcquisitionSessionState::Closing:return "Kamera bağlantısı kapatılıyor…";
    default:return "Kamera hazır değil"+(state->error.isEmpty()?QString{}:" — "+state->error);
    }
}
void MainWindow::pollEditorCameras(){
    if(runtimeCoordinator_ && runtimeCoordinator_->poll())refreshRuntime();
    if(editorCameras_.poll())refreshFlow();
    if(editorCameraAfterDrain_ && !editorCameras_.draining() && !editorSession_.busy()){
        auto after=std::move(editorCameraAfterDrain_);editorCameraAfterDrain_={};after();return;
    }
    if(editorCameraPendingExecution_ && !editorCameras_.pending(*editorCameraPendingExecution_) && !editorSession_.busy()){
        const auto target=*editorCameraPendingExecution_;editorCameraPendingExecution_.reset();editorCameraResume_=true;runExecution(target);
    }
}
bool MainWindow::drainEditorCameras(std::function<void()> after){
    if(!editorCameras_.hasTargets() && !editorCameras_.draining() && !editorCameras_.pending()){editorCameras_.stop();return false;}
    if(editorCameraAfterDrain_)return true;
    editorCameraPendingExecution_.reset();editorSession_.stop(SessionJob::Execution);editorSession_.stop(SessionJob::Teaching);editorSession_.stop(SessionJob::Image);++editorSession_.generation;
    editorCameras_.stop();editorCameraAfterDrain_=std::move(after);refreshFlow();return true;
}
void MainWindow::openEditorRecipe(const QString& id){
    if(id.isEmpty() || editorCameraAfterDrain_)return;
    if(service_.activeRecipe() && service_.activeRecipe()->id!=id){
        if(!resolveUnsavedChanges())return;
        if(drainEditorCameras([this,id]{openEditorRecipe(id);}))return;
    }else if(!applyToolProperties())return;
    try{service_.selectRecipe(id);refreshWorkspace();selectPage("editor");syncEditorCameras("*",true);refreshFlow();}
    catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
}
}
