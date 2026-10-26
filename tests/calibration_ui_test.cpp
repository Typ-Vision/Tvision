#include "app/CameraFlowSmokeAudit.hpp"
#include "app/CalibrationWizard.hpp"
#include "app/CalibrationOverview.hpp"
#include "OecsExperimentalFixture.hpp"
#include "CameraAcquisitionFixture.hpp"
#include <QGroupBox>
#include "application/CalibrationMigration.hpp"
#include "app/ImageViewer.hpp"
#include "application/CalibrationService.hpp"
#include "application/MeasurementWorkspace.hpp"
#include "application/ProjectService.hpp"
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
class CountedOecsApi:public oecs_experimental_fixture::Api {
    std::shared_ptr<std::atomic_int> opens_,closes_;
public:
    CountedOecsApi(std::shared_ptr<oecs_experimental_fixture::State> state,std::shared_ptr<std::atomic_int> opens,std::shared_ptr<std::atomic_int> closes):Api(state),opens_(opens),closes_(closes){}
    int openExclusive(void* h) override {++*opens_;return Api::openExclusive(h);}
    int closeDevice(void* h) override {++*closes_;return Api::closeDevice(h);}
};
class CameraFixture:public AcquisitionAdapter {public:QImage image;explicit CameraFixture(QImage im):image(im){}AcquisitionCapabilities capabilities()const override{return {"webcam",true};}ImageFrame acquire(const AcquisitionRequest&)const override{return ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),image.width(),image.height(),image.bytesPerLine(),PixelFormat::Gray8);}};
QImage framedRaster(bool missing=false,bool marker=true,bool clutter=false,bool bent=false){
    QImage im(900,900,QImage::Format_Grayscale8);
    for(int y=0;y<900;++y)for(int x=0;x<900;++x){double total=0;
        for(double dy:{-.25,.25})for(double dx:{-.25,.25}){double u=x+dx,v=y+dy;
            bool dark=u>=80 && u<=720 && v>=80 && v<=720 && (u<=100 || u>=700 || v<=100 || v>=700);
            if(marker && u>=100 && v>=100 && u+v<=250)dark=true;
            int c=int(std::round((u-160)/80));double bend=bent ? 7*std::sin(c*1.2):0;int r=int(std::round((v-160-bend)/80));
            if(c>=0 && c<7 && r>=0 && r<7 && !(missing && r==2 && c==3) && std::hypot(u-(160+80*c),v-(160+80*r+bend))<22)dark=true;
            if(clutter)for(int k=0;k<9;++k)if(std::hypot(u-(180+58*k),v-118)<7)dark=true;
            total+=dark?20:235;
        }im.scanLine(y)[x]=uchar(std::round(total/4));
    }return im;
}
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
    const auto evidence=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS",QDir::currentPath()+"/../ccux/ccux-09")+"/wizard";QDir().mkpath(evidence);
    QImage stripe(640,200,QImage::Format_Grayscale8);stripe.fill(20);for(int y=0;y<200;++y)for(int x=120;x<520;++x)stripe.scanLine(y)[x]=220;const auto path=tmp.filePath("stripe.png");check(stripe.save(path),"Stripe fixture");
    ToolDefinition source;source.id="recipe-source";source.kind="image_file";source.parameters={{"path",path}};
    std::optional<CalibrationModelRecord> saved;
    CalibrationWizard w(false,source,tmp.path(),[&](const auto& r){saved=r;});w.resize(1400,900);w.show();app.processEvents();setup(w);w.openFile(path);idle(w);for(const auto screen:{QSize(1400,900),QSize(850,600)}){w.resize(screen);app.processEvents();check(CameraFlowSmokeAudit::wheel(*widget<ImageViewer>(w,"wizardViewer"),w,evidence+QString("/ccux24e-calibration-wheel-%1.png").arg(screen.width())),"Calibration viewport wheel");}w.resize(1400,900);app.processEvents();widget<QComboBox>(w,"wizardUsage")->setCurrentIndex(1);click(w,"wizardNext");
    draw(w,{20,100},{620,100});click(w,"wizardDetect");idle(w);check(widget<QListWidget>(w,"wizardPoints")->count()==2,"Actual caliper endpoints");
    CameraFlowSmokeAudit::save(w,"kui4-known-normal.png");click(w,"wizardNext");widget<QDoubleSpinBox>(w,"wizardKnownMm")->setValue(10);click(w,"wizardNext");check(widget<QLabel>(w,"wizardResult")->text().contains("Hesaplandı"),"Calculated result");
    click(w,"wizardBack");check(widget<QDoubleSpinBox>(w,"wizardKnownMm")->value()==10,"Back keeps inputs");click(w,"wizardNext");click(w,"wizardBeginControl");draw(w,{20,140},{620,140});click(w,"wizardDetect");idle(w);click(w,"wizardNext");widget<QDoubleSpinBox>(w,"wizardControlMm")->setValue(10);click(w,"wizardControl");check(widget<QLabel>(w,"wizardResult")->text().contains("sınır tanımlanmadı"),"Optional control without invented acceptance");widget<QDoubleSpinBox>(w,"wizardControlMm")->setValue(9);widget<QCheckBox>(w,"wizardControlUseLimit")->setChecked(true);widget<QDoubleSpinBox>(w,"wizardControlLimit")->setValue(.1);click(w,"wizardControl");check(widget<QLabel>(w,"wizardResult")->text().contains("sınır dışında"),"Real independent control rejects wrong known length");widget<QDoubleSpinBox>(w,"wizardControlMm")->setValue(10);click(w,"wizardNext");check(!saved && w.isVisible(),"Pending edited control cannot silently save stale acceptance");click(w,"wizardControl");widget<QCheckBox>(w,"wizardControlUseLimit")->setChecked(false);click(w,"wizardControl");widget<QLineEdit>(w,"wizardName")->setText("Referans ölçü");w.resize(850,600);app.processEvents();CameraFlowSmokeAudit::save(w,"kui4-known-small.png");check(widget<QPushButton>(w,"wizardNext")->geometry().bottom()<=w.height(),"Small-screen footer visible");click(w,"wizardNext");check(saved && saved->image().origin=="setup_file" && saved->activatable() && !saved->model().validated(),"File declaration preserves origin and readiness");check(std::abs(saved->model().millimetresPerPixel()-.025)<.0001,"Independent 400px/10mm oracle");check(CalibrationModelRecord::decode(saved->encode()).name()=="Referans ölçü","Name strict round trip");
    auto corrupt=saved->encode();corrupt["name"]="tampered";bool rejected=false;try{CalibrationModelRecord::decode(corrupt);}catch(...){rejected=true;}check(rejected,"Named record content tamper rejected");
    // Legacy catalog behavior is an application contract, independent of retired pages.
    bool gate=true;const auto catalogPath=tmp.filePath("project.vfproject.active-calibrations.json");
    ActiveCalibrationService catalog(catalogPath,"project",[&]{return gate;});
    catalog.save(*saved);const auto acquired=CalibrationCapture::acquire(source,tmp.path(),{});
    auto current=CalibrationCapture::setup(source,tmp.path(),acquired,saved->setup().id,saved->setup().lensId,saved->setup().planeId);
    current.source=saved->setup().source;current.target=saved->setup().target;
    catalog.activate(saved->model().dependency().calibrationId,1,current);
    check(catalog.resolve(saved->setup()).record.name()==saved->name(),"File setup activates after actual acquisition");
    gate=false;bool blocked=false;try{catalog.deactivate();}catch(...){blocked=true;}check(blocked && catalog.active(),"Runtime/ACK gate preserves active");gate=true;
    auto legacyDependency=saved->model().dependency();legacyDependency.calibrationId="legacy-ui";const auto raw=saved->setup().source;
    auto legacyCandidate=MeasurementService{}.teach(legacyDependency,{"teach",{{119.5,100},raw},{{519.5,100},raw},10});
    auto legacyModel=MeasurementService{}.validate(legacyCandidate,{{"h",{{10,30},raw},{{110,30},raw},2.5},{"v",{{30,20},raw},{{30,120},raw},2.5}},.001);
    CalibrationService legacy(tmp.filePath("legacy.calibrations.json"));legacy.save(legacyModel);
    check(CalibrationMigration::adopt(legacy,catalog,"legacy-ui",source,tmp.path(),acquired,"Legacy Hat 1")==1,"Legacy explicit service migration");
    check(catalog.active()->id==saved->model().dependency().calibrationId,"Legacy import preserves active selection");
    check(catalog.records().last().image().origin=="legacy_record" && catalog.records().last().image().pixelHash.isEmpty(),"Legacy migration does not invent missing pixel evidence");
    ActiveCalibrationService other(tmp.filePath("other.active-calibrations.json"),"other",[&]{return gate;});
    check(!other.active() && other.records().isEmpty(),"Project change clears activity");
    ActiveCalibrationService reopened(catalogPath,"project",[&]{return gate;});
    check(!reopened.active() && reopened.remembered(),"Restart remembers without activating");
    for(int pattern=0;pattern<2;++pattern){QImage board(640,480,QImage::Format_RGB32);board.fill(Qt::white);QPainter p(&board);if(pattern==0){for(int y=0;y<7;++y)for(int x=0;x<10;++x)p.fillRect(70+x*45,65+y*45,45,45,(x+y)%2 ? Qt::white:Qt::black);}else{p.setRenderHint(QPainter::Antialiasing);p.setBrush(Qt::black);p.setPen(Qt::NoPen);for(int y=0;y<6;++y)for(int x=0;x<8;++x)p.drawEllipse(QPointF(100+x*45,100+y*45),8,8);}p.end();const auto boardPath=tmp.filePath(QString("plate%1.png").arg(pattern));check(board.save(boardPath),"Plate fixture");std::optional<CalibrationModelRecord> plate;
        CalibrationWizard d(true,source,tmp.path(),[&](const auto& r){plate=r;});d.resize(1400,900);d.show();app.processEvents();setup(d);d.openFile(boardPath);idle(d);click(d,"wizardNext");widget<QComboBox>(d,"wizardPattern")->setCurrentIndex(pattern);widget<QSpinBox>(d,"wizardColumns")->setValue(pattern==0 ? 9:8);widget<QDoubleSpinBox>(d,"wizardSpacing")->setValue(5);widget<QLineEdit>(d,"wizardTargetName")->setText(QString("Plate %1").arg(pattern));click(d,"wizardSaveTarget");check(widget<QComboBox>(d,"wizardTargets")->findText(QString("Plate %1").arg(pattern))>=0,"Named target definition can be reused");click(d,"wizardDetect");idle(d);click(d,"wizardNext");check(widget<QLabel>(d,"wizardHeading")->text().startsWith("3"),"Plate detection advance");CameraFlowSmokeAudit::save(d,QString("kui4-plate%1-normal.png").arg(pattern));click(d,"wizardNext");d.resize(850,600);app.processEvents();CameraFlowSmokeAudit::save(d,QString("kui4-plate%1-small.png").arg(pattern));click(d,"wizardNext");check(plate && !plate->activatable() && plate->model().mapping(),"Plate trial is calculated, cannot activate");check(plate->model().mapping()->observation().target.pattern==(pattern==0 ? CalibrationGridPattern::Chessboard:CalibrationGridPattern::SymmetricCircles),"Plate descriptor preserved");
    }
    CalibrationWizard cancel(false,source,tmp.path(),[](const auto&){throw std::runtime_error("Unexpected save");});cancel.show();setup(cancel);cancel.openFile(path);click(cancel,"wizardStop");idle(cancel);check(!widget<QLabel>(cancel,"wizardImageInfo")->text().contains("640"),"Stop suppresses late file completion");click(cancel,"wizardCancel");check(cancel.result()==QDialog::Rejected,"Cancel discards draft");
    const auto independentPath=tmp.filePath("independent.vfproject");
    ProjectService independentProject;
    independentProject.createProject("Recipe-free acceptance","",independentPath,"file");
    independentProject.save();
    QFile independentFile(independentPath);
    // CCUX-15h: normal wizard commands, immutable raw observations and failed fit.
    for(int scene=0;scene<5;++scene){
        const auto path=tmp.filePath(QString("framed-%1.png").arg(scene));check(framedRaster(scene==2,scene!=3,scene==1,scene==4).save(path),"framed raster save");
        CalibrationWizard d(true,source,tmp.path(),[](const auto&){});d.resize(1400,900);d.show();app.processEvents();setup(d);d.openFile(path);idle(d);click(d,"wizardNext");
        widget<QComboBox>(d,"wizardPattern")->setCurrentIndex(2);check(widget<QSpinBox>(d,"wizardColumns")->value()==7 && !widget<QSpinBox>(d,"wizardRows")->isEnabled(),"explicit locked 7x7 profile");
        widget<QDoubleSpinBox>(d,"wizardSpacing")->setValue(12);click(d,"wizardDetect");check(widget<QLabel>(d,"wizardStatus")->text().contains("teyit"),"spacing confirmation required");widget<QCheckBox>(d,"wizardSpacingConfirmed")->setChecked(true);
        auto* viewer=widget<ImageViewer>(d,"wizardViewer");viewer->zoomBy(1.15);
        if(scene==0){click(d,"wizardPlateRoi");auto a=viewer->mapFromScene(QPointF(50,50)),b=viewer->mapFromScene(QPointF(750,750));QTest::mousePress(viewer->viewport(),Qt::LeftButton,{},a);QTest::mouseMove(viewer->viewport(),b,10);QTest::mouseRelease(viewer->viewport(),Qt::LeftButton,{},b);}
        click(d,"wizardDetect");CameraFlowSmokeAudit::save(d,evidence+QString("/framed-%1-busy.png").arg(scene));idle(d);
        const bool ready=scene!=2 && scene!=3;auto text=widget<QLabel>(d,"wizardPlateDiagnostics")->text();check(text.contains(ready?"49 eşleşen":"0 eşleşen"),qPrintable(text));
        if(scene==1)check(text.contains("58 aday"),"extra candidates distinguished from assigned centres");
        CameraFlowSmokeAudit::save(d,evidence+QString("/framed-%1-normal.png").arg(scene));d.resize(850,600);viewer->fitImage();app.processEvents();CameraFlowSmokeAudit::save(d,evidence+QString("/framed-%1-small.png").arg(scene));
        if(ready){click(d,"wizardNext");click(d,"wizardNext");if(scene==4){check(widget<QLabel>(d,"wizardHeading")->text().startsWith("3") && widget<QLabel>(d,"wizardDistance")->text().contains("reddedildi"),"fit reject retains observation without save");CameraFlowSmokeAudit::save(d,evidence+"/framed-fit-reject-small.png");d.resize(1400,900);app.processEvents();CameraFlowSmokeAudit::save(d,evidence+"/framed-fit-reject-normal.png");}else {check(widget<QLabel>(d,"wizardHeading")->text().startsWith("4"),"framed candidate calculated");CameraFlowSmokeAudit::save(d,evidence+QString("/framed-%1-model-small.png").arg(scene));d.resize(1400,900);app.processEvents();CameraFlowSmokeAudit::save(d,evidence+QString("/framed-%1-model-normal.png").arg(scene));}}
        click(d,"wizardStop");CameraFlowSmokeAudit::save(d,evidence+QString("/framed-%1-stopped.png").arg(scene));
        d.openFile(path);idle(d);click(d,"wizardNext");widget<QCheckBox>(d,"wizardSpacingConfirmed")->setChecked(true);click(d,"wizardDetect");
        // Changing the target invalidates an in-flight completion even through programmatic UI input.
        widget<QDoubleSpinBox>(d,"wizardSpacing")->setValue(13);idle(d);check(widget<QLabel>(d,"wizardPlateDiagnostics")->text().contains("Gözlem yok"),"late detection cannot replace changed target");
        CameraFlowSmokeAudit::save(d,evidence+QString("/framed-%1-reopened.png").arg(scene));
        if(scene==0){widget<QCheckBox>(d,"wizardSpacingConfirmed")->setChecked(true);click(d,"wizardDetect");click(d,"wizardStop");idle(d);check(widget<QLabel>(d,"wizardPlateDiagnostics")->text().contains("Gözlem yok"),"stop rejects pending plate result");CameraFlowSmokeAudit::save(d,evidence+"/framed-cancel-normal.png");d.resize(850,600);app.processEvents();CameraFlowSmokeAudit::save(d,evidence+"/framed-cancel-small.png");}
        d.close();
    }
    check(independentFile.open(QIODevice::ReadOnly),"Saved empty project readable");
    const auto emptyProjectBytes=independentFile.readAll();independentFile.close();
    check(independentProject.project().recipes.isEmpty(),"Recipe-free acceptance begins with saved empty project");
    MeasurementWorkspace workspace(independentPath,independentProject.project().id,[]{return true;});
    MeasurementCamera camera{{"camera-A"},"Camera",{"webcam","7174",DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}};
    MeasurementSetupDescriptor descriptor{{"setup-A"},camera.id,"Setup","lens","plane",1,1,{{640,200},{},{},true}};workspace.replace({{camera},{descriptor}});
    IndependentCaptureRequest target;target.camera=camera;target.setup=descriptor;
    std::optional<CalibrationModelRecord> independentRecord;
    for(int trial=0;trial<3;++trial){IndependentCaptureBackend backend{[&]{return QVector<DeviceBinding>{camera.device};},[&](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<CameraFixture>(stripe);}};CalibrationWizard independent(false,target,tmp.path(),[&](const auto& r){if(trial<2)independentRecord=r;if(trial==1)workspace.saveTrial(r);else workspace.calibrations().save({camera.id,descriptor.id,1},r);},nullptr,backend);independent.show();app.processEvents();
        widget<QComboBox>(independent,"wizardUsage")->setCurrentIndex(trial==1 ? 0:1);if(trial==2)click(independent,"wizardAcquire");else independent.openFile(path);idle(independent);click(independent,"wizardNext");draw(independent,{20,100},{620,100});click(independent,"wizardDetect");idle(independent);click(independent,"wizardNext");widget<QDoubleSpinBox>(independent,"wizardKnownMm")->setValue(10);click(independent,"wizardNext");
        independent.resize(850,600);app.processEvents();CameraFlowSmokeAudit::save(independent,QString("kmc6-independent-%1-small.png").arg(trial));click(independent,"wizardNext");
        check(independentRecord && std::abs(independentRecord->model().millimetresPerPixel()-.025)<.00001,"Recipe-free wizard real 400px/10mm");check(workspace.environment().selections.isEmpty(),"Wizard save does not activate");}
    auto plateSetup=descriptor;plateSetup.id={"setup-plate"};plateSetup.geometry.resolution={640,480};
    workspace.replace({{camera},{descriptor,plateSetup}});
    const SetupReference plateRef{camera.id,plateSetup.id,plateSetup.revision};
    for(int pattern=0;pattern<2;++pattern){
        const auto boardPath=tmp.filePath(QString("plate%1.png").arg(pattern));
        const auto board=QImage(boardPath).convertToFormat(QImage::Format_Grayscale8);
        for(int origin=0;origin<3;++origin){
            IndependentCaptureRequest request;
            if(origin){request.camera=camera;request.setup=plateSetup;}
            IndependentCaptureBackend backend{[&]{return QVector<DeviceBinding>{camera.device};},[&](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<CameraFixture>(board);}};
            std::optional<CalibrationModelRecord> plateRecord;
            CalibrationWizard plateWizard(true,request,tmp.path(),[&](const auto& r){plateRecord=r;if(origin)workspace.calibrations().save(plateRef,r);else workspace.saveTrial(r);},nullptr,backend);
            plateWizard.resize(1400,900);plateWizard.show();app.processEvents();
            widget<QComboBox>(plateWizard,"wizardUsage")->setCurrentIndex(origin ? 1:0);
            if(origin==2)click(plateWizard,"wizardAcquire");else plateWizard.openFile(boardPath);
            idle(plateWizard);click(plateWizard,"wizardNext");
            widget<QComboBox>(plateWizard,"wizardPattern")->setCurrentIndex(pattern);
            widget<QSpinBox>(plateWizard,"wizardColumns")->setValue(pattern==0 ? 9:8);
            widget<QDoubleSpinBox>(plateWizard,"wizardSpacing")->setValue(5);
            click(plateWizard,"wizardDetect");idle(plateWizard);click(plateWizard,"wizardNext");click(plateWizard,"wizardNext");
            check(CameraFlowSmokeAudit::save(plateWizard,QString("kmc9-plate%1-origin%2-normal.png").arg(pattern).arg(origin)),"Plate normal visual evidence");
            plateWizard.resize(850,600);app.processEvents();
            check(CameraFlowSmokeAudit::save(plateWizard,QString("kmc9-plate%1-origin%2-small.png").arg(pattern).arg(origin)),"Plate small visual evidence");
            check(widget<QPushButton>(plateWizard,"wizardNext")->geometry().bottom()<=plateWizard.height(),"Plate small-screen footer visible");
            click(plateWizard,"wizardNext");
            check(plateWizard.result()==QDialog::Accepted && plateRecord && plateRecord->model().mapping(),"Recipe-free plate saves actual H for each origin");
            check(plateRecord->image().origin==(origin==0 ? "trial_file":origin==1 ? "setup_file":"acquired") && plateRecord->activatable()==(origin!=0),"Plate origin and candidate readiness remain distinct");
            const auto& mapping=*plateRecord->model().mapping();
            check(mapping.observation().target.pattern==(pattern==0 ? CalibrationGridPattern::Chessboard:CalibrationGridPattern::SymmetricCircles),"Independent plate pattern preserved");
            check(std::abs(PlaneMappingService::distance(mapping,mapping.observation().pixels[0],mapping.observation().pixels[1]).millimetres-5)<.01,"Independent 45px/5mm plate oracle");
            check(CalibrationModelRecord::decode(plateRecord->encode()).encode()==plateRecord->encode(),"Each plate origin strict replay");
            check(workspace.environment().selections.isEmpty(),"Plate save never activates");
        }
    }
    const auto original=*independentRecord;const auto originalBytes=original.encode();
    const SetupReference ref{camera.id,descriptor.id,1};const auto associated=workspace.associate(original,ref,path);
    check(original.encode()==originalBytes && associated.originalContentId()==original.contentId() && associated.image().origin=="setup_file" && associated.model().dependency().cameraId==original.model().dependency().cameraId,"Association retains original raster and session provenance");
    check(CalibrationModelRecord::decode(associated.encode()).encode()==associated.encode(),"Association strict replay");check(workspace.associate(original,ref,path).contentId()==associated.contentId(),"Association idempotent retry");
    bool missingRejected=false;try{workspace.associate(original,ref,tmp.filePath("missing.png"));}catch(...){missingRejected=true;}check(missingRejected,"Missing original raster cannot associate");
    QImage changed=stripe;changed.setPixelColor(5,5,Qt::white);const auto changedPath=tmp.filePath("changed.png");changed.save(changedPath);bool changedRejected=false;try{workspace.associate(original,ref,changedPath);}catch(...){changedRejected=true;}check(changedRejected,"Changed raster cannot associate");
    CalibrationWizard independentCancel(false,target,tmp.path(),[](const auto&){throw std::runtime_error("Unexpected independent save");});independentCancel.show();independentCancel.openFile(path);click(independentCancel,"wizardStop");idle(independentCancel);check(!widget<QLabel>(independentCancel,"wizardImageInfo")->text().contains("640"),"Independent Stop rejects late file");
    // CCUX-09: both methods issue the actual one-command application operation.
    MeasurementWorkspace commandWorkspace(tmp.filePath("ccux09.vfproject"),"ccux09",[]{return true;});
    commandWorkspace.replace({{camera},{descriptor}});
    for(int mode=0;mode<2;++mode){
        const auto selectedSetup=mode ? plateSetup:descriptor;
        // A Ready default is enough; no user-facing operational/profile selection.
        if(mode){commandWorkspace.environment();commandWorkspace.replace({{camera},{descriptor,plateSetup}});commandWorkspace.calibrations().setOperational(camera.id,plateSetup.id);}
        auto request=IndependentCaptureRequest::fromDefaults(commandWorkspace.environment(),camera.id);
        const auto raster=mode ? QImage(tmp.filePath("plate0.png")).convertToFormat(QImage::Format_Grayscale8):stripe;
        IndependentCaptureBackend backend{[&]{return QVector<DeviceBinding>{camera.device};},[raster](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<CameraFixture>(raster);}};
        CalibrationWizard commandWizard(mode,request,tmp.path(),[](const auto&){throw std::runtime_error("Separate save callback must not run");},nullptr,backend);
        const SetupReference commandRef{camera.id,selectedSetup.id,1};const auto base=commandWorkspace.environment().revision;
        bool fail=true;QString partialId;std::optional<CalibrationSaveUseResult> outcome;
        commandWizard.setSaveAndUse([&](const auto& record,const auto& probe){outcome=commandWorkspace.calibrations().saveAndUse({commandRef,base,probe.evidence()->generation},record,probe,[]{return true;},fail ? MeasurementEnvironmentRepository::Fault::AfterCandidate:MeasurementEnvironmentRepository::Fault::None);return *outcome;});
        commandWizard.resize(1400,900);commandWizard.show();app.processEvents();
        check(!widget<QLineEdit>(commandWizard,"wizardLens")->isVisible(),"Normal camera wizard hides fixed installation fields");
        // File plus fresh live probe exercises preservation of file provenance.
        if(mode)commandWizard.openFile(tmp.filePath("plate0.png"));else click(commandWizard,"wizardAcquire");idle(commandWizard);click(commandWizard,"wizardNext");
        if(mode){widget<QSpinBox>(commandWizard,"wizardColumns")->setValue(9);widget<QDoubleSpinBox>(commandWizard,"wizardSpacing")->setValue(5);}else draw(commandWizard,{20,100},{620,100});
        click(commandWizard,"wizardDetect");idle(commandWizard);click(commandWizard,"wizardNext");if(!mode)widget<QDoubleSpinBox>(commandWizard,"wizardKnownMm")->setValue(10);click(commandWizard,"wizardNext");
        const auto previous=commandWorkspace.environment().selections;
        click(commandWizard,"wizardNext");idle(commandWizard);check(outcome && outcome->state==CalibrationSaveUseState::CandidateSaved && commandWorkspace.environment().selections==previous,"Partial save preserves previous selection");partialId=outcome->contentId;
        check(widget<QLabel>(commandWizard,"wizardResult")->text().contains("Aday kaydedildi"),"Partial result is readable and retryable");
        CameraFlowSmokeAudit::save(commandWizard,(evidence+QString("/wizard-%1-partial-1400.png")).arg(mode));fail=false;
        click(commandWizard,"wizardNext");idle(commandWizard);check(outcome->state==CalibrationSaveUseState::Activated && outcome->duplicate && outcome->contentId==partialId,"Retry reuses immutable candidate and activates");
        check(commandWorkspace.calibrations().snapshots({commandRef}).first().calibration.record.image().origin==(mode ? "setup_file":"acquired"),"Save/use retains acquired/file provenance");
        commandWizard.resize(850,600);app.processEvents();CameraFlowSmokeAudit::save(commandWizard,(evidence+QString("/wizard-%1-ready-850.png")).arg(mode));click(commandWizard,"wizardNext");check(commandWizard.result()==QDialog::Accepted,"Successful workflow closes explicitly");
    }
    // Typed experimental image evidence must survive the UI math session.
    {auto r=oecs_experimental_fixture::request();auto device=oecs_experimental_fixture::state();device->pixels.fill(char(20));for(int y=0;y<2048;++y)for(int x=120;x<520;++x)device->pixels[y*2448+x]=char(220);
        MeasurementWorkspace experimental(tmp.filePath("ccux09-experimental.vfproject"),"experimental",[]{return true;});experimental.replace({{r.camera},{r.setup},{r.profile}});
        auto request=IndependentCaptureRequest::fromDefaults(experimental.environment(),r.camera.id);request.timeoutMs=3000;
        IndependentCaptureBackend backend;backend.industrialDiscover=[r]{return IndustrialDiscoverySnapshot{{r.device},{},false};};auto opens=std::make_shared<std::atomic_int>(0),closes=std::make_shared<std::atomic_int>(0);backend.experimentalApi=[device,opens,closes]{return std::make_unique<CountedOecsApi>(device,opens,closes);};
        CalibrationWizard experimentalWizard(false,request,tmp.path(),[](const auto&){},nullptr,backend);const auto base=experimental.environment().revision;const SetupReference ref{r.camera.id,r.setup.id,1};
        bool saveFailure=true;
        experimentalWizard.setSaveAndUse([&](const auto& record,const auto& probe){check(record.image().experimental.has_value(),"UI preserves typed experimental raster/model evidence");if(saveFailure){CalibrationSaveUseResult failed;failed.error="fixture save failure";return failed;}return experimental.saveAndUseCalibration({ref,base,probe.evidence()->generation,CalibrationCapturePolicy::Automatic},record,probe,[]{return true;});});
        experimentalWizard.resize(1400,900);experimentalWizard.show();app.processEvents();click(experimentalWizard,"wizardAcquire");idle(experimentalWizard);click(experimentalWizard,"wizardNext");draw(experimentalWizard,{20,100},{620,100});click(experimentalWizard,"wizardDetect");idle(experimentalWizard);click(experimentalWizard,"wizardNext");widget<QDoubleSpinBox>(experimentalWizard,"wizardKnownMm")->setValue(10);click(experimentalWizard,"wizardNext");
        check(!widget<QLabel>(experimentalWizard,"wizardResult")->text().isEmpty() && !widget<QLabel>(experimentalWizard,"wizardResult")->text().contains("Deneysel"),"calculation shown without acquisition mode label");CameraFlowSmokeAudit::save(experimentalWizard,evidence+"/experimental-result-1400.png");click(experimentalWizard,"wizardNext");
        check(*opens==1 && *closes==0 && !experimental.calibrations().status(ref).sessionPermitted,"save failure preserves live connection without activation");saveFailure=false;click(experimentalWizard,"wizardNext");
        wait([&]{return *closes==1;});idle(experimentalWizard);check(*opens==1,"successful save closes same connection");
        check(experimental.calibrations().status(ref).sessionPermitted && widget<QLabel>(experimentalWizard,"wizardResult")->text().contains("Bağımsız kontrol yapılmadı / bilinmiyor"),"UI experimental one-command activation remains experimental");
        experimentalWizard.resize(850,600);app.processEvents();CameraFlowSmokeAudit::save(experimentalWizard,evidence+"/experimental-ready-850.png");click(experimentalWizard,"wizardNext");
        CalibrationOverview overview([&]{return &experimental;});overview.resize(850,600);overview.show();app.processEvents();check(widget<QLabel>(overview,"overviewDetail")->text().contains("Bağımsız kontrol yapılmadı / bilinmiyor"),"Overview identifies experimental selection");CameraFlowSmokeAudit::save(overview,evidence+"/experimental-overview-850.png");
        MeasurementWorkspace restart(experimental.projectPath(),"experimental",[]{return true;});CalibrationOverview offline([&]{return &restart;});offline.resize(850,600);offline.show();app.processEvents();check(widget<QLabel>(offline,"overviewDetail")->text().contains("Kayıtlı / bağlantı") && !widget<QLabel>(offline,"overviewDetail")->text().contains("mm/px"),"Restart shows remembered experimental model without active values");CameraFlowSmokeAudit::save(offline,evidence+"/remembered-overview-850.png");
    }
    // CCUX-22e: real wizard commands, persistent industrial fixture, no solver needed.
    {
        persistent_fixture::Fixture f("wizard-session");const auto r=f.requests.first();auto probe=f.first();
        IndependentCaptureRequest request;request.origin=SourceOrigin::CameraAcquired;request.camera=r.camera;request.setup=r.setup;request.profile=r.profile;request.policy=CalibrationCapturePolicy::Trusted;
        IndependentCaptureBackend backend;backend.factory=f.inputs.factory;backend.industrialDiscover=[discover=f.inputs.discover]{return discover(5000,{});};
        CalibrationWizard wizard(true,request,tmp.path(),[](const auto&){},nullptr,backend);wizard.resize(1400,900);wizard.show();
        wait([&]{return probe->configs==1;});check(probe->grabs==0,"wizard entry opens without capture");
        for(int n=0;n<20;++n){click(wizard,"wizardAcquire");idle(wizard);check(widget<QLabel>(wizard,"wizardImageInfo")->text().contains("3×2"),"persistent wizard acquired raster");}
        check(probe->opens==1 && probe->configs==1 && probe->grabs==20 && probe->closes==0 && *f.discoveries==1,"wizard twenty captures reuse connection");
        CameraFlowSmokeAudit::save(wizard,evidence+"/session-normal.png");wizard.resize(850,600);app.processEvents();CameraFlowSmokeAudit::save(wizard,evidence+"/session-small.png");
        QImage file(3,2,QImage::Format_Grayscale8);file.fill(17);const auto filePath=tmp.filePath("session-file.png");check(file.save(filePath),"session file fixture");wizard.openFile(filePath);idle(wizard);wait([&]{return probe->closes==1;});check(widget<QLabel>(wizard,"wizardImageInfo")->text().contains("setup_file"),"file switch preserves origin and releases camera");
        click(wizard,"wizardAcquire");idle(wizard);check(probe->opens==2,qPrintable(QString("explicit camera return reopens: opens=%1 status=%2").arg(probe->opens.load()).arg(widget<QLabel>(wizard,"wizardStatus")->text())));
        std::promise<void> release;probe->closeGate=release.get_future().share();probe->closeEntered=false;
        click(wizard,"wizardCancel");wait([&]{return probe->closeEntered.load();});check(wizard.isVisible(),"dialog remains while SDK close pending");release.set_value();wait([&]{return !wizard.isVisible();});check(probe->closes==2 && !probe->wrongThread,"dialog closes only after actual drain");
    }
    check(independentFile.open(QIODevice::ReadOnly) && independentFile.readAll()==emptyProjectBytes,"Independent calibration preserves saved project bytes");
    check(ProjectRepository{}.load(independentPath).recipes.isEmpty(),"Independent methods and origins never create a hidden recipe");
    QTextStream(stdout)<<"PASS "<<checks<<" UI checks; fixtures "<<tmp.path()<<'\n';tmp.setAutoRemove(false);return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
