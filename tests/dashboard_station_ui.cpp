#include "app/MainWindow.hpp"
#include "application/CommunicationWorkspace.hpp"
#include "application/MachineSetup.hpp"
#include "application/PublicationPublisher.hpp"
#include <QApplication>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QImage>
#include <QElapsedTimer>
namespace visionflow {
bool MainWindow::verifyVf04Dashboard(const QString& artifacts){
    auto check=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
    check(communicationWorkspace_,"Normal main composition bound");
    const QDir root(artifacts);const auto marker=root.filePath("vf04-stage");
    auto writeStage=[&](QByteArray value){QFile f(marker);check(f.open(QIODevice::WriteOnly),"Stage write");f.write(value);};
    auto restart=[&]{QProcess child;auto environment=QProcessEnvironment::systemEnvironment();environment.insert("VISIONFLOW_TEST_ARTIFACTS",root.path());child.setProcessEnvironment(environment);child.start(QCoreApplication::applicationFilePath(),{"--vf04-smoke","--vf04-child"});check(child.waitForStarted(),"Restart normal executable");check(child.waitForFinished(60000),"Restart completed");QTextStream(stdout)<<child.readAllStandardOutput();QTextStream(stderr)<<child.readAllStandardError();check(child.exitStatus()==QProcess::NormalExit && child.exitCode()==0,"Child acceptance");};
    auto row=[&]{const auto rows=communicationWorkspace_->stations();check(rows.size()==1,"One product-created station");return rows[0];};
    if(!QFile::exists(marker)){
        selectPage("communication");findChild<QPushButton*>("machineSetupStart")->click();
        QImage image(40,24,QImage::Format_Grayscale8);image.fill(255);const auto stationPath=root.filePath("vf04-camera.png");check(image.save(stationPath),"Station image");
        bool submitted=false;
        QTimer::singleShot(0,this,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;dialog->findChild<QLineEdit*>("stationName")->setText("İstasyon 1");dialog->findChild<QLineEdit*>("stationSourcePath")->setText(stationPath);auto* buttons=dialog->findChild<QDialogButtonBox*>();buttons->button(QDialogButtonBox::Save)->click();submitted=true;});
        findChild<QPushButton*>("stationAdd")->click();check(submitted && row().source.has_value(),"Station saved through normal source dialog");
        bool restartNotice=false;try{communicationWorkspace_->previewRecipe(row().station.id,0);}catch(const std::exception& e){restartNotice=QString::fromUtf8(e.what()).contains("yeniden açın");}check(restartNotice,"Preview before restart explains required restart");
        const auto t=MachineSetup::defaults(communicationWorkspace_->sourceCatalogue().installation)[0];
        image.fill(0);const auto recipePath=root.filePath("vf04-recipe.png");check(image.save(recipePath),"Distinct recipe raster");
        for(const auto& name:{QString("Yüzey A"),QString("Yüzey B"),QString("Uyumsuz C")}){
            service_.createProject(name,"",root.filePath(name+".vfproject"),"file");service_.createRecipe(name,"",ApplicationType::DefectInspection,"file");service_.enableMultiSource();service_.setImageFile(recipePath);
            if(name=="Uyumsuz C"){service_.enableMultiSource();const auto extra=service_.addTool("image_file");service_.setImageFile(recipePath,extra);}
            service_.setPublicationAssignments(PublicationAssignments{t.id,t.revision,{}});service_.save();refreshWorkspace();selectPage("editor");
            findChild<QPushButton*>("publishRecipeV4")->click();check(!communicationWorkspace_->recipes().isEmpty(),"Published through actual editor action");
        }
        check(!row().snapshot,"Publishing never selects production recipe");writeStage("select");restart();return true;
    }
    QFile stageFile(marker);check(stageFile.open(QIODevice::ReadOnly),"Stage read");const auto stage=stageFile.readAll();stageFile.close();
    selectPage("dashboard");refreshDashboardStations();QApplication::processEvents();
    check(dashboardStations_->rowCount()==1 && dashboardPublished_->count()==3,"Dashboard product catalogue");
    if(stage=="select"){
        check(!row().snapshot && dashboardLoad_->text()=="Reçete yükle","First selection label");
        auto choose=[&](QString name){int index=-1;for(int n=0;n<dashboardPublished_->count();++n)if(dashboardPublished_->itemText(n).startsWith(name+" /"))index=n;check(index>=0,"Published choice");dashboardPublished_->setCurrentIndex(index);};
        choose("Yüzey A");findChild<QPushButton*>("dashboardCheckRecipe")->click();check(dashboardStationMessage_->text().contains("Uyumlu"),"Read-only compatibility preview");
        dashboardLoad_->click();check(row().snapshot && row().snapshot->selection.recipe.name=="Yüzey A","Dashboard loads A");check(row().snapshot->selection.recipe.definition["editor"].toObject()["schemaVersion"]==9,"Dashboard loads unchanged editor9 publication");const auto a=row().snapshot;
        check(dashboardLoad_->text()=="Reçete değiştir","Subsequent selection label");
        choose("Yüzey B");dashboardLoad_->click();const auto b=row().snapshot;check(b->selection.recipe.name=="Yüzey B" && a->activation!=b->activation && !row().automatic,"B changes activation without Auto");
        choose("Uyumsuz C");findChild<QPushButton*>("dashboardCheckRecipe")->click();check(dashboardStationMessage_->text().contains("Uyumsuz"),"Incompatible preview");dashboardLoad_->click();check(row().snapshot->activation==b->activation && dashboardStationMessage_->text().contains("yüklenemedi"),"Incompatible C keeps B activation");
        // Editing, saving and publishing another draft must leave B selected.
        service_.openProject(root.filePath("Yüzey A.vfproject"));service_.selectRecipe(service_.project().recipes[0].id);service_.save();refreshWorkspace();selectPage("editor");findChild<QPushButton*>("publishRecipeV4")->click();check(row().snapshot->activation==b->activation,"Editor open/save/publish does not switch station");selectPage("dashboard");refreshDashboardStations();
        auto* start=findChild<QPushButton*>("dashboardStartAutomatic");check(start,"Automatic production action exists");start->click();
        QElapsedTimer preparation;preparation.start();while(communicationWorkspace_->automaticProductionBusy() && preparation.elapsed()<10000){communicationWorkspace_->poll();QApplication::processEvents();}
        check(row().automatic && !communicationWorkspace_->remoteEnabled(),"Dashboard single action enables actual Run/Auto without Remote");refreshDashboardStations();
        check(dashboardAutoStatus_->text().contains("doğrulandı"),"Actual Auto completion displayed");
        for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();check(grab().save(root.filePath(QString("vf04-dashboard-%1x%2.png").arg(size.width()).arg(size.height()))),"Dashboard visual");check(dashboardLoad_->isVisible() && dashboardLoad_->geometry().width()>100,"Load action reachable");}
        communicationWorkspace_->command(row().station.id,{},"stop");communicationWorkspace_->poll();
        const auto sourcePath=row().source->path;check(QFile::rename(sourcePath,sourcePath+".vf06-held"),"Hide product source");start->click();
        check(!row().automatic && row().runtime.mode==StationMode::Stopped && dashboardAutoStatus_->text().contains("Kaynak"),"Missing source explains correction and does not start");
        check(grab().save(root.filePath("vf06-dashboard-missing-source.png")),"Missing source visual");check(QFile::rename(sourcePath+".vf06-held",sourcePath),"Restore product source");
        start->click();preparation.restart();while(communicationWorkspace_->automaticProductionBusy() && preparation.elapsed()<10000){communicationWorkspace_->poll();QApplication::processEvents();}check(row().automatic,"Restart acceptance begins from actual Auto");
        QFile expected(root.filePath("vf04-activation"));check(expected.open(QIODevice::WriteOnly),"Expected activation");expected.write(b->activation.toRfc4122());expected.close();writeStage("restore");restart();return true;
    }
    check(stage=="restore","Known acceptance stage");QFile expected(root.filePath("vf04-activation"));check(expected.open(QIODevice::ReadOnly),"Read expected activation");
    const auto restored=row();check(restored.snapshot && restored.snapshot->activation.toRfc4122()==expected.readAll() && restored.snapshot->selection.recipe.name=="Yüzey B","Normal executable restart restores B");
    check(restored.runtime.mode==StationMode::Stopped && !restored.automatic && !communicationWorkspace_->remoteEnabled(),"Restart never restores production permissions");
    check(restored.source && restored.snapshot->selection.resolvedSource==restored.source,"Restored station source exact");
    check(grab().save(root.filePath("vf04-dashboard-restart.png")),"Restart visual");return true;
}
}
