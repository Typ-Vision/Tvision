#include "application/AcquisitionService.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <opencv2/imgcodecs.hpp>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
class ProbeAdapter final : public AcquisitionAdapter {
public:
    ImageFrame image;
    mutable int calls=0;
    bool stop=false,slow=false,fail=false;
    AcquisitionCapabilities capabilities() const override {return {"test",true,false,false,false,false};}
    ImageFrame acquire(const AcquisitionRequest& request) const override {
        ++calls;
        if(slow)QThread::msleep(20);
        if(stop)request.cancel->store(true);
        if(fail)throw std::runtime_error("adapter failure");
        return image;
    }
};
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir dir(QDir(app.applicationDirPath()).filePath("acquisition-XXXXXX"));check(dir.isValid(),"test directory");
        const auto path=dir.filePath("fixture.png");
        cv::Mat pixels(12,16,CV_8UC1,cv::Scalar(73));std::vector<uchar> bytes;cv::imencode(".png",pixels,bytes);
        {QFile file(path);check(file.open(QIODevice::WriteOnly),"fixture open");check(file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size())==qint64(bytes.size()),"fixture write");}
        const ImageFileAdapter file(path);const auto caps=file.capabilities();
        check(caps.sourceKind=="image_file" && caps.softwareTrigger && !caps.externalTrigger && !caps.continuous && !caps.exposure && !caps.gain,"honest file capabilities");
        AcquisitionRequest request{"source","cycle","frame",TriggerMode::Software,5000,std::make_shared<std::atomic_bool>(false)};
        const AcquisitionService service;auto r=service.acquire(file,request);
        check(r.state==AcquisitionState::Done && r.frame.has_value(),"real file acquisition");
        check(r.frame->image.size()==QSize(16,12) && r.frame->image.pixelText(0,0)=="Gray: 73","real pixels");
        check(r.frame->sourceId==request.sourceId && r.frame->cycleId==request.cycleId && r.frame->frameId==request.frameId && r.frame->acquiredAtUtc.isValid() && r.frame->acquiredAtUtc.offsetFromUtc()==0,"identity and UTC timestamp");
        ProbeAdapter probe;probe.image=r.frame->image;
        request.trigger=TriggerMode::External;r=service.acquire(probe,request);check(r.state==AcquisitionState::Error && !r.frame && probe.calls==0,"reject trigger before adapter call");
        request.trigger=TriggerMode::Continuous;r=service.acquire(file,request);check(r.state==AcquisitionState::Error && !r.frame,"file no live mode");request.trigger=TriggerMode::Software;
        request.sourceId=" ";r=service.acquire(probe,request);check(r.state==AcquisitionState::Error && !r.frame && probe.calls==0,"reject missing identity");request.sourceId="source";
        request.timeoutMs=0;r=service.acquire(probe,request);check(r.state==AcquisitionState::Error && probe.calls==0,"reject invalid deadline");request.timeoutMs=5000;
        request.cancel->store(true);r=service.acquire(probe,request);check(r.state==AcquisitionState::Cancelled && !r.frame && probe.calls==0,"cancel before acquisition");request.cancel->store(false);
        probe.stop=true;r=service.acquire(probe,request);check(r.state==AcquisitionState::Cancelled && !r.frame,"cancel after acquisition suppresses frame");probe.stop=false;request.cancel->store(false);
        probe.slow=true;request.timeoutMs=1;r=service.acquire(probe,request);check(r.state==AcquisitionState::Timeout && !r.frame,"late frame suppressed");probe.fail=true;r=service.acquire(probe,request);check(r.state==AcquisitionState::Timeout && !r.frame,"late failure timeout");probe.slow=false;request.timeoutMs=5000;
        r=service.acquire(probe,request);check(r.state==AcquisitionState::Error && !r.frame && r.error=="adapter failure","adapter error");probe.fail=false;probe.image={};
        r=service.acquire(probe,request);check(r.state==AcquisitionState::Error && !r.frame,"empty adapter frame rejected");
        r=service.acquire(ImageFileAdapter(dir.filePath("missing.png")),request);check(r.state==AcquisitionState::Error && !r.frame,"missing file no stale frame");
        r=service.acquire(file,request);check(r.state==AcquisitionState::Done && r.frame,"recovery after failures");
        QTextStream(stdout)<<"PASS: acquisition capabilities, identity, cancellation, deadline and recovery\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}
}
