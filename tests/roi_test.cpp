#include "application/RoiService.hpp"
#include "application/EditorSession.hpp"
#include "application/RuntimeService.hpp"
#include "application/ToolCatalog.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QTextStream>
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
    QTextStream(stdout)<<checks<<" ROI checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
