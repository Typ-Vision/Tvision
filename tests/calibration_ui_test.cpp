#include "app/CalibrationWizard.hpp"
#include "app/CalibrationHome.hpp"
#include "app/ImageViewer.hpp"
#include "application/CalibrationService.hpp"
#include <QApplication>
#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QPainter>
#include <QElapsedTimer>
#include <QTextStream>
#include <QTimer>
#include <cmath>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
template<class T>T* widget(QWidget& w,const char* name){auto* p=dynamic_cast<T*>(w.findChild<QWidget*>(name));check(p,"Missing widget");return p;}
void click(QWidget& w,const char* id){auto* b=widget<QPushButton>(w,id);check(b->isEnabled(),id);QTest::mouseClick(b,Qt::LeftButton);QApplication::processEvents();}
void wait(std::function<bool()> done){QElapsedTimer t;t.start();while(!done() && t.elapsed()<15000){QApplication::processEvents();QTest::qWait(5);}check(done(),"Worker timeout");}
void idle(CalibrationWizard& w){wait([&]{return widget<QPushButton>(w,"wizardNext")->isEnabled();});}
void setup(CalibrationWizard& w){widget<QLineEdit>(w,"wizardInstallation")->setText("Hat 1");widget<QLineEdit>(w,"wizardLens")->setText("Lens 1");widget<QLineEdit>(w,"wizardPlane")->setText("Tabla 1");}
void draw(CalibrationWizard& w,QPointF a,QPointF b){click(w,"wizardDraw");auto* v=widget<ImageViewer>(w,"wizardViewer");auto start=v->mapFromScene(a),end=v->mapFromScene(b);QTest::mousePress(v->viewport(),Qt::LeftButton,{},start);QTest::mouseMove(v->viewport(),end,10);QTest::mouseRelease(v->viewport(),Qt::LeftButton,{},end);check(v->roi().has_value(),"Actual mouse ROI drawing");}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");QFile theme("../../resources/theme.qss");if(theme.open(QIODevice::ReadOnly))app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    QTemporaryDir tmp(QDir::currentPath()+"/kui4-ui-XXXXXX");check(tmp.isValid(),"Fixture directory");
    QImage stripe(640,200,QImage::Format_Grayscale8);stripe.fill(20);for(int y=0;y<200;++y)for(int x=120;x<520;++x)stripe.scanLine(y)[x]=220;const auto path=tmp.filePath("stripe.png");check(stripe.save(path),"Stripe fixture");
    ToolDefinition source;source.id="recipe-source";source.kind="image_file";source.parameters={{"path",path}};
    std::optional<CalibrationModelRecord> saved;
    CalibrationWizard w(false,source,tmp.path(),[&](const auto& r){saved=r;});w.resize(1400,900);w.show();app.processEvents();setup(w);w.openFile(path);idle(w);widget<QComboBox>(w,"wizardUsage")->setCurrentIndex(1);click(w,"wizardNext");
    draw(w,{20,100},{620,100});click(w,"wizardDetect");idle(w);check(widget<QListWidget>(w,"wizardPoints")->count()==2,"Actual caliper endpoints");
    w.grab().save("kui4-known-normal.png");click(w,"wizardNext");widget<QDoubleSpinBox>(w,"wizardKnownMm")->setValue(10);click(w,"wizardNext");check(widget<QLabel>(w,"wizardResult")->text().contains("Hesaplandı"),"Calculated result");
    click(w,"wizardBack");check(widget<QDoubleSpinBox>(w,"wizardKnownMm")->value()==10,"Back keeps inputs");click(w,"wizardNext");click(w,"wizardBeginControl");draw(w,{20,140},{620,140});click(w,"wizardDetect");idle(w);click(w,"wizardNext");widget<QDoubleSpinBox>(w,"wizardControlMm")->setValue(10);click(w,"wizardControl");check(widget<QLabel>(w,"wizardResult")->text().contains("Kabul sınırı tanımlanmadı"),"Optional control without invented acceptance");widget<QDoubleSpinBox>(w,"wizardControlMm")->setValue(9);widget<QCheckBox>(w,"wizardControlUseLimit")->setChecked(true);widget<QDoubleSpinBox>(w,"wizardControlLimit")->setValue(.1);click(w,"wizardControl");check(widget<QLabel>(w,"wizardResult")->text().contains("Sınır dışında"),"Real independent control rejects wrong known length");widget<QDoubleSpinBox>(w,"wizardControlMm")->setValue(10);click(w,"wizardNext");check(!saved && w.isVisible(),"Pending edited control cannot silently save stale acceptance");click(w,"wizardControl");widget<QCheckBox>(w,"wizardControlUseLimit")->setChecked(false);click(w,"wizardControl");widget<QLineEdit>(w,"wizardName")->setText("Referans ölçü");w.resize(850,600);app.processEvents();w.grab().save("kui4-known-small.png");check(widget<QPushButton>(w,"wizardNext")->geometry().bottom()<=w.height(),"Small-screen footer visible");click(w,"wizardNext");check(saved && saved->image().origin=="setup_file" && saved->activatable() && !saved->model().validated(),"File declaration preserves origin and readiness");check(std::abs(saved->model().millimetresPerPixel()-.025)<.0001,"Independent 400px/10mm oracle");check(CalibrationModelRecord::decode(saved->encode()).name()=="Referans ölçü","Name strict round trip");
    auto corrupt=saved->encode();corrupt["name"]="tampered";bool rejected=false;try{CalibrationModelRecord::decode(corrupt);}catch(...){rejected=true;}check(rejected,"Named record content tamper rejected");
    bool gate=true;CalibrationHomeContext context{tmp.filePath("project.vfproject"),"project",tmp.path(),source};CalibrationHome home([&]{return context;},[&]{return gate;});home.resize(1400,900);home.show();home.refresh();home.catalog()->save(*saved);home.refresh();widget<QTableWidget>(home,"calibrationHomeRecords")->selectRow(0);click(home,"calibrationHomeActivate");wait([&]{return home.catalog()->active().has_value();});check(home.catalog()->resolve(saved->setup()).record.name()==saved->name(),"File setup activates after actual acquisition");
    home.grab().save("kui4-home-normal.png");home.resize(850,600);app.processEvents();home.grab().save("kui4-home-small.png");gate=false;click(home,"calibrationHomeDeactivate");check(home.catalog()->active().has_value(),"Runtime/ACK gate preserves active");gate=true;
    auto legacyDependency=saved->model().dependency();legacyDependency.calibrationId="legacy-ui";const auto raw=saved->setup().source;auto legacyCandidate=MeasurementService{}.teach(legacyDependency,{"teach",{{119.5,100},raw},{{519.5,100},raw},10});auto legacyModel=MeasurementService{}.validate(legacyCandidate,{{"h",{{10,30},raw},{{110,30},raw},2.5},{"v",{{30,20},raw},{{30,120},raw},2.5}},.001);CalibrationService(context.path+".calibrations.json").save(legacyModel);
    QTimer::singleShot(50,[&]{auto* dialog=home.findChild<QDialog*>("legacyCalibrationImport");check(dialog,"Legacy import dialog");widget<QTableWidget>(*dialog,"legacyRecords")->selectRow(0);widget<QLineEdit>(*dialog,"legacyInstallationName")->setText("Legacy Hat 1");widget<QCheckBox>(*dialog,"legacySetupDeclaration")->setChecked(true);dialog->grab().save("kui5-legacy-import.png");click(*dialog,"legacyAdopt");});click(home,"calibrationHomeLegacy");wait([&]{return home.catalog()->records().size()==2;});check(home.catalog()->active()->id==saved->model().dependency().calibrationId,"Legacy import preserves active selection");check(home.catalog()->records().last().image().origin=="legacy_record" && home.catalog()->records().last().image().pixelHash.isEmpty(),"Legacy UI does not invent missing pixel evidence");
    QTimer::singleShot(50,[&]{auto* dialog=home.findChild<QDialog*>("robotCalibrationTemplate");check(dialog && dialog->findChildren<QPushButton*>().size()==1,"Robot template has close only");dialog->grab().save("kui4-robot.png");dialog->accept();});click(home,"calibrationHomeStart2");check(home.catalog()->active().has_value(),"Robot does not change active calibration");
    context.projectId="other";context.path=tmp.filePath("other.vfproject");home.refresh();check(!home.catalog()->active() && home.catalog()->records().isEmpty(),"Project change clears activity");context.projectId="project";context.path=tmp.filePath("project.vfproject");home.refresh();check(!home.catalog()->active() && home.catalog()->remembered(),"Restart/project return remembers without activating");
    for(int pattern=0;pattern<2;++pattern){QImage board(640,480,QImage::Format_RGB32);board.fill(Qt::white);QPainter p(&board);if(pattern==0){for(int y=0;y<7;++y)for(int x=0;x<10;++x)p.fillRect(70+x*45,65+y*45,45,45,(x+y)%2 ? Qt::white:Qt::black);}else{p.setRenderHint(QPainter::Antialiasing);p.setBrush(Qt::black);p.setPen(Qt::NoPen);for(int y=0;y<6;++y)for(int x=0;x<8;++x)p.drawEllipse(QPointF(100+x*45,100+y*45),8,8);}p.end();const auto boardPath=tmp.filePath(QString("plate%1.png").arg(pattern));check(board.save(boardPath),"Plate fixture");std::optional<CalibrationModelRecord> plate;
        CalibrationWizard d(true,source,tmp.path(),[&](const auto& r){plate=r;});d.resize(1400,900);d.show();app.processEvents();setup(d);d.openFile(boardPath);idle(d);click(d,"wizardNext");widget<QComboBox>(d,"wizardPattern")->setCurrentIndex(pattern);widget<QSpinBox>(d,"wizardColumns")->setValue(pattern==0 ? 9:8);widget<QDoubleSpinBox>(d,"wizardSpacing")->setValue(5);widget<QLineEdit>(d,"wizardTargetName")->setText(QString("Plate %1").arg(pattern));click(d,"wizardSaveTarget");check(widget<QComboBox>(d,"wizardTargets")->findText(QString("Plate %1").arg(pattern))>=0,"Named target definition can be reused");click(d,"wizardDetect");idle(d);click(d,"wizardNext");check(widget<QLabel>(d,"wizardHeading")->text().startsWith("3"),"Plate detection advance");d.grab().save(QString("kui4-plate%1-normal.png").arg(pattern));click(d,"wizardNext");d.resize(850,600);app.processEvents();d.grab().save(QString("kui4-plate%1-small.png").arg(pattern));click(d,"wizardNext");check(plate && !plate->activatable() && plate->model().mapping(),"Plate trial is calculated, cannot activate");check(plate->model().mapping()->observation().target.pattern==(pattern==0 ? CalibrationGridPattern::Chessboard:CalibrationGridPattern::SymmetricCircles),"Plate descriptor preserved");
    }
    CalibrationWizard cancel(false,source,tmp.path(),[](const auto&){throw std::runtime_error("Unexpected save");});cancel.show();setup(cancel);cancel.openFile(path);click(cancel,"wizardStop");idle(cancel);check(!widget<QLabel>(cancel,"wizardImageInfo")->text().contains("640"),"Stop suppresses late file completion");click(cancel,"wizardCancel");check(cancel.result()==QDialog::Rejected,"Cancel discards draft");
    QTextStream(stdout)<<"PASS "<<checks<<" UI checks; fixtures "<<tmp.path()<<'\n';tmp.setAutoRemove(false);return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
