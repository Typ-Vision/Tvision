#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include "../../tests/CameraAcquisitionFixture.hpp"
#include <QApplication>
#include <QAbstractButton>
#include <QDir>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTimer>

namespace visionflow {
bool MainWindow::verifyEditorCameraSessions(const QString& artifacts){
    using persistent_fixture::check;using persistent_fixture::Fixture;
    smokeMode_=true;
    auto wait=[&](auto condition){QElapsedTimer elapsed;elapsed.start();while(!condition()){check(elapsed.elapsed()<10000,"Editor camera fixture timeout");QApplication::processEvents();pollEditorCameras();QThread::msleep(1);}QApplication::processEvents();};
    auto settle=[&]{wait([&]{return !editorCameras_.pending() && !editorCameras_.draining() && !editorSession_.busy() && !editorCameraPendingExecution_;});};
    auto choose=[&](QMessageBox::StandardButton button,auto action){bool asked=false;QTimer choice;choice.setSingleShot(true);
        connect(&choice,&QTimer::timeout,this,[&]{if(auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){asked=true;box->button(button)->click();}});
        smokeMode_=false;choice.start(0);action();smokeMode_=true;check(asked,"Exit uses existing dirty decision");};
    QTemporaryDir dir(QDir(artifacts).filePath("editor-cameras-XXXXXX"));check(dir.isValid(),"Editor fixture directory");
    QJsonArray rows;
    auto install=[&](Fixture& fixture,ApplicationType type,const QString& name){
        editorCameras_.stop();settle();editorCameraComposition_=fixture.inputs;
        service_.createProject(name,"",dir.filePath(name+".vfproject"),"file");service_.createRecipe("Camera recipe","",type,"file");
        measurementWorkspace()->replace(fixture.inputs.contract);QStringList sources;
        for(auto& request:fixture.requests){const auto source=service_.addTool("industrial_camera");service_.bindIndustrialSource(source,fixture.inputs.contract,request.profile.id,3000);request.sourceId=source;sources.append(source);}
        service_.save();refreshWorkspace();selectPage("editor");return sources;
    };
    auto capture=[&](const QString& source){selectTool(source);runExecution(source);settle();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Done && editorSession_.frame && editorSession_.frame->metadata().sourceId==source,"Ready Editor source captures actual requested frame");};
    for(int count:{1,5})for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        const auto name=QString("c%1-%2").arg(count).arg(typeKey(type));Fixture f(name,count);const auto sources=install(f,type,name);
        check(*f.discoveries==0,"Project/Editor restore is offline");const auto id=service_.activeRecipe()->id;
        openEditorRecipe(id);settle();for(const auto& probe:f.probes)check(probe->opens==1 && probe->configs==1 && probe->grabs==0,"Explicit Editor open prepares without acquisition");
        for(int i=0;i<20;++i)for(const auto& source:sources)capture(source);
        for(const auto& probe:f.probes)check(probe->opens==1 && probe->configs==1 && probe->grabs==20 && probe->closes==0,"Editor twenty captures reuse prepared connection");
        service_.save();refreshWorkspace();check(f.first()->closes==0,"Save preserves connection");
        selectTool(sources.first());auto* toolName=propertiesBody_->findChild<QLineEdit*>("toolName");check(toolName,"tool properties");toolName->setText("Changed name");check(applyToolProperties(),"Rename edit");
        choose(QMessageBox::Cancel,[&]{for(int row=0;row<navigation_->count();++row)if(navigation_->item(row)->data(Qt::UserRole).toString()=="camera"){navigation_->setCurrentRow(row);break;}});
        check(pages_->currentIndex()==pageIds_.indexOf("editor") && navigation_->currentItem()->data(Qt::UserRole).toString()=="recipes" && editorCameras_.active() && f.first()->closes==0,"Cancelled exit preserves connection and selected page");
        if(count==5 && type==ApplicationType::Measurement){
            resize(1400,900);QApplication::processEvents();check(grab().save(QDir(artifacts).filePath("editor-cameras-normal.png")),"normal visual");
            resize(1000,700);QApplication::processEvents();check(grab().save(QDir(artifacts).filePath("editor-cameras-small.png")),"small visual");resize(1400,900);
        }
        choose(QMessageBox::Discard,[&]{selectPage("recipes");});wait([&]{return pages_->currentIndex()==pageIds_.indexOf("recipes");});settle();
        for(const auto& probe:f.probes)check(probe->closes==1 && !probe->wrongThread,"Discarded exit drains every camera on SDK thread");
        openEditorRecipe(id);settle();selectTool(sources.first());propertiesBody_->findChild<QLineEdit*>("toolName")->setText("Saved name");check(applyToolProperties(),"Save exit edit");
        choose(QMessageBox::Save,[&]{selectPage("recipes");});wait([&]{return pages_->currentIndex()==pageIds_.indexOf("recipes");});settle();
        for(const auto& probe:f.probes)check(probe->opens==2 && probe->closes==2,"Save-and-exit drains reopened context");
        rows.append(QJsonObject{{"type",typeKey(type)},{"cameras",count},{"capturesPerCamera",20},{"openBeforeExit",1},{"configureBeforeExit",1},{"closeBeforeExit",0},{"cancelExitPreserves",true},{"discardAndSaveExitDrain",true}});
    }
    {
        Fixture f("partial-editor",5);f.probes[f.requests[2].device.identity.serial]->failConfigure=true;auto sources=install(f,ApplicationType::Measurement,"partial");
        openEditorRecipe(service_.activeRecipe()->id);settle();check(editorCameras_.source(sources[2])->state==AcquisitionSessionState::Fault && editorCameraStatus(sources[2]).contains("hazır değil"),"Offline source is reported without blocking editing");capture(sources[0]);
        const auto failedOpens=f.probes[f.requests[2].device.identity.serial]->opens.load();
        auto disabled=service_.tools();for(auto tool:disabled)if(tool.id==sources[4]){tool.enabled=false;service_.updateTool(tool);}refreshWorkspace();settle();
        check(f.probes[f.requests[4].device.identity.serial]->closes==1 && f.first()->opens==1 && f.probes[f.requests[2].device.identity.serial]->opens==failedOpens,"Removing target closes only last target; no automatic failed-source retry");
        runExecution(sources[2]);settle();check(editorSession_.cycle && editorSession_.cycle->state==ExecutionState::Error && f.probes[f.requests[2].device.identity.serial]->opens==failedOpens+1,"Explicit retry fails once and does not loop");
        auto changed=f.inputs.contract;++changed.profiles[0].revision;changed.profiles[0].requested.exposureTimeUs=200;measurementWorkspace_->replace(changed);refreshWorkspace();settle();
        check(f.first()->opens==2 && f.first()->configs==2 && f.first()->closes==1,"Profile revision prepares only changed target");
        ++changed.setups[0].revision;changed.setups[0].geometry.reverse=GeometryMirror{true,false};measurementWorkspace_->replace(changed);refreshWorkspace();settle();
        check(f.first()->opens==3 && f.first()->closes==2,"Geometry revision invalidates and prepares changed target");
        choose(QMessageBox::Discard,[&]{selectPage("recipes");});settle();check(!service_.dirty() && f.first()->closes==3,"Discard does not refresh catalog snapshot back into dirty recipe");
        openEditorRecipe(service_.activeRecipe()->id);settle();findChild<QPushButton*>("stopFlow")->click();settle();check(!editorCameras_.active() && f.first()->closes==4,"Idle Stop releases lease");capture(sources[0]);check(f.first()->opens==5,"Explicit acquisition after Stop prepares again");
        service_.save();selectPage("recipes");settle();
    }
    {
        Fixture f("late-editor");auto sources=install(f,ApplicationType::Measurement,"late");const auto a=service_.activeRecipe()->id;
        service_.createRecipe("Other recipe","",ApplicationType::Measurement,"file");const auto b=service_.activeRecipe()->id;const auto other=service_.addTool("industrial_camera");service_.bindIndustrialSource(other,f.inputs.contract,f.requests.first().profile.id,3000);service_.save();refreshWorkspace();
        openEditorRecipe(a);settle();
        struct Gate {std::promise<void> promise;bool released=false;void release(){if(!released){released=true;promise.set_value();}}~Gate(){release();}} gate;
        f.first()->grabGate=gate.promise.get_future().share();runExecution(sources.first());wait([&]{return f.first()->grabEntered.load();});openEditorRecipe(b);
        check(service_.activeRecipe()->id==a && editorCameras_.draining(),"A to B keeps old owner until drain");gate.release();
        wait([&]{return service_.activeRecipe()->id==b && !editorCameras_.pending();});settle();check(!editorSession_.frame && f.first()->closes==1 && f.first()->opens==2,"Late A frame never enters B cache");
        f.first()->grabGate={};capture(other);service_.save();selectPage("recipes");settle();
    }
    {
        Fixture f("window-editor");const auto sources=install(f,ApplicationType::Measurement,"window");const auto id=service_.activeRecipe()->id;openEditorRecipe(id);settle();selectTool(sources.first());
        QApplication::setQuitOnLastWindowClosed(false);
        propertiesBody_->findChild<QLineEdit*>("toolName")->setText("Close cancel");check(applyToolProperties(),"Window dirty edit");
        choose(QMessageBox::Cancel,[&]{close();});check(isVisible() && editorCameras_.active() && f.first()->closes==0,"Window close cancellation preserves connection");
        choose(QMessageBox::Discard,[&]{close();});wait([&]{return !isVisible();});settle();check(f.first()->closes==1,"Window discard drains before close");
        show();openEditorRecipe(id);settle();selectTool(sources.first());propertiesBody_->findChild<QLineEdit*>("toolName")->setText("Close saved");check(applyToolProperties(),"Window save edit");
        choose(QMessageBox::Save,[&]{close();});wait([&]{return !isVisible();});settle();check(f.first()->closes==2 && !service_.dirty(),"Window save drains before close");show();QApplication::setQuitOnLastWindowClosed(true);
    }
    editorCameraComposition_.reset();QJsonObject report{{"status","PASS"},{"physicalAcceptance",false},{"matrix",rows},
        {"cases",QJsonArray{"offlineRestore","explicitOpen","repeat20PerCamera","saveReuse","cancelExit","discardExit","saveExit","partialFive","targetRemoval","explicitRetry","profileRevision","geometryRevision","idleStopReopen","lateFrameRecipeSwitch","windowCancel","windowDiscardDrain","windowSaveDrain"}}};
    QSaveFile output(QDir(artifacts).filePath("editor-camera-sessions.json"));check(output.open(QIODevice::WriteOnly),"report open");output.write(QJsonDocument(report).toJson());check(output.commit(),"report save");
    QTextStream(stdout)<<"PASS: Editor camera session lifecycle\n";return true;
}
}
