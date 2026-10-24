#include "application/EditorObservation.hpp"
#include "ProjectRepository.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok, const QString& message) {
    if (!ok) throw std::runtime_error(message.toUtf8().constData());
}
QString stringField(const QJsonObject& object, const char* key) {
    require(object.value(key).isString(),QString("Eksik veya hatalı metin alanı: %1").arg(key));
    return object.value(key).toString();
}
int integerField(const QJsonObject& object, const char* key) {
    const auto value = object.value(key);
    require(value.isDouble() && value.toDouble()>=1 && value.toDouble()<1000000000 &&
            value.toDouble()==value.toInt(),QString("Hatalı tamsayı alanı: %1").arg(key));
    return value.toInt();
}
QDateTime dateField(const QJsonObject& object, const char* key) {
    const auto text = stringField(object,key);
    require(text.endsWith('Z'),QString("UTC tarihi gerekli: %1").arg(key));
    auto date = QDateTime::fromString(text,Qt::ISODateWithMs);
    require(date.isValid(),QString("Hatalı tarih: %1").arg(key));
    return date.toUTC();
}
QString utc(const QDateTime& date) { return date.toUTC().toString(Qt::ISODateWithMs); }
void checkKeys(const QJsonObject& object, const QSet<QString>& supported) {
    for (auto it=object.begin();it!=object.end();++it)
        require(supported.contains(it.key()),"Desteklenmeyen alan; veri kaybını önlemek için dosya reddedildi: "+it.key());
}
}
QByteArray ProjectRepository::encode(const Project& project) {
    validateProject(project);
    QJsonArray recipes;
    for (const auto& r : project.recipes) {
        QJsonObject recipe{{"id",r.id},{"name",r.name},{"description",r.description},
            {"schemaVersion",r.schemaVersion},{"recipeVersion",r.recipeVersion},
            {"applicationType",typeKey(r.applicationType)},{"status",statusKey(r.status)},
            {"cameraReference",r.cameraReference},{"calibrationReference",r.calibrationReference},
            {"createdAt",utc(r.createdAt)},{"modifiedAt",utc(r.modifiedAt)},{"definition",r.definition}};
        if(r.schemaVersion==2)recipe["calibrationBinding"]=r.calibrationBinding ? QJsonValue(r.calibrationBinding->toJson()) : QJsonValue(QJsonValue::Null);
        recipes.append(recipe);
    }
    return QJsonDocument(QJsonObject{{"format","visionflow.project"},{"schemaVersion",project.schemaVersion},
        {"id",project.id},{"name",project.name},{"description",project.description},
        {"cameraReference",project.cameraReference},{"robotType",project.robotType},
        {"communicationProtocol",project.communicationProtocol},{"calibrationReference",project.calibrationReference},
        {"createdAt",utc(project.createdAt)},{"modifiedAt",utc(project.modifiedAt)},{"recipes",recipes}}).toJson();
}
Project ProjectRepository::load(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) require(false,"Proje açılamadı: " + file.errorString());
    require(file.size()<=16*1024*1024,"Proje dosyası 16 MB sınırını aşıyor.");
    const auto bytes = file.readAll();
    require(file.error()==QFileDevice::NoError,"Proje okunamadı: " + file.errorString());
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(bytes,&error);
    require(error.error==QJsonParseError::NoError && doc.isObject(),"Geçersiz proje JSON: " + error.errorString());
    const auto object = doc.object();
    require(stringField(object,"format")=="visionflow.project","Bu dosya VisionFlow proje formatında değil.");
    Project p;
    p.schemaVersion = integerField(object,"schemaVersion");
    require(p.schemaVersion==1,"Desteklenmeyen proje şema sürümü.");
    checkKeys(object,{"format","schemaVersion","id","name","description","cameraReference","robotType",
        "communicationProtocol","calibrationReference","createdAt","modifiedAt","recipes"});
    p.id = stringField(object,"id"); p.name = stringField(object,"name"); p.description = stringField(object,"description");
    p.cameraReference = stringField(object,"cameraReference"); p.robotType = stringField(object,"robotType");
    p.communicationProtocol = stringField(object,"communicationProtocol"); p.calibrationReference = stringField(object,"calibrationReference");
    p.createdAt = dateField(object,"createdAt"); p.modifiedAt = dateField(object,"modifiedAt");
    require(object.value("recipes").isArray(),"recipes bir dizi olmalı.");
    for (const auto value : object.value("recipes").toArray()) {
        require(value.isObject(),"Hatalı reçete nesnesi."); const auto r = value.toObject(); Recipe recipe;
        checkKeys(r,{"id","name","description","schemaVersion","recipeVersion","applicationType","status",
            "cameraReference","calibrationReference","createdAt","modifiedAt","definition","calibrationBinding"});
        recipe.id=stringField(r,"id"); recipe.name=stringField(r,"name"); recipe.description=stringField(r,"description");
        recipe.schemaVersion=integerField(r,"schemaVersion"); recipe.recipeVersion=integerField(r,"recipeVersion");
        if(recipe.schemaVersion==1)require(!r.contains("calibrationBinding"),"Schema 1 sürüm bağlantısı içeremez.");
        else if(recipe.schemaVersion==2) {
            require(r.contains("calibrationBinding") && (r["calibrationBinding"].isNull() || r["calibrationBinding"].isObject()),"Hatalı kalibrasyon sürüm bağlantısı.");
            if(r["calibrationBinding"].isObject())recipe.calibrationBinding=CalibrationDependency::fromJson(r["calibrationBinding"].toObject());
        } else require(false,"Desteklenmeyen reçete şema sürümü.");
        recipe.applicationType=parseType(stringField(r,"applicationType")); recipe.status=parseStatus(stringField(r,"status"));
        recipe.cameraReference=stringField(r,"cameraReference"); recipe.calibrationReference=stringField(r,"calibrationReference");
        recipe.createdAt=dateField(r,"createdAt"); recipe.modifiedAt=dateField(r,"modifiedAt");
        require(r.value("definition").isObject(),"definition bir nesne olmalı."); recipe.definition=r.value("definition").toObject();
        p.recipes.append(recipe);
    }
    validateProject(p); return p;
}
void ProjectRepository::save(const QString& path, const Project& project) const {
    const auto bytes=encode(project);
    require(bytes.size()<=16*1024*1024,"Proje dosyası 16 MB sınırını aşıyor.");
    editor_observation::Scope observation(editor_observation::AtomicWrite);
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) require(false,"Proje kaydedilemiyor: " + file.errorString());
    if (file.write(bytes)!=bytes.size()) require(false,"Proje yazılamadı: " + file.errorString());
    if (!file.commit()) require(false,"Atomik kayıt tamamlanamadı: " + file.errorString());
}
}
