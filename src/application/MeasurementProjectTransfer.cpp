#include "MeasurementProjectTransfer.hpp"
#include "MeasurementEnvironment.hpp"
#include "CalibrationService.hpp"
#include "ProductionResultCodec.hpp"
#include "ToolDependencyResolver.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <QLockFile>
#include <QCryptographicHash>
#include <memory>
namespace visionflow {namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QByteArray read(const QString& path,qint64 limit){QFile f(path);require(!QFileInfo(path).isSymLink() && f.open(QIODevice::ReadOnly) && f.size()<=limit,"Cannot read bounded transfer dependency");auto bytes=f.read(limit+1);require(f.error()==QFile::NoError && bytes.size()<=limit,"Transfer read failed");return bytes;}
void safePath(QString path){for(QFileInfo info(path);;info=QFileInfo(info.absolutePath())){require(!info.isSymLink(),"Transfer symlink/junction refused");if(info.absoluteFilePath()==info.absolutePath())break;}}
QByteArray hash(const QByteArray& b){return QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex();}
void write(const QString& path,const QByteArray& bytes){safePath(path);if(QFileInfo::exists(path)){require(read(path,bytes.size())==bytes,"Destination dependency conflict; no overwrite");return;}require(QDir().mkpath(QFileInfo(path).absolutePath()),"Destination directory unavailable");QSaveFile f(path);f.setDirectWriteFallback(false);require(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size() && f.commit(),"Transfer commit failed");}
}
void MeasurementProjectTransfer::copy(const QString& source,const QString& destination,const QString& projectId,int failAfter){
require(QFileInfo(source).isAbsolute() && QFileInfo(destination).isAbsolute() && QDir::cleanPath(source).compare(QDir::cleanPath(destination),Qt::CaseInsensitive)!=0,"Distinct absolute project destination required");safePath(source);safePath(destination);require(QDir().mkpath(QFileInfo(destination).absolutePath()),"Destination directory unavailable");
QLockFile sourceLock(source+".lock"),destinationLock(destination+".transfer.lock"),productionLock(source+".production/archive.lock"),legacyLock(source+".calibrations.json.lock");require(sourceLock.tryLock(0) && destinationLock.tryLock(0) && legacyLock.tryLock(0),"Project/dependency transfer locked");std::unique_ptr<QLockFile> targetArchive;if(QFileInfo::exists(source+".production"))require(productionLock.tryLock(0),"Production archive locked");if(QFileInfo::exists(destination+".production")){targetArchive=std::make_unique<QLockFile>(destination+".production/archive.lock");require(targetArchive->tryLock(0),"Destination archive locked");}
const auto projectBytes=read(source,16*1024*1024);const auto project=ProjectRepository{}.load(source);require(project.id==projectId,"Transfer source project mismatch");MeasurementEnvironmentRepository repository(source,projectId),target(destination,projectId);const auto bundle=repository.exportBundle();
const auto oldDirectory=QFileInfo(source).absolutePath(),newDirectory=QFileInfo(destination).absolutePath();
for(const auto& recipe:project.recipes)for(const auto& t:editorTools(recipe.definition)){
 if(t.kind=="image_file" && !t.parameters["path"].toString().isEmpty()){const auto path=t.parameters["path"].toString();require(hash(read(QDir(oldDirectory).absoluteFilePath(path),128*1024*1024))==hash(read(QDir(newDirectory).absoluteFilePath(path),128*1024*1024)),"Destination image asset missing/different; use same folder or copy assets explicitly");}
 if(!t.dependencies.isEmpty())require(ToolDependencyResolver::resolve(t,oldDirectory)==ToolDependencyResolver::resolve(t,newDirectory),"Destination tool asset dependency differs");
}
QMap<QString,QByteArray> files;if(QFileInfo::exists(source+".calibrations.json")){CalibrationService(source+".calibrations.json").records();files[".calibrations.json"]=read(source+".calibrations.json",4*1024*1024);}
qint64 total=0;int count=0;for(const auto& kind:QStringList{"snapshots","events","cycles"}){const auto folder=source+".production/"+kind;safePath(folder);for(const auto& file:QDir(folder).entryInfoList({"*.json"},QDir::Files)){require(++count<=10000,"Production transfer record limit");auto bytes=read(file.absoluteFilePath(),20*1024*1024);total+=bytes.size();require(total<=512LL*1024*1024,"Production transfer byte limit");const auto o=QJsonDocument::fromJson(bytes).object();require(o.size()==6 && o["schemaVersion"]==1 && o["format"]=="visionflow.production" && o["kind"]==kind && o["id"]==file.completeBaseName() && o["payload"].isObject() && o["sha256"].toString().toLatin1()==hash(QJsonDocument(o["payload"].toObject()).toJson(QJsonDocument::Compact)),"Production transfer archive evidence corrupt");if(kind=="cycles")ProductionResultCodec::validate(o["payload"].toObject());files[".production/"+kind+"/"+file.fileName()]=bytes;}}
if(!files.contains(".calibrations.json"))require(!QFileInfo::exists(destination+".calibrations.json"),"Unexpected destination legacy catalog");
for(const auto& kind:QStringList{"snapshots","events","cycles"}){const auto folder=destination+".production/"+kind;safePath(folder);for(const auto& file:QDir(folder).entryInfoList({"*.json"},QDir::Files)){const auto key=".production/"+kind+"/"+file.fileName();require(files.contains(key) && read(file.absoluteFilePath(),20*1024*1024)==files.value(key),"Destination contains unrelated production history");}}
if(QFileInfo::exists(destination))require(read(destination,16*1024*1024)==projectBytes,"Destination project differs; no overwrite");
if(QFileInfo::exists(target.path())){auto expected=MeasurementEnvironment::decode(bundle["environment"].toObject());expected.revision=0;const auto copied=target.exportBundle();require(copied["environment"].toObject()==expected.encode() && copied["catalog"]==bundle["catalog"],"Destination registry conflict");}else target.importBundle(bundle);
int copied=0;for(auto it=files.cbegin();it!=files.cend();++it){if(failAfter>=0 && copied==failAfter)throw std::runtime_error("Injected transfer interruption; identical retry resumes");write(destination+it.key(),it.value());++copied;}
require(read(source,16*1024*1024)==projectBytes && repository.exportBundle()==bundle,"Source changed during transfer; destination project not committed");for(auto it=files.cbegin();it!=files.cend();++it)require(read(source+it.key(),it.value().size())==it.value(),"Source history changed during transfer");
write(destination,projectBytes);
}
}
