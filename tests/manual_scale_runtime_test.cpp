#include "application/RuntimeService.hpp"
#include "application/CalibrationReportService.hpp"
#include "application/EditorSession.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QJsonDocument>
#include <iostream>
using namespace visionflow;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("manual-runtime-XXXXXX"));check(dir.isValid(),"build fixture");
    QImage image(1000,800,QImage::Format_Grayscale8);image.fill(0);for(int y=200;y<600;++y)for(int x=250;x<451;++x)image.scanLine(y)[x]=255;
    check(image.save(dir.filePath("part.png")),"save actual part image");
    ProjectService project;project.createProject("Manual","",dir.filePath("test.vfproject"),"file");const auto recipeId=project.createRecipe("Part","",ApplicationType::Measurement,"file");
    const auto source=project.addTool("image_file"),pre=project.addTool("preprocess"),left=project.addTool("edge"),right=project.addTool("edge"),measurement=project.addTool("measurement");
    for(auto t:project.tools()){if(t.id==source)t.parameters["path"]="part.png";if(t.id==right)t.parameters["direction"]="right_to_left";if(t.id==measurement){t.parameters["lowerMm"]=10.;t.parameters["upperMm"]=10.;}project.updateTool(t);}
    project.setConnections({{source,"image",pre,"image"},{pre,"image",left,"image"},{pre,"image",right,"image"},{left,"point",measurement,"p1"},{right,"point",measurement,"p2"}});project.setCalibrationSetup("lens","plane");project.save();
    const auto frame=RuntimeService::probe(*project.activeRecipe(),dir.path(),{});const auto install=CalibrationWorkflow::installation(*project.activeRecipe(),frame,"manual_planar_scale");
    rejects([&]{CalibrationWorkflow::installation(*project.activeRecipe(),frame,"future");});rejects([&]{CalibrationWorkflow::manualScale(*project.activeRecipe(),frame," bad ",1,20);});
    MeasurementService math;const auto candidate=CalibrationWorkflow::manualScale(*project.activeRecipe(),frame,"manual",1,20);
    const QVector<CalibrationSample> holdout{{"horizontal",{{20,20},install.source},{{220,20},install.source},10},{"vertical",{{30,40},install.source},{{30,140},install.source},5}};
    const auto accepted=math.validate(candidate,holdout,0);
    const auto catalogPath=project.path()+".calibrations.json";CalibrationService catalog(catalogPath,[install](const QString&){return install;});
    rejects([&]{catalog.save(candidate);});catalog.save(accepted);check(catalog.load("manual",1).dependency()==accepted.dependency(),"catalog exact replay");rejects([&]{catalog.save(accepted);});
    CalibrationWorkflow::bind(project,frame,catalogPath,"manual","manual_planar_scale");check(project.dirty() && project.activeRecipe()->status==RecipeStatus::Draft,"explicit binding draft");project.save();
    CalibrationWorkflow::bind(project,frame,catalogPath,"manual","manual_planar_scale");check(!project.dirty(),"same manual binding no-op");
    const auto savedVersion=project.activeRecipe()->recipeVersion;ProjectService reopened;reopened.openProject(project.path());reopened.selectRecipe(recipeId);check(reopened.activeRecipe()->calibrationBinding==accepted.dependency(),"recipe schema2 dependency3 roundtrip");
    reopened.save();check(!reopened.dirty() && reopened.activeRecipe()->recipeVersion==savedVersion,"no-op save revision");
    const auto reportPath=dir.filePath("manual.csv");rejects([&]{CalibrationReportService::saveCsv(reportPath,candidate);});CalibrationReportService::saveCsv(reportPath,accepted);QFile report(reportPath);check(report.open(QIODevice::ReadOnly),"open report");const auto csv=report.readAll();check(csv.contains("manual_planar_scale") && csv.contains("manual px/mm\",\"20") && csv.contains("mm/px\",\"0.05") && csv.contains(frame.metadata().frameId.toUtf8()) && csv.contains("acquired") && csv.contains("horizontal") && !csv.contains("legacy"),"CSV real input inverse provenance holdout");
    auto run=[&]{return ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),recipeId,"test",{}, {},{},[&](const ImageFrame& f){return CalibrationWorkflow::resolve(*project.activeRecipe(),f,catalogPath);});};
    auto result=run();check(result.tools.last().state==ExecutionState::Done && result.tools.last().decision==PartDecision::Pass && std::get<double>(result.tools.last().outputs["value"].data)==10 && std::get<double>(result.tools.last().outputs["pixels"].data)==200,"actual Edge 200px -> 10mm inclusive both limits");
    RuntimeService runtime(project.path()+".production",catalogPath,RuntimeIdentity::localEngineer());runtime.prepare(*project.activeRecipe(),dir.path());auto lease=runtime.begin();
    for(auto t:project.tools())if(t.id==measurement){t.parameters["lowerMm"]=10.0001;t.parameters["upperMm"]=11.;project.updateTool(t);}
    result=RuntimeService::execute(lease,catalogPath,{});runtime.finish(lease.ticket,result);check(result.tools.last().decision==PartDecision::Pass && runtime.records().size()==1,"immutable runtime accepted and persisted");
    result=run();check(result.tools.last().decision==PartDecision::Fail,"raw tolerance outside lower fails");
    for(auto t:project.tools())if(t.id==measurement){t.parameters["lowerMm"]=0.;t.parameters["upperMm"]=9.9999;project.updateTool(t);}check(run().tools.last().decision==PartDecision::Fail,"raw tolerance outside upper fails");
    auto stopped=std::make_shared<std::atomic_bool>(true);result=ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),recipeId,"cancel",stopped);for(const auto& t:result.tools)check(t.outputs.isEmpty() && t.decision==PartDecision::NotEvaluated,"Stop no result");
    auto changed=install;changed.lensId="other";rejects([&]{CalibrationService(catalogPath,[changed](const QString&){return changed;}).resolve(*project.activeRecipe());});changed=install;changed.source.id="wrong";rejects([&]{CalibrationService(catalogPath,[changed](const QString&){return changed;}).resolve(*project.activeRecipe());});
    QImage resized(900,700,QImage::Format_Grayscale8);resized.fill(0);check(resized.save(dir.filePath("part.png")),"change actual resolution");const auto fresh=RuntimeService::probe(*project.activeRecipe(),dir.path(),{});check(!CalibrationWorkflow::resolve(*project.activeRecipe(),fresh,catalogPath).error.isEmpty(),"actual fresh resolution rejected");check(image.save(dir.filePath("part.png")),"restore image");
    EditorSession session;auto context=[&]{return EditorContext{project.project().id,project.path(),recipeId,project.activeRecipe()->definition,project.activeRecipe()->calibrationBinding->toJson()};};
    auto ctx=context();result=run();check(session.acceptCycle(result,session.signature(ctx),ctx),"cache manual full flow");
    const auto next=math.validate(CalibrationWorkflow::manualScale(*project.activeRecipe(),frame,"manual",2,20),holdout,0);
    {QLockFile lock(catalogPath+".lock");check(lock.tryLock(),"catalog lock");rejects([&]{catalog.save(next);});}check(catalog.current("manual")==accepted.dependency(),"failed save preserves current");catalog.save(next);rejects([&]{runtime.begin();});result=run();check(result.tools.last().state==ExecutionState::Error && result.tools.last().outputs.isEmpty() && result.tools.last().decision==PartDecision::NotEvaluated,"latest invalidates old binding no stale mm");
    CalibrationWorkflow::bind(project,frame,catalogPath,"manual","manual_planar_scale");session.reconcile(context());check(!session.cycle || session.cycle->tools.isEmpty(),"new binding invalidates target cache");
    auto offlineSetup=accepted.dependency();offlineSetup.calibrationId="offline";offlineSetup.version=1;offlineSetup.cameraId="calibration-test:file";const auto offline=math.validate(math.manualScale(offlineSetup,20,{"file_test",offlineSetup.cameraId,"file-frame",frame.size()}),holdout,0);catalog.save(offline);rejects([&]{catalog.bind(project,"offline");});rejects([&]{CalibrationWorkflow::bind(project,frame,catalogPath,"offline","manual_planar_scale");});
    auto forgedInstall=install;forgedInstall.cameraId=offlineSetup.cameraId;rejects([&]{CalibrationService(catalogPath,[forgedInstall](const QString&){return forgedInstall;}).current("offline");});
    {QLockFile lock(project.path()+".lock");check(lock.tryLock(),"project lock");rejects([&]{project.save();});}check(project.dirty() && catalog.current("manual")==next.dependency(),"catalog committed while project save failed");
    ProjectService oldDisk;oldDisk.openProject(project.path());oldDisk.selectRecipe(recipeId);check(oldDisk.activeRecipe()->calibrationBinding==accepted.dependency(),"failed project save preserves old disk binding");rejects([&]{catalog.resolve(*oldDisk.activeRecipe());});
    project.discardChanges();project.selectRecipe(recipeId);check(project.activeRecipe()->calibrationBinding==accepted.dependency(),"discard restores saved manual binding");CalibrationWorkflow::bind(project,frame,catalogPath,"manual","manual_planar_scale");project.save();for(auto t:project.tools())if(t.id==source){t.parameters["path"]="other.png";project.updateTool(t);}check(!project.activeRecipe()->calibrationBinding && project.dirty(),"source parameters clear binding");
    std::cout<<"PASS manual catalog / recipe / Edge / immutable Runtime / CSV / invalidation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}


