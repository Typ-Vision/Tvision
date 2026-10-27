#include "application/RuntimeCoordinator.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QLockFile>
#include <QTextStream>
#include <future>
#include <QUuid>
#include <QFile>
#include <QJsonArray>
#include "application/ProductionResultCodec.hpp"
#include "domain/ToolRegistry.hpp"
using namespace visionflow;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F action,ProductionError code){try{action();}catch(const ProductionException& e){check(e.code()==code,e.what());return;}throw std::runtime_error("Expected rejection");}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
void variableContract(const QString& directory,const Recipe& base,const RuntimeIdentity& identity) {
    const auto framePath=directory+"/variables.png";
    QImage image(32,24,QImage::Format_Grayscale8);image.fill(255);check(image.save(framePath),"variable image");
    const auto sourceId=uuid(),blobId=uuid(),firstId=uuid(),secondId=uuid();
    VariableDefinition joined{uuid(),"Joined"};
    VariableDefinition x{uuid(),"Found"},y{uuid(),"Copy"},large{uuid(),"Large",VariableType::Int64},unset{uuid(),"Unset",VariableType::SpatialTransform,ScalarUnit::Millimetre};
    ValueEnvelope big;big.type=ScalarType::Int64;big.validity=ValueValidity::Valid;big.value=qint64(9007199254740993LL);
    AssignmentConfig first{firstId,1,{{x.id,ToolOutputAssignmentSource{blobId,"found",4}},{large.id,ConstantAssignmentSource{big}}}};
    AssignmentConfig second{secondId,1,{{y.id,BundleAssignmentSource{firstId,x.id,1}}}};
    ToolDefinition source{sourceId,"image_file","Source",{},true,{{"path",framePath}}};
    ToolDefinition blob{blobId,"blob","Blob",{},true,defaultParameters("blob")};
    auto definition=withRecipeVariables({}, {x,y,large,unset,joined});
    definition=withEditorTools(definition,{{secondId,"set_variables","Second",{},true,encodeAssignmentConfig(second)},blob,{firstId,"set_variables","First",{},true,encodeAssignmentConfig(first)},source});
    definition=withEditorConnections(definition,{{sourceId,"image",blobId,"image"}});
    for(bool multi:{false,true})for(auto type:{ApplicationType::DefectInspection,ApplicationType::Measurement,ApplicationType::RobotGuidance}) {
        auto recipe=base;recipe.applicationType=type;recipe.definition=multi?withMultiSourceSchema(definition):definition;
        if(multi){auto secondSource=source;secondSource.id=uuid();secondSource.name="Source2";auto secondBlob=blob;secondBlob.id=uuid();secondBlob.name="Blob2";auto tools=editorTools(recipe.definition);
            for(auto& t:tools)if(t.id==firstId){auto c=first;c.rows.append({joined.id,ToolOutputAssignmentSource{secondBlob.id,"found",4}});t.parameters=encodeAssignmentConfig(c);}tools.append(secondSource);tools.append(secondBlob);recipe.definition=withEditorTools(recipe.definition,tools);auto links=editorConnections(recipe.definition);links.append({secondSource.id,"image",secondBlob.id,"image"});recipe.definition=withEditorConnections(recipe.definition,links);}
        const auto archive=directory+"/vars-"+uuid();
        RuntimeService runtime(archive,directory+"/catalog.json",identity,{}, {}, {},[&](const Recipe& r){MultiSourceDependencies d;for(const auto& t:editorTools(r.definition))if(t.enabled && t.kind=="image_file")d.sources[t.id]={};return d;});
        RuntimeCoordinator coordinator(runtime);coordinator.load(recipe,directory);coordinator.start();
        const auto snapshot=runtime.active();const auto config=snapshot->validation().contentId;const auto dependency=snapshot->variableDependencyId();
        check(!coordinator.variables(),"Run has no variable result");
        auto run=[&]{auto lease=coordinator.begin(RuntimeTrigger::Manual);check(!coordinator.variables(),"New cycle clears previous presentation");const auto cancel=coordinator.cancellation();auto result=std::async(std::launch::async,[&]{return RuntimeService::execute(lease,directory+"/catalog.json",cancel);}).get();return std::make_pair(lease,result);};
        auto [lease,valid]=run();check(valid.state==ExecutionState::Done && valid.variables(),"Runtime worker runs assignment executor");
        check(!coordinator.variables(),"Uncommitted worker context not published");
        if(multi)check(valid.acquisitions==2 && std::get<bool>(*std::get<ValueEnvelope>(valid.variables()->values[joined.id]).value),"Runtime assignment joins actual outputs from two sources");
        const auto context=valid.variables();check(context->cycleId==valid.cycleId && context->configId==QString::fromLatin1(config.toHex()) && context->dependencyId==dependency,"Cycle/config/dependency context bound");
        check(std::get<bool>(*std::get<ValueEnvelope>(context->values[x.id]).value) && std::get<bool>(*std::get<ValueEnvelope>(context->values[y.id]).value),"DAG output and upstream bundle assigned");
        check(std::get<qint64>(*std::get<ValueEnvelope>(context->values[large.id]).value)==9007199254740993LL,"Runtime Int64 is lossless");
        check(!std::get<SpatialAssignmentValue>(context->values[unset.id]).value,"Unassigned spatial value remains unset");
        QLockFile lock(archive+"/archive.lock");check(lock.tryLock(),"variable recording lock");
        rejects([&]{coordinator.finish(lease.ticket,valid);},ProductionError::PersistenceFailure);
        check(!coordinator.variables() && coordinator.recordingPending(),"Persistence barrier hides context");lock.unlock();coordinator.retryRecording();
        check(coordinator.variables()==context,"Same frozen context visible after durable retry");
        if(multi){for(auto record:runtime.records())if(record["cycleId"]==valid.cycleId){auto tools=record["tools"].toArray();for(int n=0;n<tools.size();++n){auto t=tools[n].toObject();if(t["toolId"]==firstId){t["outputs"]=QJsonObject{};tools[n]=t;}}record["tools"]=tools;bool rejected=false;try{ProductionResultCodec::validate(record);}catch(const std::exception&){rejected=true;}check(rejected,"Historical DONE cannot silently drop assignment bundle");}}

        // A draft catalog rename and RHS binding change cannot mutate the loaded lease.
        auto renamed=x;renamed.name="Draft rename";auto draft=recipe;draft.definition=withRecipeVariables(draft.definition,{renamed,y,large,unset,joined});
        auto edited=editorTools(draft.definition);for(auto& t:edited)if(t.id==firstId){auto c=decodeAssignmentConfig(t.parameters,t.id);c.revision=2;ValueEnvelope no;no.value=false;no.validity=ValueValidity::Valid;c.rows[0].source=ConstantAssignmentSource{no};t.parameters=encodeAssignmentConfig(c);}else if(t.id==secondId){auto c=second;std::get<BundleAssignmentSource>(c.rows[0].source).revision=2;t.parameters=encodeAssignmentConfig(c);}draft.definition=withEditorTools(draft.definition,edited);
        check(runtime.active()==snapshot && snapshot->validation().contentId==config && snapshot->variableDependencyId()==dependency && coordinator.loaded()->definition==recipe.definition,"Draft does not mutate loaded definition/hash");
        // Second cycle: producer cannot acquire. No first-cycle value is reused.
        auto missingLease=coordinator.begin(RuntimeTrigger::Manual);check(!coordinator.variables(),"Next cycle starts with no prior values");check(QFile::remove(framePath),"Remove producer image after admission");
        auto missing=RuntimeService::execute(missingLease,directory+"/catalog.json",coordinator.cancellation());
        check(missing.state==ExecutionState::Error && missing.variables() && missing.variables()->cycleId!=context->cycleId,"Missing producer gets fresh error context");
        for(const auto& value:missing.variables()->values)std::visit([&](const auto& v){check(!v.value && v.validity==ValueValidity::Missing,"Error cannot restore old variable");},value);
        coordinator.finish(missingLease.ticket,missing);check(!coordinator.variables(),"ERROR hides variables");check(image.save(framePath),"Restore producer image");
        coordinator.stop();coordinator.load(recipe,directory);coordinator.start();
        auto [lateLease,late]=run();coordinator.stop();check(!coordinator.variables(),"Stop clears variables before producer drains");
        coordinator.finish(lateLease.ticket,late);check(!coordinator.variables() && coordinator.state()==RuntimeState::LoadedStopped,"Late successful producer cannot reappear after Stop");
        const auto records=runtime.records();bool skipped=false;for(const auto& record:records)if(record["cycleId"]==late.cycleId){skipped=record["state"]=="SKIPPED";for(const auto& t:record["tools"].toArray())check(t.toObject()["outputs"].toObject().isEmpty() && t.toObject()["decision"]=="NOT_EVALUATED","Stop terminal carries no result/decision");}check(skipped,"Late completion persisted as cancelled");
        // A matching-looking Editor cycle cannot issue Runtime completion authority.
        coordinator.start();auto forgedLease=coordinator.begin(RuntimeTrigger::Manual);auto editor=ExecutionService{}.run(recipe.definition,directory,recipe.id,config,{});
        check(!editor.variables(),"Editor cache/result has no Runtime context");rejects([&]{runtime.finish(forgedLease.ticket,editor);},ProductionError::InvalidCompletion);
        auto actual=RuntimeService::execute(forgedLease,directory+"/catalog.json",coordinator.cancellation());coordinator.finish(forgedLease.ticket,actual);
        // Stale lease cannot replace the current presentation.
        rejects([&]{coordinator.finish(lease.ticket,valid);},ProductionError::InvalidCompletion);check(!coordinator.variables(),"Stale completion closes presentation");
        coordinator.stop();coordinator.load(draft,directory);coordinator.start();
        check(runtime.active()->validation().contentId!=config && runtime.active()->variableDependencyId()!=dependency,"Stop+Load selects edited catalog/binding identity");
        auto [newLease,newValue]=run();coordinator.finish(newLease.ticket,newValue);
        check(!std::get<bool>(*std::get<ValueEnvelope>(coordinator.variables()->values[x.id]).value),"Boolean false is a valid value after explicit reload");
        check(coordinator.variables()->definitions.first().name=="Draft rename","Reloaded catalog visible");
        coordinator.stop();RuntimeService restart(archive,directory+"/catalog.json",identity);RuntimeCoordinator fresh(restart);check(!fresh.variables() && !restart.active(),"Restart restores no variable context/authority");
    }
    auto experimentalRecipe=base;experimentalRecipe.definition=withMultiSourceSchema(definition);
    RuntimeService experimental(directory+"/experimental-vars",directory+"/catalog.json",identity);experimental.setExperimental(true);
    RuntimeCoordinator trial(experimental);trial.load(experimentalRecipe,directory);trial.start();auto lease=trial.begin(RuntimeTrigger::Manual);
    auto value=RuntimeService::execute(lease,directory+"/catalog.json",trial.cancellation());check(value.state==ExecutionState::Done && value.variables(),"Experimental worker uses same cycle variables");trial.finish(lease.ticket,value);
    check(trial.variables() && experimental.experimental(),"Experimental context preserves separate admission policy");trial.stop();check(!trial.variables(),"Experimental Stop hides variable context");
}

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
    variableContract(dir.path(),saved,identity);
    QTextStream(stdout)<<"PASS: Runtime variables, worker isolation, valid/missing cycles, draft isolation, durable retry, late Stop/stale completion, all application types and legacy/multi;  load/run/stop, immutable copy, late startup, source gating, persistence/delivery barriers, cancellation, owner thread and restart\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
