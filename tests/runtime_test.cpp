#include "application/RuntimeService.hpp"
#include "application/RuntimeCoordinator.hpp"
#include "application/EditorSession.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QTextStream>
using namespace visionflow;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F> void rejects(F f,ProductionError code,const char* text){try{f();}catch(const ProductionException& e){check(e.code()==code,text);return;}throw std::runtime_error(text);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/runtime-fixture-XXXXXX");check(dir.isValid(),"build fixture");
    QImage image(1000,600,QImage::Format_Grayscale8);image.fill(0);for(int y=100;y<400;++y)for(int x=200;x<600;++x)image.scanLine(y)[x]=255;
    const auto imagePath=dir.path()+"/input.png";check(image.save(imagePath),"real acquisition file");
    ProjectService project;project.createProject("Runtime","",dir.path()+"/part.vfproject","file");project.createRecipe("Part","",ApplicationType::Measurement,"file");
    const auto source=project.setImageFile(imagePath),left=project.addTool("edge"),right=project.addTool("edge"),borderBlob=project.addTool("blob"),measurement=project.addTool("measurement");
    for(auto t:project.tools()){if(t.id==borderBlob){t.parameters["rejectBorderTouching"]=true;t.parameters["searchRoi"]=QJsonObject{{"x",200},{"y",100},{"width",400},{"height",300}};}if(t.id==right)t.parameters["direction"]="right_to_left";if(t.id==measurement){t.parameters["lowerMm"]=9.97;t.parameters["upperMm"]=9.98;}project.updateTool(t);}
    project.setConnections({{source,"image",borderBlob,"image"},{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",measurement,"p1"},{right,"point",measurement,"p2"}});project.setCalibrationSetup("lens","plane");
    const auto frame=RuntimeService::probe(*project.activeRecipe(),dir.path(),{});const auto installation=CalibrationWorkflow::installation(*project.activeRecipe(),frame);
    auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,installation.source},{b,installation.source},mm};};
    const auto teaching=sample("teach",{0,0},{400,0},10);const QVector<CalibrationSample> holdout{sample("x",{10,30},{810,30},20),sample("y",{40,10},{40,510},12.5)};
    const auto catalog=project.path()+".calibrations.json";const auto c=MeasurementService{}.validate(CalibrationWorkflow::teach(*project.activeRecipe(),frame,"scale",1,teaching),holdout,.001);CalibrationService(catalog).save(c);CalibrationWorkflow::bind(project,frame,catalog,"scale");project.save();
    const RuntimeIdentity identity{"engineer",[](const QString& p,ProductionAction){return p=="engineer";}};
    const auto archive=project.path()+".production";ProductionStore store(archive);RuntimeService runtime(archive,catalog,identity);
    RuntimeService denied(dir.path()+"/denied",catalog,{"guest",{}});rejects([&]{denied.validate(*project.activeRecipe(),dir.path(),frame);},ProductionError::Unauthorized,"default deny");
    const auto recipe=*project.activeRecipe();const auto v1=runtime.validate(recipe,dir.path(),frame);runtime.activate(recipe,dir.path());check(store.recent("snapshots").size()==1 && store.recent("events").size()==1,"snapshot and activation persisted");
    RuntimeService prepared(dir.path()+"/prepared",catalog,identity);RuntimeCoordinator coordinator(prepared);
    coordinator.load(recipe,dir.path());coordinator.start();
    check(prepared.records().isEmpty(),"configuration-only Run creates no frame/cycle result");
    const auto preparedCycle=coordinator.begin(RuntimeTrigger::Manual);const auto preparedResult=RuntimeService::execute(preparedCycle,catalog,coordinator.cancellation());
    coordinator.finish(preparedCycle.ticket,preparedResult);check(preparedResult.state==ExecutionState::Done,"declared calibration preparation resolves actual frame at execution");coordinator.stop();
    const auto lease=runtime.begin();auto edited=recipe;edited.name="draft changed";edited.recipeVersion++;
    rejects([&]{runtime.validate(edited,dir.path(),frame);},ProductionError::Busy,"validation cannot replace live installation");
    rejects([&]{runtime.begin();},ProductionError::Busy,"single cycle");rejects([&]{runtime.rollback(v1.id);},ProductionError::Busy,"boundary rollback");
    auto result=RuntimeService::execute(lease,catalog,{});check(result.state==ExecutionState::Done,"actual Runtime DAG");
    bool borderVerified=false;for(const auto& t:result.tools)if(t.toolId==borderBlob)borderVerified=std::get<double>(t.outputs["candidateCount"].data)==1 && std::get<double>(t.outputs["count"].data)==0 && !t.outputs["point"].valid && t.decision==PartDecision::NotEvaluated;check(borderVerified,"immutable Runtime applies border filter without part decision");
    auto find=[&](const CycleResult& cycle){for(const auto& t:cycle.tools)if(t.toolId==measurement)return t;throw std::runtime_error("measurement absent");};
    check(find(result).decision==PartDecision::Pass && runtime.active()->recipe().name==recipe.name,"immutable active result");
    auto wrong=result;wrong.tools.last().calibration.reset();rejects([&]{runtime.finish(lease.ticket,wrong);},ProductionError::InvalidCompletion,"forged calibration rejected");
    wrong=result;wrong.tools.last().decision=PartDecision::Fail;wrong.tools.last().outputs["pass"].data=false;rejects([&]{runtime.finish(lease.ticket,wrong);},ProductionError::InvalidCompletion,"decision must match active raw tolerance");
    wrong=result;wrong.tools.last().toolId="foreign tool";rejects([&]{runtime.finish(lease.ticket,wrong);},ProductionError::InvalidCompletion,"foreign tool rejected");
    QLockFile lock(archive+"/archive.lock");lock.setStaleLockTime(0);check(lock.tryLock(),"lock fixture");
    rejects([&]{runtime.finish(lease.ticket,result);},ProductionError::PersistenceFailure,"write failure visible");check(runtime.busy() && store.recent("cycles").isEmpty(),"failed log retains lease");rejects([&]{runtime.begin();},ProductionError::Busy,"failed persistence blocks next acquisition");lock.unlock();
    runtime.finish(lease.ticket,result);check(!runtime.busy() && store.recent("cycles").size()==1,"same result retry atomic");rejects([&]{runtime.finish(lease.ticket,result);},ProductionError::InvalidCompletion,"duplicate finish rejected");
    const auto record=store.recent("cycles").first();bool logged=false;for(const auto& t:record["tools"].toArray()){const auto o=t.toObject();if(o["toolId"]==measurement)logged=o["decision"]=="PASS" && o["calibration"].toObject()==c.dependency().toJson() && std::abs(o["outputs"].toObject()["value"].toObject()["value"].toDouble()-9.975)<1e-12;}
    check(logged && record["communicationState"]=="NOT_REQUESTED" && record["configId"]==QString::fromLatin1(v1.contentId),"actual mm decision manifests trace");
    check(record["milliseconds"]==result.milliseconds && record["recipeName"]==recipe.name && record["tools"].toArray().first().toObject()["outputs"].toObject()["image"].toObject()["imageInfo"].toObject()["width"]==1000,"cycle elapsed time, human names and raw frame dimensions persisted without image bytes");
    RuntimeService restarted(archive,catalog,identity);check(!restarted.active() && restarted.records().size()==1,"restart reads archive but never auto trusts VALID");rejects([&]{restarted.begin();},ProductionError::UnknownValidation,"restart requires validation and activation");
    auto tools=editorTools(edited.definition);for(auto& t:tools)if(t.id==measurement)t.parameters["upperMm"]=9.9749;edited.definition=withEditorTools(edited.definition,tools);
    const auto v2=runtime.validate(edited,dir.path(),frame);
    check(lock.tryLock(),"activation lock fixture");rejects([&]{runtime.activate(edited,dir.path());},ProductionError::PersistenceFailure,"activation commit failure");check(runtime.active()->validation().id==v1.id,"failed activation preserves old active");
    rejects([&]{runtime.begin();},ProductionError::PersistenceFailure,"start journal failure before acquisition");check(!runtime.busy(),"failed begin creates no lease");lock.unlock();
    runtime.activate(edited,dir.path());const auto failLease=runtime.begin();const auto failResult=RuntimeService::execute(failLease,catalog,{});check(find(failResult).state==ExecutionState::Done && find(failResult).decision==PartDecision::Fail,"raw boundary FAIL still DONE");runtime.finish(failLease.ticket,failResult);runtime.rollback(v1.id);check(runtime.active()->recipe().name==recipe.name,"rollback retained version");
    for(const auto type:{ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement}) {
        auto typed=recipe;typed.applicationType=type;runtime.validate(typed,dir.path(),frame);runtime.activate(typed,dir.path());const auto typedLease=runtime.begin();const auto typedResult=RuntimeService::execute(typedLease,catalog,{});runtime.finish(typedLease.ticket,typedResult);
        check(typedResult.state==ExecutionState::Done && runtime.records().first()["applicationType"]==typeKey(type) && !runtime.records().first().contains("pose"),"three types share real Runtime and no fabricated robot pose");
    }runtime.rollback(v1.id);
    auto changed=recipe;auto setup=changed.definition["planarSetup"].toObject();setup["lensId"]="new lens";changed.definition["planarSetup"]=setup;runtime.observeInstallation(changed);rejects([&]{runtime.begin();},ProductionError::DependencyChanged,"physical setup declaration invalidates active");runtime.observeInstallation(recipe);
    const auto stopped=runtime.begin();auto token=std::make_shared<std::atomic_bool>(true);const auto skipped=RuntimeService::execute(stopped,catalog,token);runtime.finish(stopped.ticket,skipped);check(find(skipped).decision==PartDecision::NotEvaluated && find(skipped).outputs.isEmpty(),"cancel persisted without decision");
    QImage smaller(500,300,QImage::Format_Grayscale8);smaller.fill(0);for(int y=40;y<240;++y)for(int x=100;x<300;++x)smaller.scanLine(y)[x]=255;check(smaller.save(imagePath),"resolution changed");const auto smallLease=runtime.begin();const auto small=RuntimeService::execute(smallLease,catalog,{});check(find(small).errorCode=="CALIBRATION_INVALID" && find(small).outputs.isEmpty(),"actual current frame resolution rechecked");runtime.finish(smallLease.ticket,small);
    coordinator.start();const auto wrongSizeCycle=coordinator.begin(RuntimeTrigger::Manual);const auto wrongSize=RuntimeService::execute(wrongSizeCycle,catalog,coordinator.cancellation());
    check(find(wrongSize).errorCode=="CALIBRATION_INVALID" && find(wrongSize).outputs.isEmpty(),"Run requirements never certify actual resolution");coordinator.finish(wrongSizeCycle.ticket,wrongSize);
    CalibrationService(catalog).save(MeasurementService{}.validate(CalibrationWorkflow::teach(recipe,frame,"scale",2,teaching),holdout,.001));rejects([&]{runtime.begin();},ProductionError::DependencyChanged,"latest calibration invalidates next cycle");rejects([&]{runtime.rollback(v2.id);},ProductionError::DependencyChanged,"latest invalidates rollback");
    rejects([&]{coordinator.begin(RuntimeTrigger::Manual);},ProductionError::DependencyChanged,"declared Run still checks latest calibration on each cycle");
    const auto id=record["recordId"].toString();store.append("cycles",id,record);auto overwrite=record;overwrite["error"]="changed";bool refused=false;try{store.append("cycles",id,overwrite);}catch(...){refused=true;}check(refused,"append only archive");
    const auto filename=archive+"/cycles/"+id+".json";QFile file(filename);check(file.open(QIODevice::ReadOnly),"read archive");auto envelope=QJsonDocument::fromJson(file.readAll()).object();file.close();envelope["sha256"]="tampered";check(file.open(QIODevice::WriteOnly),"tamper fixture");file.write(QJsonDocument(envelope).toJson());file.close();bool corrupt=false;try{store.recent("cycles");}catch(...){corrupt=true;}check(corrupt,"corrupt archive rejected");
    QTextStream(stdout)<<"PASS: Runtime acquisition, immutable versions, permission, mm decisions, restart archives, lock/failure retry, rollback, cancellation and calibration invalidation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
