#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include "../../tests/CameraAcquisitionFixture.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QLabel>
#include <QPushButton>
#include <QSaveFile>
#include <QTemporaryDir>

namespace visionflow {
bool MainWindow::verifyRuntimeCameraSessions(const QString& artifacts){
    using persistent_fixture::check;using persistent_fixture::Fixture;smokeMode_=true;
    auto wait=[&](auto condition){QElapsedTimer t;t.start();while(!condition()){check(t.elapsed()<15000,"Runtime UI fixture timeout");QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();};
    auto settle=[&]{wait([&]{return runtimeCoordinator_ && !runtimeJobs_.busy() && !runtime_->camerasPending() && runtimeCoordinator_->state()!=RuntimeState::Starting && runtimeCoordinator_->state()!=RuntimeState::Stopping;});};
    QTemporaryDir dir(QDir(artifacts).filePath("runtime-camera-ui-XXXXXX"));check(dir.isValid(),"Runtime UI fixture directory");QJsonArray rows;
    auto install=[&](Fixture& fixture,ApplicationType type,const QString& name){
        if(runtimeCoordinator_){runtimeAction("stop");settle();}
        runtimeCameraComposition_=fixture.inputs;service_.createProject(name,"",dir.filePath(name+".vfproject"),"file");service_.createRecipe("Camera run","",type,"file");measurementWorkspace()->replace(fixture.inputs.contract);
        for(const auto& request:fixture.requests){const auto source=service_.addTool("industrial_camera");service_.bindIndustrialSource(source,fixture.inputs.contract,request.profile.id,3000);}
        service_.save();refreshWorkspace();ensureRuntime();runtimeCoordinator_->load(*service_.activeRecipe(),dir.path());selectPage("runtime");refreshRuntime();
    };
    for(int count:{1,5})for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        const auto name=QString("runtime%1-%2").arg(count).arg(typeKey(type));Fixture f(name,count);install(f,type,name);check(*f.discoveries==0 && !runtimeResult_ && !runtimeViewer_->hasImage(),"Load opens no camera and shows no invented result");
        runtimeAction("run");check(runtimeCoordinator_->state()==RuntimeState::Starting && findChild<QPushButton*>("runtime_stop")->isEnabled() && !findChild<QPushButton*>("runtime_run")->isEnabled(),"Starting is responsive and can Stop");settle();
        check(runtimeCoordinator_->state()==RuntimeState::Running && !runtimeResult_ && !runtimeViewer_->hasImage(),"Running accepted only after prepared set; no Run capture");
        for(int i=0;i<20;++i){runtimeAction("cycle");wait([&]{return !runtimeJobs_.busy();});check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && runtimeResult_->frames.size()==count,"Runtime UI actual source-set cycle");}
        for(const auto& p:f.probes)check(p->opens==1 && p->configs==1 && p->grabs==20 && p->closes==0,"UI cycles reuse camera connection");
        if(count==5 && type==ApplicationType::Measurement){setWindowState(Qt::WindowNoState);resize(1400,900);QApplication::processEvents();check(grab().save(QDir(artifacts).filePath("runtime-cameras-normal.png")),"normal Runtime visual");resize(1000,700);QApplication::processEvents();check(grab().save(QDir(artifacts).filePath("runtime-cameras-small.png")),"small Runtime visual");}
        runtimeAction("stop");settle();check(runtimeCoordinator_->state()==RuntimeState::LoadedStopped && !runtimeResult_ && !runtimeViewer_->hasImage() && !ProductionResultCodec::csv(runtime_->records()).isEmpty(),"Idle Stop drains and keeps history/CSV");
        for(const auto& p:f.probes)check(p->closes==1 && !p->wrongThread,"UI stop closes every camera on its SDK thread");
        rows.append(QJsonObject{{"type",typeKey(type)},{"cameras",count},{"cycles",20},{"openPerCamera",1},{"configurePerCamera",1},{"closePerCamera",1}});
    }
    {
        Fixture f("runtime-ui-late");install(f,ApplicationType::Measurement,"late");struct Gate{std::promise<void> promise;bool released=false;void release(){if(!released){released=true;promise.set_value();}}~Gate(){release();}}gate;
        f.first()->openGate=gate.promise.get_future().share();runtimeAction("run");wait([&]{return f.first()->openEntered.load();});check(runtimeStatus_->text().contains("Hazırlanıyor"),"Starting status remains visible during late open");runtimeAction("stop");check(runtimeCoordinator_->state()==RuntimeState::Stopping && !findChild<QPushButton*>("runtime_run")->isEnabled(),"Late open Stop holds restart closed");gate.release();settle();check(runtimeCoordinator_->state()==RuntimeState::LoadedStopped && f.first()->closes==1 && f.first()->grabs==0,"Late open closes without capture");
    }
    {
        Fixture f("runtime-ui-partial",5);f.probes[f.requests[2].device.identity.serial]->failConfigure=true;install(f,ApplicationType::DefectInspection,"partial");runtimeAction("run");settle();check(runtimeCoordinator_->state()==RuntimeState::Fault && !runtimeResult_ && !runtimeViewer_->hasImage(),"Partial Runtime preparation faults without partial Running/result");for(const auto& p:f.probes)check(p->closes==1,"Partial Runtime UI set rolls back");runtimeAction("stop");settle();
    }
    runtimeCameraComposition_.reset();QJsonObject report{{"status","PASS"},{"physicalAcceptance",false},{"matrix",rows},{"cases",QJsonArray{"offlineLoad","starting","readyWithoutCapture","twentyCycles","idleStop","historyCsv","startingStopLateOpen","partialRollback5"}}};
    QSaveFile output(QDir(artifacts).filePath("runtime-camera-ui.json"));check(output.open(QIODevice::WriteOnly),"Runtime report open");output.write(QJsonDocument(report).toJson());check(output.commit(),"Runtime report save");QTextStream(stdout)<<"PASS: Runtime camera session UI\n";return true;
}
}
