#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include "Toolbox.hpp"
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
#include <QThread>
#include <QTabWidget>
#include <QUuid>
#include <stdexcept>
#include <algorithm>

namespace visionflow {
bool MainWindow::verifyEditorUsability(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto wait=[&]{QElapsedTimer timer;timer.start();do{QApplication::processEvents();QThread::msleep(1);}while(editorSession_.busy() && timer.elapsed()<20000);QApplication::processEvents();check(!editorSession_.busy(),"usability worker deadline");};
    {QSettings preferences(settingsFile_,QSettings::IniFormat);preferences.remove("properties");}
    {MainWindow firstUse(QDir(artifacts).filePath("annotation-first-use-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".ini"));check(!firstUse.findChild<QCheckBox*>("showResultAnnotations")->isChecked() && !firstUse.imageViewer_->resultAnnotationsVisible(),"first use defaults annotations off");}
    auto* annotations=findChild<QCheckBox*>("showResultAnnotations");check(annotations,"Editor annotation checkbox");
    check(annotations->isChecked()==QSettings(settingsFile_,QSettings::IniFormat).value("editor/showResultAnnotations",false).toBool(),"annotation preference loaded");
    annotations->setChecked(false);
    QImage image(256,256,QImage::Format_Grayscale8);image.fill(20);for(int y=80;y<160;++y)for(int x=0;x<256;++x)image.scanLine(y)[x]=220;
    const auto path=QDir(artifacts).filePath("usability-source.png");check(image.save(path),"usability PNG");
    service_.createProject("Usability","",QDir(artifacts).filePath("usability-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".vfproject"),"file");service_.createRecipe("Usability","",ApplicationType::Measurement,"file");
    const auto source=service_.setImageFile(path);refreshWorkspace();selectPage("editor");wait();
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
        for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);horizontal_->setSizes({190,750,300});QApplication::processEvents();toolbox_->scrollToTop();imageViewer_->fitImage();grab().save(QDir(artifacts).filePath(QString("eui05-toolbox-%1.png").arg(size.width())));}
        for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Toolbox::HeaderRole).toBool()){toolbox_->setCurrentRow(i);key(Qt::Key_Left);}
        resize(1400,900);QApplication::processEvents();toolbox_->scrollToTop();grab().save(QDir(artifacts).filePath("eui05-groups-collapsed-1400.png"));
        search->setText("Line Fit");for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();toolbox_->scrollToTop();grab().save(QDir(artifacts).filePath(QString("eui05-search-%1.png").arg(size.width())));}search->clear();
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
        if(kind=="circle_fit"){check(!imageViewer_->roiGestureActive(),"mm Circle requires current calibration before drawing");check(propertiesBody_->findChild<QComboBox*>("mt_mode")->currentText()=="calibrated","new Circle defaults to mm");propertiesBody_->findChild<QComboBox*>("mt_mode")->setCurrentText("pixel");propertiesBody_->findChild<QPushButton*>("metrologyDraw")->click();}
        check(imageViewer_->roiGestureActive() && pendingDrawTool_.isEmpty(),"new tool immediately armed on current source");
        check(propertiesBody_->findChild<QComboBox*>("input_image")->currentData()==source+"/image","visible persisted default");
        findChild<QPushButton*>("stopFlow")->click();check(!imageViewer_->roiGestureActive() && selectedToolId_==id,"Stop preserves selection and cancels drawing");
        auto* mode=propertiesBody_->findChild<QComboBox*>("propertyDetailMode");auto* manual=propertiesBody_->findChild<QWidget*>(kind+"/manual");auto* outputs=propertiesBody_->findChild<QWidget*>(kind+"/outputs");
        check(mode && mode->currentIndex()==0 && manual->isHidden() && outputs->isHidden(),"Basic and collapsed sections default");
        service_.save();const auto definition=service_.activeRecipe()->definition;const auto signature=flowSignature();const auto generation=editorSession_.generation;
        auto* scroll=findChild<QScrollArea*>("propertiesScroll");
        for(const int detail:{0,1}) {
            mode->setCurrentIndex(detail);
            for(const auto size:{QSize(1400,900),QSize(850,600)}) {resize(size);horizontal_->setSizes({155,750,300});QApplication::processEvents();scroll->ensureWidgetVisible(mode);imageViewer_->fitImage();QApplication::processEvents();check(scroll->horizontalScrollBar()->maximum()==0,"properties fit without horizontal scroll");grab().save(QDir(artifacts).filePath(QString("u1-%1-%2-%3.png").arg(kind).arg(detail ? "detailed":"basic").arg(size.width())));}
        }
        propertiesBody_->findChild<QPushButton*>(kind+"/manualToggle")->click();propertiesBody_->findChild<QPushButton*>(kind+"/outputsToggle")->click();
        check(!manual->isHidden() && !outputs->isHidden(),"manual and outputs accessible in Detailed");
        for(const auto size:{QSize(1400,900),QSize(850,600)}){
            resize(size);QApplication::processEvents();scroll->ensureWidgetVisible(manual->findChild<QDoubleSpinBox*>());QApplication::processEvents();grab().save(QDir(artifacts).filePath(QString("u1-%1-manual-%2.png").arg(kind).arg(size.width())));
            scroll->ensureWidgetVisible(outputs->findChild<QLabel*>());QApplication::processEvents();grab().save(QDir(artifacts).filePath(QString("u1-%1-outputs-%2.png").arg(kind).arg(size.width())));
        }
        check(service_.activeRecipe()->definition==definition && !service_.dirty() && flowSignature()==signature && editorSession_.generation==generation,"presentation does not edit recipe or invalidate cache");
        selectTool(source);selectTool(id);check(propertiesBody_->findChild<QComboBox*>("propertyDetailMode")->currentIndex()==1 && !propertiesBody_->findChild<QWidget*>(kind+"/manual")->isHidden(),"type preference survives refresh and selection");
        check(!imageViewer_->roiGestureActive(),"existing selection does not arm again");
        click({20,20});check(selectedToolId_==id,"image click preserves tool");
    }
    selectTool(edge);resize(1400,900);QApplication::processEvents();imageViewer_->fitImage();propertiesBody_->findChild<QPushButton*>("edgeDraw")->click();drag({40,40},{180,40});wait();
    check(selectedToolId_==edge && imageViewer_->roi().has_value(),"real ROI drag commits while preserving selection");const auto roi=imageViewer_->roi();
    click({230,230});click({100,40});check(selectedToolId_==edge,"ROI inside/outside clicks preserve selection");
    propertiesBody_->findChild<QPushButton*>("edgeDraw")->click();QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(imageViewer_,&escape);check(!imageViewer_->roiGestureActive() && selectedToolId_==edge && imageViewer_->roi()==roi,"Escape preserves prior ROI and selected tool");
    // Pending creation uses a real processed raster and never falls back to raw.
    const auto pre=add("preprocess");for(auto t:service_.tools())if(t.id==pre){t.parameters["steps"]=QJsonArray{QJsonObject{{"op","threshold"},{"size",3},{"value",100}}};service_.updateTool(t);}refreshWorkspace();const auto circle=add("circle_fit");check(pendingDrawTool_==circle && !imageViewer_->roiGestureActive(),"missing preprocess cache leaves pending intent");
    propertiesBody_->findChild<QComboBox*>("mt_mode")->setCurrentText("pixel");runExecution(pre);wait();check(pendingDrawTool_.isEmpty() && imageViewer_->roiGestureActive(),"upstream completion prepares only still-selected new tool");
    bool processed=false;for(auto* item:imageViewer_->scene()->items())if(auto* pixels=dynamic_cast<QGraphicsPixmapItem*>(item))processed=pixels->pixmap().toImage().pixelColor(0,0).red()==0;check(processed,"drawing displays actual bound processed pixels");refreshFlow();check(imageViewer_->drawingActive() && drawingImage_ && drawingImage_->preview().pixelColor(0,0).red()==0,"Flow refresh preserves current drawing raster");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(propertiesBody_->findChild<QComboBox*>("input_image"));QApplication::processEvents();grab().save(QDir(artifacts).filePath(QString("u4-source-drawing-%1.png").arg(size.width())));}
    findChild<QPushButton*>("stopFlow")->click();runFlow();wait();check(pendingDrawTool_.isEmpty() && !imageViewer_->roiGestureActive(),"Stop prevents late rearming");
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
        grab().save(QDir(artifacts).filePath(QString("u3-flow-%1.png").arg(size.width())));menu->grab().save(QDir(artifacts).filePath(QString("u3-menu-%1.png").arg(size.width())));menu->close();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();
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
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();auto* scroll=findChild<QScrollArea*>("propertiesScroll");scroll->ensureWidgetVisible(propertiesBody_->findChild<QCheckBox*>("rejectBorderTouching"));QApplication::processEvents();check(grab().save(QDir(artifacts).filePath(QString("eui02-blob-editor-%1.png").arg(size.width()))),"border filter screenshot");}
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);vertical_->setSizes(size.width()==850 ? QList<int>{360,90}:QList<int>{600,180});QApplication::processEvents();imageViewer_->fitImage();QApplication::processEvents();check(grab().save(QDir(artifacts).filePath(QString("eui01-blob-editor-%1.png").arg(size.width()))),"Blob Editor screenshot");}
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
        for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();imageViewer_->fitImage();QApplication::processEvents();check(grab().save(QDir(artifacts).filePath(QString("eui03-editor-%1-%2.png").arg(visible ? "on":"off").arg(size.width()))),"annotation Editor PNG");}
    }
    {MainWindow restarted(settingsFile_);check(restarted.findChild<QCheckBox*>("showResultAnnotations")->isChecked() && restarted.imageViewer_->resultAnnotationsVisible(),"restart loads annotation preference");}
    annotations->setChecked(false);
    service_.save();return true;
}
}
