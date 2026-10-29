#include "application/EditorSession.hpp"
#include "application/ProductionResultCodec.hpp"
#include "application/RoiService.hpp"
#include "application/RuntimeOverlay.hpp"
#include "application/RuntimeService.hpp"
#include "application/ToolResultCodec.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUuid>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,why);}
const ToolResult& result(const CycleResult& c,const QString& id){for(const auto& t:c.tools)if(t.toolId==id)return t;throw std::runtime_error("missing result");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 for(auto type:{ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement})for(bool crop:{false,true}){
    QTemporaryDir dir(QDir::currentPath()+"/efr-acceptance-XXXXXX");check(dir.isValid(),"fixture directory");
    QImage pixels(160,120,QImage::Format_Grayscale8);pixels.fill(0);for(int y=50;y<70;++y)for(int x=60;x<80;++x)pixels.scanLine(y)[x]=255;
    const auto path=dir.filePath("part.png");check(pixels.save(path),"real raster fixture");
    ProjectService project;project.createProject("EFR","",dir.filePath("part.vfproject"),"file");const auto recipeId=project.createRecipe("Common crop","",type,"file");
    const auto sourceId=project.setImageFile(path);auto source=project.tools().first();
    auto initial=ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),recipeId,"fixture",{});
    const auto raw=std::get<ImageFrame>(result(initial,sourceId).outputs["image"].data);
    auto add=[&](const QString& kind){const auto id=project.addTool(kind);for(const auto& t:project.tools())if(t.id==id)return t;throw std::runtime_error("missing definition");};
    auto outer=add("roi"),inner=add("roi"),childBlob=add("blob"),rawBlob=add("blob");
    outer.parameters=RoiService::teach(raw,{20,10,120,100},crop);
    const auto parent=crop ? ImageAccess::cropped(raw,{20,10,120,100},false):raw;
    inner.parameters=RoiService::teach(parent,crop ? QRect(10,20,80,70):QRect(30,30,80,70),true);
    QVector<ToolDefinition> tools{source,outer,inner,childBlob,rawBlob};
    QVector<ToolConnection> links{{sourceId,"image",outer.id,"image"},{outer.id,"image",inner.id,"image"},{inner.id,"image",childBlob.id,"image"},{sourceId,"image",rawBlob.id,"image"}};
    project.setConnections(links);project.updateTool(outer);project.updateTool(inner);project.save();const auto saved=*project.activeRecipe();
    project.openProject(project.path());project.selectRecipe(recipeId);check(project.activeRecipe()->definition==saved.definition && project.activeRecipe()->applicationType==type,"saved recipe type and crop roundtrip");
    EditorSession editor;const EditorContext context{project.project().id,project.path(),recipeId,saved.definition,{}};
    auto full=ExecutionService{}.run(context.definition,dir.path(),recipeId,editor.signature(context),{});
    check(full.state==ExecutionState::Done && full.acquisitions==1 && full.tools.size()==5,"common Editor Run Flow");
    check(editor.acceptCycle(full,editor.signature(context),context),"Editor accepts crop flow");
    const auto& local=result(full,childBlob.id).outputs["point"];
    check(std::get<QPointF>(local.data)==QPointF(39.5,29.5) && std::get<double>(result(full,childBlob.id).outputs["area"].data)==400,"actual crop local centroid and area");
    check(std::get<QPointF>(result(full,rawBlob.id).outputs["point"].data)==QPointF(69.5,59.5),"raw sibling coordinates stay raw");
    check(local.imageGeometry && local.imageGeometry->rootSize()==pixels.size() && local.imageGeometry->rootOffset()==QPoint(30,30) && local.imageGeometry->cropDepth()==(crop ? 2:1),"crop chain geometry");
    auto partial=ExecutionService{}.runTool(context.definition,dir.path(),recipeId,editor.signature(context),{},childBlob.id,editor.evaluation(context));
    check(partial.state==ExecutionState::Done && partial.partial && partial.acquisitions==0 && partial.executedTools==QStringList{childBlob.id},"common Editor Run Tool uses current child without acquisition");
    check(std::get<QPointF>(result(partial,childBlob.id).outputs["point"].data)==std::get<QPointF>(local.data) && editor.acceptTool(partial,editor.signature(context),context),"Run Tool preserves local result and scope");
    const auto archive=dir.filePath("archive"),catalog=project.path()+".calibrations.json";
    RuntimeService runtime(archive,catalog,RuntimeIdentity::localEngineer());runtime.prepare(saved,dir.path());check(runtime.records().isEmpty(),"Run preparation has no result");const auto lease=runtime.begin();
    auto draft=outer;draft.parameters["cropImage"]=!crop;draft.parameters["searchRoi"]=MatchService::rectangle({25,15,120,100});project.updateTool(draft);project.save();
    const auto completed=RuntimeService::execute(lease,catalog,{});check(completed.state==ExecutionState::Done,"saved immutable Runtime flow");
    const auto& point=result(completed,childBlob.id).outputs["point"];
    check(point.imageGeometry==local.imageGeometry && std::get<QPointF>(point.data)==std::get<QPointF>(local.data) && runtime.active()->recipe().definition==saved.definition,"draft rect and mode edits cannot mutate snapshot");
    for(const auto& t:completed.tools)check(t.decision==PartDecision::NotEvaluated,"no invented part PASS or robot pose");
    runtime.finish(lease.ticket,completed);const auto record=runtime.records().first();ProductionResultCodec::validate(record);
    RuntimeService restarted(archive,catalog,RuntimeIdentity::localEngineer());check(!restarted.active() && restarted.records().first()==record,"restart replays evidence without activation");
    QJsonObject childImage,childPoint,legacyRoot;
    for(const auto& v:record["tools"].toArray()){const auto t=v.toObject(),outputs=t["outputs"].toObject();if(t["toolId"]==inner.id)childImage=outputs["image"].toObject();if(t["toolId"]==childBlob.id)childPoint=outputs["point"].toObject();if(t["toolId"]==rawBlob.id)legacyRoot=outputs["point"].toObject()["typedPort"].toObject();}
    const auto geometry=ImageGeometry::fromJson(childImage["imageInfo"].toObject()["imageGeometry"].toObject());
    const auto decoded=ToolResultCodec::decode(childPoint["typedPort"].toObject());
    check(decoded.imageGeometry==point.imageGeometry && geometry==*point.imageGeometry && geometry.toJson()["crops"].toArray().size()==geometry.cropDepth(),"terminal raw size local IDs rect chain and transform roundtrip");
    check(decoded.origin.sourceId==sourceId && decoded.frameId==completed.frameId && decoded.cycleId==completed.cycleId && decoded.evaluationId==completed.evaluationId && geometry.rootCoordinateSystemId()==sourceId+"/image" && geometry.coordinateSystemId()==decoded.origin.coordinateSystemId,"terminal acquisition frame cycle and coordinate identities");
    check(geometry.toRoot(std::get<QPointF>(decoded.data))==QPointF(69.5,59.5),"historical child point explicitly maps to raw");
    check(legacyRoot["schemaVersion"]==1 && !legacyRoot.contains("imageGeometry") && !ToolResultCodec::decode(legacyRoot).imageGeometry,"old root format is not backfilled with crop scope");
    auto tampered=childPoint["typedPort"].toObject();tampered.remove("imageGeometry");tampered["schemaVersion"]=1;reject([&]{ToolResultCodec::decode(tampered);},"child cannot replay without verified scope");
    auto bad=geometry.toJson();bad["offsetX"]=31;reject([&]{ImageGeometry::fromJson(bad);},"historical transform tamper rejected");
    check(ProductionResultCodec::summary(record).contains("Görüntü mevcut değil") && ProductionResultCodec::csv({record}).contains(geometry.coordinateSystemId().toUtf8()),"Logs unavailable raster and CSV crop evidence");
    check(!childImage["imageInfo"].toObject().contains("pixels") && !childImage["imageInfo"].toObject().contains("data"),"terminal stores metadata only");
    const auto current=ExecutionService{}.run(saved.definition,dir.path(),recipeId,"current",{});const auto currentRaw=std::get<ImageFrame>(result(current,sourceId).outputs["image"].data);
    const auto graph=validateGraph(tools,links);
    check(runtimeOverlays(completed,tools,graph,currentRaw,runtime.active(),childBlob.id).layers.isEmpty(),"current raster cannot back historical result");
    check(runtimeOverlays(completed,tools,graph,raw,restarted.active(),childBlob.id).layers.isEmpty(),"replayed result without active snapshot cannot draw");
    check(!restarted.active() && restarted.records().size()==1,"viewing evidence creates no acquisition result or permit");
 }
 QTextStream(stdout)<<checks<<" combined EFR checks passed (3 application types, 2 crop modes)\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
