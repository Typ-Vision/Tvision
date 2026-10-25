#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>
#include <QThread>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QTextStream>
#include <thread>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
namespace visionflow {
namespace {
QImage stripe(){QImage image(640,200,QImage::Format_Grayscale8);image.fill(20);for(int y=0;y<200;++y)for(int x=120;x<520;++x)image.scanLine(y)[x]=220;return image;}
class RestartAdapter:public AcquisitionAdapter {
public:
    AcquisitionCapabilities capabilities()const override{return {"webcam",true};}
    ImageFrame acquire(const AcquisitionRequest&)const override{const auto image=stripe();return ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),640,200,image.bytesPerLine(),PixelFormat::Gray8);}
};
}
bool MainWindow::verifyRestartRuntime(const QString& artifacts){
    int checks=0;auto check=[&](bool b,const char* message){++checks;if(!b)throw std::runtime_error(message);};
    auto button=[&](const char* name){return findChild<QPushButton*>(name);};
    auto wait=[&]{QElapsedTimer timer;timer.start();do{QApplication::processEvents();QThread::msleep(1);}while((runtimeJobs_.busy() || (runtime_ && runtime_->sourceVerificationBusy()) || (runtimeCoordinator_ && runtimeCoordinator_->state()==RuntimeState::Stopping)) && timer.elapsed()<20000);QApplication::processEvents();check(!runtimeJobs_.busy(),"Verification/cycle worker deadline");};
    auto shot=[&](const QString& name){const auto tag=name+(runtimeCoordinator_ && runtimeCoordinator_->loaded() && runtimeCoordinator_->loaded()->definition["editor"].toObject()["schemaVersion"]==9 ? "-editor9":"-legacy");setWindowState(Qt::WindowNoState);resize(1400,900);QApplication::processEvents();check(grab().save(QDir(artifacts).filePath("b4-"+tag+"-normal.png")),"Normal screenshot");resize(850,600);QApplication::processEvents();check(grab().save(QDir(artifacts).filePath("b4-"+tag+"-small.png")),"Small screenshot");};
    QTemporaryDir dir(QDir(artifacts).filePath("b4-restart-XXXXXX"));check(dir.isValid(),"Build fixture directory");
    auto backendState=std::make_shared<std::atomic_int>(0);auto probes=std::make_shared<std::atomic_int>(0);
    const MeasurementCamera camera{{"b4-camera"},"Doğrulama kamerası",{"webcam","7274",DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}};
    const MeasurementSetupDescriptor setup{{"b4-setup"},camera.id,"Tabla","Lens","Düzlem",1,1,{{640,200},{},{},true}};
    const IndependentCaptureBackend backend{[camera,backendState,probes]{++*probes;if(backendState->load()==2)std::this_thread::sleep_for(std::chrono::milliseconds(1500));auto discovered=camera.device;if(backendState->load()==1)discovered.providerId="9999";return QVector<DeviceBinding>{discovered};},[](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<RestartAdapter>();}};
    runtimeVerificationBackend_=backend;runtimeSourceFactory_=[](const ToolDefinition& t)->std::unique_ptr<AcquisitionAdapter>{return t.kind=="webcam" ? std::make_unique<RestartAdapter>():nullptr;};
    const auto args=QCoreApplication::arguments();const auto childIndex=args.indexOf("--restart-project");
    if(childIndex>=0){
        service_.openProject(args.value(childIndex+1));service_.selectRecipe(args.value(childIndex+2));refreshWorkspace();ensureRuntime();
        runtimeCoordinator_->load(*service_.activeRecipe(),QFileInfo(service_.path()).absolutePath());selectPage("runtime");refreshRuntime();
        check(probes->load()==0 && !runtime_->active() && !runtimeResult_ && !button("runtime_run")->isEnabled(),"New process has no permission or acquisition after Load");
        runtimeAction("verify");wait();check(button("runtime_run")->isEnabled() && runtimeCoordinator_->state()==RuntimeState::LoadedStopped && !runtime_->active(),"New process explicit Verify renews permission without Run");
        runtimeAction("run");runtimeAction("cycle");wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && runtime_->records().size()==1,"New process real fixture cycle persists");
        runtimeAction("stop");QTextStream(stdout)<<"PROCESS_RESTART_OK\n";return true;
    }
    for(bool multi:{false,true})for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        service_.createProject("B4 restart","",dir.filePath(typeKey(type)+(multi ? "-editor9":"-legacy")+".vfproject"),"file");service_.save();refreshWorkspace();
        measurementWorkspace_->replace({{camera},{setup}});
        IndependentCaptureRequest request;request.origin=SourceOrigin::CameraAcquired;request.camera=camera;request.setup=setup;
        IndependentCalibrationSession session;check(session.accept(IndependentCalibrationSession::run(session.begin(request),backend)),"Teaching probe");
        auto& math=session.math();check(math.accept(CalibrationWizardSession::run(math.scanPair("width",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"Teaching caliper");math.setKnownMillimetres(10);const auto record=math.calculate().named("Kayıtlı exact model");const SetupReference target{camera.id,setup.id,1};
        measurementWorkspace_->calibrations().save(target,record);measurementWorkspace_->calibrations().setOperational(camera.id,setup.id);measurementWorkspace_->calibrations().activate(target,record.contentId(),session);
        const auto recipeId=service_.createRecipe("Restart "+typeKey(type),"",type,"file");
        if(multi)service_.enableMultiSource();
        const auto source=service_.addTool("webcam"),left=service_.addTool("edge_point"),right=service_.addTool("edge_point"),metric=service_.addTool("measurement");
        for(auto tool:service_.tools())if(tool.id==left || tool.id==right){tool.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();tool.parameters["polarity"]=tool.id==left ? "rising":"falling";service_.updateTool(tool);}
        service_.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",metric,"p1"},{right,"point",metric,"p2"}});service_.bindSourceSetup(source,target.toJson());
        if(multi){const auto file=dir.filePath("raw.png");check(stripe().save(file),"Raw sibling file");const auto raw=service_.addTool("image_file"),consumer=service_.addTool("blob");
            for(auto t:service_.tools())if(t.id==raw){t.parameters["path"]=file;service_.updateTool(t);}
            auto connections=editorConnections(service_.activeRecipe()->definition);connections.append({raw,"image",consumer,"image"});service_.setConnections(connections);
        }
        service_.save();
        QProcess child;auto env=QProcessEnvironment::systemEnvironment();const auto childArtifacts=dir.filePath(typeKey(type)+(multi ? "-multi-child":"-single-child"));QDir().mkpath(childArtifacts);env.insert("VISIONFLOW_TEST_ARTIFACTS",childArtifacts);child.setProcessEnvironment(env);
        child.start(QCoreApplication::applicationFilePath(),{"--restart-runtime-smoke","--restart-project",service_.path(),recipeId});check(child.waitForStarted(),"Restart child starts");
        QElapsedTimer childTimer;childTimer.start();while(child.state()!=QProcess::NotRunning && childTimer.elapsed()<30000){QApplication::processEvents();QThread::msleep(5);}
        const auto childLog=child.readAllStandardOutput()+child.readAllStandardError();QFile log(QDir(artifacts).filePath("process-"+typeKey(type)+(multi ? "-multi":"-single")+".log"));check(log.open(QIODevice::WriteOnly),"Child log");log.write(childLog);log.close();
        if(child.exitCode()!=0)QTextStream(stderr)<<childLog;check(child.state()==QProcess::NotRunning && child.exitStatus()==QProcess::NormalExit && child.exitCode()==0 && childLog.contains("PROCESS_RESTART_OK"),"True process restart verification/cycle");
        // A fresh workspace has the persisted selection, but no process permission.
        if(plc_)plc_->bind(nullptr,nullptr);runtimeCoordinator_.reset();runtime_.reset();measurementWorkspace_.reset();measurementWorkspaceKey_.clear();refreshWorkspace();
        service_.openProject(service_.path());service_.selectRecipe(recipeId);refreshWorkspace();selectPage("recipes");recipeTable_->selectRow(0);button("loadRecipe")->click();
        check(runtimeCoordinator_ && runtimeCoordinator_->loaded() && runtimeCoordinator_->state()==RuntimeState::LoadedStopped,"Restart static Load succeeds");
        check(runtimePreparation_->rowCount()==(multi ? 2:1),"Single and N-source readiness rows");
        check(!runtime_->active() && !runtimeJobs_.busy() && !runtimeResult_ && !runtimeViewer_->hasImage(),"Load creates no acquisition/snapshot/result");
        check(!button("runtime_run")->isEnabled() && button("runtime_verify")->isEnabled() && runtimeStatus_->text().contains("doğrulaması bekleniyor"),"Restart waits for explicit verification");shot("waiting-"+typeKey(type));
        backendState->store(1);button("runtime_verify")->click();wait();
        check(!button("runtime_run")->isEnabled() && runtimePreparation_->item(0,2)->text().contains("Hata:"),"Wrong camera shows source failure and blocks Run");shot("wrong-camera-"+typeKey(type));
        backendState->store(2);button("runtime_verify")->click();check(button("runtime_stop")->isEnabled() && !button("runtime_run")->isEnabled(),"Verification progress permits Stop");shot("verifying-"+typeKey(type));check(!resolveUnsavedChanges() && measurementWorkspace_->sourceVerification()->busy(),"Project switch cancels and retains worker ownership");button("runtime_stop")->click();wait();
        check(!button("runtime_run")->isEnabled() && runtimeStatus_->text().contains("iptal"),"Stop suppresses late verification readiness");shot("cancelled-"+typeKey(type));
        backendState->store(0);button("runtime_verify")->click();wait();
        check(button("runtime_run")->isEnabled() && runtimeCoordinator_->state()==RuntimeState::LoadedStopped && !runtime_->active() && !runtimeResult_,"Successful verification enables Run without automatic start");shot("ready-"+typeKey(type));
        button("runtime_run")->click();check(runtimeCoordinator_->state()==RuntimeState::Running && !runtimeResult_,"Run prepares without result");button("runtime_cycle")->click();wait();
        if(!runtimeResult_ || runtimeResult_->state!=ExecutionState::Done)QTextStream(stderr)<<runtimeSummary_->text()<<"\n"<<runtimeStatus_->text()<<"\n";
        check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && !runtime_->records().isEmpty(),"Actual injected acquisition cycle persists");
        bool measured=false;for(const auto& result:runtimeResult_->tools)if(result.toolId==metric)measured=result.decision==PartDecision::Pass && result.outputs["value"].valid;check(measured,"Actual measured result comes from cycle");shot("cycle-"+typeKey(type));button("runtime_stop")->click();
        // Missing exact catalog is rejected by Load; the old loaded recipe survives.
        const auto catalog=service_.path()+".active-calibrations.json";check(QFile::rename(catalog,catalog+".missing"),"Move isolated model catalog");
        selectPage("recipes");recipeTable_->selectRow(0);button("loadRecipe")->click();check(runtimeCoordinator_->loaded()->id==recipeId && workspaceError_->text().contains("Load başarısız"),"Missing model rejects static Load and preserves loaded recipe");
        selectPage("runtime");refreshRuntime();check(!button("runtime_run")->isEnabled(),"Missing model disables Run");shot("missing-model-"+typeKey(type));check(QFile::rename(catalog+".missing",catalog),"Restore isolated model catalog");
    }
    runtimeVerificationBackend_={};runtimeSourceFactory_={};QTextStream(stdout)<<checks<<" restart Runtime UI checks passed\n";return true;
}
}
