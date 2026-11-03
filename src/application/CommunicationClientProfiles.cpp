#include "CommunicationClientProfiles.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>

namespace visionflow {
namespace {
constexpr qsizetype maxProfileBytes=64*1024;
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
QString hex(const QUuid& id){return QString::fromLatin1(id.toRfc4122().toHex());}
QUuid uuid(const QJsonValue& v){
    need(v.isString(),"İstemci profili: UUID alanı metin olmalı");
    const auto text=v.toString();const auto bytes=QByteArray::fromHex(text.toLatin1());
    need(text.size()==32 && bytes.size()==16 && bytes.toHex()==text.toLatin1(),"İstemci profili: geçersiz UUID");
    const auto id=QUuid::fromRfc4122(bytes);need(!id.isNull(),"İstemci profili: sıfır UUID kullanılamaz");return id;
}
void validateLabel(const QString& label){
    need(!label.isEmpty() && !label.contains(QChar::Null) && label.toUtf8().size()<=64 && QString::fromUtf8(label.toUtf8())==label,"İstemci adı 1–64 byte UTF-8 olmalı");
}
void validateConnection(const QString& address,quint16 port){
    need(!address.isEmpty() && address.size()<=255 && address.trimmed()==address && !address.contains(QChar::Null),"İstemci profili: geçerli dinleme adresi gerekir");
    need(port!=0,"İstemci profili: port 1–65535 olmalı");
}
}
QString communicationClientProfileSchema(){return "visionflow.communication-client-profile/1";}
QByteArray exportCommunicationClientProfile(const IdentityCatalogue& cat,const CommunicationClientProfile& p){
    validateCommunicationClientProfile(p,cat);
    validateLabel(p.label);validateConnection(p.address,p.port);
    need(p.contractRevision==CommunicationPeerSessionService::contractRevision(),"İstemci profili: sözleşme revizyonu uyuşmuyor");
    const QJsonObject object{{"schema",communicationClientProfileSchema()},{"label",p.label},
        {"installation",hex(p.installation)},{"peer",hex(p.peer)},{"epoch",hex(p.epoch)},
        {"address",p.address},{"port",int(p.port)},{"contractRevision",hex(p.contractRevision)}};
    const auto bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);
    need(bytes.size()<=maxProfileBytes,"İstemci profili boyut sınırını aşıyor");return bytes;
}
CommunicationClientProfile readCommunicationClientProfile(const QByteArray& bytes){
    need(!bytes.isEmpty() && bytes.size()<=maxProfileBytes,"İstemci profili okunamadı (64 KiB sınırı)");
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);
    need(error.error==QJsonParseError::NoError && doc.isObject(),"İstemci profili geçerli JSON değil");
    const auto o=doc.object();
    need(o.size()==8,"İstemci profili: bilinmeyen veya eksik alan");
    need(o["schema"].isString() && o["schema"].toString()==communicationClientProfileSchema(),"İstemci profili şeması desteklenmiyor");
    need(o["label"].isString() && o["address"].isString() && o["port"].isDouble(),"İstemci profili: alan türü geçersiz");
    CommunicationClientProfile p;
    p.label=o["label"].toString();p.address=o["address"].toString();
    const auto port=o["port"].toDouble();need(port>=1 && port<=65535 && port==double(int(port)),"İstemci profili: port 1–65535 olmalı");
    p.port=quint16(port);
    p.installation=uuid(o["installation"]);p.peer=uuid(o["peer"]);p.epoch=uuid(o["epoch"]);
    p.contractRevision=uuid(o["contractRevision"]);
    validateLabel(p.label);validateConnection(p.address,p.port);
    need(p.contractRevision==CommunicationPeerSessionService::contractRevision(),"İstemci profili: sözleşme revizyonu uyuşmuyor");
    return p;
}
void validateCommunicationClientProfile(const CommunicationClientProfile& p,const IdentityCatalogue& cat){
    validateIdentityCatalogue(cat);
    need(p.installation==cat.installation,"İstemci profili başka makine kurulumuna ait; profil bu makinede kullanılamaz");
    const PeerEpochIdentity* active=nullptr;int activeCount=0;bool known=false,retired=false;
    for(const auto& e:cat.epochs){
        if(e.peer==p.peer && e.epoch==p.epoch){known=true;retired=e.retired;}
        if(!e.retired){++activeCount;active=&e;}
    }
    need(activeCount==1 && active,"Tek etkin kontrol kimliği yok; istemci kaydı gerekli");
    if(retired)throw std::runtime_error("İstemci profili emekliye ayrılmış epoch taşıyor; güncel profil dışa aktarın");
    need(known,"İstemci profili güncel katalogda kayıtlı değil");
    need(active->peer==p.peer && active->epoch==p.epoch,"İstemci profili etkin kontrol kimliğiyle eşleşmiyor; güncel profili kullanın");
}
}
