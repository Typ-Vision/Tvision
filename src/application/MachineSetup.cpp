#include "MachineSetup.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "persistence/PublicationTemplateStore.hpp"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
namespace visionflow {
namespace {
void need(bool b,const char* message){if(!b)throw std::runtime_error(message);}
QString registryPath(const QDir& d){return d.filePath("communication-identities-v4.json");}
QString name(int i){return i==0?QString::fromUtf8("Yüzey"):QString::fromUtf8("Ölçüm");}
QString path(const QDir& d,const PublicationTemplate& t){return d.filePath("publication-templates-v4/"+t.revision.toString(QUuid::WithoutBraces)+".json");}
// Preflight every existing manifest and default key before any mutation.
void inspect(const QDir& d,const IdentityCatalogue& c){
    QStringList errors;const auto manifests=PublicationTemplateStore(d.filePath("publication-templates-v4")).load(&errors);
    need(errors.isEmpty(),"Bozuk şablon kataloğu: mevcut manifest korunuyor; onarım gerekli.");
    const auto templates=MachineSetup::defaults(c.installation);
    for(const auto& t:templates){
        int matches=0;
        for(const auto& m:manifests)if(m.value.revision==t.revision || m.value.id==t.id){
            need(m.value.toJson()==t.toJson(),"Hazır şablon kimliğiyle çelişen manifest; üzerine yazılmadı.");++matches;
        }
        need(matches<=1,"Çift hazır şablon manifest kaydı; onarım gerekli.");
        if(QFileInfo::exists(path(d,t))){
            QFile f(path(d,t));need(f.open(QIODevice::ReadOnly),"Hazır şablon okunamadı.");
            need(QJsonDocument::fromJson(f.readAll()).object()==t.toJson(),"Hazır şablon dosya yolu çakışıyor.");
        }
        for(const auto& e:c.entries)if(e.identity.entity==t.id || e.identity.revision==t.revision)
            need(e.identity==CatalogueIdentity{CatalogueCategory::TemplateRevision,t.id,t.revision,{}} && !e.retired,"Hazır şablon katalog kaydı çelişkili veya retired; onarım gerekli.");
    }
}
IdentityCatalogue load(const QDir& d){
    try{return JsonStationIdentityStore(registryPath(d)).load();}
    catch(const std::exception&){throw std::runtime_error("Bozuk veya okunamayan mevcut kimlik kataloğu: kurulum verisi korunuyor; onarım gerekli.");}
}
}
QVector<PublicationTemplate> MachineSetup::defaults(const QUuid& installation){
    QVector<PublicationTemplate> out;
    for(const auto& token:{QByteArray("visionflow.default.surface/1"),QByteArray("visionflow.default.measurement/1")}){
        const auto id=QUuid::createUuidV5(installation,token),revision=QUuid::createUuidV5(id,"revision/1");
        // Optional reserved pose slots carry local opaque contract identities;
        // no physical adapter or capability is registered or enabled.
        auto u=[&](const char* s){return QUuid::createUuidV5(id,s);};
        out.append(PublicationTemplate::create(id,revision,{u("target"),u("target-revision"),u("source"),u("source-revision"),u("adapter")}));
    }
    out[1].numbers[0].required=true;
    // Unitless by default: a pixel measurement must never imply millimetres.
    out[1].numbers[0].name="Measurement";
    return out;
}
IdentityCatalogue MachineSetup::start(const std::function<void(const QString&)>& boundary) const {
    need(!directory_.isEmpty(),"Makine veri dizini bağlanmadı.");const QDir d(directory_);
    need(QDir().mkpath(directory_),"Makine veri dizini oluşturulamadı.");
    QLockFile lock(d.filePath("machine-setup-v4.lock"));
    need(lock.tryLock(0),"Makine kurulumu başka bir işlemde sürüyor; yeniden deneyin.");
    JsonStationIdentityStore store(registryPath(d));
    if(!QFileInfo::exists(registryPath(d))){
        // Lost catalogue alongside historical machine data is not a clean install.
        need(!QFileInfo::exists(d.filePath("communication-stations-v4.json")) && !QFileInfo::exists(d.filePath("production")) && !QFileInfo::exists(d.filePath("migration/active.json")) && !QFileInfo::exists(d.filePath("publication-templates-v4")),"Kimlik kataloğu eksik fakat mevcut makine verisi var; onarım gerekli.");
        StationIdentityRegistry(store).initialize(QUuid::createUuid());
        if(boundary)boundary("IDENTITY_COMMITTED");
    }
    auto c=load(d);inspect(d,c);const auto templates=defaults(c.installation);
    need(QDir().mkpath(d.filePath("publication-templates-v4")),"Şablon dizini oluşturulamadı.");
    for(int i=0;i<templates.size();++i){const auto& t=templates[i];
        bool exists=false;for(const auto& m:PublicationTemplateStore(d.filePath("publication-templates-v4")).load())exists|=m.value.revision==t.revision;
        if(!exists){QSaveFile f(path(d,t));f.setDirectWriteFallback(false);const auto bytes=QJsonDocument(t.toJson()).toJson();need(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size(),"Şablon yazılamadı.");if(boundary)boundary("MANIFEST_PREPARED");need(f.commit(),"Şablon atomik kaydı başarısız.");if(boundary)boundary("MANIFEST_COMMITTED");}
        c=store.load();StationIdentityRegistry registry(store);
        bool bound=false;for(const auto& e:c.entries)bound|=e.identity==CatalogueIdentity{CatalogueCategory::TemplateRevision,t.id,t.revision,{}};
        if(!bound)registry.bind(c.revision,{CatalogueCategory::TemplateRevision,t.id,t.revision,{}},name(i));
        if(boundary)boundary("KEY_COMMITTED");
    }
    return store.load();
}
QString MachineSetup::status() const {
    try{const QDir d(directory_);if(!QFileInfo::exists(registryPath(d)))return "Kurulum yapılmadı. Kurulumu başlatın.";
        const auto c=load(d);inspect(d,c);int ready=0;
        for(const auto& t:defaults(c.installation)){
            bool manifest=false;for(const auto& m:PublicationTemplateStore(d.filePath("publication-templates-v4")).load())manifest|=m.value.revision==t.revision;
            bool bound=false;for(const auto& e:c.entries)bound|=e.identity==CatalogueIdentity{CatalogueCategory::TemplateRevision,t.id,t.revision,{}};
            ready+=manifest && bound;
        }
        return QString("%1\nKurulum kimliği: %2").arg(ready==2?"Hazır: Yüzey ve Ölçüm şablonları kullanılabilir.":"Kurulum tamamlanmadı. Kurulumu başlat ile güvenle sürdürün.",c.installation.toString(QUuid::WithoutBraces));
    }catch(const std::exception& e){return QString::fromUtf8(e.what());}
}
}
