#include "PublicationPublisher.hpp"
#include "persistence/ProjectRepository.hpp"
#include "persistence/PublicationTemplateStore.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "FixedSlotPublication.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLockFile>
#include <QSaveFile>
#include <QSet>
#include <limits>
#include <stdexcept>

namespace visionflow {
namespace {
constexpr qint64 maxCatalogueBytes = 32 * 1024 * 1024;
void need(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
QString hexUuid(const QUuid &id) {
  return QString::fromLatin1(id.toRfc4122().toHex());
}
QUuid uuidValue(const QJsonValue &value, const char *what) {
  need(value.isString(), what);
  const auto hex = value.toString().toLatin1();
  const auto bytes = QByteArray::fromHex(hex);
  need(hex.size() == 32 && bytes.size() == 16 && bytes.toHex() == hex, what);
  return QUuid::fromRfc4122(bytes);
}
qint64 number(const QJsonValue &value, const char *what) {
  need(value.isDouble(), what);
  const auto n = value.toDouble();
  need(n >= 1 && n <= std::numeric_limits<quint32>::max() && n == qint64(n), what);
  return qint64(n);
}
QString text(const QJsonValue &value, const char *what, bool allowEmpty = false) {
  need(value.isString(), what);
  const auto result = value.toString();
  need(allowEmpty || !result.isEmpty(), what);
  return result;
}
QString utc(const QDateTime &date) {
  return date.toUTC().toString(Qt::ISODateWithMs);
}
QDateTime date(const QJsonValue &value, const char *what) {
  const auto result = QDateTime::fromString(text(value, what), Qt::ISODateWithMs);
  need(result.isValid() && result.timeSpec() == Qt::UTC, what);
  return result;
}
void keys(const QJsonObject &object, const QSet<QString> &allowed,
          const char *what) {
  need(object.size() == allowed.size(), what);
  for (auto i = object.begin(); i != object.end(); ++i)
    need(allowed.contains(i.key()), what);
}
QByteArray compact(const QJsonObject &object) {
  return QJsonDocument(object).toJson(QJsonDocument::Compact);
}
QByteArray sha(const QByteArray &bytes) {
  return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}
// Station catalogue names are bounded by the frozen W6 contract.
QString catalogueName(const QString &name) {
  QString result;
  for (const auto codepoint : name) {
    if ((result + codepoint).toUtf8().size() > 64) break;
    result.append(codepoint);
  }
  return result.trimmed().isEmpty() ? QString::fromUtf8("Reçete") : result;
}
quint32 catalogueKey(const IdentityCatalogue &catalogue,
                     CatalogueCategory category, const QUuid &entity,
                     const QUuid &revision) {
  for (const auto &entry : catalogue.entries)
    if (!entry.retired && entry.identity.category == category &&
        entry.identity.entity == entity && entry.identity.revision == revision)
      return entry.key;
  throw std::runtime_error("Şablon veya reçete için makine katalog anahtarı yok.");
}
QJsonObject recipeJson(const Recipe &recipe) {
  QJsonObject result{{"id", recipe.id},
                     {"name", recipe.name},
                     {"description", recipe.description},
                     {"schemaVersion", recipe.schemaVersion},
                     {"recipeVersion", recipe.recipeVersion},
                     {"applicationType", typeKey(recipe.applicationType)},
                     {"status", statusKey(recipe.status)},
                     {"cameraReference", recipe.cameraReference},
                     {"calibrationReference", recipe.calibrationReference},
                     {"createdAt", utc(recipe.createdAt)},
                     {"modifiedAt", utc(recipe.modifiedAt)},
                     {"definition", recipe.definition}};
  if (recipe.schemaVersion == 2)
    result["calibrationBinding"] =
        recipe.calibrationBinding ? QJsonValue(recipe.calibrationBinding->toJson())
                                  : QJsonValue(QJsonValue::Null);
  return result;
}
QJsonObject publicationJson(const PublishedRecipe &publication) {
  return QJsonObject{{"recipeKey", qint64(publication.recipeKey)},
                     {"revision", hexUuid(publication.revision)},
                     {"templateKey", qint64(publication.templateKey)},
                     {"recipeId", hexUuid(publication.recipeId)},
                     {"projectDirectory", publication.projectDirectory},
                     {"contentDigest", QString::fromLatin1(publication.contentDigest)},
                     {"publishedAt", utc(publication.publishedAt)},
                     {"recipe", recipeJson(publication.recipe)}};
}
PublishedRecipe readPublication(const QJsonObject &object) {
  keys(object,
       {"recipeKey", "revision", "templateKey", "recipeId", "projectDirectory",
        "contentDigest", "publishedAt", "recipe"},
       "Yayın kaydı alanları hatalı.");
  PublishedRecipe publication;
  publication.recipeKey = quint32(number(object["recipeKey"], "recipeKey"));
  publication.templateKey = quint32(number(object["templateKey"], "templateKey"));
  publication.revision = uuidValue(object["revision"], "publication revision");
  publication.recipeId = uuidValue(object["recipeId"], "recipe id");
  publication.projectDirectory = text(object["projectDirectory"], "project directory");
  publication.contentDigest = text(object["contentDigest"], "content digest").toLatin1();
  need(publication.contentDigest.size() == 64, "content digest length");
  publication.publishedAt = date(object["publishedAt"], "publishedAt");
  need(object["recipe"].isObject(), "recipe object required");
  publication.recipe = PublicationCatalogueStore::decodeRecipe(
      compact(object["recipe"].toObject()));
  const auto assignments = recipePublicationAssignments(publication.recipe.definition);
  need(bool(assignments), "Published recipe has no slot assignment.");
  need(PublicationCatalogueStore::contentDigest(
           publication.recipe, assignments->templateId,
           assignments->templateRevision) == publication.contentDigest,
       "Published content digest mismatch.");
  need(QUuid(publication.recipe.id) == publication.recipeId,
       "Published recipe id mismatch.");
  return publication;
}
QJsonObject payload(const QVector<PublishedRecipe> &publications,
                    const QUuid &installation, const QUuid &revision) {
  QJsonArray rows;
  for (const auto &publication : publications)
    rows.append(publicationJson(publication));
  return QJsonObject{{"schemaVersion", 1},
                     {"installation", hexUuid(installation)},
                     {"catalogueRevision", hexUuid(revision)},
                     {"publications", rows}};
}
struct Decoded {
  QVector<PublishedRecipe> publications;
  QUuid installation, revision;
};
Decoded decodeCatalogue(const QByteArray &bytes) {
  need(bytes.size() <= maxCatalogueBytes, "Yayın kataloğu boyut sınırını aşıyor.");
  QJsonParseError error;
  const auto document = QJsonDocument::fromJson(bytes, &error);
  need(error.error == QJsonParseError::NoError && document.isObject(),
       "Yayın kataloğu JSON bozuk.");
  const auto envelope = document.object();
  keys(envelope, {"payload", "sha256"}, "Yayın kataloğu zarfı hatalı.");
  need(envelope["payload"].isObject() && envelope["sha256"].isString(),
       "Yayın kataloğu zarfı hatalı.");
  const auto body = envelope["payload"].toObject();
  need(envelope["sha256"].toString().toLatin1() == sha(compact(body)),
       "Yayın kataloğu checksum uyuşmuyor.");
  keys(body, {"schemaVersion", "installation", "catalogueRevision", "publications"},
       "Yayın kataloğu alanları hatalı.");
  need(body["schemaVersion"] == 1 && body["publications"].isArray(),
       "Desteklenmeyen yayın kataloğu şeması.");
  Decoded decoded;
  decoded.installation = uuidValue(body["installation"], "installation");
  decoded.revision = uuidValue(body["catalogueRevision"], "catalogue revision");
  QSet<quint32> keysUsed;
  QSet<QUuid> revisions;
  for (const auto &row : body["publications"].toArray()) {
    need(row.isObject(), "Yayın kaydı nesne olmalı.");
    auto publication = readPublication(row.toObject());
    need(!keysUsed.contains(publication.recipeKey), "Yinelenen yayın anahtarı.");
    need(!revisions.contains(publication.revision), "Yinelenen yayın revizyonu.");
    keysUsed.insert(publication.recipeKey);
    revisions.insert(publication.revision);
    decoded.publications.append(publication);
  }
  return decoded;
}
} // namespace

QString PublicationCatalogueStore::path() const {
  return QDir(machineDirectory_).filePath("publication-catalogue-v4.json");
}
QVector<PublishedRecipe> PublicationCatalogueStore::load(
    QStringList *errors) const {
  QFile file(path());
  if (!QFileInfo::exists(path())) return {};
  try {
    need(file.open(QIODevice::ReadOnly), "Yayın kataloğu okunamadı.");
    return decodeCatalogue(file.readAll()).publications;
  } catch (const std::exception &e) {
    if (errors) errors->append(QString::fromUtf8(e.what()));
    return {};
  }
}
QUuid PublicationCatalogueStore::revision(QStringList *errors) const {
  QFile file(path());
  if (!QFileInfo::exists(path())) return {};
  try {
    need(file.open(QIODevice::ReadOnly), "Yayın kataloğu okunamadı.");
    return decodeCatalogue(file.readAll()).revision;
  } catch (const std::exception &e) {
    if (errors) errors->append(QString::fromUtf8(e.what()));
    return {};
  }
}
QByteArray PublicationCatalogueStore::contentDigest(
    const Recipe &recipe, const QUuid &templateId,
    const QUuid &templateRevision) {
  return sha(compact(recipeJson(recipe)) + templateId.toRfc4122() +
             templateRevision.toRfc4122());
}
QByteArray PublicationCatalogueStore::encode(
    const QVector<PublishedRecipe> &publications, const QUuid &installation,
    const QUuid &revision) {
  need(!installation.isNull() && !revision.isNull(), "Yayın kataloğu kimliği yok.");
  const auto body = payload(publications, installation, revision);
  return compact(
      {{"payload", body}, {"sha256", QString::fromLatin1(sha(compact(body)))}});
}
QVector<PublishedRecipe> PublicationCatalogueStore::decode(
    const QByteArray &bytes, QUuid *installation, QUuid *revision) {
  const auto decoded = decodeCatalogue(bytes);
  if (installation) *installation = decoded.installation;
  if (revision) *revision = decoded.revision;
  return decoded.publications;
}
QByteArray PublicationCatalogueStore::recipeBytes(const Recipe &recipe) {
  return compact(recipeJson(recipe));
}
Recipe PublicationCatalogueStore::decodeRecipe(const QByteArray &bytes) {
  QJsonParseError error;
  const auto document = QJsonDocument::fromJson(bytes, &error);
  need(error.error == QJsonParseError::NoError && document.isObject(),
       "Reçete kaydı JSON bozuk.");
  const auto object = document.object();
  static const QSet<QString> allowed{
      "id", "name", "description", "schemaVersion", "recipeVersion",
      "applicationType", "status", "cameraReference", "calibrationReference",
      "createdAt", "modifiedAt", "definition", "calibrationBinding"};
  need(object.size() == 12 || object.size() == 13, "Reçete kaydı alanları hatalı.");
  for (auto i = object.begin(); i != object.end(); ++i)
    need(allowed.contains(i.key()), "Reçete kaydı alanları hatalı.");
  Recipe recipe;
  recipe.id = text(object["id"], "recipe id");
  recipe.name = text(object["name"], "recipe name");
  recipe.description = text(object["description"], "recipe description", true);
  need(object["schemaVersion"].isDouble() && object["schemaVersion"].toInt() >= 1 &&
           object["schemaVersion"].toInt() <= 2,
       "recipe schema");
  recipe.schemaVersion = object["schemaVersion"].toInt();
  need(object["recipeVersion"].isDouble() && object["recipeVersion"].toInt() >= 1,
       "recipe version");
  recipe.recipeVersion = object["recipeVersion"].toInt();
  recipe.applicationType = parseType(text(object["applicationType"], "application type"));
  recipe.status = parseStatus(text(object["status"], "status"));
  recipe.cameraReference = text(object["cameraReference"], "camera reference", true);
  recipe.calibrationReference =
      text(object["calibrationReference"], "calibration reference", true);
  recipe.createdAt = date(object["createdAt"], "createdAt");
  recipe.modifiedAt = date(object["modifiedAt"], "modifiedAt");
  need(object["definition"].isObject(), "recipe definition");
  recipe.definition = object["definition"].toObject();
  if (recipe.schemaVersion == 2) {
    need(object["calibrationBinding"].isNull() ||
             object["calibrationBinding"].isObject(),
         "calibration binding");
    if (object["calibrationBinding"].isObject())
      recipe.calibrationBinding =
          CalibrationDependency::fromJson(object["calibrationBinding"].toObject());
  } else
    need(object["calibrationBinding"].isNull() || !object.contains("calibrationBinding"),
         "schema1 calibration binding");
  Project project;
  project.schemaVersion = 1;
  project.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  project.name = "Published";
  project.createdAt = project.modifiedAt = QDateTime::currentDateTimeUtc();
  project.recipes.append(recipe);
  validateProject(project);
  return recipe;
}

PublicationPublisher::Result PublicationPublisher::publish(
    const QString &projectPath, const QString &recipeId,
    const QString &templateDirectory, const Recipe *candidate) {
  need(!machineDirectory_.isEmpty(), "Makine veri dizini bağlanmadı.");
  need(QFileInfo(projectPath).isAbsolute() && QFileInfo(projectPath).isFile(),
       "Yayın için kaydedilmiş proje dosyası gerekir.");
  const auto project = ProjectRepository{}.load(projectPath);
  const Recipe *saved = nullptr;
  for (const auto &recipe : project.recipes)
    if (recipe.id == recipeId) saved = &recipe;
  need(saved != nullptr, "Reçete kayıtlı projede bulunamadı.");
  if (candidate)
    need(PublicationCatalogueStore::recipeBytes(*candidate) ==
             PublicationCatalogueStore::recipeBytes(*saved),
         "Kaydedilmemiş değişiklik var: önce Save, sonra yayımla.");
  const auto assignments = recipePublicationAssignments(saved->definition);
  need(bool(assignments),
       "Publish Data slot ataması yok: hazır şablonu seçip zorunlu çıktıyı bağlayın.");
  need(!templateDirectory.isEmpty(), "Makine şablon dizini bağlanmadı.");
  const auto manifests =
      PublicationTemplateStore(templateDirectory).load();
  const PublicationTemplate *machineTemplate = nullptr;
  for (const auto &manifest : manifests)
    if (manifest.value.id == assignments->templateId &&
        manifest.value.revision == assignments->templateRevision)
      machineTemplate = &manifest.value;
  need(machineTemplate != nullptr,
       "Reçetenin bağlı olduğu makine şablon revision'ı bulunamadı.");
  // Fails closed on wrong type/unit, missing required slot, unknown source and
  // DAG cycles before any machine file is written.
  const auto bindings = publicationBindings(saved->definition);
  FixedSlotPublication::capture(*machineTemplate, *assignments, bindings,
                                "publish");

  JsonStationIdentityStore identities(
      QDir(machineDirectory_).filePath("communication-identities-v4.json"));
  auto catalogueIdentities = identities.load();
  validateIdentityCatalogue(catalogueIdentities);
  const auto templateKey =
      catalogueKey(catalogueIdentities, CatalogueCategory::TemplateRevision,
                   machineTemplate->id, machineTemplate->revision);
  const auto digest = PublicationCatalogueStore::contentDigest(
      *saved, machineTemplate->id, machineTemplate->revision);

  need(QDir().mkpath(machineDirectory_), "Makine veri dizini oluşturulamadı.");
  QLockFile lock(
      QDir(machineDirectory_).filePath("publication-catalogue-v4.lock"));
  lock.setStaleLockTime(0);
  need(lock.tryLock(0), "Yayın kataloğu başka bir işlemde kullanılıyor.");

  PublicationCatalogueStore store(machineDirectory_);
  QStringList errors;
  auto publications = store.load(&errors);
  need(errors.isEmpty(), "Yayın kataloğu bozuk; mevcut dosya korunuyor.");
  QSet<quint32> usedKeys;
  for (const auto &publication : publications) {
    usedKeys.insert(publication.recipeKey);
    if (publication.recipeId == QUuid(saved->id) &&
        publication.contentDigest == digest)
      return {publication, false};
  }

  PublishedRecipe publication;
  publication.recipeKey = 0;
  // Deterministic per installation+content so an interrupted publish resumes on
  // the same revision/key instead of leaking duplicate identity entries.
  publication.revision = QUuid::createUuidV5(
      catalogueIdentities.installation,
      QByteArray("visionflow.publication/1:") + digest);
  publication.templateKey = templateKey;
  publication.recipeId = QUuid(saved->id);
  publication.projectDirectory = QFileInfo(projectPath).absolutePath();
  publication.recipe = *saved;
  publication.contentDigest = digest;
  publication.publishedAt = QDateTime::currentDateTimeUtc();
  // Reserve the immutable catalogue key first. A crash after this point leaves
  // an unused key, never a loadable publication with an unbound identity.
  StationIdentityRegistry registry(identities);
  const auto entry = registry.bind(
      catalogueIdentities.revision,
      {CatalogueCategory::RecipeRevision, publication.recipeId,
       publication.revision, {}},
      catalogueName(saved->name));
  publication.recipeKey = entry.key;
  need(!usedKeys.contains(publication.recipeKey), "Katalog anahtarı yeniden kullanıldı.");
  publications.append(publication);
  const auto nextRevision = QUuid::createUuid();
  const auto bytes = PublicationCatalogueStore::encode(
      publications, catalogueIdentities.installation, nextRevision);
  need(bytes.size() <= maxCatalogueBytes, "Yayın kataloğu boyut sınırını aşıyor.");
  QSaveFile file(store.path());
  file.setDirectWriteFallback(false);
  need(file.open(QIODevice::WriteOnly), "Yayın kataloğu yazılamadı.");
  if (boundary_) boundary_("PREPARED");
  const auto half = bytes.size() / 2;
  need(file.write(bytes.first(half)) == half, "Yayın kataloğu yazılamadı.");
  if (boundary_) boundary_("HALF_WRITTEN");
  need(file.write(bytes.sliced(half)) == bytes.size() - half,
       "Yayın kataloğu yazılamadı.");
  if (boundary_) boundary_("WRITTEN");
  need(file.commit(), "Yayın kataloğu atomik kaydı tamamlanamadı.");
  if (boundary_) boundary_("COMMITTED");
  return {publication, true};
}
} // namespace visionflow
