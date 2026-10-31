#pragma once
#include "domain/PublicationTemplate.hpp"
#include <QString>
#include <QStringList>
#include <QVector>

namespace visionflow {
// Read-only view of the machine-owned publication template manifests. Template
// revisions are prepared by the explicit migration/activation authority; the
// Editor may select them but never rewrites them.
struct PublicationTemplateManifest {
  QString path;
  PublicationTemplate value;
};
class PublicationTemplateStore {
public:
  explicit PublicationTemplateStore(QString directory)
      : directory_(std::move(directory)) {}
  QString directory() const { return directory_; }
  // Strict per-file. Invalid manifests are skipped and reported, so one bad
  // file cannot hide the remaining machine revisions.
  QVector<PublicationTemplateManifest> load(QStringList *errors = nullptr) const;
private:
  QString directory_;
};
} // namespace visionflow
