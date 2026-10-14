#pragma once
#include <QGraphicsView>
#include <functional>
#include "domain/ToolRoi.hpp"
#include "domain/ToolValues.hpp"
#include <optional>
class QGraphicsPixmapItem;
class QGraphicsRectItem;
namespace visionflow {
class ImageViewer : public QGraphicsView {
public:
    explicit ImageViewer(QWidget* parent=nullptr);
    void setImage(const QImage& image);
    // Same-geometry result preview; preserve raw-coordinate ROI, zoom and pan.
    void setPreviewPixels(const QImage& image);
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
    void setToolOverlay(const QVector<QRect>& regions,const QVector<QPointF>& points,const QVector<QLineF>& lines={},const QString& annotation={},const QByteArray& mask={},const QSize& maskSize={},const QPoint& maskOrigin={},const QVector<QPointF>& rejected={},bool densePoints=false);
    void setCodeOverlay(const CodeResults& codes);
    void setChessboardOverlay(const QVector<QPointF>& points,int columns);
    // Geometry workspace; legacy integer rectangle teaching remains separate.
    void beginRoi(int shapeIndex);
    void setRoi(const std::optional<ToolRoi>& roi,bool remember=true);
    const std::optional<ToolRoi>& roi() const {return roi_;}
    void setRoiRecipeBound(bool bound);
    void setRoiEditable(bool editable);
    void setRoiFrame(const QString& id);
    void setRoiMapping(const CoordinateFrame&,std::function<QPointF(QPointF)> toRaw={},std::function<QPointF(QPointF)> fromRaw={});
    void cancelRoi();
    bool roiGestureActive() const {return roiDrawing_>=0 || roiDragging_;}
    bool drawingActive() const {return drawing_ || roiGestureActive();}
    void undoRoi();
    void resetRoiHistory() {roiUndo_.clear();}
    void setValidDomain(const QVector<QPointF>& polygon);
    QString roiStatus() const {return roiStatus_;}
    std::function<void()> onRoiChanged;
    // User geometry changes only; frame presentation resets never write recipes.
    std::function<void()> onRoiEdited;
    std::function<void()> onRoiStatus;
    std::function<void(QRect)> onRegion;
    std::function<void(QPoint)> onPixel;
    std::function<void()> onImageClicked;
    std::function<void(QPointF)> onPointClicked;
    std::function<void(double)> onZoom;
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
private:
    bool roiPress(QMouseEvent*);
    bool roiMove(QMouseEvent*);
    bool roiRelease(QMouseEvent*);
    void renderRoi();
    void finishRoi();
    void commitRoiGeometry(const ToolRoi&);
    QVector<QPointF> roiHandles(const ToolRoi&) const;
    ToolRoi moveRoiHandle(const ToolRoi&,int,QPointF) const;
    std::optional<ToolRoi> roi_,roiDraft_,roiBeforeDrag_;
    QVector<std::optional<ToolRoi>> roiUndo_;
    QList<QGraphicsItem*> roiItems_;
    QVector<QPointF> roiVertices_,validDomain_;
    CoordinateFrame roiCoordinates_{"viewer/raw"};
    std::function<QPointF(QPointF)> roiToRaw_,roiFromRaw_;
    QString roiFrame_="viewer/raw",roiStatus_;
    int roiDrawing_=-1,roiHandle_=-1;
    bool roiRecipeBound_=false;
    bool roiDragging_=false,roiEditing_=false;
    QPointF roiStart_;
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
