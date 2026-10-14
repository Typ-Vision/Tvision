#include "application/PoseRoiService.hpp"
#include "application/ReferenceService.hpp"
#include "application/MatchService.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "application/BackendCapabilities.hpp"
#include "application/EditorSession.hpp"
#include "application/ToolExecutors.hpp"
#include "application/RuntimeService.hpp"
#include "reference_production_support.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "vision/RectangleMask.hpp"
#include "vision/codes/CodeReader.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QTextStream>
#include <QImage>
#include <QUuid>
#include <cmath>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F>void reject(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,"expected rejection");}
ToolResult result(const CycleResult& c,const QString& id){for(const auto& r:c.tools)if(r.toolId==id)return r;throw std::runtime_error("missing tool");}
void done(const CycleResult& c){if(c.state!=ExecutionState::Done){QTextStream(stderr)<<c.error<<'\n';for(const auto& r:c.tools)QTextStream(stderr)<<r.toolId<<' '<<r.message<<'\n';}check(c.state==ExecutionState::Done,"inspection DAG completes");}
constexpr int size=768;
ImageFrame frame(const QImage& image,const QString& source){FrameMetadata m{source,"frame","cycle",source+"/image",QDateTime::currentDateTimeUtc()};return ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),image.width(),image.height(),image.bytesPerLine(),PixelFormat::Gray8,m,false);}
QImage rotate(const QImage& image){QImage out(size,size,QImage::Format_Grayscale8);out.fill(30);for(int y=0;y<size;++y)for(int x=0;x<size;++x){const int sx=y-90+60,sy=60-(x-650);if(sx>=0 && sx<size && sy>=0 && sy<size)out.scanLine(y)[x]=image.constScanLine(sy)[sx];}return out;}
void paste(QImage& dst,const QImage& src,int left,int top){for(int y=0;y<src.height();++y)for(int x=0;x<src.width();++x)dst.scanLine(y+top)[x+left]=src.constScanLine(y)[x];}
ToolDefinition node(const ProjectService& project,const QString& id){for(const auto& t:project.tools())if(t.id==id)return t;throw std::runtime_error("missing node");}
QJsonObject rectangle(int x,int y,int w,int h){return {{"x",x},{"y",y},{"width",w},{"height",h}};}
}

QImage translate(const QImage& image){QImage out(size,size,QImage::Format_Grayscale8);out.fill(30);for(int y=10;y<size;++y)for(int x=20;x<size;++x)out.scanLine(y)[x]=image.constScanLine(y-10)[x-20];return out;}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2,"fixture directory");const bool reader=BackendCapabilities::get("zxing-reader").available;
    QImage qr;if(reader){qr=QImage(QDir(argv[1]).filePath("qr.pgm")).convertToFormat(QImage::Format_Grayscale8);check(qr.size()==QSize(270,270),"Independent Zint QR");}
    QTemporaryDir dir(QDir::currentPath()+"/reference-inspection-XXXXXX");QImage image(size,size,QImage::Format_Grayscale8);image.fill(30);
    for(int y=0;y<24;++y)for(int x=0;x<24;++x)image.scanLine(46+y)[51+x]=uchar(40+(x*37+y*71+x*y*13)%190);
    for(int y=166;y<172;++y)for(int x=130;x<140;++x)if(!(x>=134 && x<136 && y>=168 && y<170))image.scanLine(y)[x]=230;
    if(reader)paste(image,qr,220,250);const auto path=dir.filePath("scene.png");check(image.save(path),"Actual PNG");
    for(const QString kind:{"template_match","pattern_pose","shape_pose"})for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        const bool rigid=kind!="template_match";ProjectService p;p.createProject("References","",dir.filePath(kind+QString::number(int(type))+".vfproject"),"file");p.createRecipe("Part","",type,"file");const auto camera=p.setImageFile(path),sourceId=p.addTool(kind);
        auto source=node(p,sourceId);const auto teaching=frame(image,camera);
        if(rigid){source.parameters["angleMin"]=0.;source.parameters["angleMax"]=90.;source.parameters["angleStep"]=90.;source.parameters["pyramidLevels"]=1;source.parameters["minScore"]=.95;source.parameters=(kind=="shape_pose" ? ShapeMatchService::teach:PoseMatchService::teach)(teaching,{51,46,24,24},{9,14},source.parameters);}
        else {source.parameters["modelRoi"]=rectangle(51,46,24,24);source.parameters["modelPng"]=MatchService{}.teach(teaching,{51,46,24,24});source.parameters["minScore"]=.95;}
        p.updateTool(source);QStringList consumers;
        for(const QString consumer:reader ? QStringList{"blob","code_read"}:QStringList{"blob"}){const auto id=p.addTool(consumer);consumers<<id;auto t=node(p,id);t.parameters["searchRoi"]=consumer=="blob" ? rectangle(120,160,40,20):rectangle(220,250,270,270);p.updateTool(t);}
        if(!reader)reject([&]{p.addTool("code_read");});
        const auto baseline=ExecutionService{}.run(p.activeRecipe()->definition,dir.path(),p.activeRecipe()->id,{}, {},{},teaching);done(baseline);
        const auto baselineImage=std::get<ImageFrame>(result(baseline,camera).outputs["image"].data);
        for(const auto& id:consumers){auto t=ReferenceService::teach(node(p,id),source,rigid ? "pose":"point",result(baseline,sourceId),baselineImage,{1,-1});p.bindReference(id,toolReferenceBinding(t));}
        const auto current=frame(rigid ? rotate(image):translate(image),camera);const auto definition=p.activeRecipe()->definition;EditorContext context{p.project().id,p.path(),p.activeRecipe()->id,definition,{}};EditorSession session;
        const auto full=ExecutionService{}.run(definition,dir.path(),context.recipeId,session.signature(context),{}, {},current);done(full);check(session.acceptCycle(full,session.signature(context),context),"Current output cache");
        const auto blob=result(full,consumers[0]);check(std::get<double>(blob.outputs["area"].data)==56,"Point/Pose offset mask preserves area and hole");
        check(QLineF(std::get<QPointF>(blob.outputs["point"].data),rigid ? QPointF(541.5,164.5):QPointF(154.5,178.5)).length()<1e-6,"Actual centroid independent oracle");
        for(const auto& id:consumers){const auto target=ExecutionService{}.runTool(definition,dir.path(),context.recipeId,{}, {},id,session.evaluation(context));done(target);check(target.acquisitions==0 && target.executedTools==QStringList{id},"Inspection target only");}
        if(reader){const auto codes=std::get<CodeResults>(result(full,consumers[1]).outputs["codes"].data);check(codes.items.size()==1 && codes.items.first().text=="VISIONFLOW-T0.2","Actual Point/Pose offset QR decoded");for(const auto& q:codes.items.first().corners)check(rigid ? q.x()>190 && q.x()<460 && q.y()>249 && q.y()<520:q.x()>240 && q.x()<510 && q.y()>260 && q.y()<530,"Decoded raw polygon oracle");}
        p.save();check((rigid ? rotate(image):translate(image)).save(path),"Runtime inspection source");
        const auto production=productionAcceptance(*p.activeRecipe(),dir.path(),dir.filePath(kind+QString::number(int(type))+"-production"),p.path()+".calibrations.json");
        check(std::get<double>(result(production,consumers[0]).outputs["area"].data)==56,"Runtime actual offset Blob hole");
        if(reader)check(std::get<CodeResults>(result(production,consumers[1]).outputs["codes"].data).items.first().text=="VISIONFLOW-T0.2","Runtime actual QR");
        check(image.save(path),"Restore inspection teaching scene");
        // Bad current source cannot leave code/mask/point results.
        auto evaluation=session.evaluation(context);evaluation.cache[sourceId].result.outputs["found"].evaluationId="wrong";
        for(const auto& id:consumers){const auto c=ExecutionService{}.runTool(definition,dir.path(),context.recipeId,{}, {},id,evaluation);check(c.state==ExecutionState::Error && c.tools.first().outputs.isEmpty() && c.tools.first().mask.pixels.isEmpty(),"No stale inspection fallback");}
        auto b=toolReferenceBinding(node(p,consumers[0]));b.offsetPx={1000,1000};check(!ReferenceService::resolve(b,source,result(full,sourceId),current).roi,"Out of frame offset rejected");
    }
    if(reader){QImage scene(1100,1100,QImage::Format_Grayscale8);scene.fill(255);paste(scene,qr,415,415);paste(scene,qr.scaled(135,135,Qt::IgnoreAspectRatio,Qt::FastTransformation),90,90);const auto f=frame(scene,"source");CodeReaderOptions options;options.formats={"QRCode"};options.maxCount=1;options.trackingRoi=ToolRoi{{f.metadata().coordinateSystemId},RotatedRectangle{{550,550},{900,500},45}};const auto codes=CodeReader::read(f,options);check(codes.items.size()==1,"Outside AABB code does not consume limit");options.trackingRoi=ToolRoi{{f.metadata().coordinateSystemId},RotatedRectangle{{550,550},{160,500},45}};check(CodeReader::read(f,options).items.isEmpty(),"All decoded corners must lie in actual mask");}
    QTextStream(stdout)<<checks<<" common reference inspection checks; reader="<<reader<<'\n';return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
