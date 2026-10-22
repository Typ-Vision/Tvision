#include "app/CameraManager.hpp"
#include "app/CalibrationOverview.hpp"
#include "CameraSettingsFixture.hpp"
#include "OecsExperimentalFixture.hpp"
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
    const auto evidence=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS",QDir::currentPath()+"/../ccux/ccux-05")+"/camera";QDir().mkpath(evidence);
    if(app.arguments().contains("--physical-settings")){
        const auto path=dir.filePath("physical.vfproject");QFile file(path);check(file.open(QIODevice::WriteOnly),"physical temporary project");file.write("{}");file.close();
        CameraSession jobs;MeasurementWorkspace ws(path,"physical-settings",[&]{return !jobs.busy();});
        CameraManager view([&]{return &ws;},[]{return true;},jobs,dir.filePath("physical.ini"));view.resize(1400,900);view.show();view.scan();waitUi([&]{return !jobs.busy();});
        auto* table=view.findChild<QTableWidget*>("cameraDevices");int selected=-1;
        for(int i=0;i<table->rowCount();++i)if(table->item(i,2)->text()=="02A2220000627")selected=i;
        check(selected>=0,"physical Opto row discovered");table->selectRow(selected);
        auto* add=view.findChild<QPushButton*>("cameraRegister");check(add->isEnabled(),"physical Ekle enabled");add->click();
        auto* read=view.findChild<QPushButton*>("cameraSettingsRead");check(read->isEnabled(),"physical settings read enabled without native acquisition");read->click();waitUi([&]{return !jobs.busy();});
        auto* apply=view.findChild<QPushButton*>("cameraApply");check(apply->isEnabled(),qPrintable(view.findChild<QLabel*>("cameraStatus")->text()));
        apply->click();waitUi([&]{return !jobs.busy();});check(view.findChild<QLabel*>("cameraStatus")->text().contains("doğrulandı ve kaydedildi"),qPrintable(view.findChild<QLabel*>("cameraStatus")->text()));
        for(const auto size:{QSize(1400,900),QSize(850,600)}){view.resize(size);QApplication::processEvents();view.findChild<QScrollArea*>()->ensureWidgetVisible(read);QApplication::processEvents();check(view.grab().save(evidence+QString("/physical-settings-%1.png").arg(size.width())),"physical settings screenshot");}
        QTextStream(stdout)<<"PASS physical Camera Manager: scan -> select -> Ekle -> read -> current-value apply/save; temporary project only; "<<view.findChild<QLabel*>("cameraReadback")->text()<<'\n';return 0;
    }
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
    preview->click();waitUi([&]{return !jobs.busy();});
    check(status->text().contains("Tek görüntü") && status->text().contains("1 kare") && status->text().contains("0.0 FPS"),"single frame stops cleanly with measured stats");
    check(!view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"selected synthetic owned preview");
    view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(view.findChild<QScrollArea*>()->verticalScrollBar()->maximum());
    view.grab().save(evidence+"/camera-preview.png");
    view.stop();check(view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"Stop immediately clears stale preview");
    waitUi([&]{return !jobs.busy();});
    auto* live=view.findChild<QPushButton*>("cameraLive");
    auto* image=view.findChild<QLabel*>("cameraPreviewImage");
    auto* save=view.findChild<QPushButton*>("cameraSaveFrame");
    for(int cycle=0;cycle<2;++cycle){
        live->click();waitUi([&]{return status->text().contains("Canlı") && !image->pixmap().isNull();});
        const auto first=status->text();waitUi([&]{return status->text()!=first;});
        check(jobs.busy() && !table->isEnabled() && !apply->isEnabled() && !preview->isEnabled() && !save->isEnabled(),"live exclusive controls and no native evidence save");
        check(status->text().contains("Sıralı tek kare") && !status->text().contains("0.0 FPS"),"live sequential label and real FPS");
        for(const auto size:{QSize(1400,900),QSize(850,600)}){
            view.resize(size);QApplication::processEvents();auto* sc=view.findChild<QScrollArea*>();sc->ensureWidgetVisible(image);
            view.grab().save(evidence+QString("/live-%1-%2.png").arg(cycle).arg(size.width()));
            check(sc->horizontalScrollBar()->maximum()==0,"live small layout no horizontal overflow");
            for(auto* action:{live,view.findChild<QPushButton*>("cameraStop"),preview}){sc->ensureWidgetVisible(action);QApplication::processEvents();check(sc->viewport()->rect().contains(action->mapTo(sc->viewport(),action->rect().center())),"live actions reachable");}
        }
        view.stop();check(image->pixmap().isNull() && !save->isEnabled(),"Stop retires visible frame and evidence action");waitUi([&]{return !jobs.busy();});
    }
    view.resize(1400,900);
    state->continuous=true;preview->click();waitUi([&]{return !jobs.busy();});check(status->text().contains("Continuous"),"native continuous capability labeled separately");state->continuous=false;view.stop();
    // An independently owned physical lease is rejected with a visible reason.
    {auto rival=factory.open(request(),CameraLeaseUse::Calibration,FrameQueueMode::ProductionFifo);
        preview->click();waitUi([&]{return !jobs.busy();});
        check(image->pixmap().isNull() && status->text().contains("Görüntü alınamadı"),"busy camera clears old raster and reports failure");}
    // Transport failure after a visible frame must clear it and release the session.
    std::atomic_int frames=0;
    state->hardware->hook=[&](const QString& event){if(event=="preview" && ++frames>2)throw std::runtime_error("Synthetic transport disconnected");};
    live->click();waitUi([&]{return !jobs.busy();});
    check(image->pixmap().isNull() && status->text().contains("disconnected") && !save->isEnabled(),"disconnect clears old frame and preserves reason");
    state->hardware->hook={};
    // Programmatic selection is guarded as well as disabled user controls.
    live->click();waitUi([&]{return !image->pixmap().isNull();});table->selectRow(1);
    check(image->pixmap().isNull(),"camera change immediately retires old frame");waitUi([&]{return !jobs.busy();});table->selectRow(0);
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
    live->click();waitUi([&]{return entered.load();});
    bool transitioned=false;
    view.leave([&]{
        check(!jobs.busy() && image->pixmap().isNull(),"page/project/close continuation only after joined worker");
        auto calibrationLease=factory.open(request(),CameraLeaseUse::Calibration,FrameQueueMode::ProductionFifo);
        transitioned=true;
    });
    QTest::qWait(50);check(!transitioned,"pending callback keeps page transition deferred while UI processes events");
    check(jobs.busy() && view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"blocked preview retains worker ownership after Stop");
    release.set_value();waitUi([&]{return transitioned;});
    check(view.findChild<QLabel*>("cameraPreviewImage")->pixmap().isNull(),"late preview suppressed after joined shutdown");
    state->hardware->hook={};
    // Window owner can safely destroy the widget from the drained continuation.
    {CameraSession closingJobs;auto* closingView=new CameraManager([&]{return &workspace;},[]{return true;},closingJobs,dir.path()+"/closing.ini",nullptr,scanner,factory);
        closingView->scan();waitUi([&]{return !closingJobs.busy();});
        std::promise<void> unblock;auto ready=unblock.get_future().share();std::atomic_bool pending=false;bool closed=false;
        state->hardware->hook=[&](const QString& event){if(event=="preview"){pending=true;ready.wait();}};
        closingView->findChild<QPushButton*>("cameraLive")->click();waitUi([&]{return pending.load();});
        closingView->leave([&]{closed=true;closingView->deleteLater();});check(!closed && closingJobs.busy(),"close waits pending callback");
        unblock.set_value();waitUi([&]{return closed;});QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);check(!closingJobs.busy(),"close drains before destruction");state->hardware->hook={};
    }
    {CameraSession nativeJobs;CameraManager native([&]{return &workspace;},[]{return true;},nativeJobs,dir.path()+"/native.ini",nullptr,scanner);
     native.scan();waitUi([&]{return !nativeJobs.busy();});check(!native.findChild<QPushButton*>("cameraPreview")->isEnabled() && !native.findChild<QPushButton*>("cameraApply")->isEnabled(),"normal composition rejects synthetic native capability");}
    const auto expEvidence=evidence;QDir().mkpath(expEvidence);
    check(!view.findChild<QCheckBox*>("cameraExperimental"),"camera has no acquisition opt-in");
    view.findChild<QLabel*>("cameraTitle")->setText("Sentetik UI • edinim yok");
    view.resize(1400,900);QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-normal.png");
    view.resize(850,600);QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-small.png");
    view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(view.findChild<QScrollArea*>()->verticalScrollBar()->maximum());QApplication::processEvents();view.grab().save(expEvidence+"/camera-experimental-small-bottom.png");
    check(!view.findChild<QLabel*>("cameraExperimentalNotice") && !view.findChild<QPushButton*>("cameraSaveFrame")->isEnabled(),"persistent experimental notice; no raster means no save");
    check(view.findChild<QScrollArea*>()->horizontalScrollBar()->maximum()==0,"experimental controls fit small window");

    auto* scroll=view.findChild<QScrollArea*>();for(auto* action:{view.findChild<QPushButton*>("cameraScan"),view.findChild<QPushButton*>("cameraRename"),view.findChild<QPushButton*>("cameraSettingsRead"),preview,live,view.findChild<QPushButton*>("cameraStop")}){scroll->ensureWidgetVisible(action);QApplication::processEvents();check(scroll->viewport()->rect().contains(action->mapTo(scroll->viewport(),action->rect().center())),"small screen main action reachable by scrolling");}
    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());view.grab().save(evidence+"/camera-small-actions.png");
    {CameraSession emptyJobs;CameraManager empty([]()->MeasurementWorkspace*{return nullptr;},[]{return true;},emptyJobs,dir.path()+"/empty.ini",nullptr,scanner);
        bool guided=false;empty.onProject=[&]{guided=true;};empty.resize(850,600);empty.show();QApplication::processEvents();auto* projectAction=empty.findChild<QPushButton*>("cameraProject");check(empty.findChild<QScrollArea*>()->viewport()->rect().contains(projectAction->mapTo(empty.findChild<QScrollArea*>()->viewport(),projectAction->rect().center())),"project guidance visible without scrolling");projectAction->click();check(guided && !empty.findChild<QPushButton*>("cameraRegister")->isEnabled(),"single project guidance without recipe");empty.grab().save(evidence+"/camera-no-project.png");}
    {const auto path=dir.path()+"/legacy.vfproject";QFile file(path);check(file.open(QIODevice::WriteOnly),"legacy saved project");file.write("{}");file.close();auto r=request();auto p=r.profile;p.id={"profile-two"};p.name="Second profile";
        MeasurementEnvironmentRegistry registry(MeasurementEnvironmentRepository(path,"legacy"));registry.replace({{r.camera},{r.setup},{r.profile,p}});
        MeasurementWorkspace legacy(path,"legacy",[]{return true;});CameraSession legacyJobs;CameraManager legacyView([&]{return &legacy;},[]{return true;},legacyJobs,dir.path()+"/legacy.ini",nullptr,scanner,factory);legacyView.resize(1400,900);legacyView.show();legacyView.scan();waitUi([&]{return !legacyJobs.busy();});
        auto* profiles=legacyView.findChild<QComboBox*>("cameraProfiles");check(profiles->currentIndex()==-1 && profiles->isVisible() && !legacyView.findChild<QPushButton*>("cameraSettingsRead")->isEnabled(),"legacy ambiguity requires explicit visible selection");legacyView.grab().save(evidence+"/camera-legacy-ambiguous.png");
        legacyView.findChild<QLineEdit*>("cameraShortId")->setText("LEGACY");profiles->setCurrentIndex(1);legacyView.findChild<QPushButton*>("cameraDefaultsSave")->click();check(legacy.environment().defaults(r.camera.id).profile==p.id && !profiles->isVisible(),"explicit legacy defaults persist then hide chooser");}
    // Real experimental API contract through the widget, without a physical SDK.
    {namespace oe=oecs_experimental_fixture;
        const auto path=dir.path()+"/product-settings.vfproject";QFile file(path);check(file.open(QIODevice::WriteOnly),"settings project");file.write("{}");file.close();
        MeasurementWorkspace ws(path,"product-settings",[]{return true;});CameraSession settingJobs;auto hardware=oe::state();hardware->dynamicAuto=true;
        hardware->nodes["TriggerMode"].value=QString("On");hardware->nodes["TriggerSource"].value=QString("Software");
        auto scanner=[&](const QStringList&,const std::shared_ptr<std::atomic_bool>&){return CameraService::combine({},{{oe::request().device},{},false});};
        CameraManager product([&]{return &ws;},[]{return true;},settingJobs,dir.filePath("product-settings.ini"),nullptr,scanner,{},[hardware]{return std::make_unique<oe::Api>(hardware);});
        QFile runtime(dir.filePath("settings-api.dll"));check(runtime.open(QIODevice::WriteOnly),"settings runtime marker");runtime.write("fixture");runtime.close();
        product.findChild<QLineEdit*>("cameraExperimentalRuntime")->setText(runtime.fileName());product.resize(1400,900);product.show();product.scan();waitUi([&]{return !settingJobs.busy();});
        auto* add=product.findChild<QPushButton*>("cameraRegister");auto* read=product.findChild<QPushButton*>("cameraSettingsRead");auto* apply=product.findChild<QPushButton*>("cameraApply");
        check(add->isEnabled() && !read->isEnabled(),"unregistered product camera can add without native admission");add->click();
        check(ws.environment().current.cameras.size()==1 && add->text()=="Kamera kayıtlı" && !add->isEnabled() && read->isEnabled(),"pending registered camera can read real product settings");
        check(!product.findChild<QPushButton*>("cameraPreview")->isEnabled(),"native preview stays closed");
        read->click();waitUi([&]{return !settingJobs.busy();});
        auto* exposure=product.findChild<QLineEdit*>("cameraExposure");check(exposure->isEnabled() && exposure->text()=="5000" && apply->isEnabled(),"product scalar values enabled from SDK readback");
        check(!product.findChild<QCheckBox*>("cameraExperimental"),"product has no acquisition opt-in");check(read->isEnabled() && apply->isEnabled(),"experimental opt-in does not disable settings");
        exposure->setText("6000");apply->click();waitUi([&]{return !settingJobs.busy();});
        check(ws.environment().current.profiles.first().requested.exposureTimeUs==6000 && ws.environment().current.setups.first().pending,"real settings persisted while physical optics remain pending");
        check(hardware->calls.isEmpty() && hardware->nodes["TriggerMode"].value==FeatureValue(QString("On")),"settings do not acquire or retain altered trigger mode");
        product.findChild<QGroupBox*>("cameraAdvanced")->setChecked(true);
        QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());check(dialog,"pending optical draft");
            check(dialog->findChild<QSpinBox*>("draftWidth")->value()==2448 && dialog->findChild<QSpinBox*>("draftHeight")->value()==2048 && dialog->findChild<QLineEdit*>("draftExposure")->text()=="6000","pending draft uses actual settings geometry");
            check(dialog->findChild<QLineEdit*>("draftLens")->text().isEmpty() && dialog->findChild<QLineEdit*>("draftPlane")->text().isEmpty(),"physical optics never invented");dialog->reject();});
        product.findChild<QPushButton*>("cameraProfileEdit")->click();
        product.findChild<QGroupBox*>("cameraAdvanced")->setChecked(false);
        product.findChild<QLabel*>("cameraTitle")->setText("Sentetik Opto SDK • ürün ayar yolu");
        for(const auto size:{QSize(1400,900),QSize(850,600)}){product.resize(size);QApplication::processEvents();product.findChild<QScrollArea*>()->ensureWidgetVisible(read);QApplication::processEvents();product.grab().save(evidence+QString("/product-settings-%1.png").arg(size.width()));}
    }
    {namespace oe=oecs_experimental_fixture;
        const auto r=oe::request();const auto path=dir.path()+"/experimental.vfproject";
        QFile projectFile(path);check(projectFile.open(QIODevice::WriteOnly),"experimental project");projectFile.write("{}");projectFile.close();
        MeasurementEnvironmentRegistry registry(MeasurementEnvironmentRepository(path,"experimental-ui"));registry.replace({{r.camera},{r.setup},{r.profile}});
        MeasurementWorkspace expWorkspace(path,"experimental-ui",[]{return true;});CameraSession expJobs;auto hardware=oe::state();
        auto expScanner=[r](const QStringList&,const std::shared_ptr<std::atomic_bool>&){IndustrialDiscoverySnapshot snapshot;snapshot.devices={r.device};return CameraService::combine({},snapshot);};
        QString selectedPath;bool stopDuringSave=false;
        CameraManager expView([&]{return &expWorkspace;},[]{return true;},expJobs,dir.path()+"/exp.ini",nullptr,expScanner,{},[hardware]{return std::make_unique<oe::Api>(hardware);},[&](QWidget* parent){
            QTest::qWait(80);if(stopDuringSave)static_cast<CameraManager*>(parent)->stop();QTest::qWait(80);return selectedPath;
        });
        expView.resize(1400,900);expView.show();expView.scan();waitUi([&]{return !expJobs.busy();});
        QFile dll(dir.path()+"/fixture.dll");check(dll.open(QIODevice::WriteOnly),"fixture DLL marker");dll.write("Synthetic API only");dll.close();
        expView.findChild<QLineEdit*>("cameraExperimentalRuntime")->setText(dll.fileName());check(!expView.findChild<QCheckBox*>("cameraExperimental"),"new session has no experimental opt-in");
        expView.findChild<QSpinBox*>("cameraTimeout")->setValue(3000); // This SDK fixture asserts a 3 s stream budget.
        auto* expSave=expView.findChild<QPushButton*>("cameraSaveFrame");auto* expImage=expView.findChild<QLabel*>("cameraPreviewImage");
        expView.findChild<QPushButton*>("cameraPreview")->click();waitUi([&]{return !expJobs.busy();});
        check(!expImage->pixmap().isNull() && expSave->isEnabled(),qPrintable("single experimental visible frame can save after clean shutdown: "+expView.findChild<QLabel*>("cameraStatus")->text()));
        const auto saved=evidence+"/visible-experimental.pgm";QFile::remove(saved);QFile::remove(saved+".json");
        auto choose=[&](const QString& file,bool stop){selectedPath=file;stopDuringSave=stop;};
        choose(saved,false);expSave->click();check(QFileInfo::exists(saved) && QFileInfo::exists(saved+".json"),"visible PGM and sidecar saved");
        QFile sidecar(saved+".json");check(sidecar.open(QIODevice::ReadOnly),"saved sidecar");const auto metadata=QJsonDocument::fromJson(sidecar.readAll()).object();
        check(metadata["productionAdmission"]==false && metadata["triggerAssociationVerified"]==false && metadata["profileSnapshot"].toObject()==r.profile.toJson(),"experimental provenance retained");
        expView.findChild<QPushButton*>("cameraLive")->click();waitUi([&]{return !expImage->pixmap().isNull() && expSave->isEnabled();});
        for(const auto size:{QSize(1400,900),QSize(850,600)}){expView.resize(size);QApplication::processEvents();expView.findChild<QScrollArea*>()->ensureWidgetVisible(expImage);expView.grab().save(evidence+QString("/experimental-live-%1.png").arg(size.width()));}
        const auto stopped=evidence+"/stopped-frame.pgm";QFile::remove(stopped);QFile::remove(stopped+".json");
        choose(stopped,true);expSave->click();waitUi([&]{return !expJobs.busy();});
        check(!QFileInfo::exists(stopped) && !QFileInfo::exists(stopped+".json") && expImage->pixmap().isNull() && !expSave->isEnabled(),"Stop while save dialog open cannot write retired frame");
        expView.findChild<QPushButton*>("cameraLive")->click();waitUi([&]{return expSave->isEnabled();});
        const auto liveSaved=evidence+"/visible-live.pgm";QFile::remove(liveSaved);QFile::remove(liveSaved+".json");choose(liveSaved,false);expSave->click();
        check(QFileInfo::exists(liveSaved),"save dialog holds visible live raster while acquisition continues");expView.stop();waitUi([&]{return !expJobs.busy();});
    }
    QTextStream(stdout)<<"PASS CCUX-05: single/live/restart, measured FPS, busy lease, disconnect, camera change, deferred transition/close, late frame, normal/small UI; CCUX-03 regression\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
