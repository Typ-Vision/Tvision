#pragma once
#include "domain/Project.hpp"
#include "domain/PublicationTemplate.hpp"
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVector>
#include <functional>

namespace visionflow {
// Immutable machine-catalogue record of one published recipe revision. The
// embedded recipe is the exact saved content; station-specific camera/source
// resolution is deliberately absent (VF-04 owns it).
struct PublishedRecipe {
  quint32 recipeKey = 0;
  QUuid revision;
  quint32 templateKey = 0;
  QUuid recipeId;
  QString projectDirectory;
  Recipe recipe;
  QByteArray contentDigest; // lowercase SHA256 hex of recipe+template identity
  QDateTime publishedAt;
};

// One checksummed, atomically replaced catalogue file. A missing file is an
// empty catalogue; corruption is reported and never silently replaced.
class PublicationCatalogueStore {
public:
  explicit PublicationCatalogueStore(QString machineDirectory)
      : machineDirectory_(std::move(machineDirectory)) {}
  QString path() const;
  QVector<PublishedRecipe> load(QStringList *errors = nullptr) const;
  QUuid revision(QStringList *errors = nullptr) const;
  // Exact canonical content identity; template revision is part of the
  // contract, so a template change always yields a new publication revision.
  static QByteArray contentDigest(const Recipe &, const QUuid &templateId,
                                  const QUuid &templateRevision);
  static QByteArray encode(const QVector<PublishedRecipe> &,
                           const QUuid &installation, const QUuid &revision);
  static QVector<PublishedRecipe> decode(const QByteArray &, QUuid *installation,
                                         QUuid *revision);
  static QByteArray recipeBytes(const Recipe &);
  static Recipe decodeRecipe(const QByteArray &);

private:
  QString machineDirectory_;
};

class PublicationPublisher {
public:
  using WriteBoundary = std::function<void(const QString &)>;
  explicit PublicationPublisher(QString machineDirectory,
                                WriteBoundary boundary = {})
      : machineDirectory_(std::move(machineDirectory)),
        boundary_(std::move(boundary)) {}
  struct Result {
    PublishedRecipe publication;
    bool created = false; // false: identical content already published
  };
  // The saved project file is authoritative. When candidate is supplied it must
  // match the exact saved recipe bytes, so an unsaved editor draft is rejected.
  Result publish(const QString &projectPath, const QString &recipeId,
                 const QString &templateDirectory,
                 const Recipe *candidate = nullptr);

private:
  QString machineDirectory_;
  WriteBoundary boundary_;
};
} // namespace visionflow
