#include "CameraFlowSmokeAudit.hpp"

#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QPointer>
#include <QDir>
#include <QElapsedTimer>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPushButton>
#include <QTemporaryDir>
#include <QThread>
#include <QTextStream>
#include <QTableWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QListWidget>
#include <QJsonArray>
#include <QSpinBox>
#include <QScrollArea>
#include "domain/IndustrialSourceReference.hpp"
#include "application/SystemCalibration.hpp"
#include "application/ProductionResultCodec.hpp"
#include "application/ExperimentalRuntime.hpp"
#include <QCheckBox>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLockFile>
#include <QTextEdit>
#include <cmath>
namespace visionflow {
bool MainWindow::verifyMultiSource(const QString& artifacts){
    int checks=0;auto check=[&](bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);};
    auto wait=[&]{QElapsedTimer timer;timer.start();do{QApplication::processEvents();QThread::msleep(1);}while((editorSession_.busy() || editorCameras_.pending() || editorCameraPendingExecution_) && timer.elapsed()<30000);QApplication::processEvents();check(!editorSession_.busy() && !editorCameras_.pending() && !editorCameraPendingExecution_,"Worker/preparation deadline");};
    QTemporaryDir dir(QDir(artifacts).filePath("kmc10b-ui-XXXXXX"));check(dir.isValid(),"Build fixtures");
    service_.createProject("Multi-source Editor","",dir.filePath("project.vfproject"),"file");service_.save();
    for(int n:{1,2,8,10})for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        service_.createRecipe(QString("%1 sources %2").arg(n).arg(typeKey(type)),"",type);refreshWorkspace();selectPage("editor");wait();
        auto* more=findChild<QPushButton*>("editorMore");if(!more->isChecked())more->click();
        auto* opt=findChild<QPushButton*>("enableMultiSource");check(opt && opt->isVisible(),"Explicit migration command in recipe details");opt->click();check(service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9,"UI explicit opt-in");more->click();
        QVector<QString> sources,edges;QVector<ToolConnection> links;
        for(int i=0;i<n;++i){QImage image(256+i,160,QImage::Format_Grayscale8);image.fill(10+i);for(int y=0;y<160;++y)for(int x=80+i;x<180+i;++x)image.scanLine(y)[x]=220;
            const auto path=dir.filePath(QString("source-%1.png").arg(i));check(image.save(path),"Actual distinct PNG");const auto source=service_.addTool("image_file");service_.setImageFile(path,source);const auto edge=service_.addTool("edge_point");sources<<source;edges<<edge;
            for(auto t:service_.tools())if(t.id==edge){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{10,80},{240,80},4}}.toJson();t.parameters["polarity"]="rising";service_.updateTool(t);}links<<ToolConnection{source,"image",edge,"image"};
        }
        service_.setConnections(links);service_.save();refreshWorkspace();selectTool(edges.first());runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done && editorSession_.sourceFrames.size()==n,"UI N acquisitions");
        auto* picker=findChild<QComboBox*>("editorSourceSelector");check(picker && picker->count()==n && picker->isVisible(),"N source selector");
        for(int i=0;i<n;++i){picker->setCurrentIndex(i);picker->activated(i);check(editorSession_.frame && editorSession_.frame->metadata().sourceId==sources[i],"Viewer source selection");selectTool(edges[i]);check(editorSession_.frame->size()==QSize(256+i,160),"Tool selects correct raw raster");
            bool shown=false;for(const auto* item:imageViewer_->scene()->items())if(const auto* pix=qgraphicsitem_cast<const QGraphicsPixmapItem*>(item))if(!pix->pixmap().isNull())shown=shown || pix->pixmap().toImage().pixelColor(0,0).red()==10+i;check(shown,"Displayed raster matches selected branch");
            const auto eval=editorSession_.evaluation(editorContext(),edges[i]);check(eval.cache[edges[i]].result.outputs["point"].origin.sourceId==sources[i],"Overlay source is selected raster");
            runExecution(edges[i]);wait();check(editorSession_.cycle->acquisitions==0 && editorSession_.evaluation(editorContext(),edges[i]).cache.contains(edges[i]),"UI TargetOnly no acquisition");
        }
        if(n>1){const auto other=editorSession_.sourceFrames[sources[1]].metadata().frameId;selectTool(sources[0]);runExecution(sources[0]);wait();check(editorSession_.cycle->partial && editorSession_.sourceFrames[sources[1]].metadata().frameId==other && editorSession_.evaluation(editorContext(),edges[1]).cache.contains(edges[1]),"UI source refresh keeps other cache");}
        selectTool(edges.last());resize(1400,900);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("kmc10b-editor-%1-normal.png").arg(n))),"Normal screenshot");resize(850,600);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("kmc10b-editor-%1-small.png").arg(n))),"Small screenshot");check(picker->isEnabled(),"Small source selector enabled");
        service_.removeTool(sources.first());refreshWorkspace();check(!editorSession_.sourceFrames.contains(sources.first()),"Deleted source frame removed");service_.save();
    }
    service_.createRecipe("Industrial source draft","",ApplicationType::Measurement);
    const auto industrial=service_.addTool("industrial_camera");refreshWorkspace();selectPage("editor");selectTool(industrial);
    check(findChild<QLabel*>("industrialSourceStatus") && !findChild<QPushButton*>("runSelectedTool")->isEnabled(),"Industrial draft clearly unavailable");
    check(matchInfo_->text().isEmpty() && imageInfo_->text()=="Görüntü yok.","New industrial recipe clears previous results/frame labels");
    // Offline registry fixture: no provider or synthetic acquisition is linked into the app.
    MeasurementCamera camera{{"ui-industrial"},"Sentetik offline kamera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},PhysicalDeviceIdentity{"VF","Offline fixture","SYNTHETIC",DeviceIdentityConfidence::SerialReported}};
    GeometryDescriptor geometry;geometry.resolution={640,480};geometry.sensorRoi=QRect(0,0,640,480);geometry.binning=QSize(1,1);geometry.decimation=QSize(1,1);geometry.reverse=GeometryMirror{};geometry.complete=true;
    MeasurementSetupDescriptor setup{{"ui-setup"},camera.id,"Sentetik kurulum","lens","plane",1,1,geometry};
    AcquisitionProfile profile{{"ui-profile"},camera.id,setup.id,"Mono8 software trigger",1,{"Mono8",{true,"FrameStart","Software"},AutoMode::Off,100,AutoMode::Off,1,{}}};
    measurementWorkspace_->replace({{camera},{setup},{profile}});refreshWorkspace();selectTool(industrial);
    auto* cameras=findChild<QComboBox*>("sourceCameraSelector");check(cameras && cameras->count()==2,"Offline registry camera listed");
    QPointer<QComboBox> selectingCamera=cameras;
    QKeyEvent cameraDown(QEvent::KeyPress,Qt::Key_Down,Qt::NoModifier);
    QApplication::sendEvent(cameras,&cameraDown);
    check(selectingCamera,"Camera selector survives its input event");
    QApplication::processEvents();
    check(!editorSession_.busy() && !editorSession_.cycle,"Camera selection never acquires");
    check(IndustrialSourceReference::fromJson(service_.tools().last().parameters["sourceReference"].toObject()).camera==camera.id && SystemCalibration::setupReference(SystemCalibration::branch(*service_.activeRecipe(),industrial))->setup==setup.id,"Camera UI atomically binds profile and setup");
    service_.save();const auto definition=service_.activeRecipe()->definition;const auto industrialRecipe=service_.activeRecipe()->id;service_.openProject(service_.path());service_.selectRecipe(industrialRecipe);refreshWorkspace();selectTool(industrial);
    check(service_.activeRecipe()->definition==definition && findChild<QPushButton*>("runSelectedTool")->isEnabled(),"Bound offline source survives reopen");
    profile.revision++;profile.requested.gain=2;measurementWorkspace_->replace({{camera},{setup},{profile}});refreshWorkspace();selectTool(industrial);
    check(findChild<QLabel*>("industrialSourceStatus")->text().contains("Bağ geçersiz"),"Profile edit requires explicit rebind");
    service_.refreshCameraSources(measurementWorkspace_->environment());check(service_.dirty(),"Run preparation marks refreshed snapshot dirty");
    service_.save();refreshWorkspace();selectTool(industrial);
    for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="industrial_camera"){toolbox_->setCurrentRow(i);toolbox_->scrollToItem(toolbox_->item(i));}
    const auto output=QDir(artifacts).filePath("icm/07c");check(QDir().mkpath(output),"Industrial screenshot directory");
    resize(1400,900);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,output+"/industrial-normal.png"),"Industrial normal screenshot");
    resize(850,600);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,output+"/industrial-small.png"),"Industrial small screenshot");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(findChild<QComboBox*>("sourceCameraSelector"));QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,output+QString("/industrial-binding-%1.png").arg(size.width())),"Industrial binding screenshot");}
    runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Error && editorSession_.sourceFrames.isEmpty(),"Industrial Run Flow fails without fabricated raster");
    QTextStream(stdout)<<checks<<" multi-source UI checks passed\n";return true;
}
}

namespace visionflow {
namespace {
class RuntimeFixtureAdapter:public AcquisitionAdapter {QImage image_;public:explicit RuntimeFixtureAdapter(QImage image):image_(std::move(image)){}AcquisitionCapabilities capabilities()const override{return {"webcam",true};}ImageFrame acquire(const AcquisitionRequest&)const override{return ImageFrame::fromBuffer(image_.constBits(),image_.sizeInBytes(),image_.width(),image_.height(),image_.bytesPerLine(),PixelFormat::Gray8);}};
}
bool MainWindow::verifyMultiSourceRuntime(const QString& artifacts){
    int checks=0;auto check=[&](bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);};
    auto wait=[&]{QElapsedTimer timer;timer.start();do{QApplication::processEvents();QThread::msleep(1);}while(runtimeJobs_.busy() && timer.elapsed()<30000);QApplication::processEvents();check(!runtimeJobs_.busy(),"Runtime worker deadline");};
    QTemporaryDir dir(QDir(artifacts).filePath("kmc10d-ui-XXXXXX"));check(dir.isValid(),"Build Runtime fixtures");service_.createProject("Multi-source Runtime","",dir.filePath("project.vfproject"),"file");service_.save();
    for(int n:{1,2,8,10})for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        service_.createRecipe(QString("%1 sources %2").arg(n).arg(typeKey(type)),"",type);service_.enableMultiSource();QVector<QString> sources,edges,metrics;QVector<ToolConnection> links;const bool calibrated=n==2 && type==ApplicationType::Measurement;
        MeasurementSetupContract contract;if(calibrated){refreshWorkspace();check(bool(measurementWorkspace_),"UI Measurement workspace");for(int i=0;i<n;++i){contract.cameras.append({{"runtime-c"+QString::number(i)},"Camera "+QString::number(i),{"webcam","991"+QString::number(i),DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}});contract.setups.append({{"runtime-s"+QString::number(i)},contract.cameras.last().id,"Setup","lens","plane",1,1,{{256+i,160},{},{},true}});}measurementWorkspace_->replace(contract);}
        for(int i=0;i<n;++i){QImage image(256+i,160,QImage::Format_Grayscale8);image.fill(10+i);for(int y=0;y<160;++y)for(int x=80+i;x<180+i;++x)image.scanLine(y)[x]=220;const auto path=dir.filePath(QString("runtime-source-%1.png").arg(i));check(image.save(path),"Actual Runtime raster");const auto source=service_.addTool("image_file");service_.setImageFile(path,source);const auto edge=service_.addTool("edge_point");sources<<source;edges<<edge;for(auto t:service_.tools())if(t.id==edge){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{10,80},{240,80},4}}.toJson();t.parameters["polarity"]="rising";service_.updateTool(t);}links<<ToolConnection{source,"image",edge,"image"};
            if(calibrated){const auto camera=contract.cameras[i];const auto setup=contract.setups[i];IndependentCaptureRequest req;req.origin=SourceOrigin::CameraAcquired;req.camera=camera;req.setup=setup;IndependentCaptureBackend backend{[camera]{return QVector<DeviceBinding>{camera.device};},[image](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<RuntimeFixtureAdapter>(image);}};IndependentCalibrationSession session;check(session.accept(IndependentCalibrationSession::run(session.begin(req),backend)),"UI model actual fixture probe");auto& math=session.math();check(math.accept(CalibrationWizardSession::run(math.scanPair("width",{{"image"},ScanSegment{{10,80},{240,80},4}}))),"UI model real pair");math.setKnownMillimetres(i+1);auto model=math.calculate().named("Runtime scale "+QString::number(i));const SetupReference ref{camera.id,setup.id,1};measurementWorkspace_->calibrations().save(ref,model);measurementWorkspace_->calibrations().setOperational(camera.id,setup.id);measurementWorkspace_->calibrations().activate(ref,model.contentId(),session);service_.bindSourceSetup(source,ref.toJson());const auto right=service_.addTool("edge_point"),metric=service_.addTool("measurement");metrics<<metric;for(auto t:service_.tools())if(t.id==right){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{10,80},{240,80},4}}.toJson();t.parameters["polarity"]="falling";service_.updateTool(t);}links<<ToolConnection{source,"image",right,"image"}<<ToolConnection{edge,"point",metric,"p1"}<<ToolConnection{right,"point",metric,"p2"};}
        }
        service_.setConnections(links);service_.save();refreshWorkspace();ensureRuntime();runtimeCoordinator_->load(*service_.activeRecipe(),dir.path(),RuntimeTrigger::Manual);selectPage("runtime");refreshRuntime();check(runtimePreparation_->rowCount()==n,"N preparation rows");
        runtimeAction("run");check(runtimeCoordinator_->state()==RuntimeState::Running && !runtimeResult_ && !runtimeViewer_->hasImage(),"Run preparation without fake frame/result");check(runtimeSources_->count()==n,"Runtime source selector");runtimeAction("cycle");check(runtimeJobs_.busy() && !findChild<QPushButton*>("runtime_cycle")->isEnabled(),"Busy disables another cycle");wait();
        if(!runtimeResult_)QTextStream(stderr)<<runtimeSummary_->text()<<'\n';check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && runtimeResult_->frames.size()==n,"Runtime N actual frames persisted");
        for(int i=0;i<n;++i){runtimeSources_->setCurrentIndex(i);check(runtimeSources_->currentData()==sources[i] && runtimeTools_->count()==(calibrated ? 4:2),"Source filters branch tools");runtimeTools_->setCurrentIndex(runtimeTools_->findData(edges[i]));bool shown=false;for(const auto* item:runtimeViewer_->scene()->items())if(const auto* pix=qgraphicsitem_cast<const QGraphicsPixmapItem*>(item))if(!pix->pixmap().isNull())shown=shown || pix->pixmap().toImage().pixelColor(0,0).red()==10+i;check(shown && runtimeViewer_->roi().has_value(),"Selected branch actual raster and overlay");check(runtimePreparation_->item(i,3)->text()==runtimeResult_->frames[i].frameId,"Preparation table exact frame ID");if(calibrated){check(runtimePreparation_->item(i,0)->text().contains("runtime-c"+QString::number(i)) && runtimePreparation_->item(i,1)->text().contains("mm") && runtimePreparation_->item(i,1)->text().contains("Runtime scale "+QString::number(i)) && runtimeSummary_->text().contains("Runtime scale "+QString::number(i)),"Selected immutable model shown per source");for(const auto& result:runtimeResult_->tools)if(result.toolId==metrics[i])check(result.outputs["value"].valid && std::abs(std::get<double>(result.outputs["value"].data)-(i+1))<.001,"UI actual independent calibrated measurement");}}
        if(n==1){findChild<QPushButton*>("runtimeHistory")->click();check(pages_->currentIndex()==pageIds_.indexOf("logs"),"Operator can open history");findChild<QPushButton*>("returnToRuntime")->click();check(pages_->currentIndex()==pageIds_.indexOf("runtime") && runtimeCoordinator_->state()==RuntimeState::Running,"History return preserves operator run state");}
        runtimeViewer_->fitImage();resize(1400,900);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("kmc10d-runtime-%1-normal.png").arg(n))),"Runtime normal screenshot");if(calibrated)check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath("kmc10d-runtime-calibrated-normal.png")),"Calibrated Runtime normal screenshot");resize(850,600);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("kmc10d-runtime-%1-small.png").arg(n))),"Runtime small screenshot");if(calibrated)check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath("kmc10d-runtime-calibrated-small.png")),"Calibrated Runtime small screenshot");check(runtimeSources_->isEnabled() && runtimePreparation_->verticalScrollBar(),"Small screen sources accessible");
        runtimeAction("stop");check(!runtimeResult_ && !runtimeViewer_->hasImage(),"Stop clears current result");selectPage("logs");refreshLogs();check(logTable_->rowCount()>0 && !ProductionResultCodec::csv(runtime_->records()).isEmpty(),"Logs and CSV access");if(calibrated){bool versions=false;for(int row=0;row<logTable_->rowCount();++row){const auto text=logTable_->item(row,0)->data(Qt::UserRole).toString();versions=versions || (text.contains("Runtime scale 0") && text.contains("Runtime scale 1"));}check(versions,"Logs UI keeps per-source historical model versions");}
        if(n==1){
            int selected=-1;for(int row=0;row<logTable_->rowCount();++row)if(!logTable_->item(row,0)->data(Qt::UserRole+1).toBool()){selected=row;break;}
            check(selected>=0,"Normal history record remains selectable");logTable_->selectRow(selected);
            const auto path=QDir(artifacts).filePath("normal-selected-records.csv");exportLogCsv(path);
            QFile file(path);check(file.open(QIODevice::ReadOnly) && file.readAll()==ProductionResultCodec::csv(runtime_->records()),"Single CSV action dispatches normal records to their codec");
        }
        runtimeAction("run");runtimeAction("cycle");runtimeAction("stop");wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Skipped && runtimeResult_->frames.isEmpty() && !runtimeViewer_->hasImage(),"Busy Stop suppresses actual frames");runtimeAction("stop");
    }
    // Offline industrial presentation fixture: never injects a backend into the app.
    const auto output=QDir(artifacts).filePath("icm/08c");check(QDir().mkpath(output),"Industrial Runtime screenshot directory");
    for(int n:{1,2,8,10}){
        service_.createRecipe(QString("Offline industrial %1").arg(n),"",ApplicationType::Measurement,"industrial");MeasurementSetupContract contract;
        for(int i=0;i<n;++i){const auto suffix=QString::number(n)+"-"+QString::number(i);MeasurementCamera camera{{"offline-camera-"+suffix},"Offline camera "+suffix,{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},PhysicalDeviceIdentity{"VF","Offline model","SYNTHETIC-"+suffix,DeviceIdentityConfidence::SerialReported}};
            GeometryDescriptor geometry;geometry.resolution={640,480};geometry.sensorRoi=QRect(0,0,640,480);geometry.binning=QSize(1,1);geometry.decimation=QSize(1,1);geometry.reverse=GeometryMirror{};geometry.complete=true;
            MeasurementSetupDescriptor setup{{"offline-setup-"+suffix},camera.id,"Offline setup","lens","plane",1,1,geometry};AcquisitionProfile profile{{"offline-profile-"+suffix},camera.id,setup.id,"Mono8 software",1,{"Mono8",{true,"FrameStart","Software"},AutoMode::Off,100,AutoMode::Off,1,{}}};
            contract.cameras<<camera;contract.setups<<setup;contract.profiles<<profile;
        }
        auto combined=measurementWorkspace_->environment().current;combined.cameras+=contract.cameras;combined.setups+=contract.setups;combined.profiles+=contract.profiles;measurementWorkspace_->replace(combined);for(const auto& profile:contract.profiles){const auto source=service_.addTool("industrial_camera");service_.bindIndustrialSource(source,contract,profile.id,1000);}service_.save();refreshWorkspace();ensureRuntime();runtimeCoordinator_->load(*service_.activeRecipe(),dir.path(),RuntimeTrigger::Manual);selectPage("runtime");refreshRuntime();
        check(runtimePreparation_->rowCount()==n && !runtimeResult_ && !runtimeViewer_->hasImage(),"Industrial preparation has no fabricated result");
        for(int i=0;i<n;++i)check(runtimePreparation_->item(i,0)->text().contains("Offline camera "+QString::number(n)+"-"+QString::number(i)) && runtimePreparation_->item(i,1)->toolTip().contains("offline-profile-"+QString::number(n)+"-"+QString::number(i)) && runtimePreparation_->item(i,3)->text()=="Kare yok","Offline industrial serial/setup/profile without actual frame");
        runtimeAction("run");check(runtimeCoordinator_->state()==RuntimeState::Fault && !runtimeResult_ && !runtimeViewer_->hasImage(),"Native admission stays unavailable");
        check(runtimePreparation_->item(0,2)->text()=="Kullanılamıyor" && runtimePreparation_->item(0,1)->text().contains("Ölçüm doğrulanmadı"),"Unavailable differs from a raw-only result");
        setWindowState(Qt::WindowNoState);resize(1400,900);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,output+QString("/industrial-%1-normal.png").arg(n)),"Industrial normal screenshot");resize(850,600);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,output+QString("/industrial-%1-small.png").arg(n)),"Industrial small screenshot");
        check(runtimePreparation_->item(n-1,0)->toolTip().contains("SYNTHETIC-"+QString::number(n)+"-"+QString::number(n-1)),"All source details remain accessible");
    }
    const auto metricFixture=qEnvironmentVariable("VISIONFLOW_CCUX13_RECORD");
    if(!metricFixture.isEmpty()){
        QFile file(metricFixture);check(file.open(QIODevice::ReadOnly),"metric UI fixture record");const auto record=QJsonDocument::fromJson(file.readAll()).object();ExperimentalRuntime::validateRecord(record);
        const auto directory=record["configuration"].toObject()["directory"].toString();service_.openProject(QDir(directory).filePath("runtime.vfproject"));service_.selectRecipe(record["recipeId"].toString());refreshWorkspace();ensureRuntime();
        const auto archive=service_.path()+".production";for(const auto& historical:ProductionStore(QDir(directory).filePath("archive")).recent("experimental_cycles"))ProductionStore(archive).append("experimental_cycles",historical["recordId"].toString(),historical);
        auto* policy=findChild<QCheckBox*>("experimentalRuntime");check(!policy,"normal Runtime starts without experimental opt-in");
        MultiSourceDependencies frozen;const auto entries=record["dependencies"].toObject()["sourceSet"].toObject();QStringList metricSources;QString rawSource;
        for(auto i=entries.begin();i!=entries.end();++i){frozen.sources[i.key()].measurementSource=i->toObject()["measurementSource"].toObject();const auto model=i->toObject()["experimentalCalibration"].toObject();if(model.isEmpty()){rawSource=i.key();continue;}const auto selection=model["selection"].toObject();
            frozen.experimentalCalibrations[i.key()]=std::make_shared<ExperimentalScopedCalibrationSnapshot>(ExperimentalScopedCalibrationSnapshot{{SetupReference::fromJson(model["target"].toObject()),DeviceBinding::fromJson(model["device"].toObject()),{CalibrationModelRecord::decode(model["record"].toObject()),{selection["id"].toString(),selection["contentId"].toString(),selection["version"].toInt(),selection["revision"].toInt()},model["scope"].toObject(),model["acquisition"].toObject()}}});metricSources<<i.key();
        }
        // Offline smoke composition only: validated fixture models and exact saved raster,
        // no hardware backend or production completion is introduced.
        runtime_->setAutomaticAcquisition([frozen](const Recipe&){return frozen;});runtimeCoordinator_->load(*service_.activeRecipe(),directory);selectPage("runtime");refreshRuntime();
        check(runtimePreparation_->rowCount()==3 && !runtimeResult_ && !runtimeViewer_->hasImage(),"mixed Load is preparation only");runtimeAction("run");check(runtimeCoordinator_->state()==RuntimeState::Running && !runtimeResult_,"metric Run has no invented result");
        for(int row=0;row<runtimePreparation_->rowCount();++row){const auto id=runtimeSources_->itemData(row).toString();check(runtimePreparation_->item(row,1)->text().contains(id==rawSource?"px":"mm"),"source-specific actual units in preparation");if(id!=rawSource)check(runtimePreparation_->item(row,0)->text().contains("Synthetic OECS") && !runtimePreparation_->item(row,1)->text().contains("Deneysel"),"camera name and experimental calibration visible");}
        QLockFile lock(archive+"/archive.lock");check(lock.tryLock(0),"UI metric recording lock");runtimeAction("cycle");wait();check(runtimePending_ && !runtimeResult_ && !runtimeViewer_->hasImage(),"metric result hidden until persistent commit");lock.unlock();runtimeAction("retry");check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && runtimeResult_->frames.size()==3,"metric frozen retry commits real results");
        const auto output=QDir(artifacts).filePath("ccux13");check(QDir().mkpath(output),"metric UI evidence directory");
        for(const auto& source:metricSources){runtimeSources_->setCurrentIndex(runtimeSources_->findData(source));check(runtimeSummary_->text().contains(" mm") && runtimeSummary_->text().contains("Kullanılamıyor") && !runtimeSummary_->text().contains("Experimental") && !runtimeSummary_->text().contains("Deneysel") && runtimeSummary_->text().contains(source) && !runtimeSummary_->text().contains("PASS"),"source-scoped experimental mm result with blocked output");}
        runtimeSources_->setCurrentIndex(runtimeSources_->findData(rawSource));check(runtimeSummary_->text().contains("px2") && !runtimeSummary_->text().contains(" • mm •"),"raw branch never inherits metric model");
        setWindowState(Qt::WindowNoState);runtimeSources_->setCurrentIndex(runtimeSources_->findData(metricSources.first()));
        for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();runtimeViewer_->fitImage();QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,output+QString("/runtime-%1.png").arg(size.width())),"metric Runtime screenshot");}
        runtimeAction("stop");check(!runtimeResult_ && !runtimeViewer_->hasImage(),"metric Stop clears active display");selectPage("logs");refreshLogs();bool metric=false,legacy=false,error=false,stopped=false;
        for(int row=0;row<logTable_->rowCount();++row){const auto text=logTable_->item(row,0)->data(Qt::UserRole+2).toString();check(!logTable_->item(row,2)->text().contains("Deneysel") && logTable_->item(row,4)->text()=="Kullanılamıyor","all experimental codec versions stay separate from production Logs");if(text.contains("visionflow.experimental-runtime1"))legacy=true;if(logTable_->item(row,2)->text().contains("ERROR")){error=true;check(logTable_->item(row,3)->text()=="NOT_EVALUATED","error row has no numeric result");}if(logTable_->item(row,2)->text().contains("SKIPPED")){stopped=true;check(logTable_->item(row,3)->text()=="NOT_EVALUATED","Stop row has no numeric result");}if(logTable_->item(row,3)->text().contains(" mm")){metric=true;logTable_->selectRow(row);}}
        check(metric && legacy && error && stopped,"Logs render new mm, old raw, error and Stop records together");
        check(!findChild<QPushButton*>("exportExperimentalCsv") && findChild<QPushButton*>("exportLogsCsv"),"one CSV action for selected record type");
        const auto exportPath=output+"/selected-records.csv";exportLogCsv(exportPath);QFile exported(exportPath);check(exported.open(QIODevice::ReadOnly),"selected CSV open");
        check(exported.readAll()==ExperimentalRuntime::csv(ProductionStore(archive).recent("experimental_cycles")),"selected metric record uses compatible evidence exporter");

        for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,output+QString("/logs-%1.png").arg(size.width())),"metric Logs screenshot");}
        runtime_->setAutomaticAcquisition();check(!runtime_->active() && !runtimeResult_,"provider replacement clears prepared model");
        {MainWindow restarted(QDir(artifacts).filePath("ccux13-restart.ini"));restarted.service_.openProject(service_.path());restarted.refreshWorkspace();restarted.selectPage("logs");restarted.refreshLogs();check(!restarted.runtime_ && !restarted.findChild<QCheckBox*>("experimentalRuntime") && restarted.logTable_->rowCount()==logTable_->rowCount(),"restart history readable without restoring Run permission");}
    }
    QTextStream(stdout)<<checks<<" multi-source Runtime UI checks passed\n";return true;
}
}
