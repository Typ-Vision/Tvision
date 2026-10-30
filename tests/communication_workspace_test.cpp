#include "application/CommunicationWorkspace.hpp"
#include "application/ProjectService.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "persistence/StationActivationStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QTextStream>
using namespace visionflow;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    int checks=0;auto check=[&](bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);};auto rejects=[&](auto f){bool rejected=false;try{f();}catch(...){rejected=true;}check(rejected,"Expected fail-closed rejection");};
    QTemporaryDir root(app.applicationDirPath()+"/communication-workspace-XXXXXX");check(root.isValid(),"Fixture root");
    JsonStationIdentityStore identities(root.filePath("registry"));StationIdentityRegistry registry(identities);registry.initialize(QUuid::createUuid());
    CommunicationServerProfiles profiles(root.filePath("server.json"));CommunicationServerProfile profile;profiles.save(profile);
    StationRuntimeBoundary boundary;RuntimeService runtime(root.filePath("production"),root.filePath("calibration"),RuntimeIdentity::localEngineer());RuntimeCoordinator coordinator(runtime);
    const auto station=QUuid::createUuid();const auto entry=registry.bind(identities.load().revision,{CatalogueCategory::Station,station,{},{}},"Station");boundary.bind(station,runtime,coordinator);StationRunController run(boundary,station);
    FileStationActivationStore selections(root.filePath("selections"));StationActivationService activations(selections,identities,boundary);activations.initialize();
    ProjectService p;p.createProject("Workspace","",root.filePath("p.vfproject"),"file");p.createRecipe("Recipe","",ApplicationType(0),"file");QImage image(32,32,QImage::Format_Grayscale8);image.fill(100);const auto imagePath=root.filePath("file.png");check(image.save(imagePath),"File source");const auto source=p.setImageFile(imagePath);p.save();
    const auto revision=QUuid::createUuid(),camera=QUuid::createUuid();const auto recipe=registry.bind(identities.load().revision,{CatalogueCategory::RecipeRevision,QUuid(p.activeRecipe()->id),revision,{}},"Recipe");const auto cam=registry.bind(identities.load().revision,{CatalogueCategory::Camera,camera,{},{}},"File source identity");
    auto snapshot=activations.activate({station,entry.key,entry.name},{},{revision,recipe.key,*p.activeRecipe(),root.path(),{{source,camera,cam.key}}});
    CommunicationServerLifecycle server(profiles,root.filePath("checkpoint"));server.addStation(run);CommunicationWorkspace workspace(server,profiles,identities);workspace.bind({snapshot->station,&run,[&]{return activations.snapshot(station);},{}, {},"Publication dispatch unavailable"});
    check(workspace.listener().contains("DISABLED") && workspace.stations().size()==1,"Actual process/registry state");check(!workspace.stations().first().canRun,"Unknown ledger cannot imply safe Run");rejects([&]{workspace.command(station,snapshot->activation,"run");});
    StationDeliveryLedger ledger(root.filePath("production"),identities.load().installation);workspace.setLedger(&ledger);check(workspace.stations().first().canRun,"Durable empty ledger + stopped selection can Run");
    rejects([&]{workspace.command(station,QUuid::createUuid(),"run");});rejects([&]{workspace.command(station,snapshot->activation,"auto");});
    workspace.command(station,snapshot->activation,"run");workspace.poll();check(run.state().mode==StationMode::ArmedManual && !run.automatic(),"Run without Auto");
    CommunicationMigrationInput in;rejects([&]{workspace.prepare(in,root.filePath("migration"),"x",{});});check(!QFile::exists(root.filePath("migration/active.json")),"Running Runtime prevents migration before transaction");
    workspace.command(station,{},"stop");workspace.poll();check(workspace.drained(),"Stop ignores stale displayed activation and drains real owner");
    workspace.setMaintenanceBarrier([]{return QString("Editor busy");});rejects([&]{workspace.reviewRollback(in,root.filePath("migration"));});workspace.setMaintenanceBarrier({});
    auto held=CameraLeaseArbiter::reserve("Calibration fixture",CameraLeaseUse::Calibration,{{camera.toString(QUuid::WithoutBraces),{"Fixture","Camera","1",DeviceIdentityConfidence::SerialReported}}});
    check(workspace.stations().first().cameraOwners.contains("Calibration fixture"),"Global lease owner shown");rejects([&]{workspace.reviewRollback(in,root.filePath("migration"));});held.leases.clear();
    const auto generation=server.listenerGeneration();profile.port=53317;profile.enabled=true;workspace.saveProfile(profile);check(server.state()==CommunicationServerState::Disabled && server.port()==0 && server.listenerGeneration()==generation && profiles.load()->port==53317,"Next-start profile never alters live listener");
    check(workspace.history().contains("["),"Read-only empty history");server.beginShutdown(0);check(server.pollShutdown(),"Bounded close");
    CommunicationServerLifecycle badSelection(profiles,root.filePath("bad-selection-checkpoint"),nullptr,"Invalid migration pointer");check(badSelection.state()==CommunicationServerState::Fault && badSelection.port()==0 && profiles.load()->enabled,"Bad selection never opens enabled profile or overwrites its file");
    QTextStream(stdout)<<"PASS "<<checks<<" workspace application checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
