#include "app/MainWindow.hpp"
#include "application/EditorObservation.hpp"
#include <QApplication>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
#include <algorithm>

namespace visionflow {
bool MainWindow::verifyEditorPropertyCommits(const QString& artifacts) {
    using namespace editor_observation;
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    QTemporaryDir dir(QDir(artifacts).filePath("property-commits-XXXXXX"));check(dir.isValid(),"Properties temporary directory");
    MainWindow w(dir.filePath("panels.ini"));w.smokeMode_=true;w.resize(1400,900);w.show();
    w.service_.createProject("Properties","",dir.filePath("fixture.vfproject"),"");
    const auto recipe=w.service_.createRecipe("Properties","",ApplicationType::Measurement);
    const auto source=w.service_.addTool("image_file");
    QString geometry,point,scalar;
    for(const auto kind:{"template_match","blob","edge","pattern_pose","shape_pose","edge_point","edge_pair","line_fit","circle_fit","code_read","preprocess","geometry_measurement","measurement"}) {
        const auto id=w.service_.addTool(kind);
        for(auto tool:w.service_.tools())if(tool.id==id) {
            if(tool.kind=="template_match")tool.parameters["minScore"]=.73526712345;
            if(tool.kind=="geometry_measurement") {
                geometry=id;
                for(const auto key:{"toleranceEnabled","nominal","lower","upper"})tool.parameters.remove(key);
            }
            if(tool.kind=="edge_point")point=id;
            if(tool.kind=="measurement")scalar=id;
            w.service_.updateTool(tool);
        }
    }
    w.service_.save();w.refreshWorkspace();w.selectPage("editor");
    // Every unedited control must preserve the exact serialized recipe, including
    // rounded scores and omitted legacy tolerance fields, on commit and Save/reopen.
    for(const auto& tool:w.service_.tools()) {
        w.selectTool(tool.id);const auto before=ProjectRepository::encode(w.service_.project());
        const auto hash=QCryptographicHash::hash(before,QCryptographicHash::Sha256);
        const auto status=w.service_.activeRecipe()->status;
        Sample sample;
        {Binding binding(sample);check(w.commitToolProperties()==PropertyCommit::Unchanged,"unedited Properties result");check(w.applyToolProperties(),"unedited Properties apply");}
        check(sample.calls[Workspace]==0 && sample.calls[Editor]==0 && sample.calls[Context]==0,"no-op Properties must not refresh");
        check(!w.service_.dirty() && w.service_.activeRecipe()->status==status && QCryptographicHash::hash(ProjectRepository::encode(w.service_.project()),QCryptographicHash::Sha256)==hash,"no-op hash/status/dirty");
        {Binding binding(sample);w.saveProject();}
        check(sample.calls[Calibration]==0 && sample.calls[Runtime]==0,"Save must not rebuild hidden pages");
        check(sample.calls[Editor]==1 && sample.calls[ProjectSave]==1 && sample.calls[AtomicWrite]==0,"no-op Save single refresh and no write");
        check(ProjectRepository::encode(ProjectRepository{}.load(w.service_.path()))==before,"no-op disk/reopen equality");
        w.propertiesBody_->findChild<QLineEdit*>("toolName")->setText(tool.name+" edited");
        check(w.commitToolProperties()==PropertyCommit::Changed,"metadata edit result");
        for(const auto& edited:w.service_.tools())if(edited.id==tool.id)check(edited.parameters==tool.parameters,"metadata edit retains precise/legacy parameters");
        w.service_.save();
    }
    check(w.findChild<QPushButton*>("saveWorkspace")->focusPolicy()==Qt::TabFocus && w.findChild<QPushButton*>("addTool")->focusPolicy()==Qt::TabFocus,"mouse Save/Add retain pending editor focus");
    w.selectTool(scalar);auto* limit=w.propertiesBody_->findChild<QDoubleSpinBox*>("upperMm");limit->setValue(limit->value()+1);QApplication::processEvents();limit->setFocus();QApplication::processEvents();
    Sample numericSave;{Binding binding(numericSave);w.findChild<QPushButton*>("saveWorkspace")->click();}
    check(numericSave.calls[Editor]==1 && numericSave.calls[AtomicWrite]==1,"pending numeric Save has one refresh/write");
    check(limit->hasFocus(),"pending numeric Save retains focus");
    for(const auto& tool:w.service_.tools())if(tool.id==scalar)check(tool.parameters["upperMm"].toDouble()==limit->value(),"pending numeric final value persisted");
    w.selectTool(source);auto* name=w.propertiesBody_->findChild<QLineEdit*>("toolName");
    name->setFocus();QApplication::processEvents();
    QPointer<QLineEdit> retained=name;
    // setText leaves the final value pending (no textEdited/editingFinished).
    name->setText("Last pending name");
    Sample saved;{Binding binding(saved);w.saveProject();}
    check(saved.calls[Editor]==1 && saved.calls[Workspace]==1 && saved.calls[ProjectSave]==1 && saved.calls[AtomicWrite]==1,"pending Save commits once and refreshes once");
    check(!w.service_.dirty() && retained && retained==w.propertiesBody_->findChild<QLineEdit*>("toolName") && retained->hasFocus(),"Save retains selection/control/focus");
    check(ProjectRepository{}.load(w.service_.path()).recipes.first().definition==w.service_.activeRecipe()->definition && w.service_.tools().first().name=="Last pending name","pending final value persisted");
    // External-write conflict preserves both the draft and the previously saved file.
    auto bytes=[&]{QFile f(w.service_.path());check(f.open(QIODevice::ReadOnly),"read property fixture");return f.readAll();};
    const auto disk=bytes();QFile external(w.service_.path());check(external.open(QIODevice::Append),"external Save conflict");external.write(" ");external.close();
    retained->setText("Still dirty after failed Save");const auto version=w.service_.activeRecipe()->recipeVersion;
    Sample failedSave;{Binding binding(failedSave);w.saveProject();}
    check(w.service_.dirty() && w.service_.activeRecipe()->recipeVersion==version && retained && retained->text()=="Still dirty after failed Save" && !w.workspaceError_->isHidden(),"failed Save retains draft/version/controls and visible error");
    check(failedSave.calls[Editor]==1 && failedSave.calls[AtomicWrite]==0 && bytes()==disk+" ","failed Save refreshes once without disk write");
    check(external.open(QIODevice::WriteOnly|QIODevice::Truncate),"restore conflict fixture");external.write(disk);external.close();w.saveProject();
    for(int i=0;i<w.toolbox_->count();++i)if(w.toolbox_->item(i)->data(Qt::UserRole)=="preprocess")w.toolbox_->setCurrentRow(i);
    auto* catalogChoice=w.toolbox_->currentItem();w.toolbox_->setFocus();QApplication::processEvents();
    retained->setText("Pending before Add");const auto count=w.service_.tools().size();
    Sample added;{Binding binding(added);w.addSelectedTool();}
    check(added.calls[Editor]==1 && w.service_.tools().size()==count+1 && w.service_.tools().first().name=="Pending before Add","Add commits pending value and adds one tool with one refresh");
    check(w.selectedToolId_!=source && w.propertiesBody_->findChild<QLineEdit*>("toolName") && w.toolbox_->currentItem()==catalogChoice && w.toolbox_->hasFocus(),"Add preserves catalog focus and selects added tool");
    w.service_.save();w.selectTool(source);
    name=w.propertiesBody_->findChild<QLineEdit*>("toolName");name->setText("");
    const auto valid=ProjectRepository::encode(w.service_.project());
    Sample rejected;{Binding binding(rejected);w.saveProject();w.addSelectedTool();}
    check(rejected.calls[ProjectSave]==0 && rejected.calls[Editor]==0 && !w.service_.dirty() && ProjectRepository::encode(w.service_.project())==valid && w.service_.tools().size()==count+1,"invalid pending Properties block Save and Add without mutation");
    check(!w.workspaceError_->isHidden(),"validation error remains visible");
    name->setText(w.service_.tools().first().name);
    w.selectTool(geometry);
    auto links=editorConnections(w.service_.activeRecipe()->definition);
    links.append({point,"point",geometry,"p1"});links.append({point,"point",geometry,"p2"});w.service_.setConnections(links);w.showToolProperties();
    auto* operation=w.propertiesBody_->findChild<QComboBox*>("gm_operation");
    {const QSignalBlocker block(operation);operation->setCurrentText("line_intersection");}
    check(w.commitToolProperties()==PropertyCommit::Changed,"geometry operation mutation");
    const auto changedLinks=editorConnections(w.service_.activeRecipe()->definition);
    for(const auto& link:changedLinks)if(link.targetTool==geometry)check(link.targetPort=="image","geometry operation clears non-image connections");
    check(std::any_of(changedLinks.begin(),changedLinks.end(),[&](const auto& link){return link.targetTool==geometry && link.targetPort=="image";}),"geometry image link retained");
    w.refreshWorkspace();
    auto* lower=w.propertiesBody_->findChild<QDoubleSpinBox*>("gm_lower");lower->setValue(5);
    const auto geometryBefore=w.service_.activeRecipe()->definition;
    check(w.commitToolProperties()==PropertyCommit::Failed && w.service_.activeRecipe()->definition==geometryBefore,"invalid geometry remains atomic");
    lower->setValue(0);check(w.commitToolProperties()==PropertyCommit::Unchanged,"correcting rejected value restores no-op");
    w.service_.save();w.refreshWorkspace();w.selectTool(w.service_.tools().last().id);
    for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);for(int settle=0;settle<3;++settle)QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("property-commits-%1.png").arg(size.width()))),"Properties visuals");}
    w.service_.openProject(w.service_.path());w.service_.selectRecipe(recipe);
    check(!w.service_.dirty(),"Properties final reopen clean");
    Sample headerOnly;{Binding binding(headerOnly);w.refreshWorkspaceHeader();}
    check(headerOnly.calls[Editor]==0 && headerOnly.calls[Calibration]==0 && headerOnly.calls[Runtime]==0,"header update does not rebuild content");
    // A hidden page accumulates invalidations, then consumes them once on entry.
    w.refreshWorkspace();
    check(w.calibrationRefresh_.generation>w.calibrationRefresh_.renderedGeneration && !w.calibrationRefresh_.reason.isEmpty(),"hidden Calibration pending generation/reason");
    Sample calibrationEntry;{Binding binding(calibrationEntry);w.selectPage("calibration");}
    check(calibrationEntry.calls[Calibration]==1 && w.calibrationRefresh_.generation==w.calibrationRefresh_.renderedGeneration && w.calibrationRefresh_.reason.isEmpty(),"Calibration entry consumes generation exactly once");
    Sample runtimeEntry;{Binding binding(runtimeEntry);w.selectPage("runtime");}
    check(runtimeEntry.calls[Runtime]==1 && w.runtimeRefresh_.generation==w.runtimeRefresh_.renderedGeneration,"Runtime entry consumes generation once");
    // Changing projects while pages are hidden cannot retain the old model/cards.
    w.service_.createProject("Empty next project","",dir.filePath("next.vfproject"),"");w.service_.save();
    w.refreshWorkspace();w.selectPage("calibration");
    check(w.findChild<QLabel*>("overviewDetail")->text().isEmpty(),"new project clears old Calibration detail");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("page-refresh-%1.png").arg(size.width()))),"page refresh visuals");}
    w.selectPage("runtime");
    check(w.findChild<QLabel*>("runtimeStatus")->text().contains("Load Recipe"),"new project Runtime unloaded");
    w.service_.createProject("Broken environment","",dir.filePath("broken.vfproject"),"");w.service_.save();
    QFile broken(w.service_.path()+".measurement-environment.json");check(broken.open(QIODevice::WriteOnly),"error fixture sidecar");broken.write("{}");broken.close();
    w.refreshWorkspace();check(!w.workspaceError_->isHidden(),"workspace initialization error remains visible");w.selectPage("calibration");
    check(!w.findChild<QLabel*>("overviewStatus")->text().isEmpty() && w.findChild<QLabel*>("overviewDetail")->text().isEmpty(),"error entry clears obsolete Calibration details");
    w.service_.openProject(dir.filePath("fixture.vfproject"));w.service_.selectRecipe(recipe);w.refreshWorkspace();w.selectPage("calibration");
    check(w.calibrationRefresh_.generation==w.calibrationRefresh_.renderedGeneration,"return project consumes new generation");
    QFile report(QDir(artifacts).filePath("property-commits.json"));check(report.open(QIODevice::WriteOnly),"Properties report");
    report.write(QJsonDocument(QJsonObject{{"status","PASS"},{"uneditedToolKinds",14},{"noOpPropertyRefreshes",0},
        {"pendingSaveEditorRefreshes",static_cast<int>(saved.calls[Editor])},{"pendingAddEditorRefreshes",static_cast<int>(added.calls[Editor])},
        {"failedSaveEditorRefreshes",static_cast<int>(failedSave.calls[Editor])},{"rejectedSaveCalls",static_cast<int>(rejected.calls[ProjectSave])},
        {"preservesLegacyAndPreciseParameters",true},{"geometryConnectionCleanup",true},{"saveAndAddFocus",true},{"hiddenCalibrationSaveRefreshes",0},{"calibrationEntryRefreshes",static_cast<int>(calibrationEntry.calls[Calibration])},{"runtimeEntryRefreshes",static_cast<int>(runtimeEntry.calls[Runtime])},{"physicalAcceptance",false}}).toJson());report.close();
    w.close();activateWindow();QApplication::processEvents();
    QTextStream(stdout)<<"PASS: Properties no-op, pending Save/Add, conflict, validation, geometry and focus contracts\n";
    return true;
}
}
