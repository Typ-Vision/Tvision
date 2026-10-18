#include "IndustrialProviderFixture.hpp"
#include "app/CalibrationOverview.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QDialog>
#include <QTimer>
#include <QTest>
#include <QElapsedTimer>
#include <QScrollArea>
#include <QFile>
#include <QFontDatabase>
using namespace industrial_provider_fixture;
int main(int argc,char** argv){QApplication app(argc,argv);try{
    check(QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf")>=0,"offscreen review font");
    QDir().mkpath("icm/06b");QFile theme("../../resources/theme.qss");if(theme.open(QIODevice::ReadOnly))app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    QTemporaryDir dir(QDir::currentPath()+"/industrial-activation-XXXXXX");check(dir.isValid(),"build-only fixture");auto r=request();auto state=std::make_shared<State>();QImage im(640,200,QImage::Format_Grayscale8);im.fill(20);for(int y=0;y<200;++y)for(int x=120;x<520;++x)im.scanLine(y)[x]=220;state->raster=im;
    r.setup.geometry.resolution=im.size();r.setup.geometry.sensorRoi=QRect(QPoint(0,0),im.size());r.camera.name="Industrial calibration fixture";
    MeasurementWorkspace ws(dir.filePath("project.vfproject"),"p",[]{return true;});ws.replace({{r.camera},{r.setup},{r.profile}});const SetupReference ref{r.camera.id,r.setup.id,1};
    IndependentCaptureRequest req;req.origin=SourceOrigin::CameraAcquired;req.camera=r.camera;req.setup=r.setup;req.profile=r.profile;
    IndependentCaptureBackend backend;backend.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));backend.industrialDiscover=[&]{return IndustrialDiscoverySnapshot{{r.device},{},false};};
    IndependentCalibrationSession session;check(session.accept(IndependentCalibrationSession::run(session.begin(req),backend)),"fixture teaching raster");auto& m=session.math();check(m.accept(CalibrationWizardSession::run(m.scanPair("width",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"real teaching pair");m.setKnownMillimetres(10);const auto model=m.calculate();ws.calibrations().save(ref,model);ws.calibrations().setOperational(r.camera.id,r.setup.id);
    CalibrationOverview view([&]{return &ws;});view.setCaptureBackendForTests(backend);view.resize(1400,900);view.show();app.processEvents();auto* profiles=view.findChild<QComboBox*>("overviewProfiles");check(profiles && profiles->isVisible() && profiles->currentData()==r.profile.id.value,"exact registered profile selector");check(state->hardware->commands==1,"view selection never acquires");
    auto* rows=view.findChild<QTableWidget*>("overviewRecords");rows->selectRow(0);view.grab().save("icm/06b/overview-normal.png");view.resize(850,600);app.processEvents();view.findChild<QScrollArea*>()->ensureWidgetVisible(profiles);app.processEvents();view.grab().save("icm/06b/overview-small.png");
    QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());check(d && d->objectName()=="scopedActivationReview","industrial review");auto* action=d->findChild<QPushButton*>("activationAcquire");check(action->isEnabled() && d->findChild<QLabel*>("activationReviewText")->text().contains(r.profile.name),"profile shown before activation");d->resize(900,650);app.processEvents();d->grab().save("icm/06b/review-normal.png");d->resize(850,600);app.processEvents();d->grab().save("icm/06b/review-small.png");action->click();});view.findChild<QPushButton*>("overviewMethod3")->click();check(ws.calibrations().snapshots({ref}).size()==1 && state->hardware->commands==2,"UI explicit industrial probe activates");
    auto product=backend;product.factory=CameraProviderFactory{};view.setCaptureBackendForTests(product);view.refresh();rows->selectRow(0);QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());auto* action=d->findChild<QPushButton*>("activationAcquire");action->click();QElapsedTimer t;t.start();while(!action->isEnabled() && t.elapsed()<15000){app.processEvents();QTest::qWait(5);}check(action->isEnabled() && !d->findChild<QLabel*>("activationStatus")->text().isEmpty(),"unadmitted native error shown");d->grab().save("icm/06b/unavailable.png");d->reject();});view.findChild<QPushButton*>("overviewMethod3")->click();
    reject([&]{ws.calibrations().snapshots({ref});});
    QTextStream(stdout)<<"PASS: industrial profile selection, explicit probe/activation, normal/small review, normal factory error; synthetic fixture only\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
