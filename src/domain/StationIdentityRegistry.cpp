#include "StationIdentityRegistry.hpp"
#include <QSet>
#include <QHash>
#include <limits>
#include <stdexcept>

namespace visionflow {
namespace {
void need(bool ok) { if(!ok) throw std::runtime_error("Invalid identity catalogue"); }
constexpr auto exhausted=std::uint64_t(std::numeric_limits<std::uint32_t>::max())+1;
}
QByteArray exportIdentityUuid(const QUuid& id) { need(!id.isNull()); return id.toRfc4122(); }
QUuid importIdentityUuid(const QByteArray& bytes) {
    need(bytes.size()==16); auto id=QUuid::fromRfc4122(bytes); need(!id.isNull()); return id;
}
void validateIdentityCatalogue(const IdentityCatalogue& c) {
    need(!c.installation.isNull() && !c.revision.isNull());
    need(c.nextKey>=1 && c.nextKey<=exhausted && c.entries.size()<=65536 && c.epochs.size()<=65536);
    QSet<std::uint32_t> keys; QSet<QByteArray> identities; QSet<QUuid> activePeers;
    QSet<QByteArray> epochs;
    for(const auto& e:c.entries) {
        auto category=static_cast<std::uint32_t>(e.identity.category);
        need(category>=1 && category<=13 && e.key>0 && e.key<c.nextKey && !keys.contains(e.key));
        need(!e.identity.entity.isNull());
        const bool stable=category==1 || category==4;
        need(stable==e.identity.revision.isNull());
        need(!e.name.contains(QChar::Null) && !e.name.isEmpty() && e.name.toUtf8().size()<=64
             && QString::fromUtf8(e.name.toUtf8())==e.name);
        const auto token=QByteArray::number(category)+e.identity.entity.toRfc4122()+e.identity.revision.toRfc4122();
        need(!identities.contains(token)); identities.insert(token); keys.insert(e.key);
    }
    for(const auto& e:c.epochs) {
        need(!e.peer.isNull() && !e.epoch.isNull());
        auto token=e.peer.toRfc4122()+e.epoch.toRfc4122();
        need(!epochs.contains(token)); epochs.insert(token);
        if(!e.retired) { need(!activePeers.contains(e.peer)); activePeers.insert(e.peer); }
    }
}
void validateIdentityTransition(const IdentityCatalogue& old, const IdentityCatalogue& next) {
    validateIdentityCatalogue(old); validateIdentityCatalogue(next);
    need(old.installation==next.installation && old.revision!=next.revision && next.nextKey>=old.nextKey);
    QHash<std::uint32_t,const CatalogueEntry*> oldKeys,newKeys;
    for(const auto& e:old.entries) oldKeys.insert(e.key,&e);
    for(const auto& e:next.entries) newKeys.insert(e.key,&e);
    for(const auto& e:old.entries) {
        const auto found=newKeys.value(e.key,nullptr);
        need(found && found->identity==e.identity && (!e.retired || found->retired));
    }
    for(const auto& e:next.entries) {
        if(e.key<old.nextKey)
            need(oldKeys.contains(e.key));
    }
    QHash<QByteArray,bool> newEpochs;
    for(const auto& e:next.epochs) newEpochs.insert(e.peer.toRfc4122()+e.epoch.toRfc4122(),e.retired);
    for(const auto& e:old.epochs) {
        const auto key=e.peer.toRfc4122()+e.epoch.toRfc4122();
        need(newEpochs.contains(key) && (!e.retired || newEpochs.value(key)));
    }
}
}
