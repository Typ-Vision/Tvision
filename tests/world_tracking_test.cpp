#include "application/WorldTrackingService.hpp"
#include "application/MeasurementService.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "domain/PoseMatchConfig.hpp"
#include "domain/EdgePointConfig.hpp"
#include "domain/CircleConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <numbers>
#include <limits>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(QPointF a,QPointF b,double e){check(QLineF(a,b).length()<=e,"independent point oracle");}
template<class F>void reject(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"expected rejection");}
const CoordinateFrame raw{"camera/image"},local{"part/local"},world{"plane",LengthUnit::Millimetre,AxisConvention::XRightYDown};
FrameMetadata m{"camera","frame","cycle",raw.id,QDateTime::currentDateTimeUtc()};
ImageFrame image(const QByteArray& b){return ImageFrame::fromBuffer(b.data(),b.size(),256,256,256,PixelFormat::Gray8,m,false);}
ToolResult synthetic(const ToolDefinition& source,Pose2D pose){
    ToolResult r;r.toolId=source.id;r.state=ExecutionState::Done;
    for(const auto& d:outputPorts(source.kind)){
        PortValue v{d,false,true,m.frameId,m.cycleId,m};v.evaluationId="evaluation";v.method=source.kind=="shape_pose" ? "unsigned-oriented-chamfer-pyramid":"masked-ccoeff-normed-pyramid";v.methodVersion=1;
        if(d.id=="pose")v.data=pose;else if(d.id=="scale" || d.id=="score")v.data=1.;else if(d.id=="found")v.data=true;else if(d.id=="candidates")v.data=QString("[]");r.outputs[d.id]=v;
    }return r;
}
// Independent analytic projective fixture. Product mapping/warping is never
// used to generate raster pixels or expected transformed feature coordinates.
QPointF project(QPointF q){double w=1+.015*q.x()+.008*q.y();return {(4*q.x()+20)/w,(4*q.y()+20)/w};}
QPointF inverse(QPointF q){double s=3.54/(4-.015*q.x()-.008*q.y());return {(q.x()*s-20)/4,(q.y()*s-20)/4};}
QPointF rotate(QPointF q,double a){return {std::cos(a)*q.x()-std::sin(a)*q.y(),std::sin(a)*q.x()+std::cos(a)*q.y()};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    // Independent irregular world angles: nominal 30-degree spacing conceals
    // real 50-degree gaps, including a wrap through +/-180 degrees.
    TrackedWorldCircle angular;angular.observations.fullCircle=true;QVector<CircleFitPoint> angularPoints;
    for(int i=0;i<12;++i){const double degrees=170+(i/2)*60+(i%2)*10;const double a=degrees*std::numbers::pi/180.;const QPointF q(5*std::cos(a),5*std::sin(a));RadialScanPoint s;s.scanId=i;s.angleDegrees=i*30;s.point=q;angular.observations.scans.append(s);angularPoints.append({q});}
    angular.world=CircleFitter::fit(angularPoints);
    auto angularMetric=WorldTrackingService::metrics(angular,world,{6,.9,40});
    check(!angularMetric.metrics.deviation && std::abs(angularMetric.metrics.maxGapDegrees-50)<1e-8 && angularMetric.metrics.qualityReasons.contains("excessive_current_world_gap"),"actual gaps override nominal evenly spaced rays");
    angularMetric=WorldTrackingService::metrics(angular,world,{6,.9,50.000001});check(angularMetric.metrics.deviation && angularMetric.metrics.contour.metrics && !angularMetric.arc,"full circle wraps once and has no arc port");
    const auto outward=*angular.observations.scans[6].point*1.1;angular.observations.scans[6].point=outward;angularPoints[6].point=outward;CircleFitOptions robust;robust.inlierDistance=.1;robust.maxRms=.01;robust.maxResidual=.02;angular.world=CircleFitter::fit(angularPoints,robust);
    angularMetric=WorldTrackingService::metrics(angular,world,{6,.9,60});check(!angular.world.scans[6].used && angularMetric.metrics.observedDeviation.peakToValley>.49 && angularMetric.metrics.fitMaxResidual<.02,"radial defect includes actual outlier excluded from fit");
    ChessboardObservation obs{{"perspective",7,7,5},{m.sourceId,"fit","fit-cycle",m.coordinateSystemId,m.completedAtUtc},{256,256},{},{}};
    for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);obs.worldMillimetres<<q;obs.pixels<<project(q);}
    CalibrationDependency setup{"H",1,"pending",m.sourceId,"lens",{256,256},"plane",raw,world};
    auto sample=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{project(a),raw},{project(b),raw},QLineF(a,b).length()};};
    const auto cal=MeasurementService{}.validate(MeasurementService{}.teachHomography(setup,PlaneMappingService::fit(obs)),{sample("hx",{2,3},{27,3}),sample("hy",{3,2},{3,27})},1e-5);
    const MeasurementExecutionContext ctx{std::make_shared<PlanarCalibration>(cal),cal.dependency(),{256,256},{}};
    const Pose2D taught{project({15,15}),0,raw},current{taught.position+QPointF(2,1),.15,raw};
    auto movedRaw=[&](QPointF nominal){return current.position+rotate(project(nominal)-taught.position,.15);};
    auto nominalAtRaw=[&](QPointF p){return inverse(taught.position+rotate(p-current.position,-.15));};
    for(const QString kind:{"pattern_pose","shape_pose"}){
        QByteArray model(256*256,char(30));for(int y=0;y<24;++y)for(int x=0;x<24;++x)model[(46+y)*256+51+x]=char(40+(x*37+y*71+x*y*13)%190);
        ToolDefinition source{"pose",kind,"Pose","",true,poseMatchDefaults()};
        source.parameters=(kind=="shape_pose" ? ShapeMatchService::teach:PoseMatchService::teach)(image(model),{51,46,24,24},{9,14},source.parameters);
        auto binding=PoseRoi::teach({raw,Annulus{taught.position,1,3}},taught,local,source.id,PoseRoiService::geometryIdentity(source),{256,256},m.sourceId);binding.calibration=cal.dependency();
        auto result=synthetic(source,current);
        QByteArray stripe(256*256,char(30));
        for(int y=0;y<256;++y)for(int x=0;x<256;++x){const auto q=nominalAtRaw({double(x),double(y)});stripe[y*256+x]=char(std::lround(128+100*(std::erf((q.y()-13)/.5)-std::erf((q.y()-17)/.5)-1)));}
        const auto raster=image(stripe);const auto hash=raster.contentId();
        const auto map=PoseRoiService::worldMap(binding,source,result,raster,ctx);
        for(int y=6;y<25;++y)for(int x=6;x<25;++x){near(map.nominalToRaw({double(x),double(y)}),movedRaw({double(x),double(y)}),1e-5);near(map.rawToWorld(movedRaw({double(x),double(y)})),inverse(movedRaw({double(x),double(y)})),1e-5);}
        ToolRoi scan{world,ScanSegment{{15,9},{15,21},.2}};
        EdgeOptions edge;edge.profile={.5,1,1};edge.polarity=EdgePolarity::Rising;edge.minContrast=5;
        auto counted=map;int calls=0;counted.nominalToRaw=[&](QPointF p){++calls;const auto q=map.nominalToRaw(p);near(q,movedRaw(p),1e-5);return q;};
        check(WorldTrackingService::edge(raster,scan,.2,counted,edge).has_value() && calls>200,"every actual longitudinal/transverse tap mapped");
        ToolDefinition worldConsumer{"world-edge","edge_point","World edge","",true,edgePointDefaults()};
        worldConsumer.parameters["polarity"]="rising";worldConsumer.parameters["minContrast"]=5.;
        const auto taughtWorld=PoseRoiService::teachWorldTool(worldConsumer,scan,.2,source,synthetic(source,taught),raster,ctx);
        const auto worldBinding=worldPoseTracking(taughtWorld);
        const auto worldMap=PoseRoiService::worldMap(worldBinding,source,result,raster,ctx);
        const auto executedWorld=WorldTrackingService::execute(taughtWorld,raster,worldMap);
        check(executedWorld.outputs["point"].valid && !executedWorld.outputs["strength"].valid && executedWorld.decision==PartDecision::NotEvaluated,"non-affine teaching/executor emits only actual features");
        near(std::get<QPointF>(executedWorld.outputs["point"].data),movedRaw({15,13}),.3);
        for(const auto& key:worldBinding.toJson().keys()){
            auto edited=taughtWorld;auto envelope=worldBinding.toJson();envelope.remove(key);edited.parameters["poseTracking"]=envelope;reject([&]{ToolRegistry::validate(edited);});
        }
        auto invalidWorld=taughtWorld;auto envelope=worldBinding.toJson();envelope["mmPerSample"]=QString("0.2");invalidWorld.parameters["poseTracking"]=envelope;reject([&]{ToolRegistry::validate(invalidWorld);});
        invalidWorld=taughtWorld;envelope=worldBinding.toJson();envelope["mapping"]="raw-rigid-v1";invalidWorld.parameters["poseTracking"]=envelope;reject([&]{ToolRegistry::validate(invalidWorld);});
        reject([&]{PoseRoiService::teachWorldTool(worldConsumer,{world,ScanSegment{{-10,-10},{-10,10},1}},.2,source,synthetic(source,taught),raster,ctx);});
        const auto hit=WorldTrackingService::edge(raster,scan,.2,map,edge);check(hit.has_value(),"world edge found");near(hit->raw,movedRaw({15,13}),.3);near(hit->world,inverse(movedRaw({15,13})),.15);
        EdgePairOptions pair;pair.profile=edge.profile;pair.minContrast=5;
        const auto width=WorldTrackingService::pair(raster,scan,.2,map,pair);check(width.has_value(),"world stripe found");
        const auto wa=inverse(movedRaw({15,13})),wb=inverse(movedRaw({15,17}));near(width->first.world,wa,.15);near(width->second.world,wb,.15);check(std::abs(width->widthMm-QLineF(wa,wb).length())<.15,"width from independently mapped endpoints");
        LineScanOptions line;line.scanCount=15;line.scanWidthPx=1;line.edge=edge;line.fit.minSpan=5;line.fit.inlierDistance=.25;line.fit.maxRms=.1;line.fit.maxResidual=.2;
        const auto fitted=WorldTrackingService::line(raster,{world,RotatedRectangle{{15,15},{12,12},0}},.2,map,line);
        check(fitted.world.usedCount==15 && fitted.raw.size()==15,"world line full observations");
        const auto expectedA=inverse(movedRaw({9,13})),expectedB=inverse(movedRaw({21,13}));
        const auto expectedDirection=(expectedB-expectedA)/QLineF(expectedA,expectedB).length();
        check(QLineF(fitted.world.tangent,expectedDirection).length()<.02,"world line angle from current points");
        for(int i=0;i<fitted.raw.size();++i)near(*fitted.world.scans[i].point,inverse(*fitted.raw[i].point),1e-5);
        const auto centre=inverse(movedRaw({15,15}));
        QByteArray ring(256*256,char(30));for(int y=0;y<256;++y)for(int x=0;x<256;++x){const auto q=inverse({double(x),double(y)});ring[y*256+x]=char(std::lround(128+100*std::erf((QLineF(q,centre).length()-3)/.4)));}
        const auto circleImage=image(ring);const auto circleMap=PoseRoiService::worldMap(binding,source,result,circleImage,ctx);
        RadialScanOptions radial;radial.scanCount=32;radial.scanWidth=.1;radial.unitsPerSample=.2;radial.edge=edge;
        CircleFitOptions fit;fit.inlierDistance=.2;fit.maxRms=.1;fit.maxResidual=.2;
        const ToolRoi annulus{world,Annulus{{15,15},1,5}};
        const auto circle=WorldTrackingService::circle(circleImage,annulus,circleMap,radial,fit);
        near(circle.world.centre,centre,.1);check(std::abs(circle.world.radius-3)<.1,"actual current world radius refit");check(circle.observations.foundCount==32,"current world radial observations");
        for(const auto& s:circle.observations.scans)near(*s.point,inverse(*s.rawPoint),1e-5);
        auto arc=annulus;arc.geometry=ArcRoi{std::get<Annulus>(annulus.geometry),30,-220};
        const auto partial=WorldTrackingService::circle(circleImage,arc,circleMap,radial,fit);near(partial.world.centre,centre,.12);check(!partial.observations.fullCircle,"signed arc stays partial");
        for(const auto& nominal:{annulus,arc}){
            ToolDefinition consumer{"circle","circle_fit","World circle","",true,circleDefaults()};
            consumer.parameters["scanCount"]=32;consumer.parameters["scanWidth"]=.1;consumer.parameters["polarity"]="rising";consumer.parameters["minContrast"]=5.;consumer.parameters["inlierDistance"]=.2;consumer.parameters["maxRms"]=.1;consumer.parameters["maxResidual"]=.2;
            consumer=PoseRoiService::teachWorldTool(consumer,nominal,.2,source,synthetic(source,taught),circleImage,ctx);
            const auto job=PoseRoiService::worldMap(worldPoseTracking(consumer),source,result,circleImage,ctx);
            const auto measured=WorldTrackingService::execute(consumer,circleImage,job);
            check(measured.outputs["circleMm"].valid && measured.outputs["arcMm"].valid==!std::holds_alternative<Annulus>(nominal.geometry) && measured.outputs["accepted"].valid && !measured.outputs["circlePx"].valid,"perspective recipe emits actual world arc and tool tolerance, raw circle unavailable");
            near(std::get<Circle2D>(measured.outputs["circleMm"].data).centre,centre,.12);
            check(std::abs(std::get<double>(measured.outputs["radiusMm"].data)-3)<.12,"perspective recipe world circle refit oracle");
            check(measured.decision==PartDecision::NotEvaluated && measured.outputs["quality"].valid,"world metrics never grant a part decision");
            const auto actual=WorldTrackingService::metrics(std::holds_alternative<Annulus>(nominal.geometry) ? circle:partial,world,{6,.9,15});
            check(actual.metrics.deviation.has_value(),"actual angular quality accepted");
            if(actual.arc){
                const auto& a=*actual.arc;const auto first=*partial.observations.scans.first().point-centre;
                check(std::abs(a.startDegrees-std::atan2(first.y(),first.x())*180/std::numbers::pi)<1,"current arc start follows independent world observation");
                check(a.sweepDegrees<0 && std::abs(a.sweepDegrees+220)>1,"non-affine current arc sweep differs from nominal sweep");
            }
            auto limits=consumer;limits.parameters["diameterLower"]=std::get<double>(measured.outputs["diameterMm"].data);limits.parameters["diameterUpper"]=limits.parameters["diameterLower"];limits.parameters["deviationLower"]=std::get<double>(measured.outputs["deviationMm"].data);limits.parameters["deviationUpper"]=limits.parameters["deviationLower"];
            const auto inclusive=WorldTrackingService::execute(limits,circleImage,job);check(inclusive.outputs["accepted"].valid && std::get<bool>(inclusive.outputs["accepted"].data),"circle exact inclusive double tolerance");
            limits.parameters["diameterLower"]=0.;limits.parameters["diameterUpper"]=std::nextafter(std::get<double>(measured.outputs["diameterMm"].data),0.);
            check(!std::get<bool>(WorldTrackingService::execute(limits,circleImage,job).outputs["accepted"].data),"circle one ULP outside upper tolerance fails");
            limits=consumer;limits.parameters["maxGapDegrees"]=.1;
            const auto unqualified=WorldTrackingService::execute(limits,circleImage,job);check(unqualified.outputs["qualified"].valid && !std::get<bool>(unqualified.outputs["qualified"].data) && !unqualified.outputs["accepted"].valid && !unqualified.outputs["deviationMm"].valid && unqualified.outputs["observedDeviationMm"].valid,"quality failure retains diagnostics without tolerance");
        }
        auto missingPixels=ring;for(int y=0;y<256;++y)for(int x=0;x<256;++x){const auto q=nominalAtRaw({double(x),double(y)})-QPointF(15,15);if(std::abs(std::atan2(q.y(),q.x()))<.3)missingPixels[y*256+x]=char(128);}
        const auto missingImage=image(missingPixels);const auto missingMap=PoseRoiService::worldMap(binding,source,result,missingImage,ctx);
        const auto missing=WorldTrackingService::circle(missingImage,annulus,missingMap,radial,fit);check(missing.world.missingCount>0 && missing.world.usedCount>=24,"missing radial slots retained during world refit");near(missing.world.centre,centre,.12);
        const auto missingMetrics=WorldTrackingService::metrics(missing,world,{6,.9,15});check(!missingMetrics.metrics.contour.metrics && !missingMetrics.metrics.deviation,"missing samples cannot silently close or qualify a contour");
        auto missingEndpoint=partial;missingEndpoint.observations.scans.first().point.reset();missingEndpoint.world.scans.first().point.reset();missingEndpoint.world.scans.first().used=false;
        const auto noArc=WorldTrackingService::metrics(missingEndpoint,world,{6,.5,360});check(!noArc.arc && !noArc.metrics.deviation && noArc.metrics.qualityReasons.contains("missing_current_arc_endpoint"),"missing endpoint cannot invent current arc or tolerance");
        auto reversed=partial;std::swap(reversed.observations.scans[4].point,reversed.observations.scans[5].point);std::swap(reversed.world.scans[4].point,reversed.world.scans[5].point);
        const auto folded=WorldTrackingService::metrics(reversed,world,{6,.5,360});check(!folded.arc && !folded.metrics.deviation && folded.metrics.qualityReasons.contains("unordered_current_world_angles"),"folded observation order is not a current world arc");
        reject([&]{WorldTrackingService::metrics(partial,world,{6,.9,15},std::make_shared<std::atomic_bool>(true));});
        auto distortedPixels=ring;for(int y=0;y<256;++y)for(int x=0;x<256;++x){const auto q=nominalAtRaw({double(x),double(y)});distortedPixels[y*256+x]=char(std::lround(128+100*std::erf((QLineF(q,{15,15}).length()-3)/.4)));}
        const auto distortedImage=image(distortedPixels);const auto distortedMap=PoseRoiService::worldMap(binding,source,result,distortedImage,ctx);
        auto strict=fit;strict.maxRms=.0001;strict.maxResidual=.001;
        reject([&]{WorldTrackingService::circle(distortedImage,annulus,distortedMap,radial,strict);});
        check(std::abs(QLineF(inverse(movedRaw({18,15})),centre).length()-QLineF(inverse(movedRaw({15,18})),centre).length())>.01,"projected nominal circle has unequal current world radii");
        // Blank raster yields no edge/pair; fit never manufactures a feature.
        const auto blank=image(QByteArray(256*256,char(30)));const auto blankMap=PoseRoiService::worldMap(binding,source,result,blank,ctx);
        FitDiagnostic edgeFailure,pairFailure;check(!WorldTrackingService::edge(blank,scan,.2,blankMap,edge,{},&edgeFailure),"missing world edge");check(!WorldTrackingService::pair(blank,scan,.2,blankMap,pair,{},&pairFailure),"missing world pair");check(edgeFailure.code=="EDGE_NOT_FOUND" && edgeFailure.stage=="edge_search" && pairFailure.code=="PAIR_NOT_FOUND" && pairFailure.stage=="pair_search","world missing edge/pair shares diagnostic vocabulary");
        reject([&]{WorldTrackingService::line(blank,{world,RotatedRectangle{{15,15},{12,12},0}},.2,blankMap,line);});reject([&]{WorldTrackingService::circle(blank,annulus,blankMap,radial,fit);});
        auto invalid=ctx;++invalid.current->version;reject([&]{PoseRoiService::worldMap(binding,source,result,raster,invalid);});invalid=ctx;invalid.current->lensId="other";reject([&]{PoseRoiService::worldMap(binding,source,result,raster,invalid);});invalid=ctx;invalid.rawSize={128,128};reject([&]{PoseRoiService::worldMap(binding,source,result,raster,invalid);});
        auto noCalibration=binding;noCalibration.calibration.reset();reject([&]{PoseRoiService::worldMap(noCalibration,source,result,raster,ctx);});
        auto changedPixels=stripe;changedPixels[0]=char(99);reject([&]{WorldTrackingService::edge(image(changedPixels),scan,.2,map,edge);});
        auto changedMetadata=m;changedMetadata.cycleId="next-cycle";reject([&]{WorldTrackingService::edge(raster.withMetadata(changedMetadata),scan,.2,map,edge);});
        auto bad=result;bad.outputs["scale"].data=std::nextafter(1.,2.);reject([&]{PoseRoiService::worldMap(binding,source,bad,raster,ctx);});bad=result;bad.outputs["found"].data=false;reject([&]{PoseRoiService::worldMap(binding,source,bad,raster,ctx);});
        for(auto id:{"pose","scale","found","ambiguous","truncated"}){bad=result;bad.outputs[id].evaluationId="stale";reject([&]{PoseRoiService::worldMap(binding,source,bad,raster,ctx);});}
        auto wrong=scan;wrong.frame.axes=AxisConvention::XRightYUp;reject([&]{WorldTrackingService::edge(raster,wrong,.2,map,edge);});
        reject([&]{WorldTrackingService::edge(raster,scan,0,map,edge);});reject([&]{WorldTrackingService::edge(raster,scan,std::numeric_limits<double>::infinity(),map,edge);});
        reject([&]{WorldTrackingService::edge(raster,{world,ScanSegment{{31,10},{31,20},.1}},.2,map,edge);});
        // Centreline lies inside the nominal hull; transverse taps leave it.
        reject([&]{WorldTrackingService::edge(raster,{world,ScanSegment{{.1,10},{.1,20},1}},.2,map,edge);});
        auto outPose=current;outPose.position=project({29,15});const auto outResult=synthetic(source,outPose);const auto outMap=PoseRoiService::worldMap(binding,source,outResult,raster,ctx);
        reject([&]{WorldTrackingService::edge(raster,{world,ScanSegment{{15,9},{15,21},.2}},.2,outMap,edge);});
        // Caller edits cannot change an already created job snapshot.
        invalid=ctx;auto snapshot=PoseRoiService::worldMap(binding,source,result,raster,invalid);invalid.current->lensId="changed";near(snapshot.nominalToRaw({15,15}),movedRaw({15,15}),1e-5);
        auto cancel=std::make_shared<std::atomic_bool>(false);const auto cancellable=PoseRoiService::worldMap(binding,source,result,raster,ctx,cancel);cancel->store(true);reject([&]{WorldTrackingService::edge(raster,scan,.2,cancellable,edge);});reject([&]{WorldTrackingService::circle(circleImage,annulus,cancellable,radial,fit);});
        cancel->store(false);auto during=cancellable;int taps=0;during.nominalToRaw=[&](QPointF q){if(++taps==20)cancel->store(true);return cancellable.nominalToRaw(q);};reject([&]{WorldTrackingService::edge(raster,scan,.2,during,edge);});check(taps==20,"cancellation during sampling returns no partial feature");
        check(raster.contentId()==hash,"raw pixels immutable");
        // Actual pose producer -> uniform/Y-up world sampler. Synthetic pose
        // above isolates the non-affine sampling oracle from matcher accuracy.
        source.parameters["angleMin"]=0.;source.parameters["angleMax"]=90.;source.parameters["angleStep"]=90.;source.parameters["pyramidLevels"]=1;source.parameters["minScore"]=.95;
        QByteArray moved(256*256,char(30));for(int y=0;y<24;++y)for(int x=0;x<24;++x)moved[(120+x-9)*256+140-(y-14)]=model[(46+y)*256+51+x];
        for(int y=146;y<=154;++y)for(int x=128;x<=152;++x)moved[y*256+x]=char(std::lround(128+100*std::erf((140-x)/1.4)));
        auto run=[&](const ImageFrame& im){auto r=PoseMatchService::execute(source,{{"image",{{"image",PortType::Image,"px","image"},im,true,m.frameId,m.cycleId,m}}},m.frameId,m.cycleId,{});for(auto& v:r.outputs)v.evaluationId="actual";return r;};
        const auto actualImage=image(moved);auto teaching=run(image(model)),actual=run(actualImage);check(actual.outputs["pose"].valid,"real matcher reference selected");
        auto s=setup;s.calibrationId="scale";s.target.axes=AxisConvention::XRightYUp;
        auto scaleSample=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{a,raw},{b,raw},QLineF(a,b).length()*.1};};
        const auto scale=MeasurementService{}.validate(MeasurementService{}.teach(s,scaleSample("t",{0,0},{100,0})),{scaleSample("x",{10,20},{110,20}),scaleSample("y",{30,10},{30,110})},.001);
        MeasurementExecutionContext scaleCtx{std::make_shared<PlanarCalibration>(scale),scale.dependency(),{256,256},{}};
        auto taughtBinding=PoseRoiService::teach({raw,ScanSegment{{90,52},{90,68},2}},local,source,teaching,image(model));taughtBinding.calibration=scale.dependency();
        const auto actualMap=PoseRoiService::worldMap(taughtBinding,source,actual,actualImage,scaleCtx);
        EdgeOptions actualEdge;actualEdge.polarity=EdgePolarity::Rising;actualEdge.profile.stepPx=.25;
        const auto actualHit=WorldTrackingService::edge(actualImage,{s.target,ScanSegment{{9,-5.2},{9,-6.8},.2}},.1,actualMap,actualEdge);check(actualHit.has_value(),"real matcher world edge selected");near(actualHit->world,{14,-15},.15);
    }
    QTextStream(stdout)<<checks<<" world tracking checks passed; software fixtures only\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
