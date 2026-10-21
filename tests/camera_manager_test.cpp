#include "app/CameraManager.hpp"
#include "app/CalibrationOverview.hpp"
#include "CameraSettingsFixture.hpp"
#include <QFile>
#include <QGroupBox>
#include <QComboBox>
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
class UiProvider final:public IndustrialCameraProvider {
    Provider preview_;
    camera_settings_fixture::SettingsProvider settings_;
public:
    UiProvider(std::shared_ptr<State> state,std::shared_ptr<camera_settings_fixture::SettingsHardware> h):preview_(state),settings_(h){}
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice& d) const override{return preview_.capabilities(d);}
    std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest& r) const override{return preview_.create(r);}
    std::unique_ptr<IndustrialCameraBackend> createSettings(const MeasurementCamera& c,const DiscoveredDevice& d) const override{return settings_.createSettings(c,d);}
};
template<class F> void waitUi(F f){QElapsedTimer t;t.start();while(!f() && t.elapsed()<5000){QApplication::processEvents();QTest::qWait(1);}check(f(),"UI wait timeout");}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");
    app.setStyle("Fusion");QFile theme(":/resources/theme.qss");check(theme.open(QIODevice::ReadOnly),"product theme resource");app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    QTemporaryDir dir(QDir::currentPath()+"/icm-camera-ui-XXXXXX");check(dir.isValid(),"UI artifact directory");
    const auto evidence=QDir::currentPath()+"/../ccux/ccux-03";QDir().mkpath(evidence);
    bool gate=true;int count=0;bool missing=false;bool ambiguous=false;bool reversed=false;bool multiRoute=false;
    QFile project(dir.path()+"/project.vfproject");check(project.open(QIODevice::WriteOnly),"saved project fixture");project.write("{}");project.close();
    CameraSession jobs;
    MeasurementWorkspace workspace(dir.path()+"/project.vfproject","camera-ui",[&]{return gate && !jobs.busy();});
    auto scanner=[&](const QStringList&,const std::shared_ptr<std::atomic_bool>& cancel){
        IndustrialDiscoverySnapshot snapshot;
        for(int i=0;i<count;++i){auto d=request().device;if(i){d.identity.serial=QString(60,'S')+QString::number(i);d.identity.model="Synthetic industrial long model "+QString(50,'M');}snapshot.devices.append(d);}
        if(multiRoute){auto d=snapshot.devices.first();d.route.interfaceId="second-route";d.route.backend="gentl";d.route.producer="C:/synthetic/second.cti";d.backend.adapter="gentl";snapshot.devices.append(d);}
        if(ambiguous){auto d=snapshot.devices.first();snapshot.devices.append(d);}
        if(reversed)std::reverse(snapshot.devices.begin(),snapshot.devices.end());
        if(missing)snapshot.errors.append("Synthetic: producer/SDK missing");if(cancel->load()){snapshot.devices.clear();snapshot.cancelled=true;}
        return CameraService::combine({},snapshot);
    };
    auto state=std::make_shared<State>();
    auto h=std::make_shared<camera_settings_fixture::SettingsHardware>();h->actual.settings=request().profile.requested;
    auto& geometry=h->actual.geometry;geometry.resolution={3,2};geometry.offsetEvidence=geometry.binningEvidence=geometry.decimationEvidence=geometry.reverseEvidence=GeometryEvidence::Reported;
    for(const auto& n:QStringList{"ExposureTime","Gain"}){CameraFeature f;f.name=n;f.type=FeatureType::Float;f.access=FeatureAccess::ReadWrite;f.value=n=="Gain"?1.:100.;f.minimum=n=="Gain"?0.:10.;f.maximum=n=="Gain"?20.:1000.;f.unit=n=="Gain"?"dB":"us";h->nodes[n]=f;}
    for(const auto& n:QStringList{"ExposureAuto","GainAuto"}){CameraFeature f;f.name=n;f.type=FeatureType::Enumeration;f.access=FeatureAccess::ReadWrite;f.value=QString("Off");f.entries={{"Off",0},{"Continuous",1}};h->nodes[n]=f;}
    auto factory=CameraProviderFactory::forTests(std::make_shared<UiProvider>(state,h));
    CameraManager view([&]{return &workspace;},[&]{return gate;},jobs,dir.path()+"/local.ini",nullptr,scanner,factory);
    view.resize(1400,900);view.show();
    auto* table=view.findChild<QTableWidget*>("cameraDevices");
    auto* status=view.findChild<QLabel*>("cameraStatus");
    auto scan=[&]{view.scan();waitUi([&]{return !jobs.busy();});};
    for(int n:{0,1,5,10}){count=n;scan();check(table->rowCount()==n,"0/1/5/10 synthetic inventory");
        view.findChild<QLabel*>("cameraTitle")->setText(QString("Sentetik UI fixture • %1 kamera").arg(n));
        view.grab().save(evidence+QString("/camera-%1-normal.png").arg(n));
        view.resize(850,600);QApplication::processEvents();view.grab().save(evidence+QString("/camera-%1-small.png").arg(n));view.resize(1400,900);
    }
    multiRoute=true;count=1;scan();auto* routes=view.findChild<QComboBox*>("cameraRoutes");check(routes->currentIndex()==-1 && routes->isVisible(),"unresolved local routes are visible in details");routes->setCurrentIndex(1);view.findChild<QPushButton*>("cameraRememberRoute")->click();scan();check(routes->currentIndex()==1,"exact remembered route auto resolves");multiRoute=false;count=10;scan();view.findChild<QGroupBox*>("cameraAdvanced")->setChecked(false);
    table->selectRow(9);view.resize(850,600);QApplication::processEvents();view.grab().save(evidence+"/camera-long-small.png");
    check(view.findChild<QScrollArea*>()->horizontalScrollBar()->maximum()==0,"small window wraps long identity without page overflow");
    view.resize(1400,900);table->selectRow(0);view.findChild<QLineEdit*>("cameraName")->setText("Synthetic test camera");
    view.findChild<QLineEdit*>("cameraShortId")->setText("CAM_MAIN");view.findChild<QPushButton*>("cameraRegister")->click();auto e=workspace.environment();
    check(e.current.cameras.size()==1,"register shared CameraId");const auto camera=e.current.cameras.first();
    check(e.current.setups.size()==1 && e.current.profiles.size()==1 && e.registrations.value(camera.id.value).shortId=="CAM_MAIN","atomic automatic defaults and alias");
    view.findChild<QPushButton*>("cameraRegister")->click();scan();check(workspace.environment().current.cameras.size()==1,"repeat scan/register does not duplicate");
    table->selectRow(9);const auto serial=table->item(9,2)->text();reversed=true;scan();check(table->item(table->currentRow(),2)->text()==serial,"rescan preserves physical selection after reorder");reversed=false;scan();table->selectRow(0);
    auto* alias=view.findChild<QLineEdit*>("cameraShortId");alias->setText(QString(32,'A'));view.findChild<QLineEdit*>("cameraName")->setText(QString(120,'N'));view.findChild<QPushButton*>("cameraRename")->click();
    check(workspace.environment().current.cameras.first().id==camera.id && workspace.environment().registrations.value(camera.id.value).shortId==QString(32,'A'),"rename preserves identity");
    view.findChild<QGroupBox*>("cameraAdvanced")->setChecked(true);
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
    view.findChild<QGroupBox*>("cameraAdvanced")->setChecked(false);view.findChild<QPushButton*>("cameraSettingsRead")->click();waitUi([&]{return !jobs.busy();});
    check(apply->isEnabled() && preview->isEnabled(),"test provider capabilities enable supported controls");
    view.findChild<QLineEdit*>("cameraExposure")->setText("110");apply->click();
    check(jobs.busy() && !apply->isEnabled() && !preview->isEnabled(),"busy blocks settings and new session");
    waitUi([&]{return !jobs.busy();});
    check(view.findChild<QLabel*>("cameraReadback")->text().contains("110"),"actual configured readback displayed");
    const auto before=workspace.environment().encode();const int writes=h->writes;
    view.findChild<QLineEdit*>("cameraExposure")->setText("1001");apply->click();waitUi([&]{return !jobs.busy();});check(workspace.environment().encode()==before && h->writes==writes && !apply->isEnabled(),"out of range fails without saving or claiming success");
    view.findChild<QPushButton*>("cameraSettingsRead")->click();waitUi([&]{return !jobs.busy();});
    view.findChild<QLineEdit*>("cameraExposure")->setText("bad");apply->click();check(!jobs.busy() && status->text().contains("Sayısal"),"invalid numeric error");
    preview->click();waitUi([&]{return status->text().contains("Önizleme •");});
    check(!view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"selected synthetic owned preview");
    view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(view.findChild<QScrollArea*>()->verticalScrollBar()->maximum());
    view.grab().save(evidence+"/camera-preview.png");
    view.stop();check(view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"Stop immediately clears stale preview");
    waitUi([&]{return !jobs.busy();});
    ambiguous=true;count=1;scan();check(!preview->isEnabled() && !view.findChild<QPushButton*>("cameraSettingsRead")->isEnabled(),"ambiguous physical identity cannot open");view.grab().save(evidence+"/camera-ambiguous.png");ambiguous=false;
    gate=false;view.refresh();check(!apply->isEnabled(),"external Runtime busy gate");gate=true;
    count=0;scan();check(table->rowCount()==1 && table->item(0,4)->text().contains("Çevrimdışı"),"registered camera remains offline");
    missing=true;scan();check(status->text().contains("SDK missing") && !preview->isEnabled(),"missing SDK error no preview");
    view.findChild<QLabel*>("cameraTitle")->setText("Sentetik UI fixture • SDK eksik / kayıtlı kamera");
    view.resize(850,600);view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(0);view.grab().save(evidence+"/camera-small-error.png");
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
    const auto expEvidence=evidence;QDir().mkpath(expEvidence);
    view.findChild<QCheckBox*>("cameraExperimental")->setChecked(true);
    view.findChild<QLabel*>("cameraTitle")->setText("Sentetik UI • deneysel mod / edinim yok");
    view.resize(1400,900);QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-normal.png");
    view.resize(850,600);QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-small.png");
    view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(view.findChild<QScrollArea*>()->verticalScrollBar()->maximum());QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-small-bottom.png");
    check(view.findChild<QLabel*>("cameraExperimentalNotice")->isVisible() && !view.findChild<QPushButton*>("cameraSaveFrame")->isEnabled(),"persistent experimental notice; no raster means no save");
    check(view.findChild<QScrollArea*>()->horizontalScrollBar()->maximum()==0,"experimental controls fit small window");
    view.findChild<QCheckBox*>("cameraExperimental")->setChecked(false);
    auto* scroll=view.findChild<QScrollArea*>();for(auto* action:{view.findChild<QPushButton*>("cameraScan"),view.findChild<QPushButton*>("cameraRename"),view.findChild<QPushButton*>("cameraSettingsRead"),preview,view.findChild<QPushButton*>("cameraStop")}){scroll->ensureWidgetVisible(action);QApplication::processEvents();check(scroll->viewport()->rect().contains(action->mapTo(scroll->viewport(),action->rect().center())),"small screen main action reachable by scrolling");}
    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());view.grab().save(evidence+"/camera-small-actions.png");
    {CameraSession emptyJobs;CameraManager empty([]()->MeasurementWorkspace*{return nullptr;},[]{return true;},emptyJobs,dir.path()+"/empty.ini",nullptr,scanner);
        bool guided=false;empty.onProject=[&]{guided=true;};empty.resize(850,600);empty.show();QApplication::processEvents();auto* projectAction=empty.findChild<QPushButton*>("cameraProject");check(empty.findChild<QScrollArea*>()->viewport()->rect().contains(projectAction->mapTo(empty.findChild<QScrollArea*>()->viewport(),projectAction->rect().center())),"project guidance visible without scrolling");projectAction->click();check(guided && !empty.findChild<QPushButton*>("cameraRegister")->isEnabled(),"single project guidance without recipe");empty.grab().save(evidence+"/camera-no-project.png");}
    {const auto path=dir.path()+"/legacy.vfproject";QFile file(path);check(file.open(QIODevice::WriteOnly),"legacy saved project");file.write("{}");file.close();auto r=request();auto p=r.profile;p.id={"profile-two"};p.name="Second profile";
        MeasurementEnvironmentRegistry registry(MeasurementEnvironmentRepository(path,"legacy"));registry.replace({{r.camera},{r.setup},{r.profile,p}});
        MeasurementWorkspace legacy(path,"legacy",[]{return true;});CameraSession legacyJobs;CameraManager legacyView([&]{return &legacy;},[]{return true;},legacyJobs,dir.path()+"/legacy.ini",nullptr,scanner,factory);legacyView.resize(1400,900);legacyView.show();legacyView.scan();waitUi([&]{return !legacyJobs.busy();});
        auto* profiles=legacyView.findChild<QComboBox*>("cameraProfiles");check(profiles->currentIndex()==-1 && profiles->isVisible() && !legacyView.findChild<QPushButton*>("cameraSettingsRead")->isEnabled(),"legacy ambiguity requires explicit visible selection");legacyView.grab().save(evidence+"/camera-legacy-ambiguous.png");
        legacyView.findChild<QLineEdit*>("cameraShortId")->setText("LEGACY");profiles->setCurrentIndex(1);legacyView.findChild<QPushButton*>("cameraDefaultsSave")->click();check(legacy.environment().defaults(r.camera.id).profile==p.id && !profiles->isVisible(),"explicit legacy defaults persist then hide chooser");}
    QTextStream(stdout)<<"PASS CCUX-03: synthetic 0/1/5/10, shared CameraId, readback/preview, busy/Stop, offline/error, normal factory isolation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
