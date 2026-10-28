#include "application/ReferenceService.hpp"
#include "application/ToolOverlayGeometry.hpp"
#include "application/WorldTrackingService.hpp"
#include "application/MeasurementService.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <numbers>
#include <cmath>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
template<class F>void reject(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"expected rejection");}
void near(QPointF a,QPointF b,double e){check(QLineF(a,b).length()<e,"independent coordinate oracle");}
const CoordinateFrame raw{"camera/image"};
FrameMetadata metadata{"camera","frame","cycle",raw.id,QDateTime::currentDateTimeUtc()};
bool cropMode=false;
QPointF cropOffset(){return cropMode ? QPointF(10,10):QPointF{};}
ImageFrame image(const QByteArray& b){auto root=ImageFrame::fromBuffer(b.data(),b.size(),256,256,256,PixelFormat::Gray8,metadata,false);return cropMode ? ImageAccess::cropped(root,{10,10,220,220},false):root;}
QPointF project(QPointF q){double w=1+.015*q.x()+.008*q.y();return {(4*q.x()+20)/w,(4*q.y()+20)/w};}
QPointF inverse(QPointF q){double s=3.54/(4-.015*q.x()-.008*q.y());return {(q.x()*s-20)/4,(q.y()*s-20)/4};}
QPointF rotate(QPointF p,double a){return {std::cos(a)*p.x()-std::sin(a)*p.y(),std::sin(a)*p.x()+std::cos(a)*p.y()};}
ToolResult result(const ToolDefinition& source,QPointF p,double a=0){
    const auto scope=image(QByteArray(256*256,char(30)));p-=cropOffset();
    ToolResult r;r.toolId=source.id;r.state=ExecutionState::Done;
    for(const auto& d:outputPorts(source.kind)){
        PortValue v{d,false,true,metadata.frameId,metadata.cycleId,scope.metadata()};v.imageGeometry=scope.geometry();v.evaluationId="current";v.method=source.kind=="edge_point" ? "profile-edge":source.kind=="shape_pose" ? "unsigned-oriented-chamfer-pyramid":"masked-ccoeff-normed-pyramid";v.methodVersion=1;
        if(d.id=="pose")v.data=Pose2D{p,a,{scope.metadata().coordinateSystemId}};else if(d.id=="point")v.data=p;
        else if(d.id=="found")v.data=true;else if(d.id=="scale" || d.id=="score")v.data=1.;else if(d.type==PortType::Number)v.data=0.;else if(d.id=="candidates")v.data=QString("[]");
        r.outputs[d.id]=v;
    }return r;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    for(bool cropped:{false,true})for(bool shape:{false,true})for(bool perspective:{false,true})for(bool yUp:{false,true})for(bool pose:{false,true}){
        cropMode=cropped;if(shape && !pose)continue;
        if(perspective && yUp)continue; // Homography contract uses target-local columns/rows.
        const CoordinateFrame world{"plane",LengthUnit::Millimetre,yUp ? AxisConvention::XRightYUp:AxisConvention::XRightYDown};
        auto toRaw=[&](QPointF p){if(yUp)p.setY(-p.y());return perspective ? project(p):p*4;};
        auto toWorld=[&](QPointF p){auto q=perspective ? inverse(p):p/4;if(yUp)q.setY(-q.y());return q;};
        CalibrationDependency setup{"cal",1,"pending","camera","lens",{256,256},"plane",raw,world};
        ChessboardObservation obs{{"board",7,7,5},{"camera","fit","fit-cycle",raw.id,metadata.completedAtUtc},{256,256},{},{}};
        for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);if(yUp)q.setY(-q.y());obs.worldMillimetres<<q;obs.pixels<<toRaw(q);}
        auto sample=[&](QString id,QPointF a,QPointF b){if(yUp){a.setY(-a.y());b.setY(-b.y());}return CalibrationSample{id,{toRaw(a),raw},{toRaw(b),raw},QLineF(a,b).length()};};
        auto cal=MeasurementService{}.validate((perspective ? MeasurementService{}.teachHomography(setup,PlaneMappingService::fit(obs)):MeasurementService{}.teach(setup,sample("teach",{1,1},{21,1}))),{sample("hx",{2,3},{27,3}),sample("hy",{3,2},{3,27})},1e-5);
        const MeasurementExecutionContext ctx{std::make_shared<PlanarCalibration>(cal),cal.dependency(),{256,256},{}};
        QByteArray model(256*256,char(30));for(int y=0;y<24;++y)for(int x=0;x<24;++x)model[(46+y)*256+51+x]=char(40+(x*37+y*71+x*y*13)%190);
        ToolDefinition source{"source",pose ? (shape ? "shape_pose":"pattern_pose"):"edge_point","Source","",true,defaultParameters(pose ? (shape ? "shape_pose":"pattern_pose"):"edge_point")};
        if(pose){const auto rect=QRect(51,46,24,24).translated(-cropOffset().toPoint());source.parameters=shape ? ShapeMatchService::teach(image(model),rect,{9,14},source.parameters):PoseMatchService::teach(image(model),rect,{9,14},source.parameters);}
        const QPointF p0=toRaw({15,yUp ? -15.:15.}),p=p0+QPointF(2,1),offset(1.25,-.75);
        const double a=pose ? .15:0,da=pose ? -.06:0;
        auto moved=[&](QPointF q){return p+rotate(rotate(toRaw(q)-p0,da)+offset,a);};
        auto nominalAt=[&](QPointF r){return toWorld(p0+rotate(rotate(r-p,-a)-offset,-da));};
        for(const QString kind:{"edge_point","edge_pair","line_fit","circle_fit"})for(bool arc:{false,true}){
            if(arc && kind!="circle_fit")continue;
            ToolDefinition t{kind,kind,kind,"",true,defaultParameters(kind)};
            auto wy=[&](double y){return yUp ? -y:y;};
            ToolRoi roi{world,ScanSegment{{15,wy(9)},{15,wy(21)},.2}};
            if(kind=="line_fit")roi.geometry=RotatedRectangle{{15,wy(15)},{12,12},0};
            if(kind=="circle_fit")roi.geometry=arc ? RoiGeometry(ArcRoi{{{15,wy(15)},1,5},30,-220}):RoiGeometry(Annulus{{15,wy(15)},1,5});
            t.parameters["minContrast"]=5.;t.parameters["polarity"]=kind=="edge_pair" ? "bright":"any";
            if(kind=="circle_fit"){t.parameters["scanWidth"]=.1;t.parameters["inlierDistance"]=.2;t.parameters["maxRms"]=.1;t.parameters["maxResidual"]=.2;t.parameters["maxGapDegrees"]=30.;}
            const auto centre=toWorld(moved({15,wy(15)}));
            QByteArray bytes(256*256,char(30));
            for(int y=0;y<256;++y)for(int x=0;x<256;++x){QPointF r(x,y);const auto q=nominalAt(r);double value;
                if(kind=="circle_fit")value=128+100*std::erf((QLineF(toWorld(r),centre).length()-3)/.4);
                else {double yy=yUp ? -q.y():q.y();value=kind=="edge_pair" ? 28+100*(std::erf((yy-13)/.5)-std::erf((yy-17)/.5)):128+100*std::erf((yy-13)/.5);}
                bytes[y*256+x]=char(std::lround(value));
            }
            const auto raster=image(bytes);const QString port=pose ? "pose":"point";
            t=ReferenceService::teachWorld(t,roi,.2,source,port,result(source,p0),raster,ctx,offset,da);
            const auto b=toolReferenceBinding(t);check(b.world && b.nominal.toJson()==roi.toJson(),"world nominal preserved exactly");
            check(ReferenceBinding::fromJson(b.toJson()).toJson()==b.toJson(),"world strict codec roundtrip");
            check(ReferenceService::teachWorld(t,roi,.2,source,port,result(source,p0),raster,ctx,offset,da).parameters==t.parameters,"reteach does not accumulate offsets");
            const auto mapped=ReferenceService::resolveWorld(b,source,result(source,p,a),raster,ctx);check(mapped.map.has_value(),"current world reference resolves");
            auto counted=*mapped.map;int taps=0;const auto original=counted.nominalToImage;
            counted.nominalToImage=[&](QPointF q){++taps;const auto r=original(q);near(r,moved(q)-cropOffset(),1e-5);return r;};
            const auto measured=WorldTrackingService::execute(t,raster,counted);check(taps>100 && measured.outputs["found"].valid && std::get<bool>(measured.outputs["found"].data),"every actual tap maps; real raster feature found");
            check(measured.decision==PartDecision::NotEvaluated,"no part decision manufactured");
            const auto overlay=toolOverlayGeometry(t,t,raster,ctx,{},mapped.map,measured);check(overlay.reason.isEmpty() && !overlay.outlines.isEmpty() && !overlay.guides.isEmpty(),"24H resolved world raw overlay evidence");
            const auto hidden=toolOverlayGeometry(t,t,raster,ctx,{},WorldTrackingMap{[](QPointF)->QPointF{throw std::runtime_error("domain rejected");},{},{},{},{}},measured);check(!hidden.reason.isEmpty() && hidden.outlines.isEmpty() && hidden.guides.isEmpty(),"24H mapping failure removes every primitive");
            if(kind=="edge_point")near(std::get<QPointF>(measured.outputs["point"].data),moved({15,wy(13)})-cropOffset(),.45);
            if(kind=="edge_pair"){const double expected=QLineF(toWorld(moved({15,wy(13)})),toWorld(moved({15,wy(17)}))).length();check(std::abs(std::get<double>(measured.outputs["widthMm"].data)-expected)<.2,"current world endpoint width oracle");}
            if(kind=="line_fit"){check(measured.outputs["lineMm"].valid && measured.outputs["accepted"].valid,"actual world line refit and tolerance");check(!measured.outputs["originPx"].valid,"unavailable raw fit stays unavailable");}
            if(kind=="circle_fit"){near(std::get<QPointF>(measured.outputs["centreMm"].data),centre,.15);check(std::abs(std::get<double>(measured.outputs["radiusMm"].data)-3)<.15,"current world circle refit oracle");check(measured.outputs["arcMm"].valid==arc,"signed arc current metric");auto producer=measured;for(auto& v:producer.outputs)v.evaluationId="current";const auto unavailable=ReferenceService::current(t,"centrePx",producer,raster);check(!unavailable.anchor && unavailable.state==ExecutionState::Skipped,"unavailable raw circle centre rejected as source");}
            if(kind=="edge_point"){
                auto rawConsumer=t;rawConsumer.parameters.remove("referenceBinding");rawConsumer.parameters["scanRoi"]=ToolRoi{{raster.metadata().coordinateSystemId},ScanSegment{{40,40},{80,40},1}}.toJson();
                rawConsumer=ReferenceService::teach(rawConsumer,source,port,result(source,p0),raster);
                reject([&]{WorldTrackingService::execute(rawConsumer,raster,*mapped.map);});reject([&]{WorldTrackingService::preflight(rawConsumer,raster,*mapped.map);});
            }
            auto changed=ctx;changed.current->version++;check(!ReferenceService::resolveWorld(b,source,result(source,p,a),raster,changed).map,"stale exact calibration rejected");
            check(!ReferenceService::resolveWorld(b,source,result(source,p,a),raster,ctx,std::make_shared<std::atomic_bool>(true)).map,"cancelled mapping rejected");
            auto missing=result(source,p,a);missing.outputs["found"].data=false;check(ReferenceService::resolveWorld(b,source,missing,raster,ctx).state==ExecutionState::Skipped,"not found has no fallback");
            auto outside=b;outside.offsetPx={200,0};const auto out=ReferenceService::resolveWorld(outside,source,result(source,p,a),raster,ctx);check(out.map.has_value(),"mapping validated lazily per tap");reject([&]{WorldTrackingService::execute(t,raster,*out.map);});
            auto extra=b.toJson();extra["extra"]=1;reject([&]{ReferenceBinding::fromJson(extra);});
        }
    }
    QTextStream(stdout)<<checks<<" reference world checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
