#include "application/PoseRoiService.hpp"
#include "application/PoseMatchService.hpp"
#include "application/ShapeMatchService.hpp"
#include "application/BackendCapabilities.hpp"
#include "application/EditorSession.hpp"
#include "application/ToolExecutors.hpp"
#include "application/RuntimeService.hpp"
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
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2,"fixture directory");const bool reader=BackendCapabilities::get("zxing-reader").available;
    QImage qr;if(reader){qr=QImage(QDir(argv[1]).filePath("qr.pgm")).convertToFormat(QImage::Format_Grayscale8);check(qr.size()==QSize(270,270),"independent Zint fixture");}
    QTemporaryDir dir(QDir::currentPath()+"/tracking-inspection-XXXXXX");check(dir.isValid(),"build fixtures");
    QImage image(size,size,QImage::Format_Grayscale8);image.fill(30);
    for(int y=0;y<24;++y)for(int x=0;x<24;++x)image.scanLine(46+y)[51+x]=uchar(40+(x*37+y*71+x*y*13)%190);
    // Ten by six foreground pixels with a two by two hole, inside a much larger taught rectangle.
    for(int y=166;y<172;++y)for(int x=130;x<140;++x)if(!(x>=134 && x<136 && y>=168 && y<170))image.scanLine(y)[x]=230;
    if(reader)paste(image,qr,220,250);const auto moved=rotate(image);
    const auto path=dir.filePath("scene.png");check(image.save(path),"actual PNG");
    for(const QString kind:{"pattern_pose","shape_pose"})for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        ProjectService p;p.createProject("Tracking","",dir.filePath(kind+QString::number(int(type))+".vfproject"),"file");p.createRecipe("Part","",type,"file");const auto camera=p.setImageFile(path),poseId=p.addTool(kind);
        const auto teaching=frame(image,camera),current=frame(moved,camera);auto source=node(p,poseId);
        source.parameters["angleMin"]=0.;source.parameters["angleMax"]=90.;source.parameters["angleStep"]=90.;source.parameters["pyramidLevels"]=1;source.parameters["minScore"]=.95;
        source.parameters=(kind=="shape_pose" ? ShapeMatchService::teach:PoseMatchService::teach)(teaching,{51,46,24,24},{9,14},source.parameters);p.updateTool(source);
        auto actual=PoseMatchService::execute(source,{{"image",{{"image",PortType::Image,"px","image"},teaching,true,"frame","cycle",teaching.metadata()}}},"frame","cycle",{});for(auto& v:actual.outputs)v.evaluationId="teach";
        QVector<ToolConnection> links{{camera,"image",poseId,"image"}};QStringList consumers;
        for(const QString consumerKind:reader ? QStringList{"blob","code_read"}:QStringList{"blob"}){
            const auto id=p.addTool(consumerKind);consumers<<id;auto t=node(p,id);t.parameters["searchRoi"]=consumerKind=="blob" ? rectangle(120,160,40,20):rectangle(220,250,270,270);
            t=PoseRoiService::teachTool(t,{"part/local"},source,actual,teaching);p.updateTool(t);links.append({camera,"image",id,"image"});links.append({poseId,"pose",id,"poseReference"});
            reject([&]{ToolExecutors::get(consumerKind)(t,{{"image",{{"image",PortType::Image,"px","image"},current,true,"frame","cycle",current.metadata()}}},dir.path(),"frame","cycle",{});});
            auto bad=t;bad.parameters["searchRoi"]=rectangle(121,160,40,20);reject([&]{ToolRegistry::validate(bad);});
            bad=t;bad.parameters["searchRoi"]=QJsonObject{};reject([&]{PoseRoiService::teachTool(bad,{"part/local"},source,actual,teaching);});
            bad=t;bad.parameters["searchRoi"]=rectangle(120,160,1,20);reject([&]{PoseRoiService::teachTool(bad,{"part/local"},source,actual,teaching);});
            auto extra=t.parameters["poseTracking"].toObject();extra["extra"]=true;bad=t;bad.parameters["poseTracking"]=extra;reject([&]{ToolRegistry::validate(bad);});
        }
        p.setConnections(links);p.save();const auto id=p.activeRecipe()->id;const auto definition=p.activeRecipe()->definition;const auto revision=p.activeRecipe()->recipeVersion;p.openProject(p.path());p.selectRecipe(id);p.save();check(p.activeRecipe()->definition==definition && p.activeRecipe()->recipeVersion==revision && !p.dirty(),"persistence no-op");
        EditorContext context{p.project().id,p.path(),id,definition,{}};EditorSession session;
        auto full=ExecutionService{}.run(definition,dir.path(),id,session.signature(context),{}, {},current);done(full);check(session.acceptCycle(full,session.signature(context),context),"cache owns current results");const auto hash=session.frame->contentId();
        const auto blob=result(full,consumers[0]);check(std::get<double>(blob.outputs["area"].data)==56 && std::get<double>(blob.outputs["count"].data)==1,"rotated raw area and hole oracle");
        check(QLineF(std::get<QPointF>(blob.outputs["point"].data),{541.5,164.5}).length()<1e-6,"independent rotated centroid");
        int pixels=0;for(auto v:blob.mask.pixels)if(uchar(v))++pixels;check(pixels==56,"only real accepted pixels in mask");
        for(int y=164;y<=165;++y)for(int x=541;x<=542;++x){const auto local=QPoint(x,y)-blob.mask.origin;check(uchar(blob.mask.pixels[local.y()*blob.mask.size.width()+local.x()])==0,"raw rotated hole stays empty");}
        if(reader){const auto codes=std::get<CodeResults>(result(full,consumers[1]).outputs["codes"].data);check(codes.items.size()==1 && codes.items.first().text=="VISIONFLOW-T0.2","real rotated QR bytes");for(auto corner:codes.items.first().corners)check(corner.x()>190 && corner.x()<460 && corner.y()>249 && corner.y()<520,"reader polygon mapped to current raw frame");}
        for(const auto& tool:consumers){auto target=ExecutionService{}.runTool(definition,dir.path(),id,{}, {},tool,session.evaluation(context));done(target);check(target.acquisitions==0 && target.executedTools==QStringList{tool} && session.frame->contentId()==hash,"TargetOnly no acquisition or producer execution");
            auto badCache=[&](auto mutate,ExecutionState state){auto e=session.evaluation(context);mutate(e.cache[poseId].result);const auto r=ExecutionService{}.runTool(definition,dir.path(),id,{}, {},tool,e);check(r.state==state && r.executedTools.isEmpty() && r.tools.first().outputs.isEmpty() && r.tools.first().mask.pixels.isEmpty(),"reference failure has no absolute fallback");};
            for(const QString port:{"pose","scale","found","ambiguous","truncated"}){badCache([&](ToolResult& r){r.outputs[port].evaluationId="wrong";},ExecutionState::Error);badCache([&](ToolResult& r){r.outputs[port].method="wrong";},ExecutionState::Error);badCache([&](ToolResult& r){r.outputs[port].origin.completedAtUtc=r.outputs[port].origin.completedAtUtc.addMSecs(1);},ExecutionState::Error);}
            badCache([](ToolResult& r){r.outputs["scale"].data=std::nextafter(1.,2.);},ExecutionState::Error);badCache([](ToolResult& r){r.outputs["ambiguous"].data=true;},ExecutionState::Skipped);badCache([](ToolResult& r){auto pose=std::get<Pose2D>(r.outputs["pose"].data);pose.position={0,0};r.outputs["pose"].data=pose;},ExecutionState::Error);
            auto e=session.evaluation(context);e.cache.remove(poseId);const auto missing=ExecutionService{}.runTool(definition,dir.path(),id,{}, {},tool,e);check(missing.state==ExecutionState::Skipped && missing.executedTools.isEmpty(),"missing cached producer");
        }
        auto spare=node(p,consumers[0]);spare.id=QUuid::createUuid().toString(QUuid::WithoutBraces);spare.parameters.remove("poseTracking");auto graphTools=p.tools();graphTools.append(spare);auto broken=links;broken.append({camera,"image",spare.id,"image"});broken.append({spare.id,"region",consumers[0],"region"});const auto conflict=validateGraph(graphTools,broken);check(!conflict.valid() && conflict.errors.join(" ").contains("pose tracking cannot"),"acyclic compatible region conflict rejected");broken=links;broken.removeLast();check(!validateGraph(p.tools(),broken).valid(),"missing pose reference");broken=links;broken.last().sourceTool=camera;check(!validateGraph(p.tools(),broken).valid(),"wrong producer");
        auto edited=editorTools(definition);for(auto& t:edited)if(t.id==poseId)t.parameters["anchor"]=QJsonObject{{"x",10},{"y",14}};const auto stale=ExecutionService{}.run(withEditorTools(definition,edited),dir.path(),id,{}, {}, {},current);for(const auto& tool:consumers)check(result(stale,tool).state==ExecutionState::Error && result(stale,tool).outputs.isEmpty(),"model edit requires reteaching");
        for(const auto& tool:consumers){const auto target=ExecutionService{}.runTool(withEditorTools(definition,edited),dir.path(),id,{}, {},tool,session.evaluation(context));check(target.state==ExecutionState::Skipped && target.executedTools.isEmpty(),"model edit invalidates cached producer signature");}
        QImage repeated=moved;for(int y=0;y<24;++y)for(int x=0;x<24;++x)repeated.scanLine(46+y)[51+x]=image.constScanLine(46+y)[51+x];const auto ambiguous=ExecutionService{}.run(definition,dir.path(),id,{}, {}, {},frame(repeated,camera));check(std::get<bool>(result(ambiguous,poseId).outputs["ambiguous"].data),"actual repeated candidate ambiguity");for(const auto& tool:consumers)check(result(ambiguous,tool).state==ExecutionState::Skipped && result(ambiguous,tool).outputs.isEmpty(),"actual ambiguity blocks code and mask");
        edited=editorTools(definition);for(auto& t:edited)if(t.id==poseId)t.enabled=false;const auto disabled=ExecutionService{}.run(withEditorTools(definition,edited),dir.path(),id,{}, {}, {},current);for(const auto& tool:consumers)check(result(disabled,tool).state==ExecutionState::Skipped && result(disabled,tool).outputs.isEmpty(),"disabled producer blocks consumers");
        const auto resized=ExecutionService{}.run(definition,dir.path(),id,{}, {}, {},frame(moved.copy(0,0,700,768),camera));for(const auto& tool:consumers)check(result(resized,tool).outputs.isEmpty(),"actual resolution change rejects outputs");
        if(reader){edited=editorTools(definition);for(auto& t:edited)if(t.id==consumers[1]){t.parameters["rule"]="exact";t.parameters["expected"]="wrong";}const auto wrong=ExecutionService{}.runTool(withEditorTools(definition,edited),dir.path(),id,{}, {},consumers[1],session.evaluation(context));done(wrong);check(!std::get<bool>(wrong.tools.first().outputs["accepted"].data) && wrong.tools.first().decision==PartDecision::NotEvaluated,"content policy uses real tracked code without part PASS");}
        QImage blank(size,size,QImage::Format_Grayscale8);blank.fill(30);const auto absent=ExecutionService{}.run(definition,dir.path(),id,{}, {}, {},frame(blank,camera));for(const auto& tool:consumers)check(result(absent,tool).state==ExecutionState::Skipped && result(absent,tool).outputs.isEmpty(),"actual not-found skips");
        session.invalidateTool(context,poseId);for(const auto& tool:consumers)check(!session.evaluation(context).cache.contains(tool),"rerun clears downstream");
        const auto stopped=ExecutionService{}.run(definition,dir.path(),id,{},std::make_shared<std::atomic_bool>(true),{},current);check(stopped.state==ExecutionState::Skipped,"Stop clears");for(const auto& r:stopped.tools)check(r.outputs.isEmpty() && r.mask.pixels.isEmpty(),"no stopped mask/codes");
        check(moved.save(path),"moved PNG");auto acquired=ExecutionService{}.run(definition,dir.path(),id,{},{});done(acquired);check(acquired.acquisitions==1,"real PNG acquisition");for(const auto& tool:consumers)check(result(acquired,tool).decision==PartDecision::NotEvaluated,"no invented part decision");check(image.save(path),"restore PNG");
        RuntimeService runtime(dir.filePath("archive"),p.path()+".calibrations.json",RuntimeIdentity::localEngineer());reject([&]{runtime.checkConfiguration(*p.activeRecipe(),dir.path());});
    }
    // Independent 45-degree diamond oracle; AABB corners must never become foreground.
    const ToolRoi roi{{"raw"},RotatedRectangle{{50,50},{40*std::sqrt(2.),40*std::sqrt(2.)},45}};
    RectangleMask mask(roi,{101,101});int expected=0;for(int y=0;y<101;++y)for(int x=0;x<101;++x){const bool in=std::abs(x-50)+std::abs(y-50)<=40;check(mask.contains({double(x),double(y)})==in,"independent diamond membership");if(in)++expected;}int marked=0;for(int y=0;y<mask.pixels.rows;++y)for(int x=0;x<mask.pixels.cols;++x)if(mask.pixels.at<uchar>(y,x))++marked;check(marked==expected,"raster matches independent oracle");
    QImage bright(101,101,QImage::Format_Grayscale8);bright.fill(255);const auto raw=frame(bright,"source");auto rroi=roi;rroi.frame.id=raw.metadata().coordinateSystemId;
    ToolDefinition b;b.id="blob";b.kind="blob";b.parameters=defaultParameters("blob");const QMap<QString,PortValue> inputs{{"image",{{"image",PortType::Image,"px","image"},raw,true,"frame","cycle",raw.metadata()}}};
    const auto rawHash=raw.contentId();auto segmented=ToolExecutors::get("blob",{},rroi)(b,inputs,{},"frame","cycle",{});check(std::get<double>(segmented.outputs["area"].data)==expected,"45-degree mask excludes AABB corners");
    check(raw.contentId()==rawHash,"blob preserves owned raw");
    auto wrongFrame=rroi;wrongFrame.frame.id="other";reject([&]{ToolExecutors::get("blob",{},wrongFrame)(b,inputs,{},"frame","cycle",{});});
    bright.fill(0);const auto dark=frame(bright,"source");auto darkInputs=inputs;darkInputs["image"].data=dark;b.parameters["polarity"]="dark";segmented=ToolExecutors::get("blob",{},rroi)(b,darkInputs,{},"frame","cycle",{});check(std::get<double>(segmented.outputs["area"].data)==expected,"dark polarity also excludes AABB corners");
    reject([&]{RectangleMask(roi,{60,60});});reject([&]{RectangleMask(roi,{101,101},std::make_shared<std::atomic_bool>(true));});reject([&]{RectangleMask(ToolRoi{{"raw"},RotatedRectangle{{3000,3000},{5000,5000},0}},{6001,6001});});
    if(reader){
        QImage scene(1100,1100,QImage::Format_Grayscale8);scene.fill(255);paste(scene,qr,415,415);paste(scene,qr.scaled(135,135,Qt::IgnoreAspectRatio,Qt::FastTransformation),90,90);
        const auto f=frame(scene,"source");CodeReaderOptions options;options.formats={"QRCode"};options.maxCount=1;options.trackingRoi=ToolRoi{{f.metadata().coordinateSystemId},RotatedRectangle{{550,550},{900,500},45}};
        const auto codes=CodeReader::read(f,options);check(codes.items.size()==1 && codes.items[0].text=="VISIONFLOW-T0.2","outside code in AABB cannot count against limit");for(auto p:codes.items[0].corners)check(p.x()>415 && p.y()>415,"raw code corners retain offset");
        const auto h=f.contentId();CodeReader::read(f,options);check(f.contentId()==h,"masked reader never mutates owned raw");
        bool crosses=false;for(auto p:codes.items[0].corners)crosses|=std::abs((p.x()+p.y()-1100)/std::sqrt(2.))>80;check(crosses,"partial fixture actually cuts decoded symbol, not just quiet zone");
        options.trackingRoi=ToolRoi{{f.metadata().coordinateSystemId},RotatedRectangle{{550,550},{160,500},45}};check(CodeReader::read(f,options).items.isEmpty(),"partially masked QR cannot escape full-corner containment");
    }
    QTextStream(stdout)<<checks<<" tracking inspection checks passed; reader="<<reader<<'\n';return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
