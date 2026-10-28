#include "application/CalibratedImageGeometry.hpp"
#include "application/ToolExecutors.hpp"
#include "application/ReferenceService.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "application/WorldTrackingService.hpp"
#include "application/MeasurementService.hpp"
#include "application/ProductionResultCodec.hpp"
#include "application/ToolResultCodec.hpp"
#include "vision/metrology/ProfileSampler.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <cmath>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const QString& s){++checks;if(!b)throw std::runtime_error(s.toStdString());}
void near(QPointF a,QPointF b){check(QLineF(a,b).length()<1e-5,"independent subpixel oracle");}
template<class F>void reject(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"expected rejection");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 const FrameMetadata metadata{"camera","frame","cycle","camera/image",QDateTime::currentDateTimeUtc()};const CoordinateFrame raw{metadata.coordinateSystemId};
 QByteArray bytes(256*256,char(20));for(int y=0;y<256;++y)for(int x=110;x<170;++x)bytes[y*256+x]=char(220);
 for(int y=40;y<72;++y)for(int x=50;x<82;++x)bytes[y*256+x]=char(25+((x-50)*13+(y-40)*29+(x-50)*(y-40)*7)%70);
 const auto root=ImageFrame::fromBuffer(bytes.data(),bytes.size(),256,256,256,PixelFormat::Gray8,metadata,false);
 const auto child=ImageAccess::cropped(ImageAccess::cropped(root,{20,10,220,230},false),{10,10,190,190},false);
 for(bool perspective:{false,true})for(bool up:{false,true}){
  if(perspective && up)continue;
  const CoordinateFrame world{"plane",LengthUnit::Millimetre,up ? AxisConvention::XRightYUp:AxisConvention::XRightYDown};
  CalibrationDependency d{"cal",1,"pending","camera","lens",{256,256},"plane",raw,world};
  auto project=[](QPointF q){const double w=1+.003*q.x()+.002*q.y();return QPointF((5*q.x()+10)/w,(5*q.y()+10)/w);};
  auto inverse=[](QPointF q){const double w=4.95/(5-.003*q.x()-.002*q.y());return QPointF((q.x()*w-10)/5,(q.y()*w-10)/5);};
  auto toWorld=[&](QPointF q){if(perspective)return inverse(q);q*=.2;if(up)q.setY(-q.y());return q;};
  auto sample=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{a,raw},{b,raw},QLineF(toWorld(a),toWorld(b)).length()};};
  auto cal=[&]{if(!perspective)return MeasurementService{}.teach(d,sample("teach",{10,10},{110,10}));ChessboardObservation o{{"board",7,7,5},{"camera","fit","fit-cycle",raw.id,metadata.completedAtUtc},{256,256},{},{}};for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);o.worldMillimetres<<q;o.pixels<<project(q);}return MeasurementService{}.teachHomography(d,PlaneMappingService::fit(o));}();
  cal=MeasurementService{}.validate(cal,{sample("hx",{40,42},{100,42}),sample("hy",{42,40},{42,100})},1e-5);
  MeasurementExecutionContext ctx{std::make_shared<PlanarCalibration>(cal),cal.dependency(),root.size(),{}};
  const auto map=CalibratedImageGeometry::bind(child,ctx);const QPointF p(40.25,35.75),rawPoint=p+QPointF(30,20);near(map.imageToWorld(p),toWorld(rawPoint));near(map.worldToImage(toWorld(rawPoint)),p);
  near(map.worldToRoot(toWorld(rawPoint)),rawPoint);map.requireImage(child);
  reject([&]{map.imageToWorld({-1,10});});reject([&]{map.worldToImage(toWorld({25,45}));});
  auto wrong=ctx;wrong.rawSize=child.size();reject([&]{CalibratedImageGeometry::bind(child,wrong);});wrong=ctx;wrong.current->lensId="changed";reject([&]{CalibratedImageGeometry::bind(child,wrong);});wrong=ctx;wrong.current->workingPlaneId="changed";reject([&]{CalibratedImageGeometry::bind(child,wrong);});
  auto foreignStamp=root.metadata();foreignStamp.sourceId="other";foreignStamp.coordinateSystemId="other/image";const auto foreign=ImageAccess::cropped(root.withMetadata(foreignStamp),{30,20,190,190},false);reject([&]{CalibratedImageGeometry::bind(foreign,ctx);});
  auto stamp=child.metadata();stamp.deviceClockId="changed";reject([&]{map.requireImage(child.withMetadata(stamp));});
  auto cancel=std::make_shared<std::atomic_bool>(true);reject([&]{CalibratedImageGeometry::bind(child,ctx,cancel);});
  // Point-only measurement must convert pixels, not merely rename their frame.
  ToolResult a;a.state=ExecutionState::Done;a.outputs["p"]={{"p",PortType::Point,"px","image"},p,true,metadata.frameId,metadata.cycleId,child.metadata()};a.outputs["p"].imageGeometry=child.geometry();auto b=a;b.outputs["p"].data=p+QPointF(20,0);
  const auto distance=MeasurementService{}.distance(a,"p",b,"p",cal,cal.dependency(),root.size(),0,1e6);check(distance.state==ExecutionState::Done,distance.error);check(std::abs(*distance.millimetres-QLineF(toWorld(rawPoint),toWorld(rawPoint+QPointF(20,0))).length())<1e-5,"perspective measurement oracle");
  ToolDefinition calculate{"calculate","calculate","Calculate","",true,defaultParameters("calculate")};auto badPoint=a.outputs["p"];badPoint.origin.deviceClockId="other-clock";reject([&]{ToolExecutors::get("calculate")(calculate,{{"p1",a.outputs["p"]},{"p2",badPoint}},{},metadata.frameId,metadata.cycleId,{});});
  b.outputs["p"].imageGeometry.reset();check(MeasurementService{}.distance(a,"p",b,"p",cal,cal.dependency(),root.size(),0,1e6).state==ExecutionState::Error,"mixed point scopes rejected");
  auto encoded=ToolResultCodec::encode(a.outputs["p"]);check(ToolResultCodec::decode(encoded).imageGeometry==a.outputs["p"].imageGeometry,"typed child trace roundtrip");auto broken=encoded;broken.remove("imageGeometry");reject([&]{ToolResultCodec::decode(broken);});broken=encoded;auto scope=broken["imageGeometry"].toObject();scope["offsetX"]=31;broken["imageGeometry"]=scope;reject([&]{ToolResultCodec::decode(broken);});
  if(perspective){reject([&]{map.imageToWorld({180,180});});reject([&]{map.requireSamplingFootprint(project({29.99,18})-QPointF(30,20));});}
  // Real crop edge producer, raw reference, then world translation tracking.
  ToolDefinition source{"edge","edge_point","Edge","",true,defaultParameters("edge_point")};source.parameters["scanRoi"]=ToolRoi{{child.metadata().coordinateSystemId},ScanSegment{{50,70},{150,70},4}}.toJson();source.parameters["polarity"]="rising";
  PortValue image{{"image",PortType::Image,"px","image"},child,true,metadata.frameId,metadata.cycleId,child.metadata()};image.imageGeometry=child.geometry();
  auto result=ToolExecutors::get("edge_point",ctx)(source,{{"image",image}},{},metadata.frameId,metadata.cycleId,{});for(auto& v:result.outputs){v.evaluationId="evaluation";}
  check(result.outputs["point"].valid,"actual crop edge reference point");
  ToolDefinition consumer{"consumer","edge_point","Consumer","",true,defaultParameters("edge_point")};consumer.parameters["scanRoi"]=source.parameters["scanRoi"];
  const auto taught=ReferenceService::teach(consumer,source,"point",result,child);const auto binding=toolReferenceBinding(taught);check(binding.imageGeometry==child.geometry(),"raw reference teaching scope");check(ReferenceBinding::fromJson(binding.toJson()).toJson()==binding.toJson(),"raw reference scope codec");check(ReferenceService::resolve(binding,source,result,root).state==ExecutionState::Error,"root/child reference mismatch");
  const ToolRoi nominal{world,ScanSegment{toWorld({80,90}),toWorld({130,90}),.1}};
  auto tracked=ReferenceService::teachWorld(consumer,nominal,.2,source,"point",result,child,ctx);const auto wb=toolReferenceBinding(tracked);check(wb.rawSize==root.size() && wb.imageGeometry==child.geometry(),"world root size and crop scope distinct");
  const auto resolved=ReferenceService::resolveWorld(wb,source,result,child,ctx);check(resolved.map.has_value(),resolved.reason);near(resolved.map->nominalToImage(toWorld({100,90})),{70,70});
  const auto actual=WorldTrackingService::execute(tracked,child,*resolved.map);check(actual.outputs["found"].valid && std::get<bool>(actual.outputs["found"].data),"actual crop world reference edge");
  for(const QString poseKind:{"pattern_pose","shape_pose"}){
   ToolDefinition pose{"pose",poseKind,"Pose","",true,defaultParameters(poseKind)};pose.parameters["angleMin"]=0;pose.parameters["angleMax"]=0;pose.parameters["pyramidLevels"]=1;pose.parameters["selection"]="best";
   pose.parameters=poseKind=="shape_pose" ? ShapeMatchService::teach(child,{20,20,32,32},{12,12},pose.parameters):PoseMatchService::teach(child,{20,20,32,32},{12,12},pose.parameters);
   auto poseResult=ToolExecutors::get(poseKind)(pose,{{"image",image}},{},metadata.frameId,metadata.cycleId,{});for(auto& v:poseResult.outputs)v.evaluationId="pose-evaluation";check(poseResult.outputs["pose"].valid,"actual crop pose anchor");
   auto worldPose=PoseRoiService::teachWorldTool(consumer,nominal,.2,pose,poseResult,child,ctx);const auto pb=worldPoseTracking(worldPose);check(pb.rawSize==root.size() && pb.imageGeometry==child.geometry(),"world pose root teaching scope");check(WorldPoseTracking::fromJson(pb.toJson()).toJson()==pb.toJson(),"world pose scoped codec");
   auto legacyPose=PoseRoiService::teach(ToolRoi{{child.metadata().coordinateSystemId},ScanSegment{{50,70},{100,70},2}},{"legacy/local"},pose,poseResult,child);legacyPose.calibration=cal.dependency();legacyPose.validate();
   check(PoseRoi::fromJson(legacyPose.toJson()).toJson()==legacyPose.toJson(),"calibrated crop PoseRoi codec");const auto legacyMap=PoseRoiService::worldMap(legacyPose,pose,poseResult,child,ctx);near(legacyMap.nominalToImage(toWorld({100,90})),{70,70});
   const auto poseMap=PoseRoiService::resolveWorld(pb,pose,poseResult,child,ctx);check(poseMap.map.has_value(),poseMap.reason);const auto measured=WorldTrackingService::execute(worldPose,child,*poseMap.map);check(measured.outputs["found"].valid && std::get<bool>(measured.outputs["found"].data),"actual world pose crop sampling");
  }

 }
 QTextStream(stdout)<<checks<<" crop calibration checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
