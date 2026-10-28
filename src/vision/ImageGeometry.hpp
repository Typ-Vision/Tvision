#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>

namespace visionflow {
// Immutable pixel-centre translation from an owned raster to its acquisition root.
// Only ImageFrame/ImageAccess can construct a valid scope or attach it to pixels.
class ImageGeometry {
public:
    ImageGeometry()=default; // Empty ImageFrame only; mapping rejects it.
    QJsonObject toJson() const;
    static ImageGeometry fromJson(const QJsonObject&); // scope only; cannot attach pixels
    const QString& sourceId() const {return sourceId_;}
    QSize rootSize() const {return rootSize_;}
    QSize rasterSize() const {return rasterSize_;}
    const QString& rootCoordinateSystemId() const {return rootCoordinateSystemId_;}
    const QString& coordinateSystemId() const {return coordinateSystemId_;}
    QPoint rootOffset() const {return rootOffset_;}
    const QByteArray& identity() const {return identity_;}
    int cropDepth() const {return cropDepth_;}
    QPointF toRoot(QPointF rasterPixel) const;
    QPointF fromRoot(QPointF rootPixel) const;
    void requireFootprint(const QVector<QPointF>& rasterPixels) const;
    bool operator==(const ImageGeometry&) const =default;
private:
    friend class ImageFrame;
    friend class ImageAccess;
    static ImageGeometry root(QSize size,const QString& sourceId,const QString& coordinateId);
    ImageGeometry cropped(const QRect& rect) const;
    QSize rootSize_,rasterSize_;
    QString sourceId_,rootCoordinateSystemId_,coordinateSystemId_;
    QPoint rootOffset_;
    QByteArray identity_;
    int cropDepth_=0;
    QVector<QRect> crops_;
};
}
