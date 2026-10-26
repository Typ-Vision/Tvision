#include "application/ProductionResultCodec.hpp"
#include "application/ExperimentalRuntime.hpp"
#include "application/EditorObservation.hpp"
#include <QProcessEnvironment>
#include <QProcess>
#include <QRegularExpression>
#include "application/ToolResultCodec.hpp"
#include "CameraFlowSmokeAudit.hpp"
#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include "Toolbox.hpp"
#include "WorkspacePanel.hpp"
#include "application/ExperimentalOecsAcquisition.hpp"
#include "domain/ToolRoi.hpp"
#include <QJsonDocument>
#include <QTextStream>
#include <QAction>
#include <QDialog>
#include <QEnterEvent>
#include <QSet>
#include <QLineEdit>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QJsonArray>
#include <QListWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QThread>
#include <QTabWidget>
#include <QToolButton>
#include <QTimer>
#include <QTableWidget>
#include <QLockFile>
#include <QUuid>
#include <stdexcept>
#include <algorithm>

namespace visionflow {
bool MainWindow::verifyExperimentalEditor(const QString& artifacts) {
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    auto wait=[&]{QElapsedTimer timer;timer.start();do{QApplication::processEvents();QThread::msleep(1);}while(editorSession_.busy() && timer.elapsed()<20000);QApplication::processEvents();check(!editorSession_.busy(),"experimental Editor worker timeout");};
    const auto path=qEnvironmentVariable("VISIONFLOW_EXP_RASTER");check(!path.isEmpty(),"VISIONFLOW_EXP_RASTER required");
    auto* policy=findChild<QCheckBox*>("experimentalEditorReplay");check(!policy,"restart policy defaults off");
    const auto raw=ImageFileService{}.load(path,true);
    service_.createProject("Experimental raster","",QDir(artifacts).filePath("experimental-editor-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".vfproject"),"file");
    service_.createRecipe("Real raster px","",ApplicationType::Measurement,"file");
    const auto source=service_.setImageFile(path);service_.enableMultiSource();
    const auto line=service_.addTool("line_fit"),circle=service_.addTool("circle_fit");
    for(auto t:service_.tools())if(t.id==line || t.id==circle){
        t.parameters["mode"]="pixel";t.parameters["polarity"]=t.id==line ? "falling":"rising";
        // The real top border's raw central-difference peaks are 14.5–18.5
        // Gray8/px at x=900..1500; default 20 rejects every scan. Keep all fit
        // quality/tolerance defaults and explicitly select edge threshold 10.
        if(t.id==line)t.parameters["minContrast"]=10.;
        t.parameters["scanRoi"]=(t.id==line ? ToolRoi{{source+"/image"},RotatedRectangle{{1200,645},{600,45},0}}:ToolRoi{{source+"/image"},Annulus{{1200,975},22,52}}).toJson();
        service_.updateTool(t);
    }
    service_.setConnections({{source,"image",line,"image"},{source,"image",circle,"image"}});
    refreshWorkspace();selectPage("editor");selectTool(circle);wait();runFlow();wait();
    if(!editorSession_.cycle || editorSession_.cycle->state!=ExecutionState::Done){QString error=editorSession_.cycle?editorSession_.cycle->error:"No cycle";if(editorSession_.cycle)for(const auto& t:editorSession_.cycle->tools)error+="\n"+t.toolId+": "+t.message;throw std::runtime_error(("real raster flow: "+error).toStdString());}
    auto result=[&](const QString& id)->ToolResult{for(const auto& r:editorSession_.cycle->tools)if(r.toolId==id)return r;throw std::runtime_error("missing real measurement");};
    const auto l=result(line),c=result(circle);
    check(l.outputs["linePx"].valid && c.outputs["circlePx"].valid,"actual Line/Circle pixel fits");
    check(!l.outputs["lineMm"].valid && !c.outputs["circleMm"].valid && l.decision==PartDecision::NotEvaluated && c.decision==PartDecision::NotEvaluated,"no mm or part acceptance without reference");
    for(const auto& r:{l,c})for(const auto& v:r.outputs)check(v.origin.experimentalAcquisition==raw.metadata().experimentalAcquisition,"measurement provenance retained");
    check(!findChild<QLabel*>("experimentalEditorNotice") && !imageInfo_->text().contains(ExperimentalOecsAcquisition::notice()) && !matchInfo_->text().contains(ExperimentalOecsAcquisition::notice()),"persistent image/result notice");
    QFile report(QDir(artifacts).filePath("real-raster-report.txt"));check(report.open(QIODevice::WriteOnly),"report open");QTextStream out(&report);out<<"Real saved experimental raster only; no physical mm oracle\nLine edge threshold explicitly 20 -> 10 after default rejected 0/21; observed raw peaks 14.5..18.5 Gray8/px. Fit/tolerance defaults unchanged.\n"<<path<<"\ncontentId="<<raw.contentId().toHex()<<"\n"<<matchInfo_->text()<<"\n";out.flush();
    const auto cycle=editorSession_.cycle->cycleId;runExecution(circle);wait();
    check(editorSession_.cycle && editorSession_.cycle->partial && editorSession_.cycle->cycleId==cycle && result(circle).outputs["circlePx"].valid,"real TargetOnly without acquisition");
    service_.save();const auto project=service_.path(),recipe=service_.activeRecipe()->id;const auto version=service_.activeRecipe()->recipeVersion;const auto definition=service_.activeRecipe()->definition;
    service_.openProject(project);service_.selectRecipe(recipe);service_.save();check(service_.activeRecipe()->recipeVersion==version && service_.activeRecipe()->definition==definition,"save reopen no-op");
    refreshWorkspace();selectTool(circle);wait();runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"reopen real flow");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();imageViewer_->fitImage();QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("experimental-editor-%1.png").arg(size.width()))),"experimental screenshot");}
    {
        auto* runtimePolicy=findChild<QCheckBox*>("experimentalRuntime");check(!runtimePolicy,"Runtime restart policy off");ensureRuntime();
        runtimeCoordinator_->load(*service_.activeRecipe(),QFileInfo(project).absolutePath());selectPage("runtime");runtimeAction("run");check(runtimeCoordinator_->state()==RuntimeState::Running,"experimental Runtime prepared");runtimeAction("cycle");
        QElapsedTimer timer;timer.start();while(runtimeJobs_.busy() && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done,"experimental Runtime replay completion");
        runtimeTools_->setCurrentIndex(runtimeTools_->findData(circle));check(!runtimeSummary_->text().contains(ExperimentalOecsAcquisition::notice()) && runtimeViewer_->hasImage(),"Runtime image/result notice");
        const auto visibilityResult=*runtimeResult_;const auto visibilityRecords=runtime_->records();const auto visibilityCsv=ExperimentalRuntime::csv(visibilityRecords);const auto visibilitySnapshot=runtime_->active();
        for(const auto& tool:editorTools(visibilitySnapshot->recipe().definition))check(outputPreferences_.save({runtimeProjectId_,visibilitySnapshot->recipe().id,tool.id},QSet<QString>{}).ok(),"INT07 experimental empty preferences");
        showRuntimeResult();check(runtimeSummary_->text().contains("Sonuç çıkışları gizli") && !runtimeSummary_->text().contains("valuePx:") && !runtimeSummary_->text().contains("diameterPx:") && !runtimeSummary_->text().contains("widthPx:") && runtimeSummary_->text().contains("Parça: Değerlendirilmedi") && runtimeViewer_->hasImage(),"INT07 experimental headline and list hide scalars, retain state and viewer");
        for(const auto& tool:editorTools(visibilitySnapshot->recipe().definition))check(outputPreferences_.showAll({runtimeProjectId_,visibilitySnapshot->recipe().id,tool.id},tool.kind).ok(),"INT07 experimental show all");
        showRuntimeResult();check(runtimeSummary_->text().contains("diameterPx:") && RuntimeService::sameCompletion(visibilityResult,*runtimeResult_) && visibilitySnapshot==runtime_->active() && visibilityRecords==runtime_->records() && visibilityCsv==ExperimentalRuntime::csv(runtime_->records()),"INT07 experimental visibility preserves immutable completion terminal and CSV");
        setWindowState(Qt::WindowNoState);for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();runtimeViewer_->fitImage();QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("experimental-runtime-%1.png").arg(size.width()))),"Runtime screenshot");}
        runtimeAction("stop");check(!runtimeResult_ && !runtimeViewer_->hasImage(),"Runtime Stop clears image");selectPage("logs");refreshLogs();check(logTable_->rowCount()==1 && !logTable_->item(0,2)->text().contains("Deneysel"),"experimental Logs row");logTable_->selectRow(0);QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath("experimental-logs-850.png")),"Logs screenshot");
        selectPage("runtime");runtimeAction("run");QLockFile lock(project+".production/archive.lock");check(lock.tryLock(0),"Runtime archive fixture lock");runtimeAction("cycle");timer.restart();while(runtimeJobs_.busy() && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();check(runtimePending_ && !runtimeResult_ && !runtimeViewer_->hasImage(),"failed recording does not publish");runtimeAction("stop");lock.unlock();runtimeAction("retry");check(!runtimePending_ && !runtimeResult_ && !runtimeViewer_->hasImage() && runtimeCoordinator_->state()==RuntimeState::LoadedStopped,"record retry after Stop must not restore live pixels/results");

    }
    // Bad calibration leaves pixel diagnostics but cannot create physical values.
    for(auto t:service_.tools())if(t.id==circle){t.parameters["mode"]="calibrated";t.parameters["scanRoi"]=ToolRoi{{"plane",LengthUnit::Millimetre,AxisConvention::XRightYUp},Annulus{{12,10},2,5}}.toJson();service_.updateTool(t);}
    refreshWorkspace();selectTool(circle);runFlow();wait();const auto invalid=result(circle);
    check(!invalid.outputs.value("circleMm").valid && invalid.decision!=PartDecision::Pass,"missing calibration cannot pass");
    editorSession_.sourceFrames.clear();editorSession_.frame.reset();editorSession_.invalidateResults();
    const auto automatic=ExecutionService{}.run(definition,QFileInfo(project).absolutePath(),recipe,{},{});
    check(automatic.state==ExecutionState::Done,"normal execution selects saved experimental replay automatically");
    for(const auto& tool:automatic.tools){check(tool.decision==PartDecision::NotEvaluated,"automatic replay never grants a part decision");for(const auto& value:tool.outputs)if(value.valid)check(value.origin.experimentalAcquisition==raw.metadata().experimentalAcquisition && value.port.unit!="mm","automatic replay retains experimental origin without physical calibration");}
    {MainWindow restarted(settingsFile_);check(!restarted.findChild<QCheckBox*>("experimentalEditorReplay") && !restarted.findChild<QCheckBox*>("experimentalRuntime"),"restart has no obsolete experimental opt-in");}
    out<<"TargetOnly, Save/reopen/no-op, invalid calibration, automatic replay provenance and stopped restart passed\n";out.flush();
    return true;
}
bool MainWindow::verifyEditorUsability(const QString& artifacts) {
    if(!qEnvironmentVariableIsSet("VISIONFLOW_D3_RESTART_PROJECT") && !verifyEditorPropertyCommits(artifacts))return false;
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto wait=[&]{QElapsedTimer timer;timer.start();do{QApplication::processEvents();QThread::msleep(1);}while(editorSession_.busy() && timer.elapsed()<20000);QApplication::processEvents();check(!editorSession_.busy(),"usability worker deadline");};
    if(qEnvironmentVariableIsSet("VISIONFLOW_D3_RESTART_PROJECT")) {
        service_.openProject(qEnvironmentVariable("VISIONFLOW_D3_RESTART_PROJECT"));service_.selectRecipe(qEnvironmentVariable("VISIONFLOW_D3_RESTART_RECIPE"));refreshWorkspace();selectPage("editor");wait();selectTool(qEnvironmentVariable("VISIONFLOW_D3_RESTART_TOOL"));
        const auto boxes=propertiesBody_->findChildren<QCheckBox*>(QRegularExpression("^outputVisible_"));check(!boxes.isEmpty(),"restart rebuilds actual output checkboxes");for(auto* box:boxes)check(!box->isChecked(),"restart preserves explicit empty selection");check(!editorSession_.cycle && !editorSession_.busy() && !service_.dirty(),"restart does not run or dirty");return true;
    }
    {QSettings preferences(settingsFile_,QSettings::IniFormat);preferences.remove("properties");}
    {MainWindow firstUse(QDir(artifacts).filePath("annotation-first-use-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".ini"));check(!firstUse.findChild<QCheckBox*>("showResultAnnotations")->isChecked() && !firstUse.imageViewer_->resultAnnotationsVisible(),"first use defaults annotations off");}
    auto* annotations=findChild<QCheckBox*>("showResultAnnotations");check(annotations,"Editor annotation checkbox");
    check(annotations->isChecked()==QSettings(settingsFile_,QSettings::IniFormat).value("editor/showResultAnnotations",false).toBool(),"annotation preference loaded");
    annotations->setChecked(false);
    QImage image(256,256,QImage::Format_Grayscale8);image.fill(20);for(int y=80;y<160;++y)for(int x=0;x<256;++x)image.scanLine(y)[x]=220;
    const auto path=QDir(artifacts).filePath("usability-source.png");check(image.save(path),"usability PNG");
    service_.createProject("Usability","",QDir(artifacts).filePath("usability-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".vfproject"),"file");service_.createRecipe("Usability","",ApplicationType::Measurement,"file");
    const auto source=service_.setImageFile(path);refreshWorkspace();selectPage("editor");wait();
    // CCUX-24B: a single selection model survives every left tab and execution.
    {
        auto* tabs=findChild<QTabWidget*>("editorTabs");auto* bottom=findChild<QSplitter*>("editorBottomV1");
        check(bottom && bottom->count()==2 && !tabs->isAncestorOf(flowTools_),"Flow is outside left tabs");
        selectTool(source);
        for(const auto size:{QSize(1400,900),QSize(850,600)}){
            resize(size);QApplication::processEvents();
            for(int tab=0;tab<3;++tab){tabs->setCurrentIndex(tab);QApplication::processEvents();check(flowTools_->isVisible() && flowTools_->width()>=200 && selectedToolId_==source,"Flow survives tab and size changes");}
            runFlow();wait();check(flowTools_->isVisible() && selectedToolId_==source,"Run Flow preserves visible selection");
            runExecution(source);wait();check(flowTools_->isVisible() && selectedToolId_==source,"Run Tool preserves visible selection");
            findChild<QPushButton*>("stopFlow")->click();check(flowTools_->isVisible() && selectedToolId_==source,"Stop preserves visible selection");
            CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("ccux24b-flow-%1.png").arg(size.width())));
        }
        QSettings settings(settingsFile_,QSettings::IniFormat);settings.setValue("editor/bottomV1",QByteArray("corrupt"));settings.sync();restoreLayout();
        check(bottom->sizes()[0]>0 && bottom->sizes()[1]>=200,"corrupt splitter safely falls back");
        bottom->setSizes({1,900});saveLayout();restoreLayout();check(bottom->sizes()[0]>=120 && bottom->sizes()[1]>=200,"narrow splitter safely falls back");
        resize(1400,900);
    }
    // EUI-06: input events exercise timed reveal, focus retention and Escape.
    {
        service_.save();refreshWorkspace(); // Navigation presentation uses a clean recipe; dirty exits have their own lifecycle fixture.
        auto pause=[](int ms){QElapsedTimer t;t.start();while(t.elapsed()<ms){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();};
        auto click=[](QWidget* w){const QPointF p=w->rect().center();for(auto type:{QEvent::MouseButtonPress,QEvent::MouseButtonRelease}){QMouseEvent e(type,p,QPointF(w->mapToGlobal(p.toPoint())),Qt::LeftButton,type==QEvent::MouseButtonPress ? Qt::LeftButton:Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(w,&e);}};
        workspacePanel_->setPinned(true);QApplication::processEvents();
        const auto definition=service_.activeRecipe()->definition;const auto signature=flowSignature();const auto generation=editorSession_.generation;
        const auto page=pages_->currentIndex();const auto selected=selectedToolId_;
        auto* pin=findChild<QPushButton*>("workspacePin");auto* rail=findChild<QPushButton*>("workspaceReveal");
        click(pin);QApplication::processEvents();check(!workspacePanel_->pinned() && workspacePanel_->width()==28 && !workspacePanel_->expanded(),"pin input enters collapsed auto-hide");
        const auto viewport=imageViewer_->viewport()->size();
        QEnterEvent enter(QPointF(10,10),QPointF(10,10),QPointF(10,10));QApplication::sendEvent(rail,&enter);pause(70);check(!workspacePanel_->expanded(),"hover reveal delayed");pause(120);check(workspacePanel_->expanded(),"hover reveals after delay");
        check(imageViewer_->viewport()->size()==viewport,"floating navigation does not resize viewer");
        navigation_->setFocus();QEvent leave(QEvent::Leave);QApplication::sendEvent(rail,&leave);QApplication::sendEvent(findChild<QWidget*>("sidebar"),&leave);pause(400);check(workspacePanel_->expanded(),"keyboard focus retains panel");
        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(navigation_,&escape);check(!workspacePanel_->expanded() && rail->hasFocus(),"Escape closes and returns rail focus");
        QKeyEvent space(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier),release(QEvent::KeyRelease,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(rail,&space);QApplication::sendEvent(rail,&release);check(workspacePanel_->expanded(),"keyboard rail opens navigation");
        imageViewer_->setFocus();QDialog dialog(this);dialog.setModal(true);dialog.show();pause(400);check(workspacePanel_->expanded(),"modal retains navigation");dialog.hide();
        QMenu menu(this);menu.addAction("Test");menu.popup(mapToGlobal(QPoint(400,200)));pause(400);check(workspacePanel_->expanded(),"popup retains navigation");menu.close();
        imageViewer_->setFocus();QApplication::sendEvent(findChild<QWidget*>("sidebar"),&leave);pause(180);check(workspacePanel_->expanded(),"exit delay avoids boundary flicker");pause(230);check(!workspacePanel_->expanded(),"exit collapses after delay");
        {MainWindow reopened(settingsFile_);check(!reopened.workspacePanel_->pinned(),"restart restores local auto-hide preference");}
        QFile metrics(QDir(artifacts).filePath("workspace-viewports.txt"));check(metrics.open(QIODevice::WriteOnly),"viewport metrics file");
        for(const auto size:{QSize(1400,900),QSize(850,600)})for(bool pinned:{true,false}){
            resize(size);workspacePanel_->setPinned(pinned);horizontal_->setSizes({180,680,300});vertical_->setSizes({540,220});for(int i=0;i<3;++i)QApplication::processEvents();imageViewer_->fitImage();
            auto* editorPage=findChild<QWidget*>("editorPage");
            if(editorPage)check(editorPage->rect().contains(QRect(editorContext_->mapTo(editorPage,QPoint()),editorContext_->size())),"header status fits pinned and auto-hide layouts");
            const auto v=imageViewer_->viewport()->size();metrics.write(QString("%1 %2 %3x%4\n").arg(size.width()).arg(pinned ? "pinned":"auto").arg(v.width()).arg(v.height()).toUtf8());
            CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("workspace-%1-%2.png").arg(size.width()).arg(pinned ? "pinned":"auto")));
            if(!pinned){workspacePanel_->reveal();QApplication::processEvents();check(imageViewer_->viewport()->size()==v,"overlay maintains viewport at each size");CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("workspace-%1-overlay.png").arg(size.width())));}
        }
        selectPage("calibration");check(pages_->currentIndex()==pageIds_.indexOf("calibration"),"auto-hide shared by Calibration");selectPage("runtime");selectPage("editor");
        findChild<QAction*>("resetPanels")->trigger();QApplication::processEvents();check(workspacePanel_->pinned(),"Reset restores pinned mode");
        check(service_.activeRecipe()->definition==definition && flowSignature()==signature && editorSession_.generation==generation && pages_->currentIndex()==page && selectedToolId_==selected,"navigation presentation preserves recipe execution and selection");
        resize(1400,900);
    }
    // EUI-07: long names, secondary commands and constrained layouts.
    {
        const auto recipe=*service_.activeRecipe();
        service_.editRecipe(recipe.id,QString(64,'W'),recipe.description,recipe.cameraReference,recipe.calibrationReference);refreshWorkspace();wait();
        const auto definition=service_.activeRecipe()->definition;const auto signature=flowSignature();
        auto* more=findChild<QPushButton*>("editorMore");auto* viewerMore=findChild<QToolButton*>("viewerMore");
        auto keyClick=[](QWidget* w){w->setFocus();QKeyEvent press(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier),release(QEvent::KeyRelease,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(w,&press);QApplication::sendEvent(w,&release);};
        for(const auto size:{QSize(1400,900),QSize(850,600)}) {
            resize(size);QApplication::processEvents();
            check(width()==size.width() && height()==size.height() && !pageTitle_->isVisible() && !context_->isVisible(),"compact Editor fits exact window and hides duplicate shell heading");
            auto* editorPage=findChild<QWidget*>("editorPage");
            for(const auto name:{"saveWorkspace","runFlow","runTool","stopFlow","showResultAnnotations","editorMore"}) {
                auto* command=editorPage->findChild<QWidget*>(name);const QRect bounds(command->mapTo(editorPage,QPoint()),command->size());
                const auto* button=qobject_cast<QPushButton*>(command);
                const int readable=button ? button->fontMetrics().horizontalAdvance(button->text())+14:command->sizeHint().width();
                if(!command->isVisible() || !editorPage->rect().contains(bounds) || command->width()<readable)throw std::runtime_error(QString("command %1 width %2 readable %3 x %4 page %5 at %6").arg(name).arg(command->width()).arg(readable).arg(bounds.x()).arg(editorPage->width()).arg(size.width()).toStdString());
            }
            check(editorContext_->text().contains("DRAFT") && editorContext_->text().contains("Kaydedilmedi"),"long recipe name cannot hide draft/dirty status");
            keyClick(more);QApplication::processEvents();check(findChild<QPushButton*>("editActiveRecipe")->isVisible() && findChild<QPushButton*>("enableMultiSource")->isVisible(),"keyboard reveals secondary recipe controls");
            CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("compact-details-%1.png").arg(size.width())));keyClick(more);
            auto* menu=viewerMore->findChild<QMenu*>();bool opened=false;
            QTimer::singleShot(50,this,[&]{auto* reload=findChild<QAction*>("reloadImage");opened=menu->isVisible() && menu->activeAction()==reload && findChild<QAction*>("roiGeometry")->isVisible();
                QKeyEvent down(QEvent::KeyPress,Qt::Key_Down,Qt::NoModifier);QApplication::sendEvent(menu,&down);opened=opened && menu->activeAction()==findChild<QAction*>("zoomOut");
                menu->grab().save(QDir(artifacts).filePath(QString("compact-viewer-menu-%1.png").arg(size.width())));
                const auto zoom=imageViewer_->zoomFactor();QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(menu,&enter);opened=opened && imageViewer_->zoomFactor()<zoom;menu->close();});
            keyClick(viewerMore);QElapsedTimer menuWait;menuWait.start();while(menuWait.elapsed()<100){QApplication::processEvents();QThread::msleep(1);}
            check(opened,"keyboard opens viewer overflow commands");
            CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("compact-long-name-%1.png").arg(size.width())));
        }
        check(service_.activeRecipe()->definition==definition && flowSignature()==signature,"layout and menus preserve recipe and execution identity");
        service_.editRecipe(recipe.id,recipe.name,recipe.description,recipe.cameraReference,recipe.calibrationReference);
        // Different application types still use this exact Editor page.
        for(const auto type:{ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement}) {
            service_.createRecipe("Compact type fixture "+QString::number(int(type)),"",type,"file");refreshWorkspace();selectPage("editor");QApplication::processEvents();
            check(pages_->currentWidget()==findChild<QWidget*>("editorPage") && editorContext_->text().contains(typeLabel(type)),"three application types share compact Editor");
            CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("compact-empty-%1.png").arg(int(type))));
        }
        service_.selectRecipe(recipe.id);refreshWorkspace();wait();resize(1400,900);
        runFlow();check(editorSession_.busy() && findChild<QTabWidget*>("editorTabs")->currentWidget()==findChild<QScrollArea*>("resultsScroll"),"actual worker busy state remains visible in Results");
        CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath("compact-busy.png"));wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"compact fixture execution produces real DONE");
        // Reopen an extreme older splitter state using a dedicated test INI.
        horizontal_->setSizes({9000,1,9000});vertical_->setSizes({1,9000});
        const auto legacyIni=QDir(artifacts).filePath("compact-old-layout.ini");
        {QSettings old(legacyIni,QSettings::IniFormat);old.setValue("editor/horizontal",horizontal_->saveState());old.setValue("editor/vertical",vertical_->saveState());}
        {MainWindow old(legacyIni);old.resize(850,600);old.selectPage("editor");old.show();QApplication::processEvents();
            check(old.size()==QSize(850,600) && old.imageViewer_->height()>=80 && old.findChild<QScrollArea*>("propertiesScroll")->width()>=210,"old splitter preferences constrained to usable minimums");
            CameraFlowSmokeAudit::save(old,QDir(artifacts).filePath("compact-old-layout.png"));}
        findChild<QAction*>("resetPanels")->trigger();QApplication::processEvents();
    }
    // EUI-05: group headers are presentation, never tool kinds.
    {QSet<QString> kinds;QListWidgetItem *measurementGroup=nullptr,*lineEntry=nullptr;int groups=0;
        for(int i=0;i<toolbox_->count();++i){auto* item=toolbox_->item(i);const auto kind=item->data(Qt::UserRole).toString();
            if(item->data(Toolbox::HeaderRole).toBool()){++groups;if(item->data(Toolbox::GroupRole)=="measurement")measurementGroup=item;check(kind.isEmpty(),"header has no kind");}
            else {check(!kinds.contains(kind),"catalog kind appears exactly once");kinds.insert(kind);check(item->data(Toolbox::GroupRole)==ToolCatalog::groupForKind(kind),"kind assigned to expected group");if(kind=="line_fit")lineEntry=item;}}
        check(groups==6 && kinds.size()==ToolCatalog::entries().size() && !kinds.contains("edge") && kinds.contains("edge_point"),"semantic catalog contents without retired Edge");
        const auto definition=service_.activeRecipe()->definition;const auto signature=flowSignature();const auto generation=editorSession_.generation;
        auto key=[&](int code){QKeyEvent e(QEvent::KeyPress,code,Qt::NoModifier);QApplication::sendEvent(toolbox_,&e);};
        toolbox_->setCurrentItem(measurementGroup);check(!findChild<QPushButton*>("addTool")->isEnabled(),"Add Tool disabled for header");addSelectedTool();
        key(Qt::Key_Left);check(lineEntry->isHidden(),"keyboard collapses group");
        auto* search=findChild<QLineEdit*>("toolboxSearch");search->setText("Line Fit");check(!lineEntry->isHidden() && !measurementGroup->isHidden(),"search temporarily opens matching group");
        check(!QSettings(settingsFile_,QSettings::IniFormat).value("editor/toolboxGroups/measurement",true).toBool(),"search does not overwrite collapse preference");
        search->clear();check(lineEntry->isHidden(),"cleared search restores collapsed group");
        {Toolbox reopened(settingsFile_);bool hidden=false;for(int i=0;i<reopened.count();++i)if(reopened.item(i)->data(Qt::UserRole)=="line_fit")hidden=reopened.item(i)->isHidden();check(hidden,"group preference survives new widget");}
        toolbox_->setCurrentItem(lineEntry);check(!findChild<QPushButton*>("addTool")->isEnabled(),"hidden tool cannot be added");addSelectedTool();
        toolbox_->setCurrentItem(measurementGroup);key(Qt::Key_Right);check(!lineEntry->isHidden(),"keyboard reopens group");
        search->setText("no_such_tool");for(int i=0;i<toolbox_->count();++i)check(toolbox_->item(i)->isHidden(),"no-match filter hides groups and children");search->clear();
        check(service_.activeRecipe()->definition==definition && flowSignature()==signature && editorSession_.generation==generation,"group and search preserve recipe and execution identity");
        for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);horizontal_->setSizes({190,750,300});QApplication::processEvents();toolbox_->scrollToTop();imageViewer_->fitImage();CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("eui05-toolbox-%1.png").arg(size.width())));}
        for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Toolbox::HeaderRole).toBool()){toolbox_->setCurrentRow(i);key(Qt::Key_Left);}
        resize(1400,900);QApplication::processEvents();toolbox_->scrollToTop();CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath("eui05-groups-collapsed-1400.png"));
        search->setText("Line Fit");for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();toolbox_->scrollToTop();CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("eui05-search-%1.png").arg(size.width())));}search->clear();
        for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Toolbox::HeaderRole).toBool()){toolbox_->setCurrentRow(i);key(Qt::Key_Right);}

    }
    {QListWidgetItem* entry=nullptr;for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="calculate")entry=toolbox_->item(i);
        check(entry,"double-click catalog entry");toolbox_->scrollToItem(entry);QApplication::processEvents();
        const auto point=toolbox_->visualItemRect(entry).center();const auto before=service_.tools().size();
        for(const auto type:{QEvent::MouseButtonPress,QEvent::MouseButtonRelease,QEvent::MouseButtonDblClick,QEvent::MouseButtonRelease}) {
            QMouseEvent event(type,QPointF(point),QPointF(toolbox_->viewport()->mapToGlobal(point)),Qt::LeftButton,type==QEvent::MouseButtonRelease ? Qt::NoButton:Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(toolbox_->viewport(),&event);
        }
        check(service_.tools().size()==before+1 && service_.tools().last().kind=="calculate","double click creates exactly one tool by kind");
        service_.removeTool(selectedToolId_);refreshWorkspace();selectTool(source);
    }
    auto add=[&](const QString& kind){for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)==kind)toolbox_->setCurrentRow(i);QKeyEvent activate(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(toolbox_,&activate);QApplication::processEvents();return selectedToolId_;};
    auto mouse=[&](QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons){const auto p=imageViewer_->mapFromScene(point);QMouseEvent event(type,QPointF(p),QPointF(p),button,buttons,Qt::NoModifier);QApplication::sendEvent(imageViewer_->viewport(),&event);};
    auto click=[&](QPointF point){mouse(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);QApplication::processEvents();};
    auto drag=[&](QPointF a,QPointF b){mouse(QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);QApplication::processEvents();};
    QString edge;
    for(const QString kind:{"edge_point","edge_pair","line_fit","circle_fit"}) {
        const auto id=add(kind);if(kind=="edge_point")edge=id;
        if(kind=="circle_fit"){
            auto* circleMode=propertiesBody_->findChild<QComboBox*>("mt_mode");check(circleMode->currentText()=="calibrated","new Circle defaults calibrated");
            check(!imageViewer_->roiGestureActive() && workspaceError_->text().contains("current calibration"),"new calibrated Circle rejects drawing without its source model");
            // The generic drawing fixture below explicitly exercises the raw branch.
            circleMode->setCurrentText("pixel");QApplication::processEvents();propertiesBody_->findChild<QPushButton*>("metrologyDraw")->click();
        }
        check(imageViewer_->roiGestureActive() && pendingDrawTool_.isEmpty(),"raw tool drawing armed on current source");
        check(propertiesBody_->findChild<QComboBox*>("input_image")->currentData()==source+"/image","visible persisted default");
        findChild<QPushButton*>("stopFlow")->click();check(!imageViewer_->roiGestureActive() && selectedToolId_==id,"Stop preserves selection and cancels drawing");
        auto* mode=propertiesBody_->findChild<QComboBox*>("propertyDetailMode");auto* manual=propertiesBody_->findChild<QWidget*>(kind+"/manual");auto* outputs=propertiesBody_->findChild<QWidget*>(kind+"/outputs");
        check(mode && mode->currentIndex()==0 && manual->isHidden() && outputs->isHidden(),"Basic and collapsed sections default");
        if(kind=="edge_pair" || kind=="line_fit" || kind=="circle_fit")for(auto* button:propertiesBody_->findChildren<QPushButton*>())check(button->text()!="Calibration aç","CCUX24B removes duplicate calibration navigation");
        if(kind=="line_fit" || kind=="circle_fit") {
            auto* quality=propertiesBody_->findChild<QWidget*>(kind+"/measurementGroup1");
            auto* limits=propertiesBody_->findChild<QWidget*>(kind+"/measurementGroup2");
            check(quality && limits && quality->isHidden() && limits->isHidden(),"fit quality and product limits are separate closed groups");
            check(propertiesBody_->findChild<QDoubleSpinBox*>("mt_minPoints")->parentWidget()==quality,"quality fields grouped without reset");
            check(propertiesBody_->findChild<QLabel*>("metrologyDirectionHelp") && propertiesBody_->findChild<QLabel*>("metrologyPreflight"),"basic has arrow explanation and preflight");
            const auto state=service_.activeRecipe()->definition;propertiesBody_->findChild<QPushButton*>(kind+"/measurementGroup1Toggle")->click();
            propertiesBody_->findChild<QPushButton*>(kind+"/measurementGroup1Toggle")->click();check(service_.activeRecipe()->definition==state,"opening quality section never changes recipe");
        }
        check(outputs->findChildren<QCheckBox*>().size()==outputPorts(kind).size(),"D3 one checkbox per actual port");
        if(kind=="edge_pair") {
            auto* metricMode=propertiesBody_->findChild<QComboBox*>("mt_mode");
            metricMode->setCurrentText("pixel");check(propertiesBody_->findChild<QCheckBox*>("outputVisible_widthPx")->isChecked() && !propertiesBody_->findChild<QCheckBox*>("outputVisible_widthMm")->isChecked(),"D3 live metrology mode refreshes default checkbox without rebuilding");
            propertiesBody_->findChild<QCheckBox*>("outputVisible_widthMm")->click();metricMode->setCurrentText("calibrated");check(propertiesBody_->findChild<QCheckBox*>("outputVisible_widthPx")->isChecked() && propertiesBody_->findChild<QCheckBox*>("outputVisible_widthMm")->isChecked(),"D3 live metrology mode preserves explicit choice");
            propertiesBody_->findChild<QPushButton*>("outputDefaults")->click();check(!propertiesBody_->findChild<QCheckBox*>("outputVisible_widthPx")->isChecked() && propertiesBody_->findChild<QCheckBox*>("outputVisible_widthMm")->isChecked(),"D3 reset resolves current mode instead of captured tool");
        }
        service_.save();const auto definition=service_.activeRecipe()->definition;const auto signature=flowSignature();const auto generation=editorSession_.generation;
        auto* scroll=findChild<QScrollArea*>("propertiesScroll");
        for(const int detail:{0,1}) {
            mode->setCurrentIndex(detail);
            for(const auto size:{QSize(1400,900),QSize(850,600)}) {resize(size);horizontal_->setSizes({155,750,300});QApplication::processEvents();scroll->ensureWidgetVisible(mode);imageViewer_->fitImage();QApplication::processEvents();check(scroll->horizontalScrollBar()->maximum()==0,"properties fit without horizontal scroll");CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("u1-%1-%2-%3.png").arg(kind).arg(detail ? "detailed":"basic").arg(size.width())));}
        }
        if(kind=="line_fit" || kind=="circle_fit")for(int group:{0,1,2}) {
            auto* toggle=propertiesBody_->findChild<QPushButton*>(kind+"/measurementGroup"+QString::number(group)+"Toggle");toggle->click();
            auto* body=propertiesBody_->findChild<QWidget*>(kind+"/measurementGroup"+QString::number(group));
            for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();scroll->ensureWidgetVisible(body->findChild<QDoubleSpinBox*>());QApplication::processEvents();check(scroll->horizontalScrollBar()->maximum()==0,"measurement groups fit narrow panel");CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("mui02-%1-group%2-%3.png").arg(kind).arg(group).arg(size.width())));}
            if(group==1){scroll->ensureWidgetVisible(body->findChild<QDoubleSpinBox*>("mt_minCoverage"));QApplication::processEvents();CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("mui02-%1-coverage-850.png").arg(kind)));}
            toggle->click();
        }
        propertiesBody_->findChild<QPushButton*>(kind+"/manualToggle")->click();propertiesBody_->findChild<QPushButton*>(kind+"/outputsToggle")->click();
        check(!manual->isHidden() && !outputs->isHidden(),"manual and outputs accessible in Detailed");
        for(const auto size:{QSize(1400,900),QSize(850,600)}){
            resize(size);QApplication::processEvents();scroll->ensureWidgetVisible(manual->findChild<QDoubleSpinBox*>());QApplication::processEvents();CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("u1-%1-manual-%2.png").arg(kind).arg(size.width())));
            scroll->ensureWidgetVisible(outputs->findChild<QLabel*>());QApplication::processEvents();CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("u1-%1-outputs-%2.png").arg(kind).arg(size.width())));
        }
        check(service_.activeRecipe()->definition==definition && !service_.dirty() && flowSignature()==signature && editorSession_.generation==generation,"presentation does not edit recipe or invalidate cache");
        selectTool(source);selectTool(id);check(propertiesBody_->findChild<QComboBox*>("propertyDetailMode")->currentIndex()==1 && !propertiesBody_->findChild<QWidget*>(kind+"/manual")->isHidden(),"type preference survives refresh and selection");
        check(!imageViewer_->roiGestureActive(),"existing selection does not arm again");
        click({20,20});check(selectedToolId_==id,"image click preserves tool");
    }
    {
        flowTools_->setCurrentRow(0);flowTools_->setFocus();QKeyEvent down(QEvent::KeyPress,Qt::Key_Down,Qt::NoModifier);QApplication::sendEvent(flowTools_,&down);
        check(selectedToolId_==flowTools_->item(1)->data(Qt::UserRole).toString(),"CCUX24B keyboard retains tool ID selection");
        flowTools_->verticalScrollBar()->setValue(flowTools_->verticalScrollBar()->maximum());const auto scroll=flowTools_->verticalScrollBar()->value();const auto selected=selectedToolId_;
        refreshWorkspace();check(selectedToolId_==selected && flowTools_->verticalScrollBar()->value()==scroll,"CCUX24B rebuild retains list scroll and selection");
        runFlow();wait();check(selectedToolId_==selected && flowTools_->verticalScrollBar()->value()==scroll && flowTools_->isVisible(),"CCUX24B run retains scroll and selection");
        editorSession_.invalidateResults();refreshFlow(); // Keep the following TargetOnly fixture independent of this full-flow run.
    }
    selectTool(edge);resize(1400,900);QApplication::processEvents();imageViewer_->fitImage();propertiesBody_->findChild<QPushButton*>("edgeDraw")->click();drag({40,40},{180,40});wait();
    check(selectedToolId_==edge && imageViewer_->roi().has_value(),"real ROI drag commits while preserving selection");const auto roi=imageViewer_->roi();
    click({230,230});click({100,40});check(selectedToolId_==edge,"ROI inside/outside clicks preserve selection");
    propertiesBody_->findChild<QPushButton*>("edgeDraw")->click();QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(imageViewer_,&escape);check(!imageViewer_->roiGestureActive() && selectedToolId_==edge && imageViewer_->roi()==roi,"Escape preserves prior ROI and selected tool");
    // D3: real execution, followed by view-only changes and a fresh-process restart.
    propertiesBody_->findChild<QPushButton*>("edgeDraw")->click();drag({100,40},{100,200});wait();runExecution(edge);wait();
    check(editorSession_.cycle.has_value(),"D3 real edge cycle");
    service_.save();const auto d3Definition=service_.activeRecipe()->definition;const auto d3Signature=flowSignature();const auto d3Generation=editorSession_.generation;const auto d3Cycle=editorSession_.cycle->cycleId;const auto d3Evaluation=editorSession_.cycle->evaluationId;
    const auto d3Outputs=editorSession_.cycle->tools;const auto d3Roi=imageViewer_->roi();const auto d3Items=imageViewer_->scene()->items().size();
    auto box=[&](const QString& id){auto* result=propertiesBody_->findChild<QCheckBox*>("outputVisible_"+id);check(result,"D3 real registry checkbox");return result;};
    editor_observation::Sample toggleSample;editor_observation::Binding toggleBinding(toggleSample);
    auto unchanged=[&]{for(auto counter:{editor_observation::ModelDecode,editor_observation::PlaneReplay,editor_observation::CatalogDecode})check(toggleSample.calls[counter]==0,"INT07 output toggle does not decode replay or fit");check(!service_.dirty() && service_.activeRecipe()->definition==d3Definition && flowSignature()==d3Signature && editorSession_.generation==d3Generation && !editorSession_.busy() && editorSession_.cycle->cycleId==d3Cycle && editorSession_.cycle->evaluationId==d3Evaluation,"D3 presentation preserves recipe/hash/generation/cycle/job");check(imageViewer_->hasImage() && imageViewer_->roi()==d3Roi && imageViewer_->scene()->items().size()==d3Items,"D3 viewer and overlay retained");check(editorSession_.cycle->tools.size()==d3Outputs.size(),"D3 typed tool results retained");for(int i=0;i<d3Outputs.size();++i){const auto& before=d3Outputs[i];const auto& after=editorSession_.cycle->tools[i];check(before.outputs.keys()==after.outputs.keys() && before.points==after.points && before.lines==after.lines && before.state==after.state && before.errorCode==after.errorCode,"D3 payload structure/geometry/status retained");for(auto it=before.outputs.cbegin();it!=before.outputs.cend();++it){const auto& current=after.outputs[it.key()];check(it->valid==current.valid && it->invalidReason==current.invalidReason && it->frameId==current.frameId && it->cycleId==current.cycleId && ToolResultCodec::display(*it,4)==ToolResultCodec::display(current,4),"D3 typed values and provenance retained");}}};
    check(box("found")->isChecked() && box("pointMm")->isChecked() && !box("point")->isChecked(),"D3 Edge defaults");
    showExecutionResult();QFile d3Results(QDir(artifacts).filePath("d3-results.txt"));check(d3Results.open(QIODevice::WriteOnly),"D3 results evidence");d3Results.write(matchInfo_->text().toUtf8());d3Results.close();check(matchInfo_->text().contains("pointMm: kullanılamıyor") && ([&]{for(const auto& result:editorSession_.cycle->tools)if(result.toolId==edge){const auto reason=result.outputs.value("pointMm").invalidReason;return !reason.isEmpty() && matchInfo_->text().contains(reason);}return false;})(),"D3 selected invalid mm reason without fabricated value");
    box("point")->setFocus();QKeyEvent space(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(box("point"),&space);QKeyEvent release(QEvent::KeyRelease,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(box("point"),&release);
    check(box("point")->isChecked() && matchInfo_->text().contains("  point:"),"D3 keyboard toggle updates Results immediately");unchanged();
    propertiesBody_->findChild<QPushButton*>("outputShowAll")->click();check(box("point")->isChecked(),"D3 show all");unchanged();
    propertiesBody_->findChild<QPushButton*>("outputDefaults")->click();check(!box("point")->isChecked() && !outputPreferences_.load({service_.project().id,service_.activeRecipe()->id,edge}).selection,"D3 reset uses defaults");unchanged();
    for(auto* output:propertiesBody_->findChildren<QCheckBox*>(QRegularExpression("^outputVisible_")))if(output->isChecked())output->click();
    {QFile hidden(QDir(artifacts).filePath("ccux24b-hidden-results.txt"));check(hidden.open(QIODevice::WriteOnly),"hidden results evidence");hidden.write(matchInfo_->text().toUtf8());}
    QString sourceName;for(const auto& tool:service_.tools())if(tool.id==source)sourceName=tool.name;
    const auto selectedResultText=matchInfo_->text().section("\n"+sourceName+" •",0,0);
    check(selectedResultText.contains("Sonuç çıkışları gizli") && !matchInfo_->text().contains(d3Cycle) && selectedResultText.contains("Uyarı · pointMm") && !selectedResultText.contains("  found:"),"CCUX24B hidden identity retains invalid warning");unchanged();
    selectTool(source);check(box("image")->isChecked(),"D3 source has real image port");box("image")->click();check(imageViewer_->hasImage(),"D3 hidden image remains accessible in viewer");selectTool(edge);check(!box("found")->isChecked() && !box("pointMm")->isChecked(),"D3 selection survives tool switch");
    QLockFile locked(settingsFile_+".lock");check(locked.tryLock(0),"D3 lock fixture");box("found")->click();check(!box("found")->isChecked() && !propertiesBody_->findChild<QLabel*>("outputPreferenceError")->isHidden(),"D3 failed save restores persisted check and presents reason");locked.unlock();box("found")->click();box("found")->click();check(propertiesBody_->findChild<QLabel*>("outputPreferenceError")->isHidden(),"D3 successful retry clears storage error");
    findChild<QTabWidget*>("editorTabs")->setCurrentWidget(findChild<QScrollArea*>("resultsScroll"));
    for(const auto size:{QSize(1400,900),QSize(850,600)}) {
        resize(size);horizontal_->setSizes({145,750,310});QApplication::processEvents();auto* outputSection=propertiesBody_->findChild<QWidget*>("edge_point/outputs");if(outputSection->isHidden())propertiesBody_->findChild<QPushButton*>("edge_point/outputsToggle")->click();auto* scroll=findChild<QScrollArea*>("propertiesScroll");scroll->ensureWidgetVisible(box("pointMm"));QApplication::processEvents();check(scroll->horizontalScrollBar()->maximum()==0,"D3 checkboxes fit small panel");check(scroll->verticalScrollBar()->maximum()>0,"D3 output list scrolls");imageViewer_->fitImage();grab().save(QDir(artifacts).filePath(QString("d3-empty-%1.png").arg(size.width())));
        propertiesBody_->findChild<QPushButton*>("outputDefaults")->click();scroll->ensureWidgetVisible(box("pointMm"));QApplication::processEvents();grab().save(QDir(artifacts).filePath(QString("d3-default-%1.png").arg(size.width())));for(auto* output:propertiesBody_->findChildren<QCheckBox*>(QRegularExpression("^outputVisible_")))if(output->isChecked())output->click();
    }
    service_.save();QProcess restart;auto restartEnv=QProcessEnvironment::systemEnvironment();restartEnv.insert("VISIONFLOW_D3_RESTART_PROJECT",service_.path());restartEnv.insert("VISIONFLOW_D3_RESTART_RECIPE",service_.activeRecipe()->id);restartEnv.insert("VISIONFLOW_D3_RESTART_TOOL",edge);restart.setProcessEnvironment(restartEnv);restart.start(QCoreApplication::applicationFilePath(),{"--usability-smoke"});check(restart.waitForFinished(20000),"D3 restart deadline");QFile restartLog(QDir(artifacts).filePath("d3-restart.log"));check(restartLog.open(QIODevice::WriteOnly),"D3 restart log");restartLog.write(restart.readAllStandardOutput());restartLog.write(restart.readAllStandardError());restartLog.close();check(restart.exitStatus()==QProcess::NormalExit && restart.exitCode()==0,"D3 new process restores empty checkboxes");
    const auto failureContext=editorContext();const auto failureSignature=flowSignature();const auto acquisitionError=ExecutionService([](const ToolDefinition&)->std::unique_ptr<AcquisitionAdapter>{throw std::runtime_error("INT07_SOURCE_READ_FAILED");}).run(service_.activeRecipe()->definition,QDir(artifacts).absolutePath(),service_.activeRecipe()->id,failureSignature,{});check(editorSession_.acceptCycle(acquisitionError,failureSignature,failureContext),"D3 accepts actual acquisition failure");refreshFlow();const auto d3ErrorText=matchInfo_->text();QFile d3ErrorLog(QDir(artifacts).filePath("d3-error-results.txt"));check(d3ErrorLog.open(QIODevice::WriteOnly),"D3 error evidence");d3ErrorLog.write(d3ErrorText.toUtf8());d3ErrorLog.close();check(d3ErrorText.contains("ERROR") && d3ErrorText.contains("SKIPPED") && d3ErrorText.contains("Sonuç çıkışları gizli"),"D3 empty preserves actual acquisition error and downstream skipped state");runFlow();wait();
    QTextStream(stdout)<<"D3 output visibility acceptance passed: real cycle, keyboard, reset/all/empty, invalid mm, viewer/overlay, error/skipped, save failure, scroll, tool switch, fresh process restart.\n";
    // Pending creation uses a real processed raster and never falls back to raw.
    const auto pre=add("preprocess");for(auto t:service_.tools())if(t.id==pre){t.parameters["steps"]=QJsonArray{QJsonObject{{"op","threshold"},{"size",3},{"value",100}}};service_.updateTool(t);}refreshWorkspace();const auto circle=add("circle_fit");check(pendingDrawTool_==circle && !imageViewer_->roiGestureActive(),"missing preprocess cache leaves pending intent");
    propertiesBody_->findChild<QComboBox*>("mt_mode")->setCurrentText("pixel");runExecution(pre);wait();check(pendingDrawTool_.isEmpty() && imageViewer_->roiGestureActive(),"upstream completion prepares only still-selected new tool");
    bool processed=false;for(auto* item:imageViewer_->scene()->items())if(auto* pixels=dynamic_cast<QGraphicsPixmapItem*>(item))processed=pixels->pixmap().toImage().pixelColor(0,0).red()==0;check(processed,"drawing displays actual bound processed pixels");refreshFlow();check(imageViewer_->drawingActive() && drawingImage_ && drawingImage_->preview().pixelColor(0,0).red()==0,"Flow refresh preserves current drawing raster");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(propertiesBody_->findChild<QComboBox*>("input_image"));QApplication::processEvents();CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("u4-source-drawing-%1.png").arg(size.width())));}
    findChild<QPushButton*>("stopFlow")->click();runFlow();wait();check(pendingDrawTool_.isEmpty() && !imageViewer_->roiGestureActive(),"Stop prevents late rearming");
    selectTool(pre);
    auto rasterKey=[&]{for(auto* item:imageViewer_->scene()->items())if(auto* pixels=dynamic_cast<QGraphicsPixmapItem*>(item))return pixels->pixmap().cacheKey();return qint64(0);};
    const auto processedKey=rasterKey();check(processedKey!=0,"processed preview present");
    for(int i=0;i<5;++i)refreshFlow();
    check(rasterKey()==processedKey,"unchanged processed result refresh must not upload raw and processed again");
    service_.save();service_.createRecipe("Late file","",ApplicationType::Measurement,"file");service_.addTool("image_file");refreshWorkspace();const auto lateTool=add("edge_point");check(pendingDrawTool_==lateTool,"new tool pending before file raster");
    service_.setImageFile(path);refreshWorkspace();wait();check(selectedToolId_==lateTool && pendingDrawTool_.isEmpty() && imageViewer_->roiGestureActive(),"late file completion arms after all viewer refreshes");findChild<QPushButton*>("stopFlow")->click();
    service_.save();service_.createRecipe("No source","",ApplicationType::DefectInspection,"file");refreshWorkspace();const auto pending=add("edge_point");check(pendingDrawTool_==pending && !imageViewer_->roiGestureActive(),"no image pending");
    selectTool("");const auto late=service_.setImageFile(path);refreshWorkspace();wait();check(pendingDrawTool_.isEmpty() && !imageViewer_->roiGestureActive(),"selection cancels pending before late frame");
    Q_UNUSED(late);
    // Context-menu actions use the clicked ID and retain the selected tool.
    wait();selectTool(late);findChild<QTabWidget*>("editorTabs")->setCurrentIndex(0);const auto beforeOrder=service_.activeRecipe()->definition;
    showFlowMenu(flowTools_->visualItemRect(flowTools_->item(0)).center());
    auto* menu=flowTools_->findChild<QMenu*>("flowMoveMenu");
    check(menu && !menu->actions()[0]->isEnabled() && menu->actions()[1]->isEnabled(),"top menu boundary");
    menu->actions()[1]->trigger();menu->close();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();
    check(service_.tools().last().id==pending && selectedToolId_==late && service_.activeRecipe()->definition!=beforeOrder,"context action moves clicked tool and preserves selection");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){
        resize(size);QApplication::processEvents();showFlowMenu(flowTools_->visualItemRect(flowTools_->item(1)).center());
        menu=flowTools_->findChild<QMenu*>("flowMoveMenu");check(menu && menu->actions()[0]->isEnabled() && !menu->actions()[1]->isEnabled(),"bottom menu boundary");
        CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("u3-flow-%1.png").arg(size.width())));menu->grab().save(QDir(artifacts).filePath(QString("u3-menu-%1.png").arg(size.width())));menu->close();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();
    }
    const auto ticket=editorSession_.begin(SessionJob::Execution);
    showFlowMenu(flowTools_->visualItemRect(flowTools_->item(0)).center());menu=flowTools_->findChild<QMenu*>("flowMoveMenu");
    check(menu && !menu->actions()[0]->isEnabled() && !menu->actions()[1]->isEnabled(),"busy menu disabled");menu->close();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();editorSession_.finish(ticket);
    showFlowMenu(flowTools_->visualItemRect(flowTools_->item(1)).center());menu=flowTools_->findChild<QMenu*>("flowMoveMenu");
    const auto raceDefinition=service_.activeRecipe()->definition;const auto race=editorSession_.begin(SessionJob::Teaching);menu->actions()[0]->trigger();
    check(service_.activeRecipe()->definition==raceDefinition,"job started after menu opened rejects move");menu->close();QApplication::processEvents();editorSession_.finish(race);
    // EUI-01: exercise the application service boundary using real mouse/key events.
    service_.save();service_.createRecipe("Blob ROI acceptance","",ApplicationType::DefectInspection,"file");service_.enableMultiSource();
    const auto blobSourceA=service_.addTool("image_file"),blobSourceB=service_.addTool("image_file");service_.setImageFile(path,blobSourceA);service_.setImageFile(path,blobSourceB);
    const auto blob=service_.addTool("blob");QVector<ToolConnection> blobLinks{{blobSourceA,"image",blob,"image"}};service_.setConnections(blobLinks);service_.save();refreshWorkspace();selectTool(blob);runFlow();wait();
    check(editorSession_.frame && editorSession_.frame->metadata().sourceId==blobSourceA,"Blob selects source A raw coordinate frame");
    resize(1400,900);QApplication::processEvents();imageViewer_->fitImage();
    auto blobRect=[&]{for(const auto& t:service_.tools())if(t.id==blob)return MatchService::rectangle(t.parameters["searchRoi"].toObject());return QRect{};};
    auto blobKey=[&](int key,Qt::KeyboardModifiers mods=Qt::NoModifier){QKeyEvent e(QEvent::KeyPress,key,mods);QApplication::sendEvent(imageViewer_,&e);QApplication::processEvents();};
    propertiesBody_->findChild<QPushButton*>("searchRoi")->click();drag({40.5,40.5},{180.5,180.5});check(blobRect()==QRect(40,40,141,141),"Blob draw service persists inclusive coordinates");
    const auto drawn=blobRect();const auto definitionBeforeDrag=service_.activeRecipe()->definition;
    mouse(QEvent::MouseButtonPress,{100.5,100.5},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{110.5,115.5},Qt::NoButton,Qt::LeftButton);
    check(service_.activeRecipe()->definition==definitionBeforeDrag && findChild<QPushButton*>("stopFlow")->isEnabled(),"Blob draft does not mutate recipe and Stop is available");
    mouse(QEvent::MouseButtonRelease,{110.5,115.5},Qt::LeftButton,Qt::NoButton);QApplication::processEvents();check(blobRect()==QRect(50,55,141,141) && selectedToolId_==blob,"Blob body commits to selected service tool");
    check(!editorSession_.cycle && !editorSession_.matchOutcome && !findChild<QPushButton*>("stopFlow")->isEnabled(),"Blob commit invalidates old results and completes Stop activity");
    blobKey(Qt::Key_Z,Qt::ControlModifier);check(blobRect()==drawn,"Blob Undo remains available after refreshWorkspace");blobKey(Qt::Key_Z,Qt::ControlModifier);check(blobRect().isEmpty(),"Blob Undo initial draw restores full frame");
    propertiesBody_->findChild<QPushButton*>("searchRoi")->click();drag({40.5,40.5},{180.5,180.5});
    mouse(QEvent::MouseButtonPress,{100.5,100.5},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{120.5,125.5},Qt::NoButton,Qt::LeftButton);blobKey(Qt::Key_Escape);mouse(QEvent::MouseButtonRelease,{120.5,125.5},Qt::LeftButton,Qt::NoButton);check(blobRect()==drawn,"Editor Escape cancels Blob edit");
    mouse(QEvent::MouseButtonPress,{100.5,100.5},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{120.5,125.5},Qt::NoButton,Qt::LeftButton);findChild<QPushButton*>("stopFlow")->click();mouse(QEvent::MouseButtonRelease,{120.5,125.5},Qt::LeftButton,Qt::NoButton);check(blobRect()==drawn,"Editor Stop cancels Blob edit");
    mouse(QEvent::MouseButtonPress,{100.5,100.5},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{120.5,125.5},Qt::NoButton,Qt::LeftButton);const auto blobTicket=editorSession_.begin(SessionJob::Execution);refreshFlow();mouse(QEvent::MouseButtonRelease,{120.5,125.5},Qt::LeftButton,Qt::NoButton);check(blobRect()==drawn && !imageViewer_->drawingActive(),"busy transition cancels Blob preview");editorSession_.finish(blobTicket);refreshMatchOverlay();refreshFlow();
    mouse(QEvent::MouseButtonPress,{100.5,100.5},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{120.5,125.5},Qt::NoButton,Qt::LeftButton);selectTool(blobSourceA);mouse(QEvent::MouseButtonRelease,{120.5,125.5},Qt::LeftButton,Qt::NoButton);selectTool(blob);check(blobRect()==drawn && !imageViewer_->drawingActive(),"selection cancels Blob edit without arming existing tool");
    // Same pixel content on a different source must still retire the old gesture/history.
    mouse(QEvent::MouseButtonPress,{100.5,100.5},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{120.5,125.5},Qt::NoButton,Qt::LeftButton);blobLinks[0].sourceTool=blobSourceB;service_.setConnections(blobLinks);refreshWorkspace();runFlow();wait();mouse(QEvent::MouseButtonRelease,{120.5,125.5},Qt::LeftButton,Qt::NoButton);
    check(blobRect()==drawn && editorSession_.frame->metadata().sourceId==blobSourceB,"Blob source switch with identical pixels cancels old draft");blobKey(Qt::Key_Z,Qt::ControlModifier);check(blobRect()==drawn,"Blob source switch retires Undo history");
    // Region and reference input modes disable both the gesture and teaching controls.
    const auto producer=service_.addTool("blob");auto lockedLinks=blobLinks;lockedLinks<<ToolConnection{blobSourceB,"image",producer,"image"}<<ToolConnection{producer,"region",blob,"region"};service_.setConnections(lockedLinks);refreshWorkspace();selectTool(blob);check(!propertiesBody_->findChild<QPushButton*>("searchRoi")->isEnabled(),"region input locks Blob drawing control");drag({100.5,100.5},{120.5,125.5});check(blobRect()==drawn,"region input locks Blob body");
    lockedLinks.last()=ToolConnection{producer,"point",blob,"reference"};service_.setConnections(lockedLinks);refreshWorkspace();check(!propertiesBody_->findChild<QPushButton*>("searchRoi")->isEnabled(),"reference input locks Blob drawing control");drag({100.5,100.5},{120.5,125.5});check(blobRect()==drawn,"reference input locks Blob body");
    const auto poseProducer=service_.addTool("pattern_pose");service_.setConnections({{blobSourceB,"image",blob,"image"},{blobSourceB,"image",poseProducer,"image"},{poseProducer,"pose",blob,"poseReference"}});refreshWorkspace();check(!propertiesBody_->findChild<QPushButton*>("searchRoi")->isEnabled(),"poseReference input locks Blob drawing control");drag({100.5,100.5},{120.5,125.5});check(blobRect()==drawn,"poseReference input locks Blob body");
    service_.setConnections(blobLinks);refreshWorkspace();selectTool(blob);imageViewer_->fitImage();drag({100.5,100.5},{110.5,115.5});const auto savedBlob=blobRect();check(savedBlob!=drawn,"Blob editable after explicit input unbind");
    service_.save();const auto savedDefinition=service_.activeRecipe()->definition;const auto savedPath=service_.path(),savedRecipe=service_.activeRecipe()->id;ProjectService reopened;reopened.openProject(savedPath);reopened.selectRecipe(savedRecipe);check(reopened.activeRecipe()->definition==savedDefinition && !reopened.dirty(),"Blob Save/reopen exact definition");
    QFile beforeFile(savedPath);check(beforeFile.open(QIODevice::ReadOnly),"read saved Blob project");const auto beforeBytes=beforeFile.readAll();beforeFile.close();reopened.save();QFile afterFile(savedPath);check(afterFile.open(QIODevice::ReadOnly) && afterFile.readAll()==beforeBytes,"Blob no-op Save keeps bytes");afterFile.close();
    const auto blobPre=service_.addTool("preprocess");for(auto t:service_.tools())if(t.id==blobPre){t.parameters["steps"]=QJsonArray{QJsonObject{{"op","threshold"},{"size",3},{"value",100}}};service_.updateTool(t);}
    service_.setConnections({{blobSourceB,"image",blobPre,"image"},{blobPre,"image",blob,"image"}});refreshWorkspace();selectTool(blob);runExecution(blobPre);wait();
    bool blobProcessed=false;for(auto* item:imageViewer_->scene()->items())if(auto* pixels=dynamic_cast<QGraphicsPixmapItem*>(item))blobProcessed=blobProcessed || pixels->pixmap().toImage().pixelColor(0,0).red()==0;check(blobProcessed && imageInfo_->text().contains("İşlenmiş görüntü") && imageInfo_->text().contains("bağlı kaynak:"),"Blob edit workspace displays and labels actual bound processed raster");
    imageViewer_->fitImage();drag({100.5,100.5},{105.5,105.5});check(blobRect()!=savedBlob,"Blob editing remains available on processed input");
    // EUI-02: the checkbox edits recipe state and clears stale results.
    runExecution(blob);wait();service_.save();
    auto* border=propertiesBody_->findChild<QCheckBox*>("rejectBorderTouching");
    check(border && !border->isChecked(),"border rejection defaults off");
    const auto offSignature=flowSignature();border->setChecked(true);QApplication::processEvents();
    check(service_.dirty() && flowSignature()!=offSignature,"border edit changes recipe identity");
    check(!editorSession_.cycle || std::none_of(editorSession_.cycle->tools.cbegin(),editorSession_.cycle->tools.cend(),[&](const auto& t){return t.toolId==blob && (!t.outputs.isEmpty() || !t.mask.pixels.isEmpty());}),"border edit removes stale target outputs and mask");
    findChild<QPushButton*>("runTool")->click();wait();
    check(editorSession_.cycle && editorSession_.cycle->partial && editorSession_.cycle->executedTools==QStringList{blob},"border filter uses common target-only executor");
    const auto filtered=std::find_if(editorSession_.cycle->tools.cbegin(),editorSession_.cycle->tools.cend(),[&](const auto& t){return t.toolId==blob;});check(filtered!=editorSession_.cycle->tools.cend() && std::get<double>(filtered->outputs["candidateCount"].data)==1 && std::get<double>(filtered->outputs["count"].data)==0 && !filtered->outputs["point"].valid,"border stripe rejected with no stale point");
    service_.save();const auto borderRecipe=service_.activeRecipe()->id,borderPath=service_.path();service_.openProject(borderPath);service_.selectRecipe(borderRecipe);refreshWorkspace();selectTool(blob);
    check(propertiesBody_->findChild<QCheckBox*>("rejectBorderTouching")->isChecked() && !service_.dirty(),"border checkbox Save/reopen");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();auto* scroll=findChild<QScrollArea*>("propertiesScroll");scroll->ensureWidgetVisible(propertiesBody_->findChild<QCheckBox*>("rejectBorderTouching"));QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("eui02-blob-editor-%1.png").arg(size.width()))),"border filter screenshot");}
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);vertical_->setSizes(size.width()==850 ? QList<int>{360,90}:QList<int>{600,180});QApplication::processEvents();imageViewer_->fitImage();QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("eui01-blob-editor-%1.png").arg(size.width()))),"Blob Editor screenshot");}
    const auto beforeRecipeSwitch=service_.activeRecipe()->definition;mouse(QEvent::MouseButtonPress,{100.5,100.5},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{110.5,110.5},Qt::NoButton,Qt::LeftButton);service_.createRecipe("Cancel Blob gesture","",ApplicationType::Measurement,"file");refreshWorkspace();mouse(QEvent::MouseButtonRelease,{110.5,110.5},Qt::LeftButton,Qt::NoButton);service_.selectRecipe(savedRecipe);refreshWorkspace();check(service_.activeRecipe()->definition==beforeRecipeSwitch,"recipe switch retires Blob preview without commit");

    // EUI-03: real result toggling neither runs nor edits/invalidate the recipe.
    service_.save();service_.createRecipe("Annotation acceptance","",ApplicationType::DefectInspection,"file");service_.enableMultiSource();
    const auto annotationSourceA=service_.setImageFile(path),annotationSourceB=service_.addTool("image_file");service_.setImageFile(path,annotationSourceB);
    const auto annotationBlob=service_.addTool("blob");service_.setConnections({{annotationSourceB,"image",annotationBlob,"image"}});
    refreshWorkspace();selectTool(annotationBlob);runFlow();wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"annotation acceptance has real DONE result");service_.save();const auto annotationDefinition=service_.activeRecipe()->definition;
    const auto annotationSignature=flowSignature();
    for(bool visible:{true,false,true}){
        const auto generation=editorSession_.generation;const auto cycleId=editorSession_.cycle->cycleId;
        annotations->setChecked(visible);QApplication::processEvents();
        check(imageViewer_->resultAnnotationsVisible()==visible && !editorSession_.busy(),"toggle updates presentation without execution");
        check(service_.activeRecipe()->definition==annotationDefinition && !service_.dirty() && flowSignature()==annotationSignature && editorSession_.generation==generation && editorSession_.cycle && editorSession_.cycle->cycleId==cycleId,"toggle preserves recipe identity and current cycle");
        selectTool(annotationSourceA);selectTool(annotationSourceB);selectTool(annotationBlob);runExecution(annotationBlob);wait();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done,"annotation rerun has real DONE result");check(imageViewer_->resultAnnotationsVisible()==visible,"preference survives tool/source selection and rerun");
        for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();imageViewer_->fitImage();QApplication::processEvents();check(CameraFlowSmokeAudit::save(*this,QDir(artifacts).filePath(QString("eui03-editor-%1-%2.png").arg(visible ? "on":"off").arg(size.width()))),"annotation Editor PNG");}
    }
    {MainWindow restarted(settingsFile_);check(restarted.findChild<QCheckBox*>("showResultAnnotations")->isChecked() && restarted.imageViewer_->resultAnnotationsVisible(),"restart loads annotation preference");}
    annotations->setChecked(false);
    service_.save();service_.createRecipe("D3 multi-source identity","",ApplicationType::Measurement,"file");service_.enableMultiSource();
    const auto d3SourceA=service_.addTool("image_file");service_.setImageFile(path,d3SourceA);const auto d3SourceB=service_.addTool("image_file");service_.setImageFile(path,d3SourceB);service_.save();refreshWorkspace();selectTool(d3SourceA);runExecution(d3SourceA);wait();
    check(editorSession_.cycle && !matchInfo_->text().contains("Kaynak:"),"CCUX24B editor9 omits source identity");const auto sourceCycle=editorSession_.cycle->cycleId;const auto sourceFrame=editorSession_.frame->metadata().frameId;box("image")->click();
    check(!matchInfo_->text().contains("Kaynak:") && !matchInfo_->text().contains(sourceCycle) && !matchInfo_->text().contains(sourceFrame) && matchInfo_->text().contains("Sonuç çıkışları gizli") && imageViewer_->hasImage() && !service_.dirty() && !editorSession_.busy(),"D3 editor9 hidden image preserves actual source/frame/cycle/viewer");
    service_.createRecipe("D3 Geometry defaults","",ApplicationType::Measurement,"file");const auto d3Geometry=service_.addTool("geometry_measurement");service_.save();refreshWorkspace();selectTool(d3Geometry);
    check(box("valueMm")->isChecked() && box("status")->isChecked() && !box("valuePx")->isChecked(),"D3 Geometry calibrated defaults");
    propertiesBody_->findChild<QComboBox*>("gm_mode")->setCurrentText("pixel");QApplication::processEvents();propertiesBody_->findChild<QComboBox*>("gm_operation")->setCurrentText("line_angle");QApplication::processEvents();
    check(box("anglePx")->isChecked() && box("status")->isChecked() && !box("valueMm")->isChecked(),"D3 Geometry mode/operation defaults refresh");box("valueMm")->click();
    propertiesBody_->findChild<QComboBox*>("gm_mode")->setCurrentText("calibrated");QApplication::processEvents();propertiesBody_->findChild<QComboBox*>("gm_operation")->setCurrentText("point_distance");QApplication::processEvents();
    check(box("valueMm")->isChecked() && box("anglePx")->isChecked() && !box("valuePx")->isChecked(),"D3 Geometry explicit selection survives mode/operation change");
    service_.save();QTextStream(stdout)<<"D3 editor9 source identity and Geometry mode/operation checkbox acceptance passed.\n";return true;
}
}
