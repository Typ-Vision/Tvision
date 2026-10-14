#include "vision/matching/PoseMatcher.hpp"
#include "application/PoseMatchService.hpp"
#include "application/EditorSession.hpp"
#include "application/RuntimeService.hpp"
#include "application/ToolResultCodec.hpp"
#include "domain/PoseMatchConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "vision/ImageAccess.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <numbers>
#include <algorithm>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"expected rejection");}
cv::Mat model(){cv::Mat m(40,48,CV_8U,cv::Scalar(30));cv::rectangle(m,{4,5,11,27},cv::Scalar(210),-1);cv::circle(m,{32,12},7,cv::Scalar(150),-1);cv::line(m,{22,27},{40,34},cv::Scalar(245),3);cv::GaussianBlur(m,m,{3,3},.6);return m;}
// Independent inverse rasterizer: raw pixel centres -> taught model-local sample.
void render(cv::Mat& dst,const cv::Mat& m,cv::Point2d local,cv::Point2d anchor,double degrees,double scale){
    const double a=degrees*std::numbers::pi/180,c=std::cos(a),s=std::sin(a);
    for(int y=0;y<dst.rows;++y)for(int x=0;x<dst.cols;++x){const double dx=(x-anchor.x)/scale,dy=(y-anchor.y)/scale,u=c*dx+s*dy+local.x,v=-s*dx+c*dy+local.y;
        if(u<0 || v<0 || u>m.cols-1 || v>m.rows-1)continue;const int ix=int(u),iy=int(v);const double fx=u-ix,fy=v-iy;
        auto sample=[&](int xx,int yy){return double(m.at<uchar>(std::min(yy,m.rows-1),std::min(xx,m.cols-1)));};
        dst.at<uchar>(y,x)=cv::saturate_cast<uchar>((1-fy)*((1-fx)*sample(ix,iy)+fx*sample(ix+1,iy))+fy*((1-fx)*sample(ix,iy+1)+fx*sample(ix+1,iy+1)));
    }
}
ImageFrame frame(const cv::Mat& m){return ImageFrame::fromBuffer(m.data,m.step*m.rows,m.cols,m.rows,m.step,PixelFormat::Gray8,{"source","frame","cycle","raw",QDateTime::currentDateTimeUtc()},false);}
const ToolResult& result(const CycleResult& c,const QString& id){for(const auto& t:c.tools)if(t.toolId==id)return t;throw std::runtime_error("missing result");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto m=model();const cv::Point2d local(9.25,28.5),target(112.5,95.25);
    PoseSearch o;o.angleMin=-40;o.angleMax=40;o.angleStep=10;o.scaleMin=.8;o.scaleMax=1.2;o.scaleStep=.1;o.minScore=.9;
    for(const auto [degrees,scale]:{std::pair{-27.5,.85},std::pair{0.,1.},std::pair{27.5,1.15}}){
        cv::Mat image(200,240,CV_8U,cv::Scalar(8));render(image,m,local,target,degrees,scale);
        const auto r=PoseMatcher::find(image,m,local,{17,13,200,170},o);
        check(bool(r.selected),"rotated/scaled asymmetric target selected");const auto c=*r.selected;
        check(cv::norm(c.anchor-target)<1.5,"independent anchor oracle");check(std::abs(c.angleRadians-degrees*std::numbers::pi/180)<.05,"independent signed angle oracle");check(std::abs(c.scale-scale)<.045,"separate scale oracle");check(c.score>=.9 && !r.ambiguous,"method score and NMS");
    }
    cv::Mat image(200,240,CV_8U,cv::Scalar(8));render(image,m,local,target,20,1);
    for(int levels:{1,2,3}){auto p=o;p.pyramidLevels=levels;const auto r=PoseMatcher::find(image,m,local,{0,0,240,200},p);check(r.selected && cv::norm(r.selected->anchor-target)<1.5,"pyramid levels refine on raw pixels");}
    cv::Mat repeated(220,320,CV_8U,cv::Scalar(8));render(repeated,m,local,{70,90},20,1);render(repeated,m,local,{230,145},20,1);
    auto r=PoseMatcher::find(repeated,m,local,{0,0,320,220},o);check(r.ambiguous && !r.selected && r.candidates.size()>=2,"equal targets reject ambiguous pose");o.selectBest=true;r=PoseMatcher::find(repeated,m,local,{0,0,320,220},o);check(r.ambiguous && r.selected,"explicit best preserves ambiguity");o.selectBest=false;
    cv::Mat sym(40,40,CV_8U,cv::Scalar(20));cv::rectangle(sym,{5,9,30,22},cv::Scalar(190),-1);cv::circle(sym,{19,19},5,cv::Scalar(50),-1);
    // Exact 180-degree symmetry about the integer pixel-centre midpoint.
    for(int y=0;y<40;++y)for(int x=0;x<40;++x)sym.at<uchar>(y,x)=uchar((int(sym.at<uchar>(y,x))+int(sym.at<uchar>(39-y,39-x)))/2);
    // Ensure exact symmetry independently of in-place averaging.
    cv::Mat flipped;cv::flip(sym,flipped,-1);cv::addWeighted(sym,.5,flipped,.5,0,sym);
    cv::Mat symmetric(140,160,CV_8U,cv::Scalar(8));render(symmetric,sym,{19.5,19.5},{80,70},0,1);
    PoseSearch so;so.angleMin=0;so.angleMax=180;so.angleStep=90;so.pyramidLevels=1;so.minScore=.95;
    r=PoseMatcher::find(symmetric,sym,{19.5,19.5},{0,0,160,140},so);check(r.ambiguous && !r.selected,"symmetric orientation is ambiguous");
    cv::Mat cross(41,41,CV_8U,cv::Scalar(20));cv::rectangle(cross,{5,16,31,9},cv::Scalar(190),-1);cv::rectangle(cross,{16,5,9,31},cv::Scalar(190),-1);
    cv::Mat crossScene(140,160,CV_8U,cv::Scalar(8));render(crossScene,cross,{20,20},{80,70},0,1);
    r=PoseMatcher::find(crossScene,cross,{20,20},{0,0,160,140},so);check(r.ambiguous && !r.selected && r.candidates.size()>=3,"90-degree symmetry survives orientation NMS");
    cv::Mat triple(220,400,CV_8U,cv::Scalar(8));for(auto a:{cv::Point2d(70,90),cv::Point2d(200,90),cv::Point2d(330,90)})render(triple,m,local,a,20,1);
    auto bounded=o;bounded.maxCandidates=2;r=PoseMatcher::find(triple,m,local,{0,0,400,220},bounded);check(r.truncated && r.ambiguous && !r.selected && r.candidates.size()==2,"bounded candidates explicitly incomplete and ambiguous");
    r=PoseMatcher::find(image,m,local,{0,0,60,60},o);check(!r.selected,"absolute search excludes outside target");
    cv::Mat blank(200,240,CV_8U,cv::Scalar(8));r=PoseMatcher::find(blank,m,local,{0,0,240,200},o);check(r.candidates.empty() && !r.selected && !r.ambiguous,"constant image has no finite false perfect score");
    auto cancelled=std::make_shared<std::atomic_bool>(true);reject([&]{PoseMatcher::find(image,m,local,{0,0,240,200},o,cancelled);});
    reject([&]{PoseMatcher::find(image,m,{-1,0},{0,0,240,200},o);});reject([&]{PoseMatcher::find(image,m,local,{-1,0,240,200},o);});
    auto invalid=o;invalid.angleStep=.01;reject([&]{invalid.validate();});invalid=o;invalid.scaleMin=0;reject([&]{invalid.validate();});invalid=o;invalid.angleMin=-180;invalid.angleMax=180;invalid.angleStep=1;reject([&]{invalid.validate();});
    invalid=o;invalid.angleMin=std::numeric_limits<double>::quiet_NaN();reject([&]{invalid.validate();});invalid=o;invalid.minScore=1.1;reject([&]{invalid.validate();});invalid=o;invalid.maxCandidates=1;reject([&]{invalid.validate();});
    cv::Mat flatModel(40,40,CV_8U,cv::Scalar(20));reject([&]{PoseMatcher::find(image,flatModel,{20,20},{0,0,240,200},o);});
    auto p=poseMatchDefaults();validatePoseMatch(p);p["selection"]="first";reject([&]{validatePoseMatch(p);});p=poseMatchDefaults();p["anchor"]=QJsonObject{{"x",1},{"y",1}};reject([&]{validatePoseMatch(p);});
    p=PoseMatchService::teach(frame(m),{0,0,m.cols,m.rows},{local.x,local.y},poseMatchDefaults());check(p["anchor"].toObject()["x"].toDouble()==local.x,"teaching preserves noncentral subpixel anchor");
    p["angleMin"]=-40;p["angleMax"]=40;p["minScore"]=.9;
    ToolDefinition tool;tool.id="pose";tool.name="Pose";tool.kind="pattern_pose";tool.parameters=p;
    auto inputFrame=frame(image);PortValue input{inputPorts(tool.kind).first(),inputFrame,true,"frame","cycle",inputFrame.metadata()};
    auto tr=PoseMatchService::execute(tool,{{"image",input}},"frame","cycle",{});check(tr.state==ExecutionState::Done && tr.decision==PartDecision::NotEvaluated && tr.outputs["pose"].valid,"application actual typed pose");
    auto repeatFrame=frame(repeated);auto repeatInput=input;repeatInput.data=repeatFrame;repeatInput.origin=repeatFrame.metadata();auto amb=PoseMatchService::execute(tool,{{"image",repeatInput}},"frame","cycle",{});
    check(std::get<bool>(amb.outputs["found"].data) && std::get<bool>(amb.outputs["ambiguous"].data) && !amb.outputs["pose"].valid && !amb.outputs["scale"].valid && !amb.outputs["score"].valid && amb.points.isEmpty(),"ambiguous application clears selection metrics/overlay");
    const auto list=QJsonDocument::fromJson(std::get<QString>(amb.outputs["candidates"].data).toUtf8()).array();check(list.size()>=2 && list.first().toObject().contains("angleRadians") && list.first().toObject().contains("scale"),"bounded candidate evidence contains angle and separate scale");
    auto bestTool=tool;bestTool.parameters["selection"]="best";amb=PoseMatchService::execute(bestTool,{{"image",repeatInput}},"frame","cycle",{});check(amb.outputs["pose"].valid && std::get<bool>(amb.outputs["ambiguous"].data),"best policy keeps ambiguity evidence");
    const auto pose=std::get<Pose2D>(tr.outputs["pose"].data);check(pose.frame.id=="raw" && pose.frame.axes==AxisConvention::XRightYDown && pose.angleRadians>.2,"raw clockwise typed pose frame");
    for(auto& v:tr.outputs){v.evaluationId="direct-test";ToolResultCodec::validate(v,v.port,"frame","cycle");const auto back=ToolResultCodec::decode(ToolResultCodec::encode(v));check(ToolResultCodec::encode(back)==ToolResultCodec::encode(v),"typed pose/result roundtrip");}
    auto wrong=input;wrong.origin.sourceId="wrong";reject([&]{PoseMatchService::execute(tool,{{"image",wrong}},"frame","cycle",{});});
    auto missing=frame(blank);auto absent=PoseMatchService::execute(tool,{{"image",{input.port,missing,true,"frame","cycle",missing.metadata()}}},"frame","cycle",{});check(!absent.outputs["pose"].valid && !absent.outputs["score"].valid && absent.points.isEmpty(),"not found never publishes pose or score");
    QTemporaryDir dir(QDir::currentPath()+"/pose-XXXXXX");check(dir.isValid(),"build-only fixtures");const auto path=dir.filePath("part.png");check(cv::imwrite(path.toStdString(),image),"independent image saved");
    for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        ProjectService project;project.createProject("Pose","",dir.filePath(QString::number(int(type))+".vfproject"),"file");project.createRecipe("Part","",type,"file");const auto src=project.setImageFile(path),id=project.addTool("pattern_pose");
        for(auto t:project.tools())if(t.id==id){t.parameters=p;project.updateTool(t);}project.setConnections({{src,"image",id,"image"}});project.save();const auto recipe=project.activeRecipe()->id;const auto rev=project.activeRecipe()->recipeVersion;const auto definition=project.activeRecipe()->definition;
        project.openProject(project.path());project.selectRecipe(recipe);project.save();check(project.activeRecipe()->recipeVersion==rev && project.activeRecipe()->definition==definition && !project.dirty(),"schema7 save/reopen/no-op");
        EditorContext context{project.project().id,project.path(),recipe,definition,{}};EditorSession session;auto cycle=ExecutionService{}.run(definition,dir.path(),recipe,session.signature(context),{});if(cycle.state!=ExecutionState::Done){QTextStream(stderr)<<cycle.error<<"\n";for(const auto& t:cycle.tools)QTextStream(stderr)<<t.toolId<<" "<<t.message<<"\n";}check(cycle.state==ExecutionState::Done && result(cycle,id).outputs["pose"].valid && cycle.acquisitions==1,"shared actual DAG for application type");
        check(session.acceptCycle(cycle,session.signature(context),context),"pose cache accepted");const auto hash=session.frame->contentId();auto targetCycle=ExecutionService{}.runTool(definition,dir.path(),recipe,session.signature(context),{},id,session.evaluation(context));check(targetCycle.acquisitions==0 && targetCycle.executedTools==QStringList{id} && result(targetCycle,id).outputs["pose"].valid && session.frame->contentId()==hash,"TargetOnly actual cached image without acquisition");
        auto changed=project.tools();for(auto t:changed)if(t.id==id){t.parameters["anchor"]=QJsonObject{{"x",10},{"y",20}};project.updateTool(t);}check(project.activeRecipe()->definition!=definition,"teaching anchor changes execution signature");
        RuntimeService runtime(dir.filePath("archive"),project.path()+".calibrations.json",RuntimeIdentity::localEngineer());bool unsupported=false;try{runtime.checkConfiguration(*project.activeRecipe(),dir.path());}catch(const std::exception& e){unsupported=QString::fromUtf8(e.what()).contains("desteklenmeyen tool");}check(unsupported,"production explicitly rejects pattern_pose until trusted completion integration");
        EditorSession::suppressCancelled(cycle);for(const auto& t:cycle.tools)check(t.outputs.isEmpty() && t.points.isEmpty(),"Stop removes old pose");
    }
    const auto legacyPorts=outputPorts("template_match");check(std::none_of(legacyPorts.begin(),legacyPorts.end(),[](auto p){return p.type==PortType::Pose2D;}),"legacy match never exposes invented angle pose");
    QTextStream(stdout)<<checks<<" pose checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
