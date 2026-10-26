#include "app/MainWindow.hpp"
#include "application/ToolCatalog.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
namespace visionflow {
bool MainWindow::verifyAssignmentUi(const QString& artifacts){
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    QTemporaryDir dir(QDir(artifacts).filePath("assignments-XXXXXX"));check(dir.isValid(),"assignment fixture");
    MainWindow w(dir.filePath("panels.ini"));w.smokeMode_=true;w.show();w.resize(1400,900);
    w.service_.createProject("Assignments","",dir.filePath("assignments.vfproject"),"");const auto recipe=w.service_.createRecipe("Assignments","",ApplicationType::Measurement);
    const auto source=w.service_.addTool("image_file"),blob=w.service_.addTool("blob");w.service_.setConnections({{source,"image",blob,"image"}});
    QImage image(30,30,QImage::Format_Grayscale8);image.fill(255);const auto imagePath=dir.filePath("source.png");check(image.save(imagePath),"real file fixture");w.service_.setImageFile(imagePath);
    VariableDefinition count;count.name="Detected count";count.type=VariableType::Float64;const auto countId=w.service_.addVariable(count);
    VariableDefinition found;found.name="Detected flag";const auto foundId=w.service_.addVariable(found);
    VariableDefinition text;text.name="Part label";text.type=VariableType::String;const auto textId=w.service_.addVariable(text);
    check(ToolCatalog::canAdd("set_variables") && ToolCatalog::groupForKind("set_variables")=="calculation","toolbox enabled");
    const auto assignment=w.service_.addTool("set_variables");w.service_.save();w.refreshWorkspace();w.selectPage("editor");w.selectTool(assignment);
    auto button=[&](const char* name){auto* b=w.findChild<QPushButton*>(name);check(b,"assignment button");b->click();};
    auto target=[&](const QString& id){auto* c=w.findChild<QComboBox*>("assignmentTarget");c->setCurrentIndex(c->findData(id));};
    auto port=[&](const QString& name){auto* c=w.findChild<QComboBox*>("assignmentSource");int index=-1;for(int i=0;i<c->count();++i)if(c->itemData(i).toJsonObject()["port"]==name)index=i;check(index>=0,"compatible actual output picker");c->setCurrentIndex(index);};
    auto constant=[&](const QString& value){auto* e=w.findChild<QLineEdit*>("assignmentConstant");e->setText(value);QMetaObject::invokeMethod(e,"textEdited",Q_ARG(QString,value));};
    button("assignmentAdd");target(countId);port("count");check(!w.findChild<QLineEdit*>("assignmentConstant")->isEnabled(),"output disables constant");
    auto* picker=w.findChild<QComboBox*>("assignmentSource");for(int i=0;i<picker->count();++i)check(picker->itemData(i).toJsonObject()["port"]!="area","pixel area filtered");
    button("assignmentAdd");target(foundId);port("found");button("assignmentAdd");target(textId);constant("\"Fixture label\"");
    check(!w.findChild<QPushButton*>("assignmentDown")->isEnabled(),"last row down disabled");check(!w.service_.dirty(),"draft isolated until one service commit");check(w.applyToolProperties(),"atomic three row apply");check(w.service_.dirty(),"assignment dirty");
    w.service_.save();const auto saved=w.service_.activeRecipe()->definition;check(w.commitToolProperties()==PropertyCommit::Unchanged && !w.service_.dirty(),"no-op properties clean");
    button("assignmentAdd");target(textId);constant("\"duplicate\"");check(w.commitToolProperties()==PropertyCommit::Failed && w.service_.activeRecipe()->definition==saved,"duplicate target rejected atomically");check(!w.findChild<QLabel*>("assignmentError")->isHidden(),"inline duplicate error visible");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);QApplication::processEvents();w.findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(w.findChild<QLabel*>("assignmentError"));QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("assignment-error-%1.png").arg(size.width()))),"wrapped error screenshot");}
    button("assignmentCancel");QApplication::processEvents();check(w.service_.activeRecipe()->definition==saved && !w.service_.dirty(),"cancel restores saved draft without dirty");button("assignmentAdd");auto* targets=w.findChild<QComboBox*>("assignmentTarget");targets->addItem("Invalid UUID","invalid-target");targets->setCurrentIndex(targets->count()-1);constant("true");check(w.commitToolProperties()==PropertyCommit::Failed && w.service_.activeRecipe()->definition==saved,"unknown target atomic rejection");
    w.showToolProperties();w.findChild<QListWidget*>("assignmentRows")->setCurrentRow(0);w.findChild<QComboBox*>("assignmentSource")->setCurrentIndex(0);constant("\"wrong type\"");check(w.commitToolProperties()==PropertyCommit::Failed && w.service_.activeRecipe()->definition==saved,"incompatible constant rejected");
    w.showToolProperties();for(int i=0;i<3;++i)button("assignmentRemove");check(w.commitToolProperties()==PropertyCommit::Failed && w.service_.activeRecipe()->definition==saved,"empty group rejected");w.showToolProperties();
    w.service_.openProject(w.service_.path());w.service_.selectRecipe(recipe);w.refreshWorkspace();w.selectTool(assignment);
    auto run=[&]{const auto context=w.editorContext();const auto signature=w.editorSession_.signature(context);auto result=ExecutionService{}.run(context.definition,dir.path(),context.recipeId,signature,{});check(result.state==ExecutionState::Done,"actual saved Run Flow");AssignmentBundle bundle;for(const auto& tool:result.tools)if(tool.toolId==assignment)bundle=std::get<AssignmentBundle>(tool.outputs["bundle"].data);check(bundle.committed && bundle.fields.size()==3,"all three committed");check(std::get<bool>(*std::get<ValueEnvelope>(bundle.fields[foundId]).value),"real Boolean output");check(std::get<double>(*std::get<ValueEnvelope>(bundle.fields[countId]).value)==1,"real count output");check(std::get<QString>(*std::get<ValueEnvelope>(bundle.fields[textId]).value)=="Fixture label","typed constant");check(w.editorSession_.acceptCycle(std::move(result),signature,context),"accepted Editor cycle");return bundle;};
    run();w.showToolProperties();check(w.findChild<QLabel*>("assignmentValidity")->text().startsWith("Valid"),"accepted result validity");
    auto* rows=w.findChild<QListWidget*>("assignmentRows");rows->setCurrentRow(2);button("assignmentUp");check(w.findChild<QLabel*>("assignmentValidity")->text().startsWith("Missing"),"draft hides old valid result");check(w.applyToolProperties(),"row reorder apply");w.service_.save();run();w.showToolProperties();
    // Run Tool uses the existing partial Editor path and cannot acquire Runtime authority.
    const auto context=w.editorContext();const auto evaluation=w.editorSession_.evaluation(context,assignment);auto partial=ExecutionService{}.runTool(context.definition,dir.path(),context.recipeId,w.editorSession_.signature(context),{},assignment,evaluation);check(partial.partial && partial.targetToolId==assignment,"Run Tool stays partial preview");
    check(!w.runtimeCoordinator_ || w.runtimeCoordinator_->state()!=RuntimeState::Running,"preview never starts Runtime");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);for(int i=0;i<3;++i)QApplication::processEvents();rows=w.findChild<QListWidget*>("assignmentRows");rows->setFocus();rows->setCurrentRow(0);QKeyEvent down(QEvent::KeyPress,Qt::Key_Down,Qt::NoModifier);QApplication::sendEvent(rows,&down);check(rows->currentRow()==1,"selected row keyboard");auto* scroll=w.findChild<QScrollArea*>("propertiesScroll");scroll->ensureWidgetVisible(rows);QApplication::processEvents();check(rows->viewport()->visibleRegion().boundingRect().height()>0,"rows visible at both sizes");check(w.grab().save(QDir(artifacts).filePath(QString("assignment-editor-%1.png").arg(size.width()))),"two-size screenshot");scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("assignment-controls-%1.png").arg(size.width()))),"scrolled controls screenshot");}
    w.editorSession_.invalidateResults();w.showToolProperties();check(w.findChild<QLabel*>("assignmentValidity")->text().startsWith("Missing"),"stale result hidden");
    QFile report(QDir(artifacts).filePath("assignment-ui.json"));check(report.open(QIODevice::WriteOnly),"assignment report");report.write(QJsonDocument(QJsonObject{{"status","PASS"},{"threeRowsSaveReopenRunFlow",true},{"rowOrderSameInputs",true},{"duplicateInvalidEmptyRejected",true},{"compatiblePickers",true},{"runToolPartial",true},{"keyboard",true},{"visualSizes","1400x900,850x600"},{"nativeAcceptance",false}}).toJson());w.close();QTextStream(stdout)<<"PASS: Set Variables Editor UI\n";return true;
}
}
