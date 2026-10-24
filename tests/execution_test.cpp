#include "application/ExecutionService.hpp"
#include "application/ToolExecutors.hpp"
#include "domain/ToolRegistry.hpp"
#include <QUuid>
#include "application/ProjectService.hpp"
#include "application/MatchService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QJsonArray>
#include <opencv2/imgcodecs.hpp>
#include <stdexcept>
#include <algorithm>
using namespace visionflow;
namespace {
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void image(const QString& path,bool bright) {
    cv::Mat m(60,100,CV_8UC1,cv::Scalar(0));if(bright)m(cv::Rect(20,10,40,30)).setTo(255);
    std::vector<uchar> bytes;cv::imencode(".png",m,bytes);QFile f(path);check(f.open(QIODevice::WriteOnly),"fixture open");f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
}
const ToolResult& result(const CycleResult& c,const QString& id) {for(const auto& r:c.tools)if(r.toolId==id)return r;throw std::runtime_error("result missing");}
double number(const CycleResult& c,const QString& id,const QString& port) {return std::get<double>(result(c,id).outputs[port].data);}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        ToolRegistry::registerType({"test_constant","Test constant",1,{}, {}, {{"value",PortType::Number,"px","image"}},[](const ToolDefinition&){}});
        ToolExecutors::registerExecutor("test_constant",[](const ToolDefinition& t,const QMap<QString,PortValue>&,const QString&,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>&) {
            ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;r.outputs["value"]={outputPorts(t.kind).first(),42.0,true,frame,cycle,{t.id,frame,cycle,t.id+"/image"}};return r;
        });
        ToolDefinition extension{QUuid::createUuid().toString(),"test_constant","Constant","",true,{}};
        auto extensionCycle=ExecutionService{}.run(withEditorTools({}, {extension}),{},"test","test",{});
        check(extensionCycle.state==ExecutionState::Done && number(extensionCycle,extension.id,"value")==42,"extension executes without engine kind branch");
        bool rejected=false;try{outputPorts("unknown");}catch(const std::exception&){rejected=true;}check(rejected,"unknown port kind rejected");
        for(const auto& descriptor:ToolRegistry::entries())check(descriptor.version==(descriptor.kind=="blob" ? 4 : ((descriptor.kind=="line_fit" || descriptor.kind=="blob" || descriptor.kind=="edge_point" || descriptor.kind=="circle_fit") ? 3 : (descriptor.kind=="code_read" || descriptor.kind=="measurement" || descriptor.kind=="geometry_measurement" || descriptor.kind=="edge_point" || descriptor.kind=="edge_pair" || descriptor.kind=="circle_fit") ? 2 : 1)) && descriptor.label==toolLabel(descriptor.kind),"registry version/label");
        QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("execution-XXXXXX"));check(dir.isValid(),"test directory");const auto path=dir.filePath("fixture.png");image(path,true);
        ProjectService service;service.createProject("Graph","",dir.filePath("graph.vfproject"),"file");const auto recipe=service.createRecipe("Measure","",ApplicationType::Measurement,"file");
        const auto source=service.setImageFile(path),left=service.addTool("edge"),right=service.addTool("edge"),blob=service.addTool("blob"),distance=service.addTool("calculate");
        auto tools=service.tools();for(auto t:tools)if(t.id==right){t.parameters["direction"]="right_to_left";service.updateTool(t);}
        QVector<ToolConnection> connections={{source,"image",left,"image"},{source,"image",right,"image"},{source,"image",blob,"image"},{left,"point",distance,"p1"},{right,"point",distance,"p2"}};
        check(!validateGraph(service.tools(),{}).valid(),"required inputs");check(validateGraph(service.tools(),connections).valid(),"valid graph");
        auto bad=connections;bad[0].sourcePort="point";check(!validateGraph(service.tools(),bad).valid(),"missing port");bad=connections;bad[0].sourceTool="missing";check(!validateGraph(service.tools(),bad).valid(),"missing tool");bad=connections;bad[0].sourceTool=left;bad[0].sourcePort="point";check(!validateGraph(service.tools(),bad).valid(),"type mismatch and self cycle");
        bad=connections;bad.append(connections[0]);check(!validateGraph(service.tools(),bad).valid(),"duplicate binding");
        bad=connections;bad.append({blob,"region",blob,"region"});check(!validateGraph(service.tools(),bad).valid(),"typed cycle");
        bad=connections;bad.append({left,"strength",distance,"p1"});check(!validateGraph(service.tools(),bad).valid(),"unit/type mismatch");
        service.setConnections(connections);service.save();service.openProject(service.path());service.selectRecipe(recipe);check(editorConnections(service.activeRecipe()->definition)==connections,"connections roundtrip");
        auto legacy=service.activeRecipe()->definition;auto e=legacy["editor"].toObject();e["schemaVersion"]=2;e.remove("connections");auto array=e["tools"].toArray();
        for(int i=0;i<array.size();++i){auto o=array[i].toObject();auto p=o["parameters"].toObject();if(o["kind"]=="edge"){p.remove("searchRoi");p.remove("minContrast");}if(o["kind"]=="blob")p.remove("searchRoi");o["parameters"]=p;array[i]=o;}e["tools"]=array;legacy["editor"]=e;
        const auto migrated=withEditorTools(legacy,editorTools(legacy));check(editorTools(migrated)[1].id==left && editorTools(migrated)[1].parameters["minContrast"].toInt()==20,"schema2 ID/default migration");check(editorConnections(migrated).size()==3,"legacy image bindings");
        auto t=service.tools()[1];t.name="Renamed";service.updateTool(t);check(editorConnections(service.activeRecipe()->definition)==connections,"rename stable ID");
        auto cancel=std::make_shared<std::atomic_bool>(false);auto run=[&]{return ExecutionService{}.run(service.activeRecipe()->definition,dir.path(),recipe,"config",cancel);};
        auto c=run();check(c.state==ExecutionState::Done,"cycle DONE");check(number(c,blob,"count")==1 && number(c,blob,"area")==1200,"blob area/count");check(number(c,distance,"value")==39,"distance from directional edges");
        check(result(c,left).lines==QVector<QLineF>{QLineF(20,0,20,59)},"edge boundary overlay in image coordinates");
        check(result(c,distance).points==QVector<QPointF>{QPointF(20,29.5),QPointF(59,29.5)},"measurement overlay coordinates");
        for(const auto& r:c.tools)for(const auto& v:r.outputs)check(v.cycleId==c.cycleId && v.frameId==c.frameId,"frame cycle coherence");
        auto c2=run();check(c2.cycleId!=c.cycleId && c2.frameId!=c.frameId,"unique identities");
        image(path,false);c=run();check(!result(c,left).outputs["point"].valid,"no edge point");check(result(c,distance).state==ExecutionState::Skipped && result(c,distance).outputs.isEmpty(),"not found prevents calculation");check(number(c,blob,"count")==0,"empty segmentation");image(path,true);
        t=service.tools()[1];t.enabled=false;service.updateTool(t);c=run();check(result(c,left).state==ExecutionState::Skipped && result(c,distance).state==ExecutionState::Skipped,"disabled dependency");t.enabled=true;service.updateTool(t);
        {QFile f(path);check(f.open(QIODevice::WriteOnly),"corrupt fixture open");f.write("broken");}c=run();check(c.state==ExecutionState::Error && result(c,source).state==ExecutionState::Error && result(c,distance).state==ExecutionState::Skipped,"acquisition error propagation");image(path,true);
        cancel->store(true);c=run();check(c.state==ExecutionState::Skipped,"cancel");for(const auto& r:c.tools)check(r.outputs.isEmpty(),"cancel no output");cancel->store(false);
        t=service.tools()[1];t.parameters["searchRoi"]=MatchService::rectangle(QRect(90,0,20,20));service.updateTool(t);c=run();check(result(c,left).state==ExecutionState::Error,"ROI bounds");t.parameters["searchRoi"]=QJsonObject{};service.updateTool(t);
        t=service.tools()[1];t.parameters["direction"]="top_to_bottom";service.updateTool(t);c=run();check(std::get<QPointF>(result(c,left).outputs["point"].data).y()==10,"vertical scan");
        t.parameters["direction"]="bottom_to_top";service.updateTool(t);c=run();check(std::get<QPointF>(result(c,left).outputs["point"].data).y()==39,"reverse vertical scan");t.parameters["direction"]="left_to_right";service.updateTool(t);
        const auto match=service.addTool("template_match");for(auto m:service.tools())if(m.id==match){m.parameters["modelRoi"]=MatchService::rectangle(QRect(10,5,25,25));m.parameters["modelPng"]=MatchService{}.teach(ImageFileService{}.load(path),QRect(10,5,25,25));service.updateTool(m);}
        connections.append({source,"image",match,"image"});connections.append({match,"region",blob,"region"});service.setConnections(connections);c=run();check(result(c,match).state==ExecutionState::Done && std::get<bool>(result(c,match).outputs["found"].data),"match adapter in DAG");check(number(c,blob,"area")==300,"typed match region drives blob ROI");
        {
            // Both scopes consume the exact same immutable frame, even if the file changes.
            const auto snapshot=ImageFileService{}.load(path);
            const auto roiSource=service.addTool("blob");
            auto bindings=connections;bindings.append({source,"image",roiSource,"image"});bindings.append({roiSource,"region",match,"region"});service.setConnections(bindings);
            for(auto tool:service.tools())if(tool.id==match){tool.parameters["searchRoi"]=MatchService::rectangle(QRect(90,50,5,5));service.updateTool(tool);}
            const auto definition=service.activeRecipe()->definition;
            auto scoped=[&](const QJsonObject& def,const QString& target){return ExecutionService{}.run(def,dir.path(),recipe,"snapshot",cancel,target,snapshot);};
            const auto full=scoped(definition,{}),single=scoped(definition,match);
            check(single.state==ExecutionState::Done && single.tools.size()==3,"single tool executes only source/ROI/target");
            check(result(single,match).searchRegion==QRect(20,10,40,30) && result(single,match).searchRegion==result(full,match).searchRegion,"bound ROI overrides local ROI in both scopes");
            check(number(single,match,"score")==number(full,match,"score") && std::get<QRect>(result(single,match).outputs["region"].data)==std::get<QRect>(result(full,match).outputs["region"].data),"same snapshot match outcome");
            image(path,false);check(number(scoped(definition,match),match,"score")==number(single,match,"score"),"snapshot independent of later file changes");
            const auto fresh=ExecutionService{}.run(definition,dir.path(),recipe,"fresh",cancel,match);
            check(result(fresh,match).state==ExecutionState::Skipped,"fresh acquisition not-found ROI skips match");image(path,true);
            auto changed=editorTools(definition);for(auto& tool:changed)if(tool.id==roiSource)tool.enabled=false;
            check(result(scoped(withEditorTools(definition,changed),match),match).state==ExecutionState::Skipped,"disabled ROI upstream skips single match");
            changed=editorTools(definition);for(auto& tool:changed)if(tool.id==source)tool.enabled=false;
            check(result(scoped(withEditorTools(definition,changed),match),match).state==ExecutionState::Skipped,"disabled image source skips single match");
            changed=editorTools(definition);for(auto& tool:changed)if(tool.id==roiSource)tool.parameters["searchRoi"]=MatchService::rectangle(QRect(99,59,10,10));
            const auto invalidRoi=scoped(withEditorTools(definition,changed),match);
            check(result(invalidRoi,roiSource).state==ExecutionState::Error && result(invalidRoi,match).state==ExecutionState::Skipped,"upstream adapter error skips single match");
            auto broken=bindings;broken.last().sourceTool="missing";
            check(scoped(withEditorConnections(definition,broken),match).state==ExecutionState::Error,"missing upstream rejected");
            broken=bindings;broken.last().sourcePort="point";
            check(scoped(withEditorConnections(definition,broken),match).state==ExecutionState::Error,"invalid upstream port rejected");
            broken=bindings;broken.append({match,"region",roiSource,"region"});
            check(scoped(withEditorConnections(definition,broken),match).state==ExecutionState::Error,"upstream cycle rejected");
            broken=bindings;broken.erase(std::remove_if(broken.begin(),broken.end(),[&](const auto& binding){return binding.targetTool==distance && binding.targetPort=="p1";}),broken.end());
            auto incomplete=withEditorConnections(definition,broken);
            check(scoped(incomplete,match).state==ExecutionState::Done && scoped(incomplete,{}).state==ExecutionState::Error,"unrelated incomplete branch excluded from single tool");
            cancel->store(true);const auto stopped=scoped(definition,match);cancel->store(false);
            check(stopped.state==ExecutionState::Skipped,"single tool cancellation");for(const auto& r:stopped.tools)check(r.outputs.isEmpty() && r.regions.isEmpty() && r.searchRegion.isEmpty(),"single cancel suppresses all results");
            check(scoped(definition,"missing").state==ExecutionState::Error,"missing target rejected");
            service.removeTool(roiSource);service.setConnections(connections);
            for(auto tool:service.tools())if(tool.id==match){tool.parameters["searchRoi"]=QJsonObject{};service.updateTool(tool);}
        }
        for(auto m:service.tools())if(m.id==match){m.parameters["modelPng"]="";service.updateTool(m);}c=run();check(result(c,match).state==ExecutionState::Error && result(c,blob).state==ExecutionState::Skipped && result(c,distance).state==ExecutionState::Done,"branch errors isolated");
        check(result(c,distance).points.size()==2,"measurement endpoints retained");
        service.removeTool(left);
        for(const auto& binding:editorConnections(service.activeRecipe()->definition))check(binding.sourceTool!=left && binding.targetTool!=left,"deleted tool connections removed");
        check(!validateGraph(service.tools(),editorConnections(service.activeRecipe()->definition)).valid(),"measurement requires replacement point");
        const auto temporary=service.addTool("edge");service.removeTool(temporary);
        service.removeTool(distance);service.removeTool(match);
        check(validateGraph(service.tools(),editorConnections(service.activeRecipe()->definition)).valid(),"unrelated graph recovers after removal");service.save();
        {
            ProjectService tracked;tracked.createProject("Tracked","",dir.filePath("tracked.vfproject"),"file");const auto trackedRecipe=tracked.createRecipe("Tracked edge","",ApplicationType::Measurement,"file");
            const auto trackedPath=dir.filePath("tracked.png");cv::Mat texture(16,16,CV_8UC1);cv::RNG rng(947);rng.fill(texture,cv::RNG::UNIFORM,30,180);
            auto movedImage=[&](int dx,int dy,bool visible=true){cv::Mat pixels(120,200,CV_8UC1,cv::Scalar(10));if(visible){texture.copyTo(pixels(cv::Rect(20+dx,20+dy,16,16)));pixels(cv::Rect(100+dx,50+dy,40,20)).setTo(230);}std::vector<uchar> bytes;cv::imencode(".png",pixels,bytes);QFile f(trackedPath);check(f.open(QIODevice::WriteOnly),"tracked fixture");f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());};
            movedImage(0,0);const auto s=tracked.setImageFile(trackedPath),m=tracked.addTool("template_match"),a=tracked.addTool("edge"),b=tracked.addTool("edge"),d=tracked.addTool("calculate");
            for(auto tool:tracked.tools()) {
                if(tool.id==m){tool.parameters["modelRoi"]=MatchService::rectangle(QRect(20,20,16,16));tool.parameters["modelPng"]=MatchService{}.teach(ImageFileService{}.load(trackedPath),QRect(20,20,16,16));tool.parameters["minScore"]=0.95;}
                if(tool.id==a || tool.id==b){tool.parameters["searchRoi"]=MatchService::rectangle(tool.id==a ? QRect(90,48,20,24) : QRect(130,48,20,24));tool.parameters["referenceOrigin"]=QJsonObject{{"x",27.5},{"y",27.5}};if(tool.id==b)tool.parameters["direction"]="right_to_left";}
                tracked.updateTool(tool);
            }
            tracked.setConnections({{s,"image",m,"image"},{s,"image",a,"image"},{s,"image",b,"image"},{m,"point",a,"reference"},{m,"point",b,"reference"},{a,"point",d,"p1"},{b,"point",d,"p2"}});
            auto execute=[&]{return ExecutionService{}.run(tracked.activeRecipe()->definition,dir.path(),trackedRecipe,"tracked",cancel);};
            auto original=execute();check(original.state==ExecutionState::Done && result(original,a).searchRegion==QRect(90,48,20,24),"original reference ROI");check(number(original,d,"value")==39,"original tracked measurement");
            movedImage(30,15);auto moved=execute();check(moved.state==ExecutionState::Done,"translated cycle");check(result(moved,a).searchRegion==QRect(120,63,20,24),"ROI tracks XY reference preserving size and offset");check(std::get<QPointF>(result(moved,a).outputs["point"].data)==QPointF(130,74.5),"edge found away from match at moved box");check(number(moved,d,"value")==39,"measurement invariant under translation");
            tracked.save();tracked.openProject(tracked.path());tracked.selectRecipe(trackedRecipe);check(result(execute(),a).searchRegion==QRect(120,63,20,24),"reference teaching survives reopen");
            movedImage(-10,-10);check(result(execute(),a).searchRegion==QRect(80,38,20,24),"negative XY displacement");
            movedImage(60,0);auto outside=execute();check(result(outside,b).state==ExecutionState::Error && result(outside,d).state==ExecutionState::Skipped,"translated ROI outside frame rejects measurement");
            movedImage(0,0,false);auto absent=execute();check(result(absent,a).state==ExecutionState::Skipped && result(absent,a).lines.isEmpty(),"missing reference skips edge without stale overlay");
            movedImage(0,0);auto old=tracked.activeRecipe()->definition;auto editor=old["editor"].toObject();editor["schemaVersion"]=3;auto oldTools=editor["tools"].toArray();for(int i=0;i<oldTools.size();++i){auto o=oldTools[i].toObject();if(o["kind"]=="edge"){auto p=o["parameters"].toObject();p.remove("referenceOrigin");o["parameters"]=p;}oldTools[i]=o;}editor["tools"]=oldTools;auto oldBindings=editor["connections"].toArray();for(int i=0;i<oldBindings.size();++i){auto o=oldBindings[i].toObject();if(o["targetPort"]=="reference"){o["targetPort"]="region";o["sourcePort"]="region";}oldBindings[i]=o;}editor["connections"]=oldBindings;old["editor"]=editor;
            auto migrated=withEditorTools(old,editorTools(old));check(editorConnections(migrated)[3].targetPort=="reference","old match-region edge binding migrates");check(ExecutionService{}.run(migrated,dir.path(),trackedRecipe,"migration",cancel).state==ExecutionState::Done,"legacy match anchor migration executes");
            auto legacy4=tracked.activeRecipe()->definition;auto e4=legacy4["editor"].toObject();e4["schemaVersion"]=4;
            auto array4=e4["tools"].toArray();for(int i=0;i<array4.size();++i){auto o=array4[i].toObject();if(o["kind"]=="edge"){auto p=o["parameters"].toObject();p.remove("referenceGeometry");o["parameters"]=p;}array4[i]=o;}e4["tools"]=array4;legacy4["editor"]=e4;
            auto project4=tracked.project();project4.recipes[0].definition=legacy4;const auto legacyPath=dir.filePath("legacy4.vfproject");ProjectRepository{}.save(legacyPath,project4);
            ProjectService reopened;reopened.openProject(legacyPath);reopened.selectRecipe(trackedRecipe);check(!reopened.dirty(),"legacy migration does not dirty open");
            check(ExecutionService{}.run(reopened.activeRecipe()->definition,dir.path(),trackedRecipe,"legacy4",cancel).state==ExecutionState::Done,"schema4 baseline preserves existing relationship");
            for(auto tool:tracked.tools())if(tool.id==m){tool.name="Renamed model";tool.description="metadata";tool.parameters["minScore"]=0.9;tool.parameters["searchRoi"]=MatchService::rectangle(QRect(10,10,70,70));tracked.updateTool(tool);}
            check(execute().state==ExecutionState::Done,"metadata threshold and search settings preserve teaching");
            // New model center, with identical template pixels: the anchor is stale.
            for(auto tool:tracked.tools())if(tool.id==m){tool.parameters["modelRoi"]=MatchService::rectangle(QRect(21,20,16,16));tracked.updateTool(tool);}
            auto stale=execute();check(result(stale,m).state==ExecutionState::Done && result(stale,a).state==ExecutionState::Error && result(stale,b).state==ExecutionState::Error && result(stale,d).state==ExecutionState::Skipped,"model center change blocks edges and downstream");
            check(result(stale,a).outputs.isEmpty() && result(stale,a).searchRegion.isEmpty() && result(stale,a).lines.isEmpty(),"stale reference publishes no result or overlay");
            tracked.save();tracked.openProject(tracked.path());tracked.selectRecipe(trackedRecipe);check(result(execute(),a).state==ExecutionState::Error,"invalidation survives save reopen");
            auto sameBindings=editorConnections(tracked.activeRecipe()->definition);tracked.setConnections(sameBindings);check(!tracked.dirty() && result(execute(),a).state==ExecutionState::Error,"no-op bindings cannot bless stale reference");
            QString geometry;for(const auto& tool:tracked.tools())if(tool.id==m)geometry=referenceGeometry(tool);
            for(auto tool:tracked.tools())if(tool.id==a || tool.id==b){tool.parameters["referenceGeometry"]=geometry;tool.parameters["referenceOrigin"]=QJsonObject{{"x",27.5},{"y",27.5}};tracked.updateTool(tool);}
            check(execute().state==ExecutionState::Done && number(execute(),d,"value")==39,"reference re-teaching restores measurement");
            // Same center with a different embedded model must also invalidate.
            texture.at<uchar>(5,5)=250;movedImage(0,0);
            for(auto tool:tracked.tools())if(tool.id==m){tool.parameters["modelPng"]=MatchService{}.teach(ImageFileService{}.load(trackedPath),QRect(20,20,16,16));tracked.updateTool(tool);}
            const auto contentChanged=execute();check(result(contentChanged,m).state==ExecutionState::Done && result(contentChanged,a).state==ExecutionState::Error && result(contentChanged,d).state==ExecutionState::Skipped,"new model content at same center invalidates downstream");
            auto unstamped=tracked.tools();for(auto& tool:unstamped)if(tool.id==a)tool.parameters["referenceGeometry"]="";
            check(result(ExecutionService{}.run(withEditorTools(tracked.activeRecipe()->definition,unstamped),dir.path(),trackedRecipe,"missing stamp",cancel),a).state==ExecutionState::Error,"schema5 missing teaching identity rejected at execution");
            for(auto tool:tracked.tools())if(tool.id==a){tool.parameters["searchRoi"]=QJsonObject{};tracked.updateTool(tool);}check(result(execute(),a).state==ExecutionState::Error,"reference mode requires taught search box");
        }
        QTextStream(stdout)<<"PASS: DAG, adapters, identities, persistence, errors and cancellation\n";return 0;
    } catch(const std::exception& e) {QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}
}
