#include "app/MainWindow.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QImage>
#include <QListWidget>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextStream>
#include <QKeyEvent>
#include <QTabWidget>
#include <QStackedWidget>
#include <stdexcept>
namespace visionflow {
bool MainWindow::verifyResultUi(const QString& artifacts){
    auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    QTemporaryDir dir(QDir(artifacts).filePath("result-ui-XXXXXX"));check(dir.isValid(),"fixture directory");
    MainWindow w(dir.filePath("panels.ini"));w.smokeMode_=true;w.show();w.resize(1400,900);
    w.service_.createProject("Result","",dir.filePath("result.vfproject"),"");const auto recipe=w.service_.createRecipe("Part","",ApplicationType::DefectInspection);
    QImage image(30,30,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("part.png")),"actual image");w.service_.setImageFile(dir.filePath("part.png"));const auto blob=w.service_.addTool("blob"),result=w.service_.addTool("result");w.service_.save();w.refreshWorkspace();w.selectPage("editor");w.selectTool(result);
    auto* picker=w.findChild<QListWidget*>("resultSources");check(picker && picker->count()==1,"Boolean compatible picker filters numeric ports");picker->setFocus();picker->setCurrentRow(0);QKeyEvent space(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(picker,&space);check(picker->item(0)->checkState()==Qt::Checked,"keyboard selects explicit Boolean");
    w.findChild<QComboBox*>("resultOperator")->setCurrentText("ANY");check(w.applyToolProperties(),"apply ANY rule");w.showToolProperties();w.findChild<QCheckBox*>("toolEnabled")->setChecked(true);check(w.applyToolProperties(),"enable configured rule");w.service_.save();const auto saved=w.service_.activeRecipe()->definition;w.service_.openProject(w.service_.path());w.service_.selectRecipe(recipe);w.refreshWorkspace();w.selectTool(result);check(w.service_.activeRecipe()->definition==saved && w.commitToolProperties()==PropertyCommit::Unchanged,"rule save/reopen/no-op");
    auto run=[&]{const auto context=w.editorContext();const auto signature=w.editorSession_.signature(context);auto c=ExecutionService{}.run(context.definition,dir.path(),recipe,signature,{});check(c.state==ExecutionState::Done,"actual Editor flow");check(w.editorSession_.acceptCycle(c,signature,context),"accept current cycle");w.showExecutionResult();return c;};
    check(run().decision()==ResultDecision::Ok && w.matchInfo_->text().contains("Genel karar: OK"),"Editor general OK separate from DONE");
    w.findChild<QListWidget*>("resultSources")->item(0)->setCheckState(Qt::Unchecked);check(w.commitToolProperties()==PropertyCommit::Failed,"empty rule rejected");check(!w.findChild<QLabel*>("resultError")->isHidden(),"inline long error");
    auto* stale=w.findChild<QListWidget*>("resultSources")->item(0);auto binding=stale->data(Qt::UserRole).toJsonObject();binding["toolId"]=QString(128,'x');stale->setData(Qt::UserRole,binding);stale->setCheckState(Qt::Checked);check(w.commitToolProperties()==PropertyCommit::Failed,"stale binding rejected with long graph diagnostic");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);QApplication::processEvents();w.findChild<QScrollArea*>("propertiesScroll")->ensureWidgetVisible(w.findChild<QLabel*>("resultError"));QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("result-error-%1.png").arg(size.width()))),"error screenshot");}
    w.showToolProperties();
    for(auto t:w.service_.tools())if(t.id==blob){t.enabled=false;w.service_.updateTool(t);}w.editorSession_.reconcile(w.editorContext());check(run().decision()==ResultDecision::NotEvaluated && !w.matchInfo_->text().contains("Genel karar: OK"),"missing source removes old OK summary");
    for(auto t:w.service_.tools())if(t.id==blob){t.enabled=true;w.service_.updateTool(t);}w.service_.save();w.refreshWorkspace();w.selectTool(result);run();w.findChild<QTabWidget*>("editorTabs")->setCurrentIndex(1);
    for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);QApplication::processEvents();auto* scroll=w.findChild<QScrollArea*>("propertiesScroll");scroll->verticalScrollBar()->setValue(0);QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("result-editor-%1.png").arg(size.width()))),"Editor screenshot");scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("result-controls-%1.png").arg(size.width()))),"controls screenshot");}
    w.selectPage("runtime");check(w.pages_->currentIndex()==w.pageIds_.indexOf("runtime"),"Runtime page actually selected");
    w.runtime_=std::make_unique<RuntimeService>(dir.filePath("production"),dir.filePath("calibrations.json"),RuntimeIdentity::localEngineer());w.runtimeCoordinator_=std::make_unique<RuntimeCoordinator>(*w.runtime_);w.runtimeCoordinator_->load(*w.service_.activeRecipe(),dir.path());w.runtimeCoordinator_->start();auto lease=w.runtimeCoordinator_->begin(RuntimeTrigger::Manual);auto c=RuntimeService::execute(lease,dir.filePath("calibrations.json"),{});check(c.decision()==ResultDecision::Ok,"actual Runtime decision");w.runtimeCoordinator_->finish(lease.ticket,c);ProductionResultCodec::validate(w.runtime_->records().first());w.runtimeResult_=c;w.runtimePath_=w.service_.path();w.runtimeWorkspaceKey_=w.measurementWorkspaceKey_;w.runtimeProjectId_=w.service_.project().id;w.refreshRuntime();w.showRuntimeResult();check(w.runtimeSummary_->text().contains("Genel karar: OK"),"Runtime read-only general decision");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);for(int i=0;i<3;++i)QApplication::processEvents();w.resize(size);QApplication::processEvents();check(w.size()==size,"exact Runtime capture size");check(w.grab().save(QDir(artifacts).filePath(QString("result-runtime-%1.png").arg(size.width()))),"Runtime screenshot");}
    w.runtimeCoordinator_->stop();w.runtimeResult_.reset();w.showRuntimeResult();check(!w.runtimeSummary_->text().contains("Genel karar: OK"),"Stop clears old summary");w.close();QTextStream(stdout)<<"PASS: Result UI, Boolean picker, keyboard, empty rule, missing source, save/reopen, Editor/Runtime and two sizes\n";return true;
}
}
