#include "application/EditorSession.hpp"
#include "application/ProjectService.hpp"
#include "application/ToolResultCodec.hpp"
#include "application/RuntimeCoordinator.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/result-XXXXXX");QImage image(30,30,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("part.png")),"fixture");
    ProjectService p;p.createProject("Result","",dir.filePath("result.vfproject"),"");auto recipe=p.createRecipe("Part","",ApplicationType::DefectInspection);auto source=p.setImageFile(dir.filePath("part.png"));auto blob=p.addTool("blob");auto result=p.addTool("result");
    auto update=[&](const QString& id,auto f){for(auto t:p.tools())if(t.id==id){f(t);p.updateTool(t);}};
    update(result,[&](auto& t){t.enabled=true;t.parameters=encodeResultConfig({ResultOperator::All,{{blob,"found",4}}});});p.save();
    const auto saved=p.activeRecipe()->definition;p.openProject(p.path());p.selectRecipe(recipe);p.save();check(p.activeRecipe()->definition==saved,"Save/reopen/no-op preserves rule");
    auto run=[&](std::shared_ptr<std::atomic_bool> cancel={}){return ExecutionService{}.run(p.activeRecipe()->definition,dir.path(),recipe,"config",cancel);};
    auto c=run();check(c.state==ExecutionState::Done && c.decision()==ResultDecision::Ok,"actual control Boolean OK");
    for(const auto& t:c.tools)if(t.toolId==result)for(const auto& port:t.outputs)check(ToolResultCodec::encode(ToolResultCodec::decode(ToolResultCodec::encode(port)))==ToolResultCodec::encode(port),"cycle decision/validity roundtrip");
    EditorSession session;EditorContext context{p.project().id,p.path(),recipe,p.activeRecipe()->definition,{}};const auto signature=session.signature(context);
    auto cached=ExecutionService{}.run(context.definition,dir.path(),recipe,signature,{});check(session.acceptCycle(cached,signature,context),"cache actual Result flow");
    auto partial=ExecutionService{}.runTool(context.definition,dir.path(),recipe,signature,{},result,session.evaluation(context,result));check(partial.partial && partial.acquisitions==0 && partial.executedTools==QStringList{result} && partial.decision()==ResultDecision::Ok,"Run Tool computes Result from current cached Boolean without acquisition");
    session.invalidateTool(context,blob);check(session.cycle && session.cycle->decision()==ResultDecision::NotEvaluated,"upstream cache invalidation immediately removes old OK summary");
    update(blob,[](auto& t){t.parameters["minArea"]=1000;});c=run();check(c.state==ExecutionState::Done && c.decision()==ResultDecision::Nok,"false Boolean NOK while DONE");
    update(blob,[](auto& t){t.enabled=false;});c=run();check(c.state==ExecutionState::Done && c.decision()==ResultDecision::NotEvaluated,"disabled required source clears NOK");
    p.enableMultiSource();c=run();check(c.state==ExecutionState::Done && c.decision()==ResultDecision::NotEvaluated,"schema9 required skipped source");
    c=run(std::make_shared<std::atomic_bool>(true));check(c.decision()==ResultDecision::NotEvaluated,"cancel suppresses decision");
    update(blob,[](auto& t){t.enabled=true;t.parameters["minArea"]=1;});
    // A second source is a legal cycle-level join, without pretending its geometry shares a frame.
    auto source2=p.addTool("image_file");auto blob2=p.addTool("blob");p.setConnections({{source,"image",blob,"image"},{source2,"image",blob2,"image"}});update(source2,[&](auto& t){t.parameters["path"]="part.png";});
    update(result,[&](auto& t){t.parameters=encodeResultConfig({ResultOperator::Any,{{blob,"found",4},{blob2,"found",4}}});});c=run();check(c.state==ExecutionState::Done && c.acquisitions==2 && c.decision()==ResultDecision::Ok,"ANY over two actual source Booleans");
    for(auto type:{ApplicationType::DefectInspection,ApplicationType::Measurement,ApplicationType::RobotGuidance}) {
        auto frozen=*p.activeRecipe();frozen.applicationType=type;
        RuntimeService runtime(dir.filePath("runtime-"+typeKey(type)),dir.filePath("catalog.json"),RuntimeIdentity::localEngineer(),{},{},{},[](const Recipe& r){MultiSourceDependencies d;for(const auto& t:editorTools(r.definition))if(t.enabled && t.kind=="image_file")d.sources[t.id]={};return d;});
        RuntimeCoordinator coordinator(runtime);coordinator.load(frozen,dir.path());coordinator.start();auto lease=coordinator.begin(RuntimeTrigger::Manual);auto actual=RuntimeService::execute(lease,dir.filePath("catalog.json"),{});check(actual.decision()==ResultDecision::Ok,"three application types share immutable multi-source Result");coordinator.finish(lease.ticket,actual);ProductionResultCodec::validate(runtime.records().first());coordinator.stop();
    }
    update(blob2,[](auto& t){t.enabled=false;});c=run();check(c.decision()==ResultDecision::NotEvaluated,"ANY true never hides later skipped source");
    auto wrong=editorTools(p.activeRecipe()->definition);for(auto& t:wrong)if(t.id==result)t.parameters=encodeResultConfig({ResultOperator::All,{{blob,"count",4}}});check(!validateGraph(wrong,editorConnections(p.activeRecipe()->definition)).valid(),"non Boolean picker bypass rejected");
    QTextStream(stdout)<<"PASS: Result graph, actual control, save/reopen, missing, cancel, multi-source ANY, port codec\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
