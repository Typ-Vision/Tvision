#include "RectangleMask.hpp"
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace visionflow {
RectangleMask::RectangleMask(const ToolRoi& roi,const QSize& size,const std::shared_ptr<std::atomic_bool>& cancel){
    if(!std::holds_alternative<RotatedRectangle>(roi.geometry) || !roi.inside(size))throw std::runtime_error("Tracking rectangle outside raw image or wrong geometry");
    rectangle=std::get<RotatedRectangle>(roi.geometry);
    const auto a=rectangle.angleDegrees*std::numbers::pi/180;cosine=std::cos(a);sine=std::sin(a);
    const auto b=roi.bounds();bounds=QRect(QPoint(int(std::floor(b.left())),int(std::floor(b.top()))),QPoint(int(std::ceil(b.right())),int(std::ceil(b.bottom()))));
    if(qint64(bounds.width())*bounds.height()>16*1024*1024)throw std::runtime_error("Tracking mask exceeds 16 Mi pixels");
    pixels=cv::Mat(bounds.height(),bounds.width(),CV_8U,cv::Scalar(0));
    int included=0;
    for(int y=0;y<pixels.rows;++y){
        if(cancel && cancel->load())throw std::runtime_error("Tracking mask cancelled");
        auto* row=pixels.ptr<uchar>(y);
        for(int x=0;x<pixels.cols;++x)if(contains({double(x+bounds.x()),double(y+bounds.y())})){row[x]=255;++included;}
    }
    if(!included)throw std::runtime_error("Tracking rectangle contains no raw pixel centres");
}
bool RectangleMask::contains(QPointF raw) const {
    const auto p=raw-rectangle.centre;
    // Only numerical roundoff at the closed boundary (one billionth of a pixel).
    return std::abs(p.x()*cosine+p.y()*sine)<=rectangle.size.width()/2+1e-9 &&
           std::abs(-p.x()*sine+p.y()*cosine)<=rectangle.size.height()/2+1e-9;
}
}
