#include "ImageViewer.hpp"
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QScrollBar>
#include <cmath>
#include <algorithm>
namespace visionflow {
namespace {
QRect pixelRectangle(QPoint a,QPoint b) {return QRect(QPoint(std::min(a.x(),b.x()),std::min(a.y(),b.y())),QPoint(std::max(a.x(),b.x()),std::max(a.y(),b.y())));}
}
ImageViewer::ImageViewer(QWidget* parent) : QGraphicsView(parent),scene_(new QGraphicsScene(this)) {
    setScene(scene_); setBackgroundBrush(QColor("#101923")); setDragMode(ScrollHandDrag);
    setTransformationAnchor(AnchorUnderMouse); setResizeAnchor(AnchorViewCenter);
    setMouseTracking(true); setAccessibleName("Image Viewer"); setMinimumSize(200,180);
    setRenderHint(QPainter::SmoothPixmapTransform,false);
}
void ImageViewer::setPreviewPixels(const QImage& image) {
    if(!imageItem_ || imageItem_->pixmap().size()!=image.size()){setImage(image);return;}
    imageItem_->setPixmap(QPixmap::fromImage(image));
}
void ImageViewer::setImage(const QImage& image) {
    searchHandles_.clear();searchIdentity_.clear();searchUndo_.clear();searchEditable_=searchDragging_=middlePanning_=false;searchValue_={};
    roiItems_.clear();roi_.reset();roiDraft_.reset();roiBeforeDrag_.reset();roiStatus_="Geometri yok.";roiUndo_.clear();roiVertices_.clear();validDomain_.clear();roiDrawing_=-1;roiHandle_=-1;roiDragging_=roiEditing_=false;
    resultAnnotations_.clear();toolOverlay_.clear();
    scene_->clear(); imageItem_=nullptr; selection_=nullptr;
    modelRegion_=searchRegion_=resultRegion_=draftRegion_=nullptr; drawRegion(false);
    if (image.isNull()) { clearImage(); return; }
    imageItem_=scene_->addPixmap(QPixmap::fromImage(image));
    const QRectF rect(0,0,image.width(),image.height()); scene_->setSceneRect(rect);
    QPen pen(QColor("#66e3c4")); pen.setWidth(2); pen.setCosmetic(true);
    selection_=scene_->addRect(rect,pen); selection_->setVisible(false);
    auto region=[this](const QColor& color){QPen p(color); p.setWidth(2); p.setCosmetic(true); auto* item=scene_->addRect({},p); item->hide(); return item;};
    modelRegion_=region(QColor("#ffd166")); searchRegion_=region(QColor("#56b4ff")); resultRegion_=region(QColor("#66e3c4")); draftRegion_=region(Qt::white);
    fitImage();if(onRoiChanged)onRoiChanged();
}
void ImageViewer::clearImage(const QString& message) {
    searchHandles_.clear();searchIdentity_.clear();searchUndo_.clear();searchEditable_=searchDragging_=middlePanning_=false;searchValue_={};
    roiItems_.clear();roi_.reset();roiDraft_.reset();roiBeforeDrag_.reset();roiStatus_="Geometri yok.";roiUndo_.clear();roiVertices_.clear();validDomain_.clear();roiDrawing_=-1;roiHandle_=-1;roiDragging_=roiEditing_=false;
    resultAnnotations_.clear();toolOverlay_.clear();
    scene_->clear(); scene_->setSceneRect({}); imageItem_=nullptr; selection_=nullptr;
    emptyMessage_=message; resetTransform(); viewport()->update();
    modelRegion_=searchRegion_=resultRegion_=draftRegion_=nullptr; drawRegion(false);
    if (onZoom) onZoom(1.0); if (onPixel) onPixel({-1,-1});if(onRoiChanged)onRoiChanged();
}
void ImageViewer::fitImage() {
    if (!imageItem_) return;
    fitMode_=true; fitInView(imageItem_->boundingRect(),Qt::KeepAspectRatio); updateResultAnnotations(); renderRoi(); if (onZoom) onZoom(zoomFactor());
}
void ImageViewer::zoomBy(double factor) {
    if (!imageItem_ || !std::isfinite(factor) || factor<=0) return;
    fitMode_=false; const double target=std::clamp(zoomFactor()*factor,0.001,32.0);
    scale(target/zoomFactor(),target/zoomFactor()); updateResultAnnotations(); renderRoi(); if (onZoom) onZoom(zoomFactor());
}
void ImageViewer::actualSize() {
    if (!imageItem_) return;
    fitMode_=false; resetTransform(); updateResultAnnotations(); renderRoi(); if (onZoom) onZoom(1.0);
}
QPoint ImageViewer::imagePixelAt(const QPoint& position) const {
    if (!imageItem_) return {-1,-1};
    const QPointF point=mapToScene(position);
    if (!imageItem_->boundingRect().contains(point) || point.x()>=imageItem_->pixmap().width() || point.y()>=imageItem_->pixmap().height()) return {-1,-1};
    return {static_cast<int>(std::floor(point.x())),static_cast<int>(std::floor(point.y()))};
}
void ImageViewer::setSourceSelected(bool selected) { if (selection_) selection_->setVisible(selected); }
void ImageViewer::drawRegion(bool enabled) {
    if(enabled){cancelRoi();roiEditing_=false;renderRoi();}
    drawing_=enabled; regionStart_={-1,-1}; setDragMode(enabled ? NoDrag : ScrollHandDrag);
    viewport()->setCursor(enabled ? Qt::CrossCursor : Qt::OpenHandCursor);
    if (draftRegion_) draftRegion_->hide();
    renderSearchRegion();
}
void ImageViewer::setRegions(const QRect& model,const QRect& search,const QRect& result,bool accepted) {
    auto set=[](QGraphicsRectItem* item,const QRect& rect){if (item) {item->setRect(QRectF(rect)); item->setVisible(!rect.isEmpty());}};
    set(modelRegion_,model); set(searchRegion_,search); set(resultRegion_,result);
    if (resultRegion_) {auto pen=resultRegion_->pen(); pen.setColor(accepted ? QColor("#66e3c4") : QColor("#ff8585")); resultRegion_->setPen(pen);}
    renderSearchRegion();
}
void ImageViewer::setToolOverlay(const QVector<QRect>& regions,const QVector<QPointF>& points,const QVector<QLineF>& lines,const QString& annotation,const QByteArray& mask,const QSize& maskSize,const QPoint& maskOrigin,const QVector<QPointF>& rejected,bool densePoints) {
    resultAnnotations_.clear();
    for(auto* item:toolOverlay_) {scene_->removeItem(item);delete item;}toolOverlay_.clear();
    if(!imageItem_)return;
    QPen pen(QColor("#66e3c4"));pen.setWidth(2);pen.setCosmetic(true);
    if(!mask.isEmpty() && maskSize.width()>0 && maskSize.height()>0 && qint64(maskSize.width())*maskSize.height()==mask.size()) {
        QImage colored(maskSize,QImage::Format_ARGB32_Premultiplied);colored.fill(Qt::transparent);
        for(int y=0;y<maskSize.height();++y){auto* row=reinterpret_cast<QRgb*>(colored.scanLine(y));for(int x=0;x<maskSize.width();++x)if(mask[y*maskSize.width()+x]){
            auto at=[&](int xx,int yy){return xx>=0 && yy>=0 && xx<maskSize.width() && yy<maskSize.height() && mask[yy*maskSize.width()+xx];};
            const bool border=!at(x-1,y) || !at(x+1,y) || !at(x,y-1) || !at(x,y+1);
            row[x]=qPremultiply(qRgba(255,40,40,border ? 210 : 100));
        }}
        auto* overlay=scene_->addPixmap(QPixmap::fromImage(colored));overlay->setPos(maskOrigin);toolOverlay_<<overlay;
        addResultLabel("Kırmızı: kabul edilen\nbölge (karar değildir)",maskOrigin,QColor("#ff6666"));
    }
    for(const auto& rect:regions)toolOverlay_<<scene_->addRect(QRectF(rect),pen);
    for(const auto& line:lines)toolOverlay_<<scene_->addLine(line,pen);
    for(int i=0;i<points.size();++i) {
        const auto p=points[i];
        const double radius=densePoints ? 3:5;auto* marker=scene_->addEllipse(-radius,-radius,2*radius,2*radius,pen,QBrush(QColor("#101923")));
        marker->setPos(p);marker->setFlag(QGraphicsItem::ItemIgnoresTransformations);toolOverlay_<<marker;
        if(densePoints)continue;
        addResultLabel(QString("P%1").arg(i+1),p,QColor("#ffd166"));
    }
    QPen rejectedPen(QColor("#ff6666"));rejectedPen.setWidth(2);rejectedPen.setCosmetic(true);
    for(const auto& p:rejected){auto* marker=scene_->addEllipse(-6,-6,12,12,rejectedPen);marker->setPos(p);marker->setFlag(QGraphicsItem::ItemIgnoresTransformations);toolOverlay_<<marker;addResultLabel("outlier",p,QColor("#ff6666"));}
    if(points.size()!=2 && !annotation.isEmpty()){
        const auto at=!lines.isEmpty() ? lines.first().p1()+QPointF(densePoints ? 20:0,densePoints ? 0:12):QPointF{};
        addResultLabel(annotation,at,QColor("#ffd166"),true);
    }
    if(points.size()==2) {
        QPen distancePen(QColor("#ffd166"));distancePen.setWidth(3);distancePen.setCosmetic(true);
        toolOverlay_<<scene_->addLine(QLineF(points[0],points[1]),distancePen);
        addResultLabel(annotation.isEmpty() ? QString("%1 px").arg(QLineF(points[0],points[1]).length(),0,'f',2) : annotation,(points[0]+points[1])/2,QColor("#ffd166"));
    }
}
void ImageViewer::setResultAnnotationsVisible(bool visible) {
    annotationsManaged_=true;annotationsVisible_=visible;updateResultAnnotations();
}
void ImageViewer::updateResultAnnotations() {
    if(!annotationsManaged_)return;
    for(auto* item:resultAnnotations_){item->setVisible(annotationsVisible_);item->setScale(std::min(zoomFactor(),1.5));}
}
void ImageViewer::addResultLabel(const QString& text,QPointF at,const QColor& color,bool background) {
    auto* label=scene_->addSimpleText(text);label->setBrush(color);label->setPos(at);
    label->setFlag(QGraphicsItem::ItemIgnoresTransformations);if(background)label->setZValue(6);
    if(annotationsManaged_){auto font=label->font();font.setPointSizeF(11);label->setFont(font);}
    if(background){
        auto* box=scene_->addRect(label->boundingRect().adjusted(-3,-2,3,2),Qt::NoPen,QBrush(QColor(16,25,35,235)));
        box->setPos(at);box->setFlag(QGraphicsItem::ItemIgnoresTransformations);box->setZValue(5);
        toolOverlay_<<box;resultAnnotations_<<box;
    }
    toolOverlay_<<label;resultAnnotations_<<label;updateResultAnnotations();
}
void ImageViewer::setCodeOverlay(const CodeResults& codes) {
    if(!imageItem_)return;
    QPen pen(QColor("#66e3c4"));pen.setWidth(2);pen.setCosmetic(true);
    for(int i=0;i<codes.items.size();++i){const auto& code=codes.items[i];
        for(int j=0;j<code.corners.size();++j)toolOverlay_<<scene_->addLine(QLineF(code.corners[j],code.corners[(j+1)%code.corners.size()]),pen);
        if(!code.corners.isEmpty())addResultLabel(QString("#%1 %2").arg(i).arg(code.symbology),code.corners.first(),QColor("#ffd166"));
    }
}
void ImageViewer::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y()!=0) zoomBy(std::pow(1.2,event->angleDelta().y()/120.0));
    if (onPixel) onPixel(imagePixelAt(event->position().toPoint())); event->accept();
}
void ImageViewer::setChessboardOverlay(const QVector<QPointF>& points,int columns) {
    if(columns<2 || points.size()<columns*2)return;
    setToolOverlay({},points,{QLineF(points[0],points[columns-1]),QLineF(points[0],points[points.size()-columns])});
    for(const auto& entry:QVector<QPair<QPointF,QString>>{{points[0],"O (0,0)"},{points[columns-1],"+X"},{points[points.size()-columns],"+Y"}}){
        auto* label=scene_->addSimpleText(entry.second);label->setBrush(QColor("#56b4ff"));label->setPos(entry.first);label->setFlag(QGraphicsItem::ItemIgnoresTransformations);toolOverlay_<<label;
    }
}
void ImageViewer::mousePressEvent(QMouseEvent* event) {
    if(event->button()==Qt::MiddleButton && imageItem_){cancelRoi();middlePanning_=true;panStart_=event->position().toPoint();viewport()->setCursor(Qt::ClosedHandCursor);event->accept();return;}
    if(searchPress(event))return;
    if(roiPress(event))return;
    pressed_=event->position().toPoint(); QGraphicsView::mousePressEvent(event);
    if (drawing_ && event->button()==Qt::LeftButton) {regionStart_=imagePixelAt(pressed_); event->accept();}
}
void ImageViewer::mouseMoveEvent(QMouseEvent* event) {
    if(middlePanning_){const auto p=event->position().toPoint();const auto d=p-panStart_;horizontalScrollBar()->setValue(horizontalScrollBar()->value()-d.x());verticalScrollBar()->setValue(verticalScrollBar()->value()-d.y());panStart_=p;event->accept();return;}
    if(searchMove(event))return;
    if(roiMove(event))return;
    QGraphicsView::mouseMoveEvent(event);
    if (onPixel) onPixel(imagePixelAt(event->position().toPoint()));
    if (drawing_ && regionStart_.x()>=0 && (event->buttons() & Qt::LeftButton)) {
        auto point=mapToScene(event->position().toPoint());
        const QPoint end(std::clamp(int(std::floor(point.x())),0,imageItem_->pixmap().width()-1),std::clamp(int(std::floor(point.y())),0,imageItem_->pixmap().height()-1));
        draftRegion_->setRect(QRectF(pixelRectangle(regionStart_,end))); draftRegion_->show();
    }
}
void ImageViewer::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button()==Qt::MiddleButton && middlePanning_){middlePanning_=false;viewport()->setCursor(drawing_ ? Qt::CrossCursor:Qt::OpenHandCursor);event->accept();return;}
    if(searchRelease(event))return;
    if(roiRelease(event))return;
    if (drawing_ && event->button()==Qt::LeftButton) {
        if (regionStart_.x()>=0 && imageItem_) {
            auto point=mapToScene(event->position().toPoint());
            QPoint end(std::clamp(int(std::floor(point.x())),0,imageItem_->pixmap().width()-1),std::clamp(int(std::floor(point.y())),0,imageItem_->pixmap().height()-1));
            const QRect roi=pixelRectangle(regionStart_,end); drawRegion(false);
            if (roi.width()>=2 && roi.height()>=2){if(searchEditable_)finishSearchRegion(roi);else if(onRegion)onRegion(roi);}
        } else drawRegion(false);
        event->accept(); return;
    }
    QGraphicsView::mouseReleaseEvent(event);
    if(event->button()==Qt::LeftButton && (event->position().toPoint()-pressed_).manhattanLength()<4 && imagePixelAt(event->position().toPoint()).x()>=0 && onPointClicked)onPointClicked(mapToScene(event->position().toPoint()));
    if (event->button()==Qt::LeftButton && (event->position().toPoint()-pressed_).manhattanLength()<4 &&
        imagePixelAt(event->position().toPoint()).x()>=0 && onImageClicked) onImageClicked();
}
void ImageViewer::leaveEvent(QEvent* event) { if (onPixel) onPixel({-1,-1}); QGraphicsView::leaveEvent(event); }
void ImageViewer::resizeEvent(QResizeEvent* event) { QGraphicsView::resizeEvent(event); if (fitMode_) fitImage(); }
void ImageViewer::paintEvent(QPaintEvent* event) {
    QGraphicsView::paintEvent(event);
    if (!imageItem_) { QPainter painter(viewport()); painter.setPen(QColor("#8fa0b5")); painter.drawText(viewport()->rect().adjusted(18,18,-18,-18),Qt::AlignCenter|Qt::TextWordWrap,emptyMessage_); }
}
}
