#include "OutputVisibilityPreferences.hpp"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QLockFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <algorithm>

namespace visionflow {
namespace {
QString key(const OutputVisibilityIdentity& id) {
    if (id.projectId.trimmed().isEmpty() || id.recipeId.trimmed().isEmpty()
        || id.toolId.trimmed().isEmpty()) return {};
    // Encode components independently: separators/unicode cannot alias identities.
    return "outputVisibility/v1/" + QString::fromLatin1(id.projectId.toUtf8().toHex())
        + '/' + QString::fromLatin1(id.recipeId.toUtf8().toHex())
        + '/' + QString::fromLatin1(id.toolId.toUtf8().toHex());
}
QString storageError(QSettings::Status status) {
    if (status == QSettings::FormatError) return "Output tercih dosyası bozuk; kayıt yapılmadı.";
    if (status == QSettings::AccessError) return "Output tercih dosyasına erişilemiyor; kayıt doğrulanamadı.";
    return {};
}
constexpr int maxRecordBytes = 64 * 1024;
constexpr int maxPorts = 256;
// A unique staging file forces a fresh INI parse. QSettings caches parsed files
// across instances and does not retain FormatError in that shared cache.
QString stage(const QString& path, const QString& temporaryPath) {
    QFile temporary(temporaryPath);
    if (!temporary.open(QIODevice::WriteOnly)) return "Output tercih geçici dosyası açılamadı.";
    if (QFileInfo::exists(path)) {
        QFile original(path);
        if (!original.open(QIODevice::ReadOnly) || original.size() > 4 * 1024 * 1024)
            return "Output tercih dosyası okunamadı veya boyut sınırını aşıyor.";
        const auto bytes = original.readAll();
        if (original.error() != QFile::NoError || temporary.write(bytes) != bytes.size())
            return "Output tercih geçici kopyası yazılamadı.";
    }
    if (!temporary.flush()) return "Output tercih geçici kopyası yazılamadı.";
    temporary.close();
    return {};
}
}
OutputVisibilityPreferences::OutputVisibilityPreferences(QString settingsFile)
    : settingsFile_(std::move(settingsFile)) {}

OutputVisibilityPreferenceRead OutputVisibilityPreferences::load(const OutputVisibilityIdentity& id) const {
    const auto entry = key(id);
    if (entry.isEmpty()) return {{}, "Output tercihi için proje/reçete/araç kimliği gerekli.", 0};
    if (settingsFile_.trimmed().isEmpty()) return {{}, "Output tercih dosyası belirtilmedi.", 0};
    if (!QFileInfo::exists(settingsFile_)) return {};
    QTemporaryDir temporary;
    if (!temporary.isValid()) return {{}, "Output tercih geçici klasörü açılamadı.", 0};
    const auto stagedPath = temporary.filePath("preferences.ini");
    if (const auto error = stage(settingsFile_, stagedPath); !error.isEmpty()) return {{}, error, 0};
    QSettings settings(stagedPath, QSettings::IniFormat);
    settings.allKeys(); // Force parsing of every section before accepting a record.
    const auto raw = settings.value(entry);
    if (const auto error = storageError(settings.status()); !error.isEmpty()) return {{}, error, 0};
    if (!raw.isValid()) return {};
    const auto bytes = raw.toString().toUtf8();
    if (bytes.size() > maxRecordBytes) return {{}, "Output tercih kaydı boyut sınırını aşıyor.", 0};
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(bytes, &parseError);
    const auto object = document.object();
    const int version = object.value("defaultVersion").toInt(-1);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()
        || object.value("schemaVersion") != 1 || version < 1
        || version > OutputVisibilityPolicy::defaultVersion || !object.value("ports").isArray())
        return {{}, "Output tercih kaydı bozuk veya sürümü desteklenmiyor.", 0};
    const auto ports = object.value("ports").toArray();
    if (ports.size() > maxPorts) return {{}, "Output tercih port sınırı aşıldı.", 0};
    QSet<QString> selected;
    for (const auto& port : ports) {
        if (!port.isString() || port.toString().isEmpty())
            return {{}, "Output tercih port kaydı bozuk.", 0};
        selected.insert(port.toString());
    }
    return {selected, {}, version}; // Empty remains engaged. Unknown IDs reach D1.
}
OutputVisibilityPreferenceWrite OutputVisibilityPreferences::save(const OutputVisibilityIdentity& id,
    const OutputVisibilityOverride& selection) const {
    const auto entry = key(id);
    if (entry.isEmpty()) return {"Output tercihi için proje/reçete/araç kimliği gerekli."};
    if (settingsFile_.trimmed().isEmpty()) return {"Output tercih dosyası belirtilmedi."};
    QByteArray bytes;
    if (selection) {
        if (selection->size() > maxPorts) return {"Output tercih port sınırı aşıldı."};
        auto ids = selection->values();
        std::sort(ids.begin(), ids.end());
        QJsonArray ports;
        for (const auto& port : ids) {
            if (port.isEmpty()) return {"Output tercih port kimliği boş olamaz."};
            ports.append(port);
        }
        bytes = QJsonDocument(QJsonObject{{"schemaVersion", 1},
            {"defaultVersion", OutputVisibilityPolicy::defaultVersion}, {"ports", ports}})
            .toJson(QJsonDocument::Compact);
        if (bytes.size() > maxRecordBytes) return {"Output tercih kaydı boyut sınırını aşıyor."};
    }
    if (!QDir().mkpath(QFileInfo(settingsFile_).absolutePath()))
        return {"Output tercih klasörü oluşturulamadı."};
    QLockFile lock(settingsFile_ + ".lock");
    if (!lock.tryLock(0)) return {"Output tercih dosyası başka bir işlem tarafından kaydediliyor."};
    QTemporaryDir temporary;
    if (!temporary.isValid()) return {"Output tercih geçici klasörü açılamadı."};
    const auto stagedPath = temporary.filePath("preferences.ini");
    if (const auto error = stage(settingsFile_, stagedPath); !error.isEmpty()) return {error};
    QSettings settings(stagedPath, QSettings::IniFormat);
    settings.setAtomicSyncRequired(true);
    settings.allKeys();
    if (const auto error = storageError(settings.status()); !error.isEmpty()) return {error};
    if (!settings.isWritable()) return {"Output tercih dosyası yazılabilir değil."};
    if (selection) settings.setValue(entry, QString::fromUtf8(bytes));
    else settings.remove(entry);
    settings.sync();
    if (const auto error = storageError(settings.status()); !error.isEmpty()) return {error};
    QFile staged(stagedPath);
    if (!staged.open(QIODevice::ReadOnly)) return {"Output tercih geçici sonucu okunamadı."};
    const auto content = staged.readAll();
    if (staged.error() != QFile::NoError || content.size() > 4 * 1024 * 1024)
        return {"Output tercih geçici sonucu okunamadı veya boyut sınırını aşıyor."};
    QSaveFile output(settingsFile_);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly) || output.write(content) != content.size() || !output.commit())
        return {"Output tercih dosyası kaydedilemedi: " + output.errorString()};
    return {};
}
OutputVisibilityPreferenceWrite OutputVisibilityPreferences::reset(const OutputVisibilityIdentity& id) const {
    return save(id, std::nullopt);
}
OutputVisibilityPreferenceWrite OutputVisibilityPreferences::showAll(const OutputVisibilityIdentity& id,
    const QString& kind) const {
    return save(id, OutputVisibilityPolicy::allPorts(kind));
}
}
