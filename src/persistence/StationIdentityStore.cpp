#include "StationIdentityStore.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <limits>
#include <stdexcept>
namespace visionflow {
namespace {
constexpr qint64 maxBytes=32*1024*1024;
void need(bool ok) { if(!ok) throw std::runtime_error("Identity store invalid, stale, locked or unavailable"); }
QString uuid(const QUuid& id) {return QString::fromLatin1(id.toRfc4122().toHex());}
QUuid uuid(const QJsonValue& value) {
    need(value.isString());auto hex=value.toString().toLatin1();
    auto bytes=QByteArray::fromHex(hex);need(hex.size()==32 && bytes.size()==16 && bytes.toHex()==hex);
    return QUuid::fromRfc4122(bytes);
}
std::uint32_t number(const QJsonValue& value) {
    need(value.isDouble());const auto n=value.toDouble();
    need(n>=0 && n<=std::numeric_limits<std::uint32_t>::max());
    const auto result=static_cast<std::uint32_t>(n);need(result==n);return result;
}
QJsonObject payload(const IdentityCatalogue& c) {
    validateIdentityCatalogue(c);QJsonArray entries,epochs;
    for(const auto& e:c.entries) entries.append(QJsonObject{
        {"key",qint64(e.key)},{"category",int(e.identity.category)},{"entity",uuid(e.identity.entity)},
        {"revision",uuid(e.identity.revision)},{"parent",uuid(e.identity.parent)},{"name",e.name},{"retired",e.retired}});
    for(const auto& e:c.epochs) epochs.append(QJsonObject{{"peer",uuid(e.peer)},{"epoch",uuid(e.epoch)},{"retired",e.retired}});
    return {{"schemaVersion",1},{"installation",uuid(c.installation)},{"catalogueRevision",uuid(c.revision)},
        {"nextKey",QString::number(c.nextKey)},{"entries",entries},{"epochs",epochs}};
}
QByteArray compact(const QJsonObject& o) {return QJsonDocument(o).toJson(QJsonDocument::Compact);}
QByteArray digest(const QJsonObject& o) {return QCryptographicHash::hash(compact(o),QCryptographicHash::Sha256).toHex();}
}
QByteArray JsonStationIdentityStore::encode(const IdentityCatalogue& c) {
    const auto p=payload(c);auto bytes=compact({{"payload",p},{"sha256",QString::fromLatin1(digest(p))}});
    need(bytes.size()<=maxBytes);return bytes;
}
IdentityCatalogue JsonStationIdentityStore::decode(const QByteArray& bytes) {
    need(bytes.size()<=maxBytes);QJsonParseError error;
    const auto doc=QJsonDocument::fromJson(bytes,&error);need(error.error==QJsonParseError::NoError && doc.isObject());
    const auto envelope=doc.object();need(envelope.size()==2 && envelope["payload"].isObject() && envelope["sha256"].isString());
    const auto p=envelope["payload"].toObject();need(envelope["sha256"].toString().toLatin1()==digest(p));
    need(p.size()==6 && p["schemaVersion"]==1 && p["entries"].isArray() && p["epochs"].isArray() && p["nextKey"].isString());
    IdentityCatalogue c;c.installation=uuid(p["installation"]);c.revision=uuid(p["catalogueRevision"]);
    bool ok=false;c.nextKey=p["nextKey"].toString().toULongLong(&ok);need(ok && QString::number(c.nextKey)==p["nextKey"].toString());
    for(auto v:p["entries"].toArray()) {
        need(v.isObject());auto e=v.toObject();need(e.size()==7 && e["name"].isString() && e["retired"].isBool());
        c.entries.append({number(e["key"]),{CatalogueCategory(number(e["category"])),uuid(e["entity"]),uuid(e["revision"]),uuid(e["parent"])},e["name"].toString(),e["retired"].toBool()});
    }
    for(auto v:p["epochs"].toArray()) {
        need(v.isObject());auto e=v.toObject();need(e.size()==3 && e["retired"].isBool());
        c.epochs.append({uuid(e["peer"]),uuid(e["epoch"]),e["retired"].toBool()});
    }
    validateIdentityCatalogue(c);need(payload(c)==p);return c;
}
IdentityCatalogue JsonStationIdentityStore::load() const {
    QFile file(path_);need(file.open(QIODevice::ReadOnly) && file.size()<=maxBytes);
    return decode(file.readAll());
}
void JsonStationIdentityStore::write(const IdentityCatalogue& c) const {
    const auto bytes=encode(c);QSaveFile file(path_);file.setDirectWriteFallback(false);
    need(file.open(QIODevice::WriteOnly));if(boundary_) boundary_("PREPARED");
    const auto half=bytes.size()/2;need(file.write(bytes.first(half))==half);
    if(boundary_) boundary_("HALF_WRITTEN");
    need(file.write(bytes.sliced(half))==bytes.size()-half);if(boundary_) boundary_("WRITTEN");
    need(file.commit());if(boundary_) boundary_("COMMITTED");
}
void JsonStationIdentityStore::create(const IdentityCatalogue& c) const {
    validateIdentityCatalogue(c);need(QDir().mkpath(QFileInfo(path_).absolutePath()));
    QLockFile lock(path_+".lock");lock.setStaleLockTime(0);need(lock.tryLock(0));
    need(!QFileInfo::exists(path_));write(c);
}
void JsonStationIdentityStore::compareExchange(const QUuid& revision,const IdentityCatalogue& c) const {
    QLockFile lock(path_+".lock");lock.setStaleLockTime(0);need(lock.tryLock(0));
    const auto old=load();need(old.revision==revision);validateIdentityTransition(old,c);write(c);
}
}
