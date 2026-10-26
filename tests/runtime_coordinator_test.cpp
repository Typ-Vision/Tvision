#include "application/RuntimeCoordinator.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QLockFile>
#include <QTextStream>
#include <future>
using namespace visionflow;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F action,ProductionError code){try{action();}catch(const ProductionException& e){check(e.code()==code,e.what());return;}throw std::runtime_error("Expected rejection");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/coordinator-XXXXXX");check(dir.isValid(),"fixture");
    ProjectService project;project.createProject("Coordinator","",dir.path()+"/part.vfproject","file");
    project.createRecipe("Saved","",ApplicationType::DefectInspection,"file");
    const auto path=dir.path()+"/frame.png";QImage image(64,48,QImage::Format_Grayscale8);image.fill(255);check(image.save(path),"image fixture");
    project.setImageFile(path);project.save();auto saved=*project.activeRecipe();
    const RuntimeIdentity identity{"engineer",[](const QString& p,ProductionAction){return p=="engineer";}};
    const auto archive=project.path()+".production";RuntimeService runtime(archive,project.path()+".calibrations.json",identity);
    RuntimeCoordinator coordinator(runtime);ProductionStore store(archive);
    rejects([&]{coordinator.start();},ProductionError::UnknownValidation);
    check(coordinator.state()==RuntimeState::Unloaded,"unloaded");
    coordinator.load(saved,dir.path());saved.name="Unsaved edit";
    check(coordinator.loaded()->name=="Saved" && !runtime.active() && store.recent("snapshots").isEmpty() && store.recent("cycles").isEmpty(),"Load copies recipe without activation/acquisition/cycle");
    auto invalid=saved;invalid.definition={};rejects([&]{coordinator.load(invalid,dir.path());},ProductionError::InvalidConfiguration);
    check(coordinator.loaded()->name=="Saved","failed Load preserves previous recipe");
    auto missing=saved;auto missingTools=editorTools(missing.definition);missingTools.first().parameters["path"]=dir.path()+"/missing.png";missing.definition=withEditorTools(missing.definition,missingTools);
    rejects([&]{coordinator.load(missing,dir.path());},ProductionError::InvalidConfiguration);
    const auto old=coordinator.requestStart();rejects([&]{coordinator.requestStart();},ProductionError::Busy);
    coordinator.stop();const auto fresh=coordinator.requestStart();
    check(!coordinator.completeStart(old) && coordinator.state()==RuntimeState::Starting,"late startup callback ignored");
    check(coordinator.completeStart(fresh) && runtime.active()->recipe().name=="Saved","Run prepares loaded immutable snapshot");
    check(store.recent("cycles").isEmpty(),"Run creates no result");
    rejects([&]{coordinator.load(saved,dir.path());},ProductionError::Busy);
    rejects([&]{coordinator.begin(RuntimeTrigger::SoftwareTrigger);},ProductionError::InvalidConfiguration);
    const auto cycle=coordinator.begin(RuntimeTrigger::Manual);
    rejects([&]{coordinator.begin(RuntimeTrigger::Manual);},ProductionError::Busy);
    const auto result=RuntimeService::execute(cycle,project.path()+".calibrations.json",coordinator.cancellation());
    check(result.state==ExecutionState::Done,"real file cycle");
    auto partial=result;partial.partial=true;rejects([&]{runtime.finish(cycle.ticket,partial);},ProductionError::InvalidCompletion);
    check(!coordinator.recordingPending(),"Editor partial rejection is typed invalid completion, not record retry");
    QLockFile lock(archive+"/archive.lock");check(lock.tryLock(),"lock");
    rejects([&]{coordinator.finish(cycle.ticket,result);},ProductionError::PersistenceFailure);
    check(coordinator.recordingPending() && runtime.busy(),"recording pending retains lease");
    check(coordinator.failure()==RuntimeFailure::Persistence && !coordinator.error().isEmpty(),"typed persistence recovery");
    const auto firstCause=coordinator.error();auto altered=result;altered.milliseconds+=1;
    rejects([&]{coordinator.finish(cycle.ticket,altered);},ProductionError::InvalidCompletion);
    check(coordinator.recordingPending() && coordinator.error()==firstCause,"changed retry rejected without losing frozen record/first cause");
    coordinator.setDeliveryPending(true);coordinator.stop();
    check(coordinator.state()==RuntimeState::Stopping && coordinator.deliveryPending(),"Stop preserves pending work");
    rejects([&]{coordinator.start();},ProductionError::Busy);lock.unlock();coordinator.retryRecording();
    check(!coordinator.recordingPending() && coordinator.state()==RuntimeState::LoadedStopped && coordinator.deliveryPending(),"record retry completes Stop without losing delivery");
    rejects([&]{coordinator.load(saved,dir.path());},ProductionError::Busy);
    coordinator.setDeliveryPending(false);coordinator.start();
    const auto stopped=coordinator.begin(RuntimeTrigger::Manual);const auto cancel=coordinator.cancellation();coordinator.stop();
    check(cancel->load(),"Stop cancels worker");
    rejects([&]{coordinator.begin(RuntimeTrigger::Manual);},ProductionError::InvalidConfiguration);
    const auto skipped=RuntimeService::execute(stopped,project.path()+".calibrations.json",cancel);coordinator.finish(stopped.ticket,skipped);
    check(skipped.state==ExecutionState::Skipped && coordinator.state()==RuntimeState::LoadedStopped,"cooperative Stop terminal persisted");
    rejects([&]{coordinator.finish(stopped.ticket,skipped);},ProductionError::InvalidCompletion);
    check(!coordinator.recordingPending() && coordinator.failure()==RuntimeFailure::InvalidCompletion,"invalid completion is not persistence retry");
    rejects([&]{coordinator.retryRecording();},ProductionError::InvalidCompletion);
    const auto failing=coordinator.requestStart();rejects([&]{coordinator.completeStart(failing,"Device preparation failed");},ProductionError::InvalidConfiguration);
    check(coordinator.state()==RuntimeState::Fault && coordinator.error()=="Device preparation failed","startup failure closes admission");
    coordinator.start();auto changed=*coordinator.loaded();auto tools=editorTools(changed.definition);tools.first().parameters["path"]=dir.path()+"/other.png";changed.definition=withEditorTools(changed.definition,tools);runtime.observeInstallation(changed);
    rejects([&]{coordinator.begin(RuntimeTrigger::Manual);},ProductionError::DependencyChanged);check(coordinator.state()==RuntimeState::Fault,"dependency invalidation faults Run");
    coordinator.stop();coordinator.load(saved,dir.path(),RuntimeTrigger::SoftwareTrigger);coordinator.start();
    check(lock.tryLock(),"begin lock");rejects([&]{coordinator.begin(RuntimeTrigger::SoftwareTrigger);},ProductionError::PersistenceFailure);
    check(coordinator.state()==RuntimeState::Fault && !runtime.busy(),"failed cycle journal closes admission before acquisition");lock.unlock();
    const auto retained=runtime.active();check(lock.tryLock(),"startup lock");rejects([&]{coordinator.start();},ProductionError::PersistenceFailure);
    check(coordinator.state()==RuntimeState::Fault && runtime.active()==retained,"failed startup preserves active evidence without admitting events");lock.unlock();coordinator.start();
    rejects([&]{coordinator.begin(RuntimeTrigger::Manual);},ProductionError::InvalidConfiguration);
    const auto software=coordinator.begin(RuntimeTrigger::SoftwareTrigger);coordinator.finish(software.ticket,RuntimeService::execute(software,project.path()+".calibrations.json",coordinator.cancellation()));
    check(std::async(std::launch::async,[&]{try{coordinator.stop();}catch(const ProductionException& e){return e.code()==ProductionError::WrongThread;}return false;}).get(),"owner thread enforced");
    RuntimeService restarted(archive,project.path()+".calibrations.json",identity);RuntimeCoordinator restart(restarted);
    check(restart.state()==RuntimeState::Unloaded && !restarted.active() && restarted.records().size()==3,"restart never auto Run");
    RuntimeService denied(dir.path()+"/denied",project.path()+".calibrations.json",{"guest",{}});RuntimeCoordinator guest(denied);
    rejects([&]{guest.load(saved,dir.path());},ProductionError::Unauthorized);
    bool allowRun=false;RuntimeService policy(dir.path()+"/policy",project.path()+".calibrations.json",{"engineer",[&](const QString&,ProductionAction action){return action!=ProductionAction::Run || allowRun;}});RuntimeCoordinator guarded(policy);
    guarded.load(saved,dir.path());rejects([&]{guarded.start();},ProductionError::Unauthorized);check(guarded.state()==RuntimeState::Fault,"Run permission required");
    allowRun=true;guarded.start();check(guarded.state()==RuntimeState::Running,"startup retry");
    QTextStream(stdout)<<"PASS: load/run/stop, immutable copy, late startup, source gating, persistence/delivery barriers, cancellation, owner thread and restart\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
