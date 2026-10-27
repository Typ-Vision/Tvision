#include "PublicationService.hpp"
#include "AssignmentEvaluator.hpp"
#include "domain/ToolDefinition.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <stdexcept>
namespace visionflow {
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QByteArray digest(const QJsonObject& o){return QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();}
QSet<QString> active(const QJsonObject& d){QSet<QString> ids;for(const auto& t:editorTools(d))if(t.enabled && t.kind=="publish_data")for(const auto& id:decodePublishConfig(t.parameters).groups)ids.insert(id);return ids;}
}
QByteArray PublicationService::definitionId(const QJsonObject& d){return digest(d);}
std::shared_ptr<const PublicationSnapshot> PublicationService::capture(const QJsonObject& d,const CommunicationProfileStore& store){
    validatePublicationBindings(d);auto s=std::make_shared<PublicationSnapshot>();s->definitionId=definitionId(d);const auto profiles=store.load();const auto ids=active(d);QJsonObject dependencies;
    for(const auto& g:recipeOutputGroups(d))if(ids.contains(g.id)){
        need(profiles.contains(g.profileId),"Publication profile unresolved");const auto p=profiles[g.profileId];validateEndpointProfile(p);need(p.enabled,"Publication profile disabled");need(p.contractRevision==g.contractRevision,"Publication profile contract revision mismatch");
        for(const auto& f:g.fields)need(p.fieldMapping.contains(f.name),"Publication physical field mapping unresolved");s->profiles[p.id]=p;dependencies[p.id]=QString::fromLatin1(endpointDependencyId(p));
    }s->dependencyId=digest(dependencies);return s;
}
QJsonObject PublicationService::dependency(const QJsonObject& d,const std::shared_ptr<const PublicationSnapshot>& s){
    if(active(d).isEmpty())return {};
    need(bool(s) && s->definitionId==definitionId(d),"Publication profile unresolved or snapshot changed");
    QJsonObject deps;for(const auto& g:recipeOutputGroups(d))if(active(d).contains(g.id)){
        need(s->profiles.contains(g.profileId),"Publication profile unresolved");const auto& p=s->profiles[g.profileId];validateEndpointProfile(p);need(p.enabled && p.contractRevision==g.contractRevision,"Publication profile disabled/contract mismatch");for(const auto& f:g.fields)need(p.fieldMapping.contains(f.name),"Publication mapping unresolved");deps[p.id]=QString::fromLatin1(endpointDependencyId(p));
    }need(digest(deps)==s->dependencyId,"Publication dependency hash mismatch");return {{"schemaVersion",1},{"dependencyId",QString::fromLatin1(s->dependencyId)}};
}
void PublicationService::requireCurrent(const PublicationSnapshot& s,const QJsonObject& d,const CommunicationProfileStore& store){const auto current=capture(d,store);need(s.definitionId==current->definitionId && s.dependencyId==current->dependencyId,"Publication profile changed; Stop and reload required");}
QVector<PublicationIntent> PublicationService::evaluate(const PublishConfig& config,const QJsonObject& d,const QMap<QString,AssignmentBundle>& bundles,const QString& cycle,const QString& configId,const PublicationSnapshot& snapshot){
    // Verify the entire frozen mapping contract, including profile revisions.
    dependency(d,std::make_shared<const PublicationSnapshot>(snapshot));
    validatePublicationBindings(d);need(snapshot.definitionId==definitionId(d) && !cycle.isEmpty() && !configId.isEmpty(),"Publication snapshot/cycle mismatch");decodePublishConfig(encodePublishConfig(config));QVector<PublicationIntent> intents;QSet<QString> found;
    for(const auto& g:recipeOutputGroups(d))if(config.groups.contains(g.id)){
        need(snapshot.profiles.contains(g.profileId),"Publication profile unresolved");const auto& profile=snapshot.profiles[g.profileId];validateEndpointProfile(profile);need(profile.enabled && profile.contractRevision==g.contractRevision,"Publication profile disabled/contract mismatch");
        need(bundles.contains(g.producerId),"Publication bundle missing");const auto& b=bundles[g.producerId];need(b.committed && b.producerId==g.producerId && b.revision==g.producerRevision && b.cycleId==cycle && b.configId==configId,"Publication bundle dependency mismatch");
        AssignmentInputView input;input.cycleId=cycle;input.configId=configId;input.variables=recipeVariables(d);input.bundles=bundles;
        AssignmentConfig checks{g.producerId,g.producerRevision,{}};QSet<QString> checked;
        for(const auto& f:g.fields)if(!checked.contains(f.fieldId)){checked.insert(f.fieldId);checks.rows.append({f.fieldId,BundleAssignmentSource{g.producerId,f.fieldId,g.producerRevision}});}
        need(AssignmentEvaluator::evaluate(checks,input).bundle.has_value(),"Publication field type/unit/frame/validity dependency mismatch");
        PublicationIntent intent;intent.groupId=g.id;intent.profileId=g.profileId;intent.cycleId=cycle;intent.configId=configId;intent.payloadSchema=g.payloadSchema;intent.contractRevision=g.contractRevision;intent.required=g.required;intent.profileDependencyId=endpointDependencyId(profile);QJsonObject payload;
        for(const auto& f:g.fields){need(profile.fieldMapping.contains(f.name) && b.fields.contains(f.fieldId),"Publication field unresolved");const auto& value=b.fields[f.fieldId];std::visit([&](const auto& e){need(e.validity==ValueValidity::Valid && e.cycleId==cycle && e.configId==configId && e.producerId==g.producerId && e.producerPort==f.fieldId,"Publication invalid/stale field");},value);intent.fields[f.name]=value;payload[f.name]=encodeAssignmentValue(value);}
        intent.id=QString::fromLatin1(digest({{"cycleId",cycle},{"configId",configId},{"groupId",g.id},{"dependencyId",QString::fromLatin1(intent.profileDependencyId)}}));
        const QJsonObject packet{{"id",intent.id},{"groupId",g.id},{"profileId",g.profileId},{"cycleId",cycle},{"configId",configId},{"payloadSchema",g.payloadSchema},{"contractRevision",g.contractRevision},{"required",g.required},{"profileDependencyId",QString::fromLatin1(intent.profileDependencyId)},{"fields",payload}};
        need(QJsonDocument(packet).toJson(QJsonDocument::Compact).size()<=maxPublicationBytes,"Publication payload exceeds 64 KiB");intents.append(intent);found.insert(g.id);
    }need(found.size()==config.groups.size(),"Publication group unresolved");return intents;
}
}
