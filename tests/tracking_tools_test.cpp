#include "application/PoseRoiService.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "application/MeasurementService.hpp"
#include "domain/PoseMatchConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "vision/metrology/EdgeFinder.hpp"
#include "vision/metrology/RadialScanner.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <QJsonArray>
#include <numbers>
#include <limits>
#include <type_traits>
using namespace visionflow;
namespace {
int checks=0;void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
void near(QPointF a,QPointF b,double e=1e-7){check(QLineF(a,b).length()<=e,"independent point oracle");}
template<class F>void reject(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"expected rejection");}
const CoordinateFrame raw{"camera/image"},local{"part/local"};
const QString hash(64,'a');
FrameMetadata m{"camera","frame","cycle",raw.id,QDateTime::currentDateTimeUtc()};
ImageFrame image(const QByteArray& bytes){return ImageFrame::fromBuffer(bytes.data(),bytes.size(),256,256,256,PixelFormat::Gray8,m,false);}
ToolResult synthetic(const ToolDefinition& source,Pose2D pose){
    ToolResult r;r.toolId=source.id;r.state=ExecutionState::Done;
    for(const auto& d:outputPorts(source.kind)){
        PortValue v{d,false,true,m.frameId,m.cycleId,m};v.evaluationId="evaluation";v.method=source.kind=="shape_pose" ? "unsigned-oriented-chamfer-pyramid":"masked-ccoeff-normed-pyramid";v.methodVersion=1;
        if(d.id=="pose")v.data=pose;else if(d.id=="scale" || d.id=="score")v.data=1.;else if(d.id=="found")v.data=true;else if(d.id=="candidates")v.data=QString("[]");r.outputs[d.id]=v;
    }return r;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const Pose2D taught{{60.25,60.5},.3,raw},current{{140.5,120.25},.3+std::numbers::pi/2,raw};
    QVector<ToolRoi> shapes{{raw,ScanSegment{{70.25,55.5},{90.25,55.5},3.5}},
        {raw,Annulus{{80.25,60.5},3.25,6.75}},{raw,PolygonRoi{{{70.25,50.5},{90.25,50.5},{85.25,70.5}}}},
        {raw,ArcRoi{{{80.25,60.5},3,6},-30,-170}},{raw,RotatedRectangle{{80.25,60.5},{12,4},22}},
        {raw,PathRoi{{{70.25,50.5},{90.25,55.5},{85.25,70.5}}}}};
    auto oracle=[&](QPointF p){auto d=p-taught.position;return current.position+QPointF(-d.y(),d.x());};
    for(const auto& roi:shapes){auto b=PoseRoi::teach(roi,taught,local,"pose",hash,{256,256},m.sourceId);auto round=PoseRoi::fromJson(b.toJson());check(round.toJson()==b.toJson(),"tracking bounded exact roundtrip");auto resolved=b.resolve(current,1,{256,256});check(resolved.geometry.index()==roi.geometry.index(),"geometry type preserved");
        auto unchanged=b.resolve(taught,1,{256,256});near(unchanged.bounds().topLeft(),roi.bounds().topLeft());near(unchanged.bounds().bottomRight(),roi.bounds().bottomRight());
        reject([&]{b.resolve(current,std::nextafter(1.,2.),{256,256});});reject([&]{b.resolve(current,.9,{256,256});});reject([&]{b.resolve(current,std::numeric_limits<double>::quiet_NaN(),{256,256});});
        auto wrong=current;wrong.frame.id="other";reject([&]{b.resolve(wrong,1,{256,256});});wrong=current;wrong.position={-100,-100};reject([&]{b.resolve(wrong,1,{256,256});});
        reject([&]{b.resolve(current,1,{512,512});});
        auto json=b.toJson();json["extra"]=1;reject([&]{PoseRoi::fromJson(json);});json=b.toJson();json["schemaVersion"]=2;reject([&]{PoseRoi::fromJson(json);});
        if(auto g=std::get_if<ScanSegment>(&resolved.geometry)){const auto original=std::get<ScanSegment>(roi.geometry);near(g->start,oracle(original.start));near(g->end,oracle(original.end));check(g->width==original.width,"scan width invariant");}
        if(auto g=std::get_if<Annulus>(&resolved.geometry)){const auto original=std::get<Annulus>(roi.geometry);near(g->centre,oracle(original.centre));check(g->innerRadius==original.innerRadius && g->outerRadius==original.outerRadius,"radii invariant");}
        if(auto g=std::get_if<PolygonRoi>(&resolved.geometry)){const auto original=std::get<PolygonRoi>(roi.geometry);for(int i=0;i<g->vertices.size();++i)near(g->vertices[i],oracle(original.vertices[i]));}
        if(auto g=std::get_if<ArcRoi>(&resolved.geometry)){near(g->ring.centre,oracle(std::get<ArcRoi>(roi.geometry).ring.centre));check(g->sweepDegrees==-170 && std::abs(g->startDegrees-60)<1e-9,"signed sweep and rotated start");}
        if(auto g=std::get_if<RotatedRectangle>(&resolved.geometry))check(std::abs(g->angleDegrees-112)<1e-9 && g->size==QSizeF(12,4),"rotated rectangle not AABB");
    }
    for(const QString kind:{"pattern_pose","shape_pose"}){
    // Independent raster: exact asymmetric model plus an edge away from model support.
    QByteArray teachBytes(256*256,char(30));for(int y=0;y<24;++y)for(int x=0;x<24;++x)teachBytes[(46+y)*256+51+x]=char(40+(x*37+y*71+x*y*13)%190);
    auto teaching=image(teachBytes);ToolDefinition source{"pose",kind,"Pose","",true,poseMatchDefaults()};
    source.parameters["angleMin"]=0.;source.parameters["angleMax"]=90.;source.parameters["angleStep"]=90.;source.parameters["pyramidLevels"]=1;source.parameters["minScore"]=.95;
    source.parameters=(kind=="shape_pose" ? ShapeMatchService::teach:PoseMatchService::teach)(teaching,{51,46,24,24},{9,14},source.parameters);
    auto run=[&](const ImageFrame& frame){auto r=PoseMatchService::execute(source,{{"image",{{"image",PortType::Image,"px","image"},frame,true,m.frameId,m.cycleId,m}}},m.frameId,m.cycleId,{});for(auto& v:r.outputs)v.evaluationId="actual-evaluation";return r;};
    const auto taughtResult=run(teaching);check(taughtResult.outputs["pose"].valid,"real taught pose selected");
    auto binding=PoseRoiService::teach({raw,ScanSegment{{90,52},{90,68},2}},local,source,taughtResult,teaching);
    QByteArray moved(256*256,char(30));for(int y=0;y<24;++y)for(int x=0;x<24;++x)moved[(120+x-9)*256+140-(y-14)]=teachBytes[(46+y)*256+51+x];
    for(int y=146;y<=154;++y)for(int x=128;x<=152;++x)moved[y*256+x]=char(std::lround(128+100*std::erf((140-x)/1.4)));
    const auto movedImage=image(moved);auto actual=run(movedImage);check(actual.outputs["pose"].valid,"real rotated pose selected");const auto selected=std::get<Pose2D>(actual.outputs["pose"].data);near(selected.position,{140,120},1.5);check(std::abs(selected.angleRadians-std::numbers::pi/2)<.05,"real pose angle");
    const auto resolved=PoseRoiService::resolve(binding,source,actual,movedImage);check(resolved.state==ExecutionState::Done && resolved.roi.has_value(),"real pose resolves ROI");
    // Both actual matchers must preserve all six shapes, including orientation,
    // signed arc sweep and polygon/path order. Expected delta is independently 90 degrees.
    for(const auto& roi:shapes){
        const auto b=PoseRoiService::teach(roi,local,source,taughtResult,teaching);
        const auto decoded=PoseRoi::fromJson(b.toJson());check(decoded.toJson()==b.toJson(),"both methods roundtrip existing schema");
        const auto value=PoseRoiService::resolve(decoded,source,actual,movedImage);
        check(value.state==ExecutionState::Done && value.roi && value.roi->geometry.index()==roi.geometry.index(),"both methods resolve six geometry types");
        auto transform=[](QPointF q){const auto d=q-QPointF(60,60);return QPointF(140-d.y(),120+d.x());};
        std::visit([&](const auto& g){using T=std::decay_t<decltype(g)>;const auto& original=std::get<T>(roi.geometry);
            if constexpr(std::is_same_v<T,ScanSegment>){near(g.start,transform(original.start),1.5);near(g.end,transform(original.end),1.5);check(g.width==original.width,"actual rigid scan width");}
            else if constexpr(std::is_same_v<T,Annulus>){near(g.centre,transform(original.centre),1.5);check(g.innerRadius==original.innerRadius && g.outerRadius==original.outerRadius,"actual rigid radii");}
            else if constexpr(std::is_same_v<T,ArcRoi>){near(g.ring.centre,transform(original.ring.centre),1.5);check(std::abs(g.startDegrees-original.startDegrees-90)<3 && g.sweepDegrees==original.sweepDegrees,"actual signed arc orientation");}
            else if constexpr(std::is_same_v<T,RotatedRectangle>){near(g.centre,transform(original.centre),1.5);check(g.size==original.size && std::abs(g.angleDegrees-original.angleDegrees-90)<3,"actual rectangle orientation");}
            else {check(g.vertices.size()==original.vertices.size(),"actual vertex count");for(int i=0;i<g.vertices.size();++i)near(g.vertices[i],transform(original.vertices[i]),1.5);}
        },value.roi->geometry);
    }
    EdgeOptions options;options.polarity=EdgePolarity::Rising;options.profile.stepPx=.25;const auto edge=EdgeFinder::find(movedImage,*resolved.roi,options);check(edge.selected.has_value(),"tracked rotated scan samples real edge");near(edge.candidates[*edge.selected].point,{140,150},1.5);
    const auto rawHash=movedImage.contentId();
    auto blocked=[&](ToolResult r,ExecutionState expected){auto value=PoseRoiService::resolve(binding,source,r,movedImage);check(value.state==expected && !value.roi && !value.reason.isEmpty(),"invalid reference has no absolute fallback");};
    auto absent=image(QByteArray(256*256,char(30)));const auto notFound=run(absent);
    check(notFound.state==ExecutionState::Done && !notFound.outputs["pose"].valid,"actual absent reference has no selected pose");
    const auto absentRoi=PoseRoiService::resolve(binding,source,notFound,absent);
    check(absentRoi.state==ExecutionState::Skipped && !absentRoi.roi,"actual not-found has no ROI fallback");
    reject([&]{PoseRoiService::teach(shapes.first(),local,source,notFound,absent);});
    QByteArray repeated=teachBytes;for(int y=0;y<24;++y)for(int x=0;x<24;++x)repeated[(166+y)*256+191+x]=teachBytes[(46+y)*256+51+x];
    const auto repeatImage=image(repeated);const auto repeat=run(repeatImage);
    check(std::get<bool>(repeat.outputs["ambiguous"].data) && !repeat.outputs["pose"].valid,"actual repeated models reject selection");
    const auto repeatRoi=PoseRoiService::resolve(binding,source,repeat,repeatImage);
    check(repeatRoi.state==ExecutionState::Skipped && !repeatRoi.roi,"actual repeated models reject tracking");
    reject([&]{PoseRoiService::teach(shapes.first(),local,source,repeat,repeatImage);});
    auto bestSource=source;bestSource.parameters["selection"]="best";
    auto best=PoseMatchService::execute(bestSource,{{"image",{{"image",PortType::Image,"px","image"},repeatImage,true,m.frameId,m.cycleId,m}}},m.frameId,m.cycleId,{});
    for(auto& v:best.outputs)v.evaluationId="best-evaluation";
    check(std::get<bool>(best.outputs["ambiguous"].data) && best.outputs["pose"].valid,"actual explicit best retains ambiguity evidence");
    check(PoseRoiService::resolve(binding,bestSource,best,repeatImage).state==ExecutionState::Done,"actual explicit best resolves rigid ROI");
    if(kind=="shape_pose"){
        auto reversed=moved;for(int y=111;y<=134;++y)for(int x=131;x<=154;++x)reversed[y*256+x]=char(255-static_cast<unsigned char>(moved[y*256+x]));
        // Only the rotated model footprint is reversed; downstream edge raster is retained.
        const auto reversedImage=image(reversed);const auto polarityResult=run(reversedImage);
        const auto polarityRoi=PoseRoiService::resolve(binding,source,polarityResult,reversedImage);
        check(polarityRoi.state==ExecutionState::Done && polarityRoi.roi,"real contrast-reversed shape resolves tracking");
        const auto hit=EdgeFinder::find(reversedImage,*polarityRoi.roi,options);check(hit.selected.has_value(),"contrast-reversed model keeps actual downstream edge");
        near(hit.candidates[*hit.selected].point,{140,150},1.5);
    }
    auto otherSource=movedImage.metadata();otherSource.sourceId="other-camera";auto otherResult=actual;for(auto& v:otherResult.outputs)v.origin=otherSource;
    check(!PoseRoiService::resolve(binding,source,otherResult,movedImage.withMetadata(otherSource)).roi,"teaching source change rejected even with consistent current result");
    for(auto state:{ExecutionState::Ready,ExecutionState::Busy,ExecutionState::Error,ExecutionState::Skipped}){auto bad=actual;bad.state=state;blocked(bad,ExecutionState::Skipped);}
    auto bad=actual;bad.outputs["found"].data=false;blocked(bad,ExecutionState::Skipped);bad=actual;bad.outputs["ambiguous"].data=true;blocked(bad,ExecutionState::Skipped);bad=actual;bad.outputs["truncated"].data=true;blocked(bad,ExecutionState::Skipped);
    bad=actual;bad.outputs["pose"].valid=false;bad.outputs["pose"].invalidReason="unselected";blocked(bad,ExecutionState::Skipped);
    bad=actual;bad.outputs["scale"].data=1.001;blocked(bad,ExecutionState::Error);
    bad=actual;bad.outputs["pose"].diagnostic=true;blocked(bad,ExecutionState::Error);
    bad=actual;bad.outputs["pose"].origin.exposureAtUtc=m.completedAtUtc;blocked(bad,ExecutionState::Error);
    bad=actual;bad.outputs["scale"].origin.deviceTimestampTicks=42;blocked(bad,ExecutionState::Error);
    bad=actual;bad.outputs["pose"].methodVersion=2;blocked(bad,ExecutionState::Error);
    for(auto id:{"pose","scale","found","ambiguous","truncated"}){
        bad=actual;bad.outputs[id].method=kind=="shape_pose" ? "masked-ccoeff-normed-pyramid":"unsigned-oriented-chamfer-pyramid";blocked(bad,ExecutionState::Error);
    }
    auto swapped=source;swapped.kind=kind=="shape_pose" ? "pattern_pose":"shape_pose";
    check(PoseRoiService::geometryIdentity(swapped)!=binding.geometryId,"method kind participates in geometry identity");
    check(!PoseRoiService::resolve(binding,swapped,synthetic(swapped,selected),movedImage).roi,"switching matcher requires reteaching even with method-consistent result");
    auto disabled=source;disabled.enabled=false;check(PoseRoiService::resolve(binding,disabled,actual,movedImage).state==ExecutionState::Error,"disabled producer rejected");
    reject([&]{PoseRoiService::teach(shapes.first(),local,disabled,actual,movedImage);});
    auto other=actual;other.toolId="other-pose";blocked(other,ExecutionState::Error);
    for(auto id:{"pose","scale","found","ambiguous","truncated"}){bad=actual;bad.outputs[id].cycleId="old";blocked(bad,ExecutionState::Error);bad=actual;bad.outputs[id].origin.sourceId="other";blocked(bad,ExecutionState::Error);bad=actual;bad.outputs[id].evaluationId="other";blocked(bad,ExecutionState::Error);bad=actual;bad.outputs[id].origin.completedAtUtc=m.completedAtUtc.addMSecs(1);blocked(bad,ExecutionState::Error);bad=actual;bad.outputs.remove(id);blocked(bad,ExecutionState::Error);}
    auto edited=source;edited.parameters["anchor"]=QJsonObject{{"x",10},{"y",14}};check(!PoseRoiService::resolve(binding,edited,actual,movedImage).roi,"anchor edit invalidates teaching");edited=source;edited.parameters["minScore"]=.9;check(PoseRoiService::geometryIdentity(edited)==binding.geometryId,"threshold change preserves geometry identity");
    edited=source;edited.parameters["selection"]="best";bad=actual;bad.outputs["ambiguous"].data=true;check(PoseRoiService::resolve(binding,edited,bad,movedImage).state==ExecutionState::Done,"explicit best preserves selection policy");
    edited=source;auto modelRoi=edited.parameters["modelRoi"].toObject();modelRoi["x"]=52;edited.parameters["modelRoi"]=modelRoi;
    check(!PoseRoiService::resolve(binding,edited,actual,movedImage).roi,"model ROI edit invalidates teaching");
    edited=source;auto changedBytes=teachBytes;changedBytes[46*256+51]=char(0);
    edited.parameters=PoseMatchService::teach(image(changedBytes),{51,46,24,24},{9,14},edited.parameters);
    check(!PoseRoiService::resolve(binding,edited,actual,movedImage).roi,"model pixels edit invalidates teaching");
    edited=source;edited.name="renamed";check(PoseRoiService::geometryIdentity(edited)==binding.geometryId,"display name preserves geometry identity");
    // Raw annulus is sampled with the existing radial engine after rigid resolution.
    const Pose2D p{{120,120},.5,raw};auto ring=PoseRoi::teach({raw,Annulus{{120,120},17,23}},p,local,"pose",hash,{256,256},m.sourceId).resolve(p,1,{256,256});
    QByteArray circle(256*256,char(30));for(int y=0;y<256;++y)for(int x=0;x<256;++x)circle[y*256+x]=char(std::lround(128+100*std::erf((std::hypot(x-120.,y-120.)-20)/1.4)));
    RadialScanOptions radial;radial.scanCount=24;radial.scanWidth=1;radial.edge.polarity=EdgePolarity::Rising;auto hits=RadialScanner::find(image(circle),ring,radial);check(hits.foundCount==24,"tracked ring samples real raster");for(const auto& hit:hits.scans)check(std::abs(QLineF(*hit.rawPoint,{120,120}).length()-20)<.2,"ring independent radius oracle");
    // Non-affine perspective chain oracle. Never assign image angle to world pose.
    auto project=[](QPointF q){double w=1+.015*q.x()+.008*q.y();return QPointF((4*q.x()+20)/w,(4*q.y()+20)/w);};
    auto inverse=[](QPointF q){double w=4-.015*q.x()-.008*q.y();double sum=(4-.3-.16)/w;return QPointF((q.x()*sum-20)/4,(q.y()*sum-20)/4);};
    CoordinateFrame world{"plane",LengthUnit::Millimetre,AxisConvention::XRightYDown};
    ChessboardObservation obs{{"perspective",7,7,5},{m.sourceId,"fit","fit-cycle",m.coordinateSystemId,m.completedAtUtc},{256,256},{},{}};
    for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);obs.worldMillimetres<<q;obs.pixels<<project(q);}
    CalibrationDependency setup{"H",1,"pending",m.sourceId,"lens",{256,256},"plane",raw,world};
    auto sample=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{project(a),raw},{project(b),raw},QLineF(a,b).length()};};
    auto cal=MeasurementService{}.validate(MeasurementService{}.teachHomography(setup,PlaneMappingService::fit(obs)),{sample("hx",{2,3},{27,3}),sample("hy",{3,2},{3,27})},1e-5);
    MeasurementExecutionContext ctx{std::make_shared<PlanarCalibration>(cal),cal.dependency(),{256,256},{}};
    Pose2D hp{project({15,15}),0,raw},hc{hp.position+QPointF(2,1),.15,raw};auto hresult=synthetic(source,hc);
    auto hb=PoseRoi::teach({raw,Annulus{hp.position,1,3}},hp,local,source.id,PoseRoiService::geometryIdentity(source),{256,256},m.sourceId);hb.calibration=cal.dependency();hb=PoseRoi::fromJson(hb.toJson());
    QVector<Point2D> nominal;for(int k=0;k<24;++k){double a=2*std::numbers::pi*k/24;nominal.append({QPointF(15+2*std::cos(a),15+2*std::sin(a)),world});}
    const auto mapped=PoseRoiService::worldPoints(hb,source,hresult,movedImage,ctx,nominal);
    for(int i=0;i<nominal.size();++i){auto d=project(nominal[i].position)-hp.position;QPointF r=hc.position+QPointF(std::cos(.15)*d.x()-std::sin(.15)*d.y(),std::sin(.15)*d.x()+std::cos(.15)*d.y());near(mapped[i].position,inverse(r),1e-5);}
    check(QLineF(mapped[0].position,mapped[12].position).length()!=4.,"perspective image rigid movement is not world rigid circle");
    auto invalid=ctx;++invalid.current->version;reject([&]{PoseRoiService::worldPoints(hb,source,hresult,movedImage,invalid,nominal);});invalid=ctx;invalid.rawSize={128,128};reject([&]{PoseRoiService::worldPoints(hb,source,hresult,movedImage,invalid,nominal);});
    invalid=ctx;invalid.current->lensId="other-lens";reject([&]{PoseRoiService::worldPoints(hb,source,hresult,movedImage,invalid,nominal);});
    auto outside=nominal;outside[0].position={40,15};reject([&]{PoseRoiService::worldPoints(hb,source,hresult,movedImage,ctx,outside);});auto wrong=nominal;wrong[0].frame.axes=AxisConvention::XRightYUp;reject([&]{PoseRoiService::worldPoints(hb,source,hresult,movedImage,ctx,wrong);});
    reject([&]{PoseRoiService::worldPoints(hb,source,hresult,movedImage,ctx,nominal,std::make_shared<std::atomic_bool>(true));});check(movedImage.contentId()==rawHash,"tracking and sampling preserve owned raw pixels");
    }
    QTextStream(stdout)<<checks<<" tracking checks passed; software fixtures only\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}


