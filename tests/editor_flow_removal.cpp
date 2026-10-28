#include "app/MainWindow.hpp"
#include "app/ImageViewer.hpp"
#include "application/RuntimeCoordinator.hpp"
#include "domain/AssignmentConfig.hpp"
#include "domain/PublicationConfig.hpp"
#include "domain/ResultDecision.hpp"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <QUuid>
#include <stdexcept>

namespace visionflow {
bool MainWindow::verifyFlowRemoval(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    QTemporaryDir dir(QDir(artifacts).filePath("efr01-XXXXXX"));check(dir.isValid(),"EFR01 fixture");
    MainWindow w(dir.filePath("panels.ini"));w.smokeMode_=true;w.resize(1400,900);w.show();
    auto wait=[&]{QElapsedTimer timer;timer.start();do{QApplication::processEvents();QThread::msleep(1);}while(w.editorSession_.busy() && timer.elapsed()<20000);QApplication::processEvents();check(!w.editorSession_.busy(),"EFR01 worker deadline");};
    auto closeMenu=[&](QMenu* menu){menu->close();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();};
    auto popup=[&](const QString& id,bool settle=true){
        for(int row=0;row<w.flowTools_->count();++row)if(w.flowTools_->item(row)->data(Qt::UserRole)==id){
            w.flowTools_->scrollToItem(w.flowTools_->item(row));if(settle)QApplication::processEvents();
            w.showFlowMenu(w.flowTools_->visualItemRect(w.flowTools_->item(row)).center());
            auto* menu=w.flowTools_->findChild<QMenu*>("flowMoveMenu");check(menu && menu->actions().size()==4 && menu->actions()[2]->isSeparator() && menu->actions()[3]->text()=="Remove Tool","EFR01 menu order");return menu;
        }throw std::runtime_error("EFR01 popup target missing");
    };
    auto remove=[&](const QString& id){auto* menu=popup(id);menu->actions()[3]->trigger();closeMenu(menu);wait();};
    w.service_.createProject("Flow removal","",dir.filePath("removal.vfproject"),"file");
    for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        w.service_.createRecipe("Removal "+typeKey(type),"",type);
        const auto first=w.service_.addTool("blob"),middle=w.service_.addTool("blob"),last=w.service_.addTool("blob");
        w.refreshWorkspace();w.selectPage("editor");w.selectTool(last);wait();
        remove(first);check(w.selectedToolId_==last && w.service_.tools().size()==2,"EFR01 right click A selected B");
        w.selectTool(middle);remove(middle);check(w.selectedToolId_==last,"EFR01 selected first chooses next");
        remove(last);check(w.selectedToolId_.isEmpty() && w.flowTools_->count()==0 && !w.propertiesBody_->findChild<QPushButton*>("removeTool"),"EFR01 only tool clears Properties");
        const auto a=w.service_.addTool("blob"),b=w.service_.addTool("blob"),c=w.service_.addTool("blob");w.refreshWorkspace();w.selectTool(b);
        remove(b);check(w.selectedToolId_==c,"EFR01 selected middle chooses next");
        w.propertiesBody_->findChild<QPushButton*>("removeTool")->click();check(w.selectedToolId_==a,"EFR01 Properties selected last chooses previous");
        w.service_.save();const auto recipe=w.service_.activeRecipe()->id,project=w.service_.path();w.service_.openProject(project);w.service_.selectRecipe(recipe);w.refreshWorkspace();check(w.service_.tools().size()==1 && w.service_.tools()[0].id==a,"EFR01 removal save reopen");
    }
    // Late callbacks: idle admission, same recipe identity, and still-existing ID.
    auto target=w.service_.tools()[0].id;w.selectTool(target);
    for(auto job:{SessionJob::Execution,SessionJob::Teaching}){
        const auto ticket=w.editorSession_.begin(job);w.refreshFlow();
        auto* menu=popup(target);check(!menu->actions()[3]->isEnabled() && !w.propertiesBody_->findChild<QPushButton*>("removeTool")->isEnabled(),"EFR01 busy controls disabled");closeMenu(menu);w.editorSession_.finish(ticket);w.refreshFlow();
        menu=popup(target);const auto before=w.service_.activeRecipe()->definition;const auto race=w.editorSession_.begin(job);menu->actions()[3]->trigger();check(w.service_.activeRecipe()->definition==before,"EFR01 Run/Teach after popup rejects");closeMenu(menu);w.editorSession_.finish(race);
    }
    auto* preparing=popup(target);const auto beforePreparing=w.service_.activeRecipe()->definition;w.editorCameraPendingExecution_=QString{};preparing->actions()[3]->trigger();check(w.service_.activeRecipe()->definition==beforePreparing,"EFR01 pending camera Run rejects late action");
    // Hold the admission boundary without pumping the camera completion timer.
    preparing->close();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);w.refreshFlow();preparing=popup(target,false);check(!preparing->actions()[3]->isEnabled() && !w.propertiesBody_->findChild<QPushButton*>("removeTool")->isEnabled(),"EFR01 pending camera Run disables removal");w.editorCameraPendingExecution_.reset();closeMenu(preparing);w.refreshFlow();
    auto* stale=popup(target);w.service_.removeTool(target);w.refreshWorkspace();w.service_.save();const auto beforeMissing=w.service_.activeRecipe()->definition;stale->actions()[3]->trigger();check(!w.service_.dirty() && w.service_.activeRecipe()->definition==beforeMissing,"EFR01 missing target no-op");closeMenu(stale);
    target=w.service_.addTool("blob");w.refreshWorkspace();stale=popup(target);w.service_.save();w.service_.createRecipe("Different recipe","",ApplicationType::Measurement);const auto other=w.service_.activeRecipe()->definition;stale->actions()[3]->trigger();check(w.service_.activeRecipe()->definition==other,"EFR01 changed recipe before refresh no-op");w.refreshWorkspace();stale->actions()[3]->trigger();check(w.service_.activeRecipe()->definition==other,"EFR01 changed recipe after refresh no-op");closeMenu(stale);
    w.showFlowMenu(QPoint(-1,-1));check(!w.flowTools_->findChild<QMenu*>("flowMoveMenu"),"EFR01 blank Flow no popup");
    // A real cycle provides Editor cache/overlay and an immutable active Runtime.
    w.service_.enableMultiSource();QImage image(40,40,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("source.png")),"EFR01 raster");
    const auto source=w.service_.setImageFile(dir.filePath("source.png")),blob=w.service_.addTool("blob");w.service_.save();w.refreshWorkspace();w.selectTool(blob);wait();w.runFlow();wait();check(w.editorSession_.cycle.has_value(),"EFR01 actual cycle");
    RuntimeService runtime(dir.filePath("runtime"),dir.filePath("catalog.json"),RuntimeIdentity::localEngineer(),{},{},{},[](const Recipe& recipe){MultiSourceDependencies d;for(const auto& tool:editorTools(recipe.definition))if(tool.kind=="image_file")d.sources[tool.id]={};return d;});
    RuntimeCoordinator coordinator(runtime);coordinator.load(*w.service_.activeRecipe(),dir.path());coordinator.start();const auto snapshot=runtime.active();check(bool(snapshot),"EFR01 Runtime active snapshot");const auto frozen=snapshot->recipe().definition;
    w.pendingDrawTool_=blob;w.pendingDrawRecipe_=w.editorRecipeId_;w.drawingImage_=w.editorSession_.frame;w.imageViewer_->drawRegion(true);
    remove(blob);check(!w.editorSession_.cycle && !w.editorSession_.matchOutcome && w.pendingDrawTool_.isEmpty() && !w.drawingImage_ && !w.imageViewer_->drawingActive() && !w.editorSession_.evaluation(w.editorContext(),source).cache.contains(blob),"EFR01 removes draw intent and cache/results");
    check(runtime.active()==snapshot && snapshot->recipe().definition==frozen,"EFR01 active Runtime immutable");coordinator.stop();
    // Normal source deletion leaves a visibly invalid consumer and no stale frame.
    const auto consumer=w.service_.addTool("blob");w.refreshWorkspace();w.selectTool(consumer);w.runFlow();wait();remove(source);
    check(w.selectedToolId_==consumer && !validateGraph(w.service_.tools(),editorConnections(w.service_.activeRecipe()->definition)).valid() && w.editorSession_.sourceFrames.isEmpty() && !w.imageViewer_->hasImage(),"EFR01 source removes links/frame and exposes missing input");
    // Assignment/Result derived references remain visible, never a usable value.
    const auto producer=w.service_.addTool("blob"),assignment=w.service_.addTool("set_variables"),result=w.service_.addTool("result");
    VariableDefinition variable;variable.name="Found";const auto variableId=w.service_.addVariable(variable);
    for(auto tool:w.service_.tools())if(tool.id==assignment){tool.parameters=encodeAssignmentConfig({assignment,1,{{variableId,ToolOutputAssignmentSource{producer,"found",4}}}});w.service_.updateTool(tool);}else if(tool.id==result){tool.parameters=encodeResultConfig({ResultOperator::All,{{producer,"found",4}}});tool.enabled=true;w.service_.updateTool(tool);}
    w.refreshWorkspace();remove(producer);check(!validateGraph(w.service_.tools(),editorConnections(w.service_.activeRecipe()->definition)).valid() && w.graphInfo_->text().contains(producer),"EFR01 Variables/Result missing source visible");
    const auto failure=ExecutionService{}.run(w.service_.activeRecipe()->definition,dir.path(),w.service_.activeRecipe()->id,"removal",{});check(failure.state!=ExecutionState::Done && failure.decision()==ResultDecision::NotEvaluated && failure.publicationIntents.isEmpty(),"EFR01 dangling consumers cannot produce success");
    // Publish producer references are governed by the existing atomic rejection.
    ValueEnvelope constant;constant.validity=ValueValidity::Valid;constant.value=false;
    for(auto tool:w.service_.tools())if(tool.id==assignment){tool.parameters=encodeAssignmentConfig({assignment,1,{{variableId,ConstantAssignmentSource{constant}}}});w.service_.updateTool(tool);}
    auto uuid=[] {return QUuid::createUuid().toString(QUuid::WithoutBraces);};
    OutputGroup group{uuid(),uuid(),1,true,"vf.fields/1",assignment,1,{{"found",variableId}}};w.service_.setOutputGroups({group});const auto publish=w.service_.addTool("publish_data");
    for(auto tool:w.service_.tools())if(tool.id==publish){tool.parameters=encodePublishConfig({{group.id}});tool.enabled=true;w.service_.updateTool(tool);}
    w.refreshWorkspace();w.selectTool(assignment);w.service_.save();const auto protectedDefinition=w.service_.activeRecipe()->definition;const auto generation=w.editorSession_.generation;
    w.pendingDrawTool_=assignment;remove(assignment);
    check(w.service_.activeRecipe()->definition==protectedDefinition && !w.service_.dirty() && w.selectedToolId_==assignment && w.pendingDrawTool_==assignment && w.editorSession_.generation==generation && !w.workspaceError_->isHidden(),"EFR01 rejected delete preserves state and shows reason");
    w.propertiesBody_->findChild<QPushButton*>("removeTool")->click();check(w.service_.activeRecipe()->definition==protectedDefinition && !w.service_.dirty(),"EFR01 Properties uses same rejection");w.pendingDrawTool_.clear();
    // Normal and small-screen menu, followed by the selected replacement.
    w.service_.save();w.service_.createRecipe("Removal visual","",ApplicationType::Measurement);w.service_.enableMultiSource();w.service_.addTool("image_file");w.refreshWorkspace();w.selectPage("editor");w.findChild<QTabWidget*>("editorTabs")->setCurrentIndex(0);
    for(const auto size:{QSize(1400,900),QSize(850,600)}){
        const auto a=w.service_.addTool("image_file"),b=w.service_.addTool("image_file");w.refreshWorkspace();w.selectTool(a);w.resize(size);QApplication::processEvents();
        auto* menu=popup(a);check(menu->isVisible() && menu->actionGeometry(menu->actions()[3]).height()>0,"EFR01 popup visible");
        check(w.grab().save(QDir(artifacts).filePath(QString("efr01-before-%1.png").arg(size.width()))) && menu->grab().save(QDir(artifacts).filePath(QString("efr01-menu-%1.png").arg(size.width()))),"EFR01 popup screenshots");
        menu->actions()[3]->trigger();closeMenu(menu);check(w.selectedToolId_==b,"EFR01 visual replacement selection");check(w.grab().save(QDir(artifacts).filePath(QString("efr01-after-%1.png").arg(size.width()))),"EFR01 replacement screenshot");
    }
    QTextStream(stdout)<<"EFR-01: shared removal, three types, selection boundaries, stale menu, busy, real cache/frame cleanup, immutable Runtime, missing references and atomic publication rejection passed.\n";
    w.service_.save();w.close();return true;
}
}
