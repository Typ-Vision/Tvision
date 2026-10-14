#include "vision/Preprocessing.hpp"
#include "application/EditorSession.hpp"
#include "application/RuntimeService.hpp"
#include "domain/PreprocessConfig.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QJsonArray>
#include <QTextStream>
#include <opencv2/imgproc.hpp>
using namespace visionflow;
namespace {
int checks=0;void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(...){failed=true;}check(failed,"expected preprocess rejection");}
ImageFrame frame(const cv::Mat& m,PixelFormat format){return ImageFrame::fromBuffer(m.data,(m.rows-1)*m.step+m.cols*m.elemSize(),m.cols,m.rows,m.step,format,{"source","frame","cycle","raw"});}
QJsonObject parameters(const QString& op,double value=0,int size=3){return {{"channel","gray"},{"steps",QJsonArray{QJsonObject{{"op",op},{"size",size},{"value",value}}}}};}
const ToolResult& result(const CycleResult& c,const QString& id){for(const auto& r:c.tools)if(r.toolId==id)return r;throw std::runtime_error("missing tool");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    cv::Mat padded(15,23,CV_8U,cv::Scalar(0));auto m=padded(cv::Rect(2,0,15,15));m(cv::Rect(4,4,7,7)).setTo(200);m.at<uchar>(7,7)=0;m.at<uchar>(0,0)=255;
    const auto raw=frame(m,PixelFormat::Gray8);m=m.clone();const auto hash=raw.contentId();
    for(const auto& op:QStringList{"gaussian","median","threshold","adaptive_mean","adaptive_gaussian","erode","dilate","open","close"}){
        const double v=op=="threshold" ? 100:op.startsWith("adaptive") ? 3:0;const auto out=Preprocessing::run(raw,parameters(op,v));cv::Mat expected;
        if(op=="gaussian")cv::GaussianBlur(m,expected,{3,3},0,0,cv::BORDER_REPLICATE);
        else if(op=="median")cv::medianBlur(m,expected,3);
        else if(op=="threshold")cv::threshold(m,expected,100,255,cv::THRESH_BINARY);
        else if(op.startsWith("adaptive"))cv::adaptiveThreshold(m,expected,255,op=="adaptive_mean" ? cv::ADAPTIVE_THRESH_MEAN_C:cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY,3,3);
        else cv::morphologyEx(m,expected,op=="erode" ? cv::MORPH_ERODE:op=="dilate" ? cv::MORPH_DILATE:op=="open" ? cv::MORPH_OPEN:cv::MORPH_CLOSE,cv::getStructuringElement(cv::MORPH_RECT,{3,3}),{-1,-1},1,cv::BORDER_REPLICATE);
        check(cv::norm(ImageAccess(out).ownedCopy(),expected,cv::NORM_INF)==0,"filter reference mismatch");check(out.size()==raw.size() && out.metadata().coordinateSystemId=="raw" && out.processing().size()==1 && raw.contentId()==hash,"geometry/ownership/provenance");
    }
    auto binary=ImageAccess(Preprocessing::run(raw,parameters("threshold",200))).ownedCopy();check(binary.at<uchar>(5,5)==0 && binary.at<uchar>(0,0)==255,"threshold strict boundary");
    auto opened=ImageAccess(Preprocessing::run(raw,parameters("open"))).ownedCopy();check(opened.at<uchar>(0,0)==0 && opened.at<uchar>(7,7)==0,"open removes speck and preserves hole policy");
    for(const auto format:{PixelFormat::Rgb8,PixelFormat::Bgr8,PixelFormat::Rgba8,PixelFormat::Bgra8}){const bool rgb=format==PixelFormat::Rgb8 || format==PixelFormat::Rgba8;const int n=format==PixelFormat::Rgb8 || format==PixelFormat::Bgr8 ? 3:4;cv::Mat color(7,9,CV_MAKETYPE(CV_8U,n),rgb ? cv::Scalar(200,80,20,7):cv::Scalar(20,80,200,7));const auto f=frame(color,format);for(const auto& ch:QStringList{"red","green","blue"})check(Preprocessing::gray8(f,ch).at<uchar>(0,0)==(ch=="red" ? 200:ch=="green" ? 80:20),"semantic channel order");check(Preprocessing::gray8(f).at<uchar>(0,0)==109,"gray luminance");if(n==4)check(Preprocessing::gray8(f,"alpha").at<uchar>(0,0)==7,"alpha channel");else reject([&]{Preprocessing::gray8(f,"alpha");});}
    cv::Mat gray16(3,3,CV_16U,cv::Scalar(65535));gray16.at<ushort>(1,1)=0;check(Preprocessing::gray8(frame(gray16,PixelFormat::Gray16)).at<uchar>(0,0)==255,"gray16 normalization");
    reject([&]{Preprocessing::gray8(raw,"red");});reject([&]{validatePreprocess(parameters("median",0,4));});reject([&]{validatePreprocess(parameters("crop"));});reject([&]{validatePreprocess(parameters("median",2));});reject([&]{Preprocessing::run(raw,parameters("gaussian"),std::make_shared<std::atomic_bool>(true));});
    auto a=parameters("gaussian");auto b=parameters("threshold",100);a["steps"]=QJsonArray{a["steps"].toArray().first(),b["steps"].toArray().first()};b["steps"]=QJsonArray{b["steps"].toArray().first(),parameters("gaussian")["steps"].toArray().first()};check(Preprocessing::run(raw,a).contentId()!=Preprocessing::run(raw,b).contentId(),"filter order matters");
    auto copy=ImageAccess(Preprocessing::run(raw,parameters("median"))).ownedCopy();copy.setTo(99);check(raw.contentId()==hash,"output mutation isolated");
    QTemporaryDir dir(QDir::currentPath()+"/preprocess-XXXXXX");check(dir.isValid(),"build test directory");const auto path=dir.filePath("raw.png");check(raw.preview().save(path),"fixture save");
    ProjectService project;project.createProject("Preprocess","",dir.filePath("part.vfproject"),"file");project.createRecipe("Part","",ApplicationType::Measurement,"file");const auto source=project.setImageFile(path),pre=project.addTool("preprocess"),blob=project.addTool("blob");
    for(auto t:project.tools())if(t.id==pre){t.parameters=parameters("threshold",100);project.updateTool(t);}project.setConnections({{source,"image",pre,"image"},{pre,"image",blob,"image"}});project.save();const auto recipeId=project.activeRecipe()->id;const auto revision=project.activeRecipe()->recipeVersion;project.openProject(project.path());project.selectRecipe(recipeId);project.save();check(project.activeRecipe()->recipeVersion==revision && project.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==7,"opt-in schema and no-op reopen");
    EditorSession session;auto context=[&]{return EditorContext{project.project().id,project.path(),recipeId,project.activeRecipe()->definition,{}};};auto c=context();auto full=ExecutionService{}.run(c.definition,dir.path(),recipeId,session.signature(c),{});check(full.state==ExecutionState::Done,"preprocess real DAG");check(session.acceptCycle(full,session.signature(c),c) && session.frame->processing().isEmpty() && session.frame->contentId()==hash,"session retains acquisition raw");
    for(auto t:project.tools())if(t.id==pre){t.parameters=parameters("threshold",220);project.updateTool(t);}c=context();session.reconcile(c);auto target=ExecutionService{}.runTool(c.definition,dir.path(),recipeId,session.signature(c),{},blob,session.evaluation(c));check(target.executedTools.isEmpty(),"changed preprocessing invalidates downstream");target=ExecutionService{}.runTool(c.definition,dir.path(),recipeId,session.signature(c),{},pre,session.evaluation(c));check(target.acquisitions==0 && target.executedTools==QStringList{pre} && session.acceptTool(target,session.signature(c),c),"target preprocess uses cached raw only");target=ExecutionService{}.runTool(c.definition,dir.path(),recipeId,session.signature(c),{},blob,session.evaluation(c));check(target.acquisitions==0 && std::get<double>(result(target,blob).outputs["area"].data)==1,"downstream consumes processed pixels");
    project.save();RuntimeService runtime(dir.filePath("archive"),project.path()+".calibrations.json",RuntimeIdentity::localEngineer());runtime.prepare(*project.activeRecipe(),dir.path());const auto lease=runtime.begin();auto cycle=RuntimeService::execute(lease,project.path()+".calibrations.json",{});check(cycle.state==ExecutionState::Done,"preprocess Runtime snapshot");runtime.finish(lease.ticket,cycle);check(runtime.records().size()==1,"preprocess terminal record");
    QTextStream(stdout)<<checks<<" preprocessing checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
