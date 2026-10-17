#pragma once
#include "AcquisitionService.hpp"
#include "domain/MeasurementSetupContract.hpp"
#include <functional>
namespace visionflow {
struct WebcamSettings {
    int deviceIndex=0;
    QString backend="qt";
    // Empty providerId preserves the explicit legacy index path only.
    QString providerId;
    QSize resolution;
    void validate() const;
};
// Created, opened, read and destroyed on the calling worker. Injection is for
// contract tests; production uses Qt Multimedia.
class WebcamCapture {
public:
    virtual ~WebcamCapture()=default;
    virtual bool open(const WebcamSettings&)=0;
    virtual bool read(cv::Mat&)=0;
};
using WebcamCaptureFactory=std::function<std::unique_ptr<WebcamCapture>()>;
class WebcamAdapter final : public AcquisitionAdapter {
public:
    explicit WebcamAdapter(WebcamSettings settings={},WebcamCaptureFactory factory={});
    AcquisitionCapabilities capabilities() const override;
    ImageFrame acquire(const AcquisitionRequest&) const override;
    static QVector<DeviceBinding> discover(); // metadata only; never opens a camera
private:
    WebcamSettings settings_;
    WebcamCaptureFactory factory_;
};
}
