#include "app/CameraManager.hpp"
#include "app/CalibrationOverview.hpp"
#include "IndustrialProviderFixture.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QDir>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <future>
using namespace industrial_provider_fixture;
namespace {
template<class F> void waitUi(F f){QElapsedTimer t;t.start();while(!f() && t.elapsed()<5000){QApplication::processEvents();QTest::qWait(1);}check(f(),"UI wait timeout");}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");
    QTemporaryDir dir(QDir::currentPath()+"/icm-camera-ui-XXXXXX");check(dir.isValid(),"UI artifact directory");
    const auto evidence=QDir::currentPath()+"/../icm/05b";QDir().mkpath(evidence);
    bool gate=true;int count=0;bool missing=false;
    MeasurementWorkspace workspace(dir.path()+"/project.vfproject","camera-ui",[&]{return gate;});
    CameraSession jobs;
    auto scanner=[&](const QStringList&,const std::shared_ptr<std::atomic_bool>& cancel){
        IndustrialDiscoverySnapshot snapshot;
        for(int i=0;i<count;++i){auto d=request().device;if(i){d.identity.serial=QString(60,'S')+QString::number(i);d.identity.model="Synthetic industrial long model "+QString(50,'M');}snapshot.devices.append(d);}
        if(missing)snapshot.errors.append("Synthetic: producer/SDK missing");if(cancel->load()){snapshot.devices.clear();snapshot.cancelled=true;}
        return CameraService::combine({},snapshot);
    };
    auto state=std::make_shared<State>();auto factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));
    CameraManager view([&]{return &workspace;},[&]{return gate;},jobs,dir.path()+"/local.ini",nullptr,scanner,factory);
    view.resize(1100,950);view.show();
    auto* table=view.findChild<QTableWidget*>("cameraDevices");
    auto* status=view.findChild<QLabel*>("cameraStatus");
    auto scan=[&]{view.scan();waitUi([&]{return !jobs.busy();});};
    for(int n:{0,1,8,10}){count=n;scan();check(table->rowCount()==n,"0/1/8/10 synthetic inventory");
        view.findChild<QLabel*>("cameraTitle")->setText(QString("Sentetik UI fixture • %1 kamera").arg(n));
        view.grab().save(evidence+QString("/camera-%1-normal.png").arg(n));
    }
    table->selectRow(9);view.resize(620,600);QApplication::processEvents();view.grab().save(evidence+"/camera-long-small.png");
    check(view.findChild<QScrollArea*>()->horizontalScrollBar()->maximum()==0,"small window wraps long identity without page overflow");
    view.resize(1100,950);table->selectRow(0);view.findChild<QLineEdit*>("cameraName")->setText("Synthetic test camera");
    view.findChild<QPushButton*>("cameraRegister")->click();auto e=workspace.environment();
    check(e.current.cameras.size()==1,"register shared CameraId");const auto camera=e.current.cameras.first();
    MeasurementWorkspace other(dir.path()+"/project.vfproject","camera-ui",[]{return true;});
    check(other.environment().current.cameras.first().id==camera.id,"calibration registry sees exact same camera");
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());check(dialog,"profile draft dialog");
        dialog->findChild<QLineEdit*>("draftProfileName")->setText("Synthetic profile");
        dialog->findChild<QLineEdit*>("draftLens")->setText("test lens");dialog->findChild<QLineEdit*>("draftPlane")->setText("test plane");
        dialog->findChild<QSpinBox*>("draftWidth")->setValue(3);dialog->findChild<QSpinBox*>("draftHeight")->setValue(2);
        dialog->findChild<QLineEdit*>("draftExposure")->setText("100");dialog->findChild<QLineEdit*>("draftGain")->setText("1");
        dialog->grab().save(evidence+"/camera-profile-draft.png");
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
    });view.findChild<QPushButton*>("cameraProfileEdit")->click();
    check(workspace.environment().current.profiles.size()==1 && view.findChild<QLabel*>("cameraReadback")->text().contains("yok"),"draft profile saved without claiming readback");
    auto* apply=view.findChild<QPushButton*>("cameraApply");auto* preview=view.findChild<QPushButton*>("cameraPreview");
    check(apply->isEnabled() && preview->isEnabled(),"test provider capabilities enable supported controls");
    view.findChild<QLineEdit*>("cameraExposure")->setText("110");apply->click();
    check(jobs.busy() && !apply->isEnabled() && !preview->isEnabled(),"busy blocks settings and new session");
    waitUi([&]{return !jobs.busy();});
    check(view.findChild<QLabel*>("cameraReadback")->text().contains("110"),"actual configured readback displayed");
    preview->click();waitUi([&]{return status->text().contains("Önizleme •");});
    check(!view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"selected synthetic owned preview");
    view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(view.findChild<QScrollArea*>()->verticalScrollBar()->maximum());
    view.grab().save(evidence+"/camera-preview.png");
    view.stop();check(view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"Stop immediately clears stale preview");
    waitUi([&]{return !jobs.busy();});
    gate=false;view.refresh();check(!apply->isEnabled(),"external Runtime busy gate");gate=true;
    count=0;scan();check(table->rowCount()==1 && table->item(0,4)->text().contains("Çevrimdışı"),"registered camera remains offline");
    missing=true;scan();check(status->text().contains("SDK missing") && !preview->isEnabled(),"missing SDK error no preview");
    view.findChild<QLabel*>("cameraTitle")->setText("Sentetik UI fixture • SDK eksik / kayıtlı kamera");
    view.resize(620,600);view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(0);view.grab().save(evidence+"/camera-small-error.png");
    missing=false;count=1;view.scan();view.stop();waitUi([&]{return !jobs.busy();});
    check(view.inventory().cancelled && table->rowCount()==1,"cancel keeps registry but clears discovery");
    // A late preview completion after Stop cannot repopulate the image or release ownership early.
    count=1;scan();std::promise<void> release;auto released=release.get_future().share();std::atomic_bool entered=false;
    state->hardware->hook=[&](const QString& event){if(event=="preview"){entered=true;released.wait();}};
    preview->click();waitUi([&]{return entered.load();});view.stop();
    check(jobs.busy() && view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"blocked preview retains worker ownership after Stop");
    release.set_value();waitUi([&]{return !jobs.busy();});
    check(view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"late preview suppressed after joined shutdown");
    {CameraSession nativeJobs;CameraManager native([&]{return &workspace;},[]{return true;},nativeJobs,dir.path()+"/native.ini",nullptr,scanner);
     native.scan();waitUi([&]{return !nativeJobs.busy();});check(!native.findChild<QPushButton*>("cameraPreview")->isEnabled() && !native.findChild<QPushButton*>("cameraApply")->isEnabled(),"normal composition rejects synthetic native capability");}
    const auto expEvidence=QDir::currentPath()+"/../icm/oecs-exp-01";QDir().mkpath(expEvidence);
    view.findChild<QCheckBox*>("cameraExperimental")->setChecked(true);
    view.findChild<QLabel*>("cameraTitle")->setText("Sentetik UI • deneysel mod / edinim yok");
    view.resize(1100,950);QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-normal.png");
    view.resize(620,600);QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-small.png");
    view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(view.findChild<QScrollArea*>()->verticalScrollBar()->maximum());QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-small-bottom.png");
    check(view.findChild<QLabel*>("cameraExperimentalNotice")->isVisible() && !view.findChild<QPushButton*>("cameraSaveFrame")->isEnabled(),"persistent experimental notice; no raster means no save");
    check(view.findChild<QScrollArea*>()->horizontalScrollBar()->maximum()==0,"experimental controls fit small window");
    QTextStream(stdout)<<"PASS: synthetic 0/1/8/10, shared CameraId, readback/preview, busy/Stop, offline/error, normal factory isolation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
