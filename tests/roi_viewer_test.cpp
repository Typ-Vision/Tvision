#include "app/ImageViewer.hpp"
#include "app/RoiEditor.hpp"
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QKeyEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QPushButton>
#include <QFontDatabase>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsRectItem>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,why);}
// Reproduction probe for the pre-24E wheel algorithm, isolated from assertions.
class PreviousWheelViewer:public ImageViewer {
    void wheelEvent(QWheelEvent* e) override {if(e->angleDelta().y())scale(std::pow(1.2,e->angleDelta().y()/120.),std::pow(1.2,e->angleDelta().y()/120.));e->accept();}
};
int main(int argc,char** argv){QApplication app(argc,argv);
if(app.arguments().contains("--baseline-zoom")){
    PreviousWheelViewer v;v.resize(850,600);v.show();QImage im(2448,2048,QImage::Format_RGB32);im.fill(Qt::darkGray);v.setImage(im);app.processEvents();v.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);v.actualSize();v.centerOn(1200,1000);v.beginRoi(1);
    const QPointF at(70,70);QMouseEvent move(QEvent::MouseMove,at,v.viewport()->mapToGlobal(at.toPoint()),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(v.viewport(),&move);
    const auto before=v.viewportTransform().inverted().map(at);const auto old=v.transform();const auto bars=QPoint(v.horizontalScrollBar()->value(),v.verticalScrollBar()->value());QWheelEvent e(at,v.viewport()->mapToGlobal(at.toPoint()),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(v.viewport(),&e);const auto drift=v.viewportTransform().map(before)-at;
    QTextStream(stdout)<<"PRE24E viewport="<<at.x()<<","<<at.y()<<" scene="<<before.x()<<","<<before.y()<<" transform="<<old.m11()<<"->"<<v.transform().m11()<<" scroll="<<bars.x()<<","<<bars.y()<<"->"<<v.horizontalScrollBar()->value()<<","<<v.verticalScrollBar()->value()<<" drift="<<drift.x()<<","<<drift.y()<<"\n";return 0;
}
QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");app.setStyle("Fusion");try{
    ImageViewer viewer;viewer.resize(1400,900);viewer.show();QImage image(640,480,QImage::Format_RGB888);image.fill(QColor("#192b3b"));{QPainter p(&image);p.setPen(QColor("#34485b"));for(int x=0;x<640;x+=20)p.drawLine(x,0,x,479);for(int y=0;y<480;y+=20)p.drawLine(0,y,639,y);}
    viewer.setImage(image);viewer.setRoiFrame("test/raw");app.processEvents();viewer.actualSize();viewer.zoomBy(4);viewer.centerOn(300,240);app.processEvents();
    auto mouse=[&](QEvent::Type type,QPointF p,Qt::MouseButton button,Qt::MouseButtons buttons){const auto at=viewer.mapFromScene(p);QMouseEvent e(type,QPointF(at),QPointF(viewer.viewport()->mapToGlobal(at)),button,buttons,Qt::NoModifier);QApplication::sendEvent(viewer.viewport(),&e);};
    auto click=[&](QPointF p){mouse(QEvent::MouseButtonPress,p,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,p,Qt::LeftButton,Qt::NoButton);};
    auto drag=[&](QPointF a,QPointF b){mouse(QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);};
    auto key=[&](int k,Qt::KeyboardModifiers mods=Qt::NoModifier){QKeyEvent e(QEvent::KeyPress,k,mods);QApplication::sendEvent(&viewer,&e);};
    int commits=0;viewer.onRoiChanged=[&]{++commits;};viewer.beginRoi(0);drag({250.25,240.5},{340.75,240.5});
    check(viewer.roi() && commits==1,"single gesture commits exactly once");const auto scan=std::get<ScanSegment>(viewer.roi()->geometry);check(scan.start==QPointF(250.25,240.5) && scan.end==QPointF(340.75,240.5),"subpixel scan retained through zoom/pan");
    const auto original=viewer.roi();drag(scan.end,{350.25,250.5});check(commits==2 && std::get<ScanSegment>(viewer.roi()->geometry).end==QPointF(350.25,250.5),"endpoint handle edits geometry");viewer.undoRoi();check(viewer.roi()==original && commits==3,"handle drag one undo action");
    mouse(QEvent::MouseButtonPress,scan.start,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{240,220},Qt::NoButton,Qt::LeftButton);key(Qt::Key_Escape);check(viewer.roi()==original && commits==3 && !viewer.roiGestureActive(),"Escape handle rollback no commit");
    viewer.beginRoi(1);mouse(QEvent::MouseButtonPress,{200,200},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{350,300},Qt::NoButton,Qt::LeftButton);key(Qt::Key_Escape);check(viewer.roi()==original && commits==3,"Escape half draw leaves previous geometry");
    viewer.beginRoi(1);drag({250,200},{350,280});check(std::get<RotatedRectangle>(viewer.roi()->geometry).centre==QPointF(300,240),"rectangle gesture");drag({370,240},{300,310});check(std::abs(std::get<RotatedRectangle>(viewer.roi()->geometry).angleDegrees-90)<1e-9,"rotation handle uses geometry angle");
    viewer.beginRoi(2);drag({300,240},{360,240});check(std::get<Annulus>(viewer.roi()->geometry).innerRadius==30,"ring gesture");drag({330,240},{320,240});check(std::get<Annulus>(viewer.roi()->geometry).innerRadius==20,"ring inner handle");
    viewer.beginRoi(3);drag({300,240},{360,240});check(std::get<ArcRoi>(viewer.roi()->geometry).sweepDegrees==90,"arc gesture");drag({300,285},{255,240});check(std::abs(std::get<ArcRoi>(viewer.roi()->geometry).sweepDegrees-180)<1e-9,"arc sweep handle");
    viewer.beginRoi(4);click({250,200});click({350,200});click({350,280});click({250,280});key(Qt::Key_Return);check(std::get<PolygonRoi>(viewer.roi()->geometry).vertices.size()==4,"polygon Enter commits");drag({250,200},{240.5,190.25});check(std::get<PolygonRoi>(viewer.roi()->geometry).vertices[0]==QPointF(240.5,190.25),"polygon vertex handle subpixel");
    viewer.beginRoi(5);click({250,240});click({300,200});click({350,240});key(Qt::Key_Backspace);click({350,280});key(Qt::Key_Return);check(std::get<PathRoi>(viewer.roi()->geometry).vertices==QVector<QPointF>{{250,240},{300,200},{350,280}},"path Backspace and Enter");
    const auto previous=viewer.roi();viewer.beginRoi(0);drag({-10,200},{100,200});check(viewer.roi()==previous,"out of bounds drawing rejected without clipping");
    reject([&]{viewer.setRoi(ToolRoi{{"foreign/raw"},previous->geometry});},"foreign frame numeric edit rejected");
    check(viewer.roi()==previous,"invalid numeric edit nonmutating");
    viewer.setValidDomain({{20,20},{620,20},{620,460},{20,460}});
    const auto dir=QDir(qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS",QCoreApplication::applicationDirPath()));QDir().mkpath(dir.absolutePath());
    QVector<RoiGeometry> shapes{ScanSegment{{200.25,240.5},{440.75,240.5},40},RotatedRectangle{{320,240},{220,100},30},Annulus{{320,240},70,120},ArcRoi{{{320,240},70,120},-40,240},PolygonRoi{{{180,100},{450,130},{420,350},{210,380}}},PathRoi{{{180,240},{260,140},{370,320},{450,240}}}};
    for(int i=0;i<shapes.size();++i){viewer.setRoi(ToolRoi{{"test/raw"},shapes[i]});viewer.resize(1400,900);viewer.fitImage();app.processEvents();check(viewer.grab().save(dir.filePath(QString("t2-roi-%1.png").arg(i))),"normal geometry PNG");viewer.resize(850,600);viewer.fitImage();app.processEvents();check(viewer.grab().save(dir.filePath(QString("t2-roi-%1-small.png").arg(i))),"small geometry PNG");}
    // Actual numeric editor uses the same codec as mouse editing, not a JSON text box.
    showRoiEditor(&viewer,&viewer,"test/raw");app.processEvents();auto* dialog=viewer.findChild<QDialog*>("roiGeometryDialog");check(dialog,"numeric dialog opens");auto* table=dialog->findChild<QTableWidget*>("roiNumbers");
    for(int row=0;row<table->rowCount();++row)if(table->item(row,0)->text()=="P1.x")static_cast<QDoubleSpinBox*>(table->cellWidget(row,1))->setValue(180.125);
    dialog->findChild<QPushButton*>("roiApply")->click();check(std::get<PathRoi>(viewer.roi()->geometry).vertices[0].x()==180.125,"numeric subpixel edit reaches same geometry");
    dialog->findChild<QPushButton*>("roiUndo")->click();check(viewer.roi()->geometry==shapes.last(),"numeric edit single undo");
    auto* selector=dialog->findChild<QComboBox*>("roiShape");
    for(int kind=0;kind<6;++kind){selector->setCurrentIndex(kind);dialog->findChild<QPushButton*>("roiApply")->click();check(viewer.roi() && viewer.roi()->geometry.index()==kind,"every shape numeric apply matches declared variant");}
    check(dialog->grab().save(dir.filePath("t2-roi-numbers.png")),"numeric dialog PNG");dialog->resize(340,450);app.processEvents();check(dialog->grab().save(dir.filePath("t2-roi-numbers-small.png")),"small numeric dialog PNG");dialog->close();app.processEvents();
    const auto saved=viewer.roi();viewer.zoomBy(2);viewer.centerOn(300,240);check(viewer.roi()==saved,"zoom/pan never mutate geometry");
    QRect legacy;viewer.onRegion=[&](QRect r){legacy=r;};viewer.actualSize();viewer.centerOn(300,240);viewer.drawRegion(true);drag({200,200},{220,230});check(legacy==QRect(200,200,21,31),"legacy rectangle teaching unaffected");
    int edits=0;viewer.onRoiEdited=[&]{++edits;};viewer.setRoi(ToolRoi{{"test/raw"},ScanSegment{{100,200},{300,200},4}});check(edits==1,"explicit geometry edit notifies recipe once");viewer.undoRoi();check(edits==2,"undo is an explicit recipe edit");
    viewer.setImage(image);check(edits==2,"frame presentation reset never edits recipe");check(!viewer.roi(),"new image clears geometry and undo");viewer.undoRoi();check(!viewer.roi(),"new image cannot undo stale geometry");viewer.clearImage();check(edits==2,"clear presentation never edits recipe");check(!viewer.roi() && !viewer.hasImage(),"clear image clears geometry");
    viewer.setImage(image);viewer.setValidDomain({});viewer.actualSize();viewer.centerOn(300,240);
    CoordinateFrame worldFrame{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown};
    auto project=[](QPointF q){return QPointF((10*q.x()+100)/(1+.01*q.x()),(8*q.y()+100)/(1+.01*q.x()));};
    auto inverse=[](QPointF p){double x=(p.x()-100)/(10-.01*p.x());return QPointF(x,(p.y()*(1+.01*x)-100)/8);};
    viewer.setRoiMapping(worldFrame,project,inverse);viewer.beginRoi(2);drag(project({20,15}),project({28,15}));check(viewer.roi() && viewer.roi()->frame==worldFrame,"mapped ring drawn in world coordinates");
    auto ring=std::get<Annulus>(viewer.roi()->geometry);check(std::abs(ring.outerRadius-8)<.2,"projective gesture radius world truth");
    const auto worldBefore=viewer.roi();drag(project(ring.centre+QPointF(ring.innerRadius,0)),project(ring.centre+QPointF(3,0)));check(std::abs(std::get<Annulus>(viewer.roi()->geometry).innerRadius-3)<.2,"world ring handle inverse maps cursor");viewer.setRoiMapping(worldFrame,project,inverse);viewer.undoRoi();check(viewer.roi()==worldBefore,"mapping refresh retains world undo");
    viewer.beginRoi(3);mouse(QEvent::MouseButtonPress,project({20,15}),Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,project({28,15}),Qt::NoButton,Qt::LeftButton);key(Qt::Key_Escape);check(viewer.roi()==worldBefore,"world partial arc Escape");
    viewer.setRoi(ToolRoi{worldFrame,ArcRoi{{{20,15},4,8},-40,-220}});viewer.fitImage();app.processEvents();check(viewer.grab().save(dir.filePath("circle-world-handles.png")),"world mapped handles PNG");
    // EUI-01 integer Blob path: actual viewport mouse/key events, distinct from ToolRoi.
    viewer.setImage(image);viewer.actualSize();viewer.zoomBy(2);viewer.centerOn(300,240);app.processEvents();
    QRect search(200,180,101,81);int searchCommits=0;
    viewer.onSearchRegionEdited=[&](QRect r){search=r;++searchCommits;viewer.setSearchRegionEditing("blob/frame-a",r,true);viewer.setRegions({},r);};
    viewer.setRegions({},search);viewer.setSearchRegionEditing("blob/frame-a",search,true);
    drag({240,210},{250,225});check(search==QRect(210,195,101,81) && searchCommits==1,"Blob body move keeps inclusive dimensions and commits once");
    key(Qt::Key_Z,Qt::ControlModifier);check(search==QRect(200,180,101,81) && searchCommits==2,"Blob Undo survives presentation callback");
    const QVector<QPointF> handles{{200,180},{250.5,180},{301,180},{301,220.5},{301,261},{250.5,261},{200,261},{200,220.5}};
    const QVector<QRect> expected{{190,170,111,91},{200,170,101,91},{200,170,111,91},{200,180,111,81},{200,180,111,91},{200,180,101,91},{190,180,111,91},{190,180,111,81}};
    const QVector<QPointF> deltas{{-10,-10},{0,-10},{10,-10},{10,0},{10,10},{0,10},{-10,10},{-10,0}};
    const QVector<Qt::CursorShape> resizeCursors{Qt::SizeFDiagCursor,Qt::SizeVerCursor,Qt::SizeBDiagCursor,Qt::SizeHorCursor,Qt::SizeFDiagCursor,Qt::SizeVerCursor,Qt::SizeBDiagCursor,Qt::SizeHorCursor};
    for(int h=0;h<8;++h){mouse(QEvent::MouseMove,handles[h],Qt::NoButton,Qt::NoButton);check(viewer.viewport()->cursor().shape()==resizeCursors[h],"Blob resize cursor matches handle");drag(handles[h],handles[h]+deltas[h]);check(search==expected[h],"all eight Blob handles preserve opposite edges");key(Qt::Key_Z,Qt::ControlModifier);check(search==QRect(200,180,101,81),"each resize is one undo action");}
    mouse(QEvent::MouseButtonPress,{240,210},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{270,250},Qt::NoButton,Qt::LeftButton);const auto beforeCancel=searchCommits;key(Qt::Key_Escape);mouse(QEvent::MouseButtonRelease,{270,250},Qt::LeftButton,Qt::NoButton);check(searchCommits==beforeCancel && search==QRect(200,180,101,81),"Blob Escape preview never commits");
    drag({301,261},{-100,-100});check(search==QRect(200,180,2,2),"Blob resize clamps at two inclusive pixels");key(Qt::Key_Z,Qt::ControlModifier);
    drag({240,210},{1000,1000});check(search==QRect(539,399,101,81),"Blob translation clamps at bottom-right image edge");key(Qt::Key_Z,Qt::ControlModifier);
    viewer.zoomBy(2);viewer.centerOn(300,240);app.processEvents();const auto beforePan=search;const auto transform=viewer.transform();const auto middleSceneBefore=viewer.mapToScene(viewer.viewport()->rect().center());mouse(QEvent::MouseButtonPress,{240,210},Qt::MiddleButton,Qt::MiddleButton);mouse(QEvent::MouseMove,{250,220},Qt::NoButton,Qt::MiddleButton);mouse(QEvent::MouseButtonRelease,{250,220},Qt::MiddleButton,Qt::NoButton);check(search==beforePan && viewer.transform()==transform && viewer.mapToScene(viewer.viewport()->rect().center())!=middleSceneBefore,"middle pan inside ROI moves viewport without editing");
    const auto outsideSceneBefore=viewer.mapToScene(viewer.viewport()->rect().center());drag({400,300},{410,310});check(search==beforePan && viewer.mapToScene(viewer.viewport()->rect().center())!=outsideSceneBefore,"left drag outside Blob really pans viewport");
    viewer.drawRegion(true);drag({210,200},{230,220});check(search==QRect(210,200,21,21),"new Blob drawing uses inclusive endpoints");drag({220,210},{225,215});check(search==QRect(215,205,21,21),"drawing leaves editing available");key(Qt::Key_Z,Qt::ControlModifier);key(Qt::Key_Z,Qt::ControlModifier);check(search==beforePan,"Undo crosses redraw and body edit");
    mouse(QEvent::MouseButtonPress,{240,210},Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,{260,230},Qt::NoButton,Qt::LeftButton);viewer.setSearchRegionEditing("blob/frame-b",search,true);mouse(QEvent::MouseButtonRelease,{260,230},Qt::LeftButton,Qt::NoButton);check(search==beforePan,"frame identity change discards active preview");key(Qt::Key_Z,Qt::ControlModifier);check(search==beforePan,"new frame clears Blob undo history");
    viewer.setSearchRegionEditing("blob/frame-b",search,false);drag({240,210},{260,230});check(search==beforePan,"locked Blob exposes no editable gesture");
    viewer.setSearchRegionEditing("blob/frame-b",search,true);viewer.onSearchRegionStarting=[] {return false;};const auto beforeRejected=searchCommits;drag({240,210},{260,230});check(search==beforePan && searchCommits==beforeRejected && !viewer.roiGestureActive(),"failed gesture preflight preserves saved Blob region");viewer.onSearchRegionStarting={};
    viewer.setSearchRegionEditing("blob/frame-b",search,true);viewer.fitImage();app.processEvents();check(viewer.grab().save(dir.filePath("eui01-blob-viewer-normal.png")),"Blob viewer normal PNG");viewer.resize(850,600);viewer.fitImage();app.processEvents();check(viewer.grab().save(dir.filePath("eui01-blob-viewer-small.png")),"Blob viewer small PNG");
    // EUI-03: visibility is separate from geometry and survives overlay/image rebuilds.
    ImageViewer other;other.setImage(image);other.setToolOverlay({},{{20,20}}, {},"unchanged");
    check(other.resultAnnotationsVisible(),"non-Editor viewer keeps annotations");
    viewer.setResultAnnotationsVisible(false);viewer.setImage(image);viewer.setRoiFrame("test/raw");viewer.setRoi(ToolRoi{{"test/raw"},shapes[1]});
    const QByteArray mask(40*40,char(1));
    for(int kind=0;kind<4;++kind){
        const QVector<QPointF> points=kind==2 ? QVector<QPointF>{{150,200},{450,200}}:QVector<QPointF>{{300,240}};
        viewer.setToolOverlay({QRect(100,100,400,280)},points,{QLineF(100,240,500,240)},kind==0 ? "support=32 angle=0 RMS=0.25":kind==1 ? "Circle R=120 RMS=0.15":kind==2 ? "distance=300 px":"Blob count=1 area=1600",kind==3 ? mask:QByteArray{},kind==3 ? QSize(40,40):QSize{},QPoint(280,220),{{310,250}},kind<2);
        QMap<QGraphicsItem*,bool> geometryVisibility;for(auto* item:viewer.scene()->items())if(!dynamic_cast<QGraphicsSimpleTextItem*>(item) && item->zValue()!=5)geometryVisibility[item]=item->isVisible();
        for(bool visible:{false,true}){
            viewer.setResultAnnotationsVisible(visible);
            for(double zoom:{.1,.25,1.,4.}){
                viewer.actualSize();viewer.zoomBy(zoom);
                int labels=0;for(auto* item:viewer.scene()->items()){
                    if(auto* text=dynamic_cast<QGraphicsSimpleTextItem*>(item)){
                        if(text->zValue()==12){check(text->isVisible(),"ROI coordinate and invalidity status stays visible");continue;}
                        ++labels;check(text->isVisible()==visible,"all result labels follow preference");
                        check(std::abs(text->scale()-std::min(zoom,1.5))<1e-9 && text->font().pointSizeF()==11,"text scales at 10/25/100/400 percent with 11pt base and 150 percent cap");
                    }else if(item->zValue()==5){check(item->isVisible()==visible && std::abs(item->scale()-std::min(zoom,1.5))<1e-9,"background shares text visibility and scale");}
                    else if(item->zValue()<10)check(item->isVisible()==geometryVisibility[item],"raster geometry mask and ROI visibility unchanged");
                }check(labels>0,"fixture contains result labels");
                for(const auto size:{QSize(1400,900),QSize(850,600)}){
                    viewer.resize(size);viewer.centerOn(320,240);app.processEvents();
                    check(viewer.grab().save(dir.filePath(QString("eui08-kind%1-zoom%2-%3-%4.png").arg(kind).arg(int(zoom*100)).arg(visible ? "on":"off").arg(size.width()))),"annotation zoom and visibility PNG");
                }
            }
        }
    }
    viewer.setResultAnnotationsVisible(false);viewer.setImage(image);viewer.setToolOverlay({},{{300,240}}, {},"new result");
    for(auto* item:viewer.scene()->items())if(dynamic_cast<QGraphicsSimpleTextItem*>(item))check(!item->isVisible(),"hidden preference survives new image and Run overlay");
    viewer.setRoiFrame("test/raw");viewer.setScanGuides([](const ToolRoi&){return QVector<QLineF>{{{200,200},{260,200}},{{200,220},{260,220}}};});
    viewer.setRoi(ToolRoi{{"test/raw"},Annulus{{240,210},20,60}},false);
    for(double zoom:{.1,1.,4.}){viewer.actualSize();viewer.zoomBy(zoom);int count=0;
        for(auto* item:viewer.scene()->items())if(item->data(0)=="scanGuide"){++count;check(item->isVisible(),"guides independent of result visibility");}
        check(count==2,"zoom retains all guide shafts");}
    viewer.setRoiMapping({"world",LengthUnit::Millimetre},[](QPointF p){return QPointF(p.x()*2+10,p.y()*2+20);});
    viewer.setRoi(ToolRoi{{"world",LengthUnit::Millimetre},Annulus{{100,100},10,20}},false);
    int mapped=0;for(auto* item:viewer.scene()->items())if(item->data(0)=="scanGuide")++mapped;check(mapped==2,"world guides map through viewer mapping");
    viewer.setScanGuides([](const ToolRoi&){return QVector<QLineF>{{{200,200},{260,200}},{{200,220},{1e6,220}}};});
    mapped=0;for(auto* item:viewer.scene()->items())if(item->data(0)=="scanGuide")++mapped;check(mapped==0,"failed mapping publishes no partial guides");
    // CCUX-24E: viewport events, including moves consumed by ROI drawing.
    for(const auto screen:{QSize(1400,900),QSize(850,600)})for(const auto raster:{QSize(640,480),QSize(2448,2048)}) {
        ImageViewer zoom;zoom.resize(screen);zoom.show();QImage pixels(raster,QImage::Format_RGB32);pixels.fill(QColor("#192b3b"));zoom.setImage(pixels,"camera-a");app.processEvents();
        int geometryEdits=0;zoom.onRoiEdited=[&]{++geometryEdits;};zoom.onSearchRegionEdited=[&](QRect){++geometryEdits;};
        auto wheel=[&](QPointF at,int delta){QWheelEvent e(at,zoom.viewport()->mapToGlobal(at.toPoint()),{},QPoint(0,delta),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(zoom.viewport(),&e);QApplication::processEvents();};
        const auto points=[&]{const auto r=zoom.viewport()->rect().adjusted(25,25,-25,-25);return QVector<QPointF>{r.center(),r.topLeft(),r.topRight(),r.bottomLeft(),r.bottomRight()};};
        int bounded=0,anchored=0;
        auto measured=[&](QPointF at,int delta){
            const auto before=zoom.viewportTransform().inverted().map(at);const auto scale=zoom.zoomFactor();
            wheel(at,delta);const auto after=zoom.viewportTransform().map(before);const auto error=after-at;
            auto constrained=[](QScrollBar* b,double e){return (e>.51 && b->value()==b->maximum()) || (e<-.51 && b->value()==b->minimum());};
            const bool limited=constrained(zoom.horizontalScrollBar(),error.x()) || constrained(zoom.verticalScrollBar(),error.y());
            QTextStream(stdout)<<"wheel "<<screen.width()<<" raster="<<raster.width()<<" at="<<at.x()<<","<<at.y()<<" scale="<<scale<<"->"<<zoom.zoomFactor()<<" scene="<<before.x()<<","<<before.y()<<" error="<<error.x()<<","<<error.y()<<" scroll="<<zoom.horizontalScrollBar()->value()<<","<<zoom.verticalScrollBar()->value()<<" bounded="<<limited<<"\n";
            if(limited)++bounded;else {++anchored;check(std::abs(error.x())<=1 && std::abs(error.y())<=1,"wheel event anchor within one viewport pixel");}
        };
        zoom.fitImage();measured(points()[1],120); // First event after Fit, no prior base mouseMove.
        for(int mode=0;mode<4;++mode)for(const auto at:points()){
            zoom.setImage(pixels,"camera-a");zoom.actualSize();zoom.zoomBy(4);zoom.centerOn(raster.width()/2.,raster.height()/2.);
            if(mode==1)zoom.beginRoi(1);
            if(mode==2){const auto p=zoom.mapToScene(at.toPoint());zoom.setRoi(ToolRoi{{"viewer/raw"},ScanSegment{p,p+QPointF(20,0),4}},false);zoom.setRoiEditable(true);}
            if(mode==3){const auto p=zoom.viewport()->rect().center();QMouseEvent press(QEvent::MouseButtonPress,p,zoom.viewport()->mapToGlobal(p),Qt::MiddleButton,Qt::MiddleButton,Qt::NoModifier);QApplication::sendEvent(zoom.viewport(),&press);QMouseEvent move(QEvent::MouseMove,p+QPoint(19,11),zoom.viewport()->mapToGlobal(p+QPoint(19,11)),Qt::NoButton,Qt::MiddleButton,Qt::NoModifier);QApplication::sendEvent(zoom.viewport(),&move);QMouseEvent release(QEvent::MouseButtonRelease,p+QPoint(19,11),zoom.viewport()->mapToGlobal(p+QPoint(19,11)),Qt::MiddleButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(zoom.viewport(),&release);}
            QMouseEvent move(QEvent::MouseMove,at,zoom.viewport()->mapToGlobal(at.toPoint()),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(zoom.viewport(),&move);
            const auto fixed=zoom.viewportTransform().inverted().map(at);const auto roiBefore=zoom.roi();
            for(int n=0;n<12;++n){measured(at,120);measured(at,-120);}
            const auto drift=zoom.viewportTransform().map(fixed)-at;check(std::abs(drift.x())<=1 && std::abs(drift.y())<=1,"repeated in/out has no cumulative corner drift");check(zoom.roi()==roiBefore,"wheel preserves ROI coordinates");
        }
        check(geometryEdits==0,"wheel never edits recipe geometry");
        zoom.actualSize();zoom.zoomBy(32);const auto upper=zoom.transform();wheel(points()[1],120);check(zoom.transform()==upper,"upper bound no-op");zoom.zoomBy(.000001);const auto lower=zoom.transform();wheel(points()[1],-120);wheel(points()[0],0);check(zoom.transform()==lower,"lower bound and zero wheel no-op");
        zoom.fitImage();measured(points()[0],-120);zoom.actualSize();zoom.zoomBy(4);zoom.centerOn(raster.width()/2.,raster.height()/2.);measured(points()[2],120);
        check(zoom.grab().save(dir.filePath(QString("ccux24e-zoom-%1-%2.png").arg(screen.width()).arg(raster.width()))),"zoom evidence PNG");
        zoom.clearImage();wheel(points()[1],120);check(zoom.transform().isIdentity(),"empty viewer wheel no-op");
        QTextStream(stdout)<<"matrix anchored="<<anchored<<" bounded="<<bounded<<"\n";
    }
    QTextStream(stdout)<<checks<<" viewer checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<"\n";return 1;}}
