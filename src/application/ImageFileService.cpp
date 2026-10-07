#include "ImageFileService.hpp"
#include <QFile>
#include <QFileInfo>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
namespace visionflow {
ImageFrame ImageFileService::load(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(("Görüntü açılamadı: "+file.errorString()).toUtf8().constData());
    if (file.size()<=0 || file.size()>256*1024*1024) throw std::runtime_error("Görüntü dosyası boş veya 256 MB sınırını aşıyor.");
    const auto bytes=file.readAll();
    if (file.error()!=QFileDevice::NoError) throw std::runtime_error("Görüntü dosyası okunamadı.");
    ImageFrame frame;
    try {
        const cv::Mat encoded(1,static_cast<int>(bytes.size()),CV_8UC1,const_cast<char*>(bytes.constData()));
        frame.pixels_=cv::imdecode(encoded,cv::IMREAD_UNCHANGED);
        if (frame.pixels_.empty()) throw std::runtime_error("Görüntü formatı tanınmadı veya dosya bozuk.");
        const auto& pixels=frame.pixels_;
        if (pixels.total()>100000000) throw std::runtime_error("Görüntü 100 MP sınırını aşıyor.");
        PixelFormat format;
        if(pixels.type()==CV_8UC1)format=PixelFormat::Gray8;
        else if(pixels.type()==CV_16UC1)format=PixelFormat::Gray16;
        else if(pixels.type()==CV_8UC3)format=PixelFormat::Bgr8;
        else if(pixels.type()==CV_8UC4)format=PixelFormat::Bgra8;
        else throw std::runtime_error("Bu piksel formatı desteklenmiyor. Gray8/Gray16/RGB8/RGBA8 kullanın.");
        frame=ImageFrame::fromBuffer(pixels.data,(pixels.rows-1)*pixels.step+pixels.cols*pixels.elemSize(),pixels.cols,pixels.rows,pixels.step,format);
    } catch (const cv::Exception& e) { throw std::runtime_error(("Görüntü decode hatası: "+QString::fromUtf8(e.what())).toUtf8().constData()); }
    frame.path_=QFileInfo(path).absoluteFilePath(); return frame;
}
}
