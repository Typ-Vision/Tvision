#pragma once
#include <QGraphicsView>
#include <functional>
class QGraphicsPixmapItem;
class QGraphicsRectItem;
namespace visionflow {
class ImageViewer : public QGraphicsView {
public:
    explicit ImageViewer(QWidget* parent=nullptr);
    void setImage(const QImage& image);
    void clearImage(const QString& message="Görüntü yüklenmedi.");
    void fitImage();
    void zoomBy(double factor);
    void actualSize();
    double zoomFactor() const { return transform().m11(); }
    QPoint imagePixelAt(const QPoint& viewportPoint) const;
    bool hasImage() const { return imageItem_!=nullptr; }
    void setSourceSelected(bool selected);
    void drawRegion(bool enabled);
    void setRegions(const QRect& model,const QRect& search,const QRect& result={},bool accepted=true);
    void setToolOverlay(const QVector<QRect>& regions,const QVector<QPointF>& points,const QVector<QLineF>& lines={});
    std::function<void(QRect)> onRegion;
    std::function<void(QPoint)> onPixel;
    std::function<void()> onImageClicked;
    std::function<void(double)> onZoom;
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
private:
    QGraphicsScene* scene_{};
    QGraphicsPixmapItem* imageItem_{};
    QGraphicsRectItem* selection_{};
    QPoint pressed_;
    bool fitMode_=true;
    QString emptyMessage_="Görüntü yüklenmedi.";
    bool drawing_=false;
    QPoint regionStart_{-1,-1};
    QGraphicsRectItem* modelRegion_{};
    QGraphicsRectItem* searchRegion_{};
    QGraphicsRectItem* resultRegion_{};
    QGraphicsRectItem* draftRegion_{};
    QList<QGraphicsItem*> toolOverlay_;
};
}
