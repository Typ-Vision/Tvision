#pragma once
#include "AssignmentConfig.hpp"

namespace visionflow {
struct PublicationField {QString name,fieldId;};
struct OutputGroup {
    QString id,profileId;
    int contractRevision=0;
    bool required=true;
    QString payloadSchema="vf.fields/1",producerId;
    int producerRevision=0;
    QVector<PublicationField> fields;
};
struct PublishConfig {QStringList groups;};
constexpr int maxOutputGroups=16,maxPublicationFields=64,maxPublicationBytes=65536;
QJsonObject encodeOutputGroup(const OutputGroup&);
OutputGroup decodeOutputGroup(const QJsonObject&);
QVector<OutputGroup> recipeOutputGroups(const QJsonObject&);
QJsonObject withRecipeOutputGroups(QJsonObject,const QVector<OutputGroup>&);
QJsonObject encodePublishConfig(const PublishConfig&);
PublishConfig decodePublishConfig(const QJsonObject&);
void validatePublicationBindings(const QJsonObject&);
}
