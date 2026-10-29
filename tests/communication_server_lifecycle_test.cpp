#include "application/CommunicationServerLifecycle.hpp"
#include "application/ProjectService.hpp"
#include "application/PlcRuntimeSocket.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextStream>
#include <future>
#include <vector>
using namespace visionflow;
namespace {
int assertions=0;
void check(bool value,const char* why){++assertions;if(!value)throw std::runtime_error(why);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Expected rejection");}
QByteArray read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"Read fixture");return f.readAll();}
template<class F> void until(F ready){QElapsedTimer t;t.start();while(!ready() && t.elapsed()<2000)QCoreApplication::processEvents(QEventLoop::AllEvents,5);check(ready(),"Async timeout");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    QTemporaryDir root(app.applicationDirPath()+"/server-XXXXXX");check(root.isValid(),"Fixture root");
    const auto profilePath=root.filePath("v4.json"),checkpoint=root.filePath("checkpoint.json");CommunicationServerProfiles profiles(profilePath);
    {CommunicationServerLifecycle missing(profiles,checkpoint);check(missing.state()==CommunicationServerState::NotConfigured && !QFile::exists(profilePath),"Missing config stays missing");}
    CommunicationServerProfile p;profiles.save(p);const auto disabledBytes=read(profilePath);
    {CommunicationServerLifecycle disabled(profiles,checkpoint);check(disabled.state()==CommunicationServerState::Disabled && disabled.port()==0,"Disabled has no listener");}
    p.port=0;rejects([&]{profiles.save(p);});check(read(profilePath)==disabledBytes,"Invalid save preserves exact config");
    p.port=5000;p.maxStations=33;rejects([&]{profiles.save(p);});p.maxStations=32;
    // Obtain a free local port; production profile deliberately rejects ephemeral port zero.
    QTcpServer reservation;check(reservation.listen(QHostAddress::LocalHost,0),"Port fixture");p.port=reservation.serverPort();reservation.close();p.enabled=true;profiles.save(p);
    const auto enabledBytes=read(profilePath);
    {QTcpServer occupied;check(occupied.listen(QHostAddress::LocalHost,p.port),"Occupied port fixture");CommunicationServerLifecycle conflict(profiles,checkpoint);check(conflict.state()==CommunicationServerState::Fault && !conflict.error().isEmpty() && conflict.port()==0,"OS bind conflict, no fallback");}
    {PlcRuntimeSocket legacy;PlcSocketOptions legacyOptions;legacyOptions.port=0;legacy.listen(legacyOptions);CommunicationServerLifecycle conflict(profiles,checkpoint);check(conflict.state()==CommunicationServerState::Fault && legacy.listening(),"Existing legacy listener prevents v4, old listener unchanged");legacy.stop();}
    CommunicationServerLifecycle server(profiles,checkpoint);
    check(server.state()==CommunicationServerState::Listening && server.port()==p.port,"Startup listens exact configured port");
    const auto generation=server.listenerGeneration();check(!generation.isNull(),"Listener generation");
    {PlcRuntimeSocket legacy;PlcSocketOptions legacyOptions;legacyOptions.port=0;rejects([&]{legacy.listen(legacyOptions);});check(!legacy.listening(),"V4 selected prevents legacy second listener");}
    {CommunicationServerLifecycle conflict(profiles,checkpoint);check(conflict.state()==CommunicationServerState::Fault && !conflict.error().isEmpty() && conflict.port()==0,"Bind conflict explicit without fallback");}
    QTcpSocket client;client.connectToHost(QHostAddress::LocalHost,p.port);until([&]{return server.peer()!=nullptr && client.state()==QAbstractSocket::ConnectedState;});auto* originalPeer=server.peer();
    check(server.state()==CommunicationServerState::PeerConnected,"Connected control channel before Run");
    QTcpSocket extra;bool extraConnected=false;QObject::connect(&extra,&QTcpSocket::connected,&extra,[&]{extraConnected=true;});extra.connectToHost(QHostAddress::LocalHost,p.port);until([&]{return extraConnected && extra.state()==QAbstractSocket::UnconnectedState;});check(server.peer()==originalPeer,"Second peer cannot replace first");
    StationRuntimeBoundary boundary;
    std::vector<std::unique_ptr<RuntimeService>> runtimes;std::vector<std::unique_ptr<RuntimeCoordinator>> coordinators;std::vector<std::unique_ptr<StationRunController>> controllers;
    QVector<std::shared_ptr<const StationSnapshot>> snapshots;
    const RuntimeIdentity engineer{"engineer",[](const QString& name,ProductionAction){return name=="engineer";}};
    for(int n=0;n<3;++n){
        ProjectService project;project.createProject("Station","",root.filePath(QString("project%1.vfproject").arg(n)),"file");project.createRecipe("A","",ApplicationType(n),"file");
        QImage image(32,24,QImage::Format_Grayscale8);image.fill(255);const auto imagePath=root.filePath(QString("image%1.png").arg(n));check(image.save(imagePath),"Image");project.setImageFile(imagePath);project.save();
        auto snapshot=std::make_shared<StationSnapshot>();snapshot->station={QUuid::createUuid(),quint32(n+1),"Station"};snapshot->activation=QUuid::createUuid();snapshot->selection.recipe=*project.activeRecipe();snapshot->selection.projectDirectory=root.path();snapshot->selection.revision=QUuid::createUuid();snapshots.append(snapshot);
        runtimes.push_back(std::make_unique<RuntimeService>(root.filePath(QString("runtime%1").arg(n)),root.filePath("catalog.json"),engineer));coordinators.push_back(std::make_unique<RuntimeCoordinator>(*runtimes.back()));
        boundary.bind(snapshot->station.id,*runtimes.back(),*coordinators.back());controllers.push_back(std::make_unique<StationRunController>(boundary,snapshot->station.id));server.addStation(*controllers.back());
        check(controllers.back()->state().mode==StationMode::Stopped && !controllers.back()->automatic() && !runtimes.back()->active(),"Three station startup STOPPED/Auto false/no production snapshot");
        rejects([&]{controllers.back()->setAuto(true);});
    }
    rejects([&]{server.addStation(*controllers.front());});
    auto& station=*controllers[0];station.run(snapshots[0]);until([&]{station.poll();return station.state().mode==StationMode::ArmedManual;});check(!station.automatic(),"Run does not Auto");station.setAuto(true);check(station.automatic(),"Explicit Auto only after preparation");station.setAuto(false);check(!station.automatic(),"Disable closes admission flag");
    auto cycle=coordinators[0]->begin(RuntimeTrigger::Manual,{{"request","9007199254740993"},{"activation",snapshots[0]->activation.toString()}});
    station.stop();check(station.state().mode==StationMode::Draining && !station.automatic(),"Stop drains active cycle and closes Auto");
    client.write("control-port-alive");client.flush();until([&]{return server.peer()->bytesAvailable()>0;});check(server.peer()->readAll()=="control-port-alive","Control transport readable during drain; no codec claim");
    auto result=RuntimeService::execute(cycle,root.filePath("catalog.json"),coordinators[0]->cancellation());coordinators[0]->finish(cycle.ticket,result);station.poll();check(station.drained(),"Actual cycle drained");
    auto b=std::make_shared<StationSnapshot>(*snapshots[0]);b->activation=QUuid::createUuid();b->selection.recipe.name="B";b->selection.revision=QUuid::createUuid();station.run(b);station.stop();station.run(snapshots[0]);station.stop();
    check(server.peer()==originalPeer && server.port()==p.port && server.listenerGeneration()==generation,"Run Stop A B A retain listener and peer");
    station.run(snapshots[0]);auto pendingCycle=coordinators[0]->begin(RuntimeTrigger::Manual,{{"request","pending"}});auto pendingResult=RuntimeService::execute(pendingCycle,root.filePath("catalog.json"),coordinators[0]->cancellation());coordinators[0]->finish(pendingCycle.ticket,pendingResult);coordinators[0]->setDeliveryPending("old-terminal",true);
    QJsonObject pending;for(const auto& r:runtimes[0]->records())if(r["cycleId"]==pendingResult.cycleId)pending=r;check(!pending.isEmpty(),"Actual terminal");const auto terminalPath=root.filePath("runtime0/cycles/"+pending["recordId"].toString()+".json");const auto terminalBytes=read(terminalPath);
    // Another station remains executing at shutdown; lifecycle never fabricates completion.
    controllers[1]->run(snapshots[1]);auto unresolved=coordinators[1]->begin(RuntimeTrigger::Manual);
    server.beginShutdown(0);check(server.state()==CommunicationServerState::Closing && server.peer()==originalPeer,"Shutdown stop retains control until checkpoint");check(server.pollShutdown() && server.recoveryRequired(),"Deadline bounded with recovery");
    check(server.state()==CommunicationServerState::Closed && server.port()==0 && server.pollShutdown(),"Closed once, idempotent poll");
    check(read(terminalPath)==terminalBytes && coordinators[0]->deliveryPending() && runtimes[1]->busy(),"Pending exact bytes/barrier and unresolved acquisition retained");
    const auto recovery=QJsonDocument::fromJson(read(checkpoint)).object();check(recovery["recoveryRequired"].toBool() && recovery["stations"].toArray().size()==3,"Durable bounded shutdown recovery checkpoint");
    check(read(profilePath)==enabledBytes,"Lifecycle never rewrites profile");
    {CommunicationServerLifecycle restart(profiles,root.filePath("restart.json"));check(restart.state()==CommunicationServerState::Listening && restart.listenerGeneration()!=generation,"Restart listens new generation");RuntimeService fresh(root.filePath("runtime0"),root.filePath("catalog.json"),engineer);RuntimeCoordinator c(fresh);check(c.state()==RuntimeState::Unloaded && !fresh.active(),"Restart never restores Run");restart.beginShutdown(10);check(restart.pollShutdown() && !restart.recoveryRequired(),"Clean shutdown");}
    auto stopped=RuntimeService::execute(unresolved,root.filePath("catalog.json"),coordinators[1]->cancellation());coordinators[1]->finish(unresolved.ticket,stopped);
    check(std::async(std::launch::async,[&]{try{server.state();return false;}catch(const std::exception&){return true;}}).get(),"Owner thread enforced");
    {CommunicationServerLifecycle failedCheckpoint(profiles,root.path());failedCheckpoint.beginShutdown(0);check(failedCheckpoint.pollShutdown() && failedCheckpoint.recoveryRequired() && !failedCheckpoint.error().isEmpty(),"Checkpoint failure visible, closes bounded");}
    QFile corrupt(profilePath);check(corrupt.open(QIODevice::WriteOnly|QIODevice::Truncate),"Corrupt fixture");corrupt.write("{bad");corrupt.close();CommunicationServerLifecycle invalid(profiles,checkpoint);check(invalid.state()==CommunicationServerState::Fault && read(profilePath)=="{bad","Malformed restore does not rewrite");
    QTextStream(stdout)<<"PASS "<<assertions<<" lifecycle assertions\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
