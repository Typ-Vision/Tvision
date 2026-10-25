#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QLockFile>
#include <QTextStream>
#include <QElapsedTimer>
#include <QThread>
namespace visionflow {
bool MainWindow::verifyRuntimeFailures(const QString& artifacts){
    int checks=0;auto check=[&](bool value,const char* text){++checks;if(!value)throw std::runtime_error(text);};
    auto button=[&](const char* name){return findChild<QPushButton*>(name);};
    auto shot=[&](const QString& name){setWindowState(Qt::WindowNoState);resize(1400,900);QApplication::processEvents();check(grab().save(QDir(artifacts).filePath("c1-"+name+"-normal.png")),"normal image");resize(850,600);QApplication::processEvents();check(grab().save(QDir(artifacts).filePath("c1-"+name+"-small.png")),"small image");};
    QTemporaryDir dir(QDir(artifacts).filePath("c1-failures-XXXXXX"));check(dir.isValid(),"fixture");
    const auto file=dir.filePath("frame.png");QImage image(100,80,QImage::Format_Grayscale8);image.fill(80);check(image.save(file),"fixture raster");
    for(bool multi:{false,true})for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        const auto tag=typeKey(type)+(multi ? "-editor9":"-legacy");
        service_.createProject("C1","",dir.filePath(tag+".vfproject"),"file");service_.createRecipe("Hata kurtarma","",type,"file");if(multi)service_.enableMultiSource();service_.setImageFile(file);service_.save();refreshWorkspace();{QElapsedTimer timer;timer.start();while((editorSession_.busy() || cameraSession_.busy()) && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();}ensureRuntime();runtimeCoordinator_->load(*service_.activeRecipe(),dir.path());selectPage("runtime");runtimeAction("run");if(runtimeCoordinator_->state()!=RuntimeState::Running)QTextStream(stderr)<<runtimeStatus_->text()<<"\n"<<runtimeSummary_->text()<<"\n";check(runtimeCoordinator_->state()==RuntimeState::Running,"raw ready");
        runtimeCycle_=runtimeCoordinator_->begin(RuntimeTrigger::Manual);const auto result=RuntimeService::execute(runtimeCycle_,runtimePath_+".calibrations.json",runtimeCoordinator_->cancellation());check(result.state==ExecutionState::Done,"actual file execution");
        runtimeAction("cycle");check(runtimeSummary_->text().contains("Meşgul") && runtime_->records().isEmpty(),"Busy does not admit second cycle");shot("busy-"+tag);
        QLockFile lock(runtimePath_+".production/archive.lock");check(lock.tryLock(),"archive lock");completeRuntimeResult(result,runtimeCoordinator_->cancellation());
        check(runtimePending_ && runtimeCoordinator_->recordingPending() && button("runtime_retry")->isVisible() && !button("runtime_cycle")->isEnabled() && !runtimeViewer_->hasImage(),"only persistence offers retry");shot("persistence-"+tag);
        const auto frozen=*runtimePending_;runtimeAction("retry");check(runtimePending_ && RuntimeService::sameCompletion(*runtimePending_,frozen),"locked retry retains exact completion");lock.unlock();
        // Removing the source proves retry does not reacquire or execute.
        check(QFile::rename(file,file+".held"),"hide source before retry");runtimeAction("retry");check(!runtimePending_ && runtimeResult_ && RuntimeService::sameCompletion(*runtimeResult_,result) && runtime_->records().size()==1,"retry records same completion without acquire/execute");check(QFile::rename(file+".held",file),"restore source");
        runtimeAction("stop");runtimeAction("run");runtimeCycle_=runtimeCoordinator_->begin(RuntimeTrigger::Manual);auto late=RuntimeService::execute(runtimeCycle_,runtimePath_+".calibrations.json",runtimeCoordinator_->cancellation());const auto cancel=runtimeCoordinator_->cancellation();runtimeAction("stop");completeRuntimeResult(late,cancel);
        check(runtimeResult_ && runtimeResult_->state==ExecutionState::Skipped && !runtimeViewer_->hasImage() && runtimeCoordinator_->state()==RuntimeState::LoadedStopped,"late Stop suppresses real completion");check(runtimePreparation_->item(0,2)->text().contains("Durduruldu"),"Stop source row cannot advertise DONE");shot("stop-"+tag);
        runtimeAction("run");runtimeCycle_=runtimeCoordinator_->begin(RuntimeTrigger::Manual);auto rejected=RuntimeService::execute(runtimeCycle_,runtimePath_+".calibrations.json",runtimeCoordinator_->cancellation());rejected.recipeId="foreign-recipe";completeRuntimeResult(rejected,runtimeCoordinator_->cancellation());
        check(!runtimePending_ && !runtimeCoordinator_->recordingPending() && !button("runtime_retry")->isVisible() && runtime_->busy() && runtimeCoordinator_->failure()==RuntimeFailure::InvalidCompletion,"invalid completion has no retry and retains gate");shot("invalid-"+tag);
        // Retire this isolated invalid lease; the product intentionally has no unsafe discard action.
        if(plc_)plc_->bind(nullptr,nullptr);runtimeCoordinator_.reset();runtime_.reset();runtimeCycle_={};runtimeResult_.reset();
        ensureRuntime();runtimeCoordinator_->load(*service_.activeRecipe(),dir.path());runtimeAction("run");runtimeCycle_=runtimeCoordinator_->begin(RuntimeTrigger::Manual);check(QFile::rename(file,file+".held"),"hide source for acquisition failure");const auto failed=RuntimeService::execute(runtimeCycle_,runtimePath_+".calibrations.json",runtimeCoordinator_->cancellation());check(lock.tryLock(),"acquisition terminal lock");completeRuntimeResult(failed,runtimeCoordinator_->cancellation());check(runtimePending_ && runtimeCoordinator_->recordingPending() && runtimeCoordinator_->failure()==RuntimeFailure::Acquisition,"first acquisition reason survives disk failure");shot("acquisition-persistence-"+tag);lock.unlock();runtimeAction("retry");check(QFile::rename(file+".held",file),"restore failed source");
        check(failed.state==ExecutionState::Error && runtimeCoordinator_->failure()==RuntimeFailure::Acquisition && !button("runtime_retry")->isVisible(),"acquisition failure offers verification not record retry");const auto first=runtimeCoordinator_->error();runtimeAction("run");check(runtimeCoordinator_->error()==first,"first acquisition cause survives later rejected command");shot("acquisition-"+tag);
        runtimeAction("stop");const auto preparation=runtimeCoordinator_->requestStart();try{runtimeCoordinator_->completeStart(preparation,"Kaynak / model bağımlılığı değişti (fixture)");}catch(const ProductionException&){refreshRuntime();}check(runtimeCoordinator_->failure()==RuntimeFailure::Dependency && runtimeCoordinator_->state()==RuntimeState::Fault && !button("runtime_retry")->isVisible(),"dependency failure offers reprepare");check(!runtimeSummary_->text().contains("READY"),"empty result panel cannot claim readiness during Fault");shot("dependency-"+tag);runtimeAction("stop");
    }
    QTextStream(stdout)<<checks<<" Runtime failure/recovery UI checks passed\n";return true;
}
}
