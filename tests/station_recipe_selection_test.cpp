#include "application/CommunicationStationComposition.hpp"
#include "application/MachineSetup.hpp"
#include "application/StationSourceSetup.hpp"
#include "application/ProjectService.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "domain/ToolRegistry.hpp"
#include "persistence/ProjectRepository.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QTextStream>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QThread>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Expected selection rejection");}
struct ReentrantBoundary:StationActivationBoundary {
    mutable std::function<void()> callback;
    bool idleAndDrained(const QUuid&) const override{return true;}
    void checkConfiguration(const StationSnapshot&) const override{if(callback){auto f=std::move(callback);callback={};f();}}
};
QByteArray read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"Read");return f.readAll();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    using C=CommunicationV4Codec;
    QTemporaryDir temp(app.applicationDirPath()+"/vf04-XXXXXX");check(temp.isValid(),"Temp");QDir root(temp.path());
    MachineSetup(root.path()).start();JsonStationIdentityStore ids(root.filePath("communication-identities-v4.json"));
    const auto station=QUuid::createUuid(),camera=QUuid::createUuid(),station2=QUuid::createUuid(),camera2=QUuid::createUuid();
    const auto image=root.filePath("camera.png"),other=root.filePath("recipe.png");
    QImage bright(32,24,QImage::Format_Grayscale8);bright.fill(255);check(bright.save(image),"Camera raster");
    QImage dark(32,24,QImage::Format_Grayscale8);dark.fill(0);check(dark.save(other),"Recipe raster");
    StationSourceSetup(ids).save(ids.load().revision,station,"İstasyon 1",camera,"Kamera 1",image);
    StationSourceSetup(ids).save(ids.load().revision,station2,"İstasyon 2",camera2,"Kamera 2",other);
    const auto templ=MachineSetup::defaults(ids.load().installation)[0];
    PublicationPublisher publisher(root.path());
    auto publish=[&](QString name,int sourceCount,bool missingCalibration=false){
        ProjectService p;p.createProject(name,"",root.filePath(name+".vfproject"),"file");p.createRecipe(name,"",ApplicationType::DefectInspection,"file");p.setImageFile(other);
        if(sourceCount>1){p.enableMultiSource();const auto extra=p.addTool("image_file");p.setImageFile(other,extra);}
        if(sourceCount<0){p.removeTool(p.tools().first().id);p.addTool("webcam");}
        const auto blob=p.addTool("blob");if(sourceCount>1)p.setConnections({{p.tools().first().id,"image",blob,"image"}});
        p.setPublicationAssignments(PublicationAssignments{templ.id,templ.revision,{{PublicationSlotKind::Flag,0,{PublicationSourceKind::ToolOutput,blob,"found",ToolRegistry::get("blob").version}}}});p.save();
        if(missingCalibration){auto project=ProjectRepository{}.load(p.path());project.recipes[0].calibrationReference="missing-model";ProjectRepository{}.save(p.path(),project);}
        return publisher.publish(p.path(),p.activeRecipe()->id,root.filePath("publication-templates-v4")).publication;
    };
    const auto a=publish("A",1),b=publish("B",1),bad=publish("Two sources",2),cal=publish("Missing calibration",1,true),wrongType=publish("Webcam incompatible",-1);
    const auto publicationBefore=read(root.filePath("publication-catalogue-v4.json"));const auto identityBefore=read(root.filePath("communication-identities-v4.json"));
    std::shared_ptr<const StationSnapshot> last;
    {
        CommunicationServerProfiles profiles(root.filePath("communication-server-v4.json"));CommunicationPeerSessionService sessions(ids);
        CommunicationStationComposition machine(root.path(),{},ids,sessions);CommunicationServerLifecycle server(profiles,root.filePath("shutdown.json"),&sessions);CommunicationWorkspace ws(server,profiles,ids);machine.attach(server,ws);
        check(ws.stations().size()==2,"Two independent stations");
        auto row=[&](QUuid id){for(const auto& r:ws.stations())if(r.station.id==id)return r;throw std::runtime_error("Missing row");};
        const auto activationFile=root.filePath("communication-stations-v4.json");auto before=read(activationFile);
        ws.previewRecipe(station,a.recipeKey);check(read(activationFile)==before,"Preview never activates");
        ws.selectRecipe(station,{},a.recipeKey,a.templateKey);auto first=row(station).snapshot;check(bool(first) && !row(station).automatic,"A loaded stopped without Auto");
        check(first->selection.resolvedSource && first->selection.resolvedSource->path==image && first->selection.cameras[0].camera==camera,"Station owns camera/path");
        check(first->selection.recipe.definition==a.recipe.definition,"Published recipe unchanged");
        const auto savedSources=editorTools(a.recipe.definition);const auto bSources=editorTools(b.recipe.definition);check(savedSources[0].id!=bSources[0].id,"Distinct source tool UUIDs");
        ws.selectRecipe(station,first->activation,b.recipeKey,b.templateKey);last=row(station).snapshot;
        check(last->activation!=first->activation && last->selection.recipeKey==b.recipeKey && last->selection.cameras[0].camera==camera,"B new activation same camera");
        before=read(activationFile);
        rejects([&]{ws.selectRecipe(station,first->activation,a.recipeKey,a.templateKey);});
        rejects([&]{ws.selectRecipe(station,last->activation,bad.recipeKey,bad.templateKey);});
        rejects([&]{ws.selectRecipe(station,last->activation,cal.recipeKey,cal.templateKey);});
        rejects([&]{ws.selectRecipe(station,last->activation,wrongType.recipeKey,wrongType.templateKey);});
        rejects([&]{ws.selectRecipe(station,last->activation,a.recipeKey,999999);});
        rejects([&]{ws.selectRecipe(station,last->activation,999999,a.templateKey);});
        check(QFile::rename(image,image+".missing"),"Hide station image");rejects([&]{ws.selectRecipe(station,last->activation,a.recipeKey,a.templateKey);});check(QFile::rename(image+".missing",image),"Restore station image");
        check(read(activationFile)==before && !row(station2).snapshot,"Rejected selections preserve all state and other station");
        // The same published revision may be selected on two different machine sources.
        ws.selectRecipe(station2,{},b.recipeKey,b.templateKey);check(row(station2).snapshot->selection.cameras[0].camera==camera2,"Same immutable B different resolved station source");
        check(row(station).snapshot->activation==last->activation,"Other station selection isolated");
        const auto decoded=decodeStationCatalogue(read(activationFile).sliced(32));check(encodeStationCatalogue(decoded)==read(activationFile).sliced(32),"Schema 2 exact round trip");
        // Real Runtime load/prepare/begin/worker: B's original raster is black,
        // station raster white. Decoded pixels prove which path was processed.
        ws.command(station,last->activation,"run");ws.poll();
        rejects([&]{ws.selectRecipe(station,last->activation,a.recipeKey,a.templateKey);});
        ws.command(station,{},"stop");ws.poll();
        RuntimeService runtime(root.filePath("direct"),root.filePath("calibrations.json"),RuntimeIdentity::localEngineer());RuntimeCoordinator coordinator(runtime);
        StationRuntimeBoundary boundary;boundary.bind(station,runtime,coordinator);boundary.load(last);coordinator.start();
        const auto cycle=coordinator.begin(RuntimeTrigger::Manual);const auto result=RuntimeService::execute(cycle,root.filePath("calibrations.json"),coordinator.cancellation());coordinator.finish(cycle.ticket,result);
        check(result.state==ExecutionState::Done && result.frames.size()==1,"Real worker completed resolved source");
        bool white=false;for(const auto& tool:result.tools)for(const auto& port:tool.outputs)if(port.valid)if(const auto* frame=std::get_if<ImageFrame>(&port.data))white=frame->preview().pixelColor(0,0).red()==255;
        check(white,"Actual B execution processed Kamera 1 white raster");coordinator.stop();
        // Remote SELECT uses the same resolver; wrong remote permission and
        // exact pending-terminal barriers cannot be bypassed by local selection.
        check(read(root.filePath("communication-identities-v4.json"))==identityBefore,"Selection leaves machine identity/source bytes unchanged");
        auto cat=ids.load();const auto peer=QUuid::createUuid(),epoch=QUuid::createUuid();StationIdentityRegistry registry(ids);registry.enrollEpoch(cat.revision,peer,epoch);
        const auto connection=QUuid::createUuid();check(sessions.open(connection),"Connection");
        CommunicationV4Frame hello;hello.header.kind=1;hello.header.requestId=1;hello.header.peer=peer;hello.header.epoch=epoch;hello.body=QByteArray(384,0);hello.body.replace(0,16,CommunicationPeerSessionService::contractRevision().toRfc4122());check(sessions.handle(connection,hello).header.kind==101,"HELLO");
        auto request=[&](quint32 kind,quint64 n){CommunicationV4Frame f;f.header={kind,last->station.key,n,sessions.session(),peer,epoch,last->selection.recipeKey,b.templateKey,last->activation};f.body=QByteArray(384,0);return f;};
        auto select=request(4,2);C::putUInt32(select.body,0,a.recipeKey);C::putUInt32(select.body,4,a.templateKey);
        check(C::uint32(sessions.handle(connection,select).body,76)==25,"Remote permission denied");ws.setRemoteEnabled(true);
        check(sessions.handle(connection,select).header.kind==110,"Authorized remote SELECT accepted");ws.poll();last=row(station).snapshot;
        check(last->selection.recipeKey==a.recipeKey && last->selection.resolvedSource->path==image,"Remote uses station resolver");
        before=read(activationFile);rejects([&]{ws.selectRecipe(station,last->activation,b.recipeKey,b.templateKey);});check(read(activationFile)==before,"Pending remote terminal blocks local selection");
        StationDeliveryLedger ledger(root.filePath("production"),ids.load().installation);const V4RequestIdentity key{ids.load().installation,peer,epoch,station,2};const auto terminal=ledger.inspect(key)->value.replay;
        auto ack=request(11,2);ack.header.activation=terminal.header.activation;ack.header.recipeRevisionKey=terminal.header.recipeRevisionKey;ack.header.templateRevisionKey=terminal.header.templateRevisionKey;ack.body.replace(0,16,terminal.body.first(16));ack.body.replace(16,32,terminal.body.sliced(32,32));check(sessions.handle(connection,ack).header.kind==112,"Exact SELECT terminal ACK");
        ws.setRemoteEnabled(false);ws.selectRecipe(station,last->activation,b.recipeKey,b.templateKey);last=row(station).snapshot;
        ws.setRemoteEnabled(true);ws.command(station,last->activation,"auto");ws.poll();
        auto catalogue=request(3,10);catalogue.header.stationKey=0;catalogue.header.recipeRevisionKey=0;catalogue.header.templateRevisionKey=0;catalogue.header.activation=QUuid{};C::putUInt32(catalogue.body,0,1);C::putUInt32(catalogue.body,4,last->station.key);
        const auto metadata=sessions.handle(connection,catalogue);check(metadata.header.kind==103,"Station manifest");
        auto trigger=request(10,11);trigger.body.replace(0,16,metadata.body.sliced(248,16));check(sessions.handle(connection,trigger).header.kind==110,"Real station admission accepted B");
        rejects([&]{ws.selectRecipe(station,last->activation,a.recipeKey,a.templateKey);});
        const V4RequestIdentity productionKey{ids.load().installation,peer,epoch,station,11};QElapsedTimer timer;timer.start();
        while(timer.elapsed()<10000){ws.poll();const auto record=ledger.inspect(productionKey);if(record && record->value.replay.header.kind==111)break;QCoreApplication::processEvents();QThread::msleep(1);}
        const auto produced=ledger.inspect(productionKey);check(produced && produced->value.replay.header.kind==111,"Real execution registry sealed B terminal");const auto terminalB=produced->value.replay;
        check(C::uint32(terminalB.body,64)==1 && C::uint32(terminalB.body,84)==1 && C::uint32(terminalB.body,88)==1,"Actual station B result comes from white Kamera 1, not black recipe raster");
        ws.command(station,{},"stop");ws.poll();rejects([&]{ws.selectRecipe(station,last->activation,a.recipeKey,a.templateKey);});
        auto productionAck=request(11,11);productionAck.body.replace(0,16,terminalB.body.first(16));productionAck.body.replace(16,32,terminalB.body.sliced(32,32));check(sessions.handle(connection,productionAck).header.kind==112,"Exact production terminal ACK");ws.setRemoteEnabled(false);
        check(read(root.filePath("publication-catalogue-v4.json"))==publicationBefore,"Selections never mutate publications");
        // Identity changed only by explicit peer enrollment, never by selection.
        check(ids.load().stationSources.size()==2 && ids.load().stationSources[0].path==image,"Machine sources unchanged");
    }
    {
        CommunicationServerProfiles profiles(root.filePath("communication-server-v4.json"));CommunicationPeerSessionService sessions(ids);CommunicationStationComposition machine(root.path(),{},ids,sessions);CommunicationServerLifecycle server(profiles,root.filePath("shutdown2.json"),&sessions);CommunicationWorkspace ws(server,profiles,ids);machine.attach(server,ws);
        bool restored=false;for(const auto& row:ws.stations())if(row.station.id==station){restored=row.snapshot && row.snapshot->activation==last->activation && row.snapshot->selection.recipeKey==b.recipeKey && row.canRun && !row.automatic && row.runtime.mode==StationMode::Stopped;}
        check(restored && !ws.remoteEnabled(),"Restart restores exact resolved selection, all production permissions off");
    }
    {
        auto legacy=*last;legacy.selection.resolvedSource.reset();
        StationActivationCatalogue old{ids.load().installation,QUuid::createUuid(),{legacy}};
        const auto bytes=encodeStationCatalogue(old);check(encodeStationCatalogue(decodeStationCatalogue(bytes))==bytes,"Schema 1 exact round trip remains unchanged");
        auto next=old;next.revision=QUuid::createUuid();auto resolved=*last;resolved.activation=QUuid::createUuid();next.history.append(resolved);validateStationTransition(old,next);
        const auto migrated=decodeStationCatalogue(encodeStationCatalogue(next));check(migrated.history[0].activation==legacy.activation && stationRecipeBytes(migrated.history[0].selection)==stationRecipeBytes(legacy.selection),"Schema 1 to 2 retains legacy activation/manifest bytes");
        auto truncated=encodeStationCatalogue(next);truncated.chop(1);rejects([&]{decodeStationCatalogue(truncated);});
    }
    // Simulate event-loop reentrancy during configuration validation. The losing
    // activation CAS must preserve the already committed winner and its source.
    {
        FileStationActivationStore store(root.filePath("race.bin"));ReentrantBoundary boundary;StationActivationService service(store,ids,boundary);service.initialize();
        std::shared_ptr<const StationSnapshot> winner;
        boundary.callback=[&]{winner=service.activate(last->station,{},last->selection);};
        rejects([&]{service.activate(last->station,{},last->selection);});
        check(winner && service.snapshot(station)->activation==winner->activation && store.load().history.size()==1,"Selection race preserves only CAS winner");
        const auto stable=read(root.filePath("race.bin"));
        auto spoofed=last->selection;spoofed.resolvedSource->path=other;rejects([&]{service.activate(last->station,winner->activation,spoofed);});
        auto changed=last->selection;changed.recipe.name="Mutated publication";
        rejects([&]{service.activate(last->station,winner->activation,changed);});
        check(read(root.filePath("race.bin"))==stable,"Schema 2 rejects publication rewrite");
        boundary.callback=[&]{StationSourceSetup(ids).save(ids.load().revision,station,"İstasyon 1",camera,"Kamera 1",other);};
        rejects([&]{service.activate(last->station,winner->activation,last->selection);});
        check(read(root.filePath("race.bin"))==stable,"Source catalogue race preserves activation");
    }
    QTextStream(stdout)<<"PASS "<<checks<<" VF-04 selection/source checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
