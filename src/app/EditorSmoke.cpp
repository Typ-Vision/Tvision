#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLockFile>
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
#include <QThread>
#include <QSpinBox>
namespace visionflow {
bool MainWindow::verifyRuntime(const QString& artifacts) {
    auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    auto wait=[&]{QElapsedTimer timer;timer.start();while(runtimeJobs_.busy() && timer.elapsed()<10000){QApplication::processEvents();QThread::msleep(1);}check(!runtimeJobs_.busy(),"Runtime worker completed");};
    service_.setCalibrationSetup("fixture-lens","fixture-plane");
    const auto frame=RuntimeService::probe(*service_.activeRecipe(),QDir(artifacts).absolutePath(),{});
    CalibrationWorkflow::bind(service_,frame,service_.path()+".calibrations.json","fixture-scale");
    QString measurement;for(auto t:service_.tools())if(t.kind=="measurement"){measurement=t.id;t.parameters["upperMm"]=9.98;service_.updateTool(t);}service_.save();refreshWorkspace();selectPage("runtime");
    check(!runtimeResult_ && !runtimeViewer_->hasImage() && !runtimeSummary_->text().contains("PASS"),"Runtime initial no invented result");
    findChild<QPushButton*>("runtime_validate")->click();check(runtimeJobs_.busy(),"Runtime validation acquisition worker");check(!close(),"Runtime close guard");wait();check(runtime_->validation().has_value(),"Runtime validation accepted");workspaceError_->hide();
    check(service_.activeRecipe()->status==RecipeStatus::Draft,"validation separate from recipe disk status");findChild<QPushButton*>("runtime_activate")->click();check(runtime_->active()!=nullptr && !runtimeResult_,"Runtime activate no fabricated PASS");const auto first=runtime_->active()->validation().id;
    findChild<QPushButton*>("runtime_run")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && runtimeSummary_->text().contains("9.975000 mm • PASS"),"Runtime real calibrated PASS");runtimeTools_->setCurrentIndex(runtimeTools_->findData(measurement));runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-pass.png"));
    resize(850,600);QApplication::processEvents();runtimeViewer_->fitImage();QApplication::processEvents();check(width()==850 && height()==600,"populated Runtime minimum size");grab().save(QDir(artifacts).filePath("runtime-pass-small.png"));resize(1400,900);QApplication::processEvents();runtimeViewer_->fitImage();
    // A draft tolerance edit cannot alter the active worker's immutable snapshot.
    findChild<QPushButton*>("runtime_run")->click();for(auto t:service_.tools())if(t.id==measurement){t.parameters["upperMm"]=9.9749;service_.updateTool(t);}refreshWorkspace();wait();check(runtimeResult_ && runtimeSummary_->text().contains("PASS") && runtime_->active()->validation().id==first,"draft edit preserves active result");
    selectPage("runtime");findChild<QPushButton*>("runtime_validate")->click();wait();findChild<QPushButton*>("runtime_activate")->click();check(runtime_->active()->validation().id!=first && !runtimeResult_,"new activation clears old result");
    findChild<QPushButton*>("runtime_run")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done && runtimeSummary_->text().contains("9.975000 mm • FAIL"),"Runtime raw tolerance FAIL");runtimeTools_->setCurrentIndex(runtimeTools_->findData(measurement));runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-fail.png"));
    findChild<QPushButton*>("runtime_run")->click();findChild<QPushButton*>("runtime_stop")->click();wait();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Skipped && !runtimeSummary_->text().contains("mm •") && !runtimeViewer_->hasImage(),"Runtime Stop clears image and decision");
    findChild<QPushButton*>("runtime_run")->click();QLockFile lock(service_.path()+".production/archive.lock");lock.setStaleLockTime(0);check(lock.tryLock(),"UI log lock fixture");wait();check(runtimePending_ && runtime_->busy() && !runtimeResult_ && !findChild<QPushButton*>("runtime_run")->isEnabled() && !runtimeSummary_->text().contains("mm •"),"UI failed log suppresses result blocks next cycle");QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-record-pending.png"));lock.unlock();findChild<QPushButton*>("runtime_retry")->click();check(!runtimePending_ && !runtime_->busy() && runtimeResult_ && runtimeSummary_->text().contains("FAIL"),"UI retry same result accepted");
    runtimeHistory_->setCurrentIndex(runtimeHistory_->findData(first));findChild<QPushButton*>("runtime_rollback")->click();check(runtime_->active()->validation().id==first && !runtimeResult_,"UI explicit rollback at cycle boundary");
    selectPage("logs");check(logTable_->rowCount()>=5,"persistent Logs rows");logTable_->selectRow(0);QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-logs.png"));
    selectPage("runtime");service_.setCalibrationSetup("physical lens changed","fixture-plane");refreshWorkspace();findChild<QPushButton*>("runtime_run")->click();check(!runtimeJobs_.busy() && !runtimeResult_ && runtimeSummary_->text().contains("kurulum değişti"),"active setup invalidation blocks acquisition");QApplication::processEvents();grab().save(QDir(artifacts).filePath("runtime-invalid.png"));
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
    toolbox_->setCurrentRow(3);findChild<QPushButton*>("addTool")->click();
    const auto temporary=selectedToolId_;findChild<QPushButton*>("removeTool")->click();
    check(selectedToolId_.isEmpty(),"delete clears selected tool");
    for(const auto& tool:service_.tools())check(tool.id!=temporary,"UI temporary tool removed");
    selectTool(distance);runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"run after UI add/delete");service_.save();
    findChild<QTabWidget*>("editorTabs")->setCurrentIndex(0);QApplication::processEvents();grab().save(QDir(artifacts).filePath("step5-flow.png"));
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
    if (!editorSession_.frame || service_.tools().size()<2) return false;
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
    if (!wait() || service_.tools()[1].parameters["modelPng"].toString().isEmpty()) return false;
    beginRoi(false); commitRoi(QRect(650,600,512,512)); imageViewer_->drawRegion(false);
    // A preview alone is not a DAG image input. Run Match must reject it.
    runMatch();if(!wait() || editorSession_.matchOutcome || !editorSession_.cycle || editorSession_.cycle->state!=ExecutionState::Error)return false;
    auto* imageInput=propertiesBody_->findChild<QComboBox*>("input_image");if(!imageInput)return false;
    const auto sourceId=service_.tools()[0].id;const auto inputIndex=imageInput->findData(sourceId+"/image");if(inputIndex<1)return false;
    imageInput->setCurrentIndex(inputIndex);
    runMatch(); close(); if (!isVisible()) return false;
    if (!wait() || !editorSession_.matchOutcome || !editorSession_.matchOutcome->accepted || editorSession_.matchOutcome->rectangle.topLeft()!=QPoint(700,650)) return false;
    if(!editorSession_.references.contains(id) || editorSession_.references[id].second!=QPointF(731.5,681.5))return false;
    if(!editorSession_.cycle || editorSession_.cycle->tools.size()!=2 || editorSession_.cycle->state!=ExecutionState::Done)return false;
    // Run Match reacquires the configured source; a stale preview is never input.
    const auto fixturePath=editorSession_.frame->path();QByteArray fixtureBytes;
    {QFile file(fixturePath);if(!file.open(QIODevice::ReadOnly))return false;fixtureBytes=file.readAll();}
    QImage blank(editorSession_.frame->size(),QImage::Format_Grayscale8);blank.fill(0);if(!blank.save(fixturePath))return false;
    runMatch();if(!wait() || !editorSession_.matchOutcome || editorSession_.matchOutcome->accepted || !editorSession_.references.isEmpty())return false;
    {QFile file(fixturePath);if(!file.open(QIODevice::WriteOnly) || file.write(fixtureBytes)!=fixtureBytes.size())return false;}
    runMatch();if(!wait() || !editorSession_.matchOutcome || !editorSession_.matchOutcome->accepted)return false;
    runMatch();findChild<QPushButton*>("stopFlow")->click();
    if(!wait() || editorSession_.matchOutcome || !editorSession_.cycle || editorSession_.cycle->state!=ExecutionState::Skipped)return false;
    // A result from a previous configuration must never be published.
    runMatch(); auto tool=service_.tools()[1]; tool.parameters["minScore"]=0.99; service_.updateTool(tool); refreshWorkspace();
    if (!wait() || editorSession_.matchOutcome) return false;
    service_.save(); const auto recipe=service_.activeRecipe()->id;
    service_.openProject(service_.path()); service_.selectRecipe(recipe); refreshWorkspace(); selectTool(id);
    runMatch(); if (!wait() || !editorSession_.matchOutcome || !editorSession_.matchOutcome->accepted) return false;
    imageViewer_->fitImage(); QApplication::processEvents(); grab().save(QDir(artifacts).filePath("step4-editor.png"));
    auto source=service_.tools()[0]; source.enabled=false; service_.updateTool(source); refreshWorkspace();
    if (editorSession_.matchOutcome || editorSession_.frame) return false;
    source.enabled=true; service_.updateTool(source); service_.save(); refreshWorkspace();
    return service_.activeRecipe()->status==RecipeStatus::Draft && !service_.dirty();
}
}
