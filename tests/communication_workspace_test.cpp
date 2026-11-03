#include "application/CommunicationWorkspace.hpp"
#include "application/ProjectService.hpp"
#include "application/StationSourceSetup.hpp"
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
    CommunicationWorkspace autoWorkspace(server,profiles,identities);autoWorkspace.setLedger(&ledger);
    bool failAuto=false;autoWorkspace.bind({snapshot->station,&run,[&]{return snapshot;},[&](bool value){if(value && failAuto)throw std::runtime_error("Injected preparation failure");run.setAuto(value);},{},{}});
    rejects([&]{autoWorkspace.startAutomaticProduction();});check(run.drained(),"Missing durable source fails before Run");
    StationSourceSetup(identities).save(identities.load().revision,station,"Station",camera,"Camera",imagePath);
    auto settle=[&]{for(int i=0;i<100 && autoWorkspace.automaticProductionBusy();++i){autoWorkspace.poll();QCoreApplication::processEvents();}check(!autoWorkspace.automaticProductionBusy(),"Batch transition settles");};
    autoWorkspace.startAutomaticProduction();rejects([&]{autoWorkspace.startAutomaticProduction();});settle();check(run.automatic(),"Single action starts real Run and Auto");
    check(!autoWorkspace.remoteEnabled(),"Local Auto never enables Remote");
    autoWorkspace.startAutomaticProduction();settle();check(run.automatic(),"Already automatic station preserved");
    autoWorkspace.command(station,{},"stop");autoWorkspace.poll();check(!run.automatic() && run.drained(),"Stop clears production permission");
    failAuto=true;autoWorkspace.startAutomaticProduction();settle();check(run.drained() && !run.automatic(),"Auto failure rolls back newly started station");failAuto=false;
    autoWorkspace.startAutomaticProduction();autoWorkspace.command(station,{},"stop");settle();check(run.drained() && !run.automatic(),"Stop fences late preparation");
    autoWorkspace.startAutomaticProduction();check(QFile::rename(imagePath,imagePath+".held"),"Hide source during transition");settle();check(run.drained() && !run.automatic(),"Source loss during preparation rolls back");check(QFile::rename(imagePath+".held",imagePath),"Restore source");
    autoWorkspace.command(station,snapshot->activation,"run");autoWorkspace.poll();failAuto=true;autoWorkspace.startAutomaticProduction();settle();check(run.state().mode==StationMode::ArmedManual && !run.automatic(),"Rollback preserves pre-existing manual Run");failAuto=false;autoWorkspace.command(station,{},"stop");autoWorkspace.poll();
    const auto station2=QUuid::createUuid(),camera2=QUuid::createUuid();
    const auto source2=StationSourceSetup(identities).save(identities.load().revision,station2,"Station 2",camera2,"Camera 2",imagePath);
    StationRuntimeBoundary boundary2;RuntimeService runtime2(root.filePath("production2"),root.filePath("calibration"),RuntimeIdentity::localEngineer());RuntimeCoordinator coordinator2(runtime2);boundary2.bind(station2,runtime2,coordinator2);StationRunController run2(boundary2,station2);
    FileStationActivationStore selections2(root.filePath("selections2"));StationActivationService activations2(selections2,identities,boundary2);activations2.initialize();
    const auto catalogue2=identities.load();quint32 stationKey2=0,cameraKey2=0;for(const auto& e:catalogue2.entries){if(e.identity.entity==station2)stationKey2=e.key;if(e.identity.entity==camera2)cameraKey2=e.key;}
    auto snapshot2=activations2.activate({station2,stationKey2,"Station 2"},{},{revision,recipe.key,*p.activeRecipe(),root.path(),{{source,camera2,cameraKey2}},source2});
    bool failSecond=true;autoWorkspace.bind({snapshot2->station,&run2,[&]{return snapshot2;},[&](bool value){if(value && failSecond)throw std::runtime_error("Second station failure");run2.setAuto(value);},{},{}});
    autoWorkspace.startAutomaticProduction();settle();check(run.drained() && run2.drained(),"Second-station failure rolls back all newly started owners");
    failSecond=false;autoWorkspace.startAutomaticProduction();settle();check(run.automatic() && run2.automatic(),"Batch prepares both actual station owners");
    autoWorkspace.command(station2,{},"stop");autoWorkspace.poll();failSecond=true;autoWorkspace.startAutomaticProduction();settle();check(run.automatic() && run2.drained(),"Failed batch preserves previously automatic station");autoWorkspace.command(station,{},"stop");autoWorkspace.poll();
    const auto generation=server.listenerGeneration();profile.port=53317;profile.enabled=true;workspace.saveProfile(profile);check(server.state()==CommunicationServerState::Disabled && server.port()==0 && server.listenerGeneration()==generation && profiles.load()->port==53317,"Next-start profile never alters live listener");
    check(workspace.history().contains("["),"Read-only empty history");server.beginShutdown(0);check(server.pollShutdown(),"Bounded close");
    CommunicationServerLifecycle badSelection(profiles,root.filePath("bad-selection-checkpoint"),nullptr,"Invalid migration pointer");check(badSelection.state()==CommunicationServerState::Fault && badSelection.port()==0 && profiles.load()->enabled,"Bad selection never opens enabled profile or overwrites its file");
    QTextStream(stdout)<<"PASS "<<checks<<" workspace application checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
