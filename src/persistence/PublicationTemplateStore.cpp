#include "PublicationTemplateStore.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <algorithm>

namespace visionflow {
QVector<PublicationTemplateManifest>
PublicationTemplateStore::load(QStringList *errors) const {
  QVector<PublicationTemplateManifest> out;
  const QDir directory(directory_);
  const auto files =
      directory.entryList({"*.json"}, QDir::Files, QDir::Name);
  for (const auto &name : files) {
    const QString path = directory.filePath(name);
    try {
      if (QFileInfo(path).isSymLink())
        throw std::runtime_error("Symbolic link manifest rejected");
      QFile file(path);
      if (!file.open(QIODevice::ReadOnly) || file.size() > 20 * 1024 * 1024)
        throw std::runtime_error("Manifest unreadable or too large");
      QJsonParseError parse;
      const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
      if (parse.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error("Invalid manifest JSON");
      out.append({path, PublicationTemplate::fromJson(document.object())});
    } catch (const std::exception &e) {
      if (errors) errors->append(QFileInfo(path).fileName() + ": " +
                                 QString::fromUtf8(e.what()));
    }
  }
  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    return a.value.id.toString() == b.value.id.toString()
               ? a.value.revision.toString() < b.value.revision.toString()
               : a.value.id.toString() < b.value.id.toString();
  });
  return out;
}
} // namespace visionflow
