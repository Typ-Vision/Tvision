#pragma once
#include "domain/PublicationTemplate.hpp"
#include "domain/StationIdentityRegistry.hpp"
#include <functional>
namespace visionflow {
// Explicit machine enrollment. Deterministic per-installation defaults allow
// recovery at every atomic write boundary without a second journal authority.
class MachineSetup final {
public:
    explicit MachineSetup(QString directory):directory_(std::move(directory)){}
    IdentityCatalogue start(const std::function<void(const QString&)>& boundary={}) const;
    QString status() const;
    static QVector<PublicationTemplate> defaults(const QUuid& installation);
private:
    QString directory_;
};
}
