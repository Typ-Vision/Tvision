#include "StationIdentityRegistry.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace visionflow {
namespace {
void need(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
CatalogueEntry& entry(IdentityCatalogue& c,std::uint32_t key) {
    auto i=std::find_if(c.entries.begin(),c.entries.end(),[&](const auto& e){return e.key==key;});
    need(i!=c.entries.end(),"Unknown catalogue key");return *i;
}
}
IdentityCatalogue StationIdentityRegistry::initialize(const QUuid& installation) {
    IdentityCatalogue c; c.installation=installation;c.revision=QUuid::createUuid();
    validateIdentityCatalogue(c);store_.create(c);return c;
}
IdentityCatalogue StationIdentityRegistry::snapshot(const QUuid& expected) const {
    auto c=store_.load();validateIdentityCatalogue(c);
    need(c.revision==expected,"Catalogue revision mismatch");return c;
}
void StationIdentityRegistry::update(const QUuid& expected,const std::function<void(IdentityCatalogue&)>& mutation) {
    auto c=snapshot(expected);auto old=c;mutation(c);
    if(c==old) return;
    c.revision=QUuid::createUuid();validateIdentityTransition(old,c);store_.compareExchange(expected,c);
}
CatalogueEntry StationIdentityRegistry::bind(const QUuid& expected,const CatalogueIdentity& identity,const QString& name) {
    CatalogueEntry result;
    update(expected,[&](auto& c){
        auto found=std::find_if(c.entries.begin(),c.entries.end(),[&](const auto& e){
            return e.identity.category==identity.category && e.identity.entity==identity.entity && e.identity.revision==identity.revision;
        });
        if(found!=c.entries.end()) {
            need(found->identity==identity && !found->retired && found->name==name,"Identity conflict or retired key");
            result=*found;return;
        }
        need(c.nextKey<=std::numeric_limits<std::uint32_t>::max(),"Identity key space exhausted; admission blocked");
        result={static_cast<std::uint32_t>(c.nextKey++),identity,name,false};c.entries.append(result);
    });return result;
}
void StationIdentityRegistry::rename(const QUuid& revision,std::uint32_t key,const QString& name) {
    update(revision,[&](auto& c){auto& e=entry(c,key);need(!e.retired,"Retired key");e.name=name;});
}
void StationIdentityRegistry::retire(const QUuid& revision,std::uint32_t key) {
    update(revision,[&](auto& c){entry(c,key).retired=true;});
}
void StationIdentityRegistry::enrollEpoch(const QUuid& revision,const QUuid& peer,const QUuid& epoch) {
    update(revision,[&](auto& c){
        for(const auto& e:c.epochs) if(e.peer==peer) {
            if(e.epoch==epoch) {need(!e.retired,"Retired peer epoch");return;}
            need(e.retired,"Active peer epoch must be explicitly retired first");
        }
        c.epochs.append({peer,epoch,false});
    });
}
void StationIdentityRegistry::retireEpoch(const QUuid& revision,const QUuid& peer,const QUuid& epoch) {
    update(revision,[&](auto& c){
        auto i=std::find_if(c.epochs.begin(),c.epochs.end(),[&](const auto& e){return e.peer==peer && e.epoch==epoch;});
        need(i!=c.epochs.end(),"Unknown peer epoch");i->retired=true;
    });
}
}
