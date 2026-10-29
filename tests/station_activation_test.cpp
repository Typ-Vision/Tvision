#include "application/StationRuntimeBoundary.hpp"
#include "application/ProjectService.hpp"
#include "persistence/StationActivationStore.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "domain/ToolDefinition.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QLockFile>
#include <QTextStream>
#include <QJsonDocument>
#include <future>
#include <vector>

using namespace visionflow;
namespace {
int assertions=0;
void check(bool ok,const char* message){++assertions;if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Expected rejection");}
QUuid id(){return QUuid::createUuid();}
QByteArray read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"Read file");return f.readAll();}
void write(const QString& path,const QByteArray& bytes){QFile f(path);check(f.open(QIODevice::WriteOnly|QIODevice::Truncate),"Open write");check(f.write(bytes)==bytes.size(),"Write fixture");}
struct OfflineBoundary:StationActivationBoundary {
    bool idleAndDrained(const QUuid&) const override{return true;}
    void checkConfiguration(const StationSnapshot&) const override{}
};
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir root(app.applicationDirPath()+"/station-XXXXXX");check(root.isValid(),"Temp root");
    JsonStationIdentityStore identities(root.filePath("identities.json"));StationIdentityRegistry registry(identities);registry.initialize(id());
    auto enroll=[&](CatalogueCategory category,QUuid entity,QUuid revision=QUuid{}){return registry.bind(identities.load().revision,{category,entity,revision,{}},"Fixture");};
    FileStationActivationStore store(root.filePath("stations.bin"));StationRuntimeBoundary boundary;StationActivationService service(store,identities,boundary);
    rejects([&]{service.snapshots();});service.initialize();rejects([&]{service.initialize();});
    check(service.snapshots().isEmpty(),"Explicit empty initialization");
    std::vector<std::unique_ptr<RuntimeService>> runtimes;std::vector<std::unique_ptr<RuntimeCoordinator>> coordinators;
    QVector<StationDefinition> stations;QVector<StationRecipeSelection> recipes;QVector<std::shared_ptr<const StationSnapshot>> snapshots;
    const RuntimeIdentity engineer{"engineer",[](const QString& p,ProductionAction){return p=="engineer";}};
    for(int n=0;n<4;++n){
        auto stationId=id(),camera=id(),revision=id();const auto station=enroll(CatalogueCategory::Station,stationId),cam=enroll(CatalogueCategory::Camera,camera);
        const auto path=root.filePath(QString("part%1.vfproject").arg(n));ProjectService project;project.createProject("Station","",path,"file");
        project.createRecipe(QString("Saved%1").arg(n),"",ApplicationType(n%3),"file");
        QImage image(32,24,QImage::Format_Grayscale8);image.fill(255);const auto frame=root.filePath(QString("frame%1.png").arg(n));check(image.save(frame),"Image fixture");
        project.setImageFile(frame);project.save();auto recipe=*project.activeRecipe();const auto recipeEntry=enroll(CatalogueCategory::RecipeRevision,QUuid(recipe.id),revision);
        StationRecipeSelection selection{revision,recipeEntry.key,recipe,root.path(),{{editorTools(recipe.definition).first().id,camera,cam.key}}};
        auto runtime=std::make_unique<RuntimeService>(root.filePath(QString("runtime%1").arg(n)),root.filePath("catalog.json"),engineer);
        auto coordinator=std::make_unique<RuntimeCoordinator>(*runtime);boundary.bind(stationId,*runtime,*coordinator);
        stations.append({stationId,station.key,QString("Station %1").arg(n)});recipes.append(selection);
        snapshots.append(service.activate(stations.last(),{},selection));runtimes.push_back(std::move(runtime));coordinators.push_back(std::move(coordinator));
    }
    check(service.snapshots().size()==4,"Four stations without hardcoded triple");
    for(int n=0;n<4;++n){check(service.snapshot(stations[n].id)->selection.recipe.id==recipes[n].recipe.id,"Independent recipe");check(service.snapshot(stations[n].id)->selection.cameras==recipes[n].cameras,"Independent camera");check(coordinators[n]->state()==RuntimeState::Unloaded && !runtimes[n]->active(),"Activation opens no production permission");}
    auto draft=recipes[0];draft.recipe.name="Editor changed";draft.recipe.definition={};
    check(service.snapshot(stations[0].id)->selection.recipe.name=="Saved0" && !snapshots[0]->selection.recipe.definition.isEmpty(),"Editor COW selection cannot mutate snapshot");
    rejects([&]{boundary.bind(stations[1].id,*runtimes[1],*coordinators[1]);});rejects([&]{boundary.bind(id(),*runtimes[0],*coordinators[0]);});
    StationRuntimeBoundary mismatched;rejects([&]{mismatched.bind(id(),*runtimes[0],*coordinators[1]);});
    auto b=recipes[1]; // Different saved recipe/source/camera for station zero.
    const auto a=snapshots[0];boundary.load(a);coordinators[0]->start();auto cycle=coordinators[0]->begin(RuntimeTrigger::Manual);
    check(boundary.state(stations[0].id).mode==StationMode::Running,"Running mode is station-local");
    check(boundary.state(stations[1].id).mode==StationMode::Stopped,"Other station stays stopped");
    rejects([&]{service.activate(stations[0],a->activation,b);});coordinators[0]->stop();
    check(coordinators[0]->state()==RuntimeState::Stopping,"Stop waits for active cycle");rejects([&]{service.activate(stations[0],a->activation,b);});
    check(boundary.state(stations[0].id).mode==StationMode::Draining,"Draining mode preserved");
    auto result=RuntimeService::execute(cycle,root.filePath("catalog.json"),coordinators[0]->cancellation());coordinators[0]->finish(cycle.ticket,result);
    check(boundary.idleAndDrained(stations[0].id),"Real Runtime drained");
    // Produce an actual terminal with station/activation metadata, then preserve it through selection.
    coordinators[0]->start();QJsonObject metadata{{"station",stations[0].id.toString()},{"activation",a->activation.toString()},{"recipeRevision",recipes[0].revision.toString()},{"request","9007199254740993"}};
    auto terminalCycle=coordinators[0]->begin(RuntimeTrigger::Manual,metadata);
    auto terminal=RuntimeService::execute(terminalCycle,root.filePath("catalog.json"),coordinators[0]->cancellation());coordinators[0]->finish(terminalCycle.ticket,terminal);
    auto terminalRecord=[&]{for(const auto& record:runtimes[0]->records())if(record["cycleId"]==terminal.cycleId)return record;throw std::runtime_error("Terminal not found");};
    const auto pending=terminalRecord();check(pending["communication"].toObject()==metadata && pending["communicationState"]=="RESULT_PENDING","Actual terminal exact metadata");
    const auto pendingBytes=QJsonDocument(pending).toJson(QJsonDocument::Compact);coordinators[0]->setDeliveryPending("old-terminal",true);coordinators[0]->stop();
    const auto terminalPath=root.filePath("runtime0/cycles/"+pending["recordId"].toString()+".json");const auto durableTerminal=read(terminalPath);
    const auto other=stationRecipeBytes(service.snapshot(stations[1].id)->selection);const auto otherActivation=service.snapshot(stations[1].id)->activation;
    const auto selectedB=service.activate(stations[0],a->activation,b);
    check(selectedB->activation!=a->activation && cycle.snapshot->recipe().id==recipes[0].recipe.id,"New activation, old cycle immutable");
    check(coordinators[0]->deliveryPending() && QJsonDocument(terminalRecord()).toJson(QJsonDocument::Compact)==pendingBytes,"Pending terminal and delivery barrier preserved byte-exact");
    check(read(terminalPath)==durableTerminal,"Durable terminal exact bytes unchanged");
    rejects([&]{boundary.load(selectedB);});
    rejects([&]{service.activate(stations[0],a->activation,recipes[0]);});
    const auto selectedA=service.activate(stations[0],selectedB->activation,recipes[0]);
    check(selectedA->activation!=a->activation && selectedA->activation!=selectedB->activation,"A B A never revives activation");
    check(service.snapshot(stations[1].id)->activation==otherActivation && stationRecipeBytes(service.snapshot(stations[1].id)->selection)==other,"Other station unchanged");
    check(coordinators[0]->state()==RuntimeState::LoadedStopped && coordinators[0]->deliveryPending(),"No implicit Auto/Run or ACK on selection");
    check(boundary.state(stations[0].id).mode==StationMode::Stopped && boundary.state(stations[0].id).deliveryPending,"Stopped and pending are separate state");
    const auto stable=read(root.filePath("stations.bin"));
    auto invalid=recipes[0];invalid.recipeKey=recipes[1].recipeKey;rejects([&]{service.activate(stations[0],selectedA->activation,invalid);});
    invalid=recipes[0];invalid.cameras[0].cameraKey=recipes[1].cameras[0].cameraKey;rejects([&]{service.activate(stations[0],selectedA->activation,invalid);});
    invalid=recipes[0];invalid.cameras.append(invalid.cameras.first());rejects([&]{service.activate(stations[0],selectedA->activation,invalid);});
    invalid=recipes[0];invalid.cameras.clear();rejects([&]{service.activate(stations[0],selectedA->activation,invalid);});
    invalid=recipes[0];invalid.recipe.name="Rewrite immutable revision";rejects([&]{service.activate(stations[0],selectedA->activation,invalid);});
    invalid=recipes[0];invalid.revision=id();rejects([&]{service.activate(stations[0],selectedA->activation,invalid);});
    invalid=recipes[0];const auto freshRevision=id();auto registered=enroll(CatalogueCategory::RecipeRevision,QUuid(invalid.recipe.id),freshRevision);invalid.revision=freshRevision;invalid.recipeKey=registered.key;
    auto tools=editorTools(invalid.recipe.definition);tools.first().parameters["path"]=root.filePath("missing.png");invalid.recipe.definition=withEditorTools(invalid.recipe.definition,tools);
    rejects([&]{service.activate(stations[0],selectedA->activation,invalid);});check(read(root.filePath("stations.bin"))==stable,"All failures preserve old activation bytes");
    QLockFile lock(root.filePath("stations.bin.lock"));check(lock.tryLock(0),"Lock fixture");rejects([&]{service.activate(stations[0],selectedA->activation,b);});lock.unlock();
    check(read(root.filePath("stations.bin"))==stable,"Store failure preserves old state");
    FileStationActivationStore reopened(root.filePath("stations.bin"));StationActivationService restored(reopened,identities,boundary);
    check(restored.snapshot(stations[0].id)->activation==selectedA->activation && restored.snapshots().size()==4,"Restore exact activation/source set");
    RuntimeService restartRuntime(root.filePath("runtime0"),root.filePath("catalog.json"),engineer);RuntimeCoordinator restartCoordinator(restartRuntime);StationRuntimeBoundary restartBoundary;
    restartBoundary.bind(stations[0].id,restartRuntime,restartCoordinator);
    check(restartBoundary.state(stations[0].id).mode==StationMode::Stopped && !restartRuntime.active(),"Restart snapshot does not restore production permission");
    auto old=store.load(),next=old;next.revision=id();auto added=next.history.last();added.activation=id();next.history.append(added);store.compareExchange(old.revision,next);
    rejects([&]{store.compareExchange(old.revision,next);});auto rewritten=store.load();rewritten.revision=id();rewritten.history[0].station.name="Lost history";added.activation=id();rewritten.history.append(added);rejects([&]{store.compareExchange(next.revision,rewritten);});
    auto truncated=stable.first(stable.size()/2);write(root.filePath("corrupt.bin"),truncated);FileStationActivationStore corrupt(root.filePath("corrupt.bin"));rejects([&]{corrupt.load();});
    auto flipped=stable;flipped[flipped.size()-1]^=1;write(root.filePath("corrupt.bin"),flipped);rejects([&]{corrupt.load();});
    check(std::async(std::launch::async,[&]{try{service.snapshots();return false;}catch(const std::exception&){return true;}}).get(),"Owner thread boundary");
    OfflineBoundary offline;FileStationActivationStore manyStore(root.filePath("many.bin"));StationActivationService many(manyStore,identities,offline);many.initialize();
    for(int n=0;n<32;++n){const auto uuid=id();const auto e=enroll(CatalogueCategory::Station,uuid);many.activate({uuid,e.key,"Station"},{},recipes[0]);}
    auto excess=id();const auto e=enroll(CatalogueCategory::Station,excess);rejects([&]{many.activate({excess,e.key,"33rd"},{},recipes[0]);});check(many.snapshots().size()==32,"Frozen 32 station limit");
    registry.retire(identities.load().revision,recipes[1].recipeKey);rejects([&]{service.activate(stations[0],service.snapshot(stations[0].id)->activation,b);});
    QTextStream(stdout)<<"PASS "<<assertions<<" assertions; immutable station selection, Runtime drain and pending metadata\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL "<<e.what()<<"\n";return 1;}}
