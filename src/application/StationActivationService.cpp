#include "StationActivationService.hpp"
#include <stdexcept>

namespace visionflow {
namespace {
void need(bool ok,const char* reason) {if(!ok)throw std::runtime_error(reason);}
const CatalogueEntry& entry(const IdentityCatalogue& c,std::uint32_t key,CatalogueCategory category,const QUuid& entity,const QUuid& revision={}) {
    for(const auto& e:c.entries)if(e.key==key){need(!e.retired && e.identity.category==category && e.identity.entity==entity && e.identity.revision==revision,"Station catalogue identity mismatch or retired");return e;}
    throw std::runtime_error("Station catalogue key missing");
}
}
StationActivationService::StationActivationService(StationActivationStore& store,StationIdentityStore& identities,StationActivationBoundary& boundary):store_(store),identities_(identities),boundary_(boundary){}
void StationActivationService::owner() const {need(QThread::currentThread()==owner_,"Station activation owner thread required");}
void StationActivationService::initialize() {owner();const auto c=identities_.load();validateIdentityCatalogue(c);store_.create({c.installation,QUuid::createUuid(),{}});}
QVector<std::shared_ptr<const StationSnapshot>> StationActivationService::snapshots() const {
    owner();const auto c=store_.load();validateStationCatalogue(c);const auto identity=identities_.load();
    need(c.installation==identity.installation,"Station installation mismatch");
    QMap<QUuid,std::shared_ptr<const StationSnapshot>> active;
    for(const auto& s:c.history)active[s.station.id]=std::make_shared<const StationSnapshot>(s);
    return active.values().toVector();
}
std::shared_ptr<const StationSnapshot> StationActivationService::snapshot(const QUuid& station) const {
    for(const auto& s:snapshots())if(s->station.id==station)return s;
    return {};
}
std::shared_ptr<const StationSnapshot> StationActivationService::activate(const StationDefinition& station,const QUuid& expected,const StationRecipeSelection& selection) {
    owner();auto c=store_.load();validateStationCatalogue(c);const auto identity=identities_.load();validateIdentityCatalogue(identity);
    need(c.installation==identity.installation,"Station installation mismatch");QUuid actual;
    for(const auto& s:c.history)if(s.station.id==station.id)actual=s.activation;
    need(actual==expected,"STALE_ACTIVATION");need(boundary_.idleAndDrained(station.id),"STATION_NOT_DRAINED");
    entry(identity,station.key,CatalogueCategory::Station,station.id);
    entry(identity,selection.recipeKey,CatalogueCategory::RecipeRevision,QUuid(selection.recipe.id),selection.revision);
    for(const auto& b:selection.cameras)entry(identity,b.cameraKey,CatalogueCategory::Camera,b.camera);
    StationSnapshot next{station,QUuid::createUuid(),selection};c.history.append(next);const auto previous=c.revision;c.revision=QUuid::createUuid();validateStationCatalogue(c);
    boundary_.checkConfiguration(next);
    // Configuration check is synchronous/read-only. Recheck a reentrant boundary before commit.
    need(boundary_.idleAndDrained(station.id),"STATION_NOT_DRAINED");
    store_.compareExchange(previous,c);return std::make_shared<const StationSnapshot>(std::move(next));
}
}
