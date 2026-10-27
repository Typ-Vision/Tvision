#include "application/PoseRoiService.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "application/ProjectService.hpp"
#include "application/EditorSession.hpp"
#include "application/ToolExecutors.hpp"
#include "application/RuntimeService.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <QDir>
#include <QImage>
#include <QJsonDocument>
#include <QTextStream>
#include <cmath>
#include <numbers>
using namespace visionflow;
namespace {
int checks=0;
QByteArray fileBytes(const QString& path){QFile f(path);if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("Read legacy fixture");return f.readAll();}
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
template<class F>void reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
ToolResult result(const CycleResult& c,const QString& id){for(const auto& r:c.tools)if(r.toolId==id)return r;throw std::runtime_error("missing result");}
void done(const CycleResult& c){if(c.state!=ExecutionState::Done){QTextStream(stderr)<<c.error<<'\n';for(const auto& r:c.tools)QTextStream(stderr)<<r.toolId<<' '<<executionStateName(r.state)<<' '<<r.message<<'\n';}check(c.state==ExecutionState::Done,"DAG completes");}
QByteArray raster(){
    QByteArray b(256*256,char(30));
    for(int y=0;y<24;++y)for(int x=0;x<24;++x)b[(46+y)*256+51+x]=char(40+(x*37+y*71+x*y*13)%190);
    for(int y=46;y<=74;++y)for(int x=88;x<=132;++x)b[y*256+x]=char(std::lround(128+100*std::erf((y-60)/1.4)));
    for(int y=81;y<=109;++y)for(int x=88;x<=102;++x)b[y*256+x]=char(std::lround(28+100*(std::erf((y-90)/1.4)-std::erf((y-100)/1.4))));
    for(int y=108;y<=142;++y)for(int x=78;x<=112;++x)b[y*256+x]=char(std::lround(128+100*std::erf((std::hypot(x-95.,y-125.)-10)/1.4)));
    return b;
}
// Independent integer inverse raster rotation, without product transform or warp.
QByteArray rotate(const QByteArray& b){QByteArray out(256*256,char(30));for(int y=0;y<256;++y)for(int x=0;x<256;++x){const int sx=y-120+60,sy=60-(x-140);if(sx>=0 && sx<256 && sy>=0 && sy<256)out[y*256+x]=b[sy*256+sx];}return out;}
ImageFrame frame(const QByteArray& b,QString source){FrameMetadata m{source,"frame","cycle",source+"/image",QDateTime::currentDateTimeUtc()};return ImageFrame::fromBuffer(b.data(),b.size(),256,256,256,PixelFormat::Gray8,m,false);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/tracking-dag-XXXXXX");check(dir.isValid(),"build fixtures");const auto bytes=raster(),moved=rotate(bytes);const auto path=dir.filePath("scene.png");
    check(QImage(reinterpret_cast<const uchar*>(bytes.constData()),256,256,256,QImage::Format_Grayscale8).save(path),"save actual PNG");
    for(const QString kind:{"pattern_pose","shape_pose"})for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}) {
        ProjectService project;project.createProject("Tracking","",dir.filePath(kind+QString::number(int(type))+".vfproject"),"file");project.createRecipe("Part","",type,"file");
        const auto camera=project.setImageFile(path),poseId=project.addTool(kind);auto tools=project.tools();ToolDefinition source;
        const auto teaching=frame(bytes,camera),current=frame(moved,camera);const CoordinateFrame raw{camera+"/image"};
        for(auto t:tools)if(t.id==poseId){t.parameters["angleMin"]=0.;t.parameters["angleMax"]=90.;t.parameters["angleStep"]=90.;t.parameters["pyramidLevels"]=1;t.parameters["minScore"]=.95;t.parameters=(kind=="shape_pose" ? ShapeMatchService::teach:PoseMatchService::teach)(teaching,{51,46,24,24},{9,14},t.parameters);project.updateTool(t);source=t;}
        auto taught=PoseMatchService::execute(source,{{"image",{{"image",PortType::Image,"px","image"},teaching,true,"frame","cycle",teaching.metadata()}}},"frame","cycle",{});for(auto& v:taught.outputs)v.evaluationId="teaching";
        QVector<ToolConnection> links{{camera,"image",poseId,"image"}};QStringList consumers;
        for(const QString consumerKind:{"edge_point","edge_pair","line_fit","circle_fit"}){
            const auto id=project.addTool(consumerKind);consumers.append(id);ToolDefinition consumer;for(const auto& t:project.tools())if(t.id==id)consumer=t;
            ToolRoi roi{raw,ScanSegment{{95,50},{95,70},2}};
            if(consumerKind=="edge_pair")roi.geometry=ScanSegment{{95,85},{95,105},2};
            if(consumerKind=="line_fit")roi.geometry=RotatedRectangle{{115,60},{24,20},0};
            if(consumerKind=="circle_fit")roi.geometry=Annulus{{95,125},5,15};
            if(consumerKind=="circle_fit")consumer.parameters=defaultParameters("circle_fit");consumer.parameters["scanRoi"]=roi.toJson();consumer.parameters["polarity"]=consumerKind=="edge_pair" ? "bright":"rising";
            consumer=PoseRoiService::teachTool(consumer,{"part/local"},source,taught,teaching);project.updateTool(consumer);
            links.append({camera,"image",id,"image"});links.append({poseId,"pose",id,"poseReference"});
            reject([&]{ToolExecutors::get(consumerKind)(consumer,{{"image",{{"image",PortType::Image,"px","image"},current,true,"frame","cycle",current.metadata()}}},dir.path(),"frame","cycle",{});});
            auto edited=consumer;edited.parameters["scanRoi"]=ToolRoi{raw,ScanSegment{{95,51},{95,71},2}}.toJson();reject([&]{ToolRegistry::validate(edited);});
            edited=consumer;auto envelope=edited.parameters["poseTracking"].toObject();envelope["extra"]=true;edited.parameters["poseTracking"]=envelope;reject([&]{ToolRegistry::validate(edited);});
            if(consumerKind!="edge_point"){edited=consumer;edited.parameters["mode"]="calibrated";reject([&]{ToolRegistry::validate(edited);});}
        }
        project.setConnections(links);project.save();const auto recipeId=project.activeRecipe()->id;const auto definition=project.activeRecipe()->definition;const auto revision=project.activeRecipe()->recipeVersion;
        const auto legacyBytes=fileBytes(project.path());const auto legacyHash=QCryptographicHash::hash(legacyBytes,QCryptographicHash::Sha256);
        project.openProject(project.path());project.selectRecipe(recipeId);project.updateTool(project.tools().first());project.save();check(fileBytes(project.path())==legacyBytes && QCryptographicHash::hash(fileBytes(project.path()),QCryptographicHash::Sha256)==legacyHash,"Legacy pose exact bytes/hash no-op");check(project.activeRecipe()->definition==definition && project.activeRecipe()->recipeVersion==revision && !project.dirty(),"tracking save/reopen no-op");
        auto run=[&](QJsonObject d,const ImageFrame& f){return ExecutionService{}.run(d,dir.path(),recipeId,"cfg",{}, {},f);};
        auto cycle=run(definition,current);done(cycle);check(cycle.acquisitions==0,"provided owned frame reused");
        auto edge=result(cycle,consumers[0]);check(edge.outputs["point"].valid && QLineF(std::get<QPointF>(edge.outputs["point"].data),{140,155}).length()<1.5,"rotated edge independent oracle");
        auto pair=result(cycle,consumers[1]);check(pair.outputs["widthPx"].valid && std::abs(std::get<double>(pair.outputs["widthPx"].data)-10)<.3,"rotated caliper width oracle");
        auto line=result(cycle,consumers[2]);check(line.outputs["linePx"].valid,"rotated line fit");const auto fitted=std::get<Line2D>(line.outputs["linePx"].data);check(std::abs(fitted.end.x()-fitted.start.x())<.3,"rotated line remains vertical");
        auto circle=result(cycle,consumers[3]);check(circle.outputs["radiusPx"].valid && std::abs(std::get<double>(circle.outputs["radiusPx"].data)-10)<.3 && QLineF(std::get<QPointF>(circle.outputs["centrePx"].data),{75,155}).length()<1.5,"rotated ring centre/radius oracle");
        EditorContext context{project.project().id,project.path(),recipeId,definition,{}};EditorSession session;cycle=ExecutionService{}.run(definition,dir.path(),recipeId,session.signature(context),{}, {},current);done(cycle);check(session.acceptCycle(cycle,session.signature(context),context),"cache accepted");const auto hash=session.frame->contentId();
        {
            ToolDefinition selected;for(const auto& t:project.tools())if(t.id==consumers.first())selected=t;
            const auto taughtByEditor=session.taughtTracking(context,selected,poseId,1.);
            const auto taughtPose=poseTrackingBinding(taughtByEditor).taughtPose;const auto actualPose=std::get<Pose2D>(result(cycle,poseId).outputs["pose"].data);
            check(taughtPose.position==actualPose.position && taughtPose.angleRadians==actualPose.angleRadians && taughtPose.frame==actualPose.frame,"shared editor teaching uses current actual cached pose");
            check(session.toolImage(context,selected.id).contentId()==current.contentId(),"shared editor teaching uses connected owned image");
            project.updateToolConnections(taughtByEditor,links);const auto atomicDefinition=project.activeRecipe()->definition;
            auto invalidLinks=links;invalidLinks.last().sourceTool=camera;
            reject([&]{project.updateToolConnections(taughtByEditor,invalidLinks);});
            check(project.activeRecipe()->definition==atomicDefinition,"invalid graph edit never partially commits teaching");
            project.updateToolConnections(taughtByEditor,links);check(project.activeRecipe()->definition==atomicDefinition,"shared teaching no-op");
            auto disconnected=taughtByEditor;disconnected.parameters.remove("poseTracking");auto unlinked=links;
            unlinked.erase(std::remove_if(unlinked.begin(),unlinked.end(),[&](const auto& c){return c.targetTool==selected.id && c.targetPort=="poseReference";}),unlinked.end());
            project.updateToolConnections(disconnected,unlinked);check(validateGraph(project.tools(),editorConnections(project.activeRecipe()->definition)).valid(),"atomic clear reference keeps nominal tool valid");
            // Restore the fixture's original teaching without changing the cache context.
            project.updateToolConnections(selected,links);check(project.activeRecipe()->definition==definition,"original teaching restored");
            auto stale=context;auto changed=editorTools(stale.definition);for(auto& t:changed)if(t.id==poseId)t.parameters["anchor"]=QJsonObject{{"x",10},{"y",14}};
            stale.definition=withEditorTools(stale.definition,changed);reject([&]{session.taughtTracking(stale,selected,poseId,1.);});
            EditorSession empty;empty.frame=current;reject([&]{empty.taughtTracking(context,selected,poseId,1.);});
        }
        for(const auto& id:consumers){auto target=ExecutionService{}.runTool(definition,dir.path(),recipeId,{}, {},id,session.evaluation(context));done(target);check(target.acquisitions==0 && target.executedTools==QStringList{id} && session.frame->contentId()==hash,"TargetOnly consumer uses full cached pose without acquisition");}
        auto evaluation=session.evaluation(context);evaluation.cache.remove(poseId);auto target=ExecutionService{}.runTool(definition,dir.path(),recipeId,{}, {},consumers[0],evaluation);check(target.state==ExecutionState::Skipped && target.executedTools.isEmpty() && target.tools.first().outputs.isEmpty(),"missing pose cache has no acquisition or absolute fallback");
        evaluation=session.evaluation(context);evaluation.cache[poseId].result.outputs["scale"].method="wrong";target=ExecutionService{}.runTool(definition,dir.path(),recipeId,{}, {},consumers[0],evaluation);check(target.state==ExecutionState::Error && target.tools.first().outputs.isEmpty() && target.executedTools.isEmpty(),"full cached producer metadata validated");
        auto blockedCache=[&](auto edit,ExecutionState expected){auto e=session.evaluation(context);edit(e.cache[poseId].result);const auto c=ExecutionService{}.runTool(definition,dir.path(),recipeId,{}, {},consumers[0],e);check(c.state==expected && c.executedTools.isEmpty() && c.tools.first().outputs.isEmpty(),"cache rejection has no metric or absolute fallback");};
        for(const QString port:{"pose","scale","found","ambiguous","truncated"}){
            blockedCache([&](ToolResult& r){r.outputs[port].evaluationId="other-evaluation";},ExecutionState::Error);
            blockedCache([&](ToolResult& r){r.outputs[port].origin.completedAtUtc=r.outputs[port].origin.completedAtUtc.addMSecs(1);},ExecutionState::Error);
        }
        blockedCache([](ToolResult& r){r.outputs["scale"].data=std::nextafter(1.,2.);},ExecutionState::Error);
        blockedCache([](ToolResult& r){r.outputs["ambiguous"].data=true;},ExecutionState::Skipped);
        blockedCache([](ToolResult& r){auto p=std::get<Pose2D>(r.outputs["pose"].data);p.position={0,0};r.outputs["pose"].data=p;},ExecutionState::Error);
        auto badSize=ImageFrame::fromBuffer(bytes.data(),128*256,128,256,128,PixelFormat::Gray8,current.metadata(),false);auto resized=run(definition,badSize);for(const auto& id:consumers)check(result(resized,id).outputs.isEmpty(),"actual resolution change cannot produce measurement");
        QByteArray repeated=moved;for(int y=0;y<24;++y)for(int x=0;x<24;++x)repeated[(46+y)*256+51+x]=bytes[(46+y)*256+51+x];const auto ambiguous=run(definition,frame(repeated,camera));check(std::get<bool>(result(ambiguous,poseId).outputs["ambiguous"].data),"real two-candidate ambiguity");for(const auto& id:consumers)check(result(ambiguous,id).state==ExecutionState::Skipped && result(ambiguous,id).outputs.isEmpty(),"actual ambiguity blocks consumers");
        auto changedTools=editorTools(definition);for(auto& t:changedTools)if(t.id==poseId)t.parameters["anchor"]=QJsonObject{{"x",10},{"y",14}};auto changed=withEditorTools(definition,changedTools);target=ExecutionService{}.runTool(changed,dir.path(),recipeId,{}, {},consumers[0],session.evaluation(context));check(target.state==ExecutionState::Skipped && target.executedTools.isEmpty(),"model edit invalidates upstream signature");
        auto stale=run(changed,current);for(const auto& id:consumers)check(result(stale,id).state==ExecutionState::Error && result(stale,id).outputs.isEmpty(),"stale teaching cannot produce metric");
        auto absent=run(definition,frame(QByteArray(256*256,char(30)),camera));for(const auto& id:consumers)check(result(absent,id).state==ExecutionState::Skipped && result(absent,id).outputs.isEmpty(),"reference not found skips every consumer");
        auto broken=links;broken.removeLast();check(!validateGraph(editorTools(definition),broken).valid(),"missing pose connection rejected");broken=links;broken.last().sourceTool=camera;check(!validateGraph(editorTools(definition),broken).valid(),"wrong pose producer rejected");broken=links;broken.append({consumers[0],"point",poseId,"image"});check(!validateGraph(editorTools(definition),broken).valid(),"cycle rejected");
        auto disabledTools=editorTools(definition);for(auto& t:disabledTools)if(t.id==poseId)t.enabled=false;const auto disabled=run(withEditorTools(definition,disabledTools),current);for(const auto& id:consumers)check(result(disabled,id).state==ExecutionState::Skipped && result(disabled,id).outputs.isEmpty(),"disabled producer has no fallback");
        auto unbound=editorTools(definition);for(auto& t:unbound)if(t.id==consumers[0])t.parameters.remove("poseTracking");check(!validateGraph(unbound,links).valid(),"pose connection without teaching rejected");
        session.invalidateTool(context,poseId);for(const auto& id:consumers)check(!session.evaluation(context).cache.contains(id),"producer rerun clears downstream cached measurements");
        auto stop=std::make_shared<std::atomic_bool>(true);auto stopped=ExecutionService{}.run(definition,dir.path(),recipeId,{},stop,{},current);check(stopped.state==ExecutionState::Skipped,"Stop");for(const auto& r:stopped.tools)check(r.outputs.isEmpty() && r.lines.isEmpty() && r.points.isEmpty(),"Stop removes tracked results");
        RuntimeService runtime(dir.filePath("archive"),project.path()+".calibrations.json",RuntimeIdentity::localEngineer());reject([&]{runtime.checkConfiguration(*project.activeRecipe(),dir.path());});
        check(QImage(reinterpret_cast<const uchar*>(moved.constData()),256,256,256,QImage::Format_Grayscale8).save(path),"save moved actual PNG");
        const auto acquired=ExecutionService{}.run(definition,dir.path(),recipeId,"cfg",{});done(acquired);check(acquired.acquisitions==1,"actual PNG acquisition executes tracked DAG");
        for(const auto& id:consumers)check(result(acquired,id).state==ExecutionState::Done && result(acquired,id).decision==PartDecision::NotEvaluated,"tracked tool result is separate from part decision");
        check(QImage(reinterpret_cast<const uchar*>(bytes.constData()),256,256,256,QImage::Format_Grayscale8).save(path),"restore teaching fixture");
    }
    QTextStream(stdout)<<checks<<" tracking DAG checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
