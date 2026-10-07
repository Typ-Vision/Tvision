#include "ImageFrame.hpp"
#include <opencv2/imgproc.hpp>
#include <limits>
#include <cstring>
#include <stdexcept>
namespace visionflow {
ImageFrame ImageFrame::fromBuffer(const void* data,std::size_t bytes,int width,int height,std::size_t stride,PixelFormat format,const FrameMetadata& metadata,bool preview) {
    if(!data || width<=0 || height<=0 || std::uint64_t(width)*height>100000000)throw std::runtime_error("Frame boyutu/tamponu geçersiz.");
    int type=0,bpp=0;QString label;
    switch(format) {
    case PixelFormat::Gray8:type=CV_8UC1;bpp=1;label="Gray 8-bit";break;
    case PixelFormat::Gray16:type=CV_16UC1;bpp=2;label="Gray 16-bit (görünüm 0–65535)";break;
    case PixelFormat::Bgr8:case PixelFormat::Rgb8:type=CV_8UC3;bpp=3;label="RGB 8-bit";break;
    case PixelFormat::Bgra8:case PixelFormat::Rgba8:type=CV_8UC4;bpp=4;label="RGBA 8-bit";break;
    default:throw std::runtime_error("Desteklenmeyen frame piksel formatı.");
    }
    const auto row=std::size_t(width)*bpp;
    if(stride<row || (format==PixelFormat::Gray16 && stride%2) || std::size_t(height-1)>(std::numeric_limits<std::size_t>::max()-row)/stride || bytes<std::size_t(height-1)*stride+row)throw std::runtime_error("Frame stride/tampon uzunluğu geçersiz.");
    ImageFrame frame;frame.pixels_=cv::Mat(height,width,type);
    for(int y=0;y<height;++y)std::memcpy(frame.pixels_.ptr(y),static_cast<const unsigned char*>(data)+std::size_t(y)*stride,row);
    // Engine storage is native-endian Gray16 or OpenCV BGR/BGRA.
    if(format==PixelFormat::Rgb8)cv::cvtColor(frame.pixels_,frame.pixels_,cv::COLOR_RGB2BGR);
    if(format==PixelFormat::Rgba8)cv::cvtColor(frame.pixels_,frame.pixels_,cv::COLOR_RGBA2BGRA);
    frame.pixelFormat_=format==PixelFormat::Rgb8 ? PixelFormat::Bgr8 : format==PixelFormat::Rgba8 ? PixelFormat::Bgra8 : format;
    frame.format_=label;frame.metadata_=metadata;
    if(preview) {
        cv::Mat display;QImage::Format qformat=QImage::Format_Grayscale8;
        if(type==CV_8UC1)display=frame.pixels_;
        else if(type==CV_16UC1)frame.pixels_.convertTo(display,CV_8U,255.0/65535.0);
        else {cv::cvtColor(frame.pixels_,display,bpp==3 ? cv::COLOR_BGR2RGB : cv::COLOR_BGRA2RGBA);qformat=bpp==3 ? QImage::Format_RGB888 : QImage::Format_RGBA8888;}
        frame.preview_=QImage(display.data,width,height,static_cast<int>(display.step),qformat).copy();
        if(frame.preview_.isNull())throw std::runtime_error("Görüntü önizlemesi oluşturulamadı.");
    }return frame;
}
ImageFrame ImageFrame::withMetadata(const FrameMetadata& metadata) const {auto copy=*this;copy.metadata_=metadata;return copy;}
QString ImageFrame::pixelText(int x,int y) const {
    if(x<0 || y<0 || x>=pixels_.cols || y>=pixels_.rows)return {};
    if(pixels_.type()==CV_8UC1)return "Gray: "+QString::number(pixels_.at<uchar>(y,x));
    if(pixels_.type()==CV_16UC1)return "Gray: "+QString::number(pixels_.at<ushort>(y,x));
    if(pixels_.channels()==3){auto p=pixels_.at<cv::Vec3b>(y,x);return QString("R:%1 G:%2 B:%3").arg(p[2]).arg(p[1]).arg(p[0]);}
    auto p=pixels_.at<cv::Vec4b>(y,x);return QString("R:%1 G:%2 B:%3 A:%4").arg(p[2]).arg(p[1]).arg(p[0]).arg(p[3]);
}
}
