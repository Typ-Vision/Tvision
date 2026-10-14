#include "application/ProjectService.hpp"
#include "application/ProductionRecipeService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
double area(const CycleResult& result,const QString& id){for(const auto& t:result.tools)if(t.toolId==id)return std::get<double>(t.outputs["area"].data);throw std::runtime_error("missing blob");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/order-XXXXXX");check(dir.isValid(),"fixture directory");
    QImage image(32,32,QImage::Format_Grayscale8);image.fill(0);for(int y=4;y<12;++y)for(int x=4;x<12;++x)image.scanLine(y)[x]=220;
    const auto path=dir.filePath("source.png");check(image.save(path),"PNG");int index=0;
    for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        ProjectService s;s.createProject("Order","",dir.filePath(QString::number(index++)+".vfproject"),"file");s.createRecipe("Recipe","",type,"file");
        const auto source=s.setImageFile(path);s.save();const auto single=s.activeRecipe()->definition;
        check(!s.moveTool(source,-1) && !s.moveTool(source,1) && !s.dirty() && s.activeRecipe()->definition==single,"single boundary no-op");
        const auto pre=s.addTool("preprocess"),blob=s.addTool("blob"),independent=s.addTool("blob");
        auto links=editorConnections(s.activeRecipe()->definition);for(auto& c:links)if(c.targetTool==independent)c.sourceTool=source;s.setConnections(links);s.save();
        const auto original=s.activeRecipe()->definition;const auto originalTools=s.tools();
        ProductionRecipeService production([](const QString&,ProductionAction){return true;},[](const Recipe&){return ProductionDependencies{};});
        const auto validation=production.validate("engineer",*s.activeRecipe(),dir.path());production.activate("engineer",validation.id,*s.activeRecipe(),dir.path());const auto lease=production.beginCycle("engineer");
        check(!s.moveTool(source,-1) && !s.moveTool(independent,1) && !s.dirty(),"end boundaries no-op");
        check(s.moveTool(blob,-1) && s.moveTool(blob,-1) && s.moveTool(independent,-1),"dependent and independent move");
        check(s.tools().first().id==blob && s.dirty() && s.recipeDirty(s.activeRecipe()->id) && s.activeRecipe()->status==RecipeStatus::Draft,"order is draft edit");
        check(editorConnections(s.activeRecipe()->definition)==links,"manual and default connections preserved");
        check(withEditorTools(s.activeRecipe()->definition,originalTools)==original,"only tool list order changes");
        check(lease.snapshot->recipe().definition==original && production.active()->recipe().definition==original,"active snapshot and in-flight cycle immutable");
        const auto result=ExecutionService{}.run(s.activeRecipe()->definition,dir.path(),s.activeRecipe()->id,"order",{});
        check(result.state==ExecutionState::Done && result.acquisitions==1 && area(result,blob)==64 && area(result,independent)==64,"real source and preprocess DAG results");
        check(result.executedTools.indexOf(source)<result.executedTools.indexOf(pre) && result.executedTools.indexOf(pre)<result.executedTools.indexOf(blob),"dependencies override list order");
        const auto snapshotResult=ExecutionService{}.run(lease.snapshot->recipe().definition,dir.path(),s.activeRecipe()->id,validation.contentId,{});production.finishCycle(lease.ticket,snapshotResult);
        check(area(snapshotResult,blob)==area(result,blob),"immutable production cycle real result");
        const auto ordered=s.activeRecipe()->definition;const auto recipe=s.activeRecipe()->id,projectPath=s.path();s.save();s.openProject(projectPath);s.selectRecipe(recipe);check(s.activeRecipe()->definition==ordered,"save reopen order");
        QFile file(projectPath);check(file.open(QIODevice::ReadOnly),"saved bytes");const auto bytes=file.readAll();file.close();s.save();check(file.open(QIODevice::ReadOnly) && file.readAll()==bytes,"reopen no-op save");file.close();
        for(const auto bad: {0,2}){bool rejected=false;try{s.moveTool(blob,bad);}catch(const std::exception&){rejected=true;}check(rejected && !s.dirty(),"invalid direction atomic rejection");}
        bool rejected=false;try{s.moveTool("missing",1);}catch(const std::exception&){rejected=true;}check(rejected && !s.dirty(),"missing ID atomic rejection");
    }
    QTextStream(stdout)<<"Flow order: three application types, DAG, persistence and immutable production passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
