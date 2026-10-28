#pragma once
#include "ImageFrame.hpp"
#include <span>
namespace visionflow {
// Vision boundary: no mutable Mat header/view of a frame is exposed.
// The access object owns a shared reference; row spans live as long as it does.
class ImageAccess {
public:
    explicit ImageAccess(ImageFrame frame):frame_(std::move(frame)){}
    std::span<const std::byte> row(int y) const;
    cv::Mat ownedCopy() const;
    static ImageFrame cropped(const ImageFrame& input,const QRect& rect,bool preview=true);
    static ImageFrame processed(const ImageFrame& input,const cv::Mat& output,PixelFormat format,
                               const QString& method,int version,const QByteArray& parametersId,bool preview=true);
private:
    ImageFrame frame_;
};
}
