#include "ProductionStore.hpp"
#include "ProductionResultCodec.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
#include <QUuid>
#include <stdexcept>
namespace visionflow {
namespace {
constexpr qint64 maxFile=20*1024*1024,maxTotal=512LL*1024*1024;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QString digest(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
void kindCheck(const QString& kind){require(QStringList{"snapshots","events","cycles"}.contains(kind),"Production archive kind invalid.");}
QJsonObject read(const QString& path,const QString& kind,const QString& id){
    QFile file(path);require(!QFileInfo(path).isSymLink() && file.open(QIODevice::ReadOnly) && file.size()<=maxFile,"Production archive cannot be read / limit exceeded.");
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(file.readAll(),&error);const auto o=doc.object();
    require(error.error==QJsonParseError::NoError && doc.isObject() && o.size()==6 && o["schemaVersion"]==1 && o["format"]=="visionflow.production" && o["kind"]==kind && o["id"]==id && o["payload"].isObject() && o["sha256"].isString(),"Production archive schema invalid.");
    const auto payload=o["payload"].toObject();require(digest(payload)==o["sha256"].toString(),"Production archive content hash mismatch.");if(kind=="cycles")ProductionResultCodec::validate(payload);return payload;
}
}
ProductionStore::ProductionStore(QString directory):directory_(QDir::cleanPath(directory)) {
    require(QDir(directory_).isAbsolute(),"Production archive requires absolute directory.");
}
void ProductionStore::append(const QString& kind,const QString& id,const QJsonObject& payload) const {
    kindCheck(kind);if(kind=="cycles")ProductionResultCodec::validate(payload);require(!QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces)==id,"Production archive ID invalid.");
    require(!QFileInfo(directory_).isSymLink() && QDir().mkpath(directory_),"Production archive directory unavailable.");
    QLockFile lock(directory_+"/archive.lock");lock.setStaleLockTime(0);require(lock.tryLock(0),"Production archive locked; retry recording.");
    const QDir root(directory_);qint64 bytes=0;int count=0;
    for(const QString group:{"snapshots","events","cycles"}) {
        const auto dir=root.filePath(group);require(!QFileInfo(dir).isSymLink(),"Production archive symlink refused.");
        for(const auto& f:QDir(dir).entryInfoList({"*.json"},QDir::Files)) {require(!f.isSymLink(),"Production archive symlink refused.");bytes+=f.size();++count;}
    }
    const auto folder=root.filePath(kind);require(QDir().mkpath(folder),"Production archive folder unavailable.");
    const auto path=QDir(folder).filePath(id+".json");
    if(QFileInfo::exists(path)){require(read(path,kind,id)==payload,"Production archive overwrite refused.");return;}
    const auto data=QJsonDocument(QJsonObject{{"schemaVersion",1},{"format","visionflow.production"},{"kind",kind},{"id",id},{"sha256",digest(payload)},{"payload",payload}}).toJson(QJsonDocument::Compact);
    require(data.size()<=maxFile && bytes+data.size()<=maxTotal && count<10000,"Production archive full; no automatic retention.");
    QSaveFile file(path);file.setDirectWriteFallback(false);
    require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size() && file.commit(),"Production archive atomic commit failed; retry recording.");
}
QVector<QJsonObject> ProductionStore::recent(const QString& kind,int limit) const {
    kindCheck(kind);require(limit>=1 && limit<=256,"Production archive read limit 1..256.");
    require(!QFileInfo(directory_).isSymLink(),"Production archive symlink refused.");
    const auto folder=QDir(directory_).filePath(kind);require(!QFileInfo(folder).isSymLink(),"Production archive symlink refused.");
    const auto files=QDir(folder).entryInfoList({"*.json"},QDir::Files,QDir::Time);require(files.size()<=10000,"Production archive record limit exceeded.");
    QVector<QJsonObject> result;qint64 bytes=0;
    for(int i=0;i<std::min(limit,int(files.size()));++i){bytes+=files[i].size();require(bytes<=64*1024*1024,"Production archive read byte limit exceeded.");result.append(read(files[i].absoluteFilePath(),kind,files[i].completeBaseName()));}return result;
}
}
