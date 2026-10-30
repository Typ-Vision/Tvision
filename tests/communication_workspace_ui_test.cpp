#include "app/CommunicationPanel.hpp"
#include "application/ProjectService.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "persistence/StationActivationStore.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QImage>
#include <QFontDatabase>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QKeyEvent>
#include <QJsonDocument>
#include <QTextStream>
#include <QTcpSocket>
#include <QElapsedTimer>
using namespace visionflow;
int main(int argc,char** argv){QApplication app(argc,argv);app.setStyle("Fusion");QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");try{
    QFile theme(":/resources/theme.qss");if(theme.open(QIODevice::ReadOnly))app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    int checks=0;auto check=[&](bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);};
    auto until=[&](auto f){QElapsedTimer t;t.start();while(!f() && t.elapsed()<5000)QApplication::processEvents();check(f(),"Bounded fixture completion");};
    const auto artifacts=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS","D:/TVision/build/comm4/COMM4-16/ui");QDir().mkpath(artifacts);QTemporaryDir root(QDir(artifacts).filePath("workspace-XXXXXX"));check(root.isValid(),"Temporary fixture");
    JsonStationIdentityStore identities(root.filePath("identities.json"));StationIdentityRegistry registry(identities);registry.initialize(QUuid::createUuid());
    CommunicationServerProfiles profiles(root.filePath("server.json"));CommunicationServerProfile profile;profile.enabled=true;profile.port=53316; // bind a known exact profile; no fallback
    // Ask the OS for a free port for this isolated software fixture only.
    {QTcpServer probe;check(probe.listen(QHostAddress::LocalHost,0),"Fixture localhost");profile.port=probe.serverPort();}profiles.save(profile);
    StationRuntimeBoundary boundary;std::vector<std::unique_ptr<RuntimeService>> runtimes;std::vector<std::unique_ptr<RuntimeCoordinator>> coordinators;std::vector<std::unique_ptr<StationRunController>> runs;
    FileStationActivationStore selectionStore(root.filePath("stations.json"));StationActivationService activations(selectionStore,identities,boundary);activations.initialize();
    StationDeliveryLedger ledger(root.filePath("production"),identities.load().installation);StationDeliveryLedger::Control durableControl(ledger);
    QMap<quint32,StationSavedRecipe> saved;
    StationControlService controls(activations,identities,&durableControl,[&](quint32 key)->std::optional<StationSavedRecipe>{if(!saved.contains(key))return {};return saved[key];});
    CommunicationPeerSessionService sessions(identities);sessions.setDeliveryLedger(&ledger);
    CommunicationServerLifecycle server(profiles,root.filePath("shutdown.json"),&sessions);check(server.state()==CommunicationServerState::Listening,"Real process listener");
    CommunicationWorkspace workspace(server,profiles,identities);workspace.setLedger(&ledger);
    QVector<QUuid> ids;QVector<std::shared_ptr<const StationSnapshot>> snapshots;
    for(int n=0;n<3;++n){
        const auto id=QUuid::createUuid();ids.append(id);const auto entry=registry.bind(identities.load().revision,{CatalogueCategory::Station,id,{},{ }},QString("Station %1").arg(n+1));
        ProjectService p;p.createProject("Workspace","",root.filePath(QString("p%1.vfproject").arg(n)),"file");p.createRecipe(QString("Recipe %1").arg(n+1),"",ApplicationType(n),"file");
        QImage image(32,32,QImage::Format_Grayscale8);image.fill(255);const auto imagePath=root.filePath(QString("camera%1.png").arg(n));check(image.save(imagePath),"File source fixture");const auto source=p.setImageFile(imagePath);p.save();
        const auto camera=QUuid::createUuid();const auto cameraEntry=registry.bind(identities.load().revision,{CatalogueCategory::Camera,camera,{},{}},"Fixture camera");
        const auto revision=QUuid::createUuid();const auto recipeEntry=registry.bind(identities.load().revision,{CatalogueCategory::RecipeRevision,QUuid(p.activeRecipe()->id),revision,{}},p.activeRecipe()->name);
        const auto templateEntry=registry.bind(identities.load().revision,{CatalogueCategory::TemplateRevision,QUuid::createUuid(),QUuid::createUuid(),{}},"Template");
        runtimes.push_back(std::make_unique<RuntimeService>(root.filePath(QString("r%1").arg(n)),root.filePath("calibrations.json"),RuntimeIdentity::localEngineer()));coordinators.push_back(std::make_unique<RuntimeCoordinator>(*runtimes.back()));boundary.bind(id,*runtimes.back(),*coordinators.back());runs.push_back(std::make_unique<StationRunController>(boundary,id));server.addStation(*runs.back());
        StationRecipeSelection choice{revision,recipeEntry.key,*p.activeRecipe(),root.path(),{{source,camera,cameraEntry.key}}};saved[recipeEntry.key]={choice,templateEntry.key};
        auto snapshot=activations.activate({id,entry.key,entry.name},{},choice);snapshots.append(snapshot);controls.bind(snapshot->station,*runs.back());
        workspace.bind({snapshot->station,runs.back().get(),[&,id]{return activations.snapshot(id);},[&,id](bool on){controls.setAuto(id,activations.snapshot(id)->activation,on);controls.poll();},"Template #"+QString::number(templateEntry.key),{}});
    }
    CommunicationPanel panel;panel.setWindowTitle("VisionFlow • COMM4-16 software fixture");panel.bind(&workspace);panel.legacyHistory=[] {return QString("Legacy read-only fixture — no parser / mutation");};panel.show();
    auto* table=panel.findChild<QTableWidget*>("communicationStations");auto* listener=panel.findChild<QLabel*>("communicationListener");auto* detail=panel.findChild<QLabel*>("communicationStationDetail");
    auto button=[&](const char* name){panel.findChild<QPushButton*>(name)->click();};
    check(table->rowCount()==3,"All three stations visible concurrently");
    QTcpSocket peer;peer.connectToHost(QHostAddress::LocalHost,profile.port);until([&]{return peer.state()==QAbstractSocket::ConnectedState && server.peer();});panel.refresh();
    check(listener->text().contains("CONNECTED") && table->item(0,1)->text()=="STOPPED" && table->item(0,2)->text()=="OFF","Connected stopped never implies production readiness");
    const auto generation=server.listenerGeneration();
    for(int n=0;n<3;++n){table->selectRow(n);check(server.port()==profile.port && server.listenerGeneration()==generation,"Selection cannot change listener or port");check(detail->text().contains(snapshots[n]->activation.toString(QUuid::WithoutBraces)),"Exact activation detail");}
    table->selectRow(0);button("stationRun");until([&]{workspace.poll();return runs[0]->state().mode==StationMode::ArmedManual;});panel.refresh();check(!runs[1]->automatic() && runs[1]->state().mode==StationMode::Stopped,"Independent Run");
    button("stationAuto");controls.poll();panel.refresh();check(runs[0]->automatic(),"Auto uses shared station control port");button("stationStop");until([&]{workspace.poll();return runs[0]->drained();});panel.refresh();check(!runs[0]->automatic(),"Stop fences Auto and drains");
    bool stale=false;try{workspace.command(ids[0],QUuid::createUuid(),"run");}catch(...){stale=true;}check(stale,"Stale displayed activation cannot Run");
    // A real durable unresolved control intent makes pending visible and blocks
    // Run/Auto. Stop stays available. No artificial GUI pending flag is used.
    auto cat=identities.load();const auto peerId=QUuid::createUuid(),epoch=QUuid::createUuid();registry.enrollEpoch(cat.revision,peerId,epoch);
    CommunicationV4Frame request;request.header={6,snapshots[1]->station.key,1,QUuid::createUuid(),peerId,epoch,snapshots[1]->selection.recipeKey,saved[snapshots[1]->selection.recipeKey].templateKey,snapshots[1]->activation};request.body=QByteArray(384,0);
    auto accepted=CommunicationV4Frame{request.header,QByteArray(896,0)};accepted.header.kind=110;CommunicationV4Codec::putUInt32(accepted.body,100,1);CommunicationV4Codec::putUInt32(accepted.body,824,6);
    V4RequestIdentity key{cat.installation,peerId,epoch,ids[1],1};check(ledger.reserve({key,{request,accepted,CommunicationV4Codec::requestFingerprint(CommunicationV4Codec::encode(request,V4Direction::Request)),false},true})==0,"Durable pending fixture");
    panel.refresh();table->selectRow(1);check(table->item(1,5)->text()=="PENDING ACK" && !panel.findChild<QPushButton*>("stationRun")->isEnabled() && panel.findChild<QPushButton*>("stationStop")->isEnabled(),"Pending distinct, Run blocked, Stop available");
    button("communicationHistoryRefresh");check(panel.findChild<QPlainTextEdit*>("communicationHistory")->toPlainText().contains("BEGIN"),"Read-only real ledger evidence");
    table->selectRow(2);button("stationRun");until([&]{workspace.poll();return runs[2]->state().mode==StationMode::ArmedManual;});button("stationAuto");controls.poll();
    auto heldCamera=CameraLeaseArbiter::reserve("Runtime Station 3 fixture",CameraLeaseUse::Runtime,{{snapshots[2]->selection.cameras.first().camera.toString(QUuid::WithoutBraces),{"Fixture","Camera","COMM4-16",DeviceIdentityConfidence::SerialReported}}});
    panel.refresh();check(table->item(2,4)->text().contains("Runtime Station 3 fixture"),"Real arbiter camera owner visible");check(table->item(2,1)->text()=="ARMED_AUTO" && table->item(0,1)->text()=="STOPPED" && table->item(1,5)->text()=="PENDING ACK","Simultaneous independent station states");table->selectRow(1);
    auto* input=panel.findChild<QLineEdit*>("communicationMigrationRequest");input->setText(root.filePath("missing-request.json"));button("migration_analyze");check(panel.findChild<QLabel*>("communicationMessage")->text().contains("okunamadı"),"Migration read error accessible");button("migration_activate");check(panel.findChild<QLabel*>("communicationMessage")->text().contains("analiz"),"Apply requires reviewed analysis");
    const auto sizes=QApplication::platformName()=="offscreen"?QVector<QSize>{{1920,1080},{1280,720}}:QVector<QSize>{{1400,900},{850,600}};
    for(auto size:sizes){
        panel.resize(size);QApplication::processEvents();check(panel.size()==size,"Viewport fits");check(table->height()>=180 && table->rowCount()==3,"Three stations retained at small viewport");
        QTextStream(stdout)<<"viewport="<<panel.width()<<"x"<<panel.height()<<" platform="<<QApplication::platformName()<<" DPR="<<panel.devicePixelRatioF()<<"\n";
        check(panel.grab().save(QDir(artifacts).filePath(QString("communication-%1x%2.png").arg(size.width()).arg(size.height()))),"Screenshot");
    }
    auto* scroll=panel.findChild<QScrollArea*>("communicationScroll");
    if(app.arguments().contains("--preview")){scroll->verticalScrollBar()->setValue(0);panel.showNormal();panel.raise();panel.activateWindow();QTextStream(stdout)<<"PREVIEW "<<checks<<" checks; native keyboard/focus capture pending\n";return app.exec();}
    scroll->ensureWidgetVisible(input);input->setFocus();QApplication::processEvents();check(input->hasFocus(),"Focus reaches migration through scroll");QKeyEvent tab(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);QApplication::sendEvent(input,&tab);check(QApplication::focusWidget()!=input,"Keyboard Tab traverses controls");check(scroll->verticalScrollBar()->maximum()>0,"Small viewport vertical scrolling");
    check(panel.grab().save(QDir(artifacts).filePath("communication-small-migration-focus.png")),"Scroll/focus screenshot");
    check(server.listenerGeneration()==generation && server.port()==profile.port,"All UI operations preserve listener");
    QTextStream(stdout)<<"PASS "<<checks<<" workspace UI checks; platform="<<QApplication::platformName()<<" DPR="<<panel.devicePixelRatioF()<<"\n";
    server.beginShutdown(1000);until([&]{return server.pollShutdown();});return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
