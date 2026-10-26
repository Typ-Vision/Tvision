#include "application/GeometryMeasurementService.hpp"
#include "application/MeasurementService.hpp"
#include "application/ToolResultCodec.hpp"
#include "application/EditorSession.hpp"
#include "application/CalibrationWorkflow.hpp"
#include "application/RuntimeService.hpp"
#include "application/ProductionResultCodec.hpp"
#include "domain/GeometryMeasurementConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "domain/ToolRoi.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTextStream>
#include <QImage>
#include <cmath>
#include <numbers>
#include <limits>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double tolerance=1e-6){check(std::isfinite(a) && std::abs(a-b)<=tolerance,"independent numeric oracle");}
template<class F>void reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected geometry rejection");}
template<class F>void rejectWith(F f,const char* reason){bool rejected=false;try{f();}catch(const std::exception& e){rejected=QString::fromUtf8(e.what()).contains(reason);}check(rejected,"expected specific rejection reason");}
const ToolResult& result(const CycleResult& c,const QString& id){for(const auto& t:c.tools)if(t.toolId==id)return t;throw std::runtime_error("result missing");}
double number(const ToolResult& r,const QString& id){check(r.outputs.value(id).valid,"expected valid metric");return std::get<double>(r.outputs[id].data);}
struct Fixture {
    FrameMetadata m{"camera","frame","cycle","camera/image",QDateTime::currentDateTimeUtc()};
    CoordinateFrame raw{m.coordinateSystemId},world{"part/plane",LengthUnit::Millimetre,AxisConvention::XRightYDown};
    ImageFrame image;
    Fixture(){QByteArray bytes(256*256,char(128));image=ImageFrame::fromBuffer(bytes.data(),bytes.size(),256,256,256,PixelFormat::Gray8,m,false);}
    PortValue port(QString id,PortData data)const {
        for(const auto& d:geometryMeasurementInputs())if(d.id==id){PortValue v{d,std::move(data),true,m.frameId,m.cycleId,m};v.evaluationId="feature-evaluation";v.method="analytic-feature";v.methodVersion=1;return v;}
        throw std::runtime_error("unknown fixture port");
    }
    CalibrationDependency setup()const{return {"scale",1,"pending",m.sourceId,"lens",image.size(),"plane",raw,world};}
    PlanarCalibration uniform(bool manual=false,AxisConvention axes=AxisConvention::XRightYDown)const {
        auto d=setup();d.target.axes=axes;
        auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,raw},{b,raw},mm};};
        MeasurementService service;
        auto c=manual ? service.manualScale(d,10,{"acquired",m.sourceId,m.frameId,image.size()}):service.teach(d,sample("teach",{0,0},{100,0},10));
        return service.validate(c,{sample("hx",{20,20},{120,20},10),sample("hy",{30,30},{30,130},10)},.00001);
    }
    MeasurementExecutionContext context(const PlanarCalibration& c)const{return {std::make_shared<PlanarCalibration>(c),c.dependency(),image.size(),{}};}
};
ToolDefinition tool(QString op,QString mode="calibrated"){ToolDefinition t{"geometry","geometry_measurement","Geometry","",true,geometryMeasurementDefaults()};t.parameters["operation"]=op;t.parameters["mode"]=mode;return t;}
QVector<QPointF> linePoints(QPointF a,QPointF b){QVector<QPointF> p;for(int i=0;i<21;++i)p.append(a+(b-a)*(double(i)/20));return p;}
QVector<QPointF> circlePoints(QPointF c,double r){QVector<QPointF> p;for(int i=0;i<72;++i){const double a=2*std::numbers::pi*i/72;p.append(c+QPointF(r*std::cos(a),r*std::sin(a)));}return p;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    Fixture f;const auto initialHash=f.image.contentId();const auto c=f.uniform();auto ctx=f.context(c);
    auto execute=[&](const ToolDefinition& t,const QMap<QString,PortValue>& values,const MeasurementExecutionContext& context){return GeometryMeasurementService::execute(t,values,f.m.frameId,f.m.cycleId,{},context);};
    auto base=[&]{return QMap<QString,PortValue>{{"image",f.port("image",f.image)}};};
    auto points=base();points["p1"]=f.port("p1",QPointF(20,30));points["p2"]=f.port("p2",QPointF(50,70));
    {MeasurementExecutionContext missing;missing.error="fixture: remembered model permission missing";bool propagated=false;
    try{execute(tool("point_distance"),points,missing);}catch(const std::exception& e){const auto message=QString::fromUtf8(e.what());propagated=message.contains(missing.error) && message.contains("Kaynağı doğrula") && message.contains("Setup");}check(propagated,"Geometry preserves actual resolver cause and recovery action");}
    for(auto mode:{QString("pixel"),QString("calibrated")})for(auto op:{QString("point_distance"),QString("x_offset"),QString("y_offset")}){
        auto t=tool(op,mode);t.parameters["offset"]="signed";const auto r=execute(t,points,ctx);const double expected=op=="point_distance" ? 50:op=="x_offset" ? 30:40;
        near(number(r,mode=="pixel" ? "valuePx":"valueMm"),expected*(mode=="pixel" ? 1:.1));
        check(r.decision==PartDecision::NotEvaluated && r.calibration.has_value()==(mode=="calibrated"),"no invented part decision / correct binding");
        check(!r.outputs.value(mode=="pixel" ? "valueMm":"valuePx").valid,"no other plane fallback");
    }
    for(bool manual:{false,true}){
        auto up=f.context(f.uniform(manual,AxisConvention::XRightYUp));auto t=tool("y_offset");t.parameters["offset"]="signed";
        near(number(execute(t,points,up),"valueMm"),-4);t.parameters["offset"]="absolute";near(number(execute(t,points,up),"valueMm"),4);
    }
    auto lines=base();lines["line1"]=f.port("line1",Line2D{{20,100},{220,100},f.raw});lines["line2"]=f.port("line2",Line2D{{120,20},{120,220},f.raw});
    lines["samples1"]=f.port("samples1",PointSet{linePoints({20,100},{220,100}),f.raw});lines["samples2"]=f.port("samples2",PointSet{linePoints({120,20},{120,220}),f.raw});
    auto angle=tool("line_angle");angle.parameters["angle"]="signed_directed";near(number(execute(angle,lines,ctx),"angleMm"),90,.0001);
    auto hit=execute(tool("line_intersection"),lines,ctx);check(hit.outputs["intersectionMm"].valid,"world intersection valid");
    auto q=std::get<QPointF>(hit.outputs["intersectionMm"].data);near(q.x(),12,.0001);near(q.y(),10,.0001);
    auto parallel=lines;parallel["line2"]=f.port("line2",Line2D{{30,140},{210,140},f.raw});parallel["samples2"]=f.port("samples2",PointSet{linePoints({30,140},{210,140}),f.raw});
    near(number(execute(tool("parallel_distance"),parallel,ctx),"valueMm"),4,.0001);
    auto unavailable=execute(tool("line_intersection"),parallel,ctx);check(unavailable.state==ExecutionState::Done && !unavailable.outputs["intersectionMm"].valid && std::get<QString>(unavailable.outputs["status"].data)=="PARALLEL","parallel explicit unavailable");
    parallel["line2"]=lines["line1"];parallel["samples2"]=lines["samples1"];
    unavailable=execute(tool("line_intersection"),parallel,ctx);check(std::get<QString>(unavailable.outputs["status"].data)=="COINCIDENT" && !unavailable.outputs["intersectionMm"].valid,"coincident no fake point");
    auto pl=base();pl["p1"]=f.port("p1",QPointF(80,140));pl["line1"]=lines["line1"];pl["samples1"]=lines["samples1"];
    near(number(execute(tool("point_line_distance"),pl,ctx),"valueMm"),4,.0001);near(number(execute(tool("point_segment_distance"),pl,ctx),"valueMm"),4,.0001);
    pl["p1"]=f.port("p1",QPointF(250,140));reject([&]{execute(tool("point_line_distance"),pl,ctx);});near(number(execute(tool("point_segment_distance"),pl,ctx),"valueMm"),5,.0001);
    auto beyond=lines;beyond["line2"]=f.port("line2",Line2D{{240,20},{240,220},f.raw});beyond["samples2"]=f.port("samples2",PointSet{linePoints({240,20},{240,220}),f.raw});reject([&]{execute(tool("line_intersection"),beyond,ctx);});
    auto circles=base();circles["circle1"]=f.port("circle1",Circle2D{{80,80},20,f.raw});circles["circle2"]=f.port("circle2",Circle2D{{110,120},15,f.raw});
    circles["samples1"]=f.port("samples1",PointSet{circlePoints({80,80},20),f.raw});circles["samples2"]=f.port("samples2",PointSet{circlePoints({110,120},15),f.raw});
    near(number(execute(tool("circle_centre_distance"),circles,ctx),"valueMm"),5);
    auto one=circles;one.remove("circle2");one.remove("samples2");near(number(execute(tool("circle_radius"),one,ctx),"valueMm"),2);near(number(execute(tool("circle_diameter"),one,ctx),"valueMm"),4);
    auto pixelLines=lines;pixelLines.remove("samples1");pixelLines.remove("samples2");
    near(number(execute(tool("line_angle","pixel"),pixelLines,{}),"anglePx"),90);
    check(execute(tool("line_intersection","pixel"),pixelLines,{}).outputs["intersectionPx"].valid,"pixel intersection");
    auto pixelParallel=parallel;pixelParallel.remove("samples1");pixelParallel.remove("samples2");near(number(execute(tool("parallel_distance","pixel"),pixelParallel,{}),"valuePx"),0);
    auto pixelCircle=one;pixelCircle.remove("samples1");near(number(execute(tool("circle_radius","pixel"),pixelCircle,{}),"valuePx"),20);near(number(execute(tool("circle_diameter","pixel"),pixelCircle,{}),"valuePx"),40);
    // Full source samples remain intact while robust fitting rejects a defect point.
    auto damaged=one;auto set=std::get<PointSet>(damaged["samples1"].data);set.points[3]+=(set.points[3]-QPointF(80,80))*.4;damaged["samples1"].data=set;
    near(number(execute(tool("circle_diameter"),damaged,ctx),"valueMm"),4,.0001);
    check(f.image.contentId()==initialHash,"pure adapter preserves raw frame");
    for(const auto& input:{QString("p1"),QString("p2")})for(auto fault:{QString("frame"),QString("cycle"),QString("source"),QString("coordinates"),QString("time"),QString("unit"),QString("type"),QString("invalid"),QString("nonfinite")}){
        auto bad=points;auto& v=bad[input];if(fault=="frame")v.frameId="foreign";if(fault=="cycle")v.cycleId="foreign";if(fault=="source")v.origin.sourceId="foreign";if(fault=="coordinates")v.origin.coordinateSystemId="foreign";
        if(fault=="time")v.origin.completedAtUtc=f.m.completedAtUtc.addMSecs(1);if(fault=="unit")v.port.unit="mm";if(fault=="type")v.data=true;if(fault=="invalid")v.valid=false;if(fault=="nonfinite")v.data=QPointF(std::numeric_limits<double>::quiet_NaN(),4);
        reject([&]{execute(tool("point_distance"),bad,ctx);});
    }
    for(auto fault:{QString("binding"),QString("version"),QString("content"),QString("resolution"),QString("source"),QString("axes")}){
        auto bad=ctx;if(fault=="binding")bad.calibration.reset();if(fault=="version")++bad.current->version;if(fault=="content")bad.current->contentId=QString(64,'b');if(fault=="resolution")bad.rawSize={255,256};if(fault=="source")bad.current->cameraId="foreign";if(fault=="axes")bad.current->target.axes=AxisConvention::XRightYUp;
        reject([&]{execute(tool("point_distance"),points,bad);});
    }
    auto candidate=MeasurementService{}.teach(f.setup(),{"teach",{{0,0},f.raw},{{100,0},f.raw},10});reject([&]{execute(tool("point_distance"),points,f.context(candidate));});
    auto bad=points;bad.remove("p2");reject([&]{execute(tool("point_distance"),bad,ctx);});bad=points;bad["unused"]=f.port("p1",QPointF{});reject([&]{execute(tool("point_distance"),bad,ctx);});
    bad=points;bad["p1"].data=QPointF(256,20);reject([&]{execute(tool("point_distance"),bad,ctx);});
    for(auto fault:{QString("eval"),QString("method"),QString("frame"),QString("few"),QString("huge"),QString("duplicate")}){
        auto altered=one;auto& v=altered["samples1"];if(fault=="eval")v.evaluationId="other";if(fault=="method")v.method="other";
        auto s=std::get<PointSet>(v.data);if(fault=="frame")s.frame.id="other";if(fault=="few")s.points.resize(5);if(fault=="huge")s.points.resize(129);if(fault=="duplicate")s.points[1]=s.points[0];v.data=s;
        reject([&]{execute(tool("circle_radius"),altered,ctx);});
    }
    for(auto key:{"operation","mode","offset","angle","fitMinPoints","fitInlierDistance","fitMaxRms","fitMaxResidual","fitRansac","lineMinSpan","circleMaxCondition","circleMinAngularSpan","unknown"}){
        auto p=geometryMeasurementDefaults();p[key]=-1;reject([&]{validateGeometryMeasurement(p);});
    }
    auto fractional=geometryMeasurementDefaults();fractional["fitMinPoints"]=6.5;reject([&]{validateGeometryMeasurement(fractional);});
    auto toleranceTool=tool("x_offset");toleranceTool.parameters["offset"]="signed";toleranceTool.parameters["toleranceEnabled"]=true;toleranceTool.parameters["nominal"]=-3.;toleranceTool.parameters["lower"]=-3.;toleranceTool.parameters["upper"]=-3.;
    auto negative=points;negative["p1"].data=QPointF(110,120);negative["p2"].data=QPointF(80,80);
    check(std::get<bool>(execute(toleranceTool,negative,ctx).outputs["accepted"].data),"inclusive negative signed limits");
    toleranceTool.parameters["nominal"]=-2.;toleranceTool.parameters["lower"]=std::nextafter(-3.,0.);toleranceTool.parameters["upper"]=0.;
    check(!std::get<bool>(execute(toleranceTool,negative,ctx).outputs["accepted"].data),"nextafter lower rejects unrounded value");
    auto legacy=tool("point_distance");for(auto key:{"toleranceEnabled","nominal","lower","upper"})legacy.parameters.remove(key);ToolRegistry::validate(legacy);check(!execute(legacy,points,ctx).outputs["accepted"].valid,"T7.2 parameters remain valid without implicit tolerance");
    auto stop=std::make_shared<std::atomic_bool>(true);reject([&]{GeometryMeasurementService::execute(tool("point_distance"),points,f.m.frameId,f.m.cycleId,stop,ctx);});
    // Independent non-affine H maps known world samples; never transform seed radius.
    auto project=[](QPointF q){const double w=1+.015*q.x()+.008*q.y();return QPointF((4*q.x()+20)/w,(4*q.y()+20)/w);};
    ChessboardObservation obs{{"perspective",7,7,5},{f.m.sourceId,"fit","fit-cycle",f.m.coordinateSystemId,f.m.completedAtUtc},f.image.size(),{}, {}};
    for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF w(x*5,y*5);obs.worldMillimetres.append(w);obs.pixels.append(project(w));}
    auto hs=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{project(a),f.raw},{project(b),f.raw},QLineF(a,b).length()};};
    const auto homography=MeasurementService{}.validate(MeasurementService{}.teachHomography(f.setup(),PlaneMappingService::fit(obs)),{hs("hx",{2,3},{27,3}),hs("hy",{3,2},{3,27})},.00001);auto hc=f.context(homography);
    auto hpoints=base();hpoints["p1"]=f.port("p1",project({10,10}));hpoints["p2"]=f.port("p2",project({13,14}));near(number(execute(tool("point_distance"),hpoints,hc),"valueMm"),5,.00001);
    auto samples=circlePoints({15,15},5);QVector<QPointF> projected;QPointF mean;for(auto v:samples){auto raw=project(v);projected.append(raw);mean+=raw;}mean/=projected.size();
    auto hcircle=base();hcircle["circle1"]=f.port("circle1",Circle2D{mean,14,f.raw});hcircle["samples1"]=f.port("samples1",PointSet{projected,f.raw});
    near(number(execute(tool("circle_diameter"),hcircle,hc),"valueMm"),10,.00001);
    auto fake=hcircle;fake["circle1"].data=Circle2D{{40,40},1,f.raw};near(number(execute(tool("circle_diameter"),fake,hc),"valueMm"),10,.00001);
    const auto wrongCentre=PlaneMappingService::map(*homography.mapping(),mean);check(QLineF(wrongCentre,{15,15}).length()>.01,"mapped raw fitted centre negative control");
    auto hline=base();hline["line1"]=f.port("line1",Line2D{project({2,10}),project({28,10}),f.raw});hline["line2"]=f.port("line2",Line2D{project({5,5}),project({25,25}),f.raw});
    QVector<QPointF> a,b;for(auto v:linePoints({2,10},{28,10}))a.append(project(v));for(auto v:linePoints({5,5},{25,25}))b.append(project(v));
    hline["samples1"]=f.port("samples1",PointSet{a,f.raw});hline["samples2"]=f.port("samples2",PointSet{b,f.raw});near(number(execute(tool("line_angle"),hline,hc),"angleMm"),45,.0001);
    check(std::abs(QLineF(std::get<Line2D>(hline["line1"].data).start,std::get<Line2D>(hline["line1"].data).end).angleTo(QLineF(std::get<Line2D>(hline["line2"].data).start,std::get<Line2D>(hline["line2"].data).end))-315)>1,"raw/world angle differs");
    auto outside=hpoints;outside["p1"].data=project({31,15});reject([&]{execute(tool("point_distance"),outside,hc);});
    // Found points can all lie inside the hull while a fitted centre lies outside.
    auto arcPoints=circlePoints({-2,15},8);QVector<QPointF> arc;for(auto w:arcPoints)if(w.x()>1)arc.append(project(w));
    auto off=hcircle;off["samples1"].data=PointSet{arc,f.raw};auto arcTool=tool("circle_radius");arcTool.parameters["circleMinAngularSpan"]=30.;rejectWith([&]{execute(arcTool,off,hc);},"calibrated inner-corner region");
    // Actual file acquisition + Edge Point/Line/Circle outputs into the shared DAG.
    QTemporaryDir dir(QDir::currentPath()+"/geometry-adapter-XXXXXX");check(dir.isValid(),"build-only fixtures");
    QImage stripe(256,256,QImage::Format_Grayscale8);stripe.fill(20);for(int y=60;y<160;++y)for(int x=0;x<256;++x)stripe.scanLine(y)[x]=220;check(stripe.save(dir.filePath("stripe.png")),"independent raster");
    ProjectService projectService;projectService.createProject("Geometry","",dir.filePath("part.vfproject"),"file");projectService.createRecipe("Part","",ApplicationType::Measurement,"file");
    const auto source=projectService.setImageFile(dir.filePath("stripe.png")),edge=projectService.addTool("edge_point"),line=projectService.addTool("line_fit"),geometry=projectService.addTool("geometry_measurement");
    for(auto t:projectService.tools()){
        if(t.id==edge){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{100,190},{100,120},2}}.toJson();t.parameters["polarity"]="rising";}
        if(t.id==line){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},RotatedRectangle{{128,60},{200,50},0}}.toJson();t.parameters["polarity"]="rising";}
        if(t.id==geometry){t.parameters["operation"]="point_line_distance";t.parameters["mode"]="pixel";}projectService.updateTool(t);
    }
    auto connections=QVector<ToolConnection>{{source,"image",edge,"image"},{source,"image",line,"image"},{source,"image",geometry,"image"},{edge,"point",geometry,"p1"},{line,"linePx",geometry,"line1"}};
    projectService.setConnections(connections);check(validateGraph(projectService.tools(),connections).valid(),"operation-specific pixel graph");projectService.save();
    const auto revision=projectService.activeRecipe()->recipeVersion;const auto recipeId=projectService.activeRecipe()->id;projectService.openProject(projectService.path());projectService.selectRecipe(recipeId);projectService.save();check(projectService.activeRecipe()->recipeVersion==revision,"adapter save/reopen no-op");
    auto run=[&](const CalibrationResolver& resolver=CalibrationResolver{}){return ExecutionService{}.run(projectService.activeRecipe()->definition,dir.path(),recipeId,"cfg",{}, {},{},resolver);};
    auto cycle=run();check(cycle.state==ExecutionState::Done,"raw geometric DAG");near(number(result(cycle,geometry),"valuePx"),100,.0001);
    const auto acquired=std::get<ImageFrame>(result(cycle,source).outputs["image"].data);projectService.setCalibrationSetup("lens","plane");const auto setup=CalibrationWorkflow::installation(*projectService.activeRecipe(),acquired);
    auto ps=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,setup.source},{b,setup.source},mm};};
    const auto accepted=MeasurementService{}.validate(CalibrationWorkflow::teach(*projectService.activeRecipe(),acquired,"scale",1,ps("teach",{0,0},{100,0},10)),{ps("x",{20,20},{120,20},10),ps("y",{30,30},{30,130},10)},.00001);
    const auto catalog=projectService.path()+".calibrations.json";CalibrationService(catalog).save(accepted);CalibrationWorkflow::bind(projectService,acquired,catalog,"scale");
    for(auto t:projectService.tools())if(t.id==geometry){t.parameters["mode"]="calibrated";projectService.updateTool(t);}
    check(!validateGraph(projectService.tools(),connections).valid(),"calibrated fit requires source samples");
    connections.append({line,"observationsPx",geometry,"samples1"});projectService.setConnections(connections);
    auto wrong=connections;wrong.append({edge,"point",geometry,"p2"});check(!validateGraph(projectService.tools(),wrong).valid(),"unused input rejected at graph");
    wrong=connections;wrong.last().sourceTool=source;wrong.last().sourcePort="image";check(!validateGraph(projectService.tools(),wrong).valid(),"wrong sample type/source rejected");
    auto graphTools=projectService.tools();ToolDefinition secondLine;for(auto t:graphTools)if(t.id==line)secondLine=t;secondLine.id="foreign-line";secondLine.enabled=false;graphTools.append(secondLine);
    wrong=connections;wrong.last().sourceTool=secondLine.id;check(!validateGraph(graphTools,wrong).valid(),"same-typed samples from another feature rejected");
    auto resolver=[&](const ImageFrame& frame){return CalibrationWorkflow::resolve(*projectService.activeRecipe(),frame,catalog);};cycle=run(resolver);check(cycle.state==ExecutionState::Done,"calibrated refit DAG");near(number(result(cycle,geometry),"valueMm"),10,.0001);
    check(result(cycle,geometry).calibration==accepted.dependency() && !result(cycle,geometry).outputs["valueMm"].diagnostic,"resolved exact binding");
    for(auto t:projectService.tools())if(t.id==geometry){t.parameters["toleranceEnabled"]=true;t.parameters["lower"]=9.99;t.parameters["nominal"]=10.;t.parameters["upper"]=10.01;projectService.updateTool(t);}projectService.save();
    RuntimeService production(dir.filePath("geometry-archive"),catalog,RuntimeIdentity::localEngineer());production.prepare(*projectService.activeRecipe(),dir.path());const auto lease=production.begin();const auto completed=RuntimeService::execute(lease,catalog,{});check(completed.state==ExecutionState::Done,"Geometry Runtime acquisition/refit");
    for(auto fault:{"value","boolean","diagnostic","status","missing","overlay","calibration","frame","method","invalid","opposite"}){
        auto forged=completed;for(auto& r:forged.tools)if(r.toolId==geometry){
            if(QString(fault)=="value")r.outputs["valueMm"].data=std::get<double>(r.outputs["valueMm"].data)+.001;
            if(QString(fault)=="boolean")r.outputs["accepted"].data=false;
            if(QString(fault)=="diagnostic")r.outputs["valueMm"].diagnostic=true;
            if(QString(fault)=="status")r.outputs["status"].data=QString("PARALLEL");
            if(QString(fault)=="missing")r.outputs.remove("accepted");
            if(QString(fault)=="overlay")r.points[0]+=QPointF(.01,0);
            if(QString(fault)=="calibration")r.calibration.reset();
            if(QString(fault)=="frame")r.outputs["valueMm"].origin.sourceId="foreign";
            if(QString(fault)=="method")r.outputs["valueMm"].methodVersion=99;
            if(QString(fault)=="invalid")r.outputs["valueMm"].valid=false;
            if(QString(fault)=="opposite"){r.outputs["valuePx"].valid=true;r.outputs["valuePx"].data=100.;}
        }reject([&]{production.finish(lease.ticket,forged);});
    }
    for(auto t:projectService.tools())if(t.id==geometry){t.parameters["upper"]=100.;projectService.updateTool(t);} // active snapshot stays exact
    production.finish(lease.ticket,completed);const auto records=production.records();check(records.size()==1 && ProductionResultCodec::csv(records).contains("geometry-refit"),"Geometry Logs CSV actual typed values");check(ProductionStore(dir.filePath("geometry-archive")).recent("cycles")==records,"Geometry restart archive");
    for(auto t:projectService.tools())if(t.id==geometry){t.parameters["toleranceEnabled"]=false;projectService.updateTool(t);}
    cycle=run(resolver);
    EditorSession session;auto context=[&]{return EditorContext{projectService.project().id,projectService.path(),recipeId,projectService.activeRecipe()->definition,projectService.activeRecipe()->calibrationBinding->toJson()};};auto ec=context();check(session.acceptCycle(cycle,session.signature(ec),ec),"full adapter cache");
    for(auto t:projectService.tools())if(t.id==geometry){t.parameters["offset"]="signed";projectService.updateTool(t);}ec=context();const auto before=session.frame->contentId();
    auto target=ExecutionService{}.runTool(ec.definition,dir.path(),recipeId,"cfg",{},geometry,session.evaluation(ec),resolver);check(target.state==ExecutionState::Done && target.acquisitions==0 && target.executedTools==QStringList{geometry} && session.frame->contentId()==before,"same-frame TargetOnly refit no acquisition");near(number(result(target,geometry),"valueMm"),-10,.0001);
    for(auto t:projectService.tools())if(t.id==line){t.parameters["minContrast"]=30;projectService.updateTool(t);}ec=context();auto stale=ExecutionService{}.runTool(ec.definition,dir.path(),recipeId,"cfg",{},geometry,session.evaluation(ec),resolver);check(result(stale,geometry).state==ExecutionState::Skipped && result(stale,geometry).outputs.isEmpty(),"changed upstream invalidates adapter cache");
    auto unbound=run();check(result(unbound,geometry).state==ExecutionState::Error && result(unbound,geometry).outputs.isEmpty(),"no calibration never px fallback");
    for(auto t:projectService.tools())if(t.id==edge){t.parameters["minContrast"]=5100;projectService.updateTool(t);}auto missing=run(resolver);check(result(missing,geometry).state==ExecutionState::Skipped && result(missing,geometry).outputs.isEmpty(),"missing feature downstream SKIPPED");
    auto stopped=ExecutionService{}.run(projectService.activeRecipe()->definition,dir.path(),recipeId,"cfg",stop,{}, {},resolver);check(stopped.state==ExecutionState::Skipped,"DAG Stop");for(const auto& r:stopped.tools)check(r.outputs.isEmpty() && !r.calibration,"Stop clears geometry/binding");
    projectService.save();RuntimeService runtime(dir.filePath("archive"),catalog,RuntimeIdentity::localEngineer());runtime.prepare(*projectService.activeRecipe(),dir.path());auto missingLease=runtime.begin();auto missingCycle=RuntimeService::execute(missingLease,catalog,{});runtime.finish(missingLease.ticket,missingCycle);check(result(missingCycle,geometry).outputs.isEmpty(),"missing feature Runtime has no stale geometry");
    QImage ring(256,256,QImage::Format_Grayscale8);for(int y=0;y<256;++y)for(int x=0;x<256;++x){const double d=std::hypot(x-128.25,y-127.75)-40.;ring.scanLine(y)[x]=uchar(std::lround(128+100*std::erf(d/(std::sqrt(2.)*1.4))));}check(ring.save(dir.filePath("stripe.png")),"independent circle raster");
    const auto circleId=projectService.addTool("circle_fit");
    for(auto t:projectService.tools()){
        if(t.id==edge || t.id==line)t.enabled=false;
        if(t.id==circleId){t.parameters=defaultParameters("circle_fit");t.parameters["scanRoi"]=ToolRoi{{source+"/image"},Annulus{{128.25,127.75},34,46}}.toJson();t.parameters["stepPx"]=.5;t.parameters["polarity"]="rising";}
        if(t.id==geometry){t.parameters["operation"]="circle_diameter";t.parameters["offset"]="absolute";}projectService.updateTool(t);
    }
    projectService.setConnections({{source,"image",circleId,"image"},{source,"image",geometry,"image"},{circleId,"circlePx",geometry,"circle1"},{circleId,"observationsPx",geometry,"samples1"}});
    auto circleCycle=run(resolver);check(circleCycle.state==ExecutionState::Done,"real Circle -> calibrated geometry DAG");near(number(result(circleCycle,geometry),"valueMm"),8,.03);
    for(auto t:projectService.tools())if(t.id==geometry){t.parameters["circleMinAngularSpan"]=359.;projectService.updateTool(t);}auto failedRefit=run(resolver);check(result(failedRefit,circleId).state==ExecutionState::Done && result(failedRefit,geometry).state==ExecutionState::Error && result(failedRefit,geometry).outputs.isEmpty() && result(failedRefit,geometry).fitDiagnostic && result(failedRefit,geometry).fitDiagnostic->stage=="geometry_refit","downstream refit is distinct from missing upstream geometry");
    for(auto t:projectService.tools())if(t.id==geometry){t.parameters["circleMinAngularSpan"]=90.;projectService.updateTool(t);}
    projectService.save();RuntimeService circleRuntime(dir.filePath("circle-geometry-archive"),catalog,RuntimeIdentity::localEngineer());circleRuntime.prepare(*projectService.activeRecipe(),dir.path());auto circleLease=circleRuntime.begin();auto circleCompleted=RuntimeService::execute(circleLease,catalog,{});circleRuntime.finish(circleLease.ticket,circleCompleted);check(result(circleCompleted,geometry).outputs["valueMm"].valid,"Circle world refit exact Runtime replay");
    auto stoppedLease=circleRuntime.begin();auto stoppedCycle=RuntimeService::execute(stoppedLease,catalog,stop);circleRuntime.finish(stoppedLease.ticket,stoppedCycle);check(stoppedCycle.state==ExecutionState::Skipped && result(stoppedCycle,geometry).outputs.isEmpty(),"Geometry Runtime Stop no old output");
    // Two independent raster holes and two orthogonal raster edges; exact replay for each.
    QImage holes(256,256,QImage::Format_Grayscale8);for(int y=0;y<256;++y)for(int x=0;x<256;++x){double d=std::min(std::hypot(x-80.,y-128.),std::hypot(x-176.,y-128.))-20.;holes.scanLine(y)[x]=uchar(std::lround(128+100*std::erf(d/(std::sqrt(2.)*1.4))));}check(holes.save(dir.filePath("stripe.png")),"independent two holes raster");
    const auto secondCircle=projectService.addTool("circle_fit");
    for(auto t:projectService.tools()){
        if(t.id==circleId || t.id==secondCircle){t.parameters=defaultParameters("circle_fit");t.parameters["scanRoi"]=ToolRoi{{source+"/image"},Annulus{{t.id==circleId ? 80.:176.,128},14,26}}.toJson();t.parameters["stepPx"]=.5;t.parameters["polarity"]="rising";}
        if(t.id==geometry)t.parameters["operation"]="circle_centre_distance";projectService.updateTool(t);
    }
    projectService.setConnections({{source,"image",circleId,"image"},{source,"image",secondCircle,"image"},{source,"image",geometry,"image"},{circleId,"circlePx",geometry,"circle1"},{circleId,"observationsPx",geometry,"samples1"},{secondCircle,"circlePx",geometry,"circle2"},{secondCircle,"observationsPx",geometry,"samples2"}});projectService.save();
    RuntimeService holesRuntime(dir.filePath("holes-archive"),catalog,RuntimeIdentity::localEngineer());holesRuntime.prepare(*projectService.activeRecipe(),dir.path());auto holesLease=holesRuntime.begin();auto holesCycle=RuntimeService::execute(holesLease,catalog,{});near(number(result(holesCycle,geometry),"valueMm"),9.6,.002);holesRuntime.finish(holesLease.ticket,holesCycle);
    QImage corner(256,256,QImage::Format_Grayscale8);corner.fill(20);for(int y=100;y<256;++y)for(int x=100;x<256;++x)corner.scanLine(y)[x]=220;check(corner.save(dir.filePath("stripe.png")),"independent orthogonal edges raster");const auto verticalLine=projectService.addTool("line_fit");
    for(auto t:projectService.tools()){
        if(t.id==circleId || t.id==secondCircle)t.enabled=false;
        if(t.id==line || t.id==verticalLine){t.enabled=true;t.parameters["scanRoi"]=ToolRoi{{source+"/image"},t.id==line ? RotatedRectangle{{170,100},{100,40},0}:RotatedRectangle{{100,170},{100,40},90}}.toJson();t.parameters["polarity"]=t.id==line ? "rising":"falling";}
        if(t.id==geometry)t.parameters["operation"]="line_angle";projectService.updateTool(t);
    }
    projectService.setConnections({{source,"image",line,"image"},{source,"image",verticalLine,"image"},{source,"image",geometry,"image"},{line,"linePx",geometry,"line1"},{line,"observationsPx",geometry,"samples1"},{verticalLine,"linePx",geometry,"line2"},{verticalLine,"observationsPx",geometry,"samples2"}});projectService.save();
    RuntimeService angleRuntime(dir.filePath("angle-archive"),catalog,RuntimeIdentity::localEngineer());angleRuntime.prepare(*projectService.activeRecipe(),dir.path());auto angleLease=angleRuntime.begin();auto angleCycle=RuntimeService::execute(angleLease,catalog,{});near(number(result(angleCycle,geometry),"angleMm"),90,.001);angleRuntime.finish(angleLease.ticket,angleCycle);
    QTextStream(stdout)<<"raster circle diameter mm="<<number(result(circleCycle,geometry),"valueMm")<<"\n";
    // Ensure output schema supports diagnostic/unavailable replay without inventing decisions.
    auto replay=execute(tool("line_intersection"),parallel,ctx);for(auto v:replay.outputs){v.evaluationId="output-eval";auto decoded=ToolResultCodec::decode(ToolResultCodec::encode(v));check(decoded.valid==v.valid,"typed adapter output roundtrip");}
    QTextStream(stdout)<<checks<<" geometry adapter checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
