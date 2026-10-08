#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include "../../tests/chessboard_fixture.hpp"
#include <QApplication>
#include <QMenuBar>
#include <QAction>
#include <QStackedWidget>
#include <QMenuBar>
#include <QAction>
#include <QStackedWidget>
#include <QCheckBox>
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
#include <QPushButton>
#include <QLabel>
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
    QProcess simulator;simulator.setProgram(QDir(artifacts).filePath("visionflow_plc_simulator.exe"));
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
    findChild<QPushButton*>("runtime_run")->click();findChild<QPushButton*>("runtime_cycle")->click();QLockFile lock(service_.path()+".production/archive.lock");lock.setStaleLockTime(0);check(lock.tryLock(),"UI log lock fixture");wait();check(runtimePending_ && runtime_->busy() && !runtimeResult_ && !findChild<QPushButton*>("runtime_cycle")->isEnabled(),"failed record suppresses result and blocks cycle");QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-record-pending.png"));findChild<QPushButton*>("runtime_stop")->click();load();check(runtimeCoordinator_->loaded()->id==recipeId && runtimePending_,"pending record prevents replacement");lock.unlock();findChild<QPushButton*>("runtime_retry")->click();check(!runtimePending_ && runtimeResult_ && runtimeCoordinator_->state()==RuntimeState::LoadedStopped,"retry same terminal after Stop");
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
    toolbox_->setCurrentRow(1); findChild<QPushButton*>("addTool")->click();
    const auto matchId=selectedToolId_; if (matchId==sourceId) return false;
    auto* score=propertiesBody_->findChild<QDoubleSpinBox*>("minScore"); if (!score) return false;
    score->setValue(84.5);
    auto* name=propertiesBody_->findChild<QLineEdit*>("toolName"); if (!name) return false;
    name->setText("Match_Part"); if (!applyToolProperties()) return false;
    name->setText(""); if (applyToolProperties()) return false;
    saveProject(); if (!service_.dirty()) return false;
    name->setText("Match_Part"); if (!applyToolProperties()) return false;
    if (!service_.dirty() || service_.activeRecipe()->status!=RecipeStatus::Draft) return false;
    findChild<QPushButton*>("zoomIn")->click(); const auto zoom=imageViewer_->zoomFactor();
    findChild<QPushButton*>("zoomOut")->click(); if (imageViewer_->zoomFactor()>=zoom) return false;
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
    findChild<QPushButton*>("reloadImage")->click(); if (editorSession_.frame || imageViewer_->hasImage() || service_.dirty()) return false;
    if (!image.save(fixture)) return false;
    findChild<QPushButton*>("reloadImage")->click();
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
    selectPage("calibration");calibrationLens_->setText("fixture-lens");calibrationPlane_->setText("fixture-plane");findChild<QPushButton*>("calibrationSetup")->click();findChild<QPushButton*>("calibrationAcquire")->click();wait();
    calibrationId_->setText("fixture-scale");
    for(int r=0;r<3;++r)findChild<QPushButton*>("calibrationAddSample")->click();
    const QVector<QStringList> rows{{"teach","100","100","500","100","10"},{"holdout-x","200","50","800","50","15"},{"holdout-y","300","20","300","520","12.5"}};
    for(int r=0;r<rows.size();++r)for(int c=0;c<6;++c)calibrationSamples_->item(r,c)->setText(rows[r][c]);
    findChild<QPushButton*>("calibration_calculate")->click();wait();check(calibrationCandidate_ && !calibrationCandidate_->validated() && !calibrationStatus_->text().contains("RMS"),"UI calculate candidate without invented validation error");
    findChild<QPushButton*>("calibration_validate")->click();wait();check(calibrationCandidate_ && calibrationCandidate_->validated(),"UI independent holdout accepted");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("calibration-validated.png"));
    resize(850,600);QApplication::processEvents();check(width()==850 && height()==600,"calibration minimum size preserved");grab().save(QDir(artifacts).filePath("calibration-small.png"));resize(1400,900);QApplication::processEvents();
    findChild<QPushButton*>("calibration_save")->click();wait();check(service_.activeRecipe()->calibrationBinding && service_.dirty(),"UI save/bind DRAFT");saveProject();
    service_.openProject(service_.path());service_.selectRecipe(recipeId);refreshWorkspace();selectPage("calibration");findChild<QPushButton*>("calibration_load")->click();wait();check(calibrationSamples_->rowCount()==3 && !calibrationCandidate_,"UI saved evidence reopened without automatic acceptance");
    selectPage("editor");selectTool(measurement);runFlow();wait();check(result().decision==PartDecision::Pass && std::abs(std::get<double>(result().outputs["value"].data)-9.975)<1e-12,"UI calibrated actual Edge PASS");
    check(service_.activeRecipe()->status==RecipeStatus::Draft,"mm execution is not recipe validation");imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("measurement-pass.png"));
    propertiesBody_->findChild<QDoubleSpinBox*>("upperMm")->setValue(9.9749);check(applyToolProperties(),"UI raw fail boundary");runFlow();wait();check(result().state==ExecutionState::Done && result().decision==PartDecision::Fail,"UI FAIL separate from DONE");QApplication::processEvents();grab().save(QDir(artifacts).filePath("measurement-fail.png"));
    runFlow();findChild<QPushButton*>("stopFlow")->click();wait();check(editorSession_.cycle->state==ExecutionState::Skipped && result().outputs.isEmpty() && result().decision==PartDecision::NotEvaluated,"UI stop clears decision");
    selectPage("calibration");calibrationLens_->setText("changed-lens");findChild<QPushButton*>("calibrationSetup")->click();check(!service_.activeRecipe()->calibrationBinding,"UI installation invalidation");selectPage("editor");runFlow();wait();check(result().errorCode=="CALIBRATION_INVALID" && result().outputs.isEmpty() && !matchInfo_->text().contains("9.975"),"UI changed calibration suppresses previous mm output");QApplication::processEvents();grab().save(QDir(artifacts).filePath("measurement-invalid.png"));
    saveProject();return true;
}
bool MainWindow::verifyExecution(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto recipe=service_.createRecipe("Flow acceptance","",ApplicationType::Measurement,"file");refreshWorkspace();selectPage("editor");
    QImage fixture(1000,600,QImage::Format_Grayscale8);fixture.fill(0);{QPainter painter(&fixture);painter.fillRect(QRect(200,100,400,300),Qt::white);}
    const auto path=QDir(artifacts).filePath("step5-input.png");check(fixture.save(path),"flow fixture");check(loadEditorImage(path),"flow acquisition");
    const auto source=selectedToolId_;const auto left=service_.addTool("edge"),right=service_.addTool("edge"),blob=service_.addTool("blob"),distance=service_.addTool("calculate");
    for(auto t:service_.tools())if(t.id==right){t.parameters["direction"]="right_to_left";service_.updateTool(t);}refreshWorkspace();
    auto bind=[&](const QString& target,const QString& input,const QString& upstream,const QString& output){selectTool(target);auto* combo=propertiesBody_->findChild<QComboBox*>("input_"+input);check(combo!=nullptr,"input combo");const auto i=combo->findData(upstream+"/"+output);check(i>0,"compatible output");combo->setCurrentIndex(i);};
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
    toolbox_->setCurrentRow(3);findChild<QPushButton*>("addTool")->click();
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
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("a2-reference-restored.png"));service_.save();return true;
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
    // A preview alone is not a DAG image input. Run Match must reject it.
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
bool MainWindow::verifyChessboard(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto wait=[&]{QElapsedTimer timer;timer.start();while(editorSession_.busy() && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}check(!editorSession_.busy(),"chessboard worker timeout");};
    selectPage("calibration");const auto definition=service_.activeRecipe()->definition;const auto binding=service_.activeRecipe()->calibrationBinding;const auto sourceFrame=editorSession_.frame;
    const auto path=QDir(artifacts).filePath("chessboard-test.png");check(chessboardFixture(true).save(path),"board saved");
    importCalibrationTestImage(path);wait();check(calibrationTestImage_ && !calibrationChessboard_,"UI test import isolated");
    findChild<QLineEdit*>("chessboardTargetId")->setText("fixture-board");findChild<QDoubleSpinBox*>("chessboardSpacing")->setValue(10);
    findChild<QPushButton*>("calibrationDetectChessboard")->click();wait();check(calibrationChessboard_ && calibrationChessboard_->pixels.size()==54,"UI true detected overlay");
    check(service_.activeRecipe()->definition==definition && service_.activeRecipe()->calibrationBinding==binding && (!sourceFrame || editorSession_.frame->metadata().frameId==sourceFrame->metadata().frameId),"test import preserves recipe/source/binding");
    check(!findChild<QPushButton*>("calibration_save")->isEnabled(),"test image cannot bind legacy scale");
    findChild<QPushButton*>("calibrationFitHomography")->click();wait();check(calibrationMapping_.has_value() && calibrationMappingStatus_->text().contains("Fit RMS") && !calibrationCandidate_,"UI H candidate distinct from validation");
    const auto a=calibrationChessboard_->pixels[10],b=calibrationChessboard_->pixels[43];findChild<QDoubleSpinBox*>("trialX1")->setValue(a.x());findChild<QDoubleSpinBox*>("trialY1")->setValue(a.y());findChild<QDoubleSpinBox*>("trialX2")->setValue(b.x());findChild<QDoubleSpinBox*>("trialY2")->setValue(b.y());findChild<QPushButton*>("calibrationTrialDistance")->click();wait();check(calibrationTrialStatus_->text().contains("mm") && !calibrationTrialStatus_->text().startsWith("Mesafe yok"),"UI actual plane distance trial");QApplication::processEvents();grab().save(QDir(artifacts).filePath("homography-trial.png"));
    findChild<QDoubleSpinBox*>("trialX1")->setValue(0);findChild<QDoubleSpinBox*>("trialY1")->setValue(0);findChild<QPushButton*>("calibrationTrialDistance")->click();wait();check(calibrationTrialStatus_->text().startsWith("Mesafe yok"),"UI rejects extrapolation and clears previous mm");calibrationTrialPoints_.clear();

    const auto points=calibrationChessboard_->pixels;calibrationViewer_->zoomBy(1.2);refreshCalibration();check(calibrationChessboard_->pixels==points,"zoom independent raw points");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("chessboard-detected.png"));resize(850,600);QApplication::processEvents();calibrationViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("chessboard-small.png"));resize(1400,900);
    findChild<QSpinBox*>("chessboardColumns")->setValue(8);check(!calibrationChessboard_,"target edit clears old corners");findChild<QPushButton*>("calibrationDetectChessboard")->click();wait();check(!calibrationChessboard_,"wrong target suppresses old success");findChild<QSpinBox*>("chessboardColumns")->setValue(9);
    detectCalibrationChessboard();editorSession_.stop(SessionJob::Teaching);wait();check(!calibrationChessboard_,"cancel suppresses corners");
    detectCalibrationChessboard();findChild<QDoubleSpinBox*>("chessboardSpacing")->setValue(11);wait();check(!calibrationChessboard_,"stale target worker suppressed");
    detectCalibrationChessboard();wait();check(calibrationChessboard_.has_value(),"detection recovered");
    importCalibrationTestImage(QDir(artifacts).filePath("missing-board.png"));check(!calibrationChessboard_ && !calibrationTestImage_,"reimport clears result immediately");wait();check(!calibrationTestImage_ && !calibrationChessboard_,"failed import no previous result");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("chessboard-import-error.png"));return true;
}
}

namespace visionflow {
bool MainWindow::verifyHomography(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto wait=[&]{QElapsedTimer timer;timer.start();while((editorSession_.busy() || runtimeJobs_.busy()) && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}check(!editorSession_.busy() && !runtimeJobs_.busy(),"H UI worker timeout");};
    // Reopen an offline study through the same worker commands as the buttons.
    const auto boardPath=QDir(artifacts).filePath("homography-board.png"),studyPath=QDir(artifacts).filePath("homography-test.vfcalstudy");check(chessboardFixture(true).save(boardPath),"board fixture");
    importCalibrationTestImage(boardPath);wait();findChild<QSpinBox*>("chessboardColumns")->setValue(9);findChild<QSpinBox*>("chessboardRows")->setValue(6);findChild<QDoubleSpinBox*>("chessboardSpacing")->setValue(10);findChild<QLineEdit*>("chessboardTargetId")->setText("H-board");detectCalibrationChessboard();wait();calculateCalibrationHomography();wait();check(calibrationMapping_.has_value(),"H study fitted");
    calibrationStudyAction("save",studyPath);wait();invalidateChessboard(true);calibrationStudyAction("load",studyPath);wait();check(calibrationTestImage_ && calibrationMapping_ && calibrationCandidate_ && !calibrationCandidate_->validated(),"UI offline candidate evidence reopen");check(!findChild<QPushButton*>("calibration_save")->isEnabled(),"offline study cannot bind production");
    QApplication::processEvents();grab().save(QDir(artifacts).filePath("homography-study-reopened.png"));
    const auto recipeId=service_.createRecipe("Homography measurement","",ApplicationType::Measurement,"file");const auto source=service_.addTool("image_file"),left=service_.addTool("edge"),right=service_.addTool("edge"),measurement=service_.addTool("measurement");for(auto t:service_.tools()){if(t.id==source)t.parameters["path"]=boardPath;if(t.id==right)t.parameters["direction"]="right_to_left";if(t.id==measurement){t.parameters["lowerMm"]=0;t.parameters["upperMm"]=100;}service_.updateTool(t);}service_.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",measurement,"p1"},{right,"point",measurement,"p2"}});refreshWorkspace();selectPage("calibration");calibrationLens_->setText("H-lens-declared");calibrationPlane_->setText("H-plane");findChild<QPushButton*>("calibrationSetup")->click();findChild<QPushButton*>("calibrationAcquire")->click();wait();check(editorSession_.frame.has_value() && !calibrationTestImage_,"real acquisition source frame");
    findChild<QLineEdit*>("chessboardTargetId")->setText("H-board");findChild<QDoubleSpinBox*>("chessboardSpacing")->setValue(10);detectCalibrationChessboard();wait();check(calibrationChessboard_.has_value(),"source-specific image coordinates detected");calculateCalibrationHomography();wait();check(calibrationMapping_.has_value(),"H raw acquisition fitted");
    calibrationId_->setText("ui-plane-H");findChild<QSpinBox*>("calibrationVersion")->setValue(1);findChild<QDoubleSpinBox*>("calibrationMaxError")->setValue(.1);
    std::vector<cv::Point2f> from{{50,50},{650,50},{650,470},{50,470}},to{{85,75},{590,40},{650,455},{40,480}};const auto imageTransform=cv::getPerspectiveTransform(from,to);
    std::vector<cv::Point2f> input{{150,150},{450,150},{210,130},{210,370}},projected;cv::perspectiveTransform(input,projected,imageTransform);
    {QSignalBlocker blocker(calibrationSamples_);calibrationSamples_->setRowCount(2);for(int r=0;r<2;++r){const QStringList values{r==0 ? "independent-x" : "independent-y",QString::number(projected[r*2].x,'g',12),QString::number(projected[r*2].y,'g',12),QString::number(projected[r*2+1].x,'g',12),QString::number(projected[r*2+1].y,'g',12),r==0 ? "50" : "40"};for(int c=0;c<6;++c)calibrationSamples_->setItem(r,c,new QTableWidgetItem(values[c]));}}
    refreshCalibration();findChild<QPushButton*>("calibration_validate")->click();wait();check(calibrationCandidate_ && calibrationCandidate_->mapping() && calibrationCandidate_->validated(),"UI H independent known physical grid holdouts");check(calibrationSamples_->item(0,6) && calibrationSamples_->item(0,7) && calibrationReport_,"actual residual columns and export enabled");exportCalibrationReport(QDir(artifacts).filePath("homography-evidence.csv"));wait();findChild<QScrollArea*>("calibrationScroll")->ensureWidgetVisible(calibrationStatus_);QApplication::processEvents();grab().save(QDir(artifacts).filePath("homography-validated.png"));
    findChild<QPushButton*>("calibration_save")->click();wait();check(service_.activeRecipe()->calibrationBinding && service_.activeRecipe()->calibrationBinding->modelType=="raw_planar_homography" && service_.dirty(),"UI H save/bind typed DRAFT");saveProject();service_.openProject(service_.path());service_.selectRecipe(recipeId);refreshWorkspace();selectPage("calibration");findChild<QPushButton*>("calibration_load")->click();wait();check(calibrationMapping_ && calibrationSamples_->rowCount()==2 && !calibrationCandidate_,"UI stored acquired evidence reopened without automatic new acceptance");
    QImage part(700,520,QImage::Format_Grayscale8);part.fill(0);{QPainter p(&part);p.fillRect(250,230,250,70,Qt::white);}check(part.save(boardPath),"part same source fixed plane");
    selectPage("editor");selectTool(measurement);runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->tools.last().state==ExecutionState::Done && editorSession_.cycle->tools.last().decision==PartDecision::Pass,"UI H actual Edge mm measurement");QApplication::processEvents();grab().save(QDir(artifacts).filePath("homography-measurement.png"));
    selectPage("recipes");refreshRecipes();for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==recipeId)recipeTable_->selectRow(row);findChild<QPushButton*>("loadRecipe")->click();findChild<QPushButton*>("runtime_run")->click();check(!runtimeJobs_.busy() && !runtimeResult_,"H Run without acquisition");findChild<QPushButton*>("runtime_cycle")->click();wait();check(runtimeResult_ && runtimeResult_->tools.last().decision==PartDecision::Pass,"UI H Runtime persisted PASS");runtimeTools_->setCurrentIndex(runtimeTools_->findData(measurement));runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("homography-runtime.png"));resize(850,600);QApplication::processEvents();runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("homography-runtime-small.png"));resize(1400,900);findChild<QPushButton*>("runtime_stop")->click();
    selectPage("calibration");calibrationLens_->setText("changed-H-lens");findChild<QPushButton*>("calibrationSetup")->click();check(!service_.activeRecipe()->calibrationBinding && !calibrationMapping_,"H setup invalidates binding and preview");selectPage("editor");runFlow();wait();check(editorSession_.cycle->tools.last().outputs.isEmpty(),"H invalid source no previous mm/PASS");saveProject();return true;
}
}
