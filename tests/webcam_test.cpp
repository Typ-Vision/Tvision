#include "application/WebcamAdapter.hpp"
#include "application/ExecutionService.hpp"
#include "application/ProjectService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QThread>
#include <QTextStream>
#include <QUuid>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct State {int opens=0,reads=0,closes=0;bool failOpen=false,failRead=false,slow=false,cancelRead=false;cv::Mat pixels=cv::Mat(10,12,CV_8UC3,cv::Scalar(20,40,80));std::shared_ptr<std::atomic_bool> cancel;};
class Capture final : public WebcamCapture {
    std::shared_ptr<State> state;
public:
    explicit Capture(std::shared_ptr<State> s):state(std::move(s)){}
    ~Capture() override {++state->closes;state->pixels.setTo(0);}
    bool open(const WebcamSettings&) override {++state->opens;return !state->failOpen;}
    bool read(cv::Mat& out) override {++state->reads;if(state->slow)QThread::msleep(20);if(state->cancelRead)state->cancel->store(true);out=state->pixels;return !state->failRead;}
};
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    auto s=std::make_shared<State>();s->cancel=std::make_shared<std::atomic_bool>(false);
    WebcamAdapter adapter({},[s]{return std::make_unique<Capture>(s);});AcquisitionService service;
    AcquisitionRequest request{"source","cycle","frame",TriggerMode::Software,5000,s->cancel};
    const auto caps=adapter.capabilities();check(caps.softwareTrigger && !caps.externalTrigger && !caps.continuous && !caps.exposure && !caps.gain,"honest webcam capabilities");
    auto r=service.acquire(adapter,request);check(r.state==AcquisitionState::Done && s->closes==1,"capture cleanup");
    check(r.frame->image.preview().pixelColor(0,0)==QColor(80,40,20),"owned copy survives driver buffer reuse");
    check(r.frame->image.metadata().sourceId=="source" && !r.frame->image.metadata().exposureAtUtc,"completion not exposure");
    request.trigger=TriggerMode::External;r=service.acquire(adapter,request);check(!r.frame && s->opens==1,"reject hardware trigger before open");request.trigger=TriggerMode::Software;
    s->cancel->store(true);r=service.acquire(adapter,request);check(r.state==AcquisitionState::Cancelled && s->opens==1,"pre-cancel no open");s->cancel->store(false);
    s->slow=true;request.timeoutMs=1;r=service.acquire(adapter,request);check(r.state==AcquisitionState::Timeout && !r.frame && s->closes==2,"late frame cleanup/suppression");s->slow=false;request.timeoutMs=5000;
    s->cancelRead=true;r=service.acquire(adapter,request);check(r.state==AcquisitionState::Cancelled && !r.frame && s->closes==3,"cancel during read");s->cancelRead=false;s->cancel->store(false);
    s->failOpen=true;r=service.acquire(adapter,request);check(r.state==AcquisitionState::Error && !r.frame && s->closes==4,"open failure cleanup");s->failOpen=false;
    s->failRead=true;r=service.acquire(adapter,request);check(r.state==AcquisitionState::Error && !r.frame && s->closes==5,"unplug/read failure cleanup");s->failRead=false;
    r=service.acquire(adapter,request);check(r.state==AcquisitionState::Done,"reopen after fault");
    s->pixels=cv::Mat(4,4,CV_16UC1);r=service.acquire(adapter,request);check(r.state==AcquisitionState::Error && !r.frame && s->closes==7,"unsupported format cleanup");
    bool rejected=false;try{WebcamAdapter bad({-1,"qt"});}catch(const std::exception&){rejected=true;}check(rejected,"invalid index");
    QTemporaryDir dir(QDir(app.applicationDirPath()).filePath("webcam-XXXXXX"));check(dir.isValid(),"test directory");
    ProjectService project;project.createProject("Webcam","",dir.filePath("camera.vfproject"),"webcam");project.createRecipe("Camera","",ApplicationType::Measurement,"webcam");
    const auto source=project.addTool("webcam"),blob=project.addTool("blob");project.setConnections({{source,"image",blob,"image"}});project.save();const auto recipe=project.activeRecipe()->id;
    project.openProject(project.path());project.selectRecipe(recipe);check(project.tools()[0].parameters==defaultParameters("webcam"),"webcam save/reopen");
    cv::Mat image(12,16,CV_8UC1,cv::Scalar(255));auto frame=ImageFrame::fromBuffer(image.data,image.total(),16,12,image.step,PixelFormat::Gray8,FrameMetadata{source});
    auto c=ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),recipe,"signature",{}, {},frame);
    check(c.state==ExecutionState::Done && c.frames.size()==1 && c.frames.first().sourceId==source && c.tools[1].outputs["area"].valid,"webcam owned snapshot through real DAG");
    rejected=false;try{project.addTool("image_file");}catch(const std::exception&){rejected=true;}check(rejected && project.tools().size()==2,"single source transaction preserved");
    frame=frame.withMetadata(FrameMetadata{"foreign"});c=ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),recipe,"signature",{}, {},frame);check(c.state==ExecutionState::Error && c.tools[1].outputs.isEmpty(),"foreign source blocks downstream");
    QTextStream(stdout)<<"PASS: webcam ownership, failures, deadlines, cleanup, persistence and DAG\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
