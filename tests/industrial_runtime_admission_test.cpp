#include "IndustrialProviderFixture.hpp"
#include "application/IndustrialRuntime.hpp"
#include "application/MeasurementWorkspace.hpp"
#include "application/ProjectService.hpp"
#include <QTemporaryDir>
#include <QDir>
#include "application/ProductionResultCodec.hpp"
#include "application/ProductionStore.hpp"
#include <QLockFile>
using namespace industrial_provider_fixture;
namespace {
const RuntimeIdentity engineer{"engineer",[](const QString&,ProductionAction){return true;}};
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/icm08a-XXXXXX");check(dir.isValid(),"fixture directory");
    for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance})for(bool mixed:{false,true}){
        const auto r=request();auto state=std::make_shared<State>();auto discovered=r.device;
        IndustrialExecutionInputs inputs;inputs.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));
        int discoveries=0;inputs.discover=[&](int,const auto&){++discoveries;return IndustrialDiscoverySnapshot{{discovered},{},false};};
        ProjectService project;project.createProject("Runtime admission","",dir.filePath(QString::number(int(type))+QString::number(mixed)+".vfproject"),"file");
        project.createRecipe("Industrial admission","",type,"file");const auto source=project.addTool("industrial_camera");
        MeasurementWorkspace workspace(project.path(),project.project().id,[]{return true;});workspace.replace({{r.camera},{r.setup},{r.profile}});
        project.bindIndustrialSource(source,workspace.environment().current,r.profile.id,1000);
        if(mixed){QImage im(3,2,QImage::Format_Grayscale8);im.fill(20);const auto path=dir.filePath("raw.png");check(im.save(path),"mixed actual file");const auto file=project.addTool("image_file");project.setImageFile(path,file);}
        Recipe recipe=*project.activeRecipe();int tamper=0;
        auto dependencies=[&](const Recipe& current){auto set=workspace.productionSources(current,inputs);if(tamper==1)set.sources[source].industrialSource={{"forged",true}};if(tamper==2)set.sources.remove(source);if(tamper==3)set.industrial.reset();return set;};
        MultiSourceRuntimeService runtime(engineer,dependencies);
        runtime.prepare(recipe,dir.path());runtime.requireReady();check(state->hardware->commands==0 && discoveries>0,"prepare/Run trigger zero");
        const auto admission=runtime.active();const auto dependency=admission->dependencies()["sourceSet"].toObject()[source].toObject()["industrialSource"].toObject();
        check(decodeIndustrialRuntimeDependency(SystemCalibration::source(SystemCalibration::branch(recipe,source)),dependency).device==r.device,"snapshot exact device/backend/profile/geometry");
        for(const auto field:{"physical","backend","profile","setup","reference","unexpected"}){auto changed=dependency;changed[field]=QJsonObject{};reject([&]{decodeIndustrialRuntimeDependency(SystemCalibration::source(SystemCalibration::branch(recipe,source)),changed);});}
        auto lease=runtime.begin();check(state->hardware->commands==0,"begin admission trigger zero");
        // Editing the caller's contract and recipe after begin cannot rewrite the worker lease.
        auto environment=workspace.environment();environment.current.profiles[0].requested.gain=2;environment.current.profiles[0].revision++;workspace.replace(environment.current);
        auto tools=editorTools(recipe.definition);tools[0].parameters["sourceReference"]=QJsonValue::Null;recipe.definition=withEditorTools(recipe.definition,tools);
        const auto originalFactory=inputs.factory;inputs.factory={};inputs.routes[r.camera.id.value]={"oecs",{},"other","other"};
        const auto completed=RuntimeService::execute(lease,"",{});inputs.factory=originalFactory;inputs.routes.clear();
        check(completed.state==ExecutionState::Done && completed.frames.size()==(mixed?2:1) && completed.industrialEvidence.size()==1,"frozen single/mixed worker acquires actual frame");
        check(completed.industrialEvidence[source]->request.profile.requested.gain==1 && admission->dependencies()["sourceSet"].toObject()[source].toObject()["industrialSource"]==dependency,"caller edit does not leak into snapshot");
        auto changed=completed;auto fake=std::make_shared<IndustrialExecutionEvidence>(*completed.industrialEvidence[source]);fake->pixelHash="changed";changed.industrialEvidence[source]=fake;reject([&]{runtime.finish(lease.ticket,changed);});
        reject([&]{RuntimeService::suppressCancelled(lease,changed);});
        CycleResult forged;forged.cycleId=completed.cycleId;forged.recipeId=completed.recipeId;forged.configId=completed.configId;forged.state=completed.state;forged.frames=completed.frames;forged.tools=completed.tools;forged.industrialEvidence=completed.industrialEvidence;reject([&]{runtime.finish(lease.ticket,forged);});
        changed=completed;changed.industrialEvidence.clear();reject([&]{runtime.finish(lease.ticket,changed);});
        runtime.finish(lease.ticket,completed);check(runtime.lastCompletion().has_value(),"trusted industrial completion");
        const auto before=state->hardware->commands;reject([&]{runtime.begin();});check(state->hardware->commands==before,"changed profile requires reprepare without trigger");
        environment=workspace.environment();auto restored=environment.current.profiles[0];restored.requested=r.profile.requested;
        environment.current.profiles[0]=environment.current.profiles[0].revised(restored);workspace.replace(environment.current);
        project.bindIndustrialSource(source,workspace.environment().current,r.profile.id,1000);recipe=*project.activeRecipe();
        for(tamper=1;tamper<=3;++tamper)reject([&]{runtime.prepare(recipe,dir.path());});tamper=0;
        runtime.prepare(recipe,dir.path());
        discovered.backend.runtimeSha256=QString(64,'b');reject([&]{runtime.begin();});discovered=r.device;
        auto next=runtime.begin();discovered.backend.runtimeSha256=QString(64,'b');const auto backendChanged=RuntimeService::execute(next,"",{});
        check(backendChanged.state==ExecutionState::Error && !backendChanged.industrialEvidence.contains(source) && std::none_of(backendChanged.frames.cbegin(),backendChanged.frames.cend(),[&](const auto& f){return f.sourceId==source;}) && state->hardware->commands==before,"backend changed after begin rejected before trigger");runtime.abandon(next.ticket,"backend mismatch");discovered=r.device;
        for(int mode=0;mode<3;++mode){runtime.prepare(recipe,dir.path());auto actualLease=runtime.begin();state->foreign=mode==0;state->geometry=mode==1;state->settings=mode==2;
            const auto wrong=RuntimeService::execute(actualLease,"",{});check(wrong.state==ExecutionState::Error && wrong.industrialEvidence.isEmpty(),"actual identity/geometry/profile mismatch ERROR");runtime.abandon(actualLease.ticket,"actual mismatch");state->foreign=state->geometry=state->settings=false;}
        runtime.prepare(recipe,dir.path());auto stoppedLease=runtime.begin();auto stop=std::make_shared<std::atomic_bool>(true);const auto stopped=RuntimeService::execute(stoppedLease,"",stop);check(stopped.state==ExecutionState::Skipped && stopped.frames.isEmpty(),"Stop suppresses frame");runtime.abandon(stoppedLease.ticket,"Stop");
        auto busy=inputs.factory.open(r,CameraLeaseUse::Calibration,FrameQueueMode::ProductionFifo,1);runtime.prepare(recipe,dir.path());const auto busyLease=runtime.begin();const auto blocked=RuntimeService::execute(busyLease,"",{});check(blocked.state==ExecutionState::Error && !blocked.industrialEvidence.contains(source) && std::none_of(blocked.frames.cbegin(),blocked.frames.cend(),[&](const auto& f){return f.sourceId==source;}),"Runtime shares calibration lease");runtime.abandon(busyLease.ticket,"busy");busy.reset();
        // Only the test executable injects a backend. Native product composition stays closed.
        ProductionStore store(dir.filePath("archive"+QString::number(int(type))+QString::number(mixed)));
        int captures=0;MultiSourceRuntimeService product(engineer,dependencies,[&](const QString& kind,const QString& id,const QJsonObject& o){store.append(kind,id,o);});product.prepare(recipe,dir.path());
        auto persistent=product.begin();auto value=RuntimeService::execute(persistent,"",{});captures=state->hardware->commands;
        QLockFile lock(dir.filePath("archive"+QString::number(int(type))+QString::number(mixed))+"/archive.lock");check(lock.tryLock(),"lock archive");reject([&]{product.finish(persistent.ticket,value);});check(product.busy() && !product.lastCompletion(),"record failure blocks publication");
        auto swapped=value;swapped.industrialEvidence.clear();reject([&]{product.finish(persistent.ticket,swapped);});lock.unlock();product.finish(persistent.ticket,value);check(state->hardware->commands==captures,"retry never captures");
        const auto records=store.recent("cycles");check(records.size()==1 && records.first()["schemaVersion"]==9,"terminal9 persisted");ProductionResultCodec::validate(records.first());
        auto disk=records.first();auto branches=disk["branches"].toObject();auto branch=branches[source].toObject();auto proof=branch["industrialAcquisition"].toObject();proof["pixelHash"]="bad";branch["industrialAcquisition"]=proof;branches[source]=branch;disk["branches"]=branches;reject([&]{ProductionResultCodec::validate(disk);});
        check(ProductionResultCodec::csv(records).contains("industrialEvidence"),"industrial CSV offline");
        for(const auto key:{"physical","profile","actual","execution","acquired","pixelHash","unexpected"}){auto changedRecord=records.first();auto all=changedRecord["branches"].toObject();auto b=all[source].toObject();auto e=b["industrialAcquisition"].toObject();e[key]=QJsonObject{};b["industrialAcquisition"]=e;all[source]=b;changedRecord["branches"]=all;reject([&]{ProductionResultCodec::validate(changedRecord);});}
        auto failedLease=product.begin();state->foreign=true;auto failed=RuntimeService::execute(failedLease,"",{});state->foreign=false;product.finish(failedLease.ticket,failed);
        auto stoppedLease2=product.begin();auto late=RuntimeService::execute(stoppedLease2,"",{});RuntimeService::suppressCancelled(stoppedLease2,late);product.finish(stoppedLease2.ticket,late);
        check(store.recent("cycles").size()==3,"success partial failure and Stop survive restart reader");
        const auto archive=dir.filePath("archive"+QString::number(int(type))+QString::number(mixed));RuntimeService restarted(archive,dir.filePath("missing-catalog.json"),engineer);const auto captureCount=state->hardware->commands;
        check(!restarted.active() && restarted.records().size()==3 && ProductionResultCodec::csv(restarted.records()).contains("NOT_ACQUIRED") && state->hardware->commands==captureCount,"restart history has no SDK/registry/capture dependency or active permission");
        auto normal=inputs;normal.factory={};MultiSourceRuntimeService unavailable(engineer,[&](const Recipe& current){return workspace.productionSources(current,normal);});reject([&]{unavailable.prepare(recipe,dir.path());});
        reject([&]{workspace.productionSources(recipe);});
        const auto duplicate=project.addTool("industrial_camera");project.bindIndustrialSource(duplicate,workspace.environment().current,r.profile.id,1000);reject([&]{runtime.prepare(*project.activeRecipe(),dir.path());});
        environment=workspace.environment();auto shifted=environment.current.setups[0];shifted.geometry.sensorRoi->translate(1,0);
        environment.current.setups[0]=environment.current.setups[0].revised(shifted);workspace.replace(environment.current);
        reject([&]{runtime.begin();});check(!runtime.lastCompletion(),"same-size offset change invalidates admission; no terminal completion");
    }
    QTextStream(stdout)<<"Industrial Runtime admission: three types x single/mixed, frozen binding, prepare/begin/actual, tamper, Stop, lease and closed terminal9 persistence passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
