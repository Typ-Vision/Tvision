#include "CommunicationProfiles.hpp"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QUuid>
#include <stdexcept>
namespace visionflow {
namespace {
void need(bool ok){if(!ok)throw std::runtime_error("Invalid machine-local endpoint profile/store");}
QJsonObject encode(const EndpointProfile& p){validateEndpointProfile(p);QJsonObject mapping;for(auto i=p.fieldMapping.begin();i!=p.fieldMapping.end();++i)mapping[i.key()]=i.value();return {{"id",p.id},{"revision",p.revision},{"contractRevision",p.contractRevision},{"protocol",p.protocol},{"role",p.role},{"address",p.address},{"port",p.port},{"enabled",p.enabled},{"timeoutMs",p.timeoutMs},{"retries",p.retries},{"credentialRef",p.credentialRef},{"fieldMapping",mapping}};}
int integer(const QJsonObject& o,const char* k){need(o[k].isDouble() && o[k].toDouble()==o[k].toInt());return o[k].toInt();}
QString text(const QJsonObject& o,const char* k){need(o[k].isString());return o[k].toString();}
}
void validateEndpointProfile(const EndpointProfile& p){
    need(!QUuid(p.id).isNull() && QUuid(p.id).toString(QUuid::WithoutBraces)==p.id && p.revision>0 && p.revision<=1000000000 && p.contractRevision>0 && p.contractRevision<=1000000000);
    need(p.protocol=="tcp" && (p.role=="client" || p.role=="server") && !p.address.isEmpty() && p.address.size()<=255 && !p.address.contains(QChar::Null) && p.port>0 && p.port<=65535 && p.timeoutMs>0 && p.timeoutMs<=600000 && p.retries>=0 && p.retries<=100);
    static const QRegularExpression ref("^[A-Za-z0-9_.:/-]{1,256}$"),name("^[A-Za-z][A-Za-z0-9_.-]{0,63}$");need(p.credentialRef.isEmpty() || ref.match(p.credentialRef).hasMatch());need(p.fieldMapping.size()<=64);
    QSet<QString> destinations;for(auto i=p.fieldMapping.begin();i!=p.fieldMapping.end();++i){need(name.match(i.key()).hasMatch() && !i.value().isEmpty() && i.value().size()<=256 && !i.value().contains(QChar::Null) && !destinations.contains(i.value()));destinations.insert(i.value());}
}
QByteArray endpointDependencyId(const EndpointProfile& p){return QCryptographicHash::hash(QJsonDocument(encode(p)).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();}
QMap<QString,EndpointProfile> CommunicationProfiles::load() const{
    QFile f(path_);if(!f.exists())return {};need(f.open(QIODevice::ReadOnly) && f.size()<=1024*1024);QJsonParseError error;const auto doc=QJsonDocument::fromJson(f.readAll(),&error);need(error.error==QJsonParseError::NoError && doc.isObject());const auto o=doc.object();need(o.size()==2 && o["schemaVersion"]==1 && o["profiles"].isArray() && o["profiles"].toArray().size()<=256);QMap<QString,EndpointProfile> result;
    for(auto v:o["profiles"].toArray()){need(v.isObject());const auto j=v.toObject();need(j.size()==12);EndpointProfile p;p.id=text(j,"id");p.revision=integer(j,"revision");p.contractRevision=integer(j,"contractRevision");p.protocol=text(j,"protocol");p.role=text(j,"role");p.address=text(j,"address");p.port=integer(j,"port");need(j["enabled"].isBool());p.enabled=j["enabled"].toBool();p.timeoutMs=integer(j,"timeoutMs");p.retries=integer(j,"retries");p.credentialRef=text(j,"credentialRef");need(j["fieldMapping"].isObject());auto m=j["fieldMapping"].toObject();for(auto i=m.begin();i!=m.end();++i){need(i->isString());p.fieldMapping[i.key()]=i->toString();}validateEndpointProfile(p);need(!result.contains(p.id) && encode(p)==j);result[p.id]=p;}return result;
}
void CommunicationProfiles::save(const QMap<QString,EndpointProfile>& profiles) const{
    need(profiles.size()<=256);const auto old=load();QJsonArray a;for(auto i=profiles.begin();i!=profiles.end();++i){need(i.key()==i->id);auto o=encode(*i);if(old.contains(i.key()) && encode(old[i.key()])!=o)need(i->revision>old[i.key()].revision);a.append(o);}auto bytes=QJsonDocument(QJsonObject{{"schemaVersion",1},{"profiles",a}}).toJson(QJsonDocument::Compact);need(bytes.size()<=1024*1024);QSaveFile f(path_);need(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size() && f.commit());
}
}
