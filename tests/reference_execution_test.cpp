#include "application/PoseRoiService.hpp"
#include "application/ReferenceService.hpp"
#include "application/MatchService.hpp"
#include "application/BackendCapabilities.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "application/ProjectService.hpp"
#include "application/EditorSession.hpp"
#include "application/ToolExecutors.hpp"
#include "application/RuntimeService.hpp"
#include "reference_production_support.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QImage>
#include <QJsonDocument>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <numbers>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
template<class F>void reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
ToolResult result(const CycleResult& c,const QString& id){for(const auto& r:c.tools)if(r.toolId==id)return r;throw std::runtime_error("missing result");}
void done(const CycleResult& c){if(c.state!=ExecutionState::Done){QTextStream(stderr)<<c.error<<'\n';for(const auto& r:c.tools)QTextStream(stderr)<<r.toolId<<' '<<executionStateName(r.state)<<' '<<r.message<<'\n';}check(c.state==ExecutionState::Done,"DAG completes");}
QByteArray savedBytes(const QString& path){QFile file(path);check(file.open(QIODevice::ReadOnly),"Read persisted chain");return file.readAll();}
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

ToolDefinition node(const ProjectService& p,const QString& id){for(const auto& t:p.tools())if(t.id==id)return t;throw std::runtime_error("Missing node");}
QJsonObject rect(int x,int y,int w,int h){return {{"x",x},{"y",y},{"width",w},{"height",h}};}
QByteArray shift(const QByteArray& b,int dx,int dy){QByteArray out(256*256,char(30));for(int y=0;y<256;++y)for(int x=0;x<256;++x)if(x-dx>=0 && x-dx<256 && y-dy>=0 && y-dy<256)out[y*256+x]=b[(y-dy)*256+x-dx];return out;}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/reference-execution-XXXXXX");auto bytes=raster();
    for(int y=165;y<171;++y)for(int x=130;x<140;++x)if(!(x>=134 && x<136 && y>=167 && y<169))bytes[y*256+x]=char(230);
    const auto path=dir.filePath("scene.png");check(QImage(reinterpret_cast<const uchar*>(bytes.constData()),256,256,256,QImage::Format_Grayscale8).save(path),"PNG fixture");
    for(const QString producerKind:{"template_match","pattern_pose","shape_pose"})for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}) {
        const bool rigid=producerKind!="template_match";
        ProjectService p;p.createProject("References","",dir.filePath(producerKind+QString::number(int(type))+".vfproject"),"file");p.createRecipe("Part","",type,"file");
        const auto camera=p.setImageFile(path),sourceId=p.addTool(producerKind);const auto teaching=frame(bytes,camera);auto source=node(p,sourceId);
        if(rigid){source.parameters["angleMin"]=0.;source.parameters["angleMax"]=90.;source.parameters["angleStep"]=90.;source.parameters["pyramidLevels"]=1;source.parameters["minScore"]=.95;source.parameters=(producerKind=="shape_pose" ? ShapeMatchService::teach:PoseMatchService::teach)(teaching,{51,46,24,24},{9,14},source.parameters);}
        else {source.parameters["modelRoi"]=rect(51,46,24,24);source.parameters["modelPng"]=MatchService{}.teach(teaching,{51,46,24,24});source.parameters["minScore"]=.95;}
        p.updateTool(source);QStringList consumers;
        for(const QString kind:{"edge_point","edge_pair","line_fit","circle_fit","edge","blob"}) {
            const auto id=p.addTool(kind);consumers<<id;auto t=node(p,id);const CoordinateFrame raw{camera+"/image"};
            ToolRoi roi{raw,ScanSegment{{95,50},{95,70},2}};
            if(kind=="edge_pair")roi.geometry=ScanSegment{{95,85},{95,105},2};
            if(kind=="line_fit")roi.geometry=RotatedRectangle{{115,60},{24,20},0};
            if(kind=="circle_fit")roi.geometry=ArcRoi{{{95,125},5,15},10,-330};
            if(kind=="blob")t.parameters["searchRoi"]=rect(120,160,40,20);
            else if(kind=="edge"){t.parameters["searchRoi"]=rect(88,50,15,21);t.parameters["direction"]="top_to_bottom";}
            else {t.parameters["scanRoi"]=roi.toJson();t.parameters["polarity"]=kind=="edge_pair" ? "bright":"rising";}
            p.updateTool(t);
        }
        auto baseline=ExecutionService{}.run(p.activeRecipe()->definition,dir.path(),p.activeRecipe()->id,{}, {},{},teaching);done(baseline);
        const auto baselineImage=std::get<ImageFrame>(result(baseline,camera).outputs["image"].data);
        // Every existing Point producer uses its real port, never a geometry-to-point conversion.
        for(const auto& t:p.tools())for(const auto& port:outputPorts(t.kind))if(referenceSourcePort(t.kind,port.id)) {
            auto r=result(baseline,t.id);auto current=ReferenceService::current(t,port.id,r,baselineImage);if(!current.anchor)QTextStream(stderr)<<t.kind<<' '<<port.id<<' '<<current.reason<<'\n';check(current.anchor.has_value(),"Actual producer source matrix");
            if(port.type==PortType::Point)check(std::holds_alternative<Point2D>(*current.anchor),"Point stays Point");
            r.outputs[port.id].evaluationId="tampered";check(ReferenceService::current(t,port.id,r,baselineImage).state==ExecutionState::Error,"Mixed evaluation rejected");
        }
        const auto taughtResult=result(baseline,sourceId);
        for(const auto& id:consumers) {
            const auto t=ReferenceService::teach(node(p,id),source,rigid ? "pose":"point",taughtResult,baselineImage,{5,-2});
            p.bindReference(id,toolReferenceBinding(t));
            reject([&]{ToolExecutors::get(t.kind)(t,{},dir.path(),"frame","cycle",{});});
            auto edited=t;auto b=toolReferenceBinding(t);b.offsetPx={1,2};edited.parameters["referenceBinding"]=b.toJson();ToolRegistry::validate(edited);
        }
        // Source follows only its measured transform. Feature pixels include additional offset.
        const auto moved=rigid ? rotate(bytes):shift(bytes,20,10);const int ox=rigid ? 2:5,oy=rigid ? 5:-2;
        auto offsetScene=shift(moved,ox,oy);
        const QRect model=rigid ? QRect(131,111,24,24):QRect(71,56,24,24);
        for(int y=model.top()+oy;y<=model.bottom()+oy;++y)for(int x=model.left()+ox;x<=model.right()+ox;++x)offsetScene[y*256+x]=char(30);
        for(int y=model.top();y<=model.bottom();++y)for(int x=model.left();x<=model.right();++x)offsetScene[y*256+x]=moved[y*256+x];
        const auto current=frame(offsetScene,camera);const auto definition=p.activeRecipe()->definition;const auto recipe=p.activeRecipe()->id;
        EditorContext context{p.project().id,p.path(),recipe,definition,{}};EditorSession session;
        const auto cycle=ExecutionService{}.run(definition,dir.path(),recipe,session.signature(context),{}, {},current);done(cycle);check(session.acceptCycle(cycle,session.signature(context),context),"Editor accepts actual results");
        const auto edge=result(cycle,consumers[0]);const QPointF expected=rigid ? QPointF(142,160):QPointF(120,68);
        check(edge.outputs["point"].valid && QLineF(std::get<QPointF>(edge.outputs["point"].data),expected).length()<1.5,"Offset actual edge independent oracle");
        const auto pair=result(cycle,consumers[1]);check(std::abs(std::get<double>(pair.outputs["widthPx"].data)-10)<.3,"Offset caliper actual width");
        const auto line=std::get<Line2D>(result(cycle,consumers[2]).outputs["linePx"].data);check(rigid ? std::abs(line.end.x()-line.start.x())<.3:std::abs(line.end.y()-line.start.y())<.3,"Offset line refit");
        const auto centre=std::get<QPointF>(result(cycle,consumers[3]).outputs["centrePx"].data);check(QLineF(centre,rigid ? QPointF(77,160):QPointF(120,133)).length()<1.5,"Negative arc centre oracle");
        const auto legacy=result(cycle,consumers[4]);check(legacy.outputs["point"].method=="reference-local-difference" && QLineF(std::get<QPointF>(legacy.outputs["point"].data),expected).length()<2,"Edge local direction actual raster");
        const auto blob=result(cycle,consumers[5]);check(std::get<double>(blob.outputs["area"].data)==56,"Blob offset mask preserves hole");
        check(QLineF(std::get<QPointF>(blob.outputs["point"].data),rigid ? QPointF(34.5,199.5):QPointF(159.5,175.5)).length()<1e-6,"Blob centroid oracle");
        if(rigid){
            auto angledTools=editorTools(definition);for(auto& t:angledTools)if(t.id==consumers[0]){auto b=toolReferenceBinding(t);b.angleOffsetRadians=std::numbers::pi/2;t.parameters["referenceBinding"]=b.toJson();}
            QByteArray angled(256*256,char(30));for(int y=0;y<256;++y)for(int x=0;x<256;++x){const int sx=202-x,sy=185-y;if(sx>=0 && sx<256 && sy>=0 && sy<256)angled[y*256+x]=bytes[sy*256+sx];}
            for(int y=model.top();y<=model.bottom();++y)for(int x=model.left();x<=model.right();++x)angled[y*256+x]=moved[y*256+x];
            const auto angleCycle=ExecutionService{}.run(withEditorTools(definition,angledTools),dir.path(),recipe,{}, {},consumers[0],frame(angled,camera));done(angleCycle);
            check(QLineF(std::get<QPointF>(result(angleCycle,consumers[0]).outputs["point"].data),{107,125}).length()<1.5,"Actual angle offset rotates ROI before local XY offset");
            auto chainTools=editorTools(definition);auto links=editorConnections(definition);
            const auto circleSource=node(p,consumers[3]);auto chainBinding=toolReferenceBinding(node(p,consumers[0]));
            chainBinding.mode=ReferenceMode::Translation;chainBinding.sourceTool=circleSource.id;chainBinding.sourcePort="centrePx";
            chainBinding.geometryId=referenceGeometryIdentity(circleSource,"centrePx");chainBinding.taught=Point2D{{95,125},{camera+"/image"}};chainBinding.offsetPx={};
            for(auto& t:chainTools)if(t.id==consumers[0])t.parameters["referenceBinding"]=chainBinding.toJson();
            for(auto& c:links)if(c.targetTool==consumers[0] && c.targetPort=="poseReference"){c.sourceTool=circleSource.id;c.sourcePort="centrePx";c.targetPort="reference";}
            auto chainBytes=offsetScene;for(int y=84;y<=106;++y)for(int x=72;x<=82;++x)chainBytes[y*256+x]=char(std::lround(128+100*std::erf((y-95)/1.4)));
            const auto chainDefinition=withEditorConnections(withEditorTools(definition,chainTools),links);
            const auto chain=ExecutionService{}.run(chainDefinition,dir.path(),recipe,{}, {},{},frame(chainBytes,camera));done(chain);
            check(QLineF(std::get<QPointF>(result(chain,consumers[0]).outputs["point"].data),{77,95}).length()<1.5,"Pattern/Shape -> Circle -> Point translation does not inherit angle");
            // Persist the actual chained graph, not just its component bindings.
            for(const auto& t:chainTools)if(t.id==consumers[0])p.updateToolConnections(t,editorConnections(chainDefinition));
            p.save();const auto chainBytesOnDisk=savedBytes(p.path());const auto revision=p.activeRecipe()->recipeVersion;
            p.openProject(p.path());p.selectRecipe(recipe);p.save();
            check(savedBytes(p.path())==chainBytesOnDisk && p.activeRecipe()->recipeVersion==revision && !p.dirty(),"Chained reference save/reopen/no-op is byte stable");
            const auto reopened=ExecutionService{}.run(p.activeRecipe()->definition,dir.path(),recipe,{}, {},{},frame(chainBytes,camera));done(reopened);
            check(QLineF(std::get<QPointF>(result(reopened,consumers[0]).outputs["point"].data),{77,95}).length()<1.5,"Reopened chain repeats independent actual-raster oracle");
            EditorContext chainContext{p.project().id,p.path(),recipe,p.activeRecipe()->definition,{}};EditorSession chainSession;
            const auto cachedChain=ExecutionService{}.run(chainContext.definition,dir.path(),recipe,chainSession.signature(chainContext),{}, {},frame(chainBytes,camera));done(cachedChain);
            check(chainSession.acceptCycle(cachedChain,chainSession.signature(chainContext),chainContext),"Chained current results cached");
            auto thresholdTools=editorTools(chainContext.definition);
            for(auto& t:thresholdTools)if(t.id==circleSource.id)t.parameters["minContrast"]=t.parameters["minContrast"].toDouble()+1.;
            chainContext.definition=withEditorTools(chainContext.definition,thresholdTools);
            const auto invalidated=chainSession.evaluation(chainContext);
            check(invalidated.cache.contains(sourceId) && !invalidated.cache.contains(circleSource.id) && !invalidated.cache.contains(consumers[0]),"Circle threshold invalidates downstream Edge but preserves matcher");
            const auto blocked=ExecutionService{}.runTool(chainContext.definition,dir.path(),recipe,{}, {},consumers[0],invalidated);
            check(blocked.state==ExecutionState::Skipped && blocked.acquisitions==0 && blocked.executedTools.isEmpty(),"Chained target never silently refreshes stale upstream");
            const auto refreshedCircle=ExecutionService{}.runTool(chainContext.definition,dir.path(),recipe,chainSession.signature(chainContext),{},circleSource.id,invalidated);done(refreshedCircle);
            check(refreshedCircle.acquisitions==0 && refreshedCircle.executedTools==QStringList{circleSource.id} && chainSession.acceptTool(refreshedCircle,chainSession.signature(chainContext),chainContext),"Explicit Circle refresh runs only upstream target");
            const auto refreshedEdge=ExecutionService{}.runTool(chainContext.definition,dir.path(),recipe,{}, {},consumers[0],chainSession.evaluation(chainContext));done(refreshedEdge);
            check(refreshedEdge.acquisitions==0 && refreshedEdge.executedTools==QStringList{consumers[0]} && QLineF(std::get<QPointF>(result(refreshedEdge,consumers[0]).outputs["point"].data),{77,95}).length()<1.5,"Explicit upstream refresh restores same Edge oracle without reteaching");
            for(const auto& t:editorTools(definition))if(t.id==consumers[0])p.updateToolConnections(t,editorConnections(definition));
            links.append({consumers[0],"point",circleSource.id,"reference"});check(!validateGraph(chainTools,links).valid(),"Downstream cycle rejected");
        }
        for(const auto& id:consumers) {
            auto target=ExecutionService{}.runTool(definition,dir.path(),recipe,{}, {},id,session.evaluation(context));done(target);check(target.acquisitions==0 && target.executedTools==QStringList{id},"TargetOnly does not acquire or run upstream");
            auto e=session.evaluation(context);e.cache[sourceId].result.outputs[rigid ? "pose":"point"].method="wrong";target=ExecutionService{}.runTool(definition,dir.path(),recipe,{}, {},id,e);check(target.state==ExecutionState::Error && target.tools.first().outputs.isEmpty(),"Malformed cached reference no fallback");
            e=session.evaluation(context);e.cache.remove(sourceId);target=ExecutionService{}.runTool(definition,dir.path(),recipe,{}, {},id,e);check(target.state==ExecutionState::Skipped && target.executedTools.isEmpty(),"Missing current source skipped");
            e=session.evaluation(context);e.cache[sourceId].result.outputs["found"].data=false;target=ExecutionService{}.runTool(definition,dir.path(),recipe,{}, {},id,e);check(target.state==ExecutionState::Skipped && target.tools.first().outputs.isEmpty(),"Not-found source skips without nominal fallback");
            e=session.evaluation(context);e.cache[sourceId].result.outputs[rigid ? "pose":"point"].origin.completedAtUtc=current.metadata().completedAtUtc.addMSecs(1);target=ExecutionService{}.runTool(definition,dir.path(),recipe,{}, {},id,e);check(target.state==ExecutionState::Error && target.tools.first().outputs.isEmpty(),"Acquisition timestamp mismatch rejected");
            auto taught=session.taughtReference(context,node(p,id),sourceId,rigid ? "pose":"point",{5,-2});const auto b=toolReferenceBinding(taught);check(b.nominal.toJson()==toolReferenceBinding(node(p,id)).nominal.toJson() && b.offsetPx==QPointF(5,-2),"Reteach unoffset nominal, offset once");
        }
        auto edited=editorTools(definition);for(auto& t:edited)if(t.id==consumers[0]){auto b=toolReferenceBinding(t);b.offsetPx={6,-2};t.parameters["referenceBinding"]=b.toJson();}
        auto changed=context;changed.definition=withEditorTools(definition,edited);const auto cache=session.evaluation(changed);check(cache.cache.contains(sourceId) && !cache.cache.contains(consumers[0]),"Offset edit preserves upstream invalidates target");
        auto token=std::make_shared<std::atomic_bool>(true);const auto stopped=ExecutionService{}.runTool(definition,dir.path(),recipe,{},token,consumers[0],session.evaluation(context));check(stopped.state==ExecutionState::Skipped && stopped.tools.first().outputs.isEmpty(),"Stop no stale point");
        session.invalidateTool(context,sourceId);reject([&]{session.taughtReference(context,node(p,consumers[0]),sourceId,rigid ? "pose":"point");});
        p.save();
        check(QImage(reinterpret_cast<const uchar*>(offsetScene.constData()),256,256,256,QImage::Format_Grayscale8).save(path),"Actual Runtime scene");
        const auto production=productionAcceptance(*p.activeRecipe(),dir.path(),dir.filePath(producerKind+QString::number(int(type))+"-archive"),p.path()+".calibrations.json");
        check(QLineF(std::get<QPointF>(result(production,consumers[0]).outputs["point"].data),expected).length()<1.5,"Immutable Runtime same independent Edge oracle");
        // Independently convert the unoffset pixel fixture to a calibrated Y-up
        // plane, then use the catalog-backed Runtime resolver (no test resolver).
        Recipe worldRecipe=*p.activeRecipe();worldRecipe.schemaVersion=2;
        worldRecipe.definition["planarSetup"]=QJsonObject{{"lensId","lens"},{"workingPlaneId","plane"}};
        const CoordinateFrame raw{camera+"/image"},world{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYUp};
        CalibrationDependency setup{"production-cal",1,"pending",camera,"lens",{256,256},"plane",raw,world};
        auto sample=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{a,raw},{b,raw},QLineF(a,b).length()*.1};};
        const auto cal=MeasurementService{}.validate(MeasurementService{}.teach(setup,sample("teach",{10,10},{110,10})),
            {sample("hx",{20,20},{120,20}),sample("hy",{30,30},{30,130})},1e-6);
        const QString catalog=p.path()+".calibrations.json";CalibrationService(catalog).save(cal);
        worldRecipe.calibrationReference=cal.dependency().calibrationId;worldRecipe.calibrationBinding=cal.dependency();
        const MeasurementExecutionContext mm{std::make_shared<PlanarCalibration>(cal),cal.dependency(),{256,256},{}};
        auto worldTools=editorTools(worldRecipe.definition);
        auto q=[](QPointF v){return QPointF(v.x()*.1,-v.y()*.1);};
        for(auto& t:worldTools)if(t.kind=="edge_point" || t.kind=="edge_pair" || t.kind=="line_fit" || t.kind=="circle_fit") {
            const auto old=toolReferenceBinding(t);auto roi=old.nominal;roi.frame=world;
            if(const auto* g=std::get_if<ScanSegment>(&old.nominal.geometry))roi.geometry=ScanSegment{q(g->start),q(g->end),g->width*.1};
            else if(const auto* g=std::get_if<RotatedRectangle>(&old.nominal.geometry))roi.geometry=RotatedRectangle{q(g->centre),g->size*.1,-g->angleDegrees};
            else if(const auto* g=std::get_if<ArcRoi>(&old.nominal.geometry))roi.geometry=ArcRoi{{q(g->ring.centre),g->ring.innerRadius*.1,g->ring.outerRadius*.1},-g->startDegrees,-g->sweepDegrees};
            else if(const auto* g=std::get_if<Annulus>(&old.nominal.geometry))roi.geometry=Annulus{q(g->centre),g->innerRadius*.1,g->outerRadius*.1};
            if(t.kind=="line_fit")t.parameters["polarity"]="falling";
            if(t.kind=="circle_fit"){t.parameters["scanWidth"]=.1;t.parameters["inlierDistance"]=.2;t.parameters["maxRms"]=.1;t.parameters["maxResidual"]=.2;}
            t=ReferenceService::teachWorld(t,roi,.1,source,rigid ? "pose":"point",taughtResult,baselineImage,mm,old.offsetPx);
        }
        worldRecipe.definition=withEditorTools(worldRecipe.definition,worldTools);
        const auto worldProduction=productionAcceptance(worldRecipe,dir.path(),dir.filePath(producerKind+QString::number(int(type))+"-world-archive"),catalog);
        check(std::abs(std::get<double>(result(worldProduction,consumers[1]).outputs["widthMm"].data)-1)<.03,"Runtime calibrated width independent oracle");
        check(QLineF(std::get<QPointF>(result(worldProduction,consumers[3]).outputs["centreMm"].data),rigid ? QPointF(7.7,-16):QPointF(12,-13.3)).length()<.15,"Runtime current world centre independent oracle");
        check(QImage(reinterpret_cast<const uchar*>(bytes.constData()),256,256,256,QImage::Format_Grayscale8).save(path),"Restore teaching scene");
    }
    for(const QString direction:{"left_to_right","right_to_left","top_to_bottom","bottom_to_top"}) {
        const bool horizontal=direction=="left_to_right" || direction=="right_to_left";const double a=37*std::numbers::pi/180.;const QPointF axis=horizontal ? QPointF(std::cos(a),std::sin(a)):QPointF(-std::sin(a),std::cos(a));
        QByteArray pixels(256*256,char(30));for(int y=0;y<256;++y)for(int x=0;x<256;++x)pixels[y*256+x]=char((x-128)*axis.x()+(y-128)*axis.y()>=0 ? 240:20);
        const auto image=frame(pixels,"camera");ToolDefinition t{"edge","edge","Edge","",true,defaultParameters("edge")};t.parameters["direction"]=direction;
        ToolRoi roi{{"camera/image"},RotatedRectangle{{128,128},{40,20},37}};
        const auto r=ToolExecutors::get("edge",{},roi)(t,{{"image",{{"image",PortType::Image,"px","image"},image,true,"frame","cycle",image.metadata()}}},dir.path(),"frame","cycle",{});
        check(r.outputs["point"].valid && QLineF(std::get<QPointF>(r.outputs["point"].data),{128,128}).length()<1.5,"Oblique Edge scans all four local directions");
    }
    QTextStream(stdout)<<checks<<" common reference execution checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
