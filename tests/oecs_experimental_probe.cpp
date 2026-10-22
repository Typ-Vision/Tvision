// Explicit manual hardware run; never an automatic hardware CTest.
#include "application/ExperimentalOecsAcquisition.hpp"
#include "application/IndependentCalibrationCapture.hpp"
#include "application/IndustrialExecution.hpp"
#include "application/ProjectService.hpp"
#include "application/RuntimeCoordinator.hpp"
#include "application/ExperimentalRuntime.hpp"
#include "application/CameraPreviewSession.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <QThread>
#include <QElapsedTimer>
#include <future>
using namespace visionflow;
int main(int argc,char** argv){QCoreApplication app(argc,argv);QJsonObject out{{"productionAdmission",false},{"triggerAssociationVerified",false}};int rc=0;
    try{
        if(argc!=5 && argc!=6)throw std::runtime_error("usage: oecs_experimental_probe absolute-dll absolute-evidence-directory exposure-us count [--preview-60s|--wrong-camera|--wrong-geometry|--stop-in-flight|--timeout|--disconnect|--editor-run|--runtime-run|--runtime-stop|--runtime-timeout|--runtime-disconnect|--calibration-run]");
        const QString scenario=argc==6?QString::fromLocal8Bit(argv[5]):QString{};
        if(!scenario.isEmpty() && scenario!="--preview-60s" && scenario!="--wrong-camera" && scenario!="--wrong-geometry" && scenario!="--stop-in-flight" && scenario!="--timeout" && scenario!="--disconnect" && scenario!="--editor-run" && scenario!="--runtime-run" && scenario!="--runtime-stop" && scenario!="--runtime-timeout" && scenario!="--runtime-disconnect" && scenario!="--calibration-run")throw std::runtime_error("unknown manual scenario");
        const QString directory=QString::fromLocal8Bit(argv[2]);QDir().mkpath(directory);
        bool ok=false;const double exposure=QString::fromLocal8Bit(argv[3]).toDouble(&ok);if(!ok)throw std::runtime_error("numeric exposure required");
        const int count=QString::fromLocal8Bit(argv[4]).toInt();if(count<1 || count>1000)throw std::runtime_error("count 1..1000 required");
        IndustrialCameraRequest r;
        r.device={{"Opto Engineering Srl","COE-050-M-USB-050-IR-C","02A2220000627",DeviceIdentityConfidence::SerialReported},
            {"oecs",{},"usb","exact"},{"oecs","experimental-native",QString(64,'a'),1}};
        r.camera={{"oecs-exp"},"Manual experimental camera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},r.device.identity};
        GeometryDescriptor g;g.resolution={2448,2048};g.sensorRoi=QRect(0,0,2448,2048);g.binning=QSize(1,1);g.decimation=QSize(1,1);g.reverse=GeometryMirror{};g.complete=true;
        r.setup={{"scene"},r.camera.id,"Experimental scene","Lens unidentified","Scene reference unverified",1,1,g};
        r.profile={{"experimental"},r.camera.id,r.setup.id,"Manual experimental",1,{"Mono8",{true,"FrameBurstStart","Software"},AutoMode::Off,exposure,AutoMode::Off,0,{}}};r.sourceId="manual-experimental-preview";
        if(scenario=="--wrong-camera"){r.device.identity.serial="CCUX15-absent-camera";r.camera.physical=r.device.identity;}
        if(scenario=="--wrong-geometry"){r.setup.geometry.resolution.setWidth(2447);r.setup.geometry.sensorRoi->setWidth(2447);}
        if(scenario=="--preview-60s"){
            // Manual hardware observation through the same controller as Camera Manager.
            // Placeholder optics are explicitly unverified; this cannot grant mm admission.
            CameraPreviewSession preview;QElapsedTimer timer;timer.start();
            preview.start([&]{return ExperimentalOecsAcquisition::open(r,QString::fromLocal8Bit(argv[1]));},true,3000);
            std::optional<AcquiredFrame> last;QJsonArray samples;
            while(timer.elapsed()<60000 && preview.state()!=PreviewState::Error){
                preview.poll();if(auto frame=preview.takeLatest()){
                    if(!last)ExperimentalOecsAcquisition::save(frame->frame,QDir(directory).absoluteFilePath("preview-first.pgm"));
                    samples.append(frame->frame.image.metadata().experimentalAcquisition);last=std::move(frame->frame);
                }QThread::msleep(1);
            }
            const auto elapsed=timer.elapsed();const auto stats=preview.stats();const auto generation=preview.generation();
            if(last)ExperimentalOecsAcquisition::save(*last,QDir(directory).absoluteFilePath("preview-last.pgm"));
            preview.stop();Deadline cleanup(10000);
            while(!preview.drained() && !cleanup.expired()){preview.poll();QThread::msleep(1);}
            out["samples"]=samples;out["count"]=double(stats.completed);out["elapsedMs"]=double(elapsed);out["completionFps"]=stats.fps;
            out["acquisitionKind"]="sequential_single_frame";out["replaced"]=double(stats.replaced);
            out["shutdownReady"]=preview.drained();out["stoppedQueueEmpty"]=!preview.takeLatest();out["stopRetiredGeneration"]=preview.generation()>generation;
            out["error"]=preview.error();out["opticsVerified"]=false;out["physicalMetricAcceptance"]=false;
            if(!preview.drained() || preview.state()==PreviewState::Error || !last || elapsed<60000 || !out["stoppedQueueEmpty"].toBool() || !out["stopRetiredGeneration"].toBool())throw std::runtime_error(("Preview observation failed: "+preview.error()).toStdString());
            out["status"]="EXPERIMENTAL_PREVIEW_60S_COMPLETED";QTextStream(stdout)<<QJsonDocument(out).toJson();return 0;
        }
        const auto producer=qEnvironmentVariable("VISIONFLOW_TIMING_PROBE_PRODUCER");
        auto discover=[r,producer](int timeout,const std::shared_ptr<std::atomic_bool>& cancel){return producer.isEmpty()?IndustrialDiscoverySnapshot{{r.device},{},false}:CameraService::industrialSnapshot(CameraService{}.discover({producer},timeout,cancel));};
        out["environment"]=QJsonObject{{"vendor",r.device.identity.vendor},{"model",r.device.identity.model},{"serial",r.device.identity.serial},{"manualFallbackRoute",r.device.route.toJson()},{"producerPath",producer},{"profile",r.profile.toJson()},{"geometry",r.setup.geometry.toJson()},{"timingEnabled",AcquisitionTiming::enabled()},{"uiFirstPaintMeasured",false},{"discoveryScope",producer.isEmpty()?"fixed locator; real SDK enumerates exact serial":"application GenTL discovery, producer hash, exact SDK enumeration"}};
        if(scenario=="--calibration-run"){
            IndependentCaptureRequest capture;capture.origin=SourceOrigin::CameraAcquired;capture.camera=r.camera;capture.setup=r.setup;capture.profile=r.profile;capture.policy=CalibrationCapturePolicy::ExperimentalOecs;capture.timeoutMs=3000;
            AcquisitionTimingBinding prepareBinding(AcquisitionTiming::request("calibration-preparation"));AcquisitionTimingSpan preparation("probe.initial-preparation");IndependentCaptureBackend backend;backend.industrialDiscover=[discover]{return discover(3000,{});};IndependentCalibrationSession session;QJsonArray samples;preparation.finish();
            for(int i=0;i<count;++i){AcquisitionTimingBinding binding(AcquisitionTiming::request("calibration"));QElapsedTimer timer;timer.start();const auto result=IndependentCalibrationSession::run(session.begin(capture),backend);if(!result.error().isEmpty() || !session.accept(result))throw std::runtime_error(("Calibration capture failed: "+result.error()).toStdString());samples.append(QJsonObject{{"index",i},{"milliseconds",timer.nsecsElapsed()/1e6},{"readback",session.math().frame()->metadata().experimentalAcquisition["readback"]}});}
            out["samples"]=samples;out["count"]=count;out["status"]="CALIBRATION_CAPTURE_COMPLETED";QTextStream(stdout)<<QJsonDocument(out).toJson();return 0;
        }
        if(scenario.startsWith("--runtime-")){
            IndustrialExecutionInputs inputs;inputs.contract={{r.camera},{r.setup},{r.profile}};inputs.discover=discover;
            ProjectService p;p.createProject("Experimental Runtime","",QDir(directory).absoluteFilePath("runtime.vfproject"),"file");p.createRecipe("Experimental","",ApplicationType::Measurement,"file");const auto source=p.addTool("industrial_camera");p.bindIndustrialSource(source,inputs.contract,r.profile.id,scenario=="--runtime-timeout" ? int(exposure/1000)+1:3000);const auto blob=p.addTool("blob");p.setConnections({{source,"image",blob,"image"}});p.save();
            AcquisitionTimingBinding prepareBinding(AcquisitionTiming::request("runtime-preparation"));AcquisitionTimingSpan preparation("probe.initial-preparation");
            RuntimeService runtime(p.path()+".production",{},RuntimeIdentity::localEngineer());runtime.setExperimental(true,[inputs,source](const Recipe&){MultiSourceDependencies set;set.sources[source]={};set.industrial=std::make_shared<const IndustrialExecutionInputs>(inputs);return set;});RuntimeCoordinator coordinator(runtime);coordinator.load(*p.activeRecipe(),directory);coordinator.start();preparation.finish();
            QJsonArray samples;QElapsedTimer timer;timer.start();
            for(int i=0;i<count;++i){AcquisitionTimingBinding binding(AcquisitionTiming::request("runtime"));QElapsedTimer sampleTimer;sampleTimer.start();const auto lease=coordinator.begin(RuntimeTrigger::Manual);auto cancel=coordinator.cancellation();std::future<void> stopper;if(scenario=="--runtime-stop")stopper=std::async(std::launch::async,[cancel]{QThread::msleep(10);cancel->store(true);});auto result=RuntimeService::execute(lease,{},cancel);
                if(stopper.valid()){stopper.get();coordinator.stop();RuntimeService::suppressCancelled(lease,result);}
                if(scenario=="--runtime-timeout" && result.state!=ExecutionState::Error)throw std::runtime_error("deadline did not fail");
                if((scenario=="--runtime-run" || scenario=="--runtime-disconnect") && result.state!=ExecutionState::Done)throw std::runtime_error(("Runtime capture failed: "+result.error+([&]{QString details;for(const auto& t:result.tools)details+="\n"+t.toolId+": "+t.errorCode+" "+t.message;return details;})()).toStdString());coordinator.finish(lease.ticket,result);
                samples.append(QJsonObject{{"cycleId",result.cycleId},{"milliseconds",result.milliseconds},{"serviceTotalMs",sampleTimer.nsecsElapsed()/1e6},{"frames",int(result.frames.size())},{"state",executionStateName(result.state)},{"error",result.error},{"trustedEvidenceCount",int(result.industrialEvidence.size())}});
                if(result.state==ExecutionState::Done && (i==0 || i==count-1))for(const auto& t:result.tools)if(t.toolId==source){const auto image=std::get<ImageFrame>(t.outputs["image"].data);const auto m=image.metadata();ExperimentalOecsAcquisition::save({image,source,result.cycleId,m.frameId,m.completedAtUtc},QDir(directory).absoluteFilePath(QString("runtime-frame-%1.pgm").arg(i)));}
                if((i+1)%25==0){QFile progress(QDir(directory).absoluteFilePath("runtime-progress.json"));if(progress.open(QIODevice::WriteOnly|QIODevice::Truncate))progress.write(QJsonDocument(QJsonObject{{"completed",i+1},{"elapsedMs",double(timer.elapsed())}}).toJson());}
            }
            if(scenario=="--runtime-disconnect"){
                QFile ready(QDir(directory).absoluteFilePath("runtime-disconnect-ready.txt"));if(!ready.open(QIODevice::WriteOnly|QIODevice::NewOnly))throw std::runtime_error("disconnect readiness marker failed");ready.write("Runtime initial cycle committed. Unplug USB now; do not replug until fault observed.\n");ready.close();
                bool observed=false;int additional=0;Deadline observation(180000);
                while(!observation.expired()){
                    const auto lease=coordinator.begin(RuntimeTrigger::Manual);const auto result=RuntimeService::execute(lease,{},coordinator.cancellation());coordinator.finish(lease.ticket,result);
                    if(result.state==ExecutionState::Error){out["acquisitionFault"]=result.error;QJsonArray errors;for(const auto& t:result.tools)if(t.state==ExecutionState::Error)errors.append(t.errorCode+" "+t.message);out["toolErrors"]=errors;out["framesAfterFault"]=int(result.frames.size());observed=true;break;}
                    ++additional;QThread::msleep(100);
                }
                if(!observed)throw std::runtime_error("Runtime physical disconnect not observed in 180s");
                bool nextDenied=false;try{coordinator.begin(RuntimeTrigger::Manual);}catch(const std::exception&){nextDenied=true;}
                out["faultObserved"]=true;out["additionalFramesBeforeFault"]=additional;out["newCycleDeniedAfterFault"]=nextDenied;out["automaticReconnectAttempted"]=false;out["physicalUnplugRequiresOperatorConfirmation"]=true;
                if(!nextDenied || out["framesAfterFault"].toInt()!=0)throw std::runtime_error("Runtime disconnect leaked frame/admission");
            }
            const auto stateBeforeStop=coordinator.state();coordinator.stop();const auto history=ProductionStore(p.path()+".production").recent("experimental_cycles");QFile csv(QDir(directory).absoluteFilePath("recent-experimental.csv"));if(!csv.open(QIODevice::WriteOnly))throw std::runtime_error("CSV open failed");csv.write(ExperimentalRuntime::csv(history));
            RuntimeService restarted(p.path()+".production",{},RuntimeIdentity::localEngineer());out["samples"]=samples;out["count"]=count;out["elapsedMs"]=double(timer.elapsed());out["restartPassive"]=!restarted.active() && !restarted.experimental();out["productionRecordCount"]=ProductionStore(p.path()+".production").recent("cycles").size();out["experimentalRecordFiles"]=QDir(p.path()+".production/experimental_cycles").entryList({"*.json"},QDir::Files).size();out["scenario"]=scenario;out["physicalCommandInFlightProven"]=false;out["coordinatorStateBeforeStop"]=int(stateBeforeStop);out["status"]="EXPERIMENTAL_RUNTIME_COMPLETED";QTextStream(stdout)<<QJsonDocument(out).toJson();return 0;
        }
        if(scenario=="--editor-run"){
            AcquisitionTimingBinding prepareBinding(AcquisitionTiming::request("editor-preparation"));AcquisitionTimingSpan preparation("probe.initial-preparation");
            if(QString::fromLocal8Bit(argv[1])!=ExperimentalOecsAcquisition::installedRuntime())throw std::runtime_error("Editor uses installed runtime");
            IndustrialExecutionInputs inputs;inputs.contract={{r.camera},{r.setup},{r.profile}};
            // Probe supplies a locator; the real runtime re-enumerates and exact-opens
            // the identity. This is not a discovery acceptance measurement.
            inputs.discover=discover;
            ProjectService p;p.createProject("Experimental Editor","",QDir(directory).absoluteFilePath("editor.vfproject"),"file");
            p.createRecipe("Experimental","",ApplicationType::Measurement,"file");const auto source=p.addTool("industrial_camera");p.bindIndustrialSource(source,inputs.contract,r.profile.id,3000);p.save();preparation.finish();
            QJsonArray samples;
            for(int i=0;i<count;++i){AcquisitionTimingBinding binding(AcquisitionTiming::request("editor"));QElapsedTimer sampleTimer;sampleTimer.start();const auto cycle=ExecutionService(inputs).withExperimentalEditorReplay().run(p.activeRecipe()->definition,directory,p.activeRecipe()->id,{}, {},source);
                if(cycle.state!=ExecutionState::Done || !cycle.industrialEvidence.isEmpty())throw std::runtime_error(("Editor acquisition failed: "+cycle.error+(cycle.tools.isEmpty()?QString{}:cycle.tools.front().message)).toStdString());
                bool found=false;for(const auto& tool:cycle.tools)if(tool.toolId==source){const auto image=std::get<ImageFrame>(tool.outputs["image"].data);const auto m=image.metadata();
                    if(m.experimentalAcquisition.isEmpty())throw std::runtime_error("Editor lost provenance");auto sample=m.experimentalAcquisition;sample["milliseconds"]=sampleTimer.nsecsElapsed()/1e6;samples.append(sample);found=true;
                    if(i==0 || i==count-1)ExperimentalOecsAcquisition::save({image,source,cycle.cycleId,m.frameId,m.completedAtUtc},QDir(directory).absoluteFilePath(QString("editor-frame-%1.pgm").arg(i)));
                }if(!found)throw std::runtime_error("Editor source missing");
            }
            out["samples"]=samples;out["count"]=count;out["trustedEvidenceCount"]=0;out["status"]="EXPERIMENTAL_EDITOR_EXECUTION";QTextStream(stdout)<<QJsonDocument(out).toJson();return 0;
        }
        auto worker=ExperimentalOecsAcquisition::open(r,QString::fromLocal8Bit(argv[1]));
        const auto wait=[&](CameraState desired){Deadline deadline(32000);for(;;){worker->poll();const auto state=worker->lifecycle().state();if(state==CameraState::Fault)throw std::runtime_error(worker->lifecycle().error().toStdString());if(state==desired)return;if(deadline.expired())throw std::runtime_error("controller wait timeout");QThread::msleep(1);}};
        const auto command=[&](CameraOperation op,CameraState state){worker->submit(op,3000);wait(state);};
        command(CameraOperation::Connect,CameraState::Connected);command(CameraOperation::Configure,CameraState::Configured);command(CameraOperation::Start,CameraState::Streaming);
        QJsonArray samples;
        for(int i=0;i<count;++i){command(CameraOperation::Grab,CameraState::Streaming);auto f=worker->take();if(!f)throw std::runtime_error("missing owned frame");
            auto evidence=f->image.metadata().experimentalAcquisition;evidence["contentId"]=QString::fromLatin1(f->image.contentId().toHex());samples.append(evidence);
            if(i==0 || i==count-1)ExperimentalOecsAcquisition::save(*f,QDir(directory).absoluteFilePath(QString("frame-%1.pgm").arg(i)));
        }
        out["samples"]=samples;out["count"]=count;
        if(scenario=="--disconnect"){
            // Physical unplug is performed by the operator. Do not inject a
            // transportLost event or reconnect: observe the actual SDK fault.
            QFile ready(QDir(directory).absoluteFilePath("disconnect-ready.txt"));
            if(!ready.open(QIODevice::WriteOnly|QIODevice::NewOnly))throw std::runtime_error("cannot create disconnect readiness marker");
            ready.write("Initial owned frame saved. Acquisition active; unplug USB now.\n");ready.close();
            QTextStream(stderr)<<"DISCONNECT_READY: initial owned frame saved; unplug USB now.\n"<<Qt::flush;
            int additionalFrames=0;bool faultObserved=false;Deadline observation(180000);
            while(!observation.expired()){
                try{
                    command(CameraOperation::Grab,CameraState::Streaming);
                    if(!worker->take())throw std::runtime_error("missing owned frame during disconnect observation");
                    ++additionalFrames;
                }catch(const std::exception& e){
                    if(worker->lifecycle().state()!=CameraState::Fault)throw;
                    out["acquisitionFault"]=QString::fromUtf8(e.what());faultObserved=true;break;
                }
                QThread::msleep(100);
            }
            out["acquisitionFaultObserved"]=faultObserved;
            out["additionalFramesBeforeFault"]=additionalFrames;
            out["automaticReconnectAttempted"]=false;
            out["physicalUnplugRequiresOperatorConfirmation"]=true;
            out["frameAvailableAfterFault"]=bool(worker->take());
            if(!faultObserved)throw std::runtime_error("no acquisition fault observed within physical unplug window");
            if(out["frameAvailableAfterFault"].toBool())throw std::runtime_error("frame escaped faulted request");
        }else if(scenario=="--stop-in-flight"){
            worker->submit(CameraOperation::Grab,3000);QThread::msleep(10);worker->requestStop();
            out["stopRequestedDuringPendingGrab"]=true;
            out["physicalCommandInFlightProven"]=false;
        }else if(scenario=="--timeout"){
            worker->submit(CameraOperation::Grab,1);Deadline observation(5000);
            while(worker->lifecycle().state()!=CameraState::Fault){worker->poll();if(observation.expired())throw std::runtime_error("1ms deadline did not fault");QThread::msleep(1);}
            out["deadlineFaultObserved"]=true;out["deadlineFault"]=worker->lifecycle().error();out["physicalCommandInFlightProven"]=false;
        }
        worker->requestStop();worker->requestShutdown();Deadline cleanup(5000);while(worker->shutdownState()!=ShutdownState::Ready){if(cleanup.expired())throw std::runtime_error("cleanup wait timeout");QThread::msleep(1);}
        if(worker->take())throw std::runtime_error("frame escaped stopped/expired request");out["stoppedQueueEmpty"]=true;out["shutdownReady"]=true;
        out["status"]=scenario=="--disconnect"?"EXPERIMENTAL_ACQUISITION_FAULT_OBSERVED":"EXPERIMENTAL_OWNED_FRAMES";
    }catch(const std::exception& e){rc=1;out["status"]="ERROR";out["error"]=QString::fromUtf8(e.what());}
    QTextStream(stdout)<<QJsonDocument(out).toJson();return rc;
}
