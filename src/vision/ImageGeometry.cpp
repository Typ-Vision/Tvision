#include "ImageGeometry.hpp"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace visionflow {
namespace {
void requirePoint(QPointF p,QSize size) {
    if(size.isEmpty() || !std::isfinite(p.x()) || !std::isfinite(p.y()) ||
       p.x()<0 || p.y()<0 || p.x()>size.width()-1 || p.y()>size.height()-1)
        throw std::runtime_error("Image geometry pixel centre outside footprint");
}
QByteArray digest(const QJsonObject& value) {
    return QCryptographicHash::hash(QJsonDocument(value).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);
}
QJsonArray sizeJson(QSize size) {return {size.width(),size.height()};}
}
ImageGeometry ImageGeometry::root(QSize size,const QString& sourceId,const QString& coordinateId) {
    if(size.isEmpty())throw std::runtime_error("Invalid image root size");
    ImageGeometry result;result.rootSize_=result.rasterSize_=size;
    result.sourceId_=sourceId;result.rootCoordinateSystemId_=result.coordinateSystemId_=coordinateId;
    result.identity_=digest({{"schema","image-root/1"},{"source",sourceId},{"coordinate",coordinateId},{"size",sizeJson(size)}});
    return result;
}
ImageGeometry ImageGeometry::cropped(const QRect& rect) const {
    // QRect's right/bottom are inclusive; check exclusive extents in wide arithmetic.
    const qint64 x=rect.x(),y=rect.y();
    // Subtract the stored endpoints in wide arithmetic too: an adversarial QRect
    // can span INT_MIN..INT_MAX, for which QRect::width() itself would overflow.
    const qint64 w=qint64(rect.right())-x+1,h=qint64(rect.bottom())-y+1;
    const qint64 ox=qint64(rootOffset_.x())+x,oy=qint64(rootOffset_.y())+y;
    if(identity_.size()!=32 || x<0 || y<0 || w<=0 || h<=0 ||
       x+w>rasterSize_.width() || y+h>rasterSize_.height() || cropDepth_>=32 ||
       ox<0 || oy<0 || ox>std::numeric_limits<int>::max() || oy>std::numeric_limits<int>::max() ||
       ox+w>rootSize_.width() || oy+h>rootSize_.height())
        throw std::runtime_error("Invalid image crop geometry/footprint");
    auto child=*this;child.rasterSize_={int(w),int(h)};child.rootOffset_={int(ox),int(oy)};++child.cropDepth_;
    child.identity_=digest({{"schema","roi-crop/1"},{"parent",QString::fromLatin1(identity_.toHex())},
        {"parentCoordinate",coordinateSystemId_},{"parentSize",sizeJson(rasterSize_)},
        {"rect",QJsonArray{int(x),int(y),int(w),int(h)}},{"childSize",sizeJson(child.rasterSize_)}});
    child.coordinateSystemId_="roi-crop/"+QString::fromLatin1(child.identity_.toHex());
    return child;
}
QPointF ImageGeometry::toRoot(QPointF rasterPixel) const {
    requirePoint(rasterPixel,rasterSize_);const auto rootPixel=rasterPixel+QPointF(rootOffset_);
    requirePoint(rootPixel,rootSize_);return rootPixel;
}
QPointF ImageGeometry::fromRoot(QPointF rootPixel) const {
    requirePoint(rootPixel,rootSize_);const auto rasterPixel=rootPixel-QPointF(rootOffset_);
    requirePoint(rasterPixel,rasterSize_);return rasterPixel;
}
void ImageGeometry::requireFootprint(const QVector<QPointF>& rasterPixels) const {
    if(rasterPixels.isEmpty())throw std::runtime_error("Empty image footprint");
    for(const auto& pixel:rasterPixels)toRoot(pixel);
}
}
