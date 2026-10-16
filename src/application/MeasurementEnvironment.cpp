#include "MeasurementEnvironment.hpp"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
#include <QCryptographicHash>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void keys(const QJsonObject& o,const QSet<QString>& k){require(o.size()==k.size(),"Invalid environment fields");for(auto i=o.begin();i!=o.end();++i)require(k.contains(i.key()),"Unknown environment field");}
int integer(QJsonValue v,int min=0){require(v.isDouble() && v.toDouble()==v.toInt() && v.toInt()>=min && v.toInt()<=1000000000,"Invalid environment integer");return v.toInt();}
QString text(QJsonValue v){require(v.isString(),"Environment string required");CameraId{v.toString()}.validate();return v.toString();}
QJsonObject object(QJsonValue v){require(v.isObject(),"Environment object required");return v.toObject();}
QJsonArray array(QJsonValue v){require(v.isArray(),"Environment array required");return v.toArray();}
QString digest(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
void validateCatalog(const QJsonObject& o,const QString& project){
    keys(o,{"format","schemaVersion","projectId","records","selection","revision"});require(o["format"]=="visionflow.active-calibration" && integer(o["schemaVersion"],1)==1 && text(o["projectId"])==project,"Invalid bundle catalog");integer(o["revision"]);const auto records=array(o["records"]);require(records.size()<=128,"Catalog capacity");QMap<QString,int> versions;QVector<CalibrationModelRecord> decoded;
    for(const auto& v:records){auto r=CalibrationModelRecord::decode(object(v));const auto& d=r.model().dependency();require(d.version==versions.value(d.calibrationId)+1,"Invalid catalog history");versions[d.calibrationId]=d.version;decoded.append(r);}
    if(!o["selection"].isNull()){const auto s=object(o["selection"]);keys(s,{"id","version","contentId","revision"});require(integer(s["revision"],1)==integer(o["revision"]),"Invalid catalog selection revision");bool found=false;for(const auto& r:decoded)if(r.model().dependency().calibrationId==text(s["id"]) && r.model().dependency().version==integer(s["version"],1) && r.contentId()==text(s["contentId"]))found=true;require(found,"Dangling bundle catalog selection");}
}
const MeasurementSetupDescriptor& setup(const MeasurementEnvironment& e,const SetupReference& ref){
    ref.validate();for(const auto& s:e.history)if(s.id==ref.setup && s.revision==ref.revision){require(s.camera==ref.camera,"Wrong setup camera");return s;}throw std::runtime_error("Dangling setup reference");
}
void association(const MeasurementEnvironment& e,const RememberedCalibration& selection,const CalibrationModelRecord& r){
    const auto& s=setup(e,selection.target);const auto& d=r.model().dependency();
    require(d.calibrationId==selection.id && d.version==selection.version && r.contentId()==selection.contentId,"Dangling/tampered calibration selection");
    require(r.activatable() && s.geometry.complete && r.setup().resolution==s.geometry.resolution && r.setup().lensId==s.opticalProfile && r.setup().planeId==s.workingPlane,"Incompatible remembered model");
}
QJsonArray ids(const QSet<QString>& set){auto sorted=set.values();sorted.sort();QJsonArray a;for(const auto& s:sorted)a.append(s);return a;}
QSet<QString> idSet(QJsonValue v){QSet<QString> result;for(const auto& entry:array(v)){auto s=text(entry);require(!result.contains(s),"Duplicate archived ID");result.insert(s);}return result;}
QJsonObject readJson(const QString& path){QFile f(path);require(f.open(QIODevice::ReadOnly) && f.size()<=4*1024*1024,"Cannot read bounded environment dependency");const auto bytes=f.read(4*1024*1024+1);require(f.error()==QFileDevice::NoError && bytes.size()<=4*1024*1024,"Environment read failed");QJsonParseError error;auto d=QJsonDocument::fromJson(bytes,&error);require(error.error==QJsonParseError::NoError && d.isObject(),"Invalid environment JSON");return d.object();}
void writeJson(const QString& path,const QJsonObject& o,MeasurementEnvironmentRepository::Fault fault){
    const auto b=QJsonDocument(o).toJson();require(b.size()<=4*1024*1024,"Environment size limit");require(fault!=MeasurementEnvironmentRepository::Fault::BeforeWrite,"Injected write failure");QSaveFile f(path);f.setDirectWriteFallback(false);require(f.open(QIODevice::WriteOnly) && f.write(b)==b.size(),"Environment write failed");require(fault!=MeasurementEnvironmentRepository::Fault::BeforeCommit,"Injected precommit failure");require(f.commit(),"Environment atomic commit failed");
}
}
QJsonObject MeasurementEnvironment::encode(int capacity) const {
    CameraId{projectId}.validate();require(revision>=0 && revision<=1000000000,"Invalid environment revision");current.validate(capacity);
    require(history.size()<=4096 && selections.size()<=capacity,"Environment history/selection capacity");
    QSet<QString> cameras, setups, seen;for(const auto& c:current.cameras)cameras.insert(c.id.value);for(const auto& s:current.setups)setups.insert(s.id.value);
    for(const auto& s:archivedCameras)require(cameras.contains(s),"Dangling archived camera");for(const auto& s:archivedSetups)require(setups.contains(s),"Dangling archived setup");
    QMap<QString,int> latest;QMap<QString,MeasurementSetupDescriptor> previous;QJsonArray h;
    for(const auto& s:history){s.validate();require(cameras.contains(s.camera.value) && setups.contains(s.id.value) && s.revision==latest.value(s.id.value)+1,"Invalid setup revision history");if(previous.contains(s.id.value))require(previous[s.id.value].revised(s).revision==s.revision,"Invalid geometric revision transition");latest[s.id.value]=s.revision;previous.insert(s.id.value,s);h.append(s.toJson());}
    for(const auto& s:current.setups){require(previous.contains(s.id.value),"Missing setup history");auto p=previous[s.id.value];p.name=s.name;require(p==s,"Current setup/history mismatch");}
    QJsonObject op;for(auto i=operational.begin();i!=operational.end();++i){require(cameras.contains(i.key()) && !archivedCameras.contains(i.key()) && !archivedSetups.contains(i.value()),"Archived/dangling operational camera");bool found=false;for(const auto& s:current.setups)if(s.id.value==i.value() && s.camera.value==i.key())found=true;require(found,"Operational setup mismatch");op[i.key()]=i.value();}
    QJsonArray selectionsJson;for(const auto& s:selections){setup(*this,s.target);CameraId{s.id}.validate();require(s.version>0 && s.version<=1000000000 && s.contentId.size()==64,"Invalid remembered calibration identity");for(auto c:s.contentId)require((c>='0' && c<='9') || (c>='a' && c<='f'),"Invalid remembered content hash");require(!seen.contains(s.target.setup.value),"Duplicate setup selection");seen.insert(s.target.setup.value);selectionsJson.append(QJsonObject{{"target",s.target.toJson()},{"id",s.id},{"version",s.version},{"contentId",s.contentId}});}
    QJsonObject o{{"format","visionflow.measurement-environment"},{"schemaVersion",1},{"projectId",projectId},{"revision",revision},{"current",current.toJson(capacity)},{"history",h},{"archivedCameras",ids(archivedCameras)},{"archivedSetups",ids(archivedSetups)},{"operational",op},{"selections",selectionsJson}};o["contentId"]=digest(o);return o;
}
MeasurementEnvironment MeasurementEnvironment::decode(const QJsonObject& o,int capacity){
    keys(o,{"format","schemaVersion","projectId","revision","current","history","archivedCameras","archivedSetups","operational","selections","contentId"});auto payload=o;payload.remove("contentId");require(text(o["contentId"])==digest(payload),"Environment content hash mismatch");require(o["format"]=="visionflow.measurement-environment" && integer(o["schemaVersion"],1)==1,"Unknown environment schema");MeasurementEnvironment e;e.projectId=text(o["projectId"]);e.revision=integer(o["revision"]);e.current=MeasurementSetupContract::fromJson(object(o["current"]),capacity);
    const auto history=array(o["history"]),selections=array(o["selections"]);require(history.size()<=4096 && selections.size()<=capacity,"Environment entry capacity");for(const auto& s:history)e.history.append(MeasurementSetupDescriptor::fromJson(object(s)));e.archivedCameras=idSet(o["archivedCameras"]);e.archivedSetups=idSet(o["archivedSetups"]);const auto op=object(o["operational"]);for(auto i=op.begin();i!=op.end();++i)e.operational[i.key()]=text(i.value());
    for(const auto& v:selections){const auto s=object(v);keys(s,{"target","id","contentId","version"});e.selections.append({SetupReference::fromJson(object(s["target"])),text(s["id"]),text(s["contentId"]),integer(s["version"],1)});}e.encode(capacity);return e;
}
MeasurementEnvironmentRepository::MeasurementEnvironmentRepository(QString p,QString id,int capacity):projectPath_(std::move(p)),projectId_(std::move(id)),capacity_(capacity){require(QFileInfo(projectPath_).isAbsolute() && capacity>0 && capacity<=4096,"Absolute project path/capacity required");CameraId{projectId_}.validate();}
void MeasurementEnvironmentRepository::validateDependencies(const MeasurementEnvironment& e) const {
    if(e.selections.isEmpty())return;ActiveCalibrationService catalog(catalogPath(),projectId_,[]{return true;});const auto records=catalog.records();for(const auto& s:e.selections){bool found=false;for(const auto& r:records)if(r.contentId()==s.contentId){association(e,s,r);found=true;}require(found,"Missing selected calibration dependency");}
}
MeasurementEnvironment MeasurementEnvironmentRepository::load() const {
    MeasurementEnvironment e;e.projectId=projectId_;if(QFileInfo::exists(path()))e=MeasurementEnvironment::decode(readJson(path()),capacity_);require(e.projectId==projectId_,"Environment project isolation failure");validateDependencies(e);return e;
}
MeasurementEnvironment MeasurementEnvironmentRepository::commit(const MeasurementEnvironment& expected,MeasurementEnvironment next,Fault fault) const {
    QLockFile lock(path()+".lock");require(lock.tryLock(0),"Environment locked");QLockFile catalogLock(catalogPath()+".lock");require(catalogLock.tryLock(0),"Calibration dependency locked");const auto disk=load();require(disk.encode(capacity_)==expected.encode(capacity_),"Environment revision conflict; reload required");require(next.projectId==projectId_ && next.revision==expected.revision,"Invalid commit base");
    // No deletions or history rewriting; archive preserves referenced identities.
    for(const auto& c:disk.current.cameras){bool found=false;for(const auto& n:next.current.cameras)if(c.id==n.id){require(c.device.rebound(n.device)==n.device,"Invalid device revision transition");found=true;}require(found,"Camera deletion requires archive");}
    require(next.history.size()>=disk.history.size(),"History cannot shrink");for(int i=0;i<disk.history.size();++i)require(next.history[i]==disk.history[i],"Immutable history changed");
    next.encode(capacity_);validateDependencies(next);if(next.encode(capacity_)==disk.encode(capacity_))return disk;require(disk.revision<1000000000,"Environment revision exhausted");++next.revision;writeJson(path(),next.encode(capacity_),fault);return next;
}
MeasurementEnvironment MeasurementEnvironmentRepository::saveAndRemember(const MeasurementEnvironment& expected,const CalibrationModelRecord& record,const SetupReference& target,Fault fault) const {
    RememberedCalibration selection{target,record.model().dependency().calibrationId,record.contentId(),record.model().dependency().version};association(expected,selection,record);
    ActiveCalibrationService catalog(catalogPath(),projectId_,[]{return true;});bool exists=false;for(const auto& r:catalog.records())if(r.model().dependency().calibrationId==selection.id && r.model().dependency().version==selection.version){require(r.contentId()==selection.contentId,"Immutable candidate conflict");exists=true;}if(!exists)catalog.save(record);
    require(fault!=Fault::AfterCandidate,"Injected interruption after candidate");auto next=expected;bool replaced=false;for(auto& s:next.selections)if(s.target.setup==target.setup){s=selection;replaced=true;}if(!replaced)next.selections.append(selection);return commit(expected,next,fault);
}
QJsonObject MeasurementEnvironmentRepository::exportBundle() const {
    QLockFile lock(path()+".lock");require(lock.tryLock(0),"Environment locked");QLockFile catalogLock(catalogPath()+".lock");require(catalogLock.tryLock(0),"Calibration dependency locked");const auto e=load();QJsonValue catalog;if(QFileInfo::exists(catalogPath())){ActiveCalibrationService checked(catalogPath(),projectId_,[]{return true;});checked.records();catalog=readJson(catalogPath());}return {{"format","visionflow.measurement-bundle"},{"schemaVersion",1},{"environment",e.encode(capacity_)},{"catalog",catalog}};
}
void MeasurementEnvironmentRepository::importBundle(const QJsonObject& bundle) const {
    keys(bundle,{"format","schemaVersion","environment","catalog"});require(bundle["format"]=="visionflow.measurement-bundle" && integer(bundle["schemaVersion"],1)==1,"Unknown bundle schema");auto e=MeasurementEnvironment::decode(object(bundle["environment"]),capacity_);const auto sourceId=e.projectId;e.projectId=projectId_;e.revision=0;const auto c=bundle["catalog"];require(c.isNull() || (c.isObject() && c.toObject()["projectId"]==sourceId),"Bundle catalog project mismatch");
    if(c.isObject())validateCatalog(c.toObject(),sourceId);else require(e.selections.isEmpty(),"Missing bundle calibration dependency");
    if(c.isObject())for(const auto& selection:e.selections){bool found=false;for(const auto& v:c.toObject()["records"].toArray()){auto r=CalibrationModelRecord::decode(v.toObject());if(r.contentId()==selection.contentId){association(e,selection,r);found=true;}}require(found,"Dangling bundle selection");}
    QLockFile lock(path()+".lock");require(lock.tryLock(0),"Destination environment locked");QLockFile catalogLock(catalogPath()+".lock");require(catalogLock.tryLock(0),"Destination catalog locked");require(!QFileInfo::exists(path()),"Destination environment exists");
    if(c.isObject()){auto catalog=c.toObject();catalog["projectId"]=projectId_;if(QFileInfo::exists(catalogPath()))require(readJson(catalogPath())==catalog,"Destination candidate conflict");else writeJson(catalogPath(),catalog,Fault::None);ActiveCalibrationService checked(catalogPath(),projectId_,[]{return true;});checked.records();}
    else require(!QFileInfo::exists(catalogPath()),"Unexpected destination catalog");validateDependencies(e);writeJson(path(),e.encode(capacity_),Fault::None);
}
MeasurementEnvironmentRegistry::MeasurementEnvironmentRegistry(MeasurementEnvironmentRepository r):repository_(std::move(r)),state_(repository_.load()),thread_(QThread::currentThread()){}
void MeasurementEnvironmentRegistry::owner() const {require(thread_==QThread::currentThread(),"Registry owner thread required");}
const MeasurementEnvironment& MeasurementEnvironmentRegistry::snapshot() const {owner();return state_;}
void MeasurementEnvironmentRegistry::reload(){owner();state_=repository_.load();}
void MeasurementEnvironmentRegistry::store(MeasurementEnvironment next){owner();state_=repository_.commit(state_,std::move(next));}
void MeasurementEnvironmentRegistry::replace(MeasurementSetupContract current){owner();auto next=state_;next.current=std::move(current);for(const auto& s:next.current.setups){bool found=false;for(const auto& h:next.history)if(h.id==s.id && h.revision==s.revision){auto p=h;p.name=s.name;require(p==s,"Revision must change with geometry");found=true;}if(!found)next.history.append(s);}store(next);}
void MeasurementEnvironmentRegistry::archiveCamera(const CameraId& id){owner();auto next=state_;next.archivedCameras.insert(id.value);next.operational.remove(id.value);store(next);}
void MeasurementEnvironmentRegistry::archiveSetup(const SetupId& id){owner();auto next=state_;next.archivedSetups.insert(id.value);for(auto i=next.operational.begin();i!=next.operational.end();)if(i.value()==id.value)i=next.operational.erase(i);else ++i;store(next);}
void MeasurementEnvironmentRegistry::setOperational(const CameraId& camera,const SetupId& id){owner();auto next=state_;next.operational[camera.value]=id.value;store(next);}
}
