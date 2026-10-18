#include "ImageViewer.hpp"
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <type_traits>
#include <algorithm>
namespace visionflow {
void ImageViewer::setSearchRegionEditing(const QString& identity,const QRect& region,bool editable){
    if(searchEditable_ && !editable){cancelRoi();drawRegion(false);}
    const bool valid=region.isEmpty() || (imageItem_ && region.width()>=2 && region.height()>=2 && QRect(QPoint{},imageItem_->pixmap().size()).contains(region));
    if(identity!=searchIdentity_ || region!=searchValue_){cancelRoi();searchUndo_.clear();searchIdentity_=identity;searchValue_=region;}
    if(!editable || !valid){searchDragging_=false;searchDraft_={};}
    searchEditable_=editable && valid && imageItem_;renderSearchRegion();
}
QVector<QPointF> ImageViewer::searchHandles(const QRect& r) const {
    const QRectF f(r);const auto c=f.center();
    return {f.topLeft(),{c.x(),f.top()},f.topRight(),{f.right(),c.y()},f.bottomRight(),{c.x(),f.bottom()},f.bottomLeft(),{f.left(),c.y()}};
}
void ImageViewer::renderSearchRegion(){
    for(auto* item:searchHandles_){scene_->removeItem(item);delete item;}searchHandles_.clear();
    if(!searchEditable_ || !imageItem_ || !searchRegion_)return;
    const auto r=searchDragging_ ? searchDraft_:searchValue_;
    searchRegion_->setRect(QRectF(r));searchRegion_->setVisible(!r.isEmpty());
    if(r.isEmpty() || drawing_)return;
    QPen pen(QColor("#56b4ff"),2);pen.setCosmetic(true);
    for(auto p:searchHandles(r)){auto* h=scene_->addRect(-4,-4,8,8,pen,QBrush(QColor("#101923")));h->setPos(p);h->setFlag(QGraphicsItem::ItemIgnoresTransformations);h->setZValue(15);searchHandles_<<h;}
}
int ImageViewer::searchHit(const QPoint& p) const {
    if(!searchEditable_ || searchValue_.isEmpty() || drawing_)return -1;
    const auto handles=searchHandles(searchValue_);double best=9;int hit=-1;
    for(int i=0;i<handles.size();++i){const auto d=QLineF(QPointF(p),QPointF(mapFromScene(handles[i]))).length();if(d<best){best=d;hit=i;}}
    if(hit>=0)return hit;
    return QRectF(searchValue_).contains(mapToScene(p)) ? 8:-1;
}
bool ImageViewer::searchPress(QMouseEvent* e){
    if(e->button()!=Qt::LeftButton)return false;
    const auto hit=searchHit(e->position().toPoint());if(hit<0)return false;
    const auto starting=onSearchRegionStarting;if(starting && !starting()){e->accept();return true;}
    if(searchHit(e->position().toPoint())!=hit){e->accept();return true;}
    searchHandle_=hit;searchBefore_=searchValue_;searchDraft_=searchValue_;searchDragging_=true;
    const auto p=mapToScene(e->position().toPoint());searchStart_={int(std::floor(p.x())),int(std::floor(p.y()))};
    setFocus();setDragMode(NoDrag);if(onSearchRegionActivityChanged)onSearchRegionActivityChanged();e->accept();return true;
}
bool ImageViewer::searchMove(QMouseEvent* e){
    static const Qt::CursorShape cursors[]{Qt::SizeFDiagCursor,Qt::SizeVerCursor,Qt::SizeBDiagCursor,Qt::SizeHorCursor,Qt::SizeFDiagCursor,Qt::SizeVerCursor,Qt::SizeBDiagCursor,Qt::SizeHorCursor,Qt::SizeAllCursor};
    if(!searchDragging_){if(!drawing_ && roiDrawing_<0 && !roiDragging_){const int hit=searchHit(e->position().toPoint());viewport()->setCursor(hit>=0 ? cursors[hit]:Qt::OpenHandCursor);}return false;}
    const auto p=mapToScene(e->position().toPoint());const QPoint delta=QPoint(int(std::floor(p.x())),int(std::floor(p.y())))-searchStart_;
    const auto size=imageItem_->pixmap().size();auto r=searchBefore_;
    if(searchHandle_==8)r.translate(std::clamp(delta.x(),-r.left(),size.width()-1-r.right()),std::clamp(delta.y(),-r.top(),size.height()-1-r.bottom()));
    else {
        const int h=searchHandle_;
        if(h==0 || h==6 || h==7)r.setLeft(std::clamp(searchBefore_.left()+delta.x(),0,searchBefore_.right()-1));
        if(h==0 || h==1 || h==2)r.setTop(std::clamp(searchBefore_.top()+delta.y(),0,searchBefore_.bottom()-1));
        if(h==2 || h==3 || h==4)r.setRight(std::clamp(searchBefore_.right()+delta.x(),searchBefore_.left()+1,size.width()-1));
        if(h==4 || h==5 || h==6)r.setBottom(std::clamp(searchBefore_.bottom()+delta.y(),searchBefore_.top()+1,size.height()-1));
    }
    searchDraft_=r;renderSearchRegion();e->accept();return true;
}
void ImageViewer::finishSearchRegion(const QRect& value){
    if(value==searchValue_){renderSearchRegion();return;}
    searchUndo_<<searchValue_;if(searchUndo_.size()>64)searchUndo_.removeFirst();searchValue_=value;renderSearchRegion();
    const auto edited=onSearchRegionEdited;if(edited)edited(value);
}
bool ImageViewer::searchRelease(QMouseEvent* e){
    if(!searchDragging_ || e->button()!=Qt::LeftButton)return false;
    searchMove(e);const auto value=searchDraft_;cancelRoi();finishSearchRegion(value);e->accept();return true;
}
namespace {
QPointF polar(QPointF c,double r,double degrees){const auto a=degrees*std::numbers::pi/180;return c+QPointF(r*std::cos(a),r*std::sin(a));}
double angle(QPointF c,QPointF p){return std::atan2(p.y()-c.y(),p.x()-c.x())*180/std::numbers::pi;}
QPointF local(QPointF p,double degrees){const auto a=degrees*std::numbers::pi/180;return {p.x()*std::cos(a)+p.y()*std::sin(a),-p.x()*std::sin(a)+p.y()*std::cos(a)};}
void polygon(QPainterPath& path,const QVector<QPointF>& points,bool close){if(points.isEmpty())return;path.moveTo(points.first());for(int i=1;i<points.size();++i)path.lineTo(points[i]);if(close)path.closeSubpath();}
}
void ImageViewer::setRoiEditable(bool editable){roiEditing_=editable && roi_.has_value();renderRoi();}
void ImageViewer::setRoiRecipeBound(bool bound){roiRecipeBound_=bound;renderRoi();}
void ImageViewer::setRoiMapping(const CoordinateFrame& f,std::function<QPointF(QPointF)> toRaw,std::function<QPointF(QPointF)> fromRaw){roiToRaw_={};roiFromRaw_={};setRoiFrame(f.id);roiCoordinates_=f;roiToRaw_=std::move(toRaw);roiFromRaw_=std::move(fromRaw);}
void ImageViewer::setRoiFrame(const QString& id){roiToRaw_={};roiFromRaw_={};roiCoordinates_={id};CoordinateFrame{id}.validate();if(roiFrame_!=id){setRoi({});roiUndo_.clear();roiFrame_=id;}}
void ImageViewer::beginRoi(int kind){
    if(!imageItem_ || kind<0 || kind>5)return;
    drawRegion(false);cancelRoi();roiDrawing_=kind;roiEditing_=true;setFocus();setDragMode(NoDrag);viewport()->setCursor(Qt::CrossCursor);
    roiStatus_=kind>=4 ? "Noktaları tıklayın; Enter tamamlar, Backspace geri alır, Escape iptal eder.":"Sürükleyerek çizin; Escape iptal eder. Tutamaçlarla düzenleyin.";
    renderRoi();if(onRoiStatus)onRoiStatus();
}
void ImageViewer::setRoi(const std::optional<ToolRoi>& value,bool remember){
    if(value){value->validate();if(!imageItem_ || value->frame.id!=roiFrame_ || (roiToRaw_ ? false:!value->inside(imageItem_->pixmap().size())))throw std::runtime_error("ROI frame/image bounds mismatch");}
    if(value && roiToRaw_){for(auto p:roiHandles(*value))roiToRaw_(p);}
    if(value && roiToRaw_)for(auto p:roiHandles(*value))roiToRaw_(p);
    cancelRoi();if(value==roi_)return;
    if(remember){roiUndo_.append(roi_);if(roiUndo_.size()>64)roiUndo_.removeFirst();}
    roi_=value;roiEditing_=value.has_value();roiStatus_=value ? "Geçici ROI • raw px • reçeteye bağlı değil":"Geometri yok.";
    renderRoi();if(onRoiChanged)onRoiChanged();if(onRoiStatus)onRoiStatus();if(remember && onRoiEdited)onRoiEdited();
}
void ImageViewer::cancelRoi(){
    const bool searchWasDragging=searchDragging_;searchDragging_=false;searchDraft_={};renderSearchRegion();
    if(searchWasDragging && onSearchRegionActivityChanged)onSearchRegionActivityChanged();
    roiDraft_.reset();roiBeforeDrag_.reset();roiVertices_.clear();roiDrawing_=-1;roiHandle_=-1;roiDragging_=false;
    setDragMode(drawing_ ? NoDrag:ScrollHandDrag);viewport()->setCursor(drawing_ ? Qt::CrossCursor:Qt::OpenHandCursor);renderRoi();
}
void ImageViewer::undoRoi(){cancelRoi();if(roiUndo_.isEmpty())return;const auto value=roiUndo_.takeLast();setRoi(value,false);if(onRoiEdited)onRoiEdited();}
void ImageViewer::setValidDomain(const QVector<QPointF>& polygon){
    if(!polygon.isEmpty()){ToolRoi domain{{roiFrame_},PolygonRoi{polygon}};domain.validate();if(!imageItem_ || !domain.inside(imageItem_->pixmap().size()))throw std::runtime_error("Valid domain outside image");}
    validDomain_=polygon;renderRoi();
}
QVector<QPointF> ImageViewer::roiHandles(const ToolRoi& roi) const {
    return std::visit([](const auto& g)->QVector<QPointF>{using T=std::decay_t<decltype(g)>;
        if constexpr(std::is_same_v<T,ScanSegment>){auto d=g.end-g.start;auto n=QPointF(-d.y(),d.x())*(g.width/(2*QLineF(g.start,g.end).length()));return {g.start,g.end,(g.start+g.end)/2+n};}
        else if constexpr(std::is_same_v<T,RotatedRectangle>)return {g.centre,polar(g.centre,g.size.width()/2,g.angleDegrees),polar(g.centre,g.size.height()/2,g.angleDegrees+90),polar(g.centre,g.size.width()/2+20,g.angleDegrees)};
        else if constexpr(std::is_same_v<T,Annulus>)return {g.centre,polar(g.centre,g.innerRadius,0),polar(g.centre,g.outerRadius,0)};
        else if constexpr(std::is_same_v<T,ArcRoi>){const auto& r=g.ring;return {r.centre,polar(r.centre,r.innerRadius,0),polar(r.centre,r.outerRadius,0),polar(r.centre,(r.innerRadius+r.outerRadius)/2,g.startDegrees),polar(r.centre,(r.innerRadius+r.outerRadius)/2,g.startDegrees+g.sweepDegrees)};}
        else return g.vertices;
    },roi.geometry);
}
ToolRoi ImageViewer::moveRoiHandle(const ToolRoi& before,int handle,QPointF p) const {
    auto result=before;std::visit([&](auto& g){using T=std::decay_t<decltype(g)>;
        if constexpr(std::is_same_v<T,ScanSegment>){if(handle==0)g.start=p;else if(handle==1)g.end=p;else {auto d=g.end-g.start;g.width=2*std::abs(d.x()*(p.y()-g.start.y())-d.y()*(p.x()-g.start.x()))/QLineF(g.start,g.end).length();}}
        else if constexpr(std::is_same_v<T,RotatedRectangle>){if(handle==0)g.centre=p;else if(handle==3)g.angleDegrees=angle(g.centre,p);else {const auto v=local(p-g.centre,g.angleDegrees);if(handle==1)g.size.setWidth(2*std::abs(v.x()));else g.size.setHeight(2*std::abs(v.y()));}}
        else if constexpr(std::is_same_v<T,Annulus>){if(handle==0)g.centre=p;else if(handle==1)g.innerRadius=QLineF(g.centre,p).length();else g.outerRadius=QLineF(g.centre,p).length();}
        else if constexpr(std::is_same_v<T,ArcRoi>){if(handle==0)g.ring.centre=p;else if(handle==1)g.ring.innerRadius=QLineF(g.ring.centre,p).length();else if(handle==2)g.ring.outerRadius=QLineF(g.ring.centre,p).length();else if(handle==3){const auto end=g.startDegrees+g.sweepDegrees;g.startDegrees=angle(g.ring.centre,p);double sweep=std::fmod(end-g.startDegrees+720,360.0);g.sweepDegrees=g.sweepDegrees>0 ? sweep:sweep-360;}else {double sweep=std::fmod(angle(g.ring.centre,p)-g.startDegrees+720,360.0);g.sweepDegrees=g.sweepDegrees>0 ? sweep:sweep-360;}}
        else g.vertices[handle]=p;
    },result.geometry);return result;
}
void ImageViewer::renderRoi(){
    for(auto* item:roiItems_){scene_->removeItem(item);delete item;}roiItems_.clear();if(!imageItem_)return;
    auto add=[&](const QPainterPath& p,QColor color,Qt::PenStyle style=Qt::SolidLine){QPen pen(color,2,style);pen.setCosmetic(true);auto* item=scene_->addPath(p,pen);item->setZValue(10);roiItems_<<item;};
    if(roiEditing_){QPainterPath raw;raw.addRect(QRectF(0,0,imageItem_->pixmap().width()-1,imageItem_->pixmap().height()-1));add(raw,QColor("#678194"),Qt::DashLine);}
    if(!validDomain_.isEmpty()){QPainterPath domain;polygon(domain,validDomain_,true);add(domain,QColor("#56b4ff"),Qt::DashLine);}
    const auto shown=roiDraft_ ? roiDraft_:roi_;if(!shown)return;
    QPainterPath path;std::visit([&](const auto& g){using T=std::decay_t<decltype(g)>;
        if constexpr(std::is_same_v<T,ScanSegment>){const auto d=g.end-g.start;const auto len=QLineF(g.start,g.end).length();if(len<1e-9)return;const auto n=QPointF(-d.y(),d.x())*(g.width/(2*len));polygon(path,{g.start+n,g.end+n,g.end-n,g.start-n},true);path.moveTo(g.start);path.lineTo(g.end);if(!roiToRaw_){const auto tip=angle(g.start,g.end);path.moveTo(polar(g.end,8,tip+150));path.lineTo(g.end);path.lineTo(polar(g.end,8,tip-150));}}
        else if constexpr(std::is_same_v<T,RotatedRectangle>){QVector<QPointF> p;for(auto v:QVector<QPointF>{{-g.size.width()/2,-g.size.height()/2},{g.size.width()/2,-g.size.height()/2},{g.size.width()/2,g.size.height()/2},{-g.size.width()/2,g.size.height()/2}})p<<g.centre+local(v,-g.angleDegrees);polygon(path,p,true);}
        else if constexpr(std::is_same_v<T,Annulus>){for(auto r:{g.innerRadius,g.outerRadius}){if(roiToRaw_){QVector<QPointF> p;for(int i=0;i<=180;++i)p<<polar(g.centre,r,i*2.);polygon(path,p,true);}else path.addEllipse(g.centre,r,r);}}
        else if constexpr(std::is_same_v<T,ArcRoi>){QVector<QPointF> p;const int count=std::max(2,int(std::abs(g.sweepDegrees)/2)+1);for(int i=0;i<=count;++i)p<<polar(g.ring.centre,g.ring.outerRadius,g.startDegrees+g.sweepDegrees*i/count);for(int i=count;i>=0;--i)p<<polar(g.ring.centre,g.ring.innerRadius,g.startDegrees+g.sweepDegrees*i/count);polygon(path,p,true);}
        else polygon(path,g.vertices,std::is_same_v<T,PolygonRoi>);
    },shown->geometry);
    bool valid=false;try{if(roiToRaw_){QPainterPath mapped;for(int i=0;i<path.elementCount();++i){auto e=path.elementAt(i);auto p=roiToRaw_({e.x,e.y});if(e.isMoveTo())mapped.moveTo(p);else mapped.lineTo(p);}path=mapped;
        if(const auto* scan=std::get_if<ScanSegment>(&shown->geometry)){
            const auto a=roiToRaw_(scan->start),b=roiToRaw_(scan->end);const auto tip=angle(a,b);
            path.moveTo(polar(b,8,tip+150));path.lineTo(b);path.lineTo(polar(b,8,tip-150));
        }
        valid=true;}else valid=shown->inside(imageItem_->pixmap().size());}catch(const std::exception&){return;}
    add(path,valid ? QColor("#ffd166"):QColor("#ff8585"));
    if(roiEditing_ && !roiDraft_){QPen pen(QColor("#66e3c4"),2);pen.setCosmetic(true);for(auto p:roiHandles(*shown)){auto* h=scene_->addRect(-4,-4,8,8,pen,QBrush(QColor("#101923")));h->setFlag(QGraphicsItem::ItemIgnoresTransformations);h->setPos(roiToRaw_ ? roiToRaw_(p):p);h->setZValue(11);roiItems_<<h;}}
    auto* label=scene_->addSimpleText(valid ? (roiRecipeBound_ ? (roi_ && std::holds_alternative<ScanSegment>(roi_->geometry) ? (roiToRaw_ ? "Reçete oku • A → B • mm → raw":"Reçete oku • A → B • raw px"):(roiToRaw_ ? "Reçete halka/yay • mm → raw":"Reçete ROI • raw px")):"Geçici ROI • raw px"):"Geçersiz / sınır dışında");label->setBrush(valid ? QColor("#ffd166"):QColor("#ff8585"));label->setFlag(QGraphicsItem::ItemIgnoresTransformations);label->setPos(0,0);label->setZValue(12);roiItems_<<label;
}
bool ImageViewer::roiPress(QMouseEvent* e){
    if(!imageItem_ || e->button()!=Qt::LeftButton)return false;
    QPointF p=mapToScene(e->position().toPoint());if(roiFromRaw_)try{p=roiFromRaw_(p);}catch(const std::exception&){return false;}
    if(roiDrawing_>=0){
        if(roiDrawing_>=4){if(roiVertices_.size()<1024){roiVertices_<<p;roiDraft_=ToolRoi{roiCoordinates_,PathRoi{roiVertices_}};renderRoi();}e->accept();return true;}
        roiStart_=p;roiDragging_=true;roiDraft_.reset();e->accept();return true;
    }
    if(roiEditing_ && roi_){const auto handles=roiHandles(*roi_);double best=11;for(int i=0;i<handles.size();++i){const auto d=QLineF(e->position(),mapFromScene(roiToRaw_ ? roiToRaw_(handles[i]):handles[i])).length();if(d<best){best=d;roiHandle_=i;}}if(roiHandle_>=0){roiBeforeDrag_=roi_;roiDragging_=true;setDragMode(NoDrag);e->accept();return true;}}
    return false;
}
bool ImageViewer::roiMove(QMouseEvent* e){
    if(!imageItem_)return false;QPointF p=mapToScene(e->position().toPoint());if(roiFromRaw_)try{p=roiFromRaw_(p);}catch(const std::exception&){return false;}
    if(roiDragging_){
        if(roiHandle_>=0 && roiBeforeDrag_)roiDraft_=moveRoiHandle(*roiBeforeDrag_,roiHandle_,p);
        else if(roiDrawing_==0)roiDraft_=ToolRoi{roiCoordinates_,ScanSegment{roiStart_,p,10}};
        else if(roiDrawing_==1)roiDraft_=ToolRoi{roiCoordinates_,RotatedRectangle{(roiStart_+p)/2,{std::abs(p.x()-roiStart_.x()),std::abs(p.y()-roiStart_.y())},0}};
        else if(roiDrawing_==2){const auto r=QLineF(roiStart_,p).length();roiDraft_=ToolRoi{roiCoordinates_,Annulus{roiStart_,r/2,r}};}
        else if(roiDrawing_==3){const auto r=QLineF(roiStart_,p).length();roiDraft_=ToolRoi{roiCoordinates_,ArcRoi{{roiStart_,r/2,r},angle(roiStart_,p),90}};}
        renderRoi();e->accept();return true;
    }
    return roiDrawing_>=0;
}
void ImageViewer::commitRoiGeometry(const ToolRoi& r){
    try{setRoi(r);}
    catch(const std::exception& e){cancelRoi();roiStatus_=QString::fromUtf8(e.what());if(onRoiStatus)onRoiStatus();}
}
void ImageViewer::finishRoi(){
    if(roiDrawing_==4)roiDraft_=ToolRoi{roiCoordinates_,PolygonRoi{roiVertices_}};
    else if(roiDrawing_==5)roiDraft_=ToolRoi{roiCoordinates_,PathRoi{roiVertices_}};
    if(roiDraft_){const auto value=*roiDraft_;commitRoiGeometry(value);}else cancelRoi();
}
bool ImageViewer::roiRelease(QMouseEvent* e){
    if(e->button()!=Qt::LeftButton || (!roiDragging_ && roiDrawing_<0))return false;
    if(roiDragging_){roiMove(e);finishRoi();}e->accept();return true;
}
void ImageViewer::keyPressEvent(QKeyEvent* e){
    if(e->key()==Qt::Key_Escape && (searchDragging_ || drawing_)){cancelRoi();drawRegion(false);e->accept();return;}
    if(e->matches(QKeySequence::Undo) && searchEditable_){cancelRoi();drawRegion(false);if(!searchUndo_.isEmpty()){searchValue_=searchUndo_.takeLast();renderSearchRegion();const auto edited=onSearchRegionEdited;if(edited)edited(searchValue_);}e->accept();return;}
    if(e->key()==Qt::Key_Escape && (roiDrawing_>=0 || roiDragging_)){cancelRoi();roiStatus_="Çizim iptal edildi; önceki geometri korundu.";if(onRoiStatus)onRoiStatus();e->accept();return;}
    if(e->matches(QKeySequence::Undo) && roiEditing_){undoRoi();e->accept();return;}
    if((e->key()==Qt::Key_Enter || e->key()==Qt::Key_Return) && roiDrawing_>=4){finishRoi();e->accept();return;}
    if(e->key()==Qt::Key_Backspace && roiDrawing_>=4){if(!roiVertices_.isEmpty())roiVertices_.removeLast();roiDraft_=ToolRoi{roiCoordinates_,PathRoi{roiVertices_}};renderRoi();e->accept();return;}
    QGraphicsView::keyPressEvent(e);
}
}
