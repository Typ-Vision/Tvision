#include "ImageAccess.hpp"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <stdexcept>
namespace visionflow {
std::span<const std::byte> ImageAccess::row(int y) const {
    if(y<0 || y>=frame_.pixels_.rows)throw std::runtime_error("Image row out of bounds");
    return {reinterpret_cast<const std::byte*>(frame_.pixels_.ptr(y)),std::size_t(frame_.pixels_.cols)*frame_.pixels_.elemSize()};
}
cv::Mat ImageAccess::ownedCopy() const {return frame_.pixels_.clone();}
ImageFrame ImageAccess::cropped(const ImageFrame& input,const QRect& rect,bool preview) {
    if(input.size().isEmpty() || input.processing_.size()>=32)
        throw std::runtime_error("Invalid crop image/processing chain");
    const auto geometry=input.geometry_.cropped(rect);
    auto metadata=input.metadata_;metadata.coordinateSystemId=geometry.coordinateSystemId();
    const auto row=std::size_t(rect.width())*input.pixels_.elemSize();
    const auto* first=input.pixels_.ptr(rect.y())+std::size_t(rect.x())*input.pixels_.elemSize();
    auto result=ImageFrame::fromBuffer(first,std::size_t(rect.height()-1)*input.pixels_.step+row,
        rect.width(),rect.height(),input.pixels_.step,input.format(),metadata,preview);
    result.geometry_=geometry;
    result.processing_=input.processing_;
    result.processing_.append({input.contentId(),geometry.identity(),"roi-crop",1});
    return result;
}
ImageFrame ImageAccess::processed(const ImageFrame& input,const cv::Mat& output,PixelFormat format,const QString& method,int version,const QByteArray& parametersId,bool preview) {
    if(input.size().isEmpty() || output.empty() || QSize(output.cols,output.rows)!=input.size() || method.isEmpty() || method.size()>128 || version<1 || version>1000000 || parametersId.size()!=32 || input.processing_.size()>=32)
        throw std::runtime_error("Invalid processing provenance/geometry/chain");
    const int expected=format==PixelFormat::Gray8 ? CV_8UC1 : format==PixelFormat::Gray16 ? CV_16UC1 : (format==PixelFormat::Bgr8 || format==PixelFormat::Rgb8) ? CV_8UC3 : (format==PixelFormat::Bgra8 || format==PixelFormat::Rgba8) ? CV_8UC4 : -1;
    if(output.type()!=expected)throw std::runtime_error("Processed pixel type/format mismatch");
    auto result=ImageFrame::fromBuffer(output.data,(output.rows-1)*output.step+output.cols*output.elemSize(),output.cols,output.rows,output.step,format,input.metadata_,preview);
    result.geometry_=input.geometry_;
    result.processing_=input.processing_;result.processing_.append({input.contentId(),parametersId,method,version});return result;
}
QByteArray ImageFrame::processingId() const {
    if(processing_.isEmpty())return {};
    QJsonArray steps;for(const auto& s:processing_)steps<<QJsonObject{{"input",QString::fromLatin1(s.inputContentId.toHex())},{"parameters",QString::fromLatin1(s.parametersId.toHex())},{"method",s.method},{"version",s.version}};
    return QCryptographicHash::hash(QJsonDocument(steps).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);
}
}
