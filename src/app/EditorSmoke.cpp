#include "domain/GeometryMeasurementConfig.hpp"
#include "CameraFlowSmokeAudit.hpp"
#include <QKeyEvent>
#include <QMouseEvent>
#include "MetrologyEditor.hpp"
#include "MainWindow.hpp"
#include "PreprocessEditor.hpp"
#include "CodeReadEditor.hpp"
#include "EdgePointEditor.hpp"
#include "MetrologyEditor.hpp"
#include "application/ToolCatalog.hpp"
#include "EdgeProfileView.hpp"
#include "application/BackendCapabilities.hpp"
#include "application/ProductionResultCodec.hpp"
#include "application/ToolResultCodec.hpp"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QSplitter>
#include <QTabWidget>
#include "ImageViewer.hpp"
#include <QDialog>
#include "../../tests/chessboard_fixture.hpp"
#include <QApplication>
#include <QMenuBar>
#include <QAction>
#include <QStackedWidget>
#include <QMenuBar>
#include <QAction>
#include <QStackedWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QProcess>
#include <QLockFile>
#include <QSignalBlocker>
#include <QSettings>
#include <QScrollArea>
#include <QScrollBar>
#include <QJsonArray>
#include <QTableWidget>
#include <QFile>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QElapsedTimer>
#include <QTimer>
#include <QMessageBox>
#include <QThread>
#include <QSpinBox>
namespace visionflow {
bool MainWindow::verifyCommunication(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto wait=[&](auto predicate,int ms=10000){QElapsedTimer t;t.start();while(!predicate() && t.elapsed()<ms){QApplication::processEvents();QThread::msleep(1);}check(predicate(),"Communication UI timeout");};
    selectPage("communication");check(plc_ && !plc_->listening() && !plcStatus_->text().contains("PASS"),"Communication has no fabricated result");
    QString measurement;for(const auto& tool:editorTools(runtime_->active()->recipe().definition))if(tool.kind=="measurement")measurement=tool.id;
    runtimeAction("stop");findChild<QAction*>("setup_communication")->trigger();
    check(pages_->currentIndex()==pageIds_.indexOf("communication"),"Setup opens Communication");
    for(int i=0;i<navigation_->count();++i)check(navigation_->item(i)->data(Qt::UserRole)!="communication" && navigation_->item(i)->data(Qt::UserRole)!="calibration","Setup pages absent from sidebar");
    findChild<QCheckBox*>("plc_enabled")->setChecked(false);communicationAction("save");
    check(!findChild<QPushButton*>("plc_listen")->isEnabled() && findChild<QLabel*>("plcHint")->text().contains("işaretleyin"),"disabled listener explains activation");
    findChild<QCheckBox*>("plc_enabled")->setChecked(true);
    check(findChild<QPushButton*>("plc_listen")->isEnabled(),"activation enables listener without Run");
    findChild<QComboBox*>("recipeTrigger")->setCurrentText("SoftwareTrigger");
    plcMapping_->setCurrentIndex(plcMapping_->findData(measurement));check(plcMapping_->currentData()==measurement,"select actual Measurement by name");findChild<QSpinBox*>("plcPort")->setValue(0);findChild<QSpinBox*>("plcAckTimeout")->setValue(100);
    communicationAction("save");{QSettings settings(settingsFile_,QSettings::IniFormat);check(settings.value("socket/enabled").toBool() && settings.value("socket/port").toInt()==0 && settings.value("socket/ackTimeout").toInt()==100,"UI persists independent socket settings");}
    findChild<QPushButton*>("plc_listen")->click();check(plc_ && plc_->listening(),"UI starts TCP listener while stopped");
    const auto session=plc_->sessionId();service_.save();refreshRecipes();
    for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==service_.activeRecipe()->id)recipeTable_->selectRow(row);
    findChild<QPushButton*>("loadRecipe")->click();check(findChild<QPushButton*>("runtime_run")->isEnabled(),"listening allows Load/Run");runtimeAction("run");check(plc_->state()=="READY" && plc_->sessionId()==session,"Run binds coordinator without restarting listener");selectPage("communication");
    check(!findChild<QPushButton*>("runtime_run")->isEnabled(),"double Run remains disabled while running");
    auto cycleCount=[this]{return QDir(runtimePath_+".production/cycles").entryList({"*.json"},QDir::Files).size();};
    const auto records=cycleCount();
    QProcess simulator;simulator.setProgram(QDir(QCoreApplication::applicationDirPath()).filePath("visionflow_plc_simulator.exe"));
    simulator.setArguments({"--port",QString::number(plc_->port()),"--duplicate","--reconnect","--ack-delay","1500"});simulator.start();
    wait([&]{return plc_->state()=="ACK_TIMEOUT" || simulator.state()==QProcess::NotRunning;});
    check(plc_->state()=="ACK_TIMEOUT" && plc_->blocking() && runtimeResult_ && runtimeResult_->tools.last().decision==PartDecision::Pass,"UI real result and ACK timeout");
    check(!close(),"UI cannot close with unacknowledged PLC result");workspaceError_->hide();
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("plc-ack-timeout.png"));resize(850,600);QApplication::processEvents();grab().save(QDir(artifacts).filePath("plc-communication-small.png"));resize(1400,900);
    wait([&]{return simulator.state()==QProcess::NotRunning;});check(simulator.exitStatus()==QProcess::NormalExit && simulator.exitCode()==0,"standalone simulator duplicate/reconnect/ACK");
    QFile trace(QDir(artifacts).filePath("plc-simulator.log"));check(trace.open(QIODevice::WriteOnly),"simulator trace");trace.write(simulator.readAllStandardOutput());trace.write(simulator.readAllStandardError());trace.close();
    check(cycleCount()==records+1 && plc_->state()=="READY","one cycle for repeated request after reconnect");
    runtimeAction("stop");runtimeAction("stop");selectPage("logs");bool acked=false;for(int row=0;row<logTable_->rowCount();++row)if(logTable_->item(row,4)->text()=="ACKED")acked=true;check(acked,"Logs uses durable ACK evidence");QApplication::processEvents();grab().save(QDir(artifacts).filePath("plc-logs.png"));
    selectPage("communication");refreshCommunication();QApplication::processEvents();check(pageTitle_->text()=="Communication","Setup page title remains correct");grab().save(QDir(artifacts).filePath("plc-ready.png"));findChild<QPushButton*>("plc_stop")->click();check(!plc_->listening(),"explicit stop after ACK");
    // Saved machine settings start transport without a project or automatic Run.
    const auto settingsPath=QDir(artifacts).filePath("p3-auto-start.ini");
    {QSettings settings(settingsPath,QSettings::IniFormat);settings.clear();settings.setValue("socket/enabled",true);settings.setValue("socket/autoStart",true);settings.setValue("socket/address","127.0.0.1");settings.setValue("socket/port",0);settings.sync();}
    {MainWindow startup(settingsPath);check(startup.plc_->listening() && !startup.runtime_ && !startup.service_.hasProject(),"autoStart independent from project/Run");startup.selectPage("communication");startup.resize(850,600);startup.show();QApplication::processEvents();check(startup.pages_->currentIndex()==startup.pageIds_.indexOf("communication") && startup.pageTitle_->text()=="Communication","Setup page survives show and focus");startup.grab().save(QDir(artifacts).filePath("p3-auto-start-small.png"));startup.plc_->stop();}
    runtimeAction("stop");service_.setRuntimeOptions("Manual",measurement);service_.save();runtimeCoordinator_->load(*service_.activeRecipe(),QFileInfo(service_.path()).absolutePath());runtimeAction("run");refreshRuntime();return true;
}
bool MainWindow::verifyRuntime(const QString& artifacts) {
    auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    auto wait=[&]{QElapsedTimer timer;timer.start();while(runtimeJobs_.busy() && timer.elapsed()<10000){QApplication::processEvents();QThread::msleep(1);}check(!runtimeJobs_.busy(),"Runtime worker completed");};
    service_.setCalibrationSetup("fixture-lens","fixture-plane");
    const auto frame=RuntimeService::probe(*service_.activeRecipe(),QDir(artifacts).absolutePath(),{});
    CalibrationWorkflow::bind(service_,frame,service_.path()+".calibrations.json","fixture-scale");
    QString measurement;for(auto t:service_.tools())if(t.kind=="measurement"){measurement=t.id;t.parameters["upperMm"]=9.98;service_.updateTool(t);}service_.save();refreshWorkspace();selectPage("runtime");
    check(!runtimeResult_ && !runtimeViewer_->hasImage() && !runtimeSummary_->text().contains("PASS"),"Runtime initial no invented result");
    const auto recipeId=service_.activeRecipe()->id;
    auto select=[&](const QString& id){recipeSearch_->clear();typeFilter_->setCurrentIndex(0);refreshRecipes();for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==id)recipeTable_->selectRow(row);};
    auto load=[&](const char* choice=nullptr){select(recipeId);if(choice)QTimer::singleShot(0,this,[this,choice,artifacts]{auto* dialog=findChild<QMessageBox*>("loadRecipeChoice");if(dialog){dialog->grab().save(QDir(artifacts).filePath("p2-dirty-load.png"));dialog->findChild<QPushButton*>(choice)->click();}});findChild<QPushButton*>("loadRecipe")->click();};
    load();check(runtimeCoordinator_->loaded() && runtimeCoordinator_->loaded()->id==recipeId && !runtime_->active() && !runtimeJobs_.busy() && !runtimeViewer_->hasImage(),"Load independent saved copy without acquisition/activation");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("p2-loaded-stopped.png"));
    findChild<QPushButton*>("runtime_run")->click();check(runtimeCoordinator_->state()==RuntimeState::Running && runtime_->active() && !runtimeResult_ && !runtimeJobs_.busy(),"Run prepares without frame or fabricated PASS");
    check(operatorMode_ && isFullScreen() && !menuBar()->isEnabled() && !navigation_->isEnabled(),"Run locks fullscreen navigation");
    selectPage("calibration");check(pages_->currentIndex()==pageIds_.indexOf("runtime"),"Run blocks Setup page navigation");
    findChild<QAction*>("setup_communication")->trigger();check(pages_->currentIndex()==pageIds_.indexOf("runtime"),"Run blocks Setup action");
    check(operatorMode_ && isFullScreen() && !menuBar()->isEnabled() && !navigation_->isEnabled(),"Run locks fullscreen navigation");
    selectPage("calibration");check(pages_->currentIndex()==pageIds_.indexOf("runtime"),"Run blocks Setup page navigation");
    findChild<QAction*>("setup_communication")->trigger();check(pages_->currentIndex()==pageIds_.indexOf("runtime"),"Run blocks Setup action");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("p2-running-wait.png"));
    const auto first=runtime_->active()->validation().id;
    load();check(runtimeCoordinator_->state()==RuntimeState::Running && runtime_->active()->validation().id==first,"Load blocked while running");workspaceError_->hide();
    check(!close(),"Running close guard");workspaceError_->hide();
    findChild<QPushButton*>("runtime_cycle")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && runtimeSummary_->text().contains("9.975000 mm • PASS"),"Runtime real calibrated PASS");runtimeTools_->setCurrentIndex(runtimeTools_->findData(measurement));runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-pass.png"));
    resize(850,600);QApplication::processEvents();runtimeViewer_->fitImage();QApplication::processEvents();check(width()==850 && height()==600,"populated Runtime minimum size");grab().save(QDir(artifacts).filePath("runtime-pass-small.png"));resize(1400,900);
    findChild<QPushButton*>("runtime_cycle")->click();for(auto t:service_.tools())if(t.id==measurement){t.parameters["upperMm"]=9.9749;service_.updateTool(t);}refreshWorkspace();wait();check(runtimeResult_ && runtimeSummary_->text().contains("PASS") && runtime_->active()->validation().id==first,"draft edit preserves loaded active snapshot");
    findChild<QPushButton*>("runtime_stop")->click();check(!runtimeResult_ && !runtimeViewer_->hasImage(),"idle Stop clears current result");
    check(!operatorMode_ && !isFullScreen() && menuBar()->isEnabled() && navigation_->isEnabled(),"Stop restores window and navigation");
    findChild<QAction*>("setup_calibration")->trigger();check(pages_->currentIndex()==pageIds_.indexOf("calibration"),"Setup opens Calibration after Stop");selectPage("runtime");
    check(!operatorMode_ && !isFullScreen() && menuBar()->isEnabled() && navigation_->isEnabled(),"Stop restores window and navigation");
    findChild<QAction*>("setup_calibration")->trigger();check(pages_->currentIndex()==pageIds_.indexOf("calibration"),"Setup opens Calibration after Stop");selectPage("runtime");
    const auto version=runtimeCoordinator_->loaded()->recipeVersion;
    load("cancelLoad");check(service_.dirty() && runtimeCoordinator_->loaded()->recipeVersion==version,"dirty cancel preserves draft and load");
    load("loadSaved");check(service_.dirty() && runtimeCoordinator_->loaded()->recipeVersion==version,"dirty saved version preserves draft");
    findChild<QPushButton*>("runtime_run")->click();findChild<QPushButton*>("runtime_cycle")->click();wait();check(runtimeSummary_->text().contains("PASS"),"saved load excludes dirty tolerance");findChild<QPushButton*>("runtime_stop")->click();
    load("saveAndLoad");check(!service_.dirty() && runtimeCoordinator_->loaded()->recipeVersion==version+1,"dirty save and load new version");
    findChild<QPushButton*>("runtime_run")->click();findChild<QPushButton*>("runtime_cycle")->click();wait();check(runtimeResult_ && runtimeSummary_->text().contains("9.975000 mm • FAIL"),"new saved tolerance produces real FAIL");
    findChild<QPushButton*>("runtime_cycle")->click();findChild<QPushButton*>("runtime_stop")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Skipped && !runtimeSummary_->text().contains("mm •") && !runtimeViewer_->hasImage(),"Stop cancels shared coordinator token");
    findChild<QPushButton*>("runtime_run")->click();findChild<QPushButton*>("runtime_cycle")->click();QLockFile lock(service_.path()+".production/archive.lock");lock.setStaleLockTime(0);check(lock.tryLock(),"UI log lock fixture");wait();check(runtimePending_ && runtime_->busy() && !runtimeResult_ && !findChild<QPushButton*>("runtime_cycle")->isEnabled(),"failed record suppresses result and blocks cycle");QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-record-pending.png"));findChild<QPushButton*>("runtime_stop")->click();load();check(runtimeCoordinator_->loaded()->id==recipeId && runtimePending_,"pending record prevents replacement");const auto pendingCycle=runtimePending_->cycleId;lock.unlock();findChild<QPushButton*>("runtime_retry")->click();check(!runtimePending_ && !runtimeResult_ && !runtimeViewer_->hasImage() && runtimeCoordinator_->state()==RuntimeState::LoadedStopped,"retry after Stop persists frozen completion without restoring visible result");bool committed=false;for(const auto& record:ProductionStore(service_.path()+".production").recent("cycles"))if(record["cycleId"]==pendingCycle)committed=true;check(committed,"stopped retry durably records the original cycle");
    // Selection, editor and loaded identity remain independent. Invalid Load preserves previous copy.
    const auto selectedOnly=service_.createRecipe("Selected only","",ApplicationType::DefectInspection,"file");
    const auto other=service_.createRecipe("Unconfigured selection","",ApplicationType::Measurement,"file");service_.save();refreshWorkspace();select(selectedOnly);check(service_.activeRecipe()->id==other && runtimeCoordinator_->loaded()->id==recipeId,"selected/editor/loaded identity separation");
    select(other);findChild<QPushButton*>("loadRecipe")->click();check(runtimeCoordinator_->loaded()->id==recipeId && workspaceError_->text().contains("Korunan"),"failed Load retains previous recipe");
    select(recipeId);findChild<QPushButton*>("openRecipe")->click();check(service_.activeRecipe()->id==recipeId && runtimeCoordinator_->loaded()->id==recipeId,"Edit Recipe opens editor independently");
    QString assetPath;for(const auto& t:service_.tools())if(t.kind=="image_file")assetPath=QDir(artifacts).absoluteFilePath(t.parameters["path"].toString());
    check(!assetPath.isEmpty() && QFile::rename(assetPath,assetPath+".p2-held"),"temporarily hide isolated image asset");
    load();const bool retained=runtimeCoordinator_->loaded()->id==recipeId && workspaceError_->text().contains("bulunamadı");
    findChild<QPushButton*>("runtime_run")->click();const bool runRejected=runtimeCoordinator_->state()==RuntimeState::Fault && !runtimeJobs_.busy() && !runtimeResult_ && runtimeSummary_->text().contains("bulunamadı");
    check(QFile::rename(assetPath+".p2-held",assetPath),"restore isolated asset");check(runRejected,"Run failure visible without acquisition/result");check(retained,"missing asset Load retains previous copy with source error");
    // Missing asset and failed save both preserve the loaded copy and dirty draft.
    for(auto t:service_.tools())if(t.id==measurement){t.parameters["upperMm"]=9.98;service_.updateTool(t);}refreshWorkspace();
    QFile disk(service_.path());check(disk.open(QIODevice::ReadOnly),"read saved project");const auto savedBytes=disk.readAll();disk.close();check(disk.open(QIODevice::WriteOnly|QIODevice::Append),"external edit fixture");disk.write(" ");disk.close();
    load("saveAndLoad");check(service_.dirty() && runtimeCoordinator_->loaded()->recipeVersion==version+1,"failed save prevents Load");check(disk.open(QIODevice::WriteOnly|QIODevice::Truncate),"restore isolated project");disk.write(savedBytes);disk.close();
    load("saveAndLoad");findChild<QPushButton*>("runtime_run")->click();check(verifyCommunication(artifacts),"legacy TCP Communication regression");
    runtimeAction("stop");runtimeAction("stop");selectPage("logs");check(logTable_->rowCount()>=5,"persistent Logs rows");logTable_->selectRow(0);QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-logs.png"));
    selectPage("runtime");runtimeAction("run");service_.setCalibrationSetup("physical lens changed","fixture-plane");refreshWorkspace();findChild<QPushButton*>("runtime_cycle")->click();check(!runtimeJobs_.busy() && !runtimeResult_ && runtimeCoordinator_->state()==RuntimeState::Fault && runtimeSummary_->text().contains("kurulum değişti"),"active setup invalidation blocks acquisition");QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-invalid.png"));
    RuntimeService restored(service_.path()+".production",service_.path()+".calibrations.json",RuntimeIdentity::localEngineer());check(!restored.active() && restored.records().size()>=5,"restart archive not activation permit");
    resize(850,600);QApplication::processEvents();check(width()==850 && height()==600,"Runtime minimum size");grab().save(QDir(artifacts).filePath("runtime-small.png"));resize(1400,900);QApplication::processEvents();service_.save();return true;
}
bool MainWindow::verifyWebcam(const QString& artifacts) {
    const auto recipe=service_.createRecipe("Webcam smoke","",ApplicationType::Measurement,"webcam");
    selectPage("editor");refreshWorkspace();
    for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="webcam")toolbox_->setCurrentRow(i);
    findChild<QPushButton*>("addTool")->click();
    auto* index=propertiesBody_->findChild<QSpinBox*>("webcamIndex");
    auto* timeout=propertiesBody_->findChild<QSpinBox*>("webcamTimeout");
    if(!index || !timeout)return false;
    index->setValue(1);timeout->setValue(10000);index->setValue(0);
    const auto source=selectedToolId_;
    service_.save();service_.openProject(service_.path());service_.selectRecipe(recipe);refreshWorkspace();selectTool(source);
    if(service_.tools().size()!=1 || service_.tools().first().parameters["timeoutMs"]!=10000 || editorSession_.frame || editorSession_.cycle)return false;
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("webcam-properties.png"));
    if(QCoreApplication::arguments().contains("--webcam-capture")) {
        runFlow();QElapsedTimer timer;timer.start();
        while(editorSession_.busy(SessionJob::Execution) && timer.elapsed()<15000){QApplication::processEvents();QThread::msleep(1);}
        if(editorSession_.busy(SessionJob::Execution) || !editorSession_.cycle || editorSession_.cycle->state!=ExecutionState::Done || !editorSession_.frame)return false;
        QApplication::processEvents();grab().save(QDir(artifacts).filePath("webcam-capture.png"));
    }
    return true;
}
bool MainWindow::verifyEditor(const QString& artifacts) {
    if (!service_.activeRecipe()) return false;
    const QString recipeId=service_.activeRecipe()->id;
    const QString fixture=QDir(artifacts).filePath("görüntü_ölçüm.png");
    QImage image(2592,1944,QImage::Format_RGB888); image.fill(QColor(51,102,153));
    {
        QPainter painter(&image); painter.fillRect(QRect(160,160,2272,1624),QColor("#243242"));
        painter.setPen(QPen(QColor("#62768f"),2));
        for (int x=200;x<2400;x+=160) painter.drawLine(x,200,x,1740);
        for (int y=200;y<1740;y+=160) painter.drawLine(200,y,2400,y);
        painter.setBrush(QColor("#9eaab4")); painter.setPen(QPen(QColor("#d6e1ea"),7));
        painter.drawRoundedRect(QRect(700,650,1192,650),90,90);
        painter.setBrush(QColor("#243242")); painter.drawEllipse(QPoint(960,975),110,110); painter.drawEllipse(QPoint(1630,975),110,110);
        painter.setPen(QColor("#edf4fb")); auto font=painter.font(); font.setPixelSize(52); painter.setFont(font);
        painter.drawText(210,340,"VisionFlow • 5 MP test görüntüsü");
    }
    if (!image.save(fixture)) return false;
    if (!loadEditorImage(fixture) || !editorSession_.frame || editorSession_.frame->size()!=image.size()) return false;
    if (!editorSession_.frame->pixelText(0,0).contains("R:51 G:102 B:153")) return false;
    const auto sourceId=selectedToolId_;
    const QString invalid=QDir(artifacts).filePath("corrupt-image.png");
    {QFile file(invalid); if (!file.open(QIODevice::WriteOnly)) return false; file.write("not an image");}
    if (loadEditorImage(invalid) || !editorSession_.frame || service_.imageFilePath()!=fixture) return false;
    for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="template_match")toolbox_->setCurrentRow(i);
    findChild<QPushButton*>("addTool")->click();
    const auto matchId=selectedToolId_; if (matchId==sourceId) return false;
    auto* score=propertiesBody_->findChild<QDoubleSpinBox*>("minScore"); if (!score) return false;
    score->setValue(84.5);
    auto* name=propertiesBody_->findChild<QLineEdit*>("toolName"); if (!name) return false;
    name->setText("Match_Part"); if (!applyToolProperties()) return false;
    name->setText(""); if (applyToolProperties()) return false;
    saveProject(); if (!service_.dirty()) return false;
    name->setText("Match_Part"); if (!applyToolProperties()) return false;
    if (!service_.dirty() || service_.activeRecipe()->status!=RecipeStatus::Draft) return false;
    findChild<QAction*>("zoomIn")->trigger(); const auto zoom=imageViewer_->zoomFactor();
    findChild<QAction*>("zoomOut")->trigger(); if (imageViewer_->zoomFactor()>=zoom) return false;
    selectTool(sourceId); if (!propertiesBody_->findChild<QPushButton*>("removeTool")) return false;
    imageViewer_->actualSize(); imageViewer_->centerOn(1000,900);
    const auto viewPoint=imageViewer_->mapFromScene(QPointF(1000,900));
    QMouseEvent move(QEvent::MouseMove,QPointF(viewPoint),QPointF(imageViewer_->viewport()->mapToGlobal(viewPoint)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(imageViewer_->viewport(),&move);
    if (!pixelInfo_->text().contains("X:1000 Y:900")) return false;
    selectTool(matchId); service_.save(); service_.openProject(service_.path()); service_.selectRecipe(recipeId); imageBinding_.clear(); refreshWorkspace();
    const auto tools=service_.tools(); if (tools.size()!=2 || tools[1].id!=matchId || tools[1].name!="Match_Part" || std::abs(tools[1].parameters["minScore"].toDouble()-0.845)>1e-9) return false;
    if (!editorSession_.frame || editorSession_.frame->size()!=image.size()) return false;
    selectTool(matchId); selectPage("editor"); imageViewer_->fitImage(); QApplication::processEvents();
    grab().save(QDir(artifacts).filePath("step3-editor.png"));
    // A different recipe must never inherit the previous recipe's frame.
    service_.selectRecipe(service_.project().recipes[0].id); refreshWorkspace();
    if (editorSession_.frame || imageViewer_->hasImage()) return false;
    service_.selectRecipe(recipeId); refreshWorkspace();
    if (!editorSession_.frame || service_.dirty()) return false;
    selectTool(sourceId); auto* enabled=propertiesBody_->findChild<QCheckBox*>("toolEnabled");
    enabled->setChecked(false); if (imageViewer_->hasImage()) return false;
    enabled->setChecked(true); if (!imageViewer_->hasImage()) return false;
    service_.save(); refreshWorkspace(); selectTool(matchId);
    {QFile file(fixture); if (!file.open(QIODevice::WriteOnly)) return false; file.write("broken after save");}
    findChild<QAction*>("reloadImage")->trigger(); if (editorSession_.frame || imageViewer_->hasImage() || service_.dirty()) return false;
    if (!image.save(fixture)) return false;
    findChild<QAction*>("reloadImage")->trigger();
    if (!imageViewer_->hasImage() || !workspaceError_->isHidden() || service_.dirty()) return false;
    QApplication::processEvents(); grab().save(QDir(artifacts).filePath("step3-editor.png")); return true;
}
bool MainWindow::verifyMeasurement(const QString& artifacts) {
    auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    check(smokeMode_ && service_.path().startsWith(QDir(artifacts).absolutePath()),"isolated calibration fixture");
    QFile::remove(service_.path()+".calibrations.json");
    const auto recipeId=service_.createRecipe("Calibrated measurement","",ApplicationType::Measurement,"file");refreshWorkspace();selectPage("editor");
    QImage fixture(1000,600,QImage::Format_Grayscale8);fixture.fill(0);{QPainter p(&fixture);p.fillRect(QRect(200,100,400,300),Qt::white);}const auto imagePath=QDir(artifacts).filePath("measurement-input.png");check(fixture.save(imagePath),"measurement image");check(loadEditorImage(imagePath),"measurement source");
    const auto source=selectedToolId_,left=service_.addTool("edge"),right=service_.addTool("edge"),measurement=service_.addTool("measurement");
    for(auto t:service_.tools())if(t.id==right){t.parameters["direction"]="right_to_left";service_.updateTool(t);}refreshWorkspace();
    auto bind=[&](QString target,QString port,QString upstream,QString output){selectTool(target);auto* c=propertiesBody_->findChild<QComboBox*>("input_"+port);check(c!=nullptr,"measurement input");const auto i=c->findData(upstream+"/"+output);check(i>0,"measurement compatible input");c->setCurrentIndex(i);};
    bind(left,"image",source,"image");bind(right,"image",source,"image");bind(measurement,"p1",left,"point");bind(measurement,"p2",right,"point");
    selectTool(measurement);propertiesBody_->findChild<QDoubleSpinBox*>("lowerMm")->setValue(9.97);propertiesBody_->findChild<QDoubleSpinBox*>("upperMm")->setValue(9.98);check(applyToolProperties(),"measurement limits");
    auto wait=[&]{QElapsedTimer t;t.start();while(editorSession_.busy() && t.elapsed()<15000){QApplication::processEvents();QThread::msleep(1);}check(!editorSession_.busy(),"calibration/UI worker timeout");};
    auto result=[&]{check(editorSession_.cycle.has_value(),"measurement cycle");for(const auto& t:editorSession_.cycle->tools)if(t.toolId==measurement)return t;throw std::runtime_error("measurement result missing");};
    runFlow();wait();check(result().errorCode=="CALIBRATION_INVALID" && result().outputs.isEmpty(),"UI unbound error without value");
    service_.setCalibrationSetup("fixture-lens","fixture-plane");
    runExecution(source);wait();check(editorSession_.frame.has_value(),"Legacy fixture acquired");
    const auto pixels=CalibrationWorkflow::installation(*service_.activeRecipe(),*editorSession_.frame).source;
    const auto candidate=CalibrationWorkflow::teach(*service_.activeRecipe(),*editorSession_.frame,"fixture-scale",1,{"teach",{{100,100},pixels},{{500,100},pixels},10});
    const auto frame=candidate.dependency().source;
    const auto model=MeasurementService{}.validate(candidate,{{"holdout-x",{{200,50},frame},{{800,50},frame},15},{"holdout-y",{{300,20},frame},{{300,520},frame},12.5}},.01);
    CalibrationService(service_.path()+".calibrations.json").save(model);
    CalibrationWorkflow::bind(service_,*editorSession_.frame,service_.path()+".calibrations.json","fixture-scale");
    const auto savedDependency=model.dependency();saveProject();
    service_.openProject(service_.path());service_.selectRecipe(recipeId);refreshWorkspace();selectPage("calibration");
    check(service_.activeRecipe()->calibrationBinding==savedDependency,"Legacy binding survives reopen without old UI");
    selectPage("editor");selectTool(measurement);runFlow();wait();check(result().decision==PartDecision::Pass && std::abs(std::get<double>(result().outputs["value"].data)-9.975)<1e-12,"UI calibrated actual Edge PASS");
    check(service_.activeRecipe()->status==RecipeStatus::Draft,"mm execution is not recipe validation");imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("measurement-pass.png"));
    propertiesBody_->findChild<QDoubleSpinBox*>("upperMm")->setValue(9.9749);check(applyToolProperties(),"UI raw fail boundary");runFlow();wait();check(result().state==ExecutionState::Done && result().decision==PartDecision::Fail,"UI FAIL separate from DONE");QApplication::processEvents();grab().save(QDir(artifacts).filePath("measurement-fail.png"));
    runFlow();findChild<QPushButton*>("stopFlow")->click();wait();check(editorSession_.cycle->state==ExecutionState::Skipped && result().outputs.isEmpty() && result().decision==PartDecision::NotEvaluated,"UI stop clears decision");
    selectPage("calibration");service_.setCalibrationSetup("changed-lens","fixture-plane");check(!service_.activeRecipe()->calibrationBinding,"UI installation invalidation");selectPage("editor");runFlow();wait();check(result().errorCode=="CALIBRATION_INVALID" && result().outputs.isEmpty() && !matchInfo_->text().contains("9.975"),"UI changed calibration suppresses previous mm output");QApplication::processEvents();grab().save(QDir(artifacts).filePath("measurement-invalid.png"));
    saveProject();return true;
}
bool MainWindow::verifyExecution(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto recipe=service_.createRecipe("Flow acceptance","",ApplicationType::Measurement,"file");refreshWorkspace();selectPage("editor");
    QImage fixture(1000,600,QImage::Format_Grayscale8);fixture.fill(0);{QPainter painter(&fixture);painter.fillRect(QRect(200,100,400,300),Qt::white);}
    const auto path=QDir(artifacts).filePath("step5-input.png");check(fixture.save(path),"flow fixture");check(loadEditorImage(path),"flow acquisition");
    const auto source=selectedToolId_;const auto left=service_.addTool("edge"),right=service_.addTool("edge"),blob=service_.addTool("blob"),distance=service_.addTool("calculate");
    for(auto t:service_.tools())if(t.id==right){t.parameters["direction"]="right_to_left";service_.updateTool(t);}refreshWorkspace();
    auto bind=[&](const QString& target,const QString& input,const QString& upstream,const QString& output){selectTool(target);auto* combo=propertiesBody_->findChild<QComboBox*>("input_"+input);
        // Keep the legacy Edge byte/algorithm regression after the old picker
        // was replaced. The new common picker is covered by verifyPoseTracking.
        if(!combo && input=="reference"){
            const auto point=referencePoint(upstream);check(point.has_value(),"legacy current reference point");
            for(auto t:service_.tools())if(t.id==target){t.parameters["referenceOrigin"]=QJsonObject{{"x",point->x()},{"y",point->y()}};
                auto links=editorConnections(service_.activeRecipe()->definition);links.erase(std::remove_if(links.begin(),links.end(),[&](const auto& c){return c.targetTool==target && c.targetPort==input;}),links.end());links.append({upstream,output,target,input});service_.updateToolConnections(t,links);refreshWorkspace();return;}
        }
        check(combo!=nullptr,"input combo");const auto i=combo->findData(upstream+"/"+output);check(i>0,"compatible output");combo->setCurrentIndex(i);};
    bind(left,"image",source,"image");bind(right,"image",source,"image");bind(blob,"image",source,"image");bind(distance,"p1",left,"point");bind(distance,"p2",right,"point");
    check(validateGraph(service_.tools(),editorConnections(service_.activeRecipe()->definition)).valid(),"UI valid DAG");
    auto wait=[&]{QElapsedTimer timer;timer.start();while((editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Teaching)) && timer.elapsed()<10000){QApplication::processEvents();QThread::msleep(1);}check(!editorSession_.busy(SessionJob::Execution) && !editorSession_.busy(SessionJob::Teaching),"flow/teaching worker timeout");};
    findChild<QPushButton*>("runFlow")->click();check(editorSession_.busy(SessionJob::Execution),"flow starts async");close();check(isVisible(),"close while busy");wait();
    check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"UI cycle DONE");
    for(const auto& r:editorSession_.cycle->tools)if(r.toolId==distance)check(std::get<double>(r.outputs["value"].data)==399,"UI distance");
    check(service_.activeRecipe()->status==RecipeStatus::Draft,"execution preserves DRAFT");
    const auto bindings=editorConnections(service_.activeRecipe()->definition);service_.save();service_.openProject(service_.path());service_.selectRecipe(recipe);refreshWorkspace();check(editorConnections(service_.activeRecipe()->definition)==bindings,"UI reopen graph");selectTool(distance);
    runFlow();auto t=service_.tools()[1];t.parameters["minContrast"]=30;service_.updateTool(t);refreshWorkspace();wait();check(!editorSession_.cycle,"stale cycle rejected");
    runFlow();findChild<QPushButton*>("stopFlow")->click();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Skipped,"UI cancellation");
    runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"run after stop");
    service_.save();selectTool(distance);imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("step5-editor.png"));
    selectTool(left);QApplication::processEvents();grab().save(QDir(artifacts).filePath("fix-edge.png"));
    selectTool(blob);QApplication::processEvents();grab().save(QDir(artifacts).filePath("step5-blob.png"));
    const auto rawHash=editorSession_.frame->contentId();const auto frameId=editorSession_.frame->metadata().frameId;
    const auto transform=imageViewer_->transform();const auto scrollX=imageViewer_->horizontalScrollBar()->value(),scrollY=imageViewer_->verticalScrollBar()->value();
    auto* threshold=propertiesBody_->findChild<QSpinBox*>("threshold");check(threshold!=nullptr,"Blob threshold control");threshold->setValue(180);
    findChild<QPushButton*>("runTool")->click();wait();
    check(editorSession_.cycle && editorSession_.cycle->partial && editorSession_.cycle->executedTools==QStringList{blob} && editorSession_.cycle->acquisitions==0,"UI Run Tool invokes only Blob");
    check(editorSession_.frame->contentId()==rawHash && editorSession_.frame->metadata().frameId==frameId && imageViewer_->transform()==transform && imageViewer_->horizontalScrollBar()->value()==scrollX && imageViewer_->verticalScrollBar()->value()==scrollY,"Run Tool preserves raw frame and zoom/pan");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("p4-blob-target.png"));resize(850,600);QApplication::processEvents();grab().save(QDir(artifacts).filePath("p4-blob-target-small.png"));resize(1400,900);
    check(propertiesBody_->findChild<QSpinBox*>("minArea") && propertiesBody_->findChild<QSpinBox*>("maxArea") && propertiesBody_->findChild<QComboBox*>("blobPolarity") && propertiesBody_->findChild<QComboBox*>("blobConnectivity"),"Blob filter controls");
    propertiesBody_->findChild<QSpinBox*>("minArea")->setValue(10);check(applyToolProperties(),"Blob min area update");
    check(!editorSession_.cycle || std::none_of(editorSession_.cycle->tools.begin(),editorSession_.cycle->tools.end(),[&](const auto& r){return r.toolId==blob && !r.mask.pixels.isEmpty();}),"parameter invalidates stale mask");
    findChild<QPushButton*>("runTool")->click();wait();
    bool mask=false;for(const auto& r:editorSession_.cycle->tools)if(r.toolId==blob)mask=!r.mask.pixels.isEmpty();check(mask && editorSession_.frame->contentId()==rawHash,"Run Tool restores mask on same raw frame");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("p5-blob-mask.png"));resize(850,600);QApplication::processEvents();grab().save(QDir(artifacts).filePath("p5-blob-mask-small.png"));resize(1400,900);
    for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="edge_point")toolbox_->setCurrentRow(i);
    findChild<QPushButton*>("addTool")->click();
    const auto temporary=selectedToolId_;findChild<QPushButton*>("removeTool")->click();
    check(selectedToolId_.isEmpty(),"delete clears selected tool");
    for(const auto& tool:service_.tools())check(tool.id!=temporary,"UI temporary tool removed");
    selectTool(distance);runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"run after UI add/delete");service_.save();
    findChild<QTabWidget*>("editorTabs")->setCurrentIndex(0);QApplication::processEvents();grab().save(QDir(artifacts).filePath("step5-flow.png"));
    service_.createRecipe("Blob mask acceptance","",ApplicationType::DefectInspection,"file");refreshWorkspace();
    const auto maskPath=QDir(artifacts).filePath("p5-ring-input.png");QImage ring(600,400,QImage::Format_Grayscale8);ring.fill(25);
    {QPainter p(&ring);p.fillRect(QRect(160,100,240,200),Qt::white);p.fillRect(QRect(220,150,120,100),QColor(25,25,25));p.fillRect(QRect(100,70,5,5),Qt::white);}
    check(ring.save(maskPath) && loadEditorImage(maskPath),"hollow Blob image");const auto maskSource=selectedToolId_;const auto maskTool=service_.addTool("blob");service_.setConnections({{maskSource,"image",maskTool,"image"}});
    for(auto t:service_.tools())if(t.id==maskTool){t.parameters["minArea"]=100;t.parameters["maxArea"]=50000;t.parameters["searchRoi"]=MatchService::rectangle(QRect(70,50,400,300));service_.updateTool(t);}
    refreshWorkspace();selectTool(maskTool);runFlow();wait();bool hollow=false;for(const auto& r:editorSession_.cycle->tools)if(r.toolId==maskTool)hollow=std::get<double>(r.outputs["area"].data)==36000 && std::get<double>(r.outputs["count"].data)==1 && !r.mask.pixels[(200-50)*400+(280-70)];check(hollow,"hollow Blob pixel area and filtered mask");
    imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("p5-blob-ring.png"));resize(850,600);QApplication::processEvents();grab().save(QDir(artifacts).filePath("p5-blob-ring-small.png"));findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(propertiesBody_->findChild<QSpinBox*>("maxArea"));QApplication::processEvents();grab().save(QDir(artifacts).filePath("p5-blob-properties-small.png"));resize(1400,900);QApplication::processEvents();grab().save(QDir(artifacts).filePath("p5-blob-properties.png"));service_.save();
    const auto trackingRecipe=service_.createRecipe("Reference tracking","",ApplicationType::Measurement,"file");refreshWorkspace();
    const auto trackingPath=QDir(artifacts).filePath("reference-input.png");
    auto trackingImage=[&](int dx,int dy){QImage image(1000,600,QImage::Format_Grayscale8);image.fill(0);QPainter painter(&image);painter.fillRect(QRect(200+dx,100+dy,400,300),Qt::white);painter.end();check(image.save(trackingPath),"tracking UI fixture");};
    trackingImage(0,0);check(loadEditorImage(trackingPath),"tracking UI source");const auto trackingSource=selectedToolId_;
    const auto trackingMatch=service_.addTool("template_match"),trackingEdge=service_.addTool("edge");
    for(auto tool:service_.tools())if(tool.id==trackingMatch){tool.parameters["modelRoi"]=MatchService::rectangle(QRect(190,90,32,32));tool.parameters["modelPng"]=MatchService{}.teach(*editorSession_.frame,QRect(190,90,32,32));tool.parameters["minScore"]=0.95;service_.updateTool(tool);}
    refreshWorkspace();bind(trackingMatch,"image",trackingSource,"image");bind(trackingEdge,"image",trackingSource,"image");runFlow();wait();
    bind(trackingEdge,"reference",trackingMatch,"point");QApplication::processEvents();
    beginRoi(false);commitRoi(QRect(190,140,30,100));imageViewer_->drawRegion(false);
    runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"reference UI original cycle");
    trackingImage(30,20);runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"reference UI moved cycle");
    for(const auto& result:editorSession_.cycle->tools)if(result.toolId==trackingEdge){check(result.searchRegion==QRect(220,160,30,100),"reference UI translated search box");check(std::get<QPointF>(result.outputs["point"].data)==QPointF(230,209.5),"reference UI moved edge point");}
    // Re-teaching on the moved frame must anchor to its current match, even after
    // another property edit invalidates the displayed cycle results.
    auto tool=service_.tools().last();tool.parameters["minContrast"]=25;service_.updateTool(tool);refreshWorkspace();selectTool(trackingEdge);
    beginRoi(false);commitRoi(QRect(220,160,30,100));imageViewer_->drawRegion(false);runFlow();wait();
    for(const auto& result:editorSession_.cycle->tools)if(result.toolId==trackingEdge)check(result.searchRegion==QRect(220,160,30,100),"moved frame re-teaching preserves anchor");
    imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("reference-edge.png"));
    selectTool(trackingMatch);beginRoi(true);commitRoi(QRect(220,110,32,32));imageViewer_->drawRegion(false);wait();
    runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Error,"model re-teaching blocks old edge anchor");
    for(const auto& result:editorSession_.cycle->tools)if(result.toolId==trackingEdge)check(result.state==ExecutionState::Error && result.outputs.isEmpty() && result.searchRegion.isEmpty(),"stale UI edge output suppressed");
    service_.save();service_.openProject(service_.path());service_.selectRecipe(trackingRecipe);refreshWorkspace();runFlow();wait();
    check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Error,"UI reference invalidation survives reopen");selectTool(trackingEdge);
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("a2-reference-invalid.png"));
    beginRoi(false);commitRoi(QRect(220,160,30,100));imageViewer_->drawRegion(false);runFlow();wait();
    check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"UI search ROI re-teaching restores execution");
    for(const auto& result:editorSession_.cycle->tools)if(result.toolId==trackingEdge)check(result.searchRegion==QRect(220,160,30,100),"UI re-taught anchor keeps requested box");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("a2-reference-restored.png"));service_.save();
    const auto definitionBeforeGeometry=service_.activeRecipe()->definition;
    findChild<QAction*>("roiGeometry")->trigger();QApplication::processEvents();
    auto* geometryDialog=findChild<QDialog*>("roiGeometryDialog");check(geometryDialog,"ROI geometry entry opens numeric editor");
    geometryDialog->findChild<QComboBox*>("roiShape")->setCurrentIndex(3);geometryDialog->findChild<QPushButton*>("roiApply")->click();
    if(!imageViewer_->roi() || !std::holds_alternative<ArcRoi>(imageViewer_->roi()->geometry))throw std::runtime_error(("numeric arc: "+geometryDialog->findChild<QLabel*>("roiStatus")->text()).toStdString());
    check(service_.activeRecipe()->definition==definitionBeforeGeometry && !service_.dirty(),"temporary geometry never edits recipe or grants result");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("t2-editor-roi.png"));geometryDialog->grab().save(QDir(artifacts).filePath("t2-editor-roi-dialog.png"));
    resize(850,600);QApplication::processEvents();grab().save(QDir(artifacts).filePath("t2-editor-roi-small.png"));resize(1400,900);
    geometryDialog->close();imageViewer_->setRoi({});QApplication::processEvents();return true;
}
bool MainWindow::verifyMatching(const QString& artifacts) {
    auto failure=[this](int line){throw std::runtime_error((QString("Matching smoke line %1: ").arg(line)+matchInfo_->text()).toStdString());return false;};
    if (!editorSession_.frame || service_.tools().size()<2) return failure(__LINE__);
    const auto id=service_.tools()[1].id; selectTool(id); selectPage("editor");
    auto wait=[this]{QElapsedTimer timeout; timeout.start(); while((editorSession_.busy(SessionJob::Teaching) || editorSession_.busy(SessionJob::Execution)) && timeout.elapsed()<10000) {QApplication::processEvents(); QThread::msleep(1);} return !editorSession_.busy(SessionJob::Teaching) && !editorSession_.busy(SessionJob::Execution);};
    // Exercise the actual viewport ROI gesture at 1:1 scale.
    imageViewer_->actualSize(); imageViewer_->centerOn(710,670); QApplication::processEvents();
    beginRoi(true);
    const auto a=imageViewer_->mapFromScene(QPointF(700,650)); const auto b=imageViewer_->mapFromScene(QPointF(763,713));
    auto mouse=[this](QEvent::Type type,QPoint p,Qt::MouseButton button,Qt::MouseButtons buttons){
        QMouseEvent event(type,QPointF(p),QPointF(imageViewer_->viewport()->mapToGlobal(p)),button,buttons,Qt::NoModifier); QApplication::sendEvent(imageViewer_->viewport(),&event);
    };
    mouse(QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton); mouse(QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton); mouse(QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);
    if (!wait() || service_.tools()[1].parameters["modelPng"].toString().isEmpty()) return failure(__LINE__);
    beginRoi(false); commitRoi(QRect(650,600,512,512)); imageViewer_->drawRegion(false);
    // Remove the creation default explicitly: a preview alone is not a DAG input.
    auto* initialImageInput=propertiesBody_->findChild<QComboBox*>("input_image");if(!initialImageInput || initialImageInput->currentIndex()<1)return failure(__LINE__);initialImageInput->setCurrentIndex(0);
    runMatch();if(!wait() || editorSession_.matchOutcome || !editorSession_.cycle || editorSession_.cycle->state!=ExecutionState::Error)return failure(__LINE__);
    auto* imageInput=propertiesBody_->findChild<QComboBox*>("input_image");if(!imageInput)return failure(__LINE__);
    const auto sourceId=service_.tools()[0].id;const auto inputIndex=imageInput->findData(sourceId+"/image");if(inputIndex<1)return failure(__LINE__);
    imageInput->setCurrentIndex(inputIndex);
    runMatch(); close(); if (!isVisible()) return failure(__LINE__);
    if (!wait() || !editorSession_.matchOutcome || !editorSession_.matchOutcome->accepted || editorSession_.matchOutcome->rectangle.topLeft()!=QPoint(700,650)) return failure(__LINE__);
    if(!editorSession_.references.contains(id) || editorSession_.references[id].second!=QPointF(731.5,681.5))return failure(__LINE__);
    if(!editorSession_.cycle || editorSession_.cycle->tools.size()!=2 || editorSession_.cycle->state!=ExecutionState::Done)return failure(__LINE__);
    // Run Tool consumes the same raw frame. Source asset changes require explicit acquisition.
    const auto fixturePath=editorSession_.frame->path();const auto rawHash=editorSession_.frame->contentId();QByteArray fixtureBytes;
    {QFile file(fixturePath);if(!file.open(QIODevice::ReadOnly))return failure(__LINE__);fixtureBytes=file.readAll();}
    QImage blank(editorSession_.frame->size(),QImage::Format_Grayscale8);blank.fill(0);if(!blank.save(fixturePath))return failure(__LINE__);
    runMatch();if(!wait() || editorSession_.matchOutcome || !editorSession_.cycle || !editorSession_.cycle->executedTools.isEmpty() || editorSession_.frame->contentId()!=rawHash)return failure(__LINE__);
    {QFile file(fixturePath);if(!file.open(QIODevice::WriteOnly) || file.write(fixtureBytes)!=fixtureBytes.size())return failure(__LINE__);}
    runExecution(sourceId);if(!wait())return failure(__LINE__);
    runMatch();if(!wait() || !editorSession_.matchOutcome || !editorSession_.matchOutcome->accepted || editorSession_.cycle->acquisitions!=0 || editorSession_.cycle->executedTools!=QStringList{id})return failure(__LINE__);
    runMatch();findChild<QPushButton*>("stopFlow")->click();
    if(!wait() || editorSession_.matchOutcome || !editorSession_.cycle || editorSession_.cycle->state!=ExecutionState::Skipped)return failure(__LINE__);
    // A result from a previous configuration must never be published.
    runMatch(); auto tool=service_.tools()[1]; tool.parameters["minScore"]=0.99; service_.updateTool(tool); refreshWorkspace();
    if (!wait() || editorSession_.matchOutcome) return failure(__LINE__);
    service_.save(); const auto recipe=service_.activeRecipe()->id;
    service_.openProject(service_.path()); service_.selectRecipe(recipe); refreshWorkspace(); selectTool(id);
    runMatch(); if (!wait() || !editorSession_.matchOutcome || !editorSession_.matchOutcome->accepted) return failure(__LINE__);
    imageViewer_->fitImage(); QApplication::processEvents(); grab().save(QDir(artifacts).filePath("step4-editor.png"));
    auto source=service_.tools()[0]; source.enabled=false; service_.updateTool(source); refreshWorkspace();
    if (editorSession_.matchOutcome || editorSession_.frame) return failure(__LINE__);
    source.enabled=true; service_.updateTool(source); service_.save(); refreshWorkspace();
    return service_.activeRecipe()->status==RecipeStatus::Draft && !service_.dirty();
}
}

namespace visionflow {
bool MainWindow::verifyCodeReading(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    QListWidgetItem* entry=nullptr;for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="code_read")entry=toolbox_->item(i);
    check(entry,"code Toolbox entry");
    if(!BackendCapabilities::get("zxing-reader").available){check(!(entry->flags() & Qt::ItemIsEnabled),"OFF backend disables code entry");return true;}
    auto wait=[&]{QElapsedTimer timer;timer.start();while((editorSession_.busy() || runtimeJobs_.busy()) && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}check(!editorSession_.busy() && !runtimeJobs_.busy(),"code UI worker deadline");};
    const auto recipe=service_.createRecipe("Kod Oku UI","",ApplicationType::DefectInspection,"file");refreshWorkspace();selectPage("editor");
    const auto path=QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../deps/code-fixtures/qr.pgm");check(loadEditorImage(path),"code independent fixture");const auto source=selectedToolId_;
    toolbox_->setCurrentItem(entry);findChild<QPushButton*>("addTool")->click();const auto reader=selectedToolId_;check(reader!=source,"code Toolbox actual tool");
    service_.setConnections({{source,"image",reader,"image"}});refreshWorkspace();selectTool(reader);
    auto settings=[&]{return dynamic_cast<CodeReadEditor*>(propertiesBody_->findChild<QWidget*>("codeReadSettings"));};check(settings(),"code editor exists");
    auto* formats=propertiesBody_->findChild<QListWidget*>("codeFormats");check(formats && formats->count()==8,"eight format choices");for(int i=1;i<formats->count();++i)formats->item(i)->setCheckState(Qt::Unchecked);
    propertiesBody_->findChild<QComboBox*>("codeRule")->setCurrentIndex(1);propertiesBody_->findChild<QLineEdit*>("codeExpected")->setText("VISIONFLOW-T0.2");check(applyToolProperties(),"code content settings");
    beginRoi(false);commitRoi(QRect(QPoint(0,0),editorSession_.frame->size()));check(!settings()->parameters()["searchRoi"].toObject().isEmpty(),"code rectangle saved");
    runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done && matchInfo_->text().contains("#0 QR | \"VISIONFLOW-T0.2\""),"code readable result list");
    const int overlays=imageViewer_->scene()->items().size();imageViewer_->setToolOverlay({},{});check(overlays-imageViewer_->scene()->items().size()==5,"four actual polygon edges and label");showExecutionResult();check(!matchInfo_->text().contains("PASS") && !matchInfo_->text().contains("score"),"no code confidence or part decision");
    const auto raw=editorSession_.frame->contentId();imageViewer_->zoomBy(1.1);const auto zoom=imageViewer_->transform();runExecution(reader);wait();check(editorSession_.cycle->partial && editorSession_.cycle->executedTools==QStringList{reader} && editorSession_.cycle->acquisitions==0 && editorSession_.frame->contentId()==raw && imageViewer_->transform()==zoom,"code TargetOnly raw zoom acquisition");
    resize(1400,900);horizontal_->setSizes({160,700,360});vertical_->setSizes({570,220});QApplication::processEvents();imageViewer_->fitImage();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(propertiesBody_->findChild<QComboBox*>("codeRule"));QApplication::processEvents();grab().save(QDir(artifacts).filePath("code-editor.png"));resize(850,600);vertical_->setSizes({360,125});QApplication::processEvents();imageViewer_->fitImage();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(propertiesBody_->findChild<QComboBox*>("codeRule"));findChild<QScrollArea*>("propertiesScroll")->horizontalScrollBar()->setValue(0);QApplication::processEvents();grab().save(QDir(artifacts).filePath("code-editor-small.png"));resize(1400,900);
    propertiesBody_->findChild<QPushButton*>("clearSearch")->click();check(settings()->parameters()["searchRoi"].toObject().isEmpty(),"clear code ROI");const auto expected=settings()->parameters();service_.save();const auto revision=service_.activeRecipe()->recipeVersion;service_.openProject(service_.path());service_.selectRecipe(recipe);refreshWorkspace();wait();selectPage("editor");selectTool(reader);check(settings() && settings()->parameters()==expected,"code settings save reopen");service_.save();check(service_.activeRecipe()->recipeVersion==revision,"code no-op revision");
    const QStringList files{"qr","datamatrix","code128","code39","ean8","ean13","upca","upce"};
    for(const auto& file:files){check(loadEditorImage(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../deps/code-fixtures/"+file+".pgm")),"format fixture load");selectTool(reader);auto* list=propertiesBody_->findChild<QListWidget*>("codeFormats");for(int i=0;i<list->count();++i)list->item(i)->setCheckState(Qt::Checked);propertiesBody_->findChild<QComboBox*>("codeRule")->setCurrentIndex(0);check(applyToolProperties(),"all formats no text rule");runFlow();wait();bool found=false;for(const auto& r:editorSession_.cycle->tools)if(r.toolId==reader)found=r.state==ExecutionState::Done && std::get<CodeResults>(r.outputs.value("codes").data).items.size()==1;check(found,"UI actual independent format read");}
    check(loadEditorImage(path),"restore QR");selectTool(reader);runFlow();wait();runExecution(reader);editorSession_.stop(SessionJob::Execution);wait();for(const auto& r:editorSession_.cycle->tools)if(r.toolId==reader)check(r.outputs.isEmpty(),"code Stop clears outputs");check(!matchInfo_->text().contains("#0 QR"),"Stop clears code list");
    service_.save();refreshWorkspace();selectPage("recipes");recipeSearch_->clear();typeFilter_->setCurrentIndex(0);refreshRecipes();for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==recipe)recipeTable_->selectRow(row);findChild<QPushButton*>("loadRecipe")->click();findChild<QPushButton*>("runtime_run")->click();check(!runtimeResult_,"code Run no invented result");for(auto t:service_.tools())if(t.id==reader){t.parameters["rule"]="exact";t.parameters["expected"]="draft mismatch";service_.updateTool(t);}
    findChild<QPushButton*>("runtime_cycle")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && runtimeSummary_->text().contains("VISIONFLOW-T0.2"),"Runtime code list");bool accepted=false;for(const auto& r:runtimeResult_->tools)if(r.toolId==reader)accepted=std::get<bool>(r.outputs.value("accepted").data);check(accepted,"immutable code Runtime ignores draft");runtimeTools_->setCurrentIndex(runtimeTools_->findData(reader));runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("code-runtime.png"));
    findChild<QPushButton*>("runtime_stop")->click();check(!runtimeResult_ && !runtimeViewer_->hasImage(),"code Runtime Stop");selectPage("logs");check(logTable_->rowCount()>0,"code Logs rows");logTable_->selectRow(0);QApplication::processEvents();grab().save(QDir(artifacts).filePath("code-logs.png"));const auto records=ProductionStore(service_.path()+".production").recent("cycles");check(ProductionResultCodec::summary(records.first()).contains("VISIONFLOW-T0.2") && ProductionResultCodec::csv(records).contains("VISIONFLOW-T0.2"),"code restart evidence CSV");selectPage("editor");selectTool(reader);service_.save();return true;
}
bool MainWindow::verifyPreprocessing(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto wait=[&]{QElapsedTimer timer;timer.start();while(editorSession_.busy() && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}check(!editorSession_.busy(),"preprocess UI worker timeout");};
    const auto recipe=service_.createRecipe("Preprocess acceptance","",ApplicationType::DefectInspection,"file");refreshWorkspace();selectPage("editor");
    QImage fixture(500,320,QImage::Format_RGB888);fixture.fill(QColor(20,100,200));{QPainter painter(&fixture);painter.fillRect(QRect(100,70,220,150),QColor(220,70,20));}
    const auto path=QDir(artifacts).filePath("preprocess-input.png");check(fixture.save(path),"preprocess UI fixture");check(loadEditorImage(path),"preprocess raw acquisition");const auto source=selectedToolId_,pre=service_.addTool("preprocess"),blob=service_.addTool("blob");service_.setConnections({{source,"image",pre,"image"},{pre,"image",blob,"image"}});refreshWorkspace();selectTool(pre);
    auto* channel=propertiesBody_->findChild<QComboBox*>("preprocessChannel");check(channel,"preprocess channel selector");channel->setCurrentIndex(channel->findData("red"));
    propertiesBody_->findChild<QPushButton*>("preprocessAdd")->click();auto* op=propertiesBody_->findChild<QComboBox*>("preprocessOperation");op->setCurrentIndex(op->findData("threshold"));
    auto* settings=dynamic_cast<PreprocessEditor*>(propertiesBody_->findChild<QWidget*>("preprocessSettings"));check(settings && settings->parameters()["steps"].toArray().size()==1,"preprocess step edit");
    runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done && editorSession_.frame && editorSession_.frame->processing().isEmpty(),"preprocess UI real flow raw frame");
    bool output=false;for(const auto& r:editorSession_.cycle->tools)if(r.toolId==pre){const auto image=std::get<ImageFrame>(r.outputs["image"].data);output=image.processing().size()==1 && image.contentId()!=editorSession_.frame->contentId();}check(output && imageInfo_->text().contains("İşlenmiş Gray8"),"actual processed output displayed");
    imageViewer_->zoomBy(1.2);const auto transform=imageViewer_->transform();const auto hash=editorSession_.frame->contentId();runExecution(pre);wait();check(editorSession_.cycle->partial && editorSession_.cycle->executedTools==QStringList{pre} && editorSession_.frame->contentId()==hash && imageViewer_->transform()==transform,"preprocess Run Tool preserves raw and zoom");
    resize(1400,900);horizontal_->setSizes({160,700,360});vertical_->setSizes({610,180});QApplication::processEvents();
    findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(settings);findChild<QScrollArea*>("propertiesScroll")->horizontalScrollBar()->setValue(0);QApplication::processEvents();grab().save(QDir(artifacts).filePath("preprocess-editor.png"));
    resize(850,600);QApplication::processEvents();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(settings);findChild<QScrollArea*>("propertiesScroll")->horizontalScrollBar()->setValue(0);QApplication::processEvents();grab().save(QDir(artifacts).filePath("preprocess-editor-small.png"));resize(1400,900);
    propertiesBody_->findChild<QPushButton*>("preprocessAdd")->click();propertiesBody_->findChild<QPushButton*>("preprocessUp")->click();settings=dynamic_cast<PreprocessEditor*>(propertiesBody_->findChild<QWidget*>("preprocessSettings"));check(settings->parameters()["steps"].toArray().first().toObject()["op"]=="gaussian","preprocess step order");propertiesBody_->findChild<QPushButton*>("preprocessDelete")->click();
    const auto expected=settings->parameters();service_.save();service_.openProject(service_.path());service_.selectRecipe(recipe);refreshWorkspace();selectPage("editor");selectTool(pre);settings=dynamic_cast<PreprocessEditor*>(propertiesBody_->findChild<QWidget*>("preprocessSettings"));check(settings && settings->parameters()==expected && !service_.dirty(),"preprocess UI save reopen");runFlow();wait();check(editorSession_.cycle->state==ExecutionState::Done,"preprocess UI saved flow");
    runExecution(pre);editorSession_.stop(SessionJob::Execution);wait();for(const auto& r:editorSession_.cycle->tools)if(r.toolId==pre)check(r.outputs.isEmpty(),"preprocess Stop clears result");service_.save();return true;
}
}

namespace visionflow {
bool MainWindow::verifyEdgePoint(const QString& artifacts) {
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    auto wait=[&]{QApplication::processEvents();QElapsedTimer timer;timer.start();while((editorSession_.busy() || runtimeJobs_.busy()) && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();check(!editorSession_.busy() && !runtimeJobs_.busy(),"edge UI worker deadline");};
    const auto recipe=service_.createRecipe("Yönlü kenar mm","",ApplicationType::Measurement,"file");refreshWorkspace();selectPage("editor");
    QImage fixture(1000,600,QImage::Format_Grayscale8);fixture.fill(20);{QPainter p(&fixture);p.fillRect(200,100,400,300,QColor(220,220,220));}const auto path=QDir(artifacts).filePath("edge-stripe.png");check(fixture.save(path) && loadEditorImage(path),"edge UI independent 400px stripe");const auto source=selectedToolId_;
    QListWidgetItem* entry=nullptr;bool industrialSource=false;
    for(int i=0;i<toolbox_->count();++i) {
        const auto kind=toolbox_->item(i)->data(Qt::UserRole).toString();
        if(kind=="edge_point")entry=toolbox_->item(i);
        if(kind=="industrial_camera")industrialSource=true;
    }
    int catalogCount=0;for(int i=0;i<toolbox_->count();++i)if(!toolbox_->item(i)->data(Qt::UserRole).toString().isEmpty())++catalogCount;
    check(entry && industrialSource && catalogCount==ToolCatalog::entries().size(),"all new catalog kinds present independently of group headers");toolbox_->setCurrentItem(entry);findChild<QPushButton*>("addTool")->click();const auto left=selectedToolId_;findChild<QPushButton*>("addTool")->click();const auto right=selectedToolId_,measurement=service_.addTool("measurement");refreshWorkspace();
    auto bind=[&](QString target,QString input,QString upstream,QString output){selectTool(target);auto* c=propertiesBody_->findChild<QComboBox*>("input_"+input);check(c && c->findData(upstream+"/"+output)>0,"edge UI compatible binding");c->setCurrentIndex(c->findData(upstream+"/"+output));};
    bind(left,"image",source,"image");bind(right,"image",source,"image");bind(measurement,"p1",left,"point");bind(measurement,"p2",right,"point");
    selectTool(left);imageViewer_->fitImage();findChild<QPushButton*>("edgeDraw")->click();
    auto drag=[&](QPointF a,QPointF b){const auto start=imageViewer_->mapFromScene(a),end=imageViewer_->mapFromScene(b);QMouseEvent press(QEvent::MouseButtonPress,QPointF(start),QPointF(start),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(imageViewer_->viewport(),&press);QMouseEvent move(QEvent::MouseMove,QPointF(end),QPointF(end),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(imageViewer_->viewport(),&move);QMouseEvent release(QEvent::MouseButtonRelease,QPointF(end),QPointF(end),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(imageViewer_->viewport(),&release);QApplication::processEvents();};
    drag({60,250},{900,250});wait();bool taught=false;for(const auto& t:service_.tools())if(t.id==left)taught=!t.parameters["scanRoi"].toObject().isEmpty();check(taught,"drawn arrow committed to recipe");
    auto numeric=[&](const QString& id){selectTool(id);for(const auto& v:QVector<QPair<QString,double>>{{"ax",60.25},{"ay",250.25},{"bx",900.25},{"by",250.25},{"width",4}})propertiesBody_->findChild<QDoubleSpinBox*>("scan_"+v.first)->setValue(v.second);propertiesBody_->findChild<QPushButton*>("edgeApplyScan")->click();wait();};
    numeric(left);
    const auto beforeHandle=imageViewer_->roi();check(beforeHandle.has_value(),"bound numeric scan exists");const auto beforeScan=std::get<ScanSegment>(beforeHandle->geometry);drag(beforeScan.start,beforeScan.start+QPointF(15,5));wait();check(imageViewer_->roi()!=beforeHandle,"bound handle edits scan");imageViewer_->undoRoi();wait();check(imageViewer_->roi()==beforeHandle,"bound geometry undo persists exact scan");
    findChild<QPushButton*>("edgeDraw")->click();imageViewer_->cancelRoi();check(imageViewer_->roi()==beforeHandle,"cancel drawing preserves taught arrow");
    findChild<QAction*>("roiGeometry")->trigger();QApplication::processEvents();auto* temporary=findChild<QDialog*>("roiGeometryDialog");check(temporary,"temporary geometry dialog");imageViewer_->setRoi(ToolRoi{{source+"/image"},ScanSegment{{100,240},{800,240},8}});refreshMatchOverlay();bool kept=false;for(const auto& t:service_.tools())if(t.id==left)kept=t.parameters["scanRoi"].toObject()==beforeHandle->toJson();check(kept,"temporary geometry never changes bound recipe scan");temporary->close();wait();check(imageViewer_->roi()==beforeHandle,"closing temporary workspace restores recipe arrow");
    numeric(right);selectTool(left);propertiesBody_->findChild<QComboBox*>("ep_polarity")->setCurrentIndex(0);selectTool(right);propertiesBody_->findChild<QComboBox*>("ep_polarity")->setCurrentIndex(1);selectTool(measurement);propertiesBody_->findChild<QDoubleSpinBox*>("lowerMm")->setValue(9.999);propertiesBody_->findChild<QDoubleSpinBox*>("upperMm")->setValue(10.001);check(applyToolProperties(),"edge measurement limits");
    auto result=[&](const QString& id)->ToolResult{check(editorSession_.cycle.has_value(),"edge UI cycle exists");for(const auto& r:editorSession_.cycle->tools)if(r.toolId==id)return r;throw std::runtime_error((QString("edge UI result missing: ")+id+"; selected="+selectedToolId_+"; "+matchInfo_->text()).toStdString());};
    selectTool(left);runFlow();wait();check(result(left).state==ExecutionState::Done && result(left).edgeDiagnostics && result(left).edgeDiagnostics->candidates.size()==2 && edgeProfile_->hasDiagnostics(),"real profile and accepted/rejected candidates");check(result(measurement).errorCode=="CALIBRATION_INVALID","unbound mm no invented result");
    const auto point=std::get<QPointF>(result(left).outputs["point"].data);check(std::abs(point.x()-199.5)<.02,"UI actual subpixel edge oracle");
    imageViewer_->zoomBy(1.2);const auto transform=imageViewer_->transform();const auto hash=editorSession_.frame->contentId();runExecution(left);wait();check(editorSession_.cycle->partial && editorSession_.cycle->executedTools==QStringList{left} && editorSession_.cycle->acquisitions==0 && editorSession_.frame->contentId()==hash && imageViewer_->transform()==transform,"edge TargetOnly preserves raw zoom and no acquisition");
    findChild<QPushButton*>("edgeReverse")->click();wait();propertiesBody_->findChild<QComboBox*>("ep_polarity")->setCurrentIndex(1);runExecution(left);wait();check(std::abs(std::get<QPointF>(result(left).outputs["point"].data).x()-point.x())<.02,"reverse arrow plus polarity same physical edge");findChild<QPushButton*>("edgeReverse")->click();wait();propertiesBody_->findChild<QComboBox*>("ep_polarity")->setCurrentIndex(0);
    // Calibration stays in the existing UI/application workflow.
    service_.setCalibrationSetup("edge-lens","edge-plane");runExecution(source);wait();
    const auto candidate=CalibrationWorkflow::manualScale(*service_.activeRecipe(),*editorSession_.frame,"ui-edge-scale",1,40);
    const auto coordinate=candidate.dependency().source;
    const auto accepted=MeasurementService{}.validate(candidate,{{"edge-holdout-x",{{20,20},coordinate},{{420,20},coordinate},10},{"edge-holdout-y",{{30,40},coordinate},{{30,240},coordinate},5}},.01);
    CalibrationService(service_.path()+".calibrations.json").save(accepted);
    CalibrationWorkflow::bind(service_,*editorSession_.frame,service_.path()+".calibrations.json","ui-edge-scale","manual_planar_scale");
    check(service_.activeRecipe()->calibrationBinding.has_value(),"edge calibration service bound");
    selectPage("editor");selectTool(left);runFlow();wait();check(result(measurement).decision==PartDecision::Pass && std::abs(std::get<double>(result(measurement).outputs["value"].data)-10)<.001,"Edge Point to actual 10mm PASS");
    service_.save();const auto revision=service_.activeRecipe()->recipeVersion;const auto definition=service_.activeRecipe()->definition;service_.openProject(service_.path());service_.selectRecipe(recipe);service_.save();check(service_.activeRecipe()->recipeVersion==revision && service_.activeRecipe()->definition==definition,"edge Save reopen no-op");refreshWorkspace();selectTool(left);runFlow();wait();
    resize(1400,900);horizontal_->setSizes({155,680,370});vertical_->setSizes({510,270});findChild<QTabWidget*>("editorTabs")->setCurrentIndex(2);QApplication::processEvents();imageViewer_->fitImage();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(propertiesBody_->findChild<QPushButton*>("edgeDraw"));QApplication::processEvents();grab().save(QDir(artifacts).filePath("edge-editor.png"));edgeProfile_->grab().save(QDir(artifacts).filePath("edge-profile.png"));resize(850,600);QApplication::processEvents();imageViewer_->fitImage();grab().save(QDir(artifacts).filePath("edge-editor-small.png"));resize(1400,900);
    selectTool(measurement);runFlow();wait();QApplication::processEvents();grab().save(QDir(artifacts).filePath("edge-measurement.png"));service_.save();
    selectPage("recipes");refreshRecipes();for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==recipe)recipeTable_->selectRow(row);findChild<QPushButton*>("loadRecipe")->click();findChild<QPushButton*>("runtime_run")->click();
    for(auto t:service_.tools())if(t.id==left){t.parameters["minContrast"]=5100;service_.updateTool(t);}findChild<QPushButton*>("runtime_cycle")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done,"edge Runtime real DONE");bool mm=false;for(const auto& r:runtimeResult_->tools)if(r.toolId==measurement)mm=r.decision==PartDecision::Pass && std::abs(std::get<double>(r.outputs["value"].data)-10)<.001;check(mm,"immutable Runtime edge draft threshold ignored mm PASS");runtimeTools_->setCurrentIndex(runtimeTools_->findData(left));check(runtimeEdgeProfile_->hasDiagnostics(),"Runtime live edge profile");runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("edge-runtime.png"));runtimeEdgeProfile_->grab().save(QDir(artifacts).filePath("edge-runtime-profile.png"));runtimeTools_->setCurrentIndex(runtimeTools_->findData(measurement));QApplication::processEvents();grab().save(QDir(artifacts).filePath("edge-runtime-measurement.png"));
    findChild<QPushButton*>("runtime_stop")->click();check(!runtimeResult_ && !runtimeEdgeProfile_->hasDiagnostics() && !runtimeViewer_->hasImage(),"Runtime Stop clears point profile and mm");const auto records=ProductionStore(service_.path()+".production").recent("cycles");check(ProductionResultCodec::summary(records.first()).contains("PASS") && ProductionResultCodec::csv(records).contains("profile-edge"),"edge terminal restart mm evidence CSV");
    selectPage("editor");selectTool(left);runFlow();wait();check(result(left).outputs["found"].valid && !std::get<bool>(result(left).outputs["found"].data) && !result(left).outputs["point"].valid && edgeProfile_->hasDiagnostics() && result(measurement).state==ExecutionState::Skipped && result(measurement).outputs.isEmpty(),"not found profile retained but mm skipped no stale value");
    propertiesBody_->findChild<QDoubleSpinBox*>("ep_minContrast")->setValue(20);check(applyToolProperties(),"restore contrast");check(!edgeProfile_->hasDiagnostics(),"parameter edit clears old profile");runFlow();findChild<QPushButton*>("stopFlow")->click();wait();check(!edgeProfile_->hasDiagnostics() && result(left).outputs.isEmpty() && !result(left).edgeDiagnostics && result(measurement).decision==PartDecision::NotEvaluated,"editor Stop clears diagnostic and decision");
    service_.save();return true;
}
}

namespace visionflow {
bool MainWindow::verifyMetrology(const QString& artifacts){
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    auto wait=[&]{QApplication::processEvents();QElapsedTimer timer;timer.start();while((editorSession_.busy() || runtimeJobs_.busy()) && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();check(!editorSession_.busy() && !runtimeJobs_.busy(),"metrology UI deadline");};
    QString source;for(const auto& t:service_.tools())if(t.kind=="image_file")source=t.id;check(!source.isEmpty() && service_.activeRecipe()->calibrationBinding.has_value(),"real previous UI calibration");
    selectPage("editor");selectTool(source);runExecution(source);wait();check(editorSession_.frame.has_value(),"metrology fresh frame after Stop");QString pair,line;for(const QString kind:{"edge_pair","line_fit"}){QListWidgetItem* item=nullptr;for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)==kind)item=toolbox_->item(i);check(item,"metrology Toolbox entry");toolbox_->setCurrentItem(item);findChild<QPushButton*>("addTool")->click();const auto id=selectedToolId_;if(kind=="edge_pair")pair=id;else line=id;check(propertiesBody_->findChild<QComboBox*>("mt_mode")->currentText()=="calibrated","new metrology defaults to mm");propertiesBody_->findChild<QComboBox*>("mt_mode")->setCurrentText("pixel");auto* input=propertiesBody_->findChild<QComboBox*>("input_image");input->setCurrentIndex(input->findData(source+"/image"));}
    auto setGeometry=[&](const QString& id,const QVector<QPair<QString,double>>& coordinates){selectTool(id);for(const auto& v:coordinates)propertiesBody_->findChild<QDoubleSpinBox*>("mt_roi_"+v.first)->setValue(v.second);propertiesBody_->findChild<QPushButton*>("metrologyApplyRoi")->click();wait();check(imageViewer_->roi().has_value(),"numeric geometry commits");};
    setGeometry(pair,{{"ax",60.25},{"ay",250.25},{"bx",900.25},{"by",250.25},{"width",4}});
    const auto scan=imageViewer_->roi();imageViewer_->undoRoi();wait();check(!imageViewer_->roi(),"pair undo recipe geometry");setGeometry(pair,{{"ax",60.25},{"ay",250.25},{"bx",900.25},{"by",250.25},{"width",4}});check(imageViewer_->roi()==scan,"pair exact geometry restored");
    setGeometry(line,{{"cx",200},{"cy",250},{"width",240},{"height",80},{"angle",90}});propertiesBody_->findChild<QComboBox*>("mt_polarity")->setCurrentIndex(propertiesBody_->findChild<QComboBox*>("mt_polarity")->findData("falling"));
    auto result=[&](const QString& id)->ToolResult{check(editorSession_.cycle.has_value(),"metrology cycle");for(const auto& r:editorSession_.cycle->tools)if(r.toolId==id)return r;throw std::runtime_error("metrology result absent");};
    runFlow();wait();check(result(pair).state==ExecutionState::Done && result(line).state==ExecutionState::Done,"new UI pixel tools");check(!result(pair).outputs["widthMm"].valid && result(line).outputs["linePx"].diagnostic,"pixel mode explicitly diagnostic");
    for(auto id:{pair,line}){selectTool(id);propertiesBody_->findChild<QComboBox*>("mt_mode")->setCurrentText("calibrated");propertiesBody_->findChild<QDoubleSpinBox*>("mt_lower")->setValue(id==pair ? 9.999:5.9);propertiesBody_->findChild<QDoubleSpinBox*>("mt_upper")->setValue(id==pair ? 10.001:6.1);check(applyToolProperties(),"calibrated Properties");}
    selectTool(pair);runFlow();wait();check(std::abs(std::get<double>(result(pair).outputs["widthMm"].data)-10)<.001 && std::get<bool>(result(pair).outputs["accepted"].data),"real UI caliper 10 mm accepted");check(std::abs(std::get<double>(result(line).outputs["spanMm"].data)-5.95)<.001,"real line world support");
    auto hash=editorSession_.frame->contentId();imageViewer_->zoomBy(1.2);const auto zoom=imageViewer_->transform();runExecution(pair);wait();check(editorSession_.cycle->partial && editorSession_.cycle->executedTools==QStringList{pair} && editorSession_.cycle->acquisitions==0 && editorSession_.frame->contentId()==hash && imageViewer_->transform()==zoom,"metrology TargetOnly");
    service_.save();const auto recipe=service_.activeRecipe()->id;const auto revision=service_.activeRecipe()->recipeVersion;service_.openProject(service_.path());service_.selectRecipe(recipe);service_.save();check(service_.activeRecipe()->recipeVersion==revision,"metrology Save reopen no-op");refreshWorkspace();selectTool(pair);runFlow();wait();
    resize(1400,900);horizontal_->setSizes({155,750,300});vertical_->setSizes({540,240});findChild<QTabWidget*>("editorTabs")->setCurrentIndex(1);imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("metrology-caliper.png"));resize(850,600);QApplication::processEvents();imageViewer_->fitImage();grab().save(QDir(artifacts).filePath("metrology-small.png"));resize(1400,900);
    // Independently displace three local edge strips; the line support remains
    // at x=199.5 and the red outlier markers must remain visible.
    const auto imagePath=QDir(artifacts).filePath("edge-stripe.png");QImage corrupt(imagePath);for(int y0:{163,211,282})for(int y=y0;y<y0+8;++y)for(int x=200;x<212;++x)corrupt.scanLine(y)[x]=20;check(corrupt.save(imagePath),"independent local defects");selectTool(line);runFlow();wait();check(result(line).state==ExecutionState::Done && result(line).rejectedPoints.size()==3 && std::get<double>(result(line).outputs["usedCount"].data)==18,"line UI actual three outliers");imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("metrology-line-outliers.png"));service_.save();
    selectPage("recipes");refreshRecipes();for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==recipe)recipeTable_->selectRow(row);findChild<QPushButton*>("loadRecipe")->click();wait();findChild<QPushButton*>("runtime_run")->click();wait();check(runtime_ && runtime_->active(),"metrology Runtime prepared");for(auto t:service_.tools())if(t.id==pair || t.id==line){t.parameters["minContrast"]=5100;service_.updateTool(t);}findChild<QPushButton*>("runtime_cycle")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done,"metrology immutable Runtime real cycle");runtimeTools_->setCurrentIndex(runtimeTools_->findData(line));check(!CameraFlowSmokeAudit::runtimeScan(*runtimeViewer_,line).isEmpty(),"Runtime readonly line scan layer");runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("metrology-runtime-line.png"));runtimeTools_->setCurrentIndex(runtimeTools_->findData(pair));QApplication::processEvents();grab().save(QDir(artifacts).filePath("metrology-runtime-pair.png"));
    findChild<QPushButton*>("runtime_stop")->click();check(!runtimeResult_ && !runtimeViewer_->hasImage(),"metrology Stop removes results");const auto records=ProductionStore(service_.path()+".production").recent("cycles");const auto csv=ProductionResultCodec::csv(records);check(csv.contains("robust-line") && csv.contains("profile-pair") && csv.contains("outlier"),"metrology persistent typed scan evidence CSV");
    refreshWorkspace();selectPage("editor");selectTool(pair);runFlow();wait();check(!result(pair).outputs["widthMm"].valid && !result(pair).outputs["accepted"].valid && result(line).outputs.isEmpty(),"no found/line error clears old metric");for(auto t:service_.tools())if(t.id==pair || t.id==line){t.parameters["minContrast"]=20;service_.updateTool(t);}refreshWorkspace();runFlow();findChild<QPushButton*>("stopFlow")->click();wait();for(auto id:{pair,line})check(result(id).outputs.isEmpty() && result(id).rejectedPoints.isEmpty(),"metrology late Stop clears overlays");service_.save();return true;
}
}

namespace visionflow {
bool MainWindow::verifyCircle(const QString& artifacts){
    {
        auto p=defaultParameters("circle_fit");p["mode"]="calibrated";p["inlierDistance"]=.123;p["maxRms"]=.047;p["maxCondition"]=123.123456789123;
        MetrologyEditor editor("circle_fit",p,"raw",true);
        editor.findChild<QComboBox*>("mt_mode")->setCurrentText("pixel");
        auto changed=editor.parameters();
        if(changed["maxCondition"]!=p["maxCondition"])throw std::runtime_error("Circle unrelated high precision setting preserved");
        if(changed["inlierDistance"]!=.123 || changed["maxRms"]!=.047 || changed["unitsPerSample"]!=1.)throw std::runtime_error("Circle unit switch preserves custom limits");
        if(editor.findChild<QDoubleSpinBox*>("mt_minCoverage")->value()!=90.)throw std::runtime_error("coverage displayed as percent");
        editor.findChild<QDoubleSpinBox*>("mt_minCoverage")->setValue(85.);
        if(editor.parameters()["minCoverage"]!=.85)throw std::runtime_error("coverage stored as original fraction");
        editor.findChild<QPushButton*>("metrologyUnitProfile")->click();changed=editor.parameters();
        if(changed["inlierDistance"]!=1. || changed["maxRms"]!=.35 || changed["scanWidth"]!=1.)throw std::runtime_error("Circle explicit pixel profile");
    }

    auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    auto wait=[&]{QApplication::processEvents();QElapsedTimer timer;timer.start();while((editorSession_.busy() || runtimeJobs_.busy()) && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();check(!editorSession_.busy() && !runtimeJobs_.busy(),"Circle UI deadline");};
    QImage raster(192,192,QImage::Format_Grayscale8);for(int y=0;y<192;++y)for(int x=0;x<192;++x){double d=std::hypot(x-96.25,y-95.75)-40;raster.scanLine(y)[x]=uchar(std::lround(128+100*std::erf(d/(std::sqrt(2.)*1.4))));}auto imagePath=QDir(artifacts).filePath("circle-ui-raster.png");check(raster.save(imagePath),"Circle UI independent raster");
    service_.createProject("Circle UI","",QDir(artifacts).filePath("circle-ui-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".vfproject"),"file");service_.createRecipe("Circle","",ApplicationType::Measurement,"file");const auto source=service_.setImageFile(imagePath);refreshWorkspace();selectPage("editor");selectTool(source);runExecution(source);wait();check(editorSession_.frame.has_value(),"Circle UI actual acquisition");
    QListWidgetItem* item=nullptr;for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="circle_fit")item=toolbox_->item(i);check(item,"Circle Toolbox");toolbox_->setCurrentItem(item);findChild<QPushButton*>("addTool")->click();const auto id=selectedToolId_;check(propertiesBody_->findChild<QComboBox*>("mt_mode")->currentText()=="calibrated","new Circle defaults to calibrated");propertiesBody_->findChild<QComboBox*>("mt_mode")->setCurrentText("pixel");auto* input=propertiesBody_->findChild<QComboBox*>("input_image");input->setCurrentIndex(input->findData(source+"/image"));
    auto geometry=[&](double cx,double cy,double inner,double outer,double sweep){selectTool(id);for(const auto& v:QVector<QPair<QString,double>>{{"cx",cx},{"cy",cy},{"inner",inner},{"outer",outer},{"start",17.3},{"sweep",sweep}})propertiesBody_->findChild<QDoubleSpinBox*>("mt_roi_"+v.first)->setValue(v.second);propertiesBody_->findChild<QPushButton*>("metrologyApplyRoi")->click();wait();check(imageViewer_->roi().has_value(),"Circle numeric ROI commit");};
    geometry(96.25,95.75,34,46,360);auto before=imageViewer_->roi();imageViewer_->undoRoi();wait();check(!imageViewer_->roi(),"Circle undo bound ROI");geometry(96.25,95.75,34,46,360);check(imageViewer_->roi()==before,"Circle exact subpixel numeric restore");
    propertiesBody_->findChild<QComboBox*>("mt_polarity")->setCurrentIndex(propertiesBody_->findChild<QComboBox*>("mt_polarity")->findData("rising"));runFlow();wait();auto result=[&]()->ToolResult{check(editorSession_.cycle.has_value(),"Circle cycle");for(const auto& r:editorSession_.cycle->tools)if(r.toolId==id)return r;throw std::runtime_error("Circle result missing");};check(result().state==ExecutionState::Done && std::abs(std::get<double>(result().outputs["diameterPx"].data)-80)<.3,"Circle UI actual px");
    // EUI-04: preview changes nominal geometry immediately, commits only on finish.
    {const auto original=imageViewer_->roi();const auto definition=service_.activeRecipe()->definition;
        auto* sweep=propertiesBody_->findChild<QDoubleSpinBox*>("mt_roi_sweep");sweep->setValue(90);
        check(service_.activeRecipe()->definition==definition && imageViewer_->roi()==original && !editorSession_.cycle,"preview preserves recipe and clears stale fit");
        int arrows=0;for(auto* item:imageViewer_->scene()->items())if(item->data(0)=="scanGuide")++arrows;
        check(arrows==72,"all scan guides visible before Run");
        resize(1400,900);propertiesBody_->findChild<QComboBox*>("propertyDetailMode")->setCurrentIndex(0);QApplication::processEvents();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(sweep);imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("eui04-arc-preview-1400.png"));
        resize(850,600);QApplication::processEvents();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(sweep);imageViewer_->fitImage();grab().save(QDir(artifacts).filePath("eui04-arc-preview-850.png"));resize(1400,900);
        QMetaObject::invokeMethod(sweep,"editingFinished",Qt::DirectConnection);wait();
        check(std::get<ArcRoi>(imageViewer_->roi()->geometry).sweepDegrees==90,"angle finish commits ROI atomically");
        const auto valid=service_.activeRecipe()->definition;sweep=propertiesBody_->findChild<QDoubleSpinBox*>("mt_roi_sweep");sweep->setValue(0);QMetaObject::invokeMethod(sweep,"editingFinished",Qt::DirectConnection);wait();
        check(service_.activeRecipe()->definition==valid && std::get<ArcRoi>(imageViewer_->roi()->geometry).sweepDegrees==90,"invalid zero sweep preserves last valid geometry");
        geometry(96.25,95.75,34,46,360);
    }
    // EUI-08 visual matrix uses the real Editor/application guide path before Run.
    ToolDefinition guideTool;for(const auto& t:service_.tools())if(t.id==id)guideTool=t;
    for(const auto direction:{QString("outward"),QString("inward")}){
        auto configured=guideTool;configured.parameters["scanCount"]=10;configured.parameters["direction"]=direction;
        service_.updateTool(configured);refreshWorkspace();
        for(double angle:{360.,180.,90.}){
            geometry(96.25,95.75,34,46,angle);
            int arrows=0;for(auto* item:imageViewer_->scene()->items())if(item->data(0)=="scanGuide")++arrows;
            check(arrows==10 && !editorSession_.cycle,"ten nominal arrows with no stale fit");
            if(angle==360.)check(propertiesBody_->findChild<QLabel*>("metrologyPreflight")->text().contains("MUI_GAP"),"sparse circle gap warning before Run");

            for(const auto size:{QSize(1400,900),QSize(850,600)}){
                resize(size);QApplication::processEvents();imageViewer_->fitImage();QApplication::processEvents();
                check(grab().save(QDir(artifacts).filePath(QString("eui08-circle-%1-%2-%3.png").arg(direction).arg(int(angle)).arg(size.width()))),"Circle direction and sweep PNG");
            }
        }
    }
    service_.updateTool(guideTool);refreshWorkspace();selectTool(id);resize(1400,900);
    service_.setCalibrationSetup("lens","plane");const auto frame=*editorSession_.frame;const auto setup=CalibrationWorkflow::installation(*service_.activeRecipe(),frame);auto sample=[&](QString name,QPointF a,QPointF b,double mm){return CalibrationSample{name,{a,setup.source},{b,setup.source},mm};};const auto accepted=MeasurementService{}.validate(CalibrationWorkflow::teach(*service_.activeRecipe(),frame,"circle-scale",1,sample("teach",{0,0},{100,0},10)),{sample("x",{10,20},{110,20},10),sample("y",{30,10},{30,110},10)},.001);const auto catalog=service_.path()+".calibrations.json";CalibrationService(catalog).save(accepted);CalibrationWorkflow::bind(service_,frame,catalog,"circle-scale");refreshWorkspace();selectTool(id);propertiesBody_->findChild<QComboBox*>("mt_mode")->setCurrentText("calibrated");wait();check(!imageViewer_->roi(),"mode changes discard raw teaching");
    geometry(9.625,-9.575,3.4,4.6,360);for(auto t:service_.tools())if(t.id==id){t.parameters["scanWidth"]=.1;t.parameters["inlierDistance"]=.1;t.parameters["maxRms"]=.035;t.parameters["maxResidual"]=.1;t.parameters["diameterLower"]=7.97;t.parameters["diameterUpper"]=8.03;t.parameters["deviationUpper"]=.04;service_.updateTool(t);}refreshWorkspace();selectTool(id);runFlow();wait();check(result().state==ExecutionState::Done && result().outputs["circleMm"].valid && std::get<bool>(result().outputs["accepted"].data),"Circle UI calibrated tolerance");
    auto hash=editorSession_.frame->contentId();runExecution(id);wait();check(editorSession_.cycle->partial && editorSession_.cycle->acquisitions==0 && editorSession_.cycle->executedTools==QStringList{id} && editorSession_.frame->contentId()==hash,"Circle UI TargetOnly");
    resize(1400,900);horizontal_->setSizes({155,750,300});vertical_->setSizes({540,240});findChild<QTabWidget*>("editorTabs")->setCurrentIndex(1);runFlow();wait();imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("circle-calibrated.png"));resize(850,600);QApplication::processEvents();imageViewer_->fitImage();grab().save(QDir(artifacts).filePath("circle-small.png"));resize(1400,900);
    for(auto t:service_.tools())if(t.id==id){t.parameters["maxGapDegrees"]=4.99;service_.updateTool(t);}refreshWorkspace();runFlow();wait();check(result().state==ExecutionState::Done && !result().outputs["accepted"].valid && result().message.contains("NOT_EVALUATED"),"Circle UI quality NOT_EVALUATED");imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("circle-invalid-coverage.png"));
    for(auto t:service_.tools())if(t.id==id){t.parameters["maxGapDegrees"]=15;service_.updateTool(t);}refreshWorkspace();geometry(9.625,-9.575,3.4,4.6,-220);runFlow();wait();check(std::get<QString>(result().outputs["scope"].data)=="observed-arc" && !result().outputs["circularityIndex"].valid,"Circle UI signed arc");imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("circle-arc.png"));geometry(9.625,-9.575,3.4,4.6,360);service_.save();const auto recipe=service_.activeRecipe()->id,revision=QString::number(service_.activeRecipe()->recipeVersion);service_.openProject(service_.path());service_.selectRecipe(recipe);service_.save();check(QString::number(service_.activeRecipe()->recipeVersion)==revision,"Circle UI save reopen no-op");refreshWorkspace();selectTool(id);
    selectPage("recipes");refreshRecipes();for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==recipe)recipeTable_->selectRow(row);findChild<QPushButton*>("loadRecipe")->click();wait();findChild<QPushButton*>("runtime_run")->click();wait();check(runtime_ && runtime_->active(),"Circle Runtime prepared");for(auto t:service_.tools())if(t.id==id){t.parameters["minContrast"]=5100;service_.updateTool(t);}findChild<QPushButton*>("runtime_cycle")->click();wait();if(!runtimeResult_ || runtimeResult_->state!=ExecutionState::Done)QTextStream(stderr)<<"Circle runtime error: "<<runtimeCoordinator_->error()<<" summary="<<runtimeSummary_->text()<<"\n";check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done,"Circle immutable Runtime");runtimeTools_->setCurrentIndex(runtimeTools_->findData(id));runtimeViewer_->fitImage();QApplication::processEvents();check(!CameraFlowSmokeAudit::runtimeScan(*runtimeViewer_,id).isEmpty(),"Circle readonly world scan layer");grab().save(QDir(artifacts).filePath("circle-runtime.png"));findChild<QPushButton*>("runtime_stop")->click();check(!runtimeResult_ && !runtimeViewer_->hasImage(),"Circle Stop clears Runtime");check(ProductionResultCodec::csv(ProductionStore(service_.path()+".production").recent("cycles")).contains("radial-circle"),"Circle persisted typed CSV");
    refreshWorkspace();selectPage("editor");selectTool(id);runFlow();wait();check(result().state==ExecutionState::Error && result().outputs.isEmpty(),"Circle no fit clears old result");
    check(result().fitDiagnostic && result().errorCode=="CIRCLE_POINTS" && result().fitDiagnostic->rejections.contrast>0,"Circle visible failed scan summary");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();vertical_->setSizes({330,240});imageViewer_->fitImage();QApplication::processEvents();check(grab().save(QDir(artifacts).filePath(QString("mui03-diagnostic-%1.png").arg(size.width()))),"failed-run guidance screenshot");}resize(1400,900);
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();grab().save(QDir(artifacts).filePath(QString("mui01-circle-failure-%1.png").arg(size.width())));}
    service_.save();selectPage("recipes");refreshRecipes();for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==recipe)recipeTable_->selectRow(row);
    findChild<QPushButton*>("loadRecipe")->click();wait();findChild<QPushButton*>("runtime_run")->click();wait();findChild<QPushButton*>("runtime_cycle")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Error,"Runtime failed scan is ERROR");runtimeTools_->setCurrentIndex(runtimeTools_->findData(id));check(runtimeSummary_->text().startsWith("Circle_/_Arc_1 • ERROR") && runtimeSummary_->text().contains("CIRCLE_POINTS"),"blocking diagnosis precedes metadata");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();grab().save(QDir(artifacts).filePath(QString("mui03-runtime-%1.png").arg(size.width())));}
    const auto failedRecords=ProductionStore(service_.path()+".production").recent("cycles");check(ProductionResultCodec::summary(failedRecords.first()).contains("CIRCLE_POINTS") && ProductionResultCodec::csv({failedRecords.first()}).contains("measurementDiagnostic"),"persisted failure explanation");
    findChild<QPushButton*>("runtimeHistory")->click();QApplication::processEvents();logTable_->setCurrentCell(0,0);check(findChild<QTextEdit*>("logDetail")->toPlainText().contains("CIRCLE_POINTS"),"Logs opens persisted failed-run explanation");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();grab().save(QDir(artifacts).filePath(QString("mui03-logs-%1.png").arg(size.width())));}
    selectPage("runtime");findChild<QPushButton*>("runtime_stop")->click();refreshWorkspace();selectPage("editor");selectTool(id);

for(auto t:service_.tools())if(t.id==id){t.parameters["minContrast"]=20;service_.updateTool(t);}refreshWorkspace();runFlow();findChild<QPushButton*>("stopFlow")->click();wait();check(result().outputs.isEmpty() && result().lines.isEmpty(),"Circle late Stop clears overlays");service_.save();
    resize(1400,900);horizontal_->setSizes({155,750,300});vertical_->setSizes({540,240});QApplication::processEvents();
    auto projectPoint=[](QPointF q){double w=1+.004*q.x()+.002*q.y();return QPointF((3.5*q.x()+.4*q.y()+20)/w,(.2*q.x()+4*q.y()+20)/w);};
    auto unproject=[](QPointF p){double a=3.5-.004*p.x(),b=.4-.002*p.x(),c=.2-.004*p.y(),d=4-.002*p.y(),det=a*d-b*c;return QPointF(((p.x()-20)*d-b*(p.y()-20))/det,(a*(p.y()-20)-c*(p.x()-20))/det);};
    auto perspectiveRaster=[&](bool damaged){QImage pixels(192,192,QImage::Format_Grayscale16);for(int y=0;y<192;++y)for(int x=0;x<192;++x){const auto q=unproject({double(x),double(y)})-QPointF(15,15);double angle=std::atan2(q.y(),q.x())*180/3.141592653589793;double radius=damaged && angle>29 && angle<41 ? 8:7;double d=std::hypot(q.x(),q.y())-radius;reinterpret_cast<quint16*>(pixels.scanLine(y))[x]=quint16(std::lround((.5+.4*std::erf(d/(std::sqrt(2.)*.4)))*65535));}check(pixels.save(imagePath),"Circle perspective analytic raster");};
    perspectiveRaster(false);selectTool(source);runExecution(source);wait();const auto hframe=*editorSession_.frame;
    ChessboardObservation obs{{"circle-ui-projective",7,7,5},hframe.metadata(),{192,192},{},{}};for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);obs.worldMillimetres.append(q);obs.pixels.append(projectPoint(q));}
    auto hs=accepted.dependency();hs.calibrationId="circle-h";hs.target.axes=AxisConvention::XRightYDown;auto hold=[&](QString name,QPointF a,QPointF b){return CalibrationSample{name,{projectPoint(a),hs.source},{projectPoint(b),hs.source},QLineF(a,b).length()};};const auto h=MeasurementService{}.validate(MeasurementService{}.teachHomography(hs,PlaneMappingService::fit(obs)),{hold("hx",{2,3},{27,3}),hold("hy",{3,2},{3,28})},.0001);CalibrationService(catalog).save(h);CalibrationWorkflow::bind(service_,hframe,catalog,"circle-h","raw_planar_homography");
    for(auto t:service_.tools())if(t.id==id){t.parameters["scanRoi"]=ToolRoi{h.dependency().target,ArcRoi{{{15,15},3,11},17.3,360}}.toJson();t.parameters["unitsPerSample"]=.25;t.parameters["scanWidth"]=.25;t.parameters["diameterLower"]=13.94;t.parameters["diameterUpper"]=14.06;t.parameters["deviationUpper"]=.02;service_.updateTool(t);}refreshWorkspace();selectTool(id);runFlow();wait();check(result().state==ExecutionState::Done && std::abs(std::get<double>(result().outputs["diameterMm"].data)-14)<.06 && std::get<bool>(result().outputs["accepted"].data),"Circle UI actual perspective mm");imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("circle-perspective.png"));
    perspectiveRaster(true);runFlow();wait();check(result().state==ExecutionState::Done && !result().rejectedPoints.isEmpty() && std::get<double>(result().outputs["deviationMm"].data)>.5 && !std::get<bool>(result().outputs["deviationAccepted"].data),"Circle UI fitted outlier remains in defect deviation");imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("circle-perspective-outliers.png"));service_.save();return true;
}
}

namespace visionflow {
bool MainWindow::verifyGeometry(const QString& artifacts){
    auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    auto wait=[&]{QApplication::processEvents();QElapsedTimer timer;timer.start();while((editorSession_.busy() || runtimeJobs_.busy()) && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();check(!editorSession_.busy() && !runtimeJobs_.busy(),"Geometry UI deadline");};
    QImage stripe(256,256,QImage::Format_Grayscale8);stripe.fill(20);for(int y=60;y<160;++y)for(int x=0;x<256;++x)stripe.scanLine(y)[x]=220;const auto imagePath=QDir(artifacts).filePath("geometry-stripe.png");check(stripe.save(imagePath),"Geometry independent raster");
    service_.createProject("Geometry UI","",QDir(artifacts).filePath("geometry-ui-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".vfproject"),"file");service_.createRecipe("Geometry","",ApplicationType::Measurement,"file");const auto source=service_.setImageFile(imagePath),edge=service_.addTool("edge_point"),edge2=service_.addTool("edge_point"),line=service_.addTool("line_fit");
    for(auto t:service_.tools()){
        if(t.id==edge || t.id==edge2){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},t.id==edge ? ScanSegment{{100,190},{100,120},2}:ScanSegment{{100,20},{100,100},2}}.toJson();t.parameters["polarity"]="rising";}
        if(t.id==line){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},RotatedRectangle{{128,60},{200,50},0}}.toJson();t.parameters["polarity"]="rising";}service_.updateTool(t);
    }
    service_.setConnections({{source,"image",edge,"image"},{source,"image",edge2,"image"},{source,"image",line,"image"}});refreshWorkspace();selectPage("editor");runFlow();wait();check(editorSession_.frame.has_value(),"Geometry acquired frame");
    service_.setCalibrationSetup("lens","plane");const auto frame=*editorSession_.frame;const auto setup=CalibrationWorkflow::installation(*service_.activeRecipe(),frame);auto sample=[&](QString name,QPointF a,QPointF b,double mm){return CalibrationSample{name,{a,setup.source},{b,setup.source},mm};};const auto accepted=MeasurementService{}.validate(CalibrationWorkflow::teach(*service_.activeRecipe(),frame,"geometry-scale",1,sample("teach",{0,0},{100,0},10)),{sample("x",{10,20},{110,20},10),sample("y",{30,10},{30,110},10)},.001);const auto catalog=service_.path()+".calibrations.json";CalibrationService(catalog).save(accepted);CalibrationWorkflow::bind(service_,frame,catalog,"geometry-scale");refreshWorkspace();
    QListWidgetItem* item=nullptr;for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="geometry_measurement")item=toolbox_->item(i);check(item,"Geometry Toolbox");toolbox_->setCurrentItem(item);findChild<QPushButton*>("addTool")->click();const auto id=selectedToolId_;
    // CCUX-24D: real recipe commit/save/reopen retains untouched doubles.
    for(auto t:service_.tools())if(t.id==id){t.parameters["nominal"]=1.23456789;t.parameters["fitMaxRms"]=.000123456789;service_.updateTool(t);}
    selectTool(id);const auto precisionDefinition=service_.activeRecipe()->definition;service_.save();const auto precisionRevision=service_.activeRecipe()->recipeVersion;
    check(applyToolProperties() && service_.activeRecipe()->definition==precisionDefinition,"precision no-op Properties preserves recipe content");
    selectTool(source);selectTool(id);check(applyToolProperties(),"precision tool switch commit");service_.save();check(service_.activeRecipe()->recipeVersion==precisionRevision,"precision no-op save hash/version");
    const auto precisionRecipe=service_.activeRecipe()->id;service_.openProject(service_.path());service_.selectRecipe(precisionRecipe);refreshWorkspace();selectTool(id);check(applyToolProperties() && service_.activeRecipe()->definition==precisionDefinition,"precision save reopen retains exact recipe");
    propertiesBody_->findChild<QCheckBox*>("gm_toleranceEnabled")->toggle();
    for(const auto& t:service_.tools())if(t.id==id)check(t.parameters["nominal"]==1.23456789 && t.parameters["fitMaxRms"]==.000123456789,"unrelated real recipe commit keeps legacy precision");
    auto* preciseInput=propertiesBody_->findChild<QDoubleSpinBox*>("gm_nominal");preciseInput->setFocus();preciseInput->selectAll();
    const auto localizedNominal=preciseInput->locale().toString(1.235,'f',3);
    for(const auto ch:localizedNominal){QKeyEvent key(QEvent::KeyPress,ch.unicode(),Qt::NoModifier,QString(ch));QApplication::sendEvent(preciseInput,&key);}
    check(applyToolProperties(),"explicit three-decimal recipe edit");
    for(auto t:service_.tools())if(t.id==id){check(t.parameters["nominal"]==1.235,"explicit recipe value committed");t.parameters=geometryMeasurementDefaults();service_.updateTool(t);}refreshWorkspace();selectTool(id);
    auto binding=[&](QString input,QString producer,QString port){auto* c=propertiesBody_->findChild<QComboBox*>("input_"+input);check(c && c->findData(producer+"/"+port)>0,"typed feature choice present");c->setCurrentIndex(c->findData(producer+"/"+port));wait();};
    auto operation=[&](QString op){selectTool(id);propertiesBody_->findChild<QComboBox*>("gm_operation")->setCurrentText(op);wait();};
    auto result=[&]()->ToolResult{check(editorSession_.cycle.has_value(),"Geometry UI cycle");for(const auto& r:editorSession_.cycle->tools)if(r.toolId==id)return r;throw std::runtime_error("Geometry result absent");};
    binding("image",source,"image");check(propertiesBody_->findChild<QComboBox*>("input_p1") && !propertiesBody_->findChild<QComboBox*>("input_line1"),"only required point inputs visible");binding("p1",edge,"point");binding("p2",edge2,"point");runFlow();wait();check(result().outputs["valueMm"].valid && std::abs(std::get<double>(result().outputs["valueMm"].data)-10)<.001,"Geometry two edges 10mm");
    operation("y_offset");propertiesBody_->findChild<QComboBox*>("gm_offset")->setCurrentText("signed");wait();binding("p1",edge2,"point");binding("p2",edge,"point");
    for(const auto& pair:QVector<QPair<QString,double>>{{"lower",-10.01},{"nominal",-10},{"upper",-9.99}})propertiesBody_->findChild<QDoubleSpinBox*>("gm_"+pair.first)->setValue(pair.second);check(applyToolProperties(),"negative signed limits");propertiesBody_->findChild<QCheckBox*>("gm_toleranceEnabled")->setChecked(true);runFlow();wait();check(std::get<bool>(result().outputs["accepted"].data) && std::abs(std::get<double>(result().outputs["valueMm"].data)+10)<.001,"Y-up signed -10mm inclusive tolerance");
    operation("point_line_distance");for(const auto& pair:QVector<QPair<QString,double>>{{"lower",9.99},{"nominal",10},{"upper",10.01}})propertiesBody_->findChild<QDoubleSpinBox*>("gm_"+pair.first)->setValue(pair.second);check(applyToolProperties(),"distance limits");propertiesBody_->findChild<QComboBox*>("gm_offset")->setCurrentText("absolute");wait();binding("p1",edge,"point");binding("line1",line,"linePx");auto* samples=propertiesBody_->findChild<QComboBox*>("input_samples1");check(samples && !samples->isEnabled() && samples->currentData()==line+"/observationsPx","same producer samples auto paired");check(!propertiesBody_->findChild<QComboBox*>("input_p2"),"unused input hidden");runFlow();wait();check(std::get<bool>(result().outputs["accepted"].data) && result().lines.size()==2,"Geometry refit perpendicular helper");
    operation("line_angle");binding("line1",line,"linePx");binding("line2",line,"linePx");for(const auto& pair:QVector<QPair<QString,double>>{{"lower",0},{"nominal",0},{"upper",0}})propertiesBody_->findChild<QDoubleSpinBox*>("gm_"+pair.first)->setValue(pair.second);check(applyToolProperties(),"angle degree limits");runFlow();wait();check(std::get<double>(result().outputs["angleMm"].data)==0 && !result().outputs["valueMm"].valid,"Geometry UI angle degrees");
    operation("point_line_distance");for(const auto& pair:QVector<QPair<QString,double>>{{"lower",9.99},{"nominal",10},{"upper",10.01}})propertiesBody_->findChild<QDoubleSpinBox*>("gm_"+pair.first)->setValue(pair.second);check(applyToolProperties(),"restore distance limits");binding("p1",edge,"point");binding("line1",line,"linePx");runFlow();wait();
    const auto hash=editorSession_.frame->contentId();runExecution(id);wait();check(editorSession_.cycle->partial && editorSession_.cycle->acquisitions==0 && editorSession_.frame->contentId()==hash,"Geometry UI TargetOnly");
    resize(1400,900);horizontal_->setSizes({155,750,300});vertical_->setSizes({540,240});findChild<QTabWidget*>("editorTabs")->setCurrentIndex(1);imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("geometry-editor.png"));check(CameraFlowSmokeAudit::wheel(*imageViewer_,*this,QDir(artifacts).filePath("ccux24e-editor-wheel-1400.png")),"Editor wheel normal");propertiesBody_->findChild<QComboBox*>("input_line1")->setFocus();if(auto* scroll=qobject_cast<QScrollArea*>(propertiesBody_->parentWidget()->parentWidget()))scroll->ensureWidgetVisible(propertiesBody_->findChild<QComboBox*>("input_line1"));QApplication::processEvents();grab().save(QDir(artifacts).filePath("geometry-features.png"));resize(850,600);QApplication::processEvents();imageViewer_->fitImage();grab().save(QDir(artifacts).filePath("geometry-small.png"));check(CameraFlowSmokeAudit::wheel(*imageViewer_,*this,QDir(artifacts).filePath("ccux24e-editor-wheel-850.png")),"Editor wheel small");resize(1400,900);
    service_.save();const auto recipe=service_.activeRecipe()->id;const auto revision=service_.activeRecipe()->recipeVersion;service_.openProject(service_.path());service_.selectRecipe(recipe);service_.save();check(service_.activeRecipe()->recipeVersion==revision,"Geometry UI Save reopen no-op");refreshWorkspace();selectPage("recipes");refreshRecipes();for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==recipe)recipeTable_->selectRow(row);findChild<QPushButton*>("loadRecipe")->click();wait();findChild<QPushButton*>("runtime_run")->click();wait();check(runtime_ && runtime_->active(),"Geometry Load/Run");
    for(auto t:service_.tools())if(t.id==id){t.parameters["upper"]=100;service_.updateTool(t);}findChild<QPushButton*>("runtime_cycle")->click();wait();if(!runtimeResult_)QTextStream(stderr)<<runtimeCoordinator_->error();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done,"Geometry immutable Runtime recorded");runtimeTools_->setCurrentIndex(runtimeTools_->findData(id));runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("geometry-runtime.png"));for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();check(CameraFlowSmokeAudit::wheel(*runtimeViewer_,*this,QDir(artifacts).filePath(QString("ccux24e-runtime-wheel-%1.png").arg(size.width()))),"Runtime viewport wheel");}findChild<QPushButton*>("runtime_stop")->click();check(!runtimeResult_ && !runtimeViewer_->hasImage(),"Geometry Runtime Stop clears");check(ProductionResultCodec::csv(ProductionStore(service_.path()+".production").recent("cycles")).contains("geometry-refit/point_line_distance"),"Geometry Logs/restart/CSV");
    refreshWorkspace();selectPage("editor");selectTool(id);runFlow();findChild<QPushButton*>("stopFlow")->click();wait();check(result().outputs.isEmpty() && result().lines.isEmpty(),"Geometry Editor Stop clears");return true;
}
}

namespace visionflow {
bool MainWindow::verifyRoi(const QString& artifacts) {
    auto check=[](bool b,const char* text){if(!b)throw std::runtime_error(text);};
    auto wait=[&]{QElapsedTimer timer;timer.start();while(editorSession_.busy() && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}check(!editorSession_.busy(),"ROI UI timeout");};
    service_.createProject("ROI acceptance","",QDir(artifacts).filePath("roi-ui-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".vfproject"),"file");
    const auto recipe=service_.createRecipe("ROI fixture","",ApplicationType::DefectInspection,"file");refreshWorkspace();selectPage("editor");
    QImage fixture(500,320,QImage::Format_Grayscale8);fixture.fill(0);{QPainter p(&fixture);p.fillRect(QRect(60,60,40,40),Qt::white);p.fillRect(QRect(340,200,60,60),Qt::white);}
    const auto path=QDir(artifacts).filePath("roi-input.png");check(fixture.save(path) && loadEditorImage(path),"ROI fixture acquisition");
    const auto source=selectedToolId_,roi=service_.addTool("roi"),blob=service_.addTool("blob");
    QVector<ToolConnection> links{{source,"image",roi,"image"},{source,"image",blob,"image"}};service_.setConnections(links);refreshWorkspace();selectTool(roi);
    resize(1400,900);QApplication::processEvents();imageViewer_->fitImage();
    auto draw=[&](QPointF a,QPointF b){
        auto* button=propertiesBody_->findChild<QPushButton*>("searchRoi");check(button && button->isEnabled(),"ROI draw enabled");button->click();
        auto mouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons){const auto pos=imageViewer_->mapFromScene(point);QMouseEvent e(type,QPointF(pos),QPointF(imageViewer_->viewport()->mapToGlobal(pos)),button,buttons,Qt::NoModifier);QApplication::sendEvent(imageViewer_->viewport(),&e);QApplication::processEvents();};
        mouse(QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);
    };
    draw({40.5,40.5},{130.5,130.5});
    for(auto t:service_.tools())if(t.id==roi){check(!t.parameters["searchRoi"].toObject().isEmpty(),"mouse drawing persists ROI");t.name="Inspection ROI";service_.updateTool(t);}
    selectTool(blob);auto* regionInput=propertiesBody_->findChild<QComboBox*>("input_region");check(regionInput && regionInput->findData(roi+"/region")>0,"ROI available in actual Region picker");regionInput->setCurrentIndex(regionInput->findData(roi+"/region"));QApplication::processEvents();selectTool(roi);runFlow();wait();
    auto area=[&]{for(const auto& r:editorSession_.cycle->tools)if(r.toolId==blob)return std::get<double>(r.outputs["area"].data);return -1.;};
    check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done && area()==1600,"UI ROI limits actual Blob");
    runExecution(roi);wait();check(editorSession_.cycle->state==ExecutionState::Done && editorSession_.cycle->executedTools==QStringList{roi},"UI Run Tool ROI");runExecution(blob);wait();check(area()==1600,"UI Run Tool downstream");
    service_.save();const auto saved=service_.activeRecipe()->definition;service_.openProject(service_.path());service_.selectRecipe(recipe);refreshWorkspace();selectPage("editor");selectTool(roi);runFlow();wait();check(service_.activeRecipe()->definition==saved && area()==1600,"UI ROI reopen");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();imageViewer_->fitImage();grab().save(QDir(artifacts).filePath(QString("roi-editor-%1.png").arg(size.width())));}
    resize(1400,900);QApplication::processEvents();imageViewer_->fitImage();draw({300.5,170.5},{430.5,290.5});runFlow();wait();check(area()==3600,"UI redraw selects second object");service_.save();return true;
}
}
