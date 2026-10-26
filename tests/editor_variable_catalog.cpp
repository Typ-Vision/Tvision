#include "app/MainWindow.hpp"
#include "domain/AssignmentConfig.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <stdexcept>
namespace visionflow {
bool MainWindow::verifyVariableCatalog(const QString& artifacts) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    QTemporaryDir dir(QDir(artifacts).filePath("variables-XXXXXX"));check(dir.isValid(),"variable temp directory");
    MainWindow w(dir.filePath("panels.ini"));w.smokeMode_=true;w.resize(1400,900);w.show();
    w.service_.createProject("Variables","",dir.filePath("variables.vfproject"),"");
    const auto recipe=w.service_.createRecipe("Variables","",ApplicationType::Measurement);
    const auto source=w.service_.addTool("image_file");w.service_.save();w.refreshWorkspace();w.selectPage("editor");w.selectTool(source);
    auto* table=w.variableTable_;auto* tabs=w.findChild<QTabWidget*>("editorTabs");tabs->setCurrentWidget(w.findChild<QWidget*>("variableCatalog"));
    const auto baseline=ProjectRepository::encode(w.service_.project());
    auto open=[&](const QString& id,const std::function<void(QDialog*)>& action){
        std::exception_ptr failure;
        QTimer::singleShot(0,&w,[&]{auto* dialog=w.findChild<QDialog*>("variableDialog");try{check(dialog,"variable dialog");action(dialog);}catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
        w.editVariable(id);if(failure)std::rethrow_exception(failure);
    };
    auto press=[](QDialog* d,QDialogButtonBox::StandardButton b){d->findChild<QDialogButtonBox*>("variableDialogButtons")->button(b)->click();};
    open({},[&](QDialog* d){d->findChild<QLineEdit*>("variableName")->setText("Canceled");press(d,QDialogButtonBox::Cancel);});
    check(!w.service_.dirty() && ProjectRepository::encode(w.service_.project())==baseline,"add cancel no dirty");
    open({},[&](QDialog* d){d->findChild<QLineEdit*>("variableName")->setText("Local flag");d->findChild<QPlainTextEdit*>("variableComment")->setPlainText("Boolean false is a real value");press(d,QDialogButtonBox::Save);});
    check(w.service_.dirty() && w.service_.variables().size()==1,"add command dirty");
    const auto id=w.service_.variables().first().id;check(!id.isEmpty(),"generated UUID");w.service_.save();
    const auto saved=ProjectRepository::encode(w.service_.project());
    open({},[&](QDialog* d){d->findChild<QLineEdit*>("variableName")->setText("Local flag");press(d,QDialogButtonBox::Save);check(d->isVisible() && d->findChild<QLabel*>("variableDialogError")->text().contains("Duplicate"),"duplicate add error");press(d,QDialogButtonBox::Cancel);});
    check(!w.service_.dirty() && w.service_.variables().size()==1,"duplicate add atomic");
    open(id,[&](QDialog* d){check(d->findChild<QLineEdit*>("variableUuid")->isReadOnly(),"immutable UUID UI");press(d,QDialogButtonBox::Save);});
    check(!w.service_.dirty() && ProjectRepository::encode(w.service_.project())==saved,"no-op edit no dirty");
    open(id,[&](QDialog* d){d->findChild<QLineEdit*>("variableName")->setText("Changed but canceled");press(d,QDialogButtonBox::Cancel);});
    check(!w.service_.dirty() && w.service_.variables().first().name=="Local flag","edit cancel");
    const QString longName="Uzun değişken adı — " + QString(40,'X');
    open(id,[&](QDialog* d){d->findChild<QLineEdit*>("variableName")->setText(longName);press(d,QDialogButtonBox::Save);});
    check(w.service_.variables().first().id==id && w.service_.variables().first().name==longName,"rename retains UUID");
    w.selectTool(source);w.propertiesBody_->findChild<QLineEdit*>("toolName")->setText("Pending source name");
    open(id,[&](QDialog* d){press(d,QDialogButtonBox::Cancel);});
    check(w.service_.tools().first().name=="Pending source name","catalog transition commits original Properties binding");
    const auto assignment=w.service_.addTool("set_variables");
    ValueEnvelope constant;constant.value=false;constant.validity=ValueValidity::Valid;
    for(auto tool:w.service_.tools())if(tool.id==assignment){tool.parameters=encodeAssignmentConfig({assignment,1,{{id,ConstantAssignmentSource{constant}}}});w.service_.updateTool(tool);}
    w.service_.save();w.refreshWorkspace();
    table->selectRow(0);w.removeSelectedVariable();
    check(!w.service_.dirty() && w.service_.variables().size()==1 && !w.findChild<QLabel*>("variableError")->isHidden() && w.findChild<QLabel*>("variableError")->text().contains("referenced"),"referenced delete atomic visible error");
    open(id,[&](QDialog* d){d->findChild<QComboBox*>("variableType")->setCurrentIndex(int(VariableType::Float64));press(d,QDialogButtonBox::Save);check(d->isVisible() && d->findChild<QLabel*>("variableDialogError")->text().contains("referenced"),"referenced type error");press(d,QDialogButtonBox::Cancel);});
    check(!w.service_.dirty(),"rejected type no dirty");
    table->selectRow(1);check(!w.findChild<QPushButton*>("variableEdit")->isEnabled() && !w.findChild<QPushButton*>("variableRemove")->isEnabled(),"system read only actions");
    w.removeSelectedVariable();check(!w.service_.dirty() && w.service_.variables().size()==1,"system remove ignored");
    QImage image(40,40,QImage::Format_Grayscale8);image.fill(0);const auto imagePath=dir.filePath("source.png");check(image.save(imagePath),"variable source image");w.service_.setImageFile(imagePath);w.refreshWorkspace();
    const auto context=w.editorContext();const auto signature=w.editorSession_.signature(context);
    auto result=ExecutionService{}.run(context.definition,dir.path(),context.recipeId,signature,{});
    check(result.state==ExecutionState::Done && w.editorSession_.acceptCycle(std::move(result),signature,context),"actual assignment cycle accepted");
    w.refreshVariableCatalog(context);
    check(table->item(0,4)->text()=="false" && table->item(0,5)->text()=="Valid" && table->item(0,8)->text()==w.editorSession_.cycle->cycleId,"current cycle false value and validity");
    QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath("variable-current-cycle.png")),"current cycle visual");
    w.editorSession_.invalidateResults();w.refreshVariableCatalog(context);
    check(table->item(0,4)->text()=="—" && table->item(0,5)->text().startsWith("Missing"),"invalidation no stale value");
    for(int i=0;i<20;++i){VariableDefinition v;v.name=QString("Catalog row %1").arg(i);w.service_.addVariable(v);}
    w.refreshWorkspace();tabs->setCurrentWidget(w.findChild<QWidget*>("variableCatalog"));
    for(const auto size:{QSize(1400,900),QSize(850,600)}){
        w.resize(size);for(int settle=0;settle<3;++settle)QApplication::processEvents();
        table->setFocus();table->selectRow(0);QKeyEvent down(QEvent::KeyPress,Qt::Key_Down,Qt::NoModifier);QApplication::sendEvent(table,&down);check(table->currentRow()==1,"table keyboard navigation");
        check(table->horizontalScrollBar()->maximum()>0 && table->verticalScrollBar()->maximum()>0,"catalog scroll both dimensions");
        check(table->viewport()->visibleRegion().boundingRect().height()>=table->rowHeight(0)*2,"two visible catalog rows at both sizes");
        table->selectRow(0);table->scrollToTop();table->horizontalScrollBar()->setValue(0);
        check(table->item(0,0)->toolTip()==longName,"full long-name tooltip");
        check(w.grab().save(QDir(artifacts).filePath(QString("variable-catalog-%1.png").arg(size.width()))),"catalog visual");
    }
    open(id,[&](QDialog* d){d->resize(550,450);d->findChild<QLineEdit*>("variableName")->setText("");press(d,QDialogButtonBox::Save);QApplication::processEvents();check(!d->findChild<QLabel*>("variableDialogError")->text().isEmpty(),"invalid name visible error");check(d->grab().save(QDir(artifacts).filePath("variable-dialog-error.png")),"dialog error visual");press(d,QDialogButtonBox::Cancel);});
    w.service_.removeTool(assignment);w.refreshWorkspace();table->selectRow(0);w.removeSelectedVariable();check(w.service_.variables().size()==20,"unreferenced delete");
    w.service_.save();w.service_.openProject(w.service_.path());w.service_.selectRecipe(recipe);w.refreshWorkspace();check(!w.service_.dirty() && w.service_.variables().size()==20,"catalog save reopen");
    QFile report(QDir(artifacts).filePath("variable-catalog.json"));check(report.open(QIODevice::WriteOnly),"variable report");report.write(QJsonDocument(QJsonObject{{"status","PASS"},{"addRenameCancelDelete",true},{"uuidImmutable",true},{"systemReadOnly",true},{"referencedDeleteAndTypeRejected",true},{"noOpClean",true},{"currentCycleFalseAndMissing",true},{"propertiesBinding",true},{"keyboardAndScroll",true},{"visualSizes","1400x900,850x600"},{"nativeAcceptance",false}}).toJson());
    w.close();QTextStream(stdout)<<"PASS: Variable Catalog UI contracts\n";return true;
}
}
