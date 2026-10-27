#include "application/ProjectService.hpp"
#include "application/ToolCatalog.hpp"
#include <QSet>
#include "application/ExecutionService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QImage>
#include <stdexcept>
#include <algorithm>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QString bound(const ProjectService& s,const QString& id){for(const auto& c:editorConnections(s.activeRecipe()->definition))if(c.targetTool==id && c.targetPort=="image")return c.sourceTool;return {};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QSet<QString> catalog;
    for(const auto& entry:ToolCatalog::entries()){check(!catalog.contains(entry.kind),"unique new catalog kind");catalog.insert(entry.kind);check(!ToolCatalog::groupForKind(entry.kind).isEmpty(),"each kind has one group");}
    for(const auto& entry:ToolRegistry::entries())check(catalog.contains(entry.kind)==(entry.kind!="edge"),"only retired Edge excluded from new catalog; Publish Data available");
    check(!ToolCatalog::canAdd("edge") && !ToolCatalog::canAdd("missing") && ToolCatalog::canAdd("edge_point"),"new entry policy");
    check(ToolCatalog::groupForKind("future_extension")=="other","future extension fallback");
    auto extension=ToolRegistry::get("calculate");extension.kind="fixture_extension";extension.label="Fixture extension";ToolRegistry::registerType(extension);
    check(ToolCatalog::canAdd(extension.kind) && ToolCatalog::groupForKind(extension.kind)=="other","registered extension retained");
    check(outputPorts("edge")[1].unit=="gray8" && outputPorts("edge_point")[1].unit=="gray8/px","legacy and subpixel strength units stay distinct");
    QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("creation-XXXXXX"));check(dir.isValid(),"artifact dir");
    QImage image(128,128,QImage::Format_Grayscale8);image.fill(100);const auto path=dir.filePath("source.png");check(image.save(path),"PNG");
    int index=0;
    for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}) {
        ProjectService s;s.createProject("Creation","",dir.filePath(QString::number(index++)+".vfproject"),"file");s.createRecipe("Recipe","",type,"file");
        const auto orphan=s.addTool("edge_point");check(bound(s,orphan).isEmpty(),"no source no default");
        const auto source=s.setImageFile(path);check(bound(s,orphan).isEmpty(),"source added later does not rewrite old tool");
        const auto p1=s.addTool("preprocess"),p2=s.addTool("preprocess");check(bound(s,p1)==source && bound(s,p2)==p1,"preprocess chain");
        const auto circle=s.addTool("circle_fit");check(bound(s,circle)==p2,"nearest preprocess");
        check(bound(s,s.addTool("calculate")).isEmpty() && bound(s,s.addTool("measurement")).isEmpty(),"non-image tools");
        auto links=editorConnections(s.activeRecipe()->definition);for(auto& c:links)if(c.targetTool==p2)c.sourceTool=source;s.setConnections(links);
        const auto branch=s.addTool("line_fit");check(bound(s,branch)==p2,"nearest valid branch");
        auto ambiguous=links;ambiguous.append({p1,"image",p2,"image"});s.setConnections(ambiguous);
        check(bound(s,s.addTool("edge_point"))==p1,"ambiguous preprocess excluded without adding a graph error");s.setConnections(links);
        for(auto t:s.tools())if(t.id==p2){t.enabled=false;s.updateTool(t);}check(bound(s,s.addTool("edge_pair"))==p1,"disabled preprocess excluded");
        links=editorConnections(s.activeRecipe()->definition);links.erase(std::remove_if(links.begin(),links.end(),[&](const auto& c){return c.targetTool==p1;}),links.end());s.setConnections(links);
        check(bound(s,s.addTool("blob"))==source,"disconnected preprocess excluded");
        links=editorConnections(s.activeRecipe()->definition);for(auto& c:links)if(c.targetTool==circle)c.sourceTool=source;s.setConnections(links);
        s.addTool("preprocess");check(bound(s,circle)==source,"manual override kept");
        links=editorConnections(s.activeRecipe()->definition);links.erase(std::remove_if(links.begin(),links.end(),[&](const auto& c){return c.targetTool==circle;}),links.end());for(const auto& t:s.tools())if(t.id==circle)s.updateToolConnections(t,links);
        s.save();const auto recipe=s.activeRecipe()->id,projectPath=s.path();QFile before(projectPath);check(before.open(QIODevice::ReadOnly),"read saved");const auto bytes=before.readAll();before.close();
        s.openProject(projectPath);s.selectRecipe(recipe);s.updateTool(s.tools().last());s.save();QFile after(projectPath);check(after.open(QIODevice::ReadOnly) && after.readAll()==bytes && bound(s,circle).isEmpty(),"reopen no-op and deliberate disconnection");after.close();
        for(auto t:s.tools())if(t.id==source){t.enabled=false;s.updateTool(t);}check(bound(s,s.addTool("edge_point")).isEmpty(),"disabled source excluded");
        const auto beforeRejected=s.activeRecipe()->definition;bool rejected=false;try{s.addTool("webcam");}catch(const std::exception&){rejected=true;}check(rejected && s.activeRecipe()->definition==beforeRejected,"multiple acquisition rejected atomically");
        s.save();s.createRecipe("Webcam","",type,"webcam");const auto webcam=s.addTool("webcam");check(bound(s,s.addTool("preprocess"))==webcam,"webcam source default without hardware acquisition");
    }
    // Internal compatibility API still supports old definitions; the UI creation catalog retires Edge.
    {ProjectService legacy;legacy.createProject("Legacy","",dir.filePath("legacy.vfproject"),"file");legacy.createRecipe("Legacy","",ApplicationType::Measurement,"file");
        legacy.setImageFile(path);const auto id=legacy.addTool("edge");legacy.save();const auto recipe=legacy.activeRecipe()->id;
        QFile file(legacy.path());check(file.open(QIODevice::ReadOnly),"legacy bytes");const auto bytes=file.readAll();file.close();
        legacy.openProject(legacy.path());legacy.selectRecipe(recipe);legacy.save();check(file.open(QIODevice::ReadOnly) && file.readAll()==bytes,"legacy Edge no-op bytes preserved");
        bool same=false;for(const auto& t:legacy.tools())if(t.id==id)same=t.kind=="edge";check(same,"legacy Edge kind not migrated");
        const auto cycle=ExecutionService{}.run(legacy.activeRecipe()->definition,dir.path(),recipe,"legacy",std::make_shared<std::atomic_bool>(false));
        check(cycle.state==ExecutionState::Done,"legacy Edge still executes");}
    // A default is a persisted real DAG edge, exercised by actual PNG acquisition.
    ProjectService run;run.createProject("Run","",dir.filePath("run.vfproject"),"file");run.createRecipe("Run","",ApplicationType::Measurement,"file");const auto source=run.setImageFile(path);const auto pre=run.addTool("preprocess"),last=run.addTool("preprocess");
    const auto result=ExecutionService{}.run(run.activeRecipe()->definition,dir.path(),run.activeRecipe()->id,"creation",std::make_shared<std::atomic_bool>(false));
    check(result.state==ExecutionState::Done && result.acquisitions==1 && result.executedTools==QStringList{source,pre,last},"real auto-connected DAG execution");
    QTextStream(stdout)<<"Tool creation policy: three application types and real DAG passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
