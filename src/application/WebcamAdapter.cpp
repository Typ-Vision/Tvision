#include "WebcamAdapter.hpp"
#include <opencv2/imgproc.hpp>
#include <QCamera>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QEventLoop>
#include <QTimer>
#include <QMutex>
#include <stdexcept>
namespace visionflow {
namespace {
QMutex captureMutex;
bool stopped(const AcquisitionRequest& r) {return (r.cancel && r.cancel->load()) || (r.deadline && r.deadline->expired());}
class QtCapture final : public WebcamCapture {
    AcquisitionRequest request_;
    std::unique_ptr<QCamera> camera_;
    QMediaCaptureSession session_;
    QVideoSink sink_;
public:
    explicit QtCapture(AcquisitionRequest r):request_(std::move(r)){}
    ~QtCapture() override {if(camera_)camera_->stop();session_.setCamera(nullptr);session_.setVideoSink(nullptr);}
    bool open(const WebcamSettings& s) override {
        const auto devices=QMediaDevices::videoInputs();
        if(s.deviceIndex>=devices.size())return false;
        camera_=std::make_unique<QCamera>(devices[s.deviceIndex]);
        session_.setCamera(camera_.get());session_.setVideoSink(&sink_);return camera_->isAvailable();
    }
    bool read(cv::Mat& pixels) override {
        QEventLoop loop;QTimer poll;poll.setInterval(10);QString error;
        QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(stopped(request_))loop.quit();});
        QObject::connect(camera_.get(),&QCamera::errorOccurred,&loop,[&](QCamera::Error,const QString& text){error=text;loop.quit();});
        QObject::connect(&sink_,&QVideoSink::videoFrameChanged,&loop,[&](const QVideoFrame& frame){
            if(stopped(request_)){loop.quit();return;}
            if(!pixels.empty())return;
            try {
                const auto image=frame.toImage().convertToFormat(QImage::Format_RGB888);
                if(image.isNull())return;
                cv::Mat rgb(image.height(),image.width(),CV_8UC3,const_cast<uchar*>(image.constBits()),image.bytesPerLine());
                cv::cvtColor(rgb,pixels,cv::COLOR_RGB2BGR);
            }catch(const std::exception& e){error=QString::fromUtf8(e.what());}
            loop.quit();
        });
        poll.start();camera_->start();
        if(camera_->error()==QCamera::NoError && pixels.empty() && !stopped(request_))loop.exec();
        camera_->stop();
        if(!error.isEmpty())throw std::runtime_error(error.toUtf8().constData());
        return !pixels.empty();
    }
};
}
void WebcamSettings::validate() const {
    if(deviceIndex<0 || deviceIndex>63 || backend!="qt")throw std::runtime_error("Webcam index 0–63 ve backend qt olmalı.");
}
WebcamAdapter::WebcamAdapter(WebcamSettings settings,WebcamCaptureFactory factory):settings_(std::move(settings)),factory_(std::move(factory)) {settings_.validate();}
AcquisitionCapabilities WebcamAdapter::capabilities() const {return {"webcam",true,false,false,false,false};}
ImageFrame WebcamAdapter::acquire(const AcquisitionRequest& request) const {
    if(request.trigger!=TriggerMode::Software)throw std::runtime_error("Webcam yalnız host tek kare isteğini destekler.");
    if(request.timeoutMs<=0)throw std::runtime_error("Webcam timeout pozitif olmalı.");
    auto effective=request;
    effective.deadline=request.deadline ? request.deadline->limitedTo(request.timeoutMs) : Deadline(request.timeoutMs);
    if(stopped(effective))return {};
    // Do not wait behind another blocking device operation.
    if(!captureMutex.tryLock())throw std::runtime_error("Webcam başka edinim tarafından kullanılıyor.");
    struct Unlock {~Unlock(){captureMutex.unlock();}} unlock;
    auto capture=factory_ ? factory_() : std::make_unique<QtCapture>(effective);
    if(!capture || !capture->open(settings_))throw std::runtime_error("Webcam açılamadı; cihaz index, backend ve Windows kamera iznini kontrol edin.");
    if(stopped(effective))return {};
    cv::Mat pixels;
    if(!capture->read(pixels) || pixels.empty())throw std::runtime_error("Webcam boş/başarısız kare döndürdü.");
    if(stopped(effective))return {};
    PixelFormat format;
    if(pixels.type()==CV_8UC1)format=PixelFormat::Gray8;
    else if(pixels.type()==CV_8UC3)format=PixelFormat::Bgr8;
    else if(pixels.type()==CV_8UC4)format=PixelFormat::Bgra8;
    else throw std::runtime_error("Webcam piksel formatı desteklenmiyor.");
    // Copy before capture destruction/release; no driver-owned storage escapes.
    return ImageFrame::fromBuffer(pixels.data,(pixels.rows-1)*pixels.step+pixels.cols*pixels.elemSize(),pixels.cols,pixels.rows,pixels.step,format);
}
}
