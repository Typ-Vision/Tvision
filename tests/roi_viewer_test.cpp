#include "app/ImageViewer.hpp"
#include "app/RoiEditor.hpp"
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QPushButton>
#include <QFontDatabase>
#include <QPainter>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,why);}
int main(int argc,char** argv){QApplication app(argc,argv);QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");app.setStyle("Fusion");try{
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
    const auto dir=QDir(QCoreApplication::applicationDirPath());
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
    QTextStream(stdout)<<checks<<" viewer checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<"\n";return 1;}}
