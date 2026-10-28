#include "application/RoiService.hpp"
#include "application/MatchService.hpp"
#include "application/MeasurementService.hpp"
#include "application/ReferenceService.hpp"
#include "application/RuntimeService.hpp"
#include "application/RuntimeOverlay.hpp"
#include <QTemporaryDir>
#include "application/ToolExecutors.hpp"
#include "application/BackendCapabilities.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "application/ToolResultCodec.hpp"
#include "application/ProductionResultCodec.hpp"
#include "vision/Preprocessing.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <QSet>
#include <QUuid>
#include <opencv2/imgproc.hpp>
#include <cmath>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const QString& s){++checks;if(!b)throw std::runtime_error(s.toStdString());}
ToolDefinition tool(QString id,QString kind){ToolDefinition t{QUuid::createUuidV5(QUuid("12345678-1234-1234-1234-123456789abc"),id.toUtf8()).toString(QUuid::WithoutBraces),kind,id,"",true,defaultParameters(kind)};const auto b=ToolRegistry::get(kind).requiredBackend;if(!b.isEmpty()){const auto c=BackendCapabilities::get(b);check(c.available,c.reason);t.dependencies.append({ToolDependencyKind::Backend,b,c.version,c.contentId,{}});}return t;}
const ToolResult& result(const CycleResult& c,QString id){for(const auto& r:c.tools)if(r.toolId==id)return r;throw std::runtime_error("missing result");}
ImageFrame frame(cv::Mat m){return ImageFrame::fromBuffer(m.data,m.step*m.rows,m.cols,m.rows,m.step,PixelFormat::Gray8,{tool("source","image_file").id,"frame","cycle",tool("source","image_file").id+"/image",QDateTime::currentDateTimeUtc()},false);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 const QSet<QString> kinds{"roi","preprocess","template_match","blob","edge","code_read","pattern_pose","shape_pose","edge_point","edge_pair","line_fit","circle_fit","geometry_measurement"};
 QSet<QString> actual;for(const auto& d:ToolRegistry::entries())for(const auto& p:d.inputs)if(p.type==PortType::Image)actual.insert(d.kind);check(actual==kinds,"registry consumer matrix changed");
 for(const auto& kind:kinds)for(int mode=0;mode<4;++mode){
  QTextStream(stderr)<<kind<<" mode "<<mode<<"\n";
  cv::Mat pixels(300,320,CV_8U,cv::Scalar(20));
  if(kind=="circle_fit")for(int y=0;y<pixels.rows;++y)for(int x=0;x<pixels.cols;++x)pixels.at<uchar>(y,x)=uchar(std::lround(128+100*std::erf((std::hypot(x-160.,y-152.)-40)/1.5)));
  else if(kind=="pattern_pose" || kind=="shape_pose" || kind=="template_match"){
   cv::rectangle(pixels,{116,105,13,33},cv::Scalar(220),-1);cv::circle(pixels,{144,115},8,cv::Scalar(140),-1);cv::line(pixels,{133,131},{153,140},cv::Scalar(245),3);
  }else if(kind=="code_read"){
   check(argc==2,"code fixture directory");QImage qr(QDir(argv[1]).filePath("qr.pgm"));check(!qr.isNull(),"real QR fixture required");qr=qr.convertToFormat(QImage::Format_Grayscale8);qr=qr.scaled(qr.width()/2,qr.height()/2,Qt::IgnoreAspectRatio,Qt::FastTransformation);check(qr.width()<=240 && qr.height()<=240,"QR fixture size");pixels.setTo(255);for(int y=0;y<qr.height();++y)for(int x=0;x<qr.width();++x)pixels.at<uchar>(y+30,x+40)=qr.constScanLine(y)[x];
  }else cv::rectangle(pixels,{127,0,60,300},cv::Scalar(220),-1);
  for(int y=40;y<72;++y)for(int x=240;x<272;++x)pixels.at<uchar>(y,x)=uchar(25+((x-240)*13+(y-40)*29+(x-240)*(y-40)*7)%70);
  auto root=frame(pixels),input=root;QVector<ToolDefinition> tools{tool("source","image_file")};QVector<ToolConnection> links;QString upstream=tools.first().id;
  if(mode==2){auto pre=tool("before","preprocess");tools<<pre;links.append({upstream,"image",pre.id,"image"});upstream=pre.id;input=Preprocessing::run(input,pre.parameters,{});}
  if(mode==3){auto outer=tool("outer","roi");outer.parameters=RoiService::teach(input,{16,12,288,280},true);tools<<outer;links.append({upstream,"image",outer.id,"image"});upstream=outer.id;input=ImageAccess::cropped(input,{16,12,288,280},false);}
  auto crop=tool("crop","roi");const QRect rect=mode==3 ? QRect(16,12,256,256):QRect(32,24,256,256);crop.parameters=RoiService::teach(input,rect,mode!=0);tools<<crop;links.append({upstream,"image",crop.id,"image"});upstream=crop.id;if(mode)input=ImageAccess::cropped(input,rect,false);
  if(mode==3){auto pre=tool("after","preprocess");tools<<pre;links.append({upstream,"image",pre.id,"image"});upstream=pre.id;input=Preprocessing::run(input,pre.parameters,{});}
  auto consumer=tool("consumer",kind);const QPointF off=input.geometry().rootOffset();const CoordinateFrame local{input.metadata().coordinateSystemId};
  auto q=[&](double x,double y){return QPointF(x,y)-off;};
  if(kind=="roi")consumer.parameters=RoiService::teach(input,{10,10,64,64},true);
  if(kind=="template_match"){const QRect r=QRect(112,100,48,48).translated(-off.toPoint());consumer.parameters["modelRoi"]=MatchService::rectangle(r);consumer.parameters["modelPng"]=MatchService{}.teach(input,r);}
  if(kind=="pattern_pose" || kind=="shape_pose"){
   consumer.parameters["angleMin"]=0;consumer.parameters["angleMax"]=0;consumer.parameters["pyramidLevels"]=1;consumer.parameters["selection"]="best";
   const auto r=QRect(112,100,48,48).translated(-off.toPoint());consumer.parameters=kind=="shape_pose" ? ShapeMatchService::teach(input,r,{12,20},consumer.parameters):PoseMatchService::teach(input,r,{12,20},consumer.parameters);
  }
  if(kind=="edge_point" || kind=="edge_pair")consumer.parameters["scanRoi"]=ToolRoi{local,ScanSegment{q(90,150),q(220,150),4}}.toJson();
  if(kind=="line_fit")consumer.parameters["scanRoi"]=ToolRoi{local,RotatedRectangle{q(145,150),{110,90},90}}.toJson();
  if(kind=="circle_fit"){consumer.parameters["mode"]="pixel";consumer.parameters["scanRoi"]=ToolRoi{local,Annulus{q(160,152),34,46}}.toJson();consumer.parameters["polarity"]="rising";}
  if(kind=="geometry_measurement"){
   consumer.parameters["mode"]="pixel";
   auto edge=tool("point","edge_point");edge.parameters["scanRoi"]=ToolRoi{local,ScanSegment{q(90,150),q(220,150),4}}.toJson();tools<<edge;links.append({upstream,"image",edge.id,"image"});links.append({edge.id,"point",consumer.id,"p1"});links.append({edge.id,"point",consumer.id,"p2"});
  }
  const bool referenceCapable=supportsReference(kind);
  if(referenceCapable && (kind=="blob" || kind=="edge" || kind=="code_read"))consumer.parameters["searchRoi"]=MatchService::rectangle(QRect(10,10,input.size().width()-20,input.size().height()-20));
  auto ref=tool("reference","edge_point");
  if(referenceCapable){ref.parameters["scanRoi"]=ToolRoi{local,ScanSegment{q(50,kind=="code_read" ? 80:150),q(230,kind=="code_read" ? 80:150),2}}.toJson();ref.parameters["minContrast"]=1.;tools<<ref;links.append({upstream,"image",ref.id,"image"});}
  auto pose=tool("pose",mode%2 ? "shape_pose":"pattern_pose");
  if(referenceCapable && kind!="edge"){
   pose.parameters["minScore"]=.1;pose.parameters["angleMin"]=0;pose.parameters["angleMax"]=0;pose.parameters["pyramidLevels"]=1;pose.parameters["selection"]="best";
   const auto rect=QRect(240,40,32,32).translated(-off.toPoint());pose.parameters=pose.kind=="shape_pose" ? ShapeMatchService::teach(input,rect,{12,12},pose.parameters):PoseMatchService::teach(input,rect,{12,12},pose.parameters);
   tools<<pose;links.append({upstream,"image",pose.id,"image"});
  }
  tools<<consumer;links.append({upstream,"image",consumer.id,"image"});auto definition=withEditorConnections(withEditorTools({},tools),links);
  const auto cycle=ExecutionService{}.run(definition,{},"recipe","config",{}, {},root);const auto& r=result(cycle,consumer.id);
  check(cycle.state==ExecutionState::Done,kind+" mode "+QString::number(mode)+": "+r.message+" "+cycle.error);
  check(!r.outputs.isEmpty() && r.decision==PartDecision::NotEvaluated,"real output without part decision");
  for(const auto& v:r.outputs)if(v.valid){check(v.origin.sourceId==tools.first().id && v.frameId==cycle.frameId && v.cycleId==cycle.cycleId,"actual acquisition provenance");check(v.imageGeometry.has_value(),"consumer geometry scope");if(v.port.type!=PortType::Image){const auto round=ToolResultCodec::decode(ToolResultCodec::encode(v));check(!mode || round.imageGeometry.has_value(),"child trace roundtrip");}}
  if(referenceCapable){
   const auto current=std::get<ImageFrame>(result(cycle,upstream).outputs["image"].data);
   check(result(cycle,ref.id).outputs["point"].valid,"real local reference producer");
   auto bound=ReferenceService::teach(consumer,ref,"point",result(cycle,ref.id),current);
   auto configured=tools;configured.last()=bound;auto trackedLinks=links;trackedLinks.append({ref.id,"point",bound.id,"reference"});
   const auto tracked=ExecutionService{}.run(withEditorConnections(withEditorTools({},configured),trackedLinks),{},"recipe","config",{}, {},root);
   check(tracked.state==ExecutionState::Done,kind+" crop raw reference: "+result(tracked,bound.id).message);
   if(kind!="edge"){
    const auto& poseResult=result(cycle,pose.id);check(poseResult.outputs["pose"].valid,"real crop pose producer");
    for(bool unified:{false,true}){
     auto rigid=unified ? ReferenceService::teach(consumer,pose,"pose",poseResult,current):PoseRoiService::teachTool(consumer,{"tracking/local"},pose,poseResult,current);
     configured=tools;configured.last()=rigid;trackedLinks=links;trackedLinks.append({pose.id,"pose",rigid.id,"poseReference"});
     const auto rigidCycle=ExecutionService{}.run(withEditorConnections(withEditorTools({},configured),trackedLinks),{},"recipe","config",{}, {},root);
     check(rigidCycle.state==ExecutionState::Done,kind+" crop rigid reference: "+result(rigidCycle,rigid.id).message);
    }
   }
  }
  if(QStringList{"edge_point","edge_pair","line_fit","circle_fit","geometry_measurement"}.contains(kind)){
   const CoordinateFrame raw{root.metadata().coordinateSystemId},world{"plane",LengthUnit::Millimetre,AxisConvention::XRightYDown};
   CalibrationDependency d{"scale",1,"pending",root.metadata().sourceId,"lens",root.size(),"plane",raw,world};
   auto sample=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{a,raw},{b,raw},QLineF(a,b).length()*.2};};
   const auto cal=MeasurementService{}.validate(MeasurementService{}.teach(d,sample("teach",{10,10},{110,10})),{sample("hx",{20,20},{120,20}),sample("hy",{30,30},{30,130})},1e-5);
   const MeasurementExecutionContext ctx{std::make_shared<PlanarCalibration>(cal),cal.dependency(),root.size(),{}};
   auto calibrated=consumer;if(kind!="edge_point")calibrated.parameters["mode"]="calibrated";
   if(kind=="circle_fit"){calibrated.parameters["scanRoi"]=ToolRoi{world,Annulus{{32,30.4},6.8,9.2}}.toJson();calibrated.parameters["unitsPerSample"]=.2;calibrated.parameters["scanWidth"]=.2;}
   auto calibratedTools=tools;calibratedTools.last()=calibrated;auto calibratedLinks=links;
   if(kind=="edge_point")for(const QString downstreamKind:{"calculate","measurement"}){auto downstream=tool(downstreamKind,downstreamKind);calibratedTools<<downstream;calibratedLinks.append({consumer.id,"point",downstream.id,"p1"});calibratedLinks.append({consumer.id,"point",downstream.id,"p2"});}
   const auto calibratedCycle=ExecutionService{}.run(withEditorConnections(withEditorTools({},calibratedTools),calibratedLinks),{},"recipe","config",{}, {},root,[ctx](const ImageFrame&){return ctx;});
   const auto& measured=result(calibratedCycle,consumer.id);check(calibratedCycle.state==ExecutionState::Done,kind+" calibrated crop: "+measured.message+calibratedCycle.error);
   const QString metric=kind=="edge_point" ? "pointMm":kind=="edge_pair" ? "widthMm":kind=="line_fit" ? "lineMm":kind=="circle_fit" ? "diameterMm":"valueMm";check(measured.outputs[metric].valid,"real calibrated crop metric");
   if(kind=="edge_pair")check(std::abs(std::get<double>(measured.outputs[metric].data)-12)<.03,"crop physical stripe oracle");
   if(kind=="circle_fit")check(std::abs(std::get<double>(measured.outputs[metric].data)-16)<.08,"crop physical circle oracle");
   if(kind=="edge_point")for(const QString downstreamKind:{"calculate","measurement"}){const auto& downstream=result(calibratedCycle,tool(downstreamKind,downstreamKind).id);check(downstream.state==ExecutionState::Done && downstream.outputs.first().imageGeometry==input.geometry(),"point-only downstream retains crop scope");}
  }
  if(kind=="code_read"){const auto codes=std::get<CodeResults>(r.outputs["codes"].data);check(codes.items.size()==1 && codes.items[0].text=="VISIONFLOW-T0.2","real crop QR read");for(auto p:codes.items[0].corners)input.geometry().toRoot(p);}
  if(kind=="blob")check(std::get<double>(r.outputs["count"].data)==1,"real one component");
  if(kind=="blob" && mode==3){
   QTemporaryDir dir(QDir::currentPath()+"/crop-runtime-XXXXXX");check(dir.isValid(),"runtime fixture directory");
   QImage file(pixels.data,pixels.cols,pixels.rows,int(pixels.step),QImage::Format_Grayscale8);const auto path=dir.filePath("source.png");check(file.save(path),"runtime exact raster");
   auto runtimeTools=tools;auto runtimeLinks=links;
   runtimeTools.erase(std::remove_if(runtimeTools.begin(),runtimeTools.end(),[&](const auto& t){return t.id==ref.id || t.id==pose.id;}),runtimeTools.end());
   runtimeLinks.erase(std::remove_if(runtimeLinks.begin(),runtimeLinks.end(),[&](const auto& l){return l.targetTool==ref.id || l.targetTool==pose.id;}),runtimeLinks.end());
   runtimeTools.first().parameters["path"]=path;
   ProjectService project;project.createProject("Crop runtime","",dir.filePath("part.vfproject"),"file");project.createRecipe("Crop","",ApplicationType::DefectInspection,"file");project.save();auto saved=*project.activeRecipe();saved.definition=withEditorConnections(withEditorTools({},runtimeTools),runtimeLinks);
   RuntimeService runtime(dir.filePath("archive"),project.path()+".calibrations.json",RuntimeIdentity::localEngineer());runtime.prepare(saved,dir.path());const auto lease=runtime.begin();
   auto edited=runtimeTools;edited[1].parameters["cropImage"]=false;saved.definition=withEditorConnections(withEditorTools({},edited),runtimeLinks);
   const auto completed=RuntimeService::execute(lease,project.path()+".calibrations.json",{});check(completed.state==ExecutionState::Done,"immutable crop Runtime");const auto& measured=result(completed,consumer.id);check(measured.outputs["point"].imageGeometry && measured.outputs["point"].imageGeometry->cropDepth()==2,"frozen nested crop scope");
   const auto selected=std::get<ImageFrame>(result(completed,upstream).outputs["image"].data),rawFrame=std::get<ImageFrame>(result(completed,tools.first().id).outputs["image"].data);const auto graph=validateGraph(runtimeTools,runtimeLinks);
   check(runtimeOverlays(completed,runtimeTools,graph,rawFrame,runtime.active(),consumer.id).layers.isEmpty(),"child overlay hidden on root raster");check(!runtimeOverlays(completed,runtimeTools,graph,selected,runtime.active(),consumer.id).layers.isEmpty(),"child overlay on matching raster");
   runtime.finish(lease.ticket,completed);const auto record=runtime.records().first();ProductionResultCodec::validate(record);check(ProductionStore(dir.filePath("archive")).recent("cycles").first()==record,"nested child terminal replay after restart");check(ProductionResultCodec::csv({record}).contains("roi-crop/"),"CSV retains child geometry evidence");
  }

  if(kind=="template_match")check(std::get<double>(r.outputs["score"].data)>.99,"real NCC score");
  if(kind=="pattern_pose" || kind=="shape_pose")check(r.outputs["pose"].valid && std::get<bool>(r.outputs["found"].data),"real pose found");
  if(kind=="edge_point")check(std::abs(std::get<QPointF>(r.outputs["point"].data).x()-(126.5-off.x()))<.1,"local edge pixel oracle");
  if(kind=="edge_pair")check(std::abs(std::get<double>(r.outputs["widthPx"].data)-60)<.1,"real stripe width");
  if(kind=="circle_fit")check(std::abs(std::get<double>(r.outputs["diameterPx"].data)-80)<.4,"real circle diameter");
  if(kind=="geometry_measurement")check(std::get<double>(r.outputs["valuePx"].data)==0,"real identical point distance");
 }
 {
  QTemporaryDir dir(QDir::currentPath()+"/crop-teaching-XXXXXX");check(dir.isValid(),"teaching lifecycle directory");QImage pixels(128,128,QImage::Format_Grayscale8);pixels.fill(20);for(int y=40;y<72;++y)for(int x=40;x<72;++x)pixels.scanLine(y)[x]=uchar(30+(x*13+y*29+x*y*7)%200);const auto path=dir.filePath("teaching.png");check(pixels.save(path),"teaching lifecycle raster");
  ProjectService p;p.createProject("Teaching","",dir.filePath("part.vfproject"),"file");p.createRecipe("Teaching","",ApplicationType::DefectInspection,"file");const auto source=p.setImageFile(path),roi=p.addTool("roi"),match=p.addTool("pattern_pose"),blob=p.addTool("blob");
  QVector<ToolConnection> links{{source,"image",roi,"image"},{roi,"image",match,"image"},{roi,"image",blob,"image"}};p.setConnections(links);
  auto get=[&](QString id){for(const auto& t:p.tools())if(t.id==id)return t;throw std::runtime_error("missing teaching tool");};auto run=[&]{return ExecutionService{}.run(p.activeRecipe()->definition,dir.path(),p.activeRecipe()->id,"teaching",{});};
  auto cycle=run();const auto raw=std::get<ImageFrame>(result(cycle,source).outputs["image"].data);auto cropped=get(roi);cropped.parameters=RoiService::teach(raw,{10,10,100,100},true);p.updateTool(cropped);cycle=run();const auto image=std::get<ImageFrame>(result(cycle,roi).outputs["image"].data);
  auto model=get(match);model.parameters["angleMin"]=0;model.parameters["angleMax"]=0;model.parameters["selection"]="best";model.parameters=PoseMatchService::teach(image,{30,30,32,32},{12,12},model.parameters);p.updateTool(model);cycle=run();
  auto consumer=get(blob);consumer.parameters["searchRoi"]=MatchService::rectangle({5,5,85,85});consumer=ReferenceService::teach(consumer,model,"pose",result(cycle,match),std::get<ImageFrame>(result(cycle,roi).outputs["image"].data));links.append({match,"pose",blob,"poseReference"});p.updateToolConnections(consumer,links);p.save();
  cropped.parameters["cropImage"]=false;p.updateTool(cropped);cropped.parameters["cropImage"]=true;p.updateTool(cropped);p.save();const auto recipe=p.activeRecipe()->id;p.openProject(p.path());p.selectRecipe(recipe);
  check(get(match).parameters["modelPng"].toString().isEmpty(),"crop mode A B A permanently clears model pixels");check(toolReferenceBinding(get(blob)).invalidated,"reference teaching tombstone survives mode A B A and reopen");
 }
 QTextStream(stdout)<<checks<<" crop consumer checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
