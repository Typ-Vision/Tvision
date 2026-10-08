#include "application/EditorSession.hpp"
#include "application/RuntimeService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QTextStream>
using namespace visionflow;
namespace {
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
const ToolResult& tool(const CycleResult& c,const QString& id){for(const auto& t:c.tools)if(t.toolId==id)return t;throw std::runtime_error("missing result");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    QTemporaryDir dir(QDir::currentPath()+"/target-only-XXXXXX");check(dir.isValid(),"fixture directory");
    QImage image(100,60,QImage::Format_Grayscale8);image.fill(0);for(int y=10;y<40;++y)for(int x=20;x<60;++x)image.scanLine(y)[x]=255;
    const auto path=dir.filePath("part.png");check(image.save(path),"raw fixture");
    ProjectService project;project.createProject("Target","",dir.filePath("part.vfproject"),"file");project.createRecipe("Part","",ApplicationType::Measurement,"file");
    const auto source=project.setImageFile(path),left=project.addTool("edge"),right=project.addTool("edge"),blob=project.addTool("blob"),distance=project.addTool("calculate");
    for(auto t:project.tools())if(t.id==right){t.parameters["direction"]="right_to_left";project.updateTool(t);}
    project.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{source,"image",blob,"image"},{left,"point",distance,"p1"},{right,"point",distance,"p2"}});
    project.save();check(!project.dirty() && !project.activeRecipe()->definition.contains("runtime"),"legacy missing mode opens without migration/dirty");
    project.setRuntimeOptions("SoftwareTrigger",{});project.save();const auto savedVersion=project.activeRecipe()->recipeVersion;const auto savedRecipe=project.activeRecipe()->id;
    project.openProject(project.path());project.selectRecipe(savedRecipe);check(project.activeRecipe()->definition["runtime"].toObject()["trigger"]=="SoftwareTrigger","saved mode survives reopen");
    project.setRuntimeOptions("SoftwareTrigger",{});project.save();check(!project.dirty() && project.activeRecipe()->recipeVersion==savedVersion,"mode no-op preserves version");
    bool invalidMode=false;try{project.setRuntimeOptions("HardwareFrame",{});}catch(...){invalidMode=true;}check(invalidMode && !project.dirty(),"unimplemented mode rejected atomically");
    EditorSession session;
    auto context=[&]{return EditorContext{project.project().id,project.path(),project.activeRecipe()->id,project.activeRecipe()->definition,{}};};
    auto full=[&]{const auto ticket=session.begin(SessionJob::Execution);++session.generation;const auto c=context();auto result=ExecutionService{}.run(c.definition,dir.path(),c.recipeId,session.signature(c),{});check(result.acquisitions==1 && result.executedTools.size()==project.tools().size(),"full acquisition/engine counters");check(session.acceptCycle(result,session.signature(c),c),"accept full flow");session.finish(ticket);};
    auto partial=[&](const QString& id){const auto c=context();return ExecutionService{}.runTool(c.definition,dir.path(),c.recipeId,session.signature(c),{},id,session.evaluation(c));};
    auto accept=[&](CycleResult result){const auto c=context();check(session.acceptTool(result,session.signature(c),c),"accept partial evaluation");};
    full();const auto checksum=session.frame->contentId(),frameId=session.frame->metadata().frameId.toUtf8(),cycleId=session.frame->metadata().cycleId.toUtf8();
    for(auto t:project.tools())if(t.id==blob){t.parameters["threshold"]=200;project.updateTool(t);}
    session.reconcile(context());check(session.cycle && session.cycle->tools.size()==4,"target invalidated; upstream/independent branch retained");
    auto r=partial(blob);check(r.partial && r.tools.size()==1 && r.executedTools==QStringList{blob} && r.acquisitions==0,"only target executor; no source/upstream acquisition");
    check(r.frameId.toUtf8()==frameId && r.cycleId.toUtf8()==cycleId && !r.evaluationId.isEmpty(),"evaluation identity separate from original frame/cycle");
    check(std::get<double>(tool(r,blob).outputs["area"].data)==1200,"actual raw pixel area");accept(r);check(session.frame->contentId()==checksum,"raw buffer unchanged");
    r=partial(distance);check(r.executedTools==QStringList{distance} && r.acquisitions==0 && std::get<double>(tool(r,distance).outputs["value"].data)==39,"cached point inputs without edge rerun");accept(r);
    for(auto t:project.tools())if(t.id==left){t.parameters["minContrast"]=30;project.updateTool(t);}
    session.reconcile(context());r=partial(distance);check(r.executedTools.isEmpty() && tool(r,distance).state==ExecutionState::Skipped,"stale upstream rejected without hidden fallback");
    r=partial(left);check(r.executedTools==QStringList{left},"explicit upstream run");accept(r);r=partial(distance);check(r.executedTools==QStringList{distance},"fresh explicit upstream enables target");accept(r);
    for(auto t:project.tools())if(t.id==left){t.enabled=false;project.updateTool(t);}
    r=partial(distance);check(r.executedTools.isEmpty() && tool(r,distance).outputs.isEmpty(),"disabled upstream cache invalid");
    for(auto t:project.tools())if(t.id==left){t.enabled=true;project.updateTool(t);}
    full();auto captured=context();r=partial(blob);++session.generation;check(!session.acceptTool(r,session.signature(captured),context()),"new frame generation rejects late result");full();
    captured=context();const auto sig=session.signature(captured);r=partial(blob);for(auto t:project.tools())if(t.id==blob){t.parameters["threshold"]=100;project.updateTool(t);}check(!session.acceptTool(r,sig,context()),"changed parameter rejects worker result");full();
    auto ticket=session.begin(SessionJob::Execution);r=partial(blob);session.stop(SessionJob::Execution);accept(r);check(tool(*session.cycle,blob).outputs.isEmpty() && session.cycle->state==ExecutionState::Skipped,"Stop clears late target output");session.finish(ticket);full();
    auto altered=context();altered.projectId="other";auto e=session.evaluation(altered);check(e.cache.isEmpty(),"project provenance isolated");altered=context();altered.calibrationBinding={{"changed",true}};check(session.evaluation(altered).cache.isEmpty(),"binding provenance isolated");
    r=partial(source);check(r.state==ExecutionState::Error && r.executedTools.isEmpty(),"TargetOnly never acquires source");
    const auto match=project.addTool("template_match");
    for(auto t:project.tools())if(t.id==match){t.parameters["modelRoi"]=MatchService::rectangle(QRect(10,5,25,25));t.parameters["modelPng"]=MatchService{}.teach(*session.frame,QRect(10,5,25,25));project.updateTool(t);}
    auto links=editorConnections(project.activeRecipe()->definition);links.append({source,"image",match,"image"});links.append({match,"region",blob,"region"});project.setConnections(links);full();
    for(auto t:project.tools())if(t.id==match){t.parameters["minScore"]=.99;project.updateTool(t);}
    r=partial(blob);check(r.executedTools.isEmpty(),"transitive Match dependency invalidates region without full-image fallback");
    r=partial(match);accept(r);r=partial(blob);check(r.executedTools==QStringList{blob},"explicit Match refresh permits Blob on cached region");accept(r);
    for(auto t:project.tools())if(t.id==match){t.parameters["modelRoi"]=MatchService::rectangle(QRect(15,5,25,25));t.parameters["modelPng"]=MatchService{}.teach(*session.frame,QRect(15,5,25,25));project.updateTool(t);}
    r=partial(blob);check(r.executedTools.isEmpty(),"changed model/geometric evidence invalidates dependent input");
    image.fill(0);check(image.save(path),"external asset change");r=partial(blob);check(r.executedTools.isEmpty() && tool(r,blob).outputs.isEmpty(),"changed source asset invalidates cached raw input");
    RuntimeService runtime(dir.filePath("archive"),dir.filePath("catalog"),RuntimeIdentity::localEngineer());bool denied=false;try{runtime.finish(1,r);}catch(...){denied=true;}check(denied && runtime.records().isEmpty(),"partial editor result cannot become production evidence");
    QTextStream(stdout)<<"PASS: TargetOnly actual pixels, zero acquisition/upstream, cache invalidation, identity, cancellation and Runtime isolation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
