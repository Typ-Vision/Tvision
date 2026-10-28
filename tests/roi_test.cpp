#include "application/RoiService.hpp"
#include "application/EditorSession.hpp"
#include "application/RuntimeService.hpp"
#include "application/ToolCatalog.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QTextStream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include "application/ToolResultCodec.hpp"
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(...){failed=true;}check(failed,"expected rejection");}
const ToolResult& result(const CycleResult& cycle,const QString& id){for(const auto& r:cycle.tools)if(r.toolId==id)return r;throw std::runtime_error("missing tool");}
double count(const CycleResult& cycle,const QString& id){return std::get<double>(result(cycle,id).outputs["count"].data);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/roi-XXXXXX");check(dir.isValid(),"fixture directory");
    QImage pixels(200,120,QImage::Format_Grayscale8);pixels.fill(0);
    for(int y=20;y<40;++y)for(int x=20;x<40;++x)pixels.scanLine(y)[x]=255;
    for(int y=60;y<90;++y)for(int x=140;x<170;++x)pixels.scanLine(y)[x]=255;
    const auto path=dir.filePath("input.png");check(pixels.save(path),"fixture save");
    check(ToolCatalog::canAdd("roi") && ToolCatalog::groupForKind("roi")=="processing","PreProcess catalog");
    for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        ProjectService project;project.createProject("ROI","",dir.filePath(typeKey(type)+".vfproject"),"file");
        project.createRecipe("ROI fixture","",type,"file");
        const auto source=project.setImageFile(path),pre=project.addTool("preprocess"),roi=project.addTool("roi"),blob=project.addTool("blob");
        QVector<ToolConnection> links{{source,"image",pre,"image"},{pre,"image",roi,"image"},{pre,"image",blob,"image"}};
        project.setConnections(links);
        const auto recipe=project.activeRecipe()->id;
        auto run=[&]{return ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),recipe,"config",{});};
        auto full=run();check(result(full,roi).state==ExecutionState::Error && count(full,blob)==2,"undrawn ROI error and full domain oracle");
        const auto image=std::get<ImageFrame>(result(full,pre).outputs["image"].data);const auto hash=image.contentId();
        auto teach=[&](QRect r){for(auto t:project.tools())if(t.id==roi){t.name="Inspection area";t.parameters=RoiService::teach(image,r);project.updateTool(t);}};
        teach({10,10,50,40});links.append({roi,"region",blob,"region"});project.setConnections(links);
        full=run();check(full.state==ExecutionState::Done && count(full,blob)==1,"ROI excludes outside object");
        check(std::get<double>(result(full,blob).outputs["area"].data)==400 && result(full,blob).searchRegion==QRect(10,10,50,40),"actual inside area and operation bounds");
        check(result(full,roi).outputs["region"].origin.sourceId==source && result(full,roi).outputs["region"].origin.coordinateSystemId==image.metadata().coordinateSystemId && image.contentId()==hash && image.size()==QSize(200,120),"no crop or origin/processing mutation");
        project.save();const auto definition=project.activeRecipe()->definition;project.openProject(project.path());project.selectRecipe(recipe);
        check(project.activeRecipe()->definition==definition && count(run(),blob)==1,"name ROI and links survive reopen");
        EditorSession editor;auto context=[&]{return EditorContext{project.project().id,project.path(),recipe,project.activeRecipe()->definition,{}};};auto c=context();
        full=ExecutionService{}.run(c.definition,dir.path(),recipe,editor.signature(c),{});check(editor.acceptCycle(full,editor.signature(c),c),"accept ROI flow");
        auto target=ExecutionService{}.runTool(c.definition,dir.path(),recipe,editor.signature(c),{},blob,editor.evaluation(c));check(target.state==ExecutionState::Done && target.acquisitions==0 && count(target,blob)==1,"Run Tool consumes current ROI");
        teach({130,50,50,50});c=context();editor.reconcile(c);
        target=ExecutionService{}.runTool(c.definition,dir.path(),recipe,editor.signature(c),{},blob,editor.evaluation(c));check(target.executedTools.isEmpty(),"changed ROI invalidates downstream cache");
        target=ExecutionService{}.runTool(c.definition,dir.path(),recipe,editor.signature(c),{},roi,editor.evaluation(c));check(target.state==ExecutionState::Done && target.acquisitions==0 && editor.acceptTool(target,editor.signature(c),c),"Run Tool updates ROI without acquisition");
        target=ExecutionService{}.runTool(c.definition,dir.path(),recipe,editor.signature(c),{},blob,editor.evaluation(c));check(count(target,blob)==1 && std::get<double>(result(target,blob).outputs["area"].data)==900,"changed ROI selects outside object");
        project.save();RuntimeService runtime(dir.filePath(typeKey(type)+"-records"),project.path()+".calibrations.json",RuntimeIdentity::localEngineer());runtime.prepare(*project.activeRecipe(),dir.path());const auto lease=runtime.begin();
        teach({80,10,30,30}); // Running snapshot must retain its original ROI.
        auto cycle=RuntimeService::execute(lease,project.path()+".calibrations.json",{});check(cycle.state==ExecutionState::Done && std::get<double>(result(cycle,blob).outputs["area"].data)==900,"Runtime immutable ROI snapshot");runtime.finish(lease.ticket,cycle);check(runtime.records().size()==1,"Runtime terminal persistence");
        check(count(run(),blob)==0,"empty ROI produces actual zero count");
        for(auto t:project.tools())if(t.id==roi){t.parameters["searchRoi"]=MatchService::rectangle({190,110,20,20});project.updateTool(t);}
        auto bad=run();check(result(bad,roi).state==ExecutionState::Error && result(bad,roi).outputs.isEmpty() && result(bad,blob).state==ExecutionState::Skipped,"out of bounds fails closed");
        teach({10,10,50,40});
        for(auto t:project.tools())if(t.id==roi){t.parameters["shape"]="circle";reject([&]{project.updateTool(t);});t.parameters.remove("shape");t.parameters["searchRoi"]=QJsonObject{{"x",1.5},{"y",0},{"width",10},{"height",10}};reject([&]{project.updateTool(t);});}
        auto changed=image.metadata();changed.sourceId="different-source";
        ToolDefinition roiTool;for(const auto& t:project.tools())if(t.id==roi)roiTool=t;
        auto direct=[&](const ImageFrame& f){return RoiService::execute(roiTool,{{"image",{inputPorts("roi").first(),f,true,f.metadata().frameId,f.metadata().cycleId,f.metadata()}}},f.metadata().frameId,f.metadata().cycleId);};
        reject([&]{direct(image.withMetadata(changed));});changed=image.metadata();changed.coordinateSystemId="other-axes";reject([&]{direct(image.withMetadata(changed));});
        QImage larger(201,120,QImage::Format_Grayscale8);larger.fill(0);auto resized=ImageFrame::fromBuffer(larger.constBits(),larger.sizeInBytes(),201,120,larger.bytesPerLine(),PixelFormat::Gray8,image.metadata());reject([&]{direct(resized);});
        auto wrongPort=links;wrongPort.append({roi,"region",pre,"region"});check(!validateGraph(project.tools(),wrongPort).valid(),"unsupported Region input rejected");
        auto stop=std::make_shared<std::atomic_bool>(true);const auto stopped=ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),recipe,"config",stop);check(stopped.state==ExecutionState::Skipped,"Stop suppresses ROI cycle");for(const auto& r:stopped.tools)check(r.outputs.isEmpty(),"Stop clears ROI and consumer outputs");
        project.enableMultiSource();const auto other=project.addTool("image_file");project.setImageFile(path,other);
        auto multiLinks=links;multiLinks[1]={other,"image",roi,"image"};project.setConnections(multiLinks);
        bad=run();check(bad.state==ExecutionState::Error,"cross-source Region binding rejected");
        multiLinks[2]={other,"image",blob,"image"};project.setConnections(multiLinks);bad=run();check(result(bad,roi).state==ExecutionState::Error,"same geometry different source needs reteach");
        const auto otherImage=std::get<ImageFrame>(result(bad,other).outputs["image"].data);for(auto t:project.tools())if(t.id==roi){t.parameters=RoiService::teach(otherImage,{10,10,50,40});project.updateTool(t);}
        check(run().state==ExecutionState::Done && count(run(),blob)==1,"multi-source ROI uses its bound source");
    }
    // Dedicated crop graph, independent pixel oracle, legacy codec and lifecycle.
    ProjectService crop;crop.createProject("Crop","",dir.filePath("crop.vfproject"),"file");
    const auto recipe=crop.createRecipe("Crop","",ApplicationType::Measurement,"file");
    const auto source=crop.setImageFile(path),r1=crop.addTool("roi"),r2=crop.addTool("roi"),pre=crop.addTool("preprocess"),branch=crop.addTool("preprocess");
    crop.setConnections({{source,"image",r1,"image"},{r1,"image",r2,"image"},{r2,"image",pre,"image"},{source,"image",branch,"image"}});
    auto run=[&]{return ExecutionService{}.run(crop.activeRecipe()->definition,dir.path(),recipe,"crop",{});};
    auto tool=[&](const QString& id){for(const auto& t:crop.tools())if(t.id==id)return t;throw std::runtime_error("tool missing");};
    auto full=run();const auto raw=std::get<ImageFrame>(result(full,source).outputs["image"].data);
    auto first=tool(r1);first.parameters=RoiService::teach(raw,{10,10,50,40});crop.updateTool(first);
    full=run();const auto pass=std::get<ImageFrame>(result(full,r1).outputs["image"].data);
    check(pass.contentId()==raw.contentId() && pass.processingId()==raw.processingId() && pass.geometry()==raw.geometry(),"false passes unchanged immutable frame");
    for(const auto bad:QJsonArray{QJsonValue(),QJsonValue(0),QJsonValue("true")}){auto invalid=first;invalid.parameters["cropImage"]=bad;reject([&]{crop.updateTool(invalid);});}
    check(outputPorts("roi").size()==2 && outputPorts("roi")[0].id=="region" && outputPorts("roi")[1].id=="image" && ToolRegistry::get("roi").version==2,"stable port order and revision");
    first.parameters["cropImage"]=true;crop.updateTool(first);full=run();
    const auto child=std::get<ImageFrame>(result(full,r1).outputs["image"].data);
    check(child.size()==QSize(50,40) && child.geometry().rootOffset()==QPoint(10,10) && child.preview().pixelColor(10,10).red()==255 && child.preview().pixelColor(0,0).red()==0,"real child pixels and offset");
    check(result(full,r1).outputs["region"].origin.coordinateSystemId==raw.metadata().coordinateSystemId && result(full,r1).outputs["image"].origin.coordinateSystemId==child.metadata().coordinateSystemId && result(full,r1).outputs["region"].imageGeometry==raw.geometry(),"two ROI ports have distinct own scopes");
    auto second=tool(r2);second.parameters=RoiService::teach(child,{5,5,30,30},true);crop.updateTool(second);full=run();
    const auto nested=std::get<ImageFrame>(result(full,r2).outputs["image"].data);
    check(full.state==ExecutionState::Done && nested.size()==QSize(30,30) && nested.geometry().rootOffset()==QPoint(15,15) && std::get<ImageFrame>(result(full,pre).outputs["image"].data).geometry()==nested.geometry(),"nested ROI then preprocess");
    EditorSession editor;auto context=[&]{return EditorContext{crop.project().id,crop.path(),recipe,crop.activeRecipe()->definition,{}};};auto c=context();
    full=ExecutionService{}.run(c.definition,dir.path(),recipe,editor.signature(c),{});check(editor.acceptCycle(full,editor.signature(c),c),"accept child graph");
    auto target=ExecutionService{}.runTool(c.definition,dir.path(),recipe,editor.signature(c),{},pre,editor.evaluation(c));check(target.state==ExecutionState::Done && target.acquisitions==0,"cached child input validates acquisition ancestry");
    const auto branchSignature=ExecutionService::toolSignature(c.definition,dir.path(),branch,"scope");
    editor.references[r1]={editor.referenceSignature(c,r1),QPointF(10,10)};editor.resultSignature="old";
    first.parameters["cropImage"]=false;crop.updateTool(first);c=context();editor.reconcile(c);
    check(!editor.references.contains(r1) && editor.resultSignature.isEmpty(),"upstream edit discards cached teaching anchor and presentation");
    check(tool(r2).parameters["imageBinding"].toObject().isEmpty(),"mode edit clears downstream ROI teaching");
    check(ExecutionService::toolSignature(c.definition,dir.path(),branch,"scope")==branchSignature,"unrelated branch signature unchanged");
    first.parameters["cropImage"]=true;crop.updateTool(first);c=context();editor.reconcile(c);
    target=ExecutionService{}.runTool(c.definition,dir.path(),recipe,editor.signature(c),{},pre,editor.evaluation(c));check(target.executedTools.isEmpty(),"A B A does not revive downstream result");
    full=run();check(result(full,r2).state==ExecutionState::Error && result(full,r2).outputs.isEmpty(),"A B A does not revive ROI teaching");
    second=tool(r2);second.parameters=RoiService::teach(child,{5,5,30,30},true);crop.updateTool(second);check(run().state==ExecutionState::Done,"explicit reteach restores permission");
    // Same size, different rect also invalidates teaching, including save/reopen.
    first.parameters=RoiService::teach(raw,{11,10,50,40},true);crop.updateTool(first);first.parameters=RoiService::teach(raw,{10,10,50,40},true);crop.updateTool(first);
    crop.save();crop.openProject(crop.path());crop.selectRecipe(recipe);check(tool(r2).parameters["imageBinding"].toObject().isEmpty(),"rect A B A stale binding survives reopen");
    second=tool(r2);second.parameters=RoiService::teach(child,{5,5,30,30},true);crop.updateTool(second);
    // Cross-scope Region+Image is a real execution error, never a hidden rebase.
    const auto blob=crop.addTool("blob");auto links=editorConnections(crop.activeRecipe()->definition);for(auto& l:links)if(l.targetTool==blob)l.sourceTool=r1;links.append({r1,"region",blob,"region"});crop.setConnections(links);
    full=run();check(result(full,blob).state==ExecutionState::Error && result(full,blob).outputs.isEmpty(),"parent region and child image rejected");
    crop.removeTool(blob);
    // Missing boolean is normalized only on read for schema 7/8/9.
    for(const int schema:{7,8,9}){
        auto definition=crop.activeRecipe()->definition;auto e=definition["editor"].toObject();auto ts=e["tools"].toArray();for(int n=0;n<ts.size();++n){auto t=ts[n].toObject();if(t["kind"]=="roi"){auto p=t["parameters"].toObject();p.remove("cropImage");t["parameters"]=p;ts[n]=t;}}
        e["tools"]=ts;e["schemaVersion"]=schema;definition["editor"]=e;
        const auto decoded=editorTools(definition);for(const auto& t:decoded)if(t.kind=="roi")check(t.parameters["cropImage"].isBool() && !t.parameters["cropImage"].toBool(),"legacy missing crop false");
        const auto canonical=withEditorTools(definition,decoded);check(withEditorTools(canonical,editorTools(canonical))==canonical && editorConnections(canonical)==editorConnections(definition),"canonical save/reopen connections stable");
        crop.save();QFile file(crop.path());check(file.open(QIODevice::ReadOnly),"read project fixture");auto root=QJsonDocument::fromJson(file.readAll()).object();file.close();auto recipes=root["recipes"].toArray();auto record=recipes[0].toObject();record["definition"]=definition;recipes[0]=record;root["recipes"]=recipes;
        const auto fixture=dir.filePath(QString("legacy-roi-%1.vfproject").arg(schema));QFile legacyFile(fixture);check(legacyFile.open(QIODevice::WriteOnly) && legacyFile.write(QJsonDocument(root).toJson())>0,"write independent old two-field fixture");legacyFile.close();
        ProjectService legacy;legacy.openProject(fixture);legacy.selectRecipe(recipe);check(!legacy.dirty() && legacy.tools()[1].id==r1 && legacy.tools()[1].parameters["cropImage"]==false,"disk open normalizes without dirty/UUID loss");
        auto renamed=legacy.tools()[1];renamed.name="Legacy renamed";legacy.updateTool(renamed);legacy.save();const auto saved=legacy.activeRecipe()->definition;legacy.openProject(fixture);legacy.selectRecipe(recipe);check(legacy.activeRecipe()->definition==saved && editorConnections(saved)==editorConnections(definition),"old ROI saved as canonical boolean and connections retained");
    }
    // Crop budget and stop fail without publishing partial ROI outputs.
    crop.enableMultiSource();full=run();check(result(full,r1).state==ExecutionState::Done && result(full,r2).state==ExecutionState::Done,"multi-source child ancestry");
    const auto budget=quint64(raw.size().width())*raw.size().height()*12+100;
    const auto limited=ExecutionService{}.run(crop.activeRecipe()->definition,dir.path(),recipe,"budget",{},{},{},{},nullptr,{},ExecutionLimits{30000,budget});
    check(result(limited,r1).state==ExecutionState::Error && result(limited,r1).outputs.isEmpty(),"crop allocation budget fails closed");
    // Renaming is non-geometric; upstream source configuration edits are sticky.
    first=tool(r1);auto renamed=tool(source);renamed.name="Source renamed";crop.updateTool(renamed);check(tool(r1).parameters["imageBinding"]==first.parameters["imageBinding"],"rename preserves teaching");
    const auto originalSource=renamed;renamed.parameters["path"]="other.png";crop.updateTool(renamed);crop.updateTool(originalSource);
    check(tool(r1).parameters["imageBinding"].toObject().isEmpty() && tool(r2).parameters["imageBinding"].toObject().isEmpty(),"source A B A cannot restore ROI teaching");

    QTextStream(stdout)<<checks<<" ROI checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
