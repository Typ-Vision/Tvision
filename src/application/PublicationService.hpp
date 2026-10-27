#pragma once
#include "domain/PublicationConfig.hpp"
#include "CommunicationProfiles.hpp"
#include <memory>
namespace visionflow {
struct PublicationIntent {
    QString id,groupId,profileId,cycleId,configId,payloadSchema;
    int contractRevision=0;bool required=true;
    QByteArray profileDependencyId;
    QMap<QString,AssignmentValue> fields;
};
struct PublicationSnapshot {
    QMap<QString,EndpointProfile> profiles;
    QByteArray definitionId,dependencyId;
};
class PublicationService {
public:
    // Resolves local profiles without connecting. The returned copy is frozen.
    static std::shared_ptr<const PublicationSnapshot> capture(const QJsonObject&,const CommunicationProfileStore&);
    static QJsonObject dependency(const QJsonObject&,const std::shared_ptr<const PublicationSnapshot>&);
    static QByteArray definitionId(const QJsonObject&);
    static void requireCurrent(const PublicationSnapshot&,const QJsonObject&,const CommunicationProfileStore&);
    static QVector<PublicationIntent> evaluate(const PublishConfig&,const QJsonObject&,
        const QMap<QString,AssignmentBundle>&,const QString& cycleId,const QString& configId,
        const PublicationSnapshot&);
};
}
