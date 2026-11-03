#include "StationDefinition.hpp"
#include "ToolRegistry.hpp"
#include "IndustrialSourceReference.hpp"
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QJsonDocument>
#include <QSet>
#include <stdexcept>
#include <algorithm>

namespace visionflow {
namespace {
void need(bool ok) {if(!ok)throw std::runtime_error("Invalid station activation catalogue/transition");}
void setup(QDataStream& stream) {stream.setVersion(QDataStream::Qt_6_0);stream.setByteOrder(QDataStream::BigEndian);}
void writeSelection(QDataStream& out,const StationRecipeSelection& s) {
    const auto& r=s.recipe;
    out<<s.revision<<quint32(s.recipeKey)<<s.projectDirectory<<r.id<<r.name<<r.description
       <<qint32(r.applicationType)<<r.cameraReference<<r.calibrationReference
       <<QJsonDocument(r.calibrationBinding ? r.calibrationBinding->toJson():QJsonObject{}).toJson(QJsonDocument::Compact)
       <<qint32(r.schemaVersion)<<qint32(r.recipeVersion)<<qint32(r.status)<<r.createdAt<<r.modifiedAt
       <<QJsonDocument(r.definition).toJson(QJsonDocument::Compact)<<quint32(s.cameras.size());
    for(const auto& c:s.cameras)out<<c.sourceId<<c.camera<<quint32(c.cameraKey);
}
QJsonObject object(const QByteArray& bytes) {
    QJsonParseError error;auto doc=QJsonDocument::fromJson(bytes,&error);
    need(error.error==QJsonParseError::NoError && doc.isObject());return doc.object();
}
void writeResolution(QDataStream& out,const StationRecipeSelection& s) {
    out<<bool(s.resolvedSource);
    if(s.resolvedSource){const auto& r=*s.resolvedSource;out<<r.station<<r.camera<<r.revision<<r.path;}
}
StationRecipeSelection readSelection(QDataStream& in,bool extended) {
    StationRecipeSelection s;auto& r=s.recipe;quint32 key,count;qint32 type,schema,version,status;QByteArray binding,definition;
    in>>s.revision>>key>>s.projectDirectory>>r.id>>r.name>>r.description>>type>>r.cameraReference>>r.calibrationReference
      >>binding>>schema>>version>>status>>r.createdAt>>r.modifiedAt>>definition>>count;
    need(in.status()==QDataStream::Ok && count<=256 && type>=0 && type<=2 && status>=0 && status<=2);
    s.recipeKey=key;r.applicationType=ApplicationType(type);r.schemaVersion=schema;r.recipeVersion=version;r.status=RecipeStatus(status);
    const auto b=object(binding);if(!b.isEmpty())r.calibrationBinding=CalibrationDependency::fromJson(b);
    r.definition=object(definition);
    for(quint32 n=0;n<count;++n){StationCameraBinding c;in>>c.sourceId>>c.camera>>key;c.cameraKey=key;s.cameras.append(c);}
    if(extended){bool present=false;in>>present;if(present){StationFileSource r;in>>r.station>>r.camera>>r.revision>>r.path;s.resolvedSource=r;}}
    return s;
}
QByteArray snapshotBytes(const StationSnapshot& s,bool extended=false) {
    QByteArray bytes;QDataStream out(&bytes,QIODevice::WriteOnly);setup(out);
    out<<s.station.id<<quint32(s.station.key)<<s.station.name<<s.activation;writeSelection(out,s.selection);if(extended || s.selection.resolvedSource)writeResolution(out,s.selection);return bytes;
}
}
QByteArray stationRecipeBytes(const StationRecipeSelection& s) {
    QByteArray bytes;QDataStream out(&bytes,QIODevice::WriteOnly);setup(out);writeSelection(out,s);if(s.resolvedSource)writeResolution(out,s);return bytes;
}
QByteArray stationPublicationBytes(const StationRecipeSelection& s) {
    auto copy=s;copy.cameras.clear();copy.resolvedSource.reset();return stationRecipeBytes(copy);
}
bool stationManifestMatches(const StationRecipeSelection& saved,const StationRecipeSelection& selected) {
    return selected.resolvedSource ? stationPublicationBytes(saved)==stationPublicationBytes(selected)
        : stationRecipeBytes(saved)==stationRecipeBytes(selected);
}
Recipe stationExecutionRecipe(const StationRecipeSelection& s) {
    if(!s.resolvedSource)return s.recipe;
    const auto& source=*s.resolvedSource;
    need(!source.station.isNull() && !source.camera.isNull() && !source.revision.isNull()
        && QDir::isAbsolutePath(source.path) && s.cameras.size()==1 && s.cameras[0].camera==source.camera);
    auto recipe=s.recipe;auto tools=editorTools(recipe.definition);int count=0;
    for(auto& tool:tools)if(tool.enabled && ToolRegistry::get(tool.kind).acquisitionSource){
        need(tool.kind=="image_file" && tool.id==s.cameras[0].sourceId);
        tool.parameters["path"]=source.path;++count;
    }
    need(count==1);recipe.definition=withEditorTools(recipe.definition,tools);return recipe;
}
void validateStationCatalogue(const StationActivationCatalogue& c) {
    need(!c.installation.isNull() && !c.revision.isNull() && c.history.size()<=4096);
    QSet<QUuid> activations,stations;QMap<QUuid,std::uint32_t> keys;QMap<std::uint32_t,QUuid> owners;
    QMap<QUuid,QByteArray> revisions,legacyBindings;
    for(const auto& s:c.history) {
        need(!s.station.id.isNull() && s.station.key && !s.station.name.trimmed().isEmpty() && !s.activation.isNull() && !activations.contains(s.activation));
        activations.insert(s.activation);stations.insert(s.station.id);
        need(stations.size()<=32 && (!keys.contains(s.station.id) || keys[s.station.id]==s.station.key) && (!owners.contains(s.station.key) || owners[s.station.key]==s.station.id));
        keys[s.station.id]=s.station.key;owners[s.station.key]=s.station.id;
        const auto& r=s.selection;
        need(!r.revision.isNull() && r.recipeKey && !QUuid(r.recipe.id).isNull() && r.recipe.recipeVersion>0 && (r.recipe.schemaVersion==1 || r.recipe.schemaVersion==2));
        need(!r.projectDirectory.isEmpty() && QDir::isAbsolutePath(r.projectDirectory) && r.cameras.size()<=256);
        need(int(r.recipe.applicationType)>=0 && int(r.recipe.applicationType)<=2 && int(r.recipe.status)>=0 && int(r.recipe.status)<=2);
        need(r.recipe.createdAt.isValid() && r.recipe.modifiedAt.isValid() && (r.recipe.schemaVersion==2 || !r.recipe.calibrationBinding));
        QSet<QString> sources;
        for(const auto& b:r.cameras){need(!b.sourceId.trimmed().isEmpty() && !sources.contains(b.sourceId) && !b.camera.isNull() && b.cameraKey);sources.insert(b.sourceId);}
        QSet<QString> required;
        for(const auto& tool:editorTools(r.recipe.definition))if(tool.enabled && ToolRegistry::get(tool.kind).acquisitionSource) {
            required.insert(tool.id);
            if(tool.kind=="industrial_camera") {
                const auto ref=IndustrialSourceReference::fromJson(tool.parameters["sourceReference"].toObject());
                for(const auto& b:r.cameras)if(b.sourceId==tool.id)need(b.camera==QUuid(ref.camera.value));
            }
        }
        need(sources==required && !required.isEmpty());
        const auto bytes=stationPublicationBytes(r);
        need(!revisions.contains(r.revision) || revisions[r.revision]==bytes);revisions[r.revision]=bytes;
        if(r.resolvedSource){need(r.resolvedSource->station==s.station.id);stationExecutionRecipe(r);}
        else {const auto legacy=stationRecipeBytes(r);need(!legacyBindings.contains(r.revision) || legacyBindings[r.revision]==legacy);legacyBindings[r.revision]=legacy;}
    }
}
QByteArray encodeStationCatalogue(const StationActivationCatalogue& c) {
    validateStationCatalogue(c);QByteArray bytes;QDataStream out(&bytes,QIODevice::WriteOnly);setup(out);
    const bool extended=std::any_of(c.history.cbegin(),c.history.cend(),[](const auto& s){return bool(s.selection.resolvedSource);});
    out<<QByteArray("visionflow.station-activations")<<quint32(extended?2:1)<<c.installation<<c.revision<<quint32(c.history.size());
    for(const auto& s:c.history)out<<snapshotBytes(s,extended);
    need(out.status()==QDataStream::Ok && bytes.size()<=16*1024*1024);return bytes;
}
StationActivationCatalogue decodeStationCatalogue(const QByteArray& bytes) {
    need(bytes.size()<=16*1024*1024);QDataStream in(bytes);setup(in);QByteArray magic;quint32 schema,count;StationActivationCatalogue c;
    in>>magic>>schema>>c.installation>>c.revision>>count;
    need(in.status()==QDataStream::Ok && magic=="visionflow.station-activations" && (schema==1 || schema==2) && count<=4096);
    for(quint32 n=0;n<count;++n){QByteArray bytes;in>>bytes;QDataStream entry(bytes);setup(entry);StationSnapshot s;quint32 key;
        entry>>s.station.id>>key>>s.station.name>>s.activation;s.station.key=key;s.selection=readSelection(entry,schema==2);
        need(entry.status()==QDataStream::Ok && entry.atEnd());c.history.append(s);}
    need(in.status()==QDataStream::Ok && in.atEnd());validateStationCatalogue(c);return c;
}
void validateStationTransition(const StationActivationCatalogue& old,const StationActivationCatalogue& next) {
    validateStationCatalogue(old);validateStationCatalogue(next);
    need(old.installation==next.installation && old.revision!=next.revision && next.history.size()==old.history.size()+1);
    for(qsizetype n=0;n<old.history.size();++n)need(snapshotBytes(old.history[n])==snapshotBytes(next.history[n]));
}
}
