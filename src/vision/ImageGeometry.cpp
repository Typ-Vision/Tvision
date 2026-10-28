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
QJsonObject ImageGeometry::toJson() const {
    QJsonObject o{{"schemaVersion",1},{"sourceId",sourceId_},{"rootCoordinates",rootCoordinateSystemId_},
        {"coordinates",coordinateSystemId_},{"rootWidth",rootSize_.width()},{"rootHeight",rootSize_.height()},
        {"width",rasterSize_.width()},{"height",rasterSize_.height()},{"offsetX",rootOffset_.x()},{"offsetY",rootOffset_.y()},
        {"identity",QString::fromLatin1(identity_.toHex())},{"depth",cropDepth_}};
    if(cropDepth_){QJsonArray chain;for(const auto& r:crops_)chain.append(QJsonArray{r.x(),r.y(),r.width(),r.height()});o["crops"]=chain;}return o;
}
ImageGeometry ImageGeometry::fromJson(const QJsonObject& o) {
    auto require=[](bool b){if(!b)throw std::runtime_error("Invalid image geometry scope");};
    QStringList keys{"schemaVersion","sourceId","rootCoordinates","coordinates","rootWidth","rootHeight","width","height","offsetX","offsetY","identity","depth"};
    if(o.contains("crops"))keys.append("crops");
    require(o.size()==keys.size());for(auto i=o.begin();i!=o.end();++i)require(keys.contains(i.key()));
    auto integer=[&](const char* k,int lo,int hi){const auto v=o[k];require(v.isDouble() && v.toDouble()==v.toInt() && v.toInt()>=lo && v.toInt()<=hi);return v.toInt();};
    auto text=[&](const char* k){require(o[k].isString() && !o[k].toString().isEmpty() && o[k].toString().size()<=256);return o[k].toString();};
    require(integer("schemaVersion",1,1)==1);ImageGeometry g;
    g.sourceId_=text("sourceId");g.rootCoordinateSystemId_=text("rootCoordinates");g.coordinateSystemId_=text("coordinates");
    g.rootSize_={integer("rootWidth",1,16777216),integer("rootHeight",1,16777216)};
    g.rasterSize_={integer("width",1,g.rootSize_.width()),integer("height",1,g.rootSize_.height())};
    require(qint64(g.rootSize_.width())*g.rootSize_.height()<=16777216);
    g.rootOffset_={integer("offsetX",0,g.rootSize_.width()-g.rasterSize_.width()),integer("offsetY",0,g.rootSize_.height()-g.rasterSize_.height())};
    g.cropDepth_=integer("depth",0,32);const auto hex=text("identity").toLatin1();g.identity_=QByteArray::fromHex(hex);
    require(g.identity_.size()==32 && g.identity_.toHex()==hex);
    if(!g.cropDepth_)require(!o.contains("crops") && g==root(g.rootSize_,g.sourceId_,g.rootCoordinateSystemId_));
    else {
        require(g.coordinateSystemId_=="roi-crop/"+QString::fromLatin1(hex) && !g.rootCoordinateSystemId_.startsWith("roi-crop/"));
        require(o["crops"].isArray() && o["crops"].toArray().size()==g.cropDepth_);
        auto replay=root(g.rootSize_,g.sourceId_,g.rootCoordinateSystemId_);
        for(const auto& v:o["crops"].toArray()){
            require(v.isArray() && v.toArray().size()==4);const auto a=v.toArray();
            for(const auto& n:a)require(n.isDouble() && n.toDouble()==n.toInt() && n.toInt()>=0 && n.toInt()<=16777216);
            replay=replay.cropped(QRect(a[0].toInt(),a[1].toInt(),a[2].toInt(),a[3].toInt()));
        }
        g.crops_=replay.crops_;require(g==replay);
    }
    return g;
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
    auto child=*this;child.rasterSize_={int(w),int(h)};child.rootOffset_={int(ox),int(oy)};++child.cropDepth_;child.crops_.append(QRect(int(x),int(y),int(w),int(h)));
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
