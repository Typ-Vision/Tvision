#include "MeasurementEnvironment.hpp"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
#include <QCryptographicHash>
#include <stdexcept>
#include <QUuid>
#include <QRegularExpression>

namespace visionflow {
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void keys(const QJsonObject& o,const QSet<QString>& k){require(o.size()==k.size(),"Invalid environment fields");for(auto i=o.begin();i!=o.end();++i)require(k.contains(i.key()),"Unknown environment field");}
int integer(QJsonValue v,int min=0){require(v.isDouble() && v.toDouble()==v.toInt() && v.toInt()>=min && v.toInt()<=1000000000,"Invalid environment integer");return v.toInt();}
QString text(QJsonValue v){require(v.isString(),"Environment string required");CameraId{v.toString()}.validate();return v.toString();}
QJsonObject object(QJsonValue v){require(v.isObject(),"Environment object required");return v.toObject();}
QJsonArray array(QJsonValue v){require(v.isArray(),"Environment array required");return v.toArray();}
QString digest(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
QVector<CalibrationModelRecord> validateCatalog(const QJsonObject& o,const QString& project){
    keys(o,{"format","schemaVersion","projectId","records","selection","revision"});require(o["format"]=="visionflow.active-calibration" && integer(o["schemaVersion"],1)==1 && text(o["projectId"])==project,"Invalid bundle catalog");integer(o["revision"]);const auto records=array(o["records"]);require(records.size()<=128,"Catalog capacity");QMap<QString,int> versions;QVector<CalibrationModelRecord> decoded;
    for(const auto& v:records){auto r=CalibrationModelRecord::decode(object(v));const auto& d=r.model().dependency();require(d.version==versions.value(d.calibrationId)+1,"Invalid catalog history");versions[d.calibrationId]=d.version;decoded.append(r);}
    if(!o["selection"].isNull()){const auto s=object(o["selection"]);keys(s,{"id","version","contentId","revision"});require(integer(s["revision"],1)==integer(o["revision"]),"Invalid catalog selection revision");bool found=false;for(const auto& r:decoded)if(r.model().dependency().calibrationId==text(s["id"]) && r.model().dependency().version==integer(s["version"],1) && r.contentId()==text(s["contentId"]))found=true;require(found,"Dangling bundle catalog selection");}return decoded;
}
const MeasurementSetupDescriptor& setup(const MeasurementEnvironment& e,const SetupReference& ref){
    ref.validate();for(const auto& s:e.history)if(s.id==ref.setup && s.revision==ref.revision){require(s.camera==ref.camera,"Wrong setup camera");return s;}throw std::runtime_error("Dangling setup reference");
}
void association(const MeasurementEnvironment& e,const RememberedCalibration& selection,const CalibrationModelRecord& r){
    const auto& s=setup(e,selection.target);const auto& d=r.model().dependency();
    require(d.calibrationId==selection.id && d.version==selection.version && r.contentId()==selection.contentId,"Dangling/tampered calibration selection");
    require((r.activatable() || r.experimentallyActivatable()) && s.geometry.complete && r.setup().resolution==s.geometry.resolution && r.setup().lensId==s.opticalProfile && r.setup().planeId==s.workingPlane,"Incompatible remembered model");
}
QJsonArray ids(const QSet<QString>& set){auto sorted=set.values();sorted.sort();QJsonArray a;for(const auto& s:sorted)a.append(s);return a;}
QSet<QString> idSet(QJsonValue v){QSet<QString> result;for(const auto& entry:array(v)){auto s=text(entry);require(!result.contains(s),"Duplicate archived ID");result.insert(s);}return result;}
QJsonObject readJson(const QString& path){QFile f(path);require(f.open(QIODevice::ReadOnly) && f.size()<=4*1024*1024,"Cannot read bounded environment dependency");const auto bytes=f.read(4*1024*1024+1);require(f.error()==QFileDevice::NoError && bytes.size()<=4*1024*1024,"Environment read failed");QJsonParseError error;auto d=QJsonDocument::fromJson(bytes,&error);require(error.error==QJsonParseError::NoError && d.isObject(),"Invalid environment JSON");return d.object();}
void writeJson(const QString& path,const QJsonObject& o,MeasurementEnvironmentRepository::Fault fault){
    const auto b=QJsonDocument(o).toJson();require(b.size()<=4*1024*1024,"Environment size limit");require(fault!=MeasurementEnvironmentRepository::Fault::BeforeWrite,"Injected write failure");QSaveFile f(path);f.setDirectWriteFallback(false);require(f.open(QIODevice::WriteOnly) && f.write(b)==b.size(),"Environment write failed");require(fault!=MeasurementEnvironmentRepository::Fault::BeforeCommit,"Injected precommit failure");require(f.commit(),"Environment atomic commit failed");
}
}
CameraDefaults MeasurementEnvironment::defaults(const CameraId& id) const {
    id.validate();const MeasurementCamera* camera=nullptr;for(const auto& c:current.cameras)if(c.id==id)camera=&c;
    if(!camera)return {};
    if(archivedCameras.contains(id.value))return {CameraDefaultsStatus::Archived,{},{}};
    std::optional<SetupId> selected;
    if(operational.contains(id.value))selected=SetupId{operational[id.value]};
    else if(registrations.contains(id.value))selected=registrations[id.value].setup;
    else for(const auto& s:current.setups)if(s.camera==id && !archivedSetups.contains(s.id.value)){
        if(selected)return {CameraDefaultsStatus::Ambiguous,{},{}};selected=s.id;
    }
    if(!selected)return {};
    if(archivedSetups.contains(selected->value))return {CameraDefaultsStatus::Archived,{},{}};
    const MeasurementSetupDescriptor* setup=nullptr;for(const auto& s:current.setups)if(s.id==*selected && s.camera==id)setup=&s;
    if(!setup)return {};
    std::optional<ProfileId> profile;
    if(registrations.contains(id.value) && registrations[id.value].setup==*selected)profile=registrations[id.value].profile;
    else for(const auto& p:current.profiles)if(p.camera==id && p.setup==*selected){
        if(profile)return {CameraDefaultsStatus::Ambiguous,selected,{}};profile=p.id;
    }
    if(camera->physical && !profile)return {CameraDefaultsStatus::Missing,selected,{}};
    bool pending=setup->pending || !setup->geometry.complete;
    for(const auto& p:current.profiles)if(profile && p.id==*profile)pending=pending || p.pending;
    return {pending ? CameraDefaultsStatus::Pending:CameraDefaultsStatus::Ready,selected,profile};
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
    QJsonObject registrationsJson;QSet<QString> aliases;
    for(auto i=registrations.cbegin();i!=registrations.cend();++i){
        const auto& r=i.value();require(cameras.contains(i.key()),"Dangling camera registration");
        require(QRegularExpression(QRegularExpression::anchoredPattern("[A-Za-z][A-Za-z0-9_]{0,31}")).match(r.shortId).hasMatch(),"Invalid camera short ID");
        const auto alias=r.shortId.toCaseFolded();require(!aliases.contains(alias),"Duplicate camera short ID");aliases.insert(alias);
        bool bound=false;for(const auto& p:current.profiles)for(const auto& s:current.setups)
            if(p.id==r.profile && p.camera.value==i.key() && p.setup==r.setup && s.id==r.setup && s.camera.value==i.key())bound=true;
        require(bound,"Dangling default setup/profile registration");
        registrationsJson[i.key()]=QJsonObject{{"shortId",r.shortId},{"setupId",r.setup.value},{"profileId",r.profile.value}};
    }
    QJsonObject o{{"format","visionflow.measurement-environment"},{"schemaVersion",registrations.isEmpty() ? 1:2},{"projectId",projectId},{"revision",revision},{"current",current.toJson(capacity)},{"history",h},{"archivedCameras",ids(archivedCameras)},{"archivedSetups",ids(archivedSetups)},{"operational",op},{"selections",selectionsJson}};if(!registrations.isEmpty())o["registrations"]=registrationsJson;o["contentId"]=digest(o);return o;
}
MeasurementEnvironment MeasurementEnvironment::decode(const QJsonObject& o,int capacity){
    const int version=integer(o["schemaVersion"],1);require(version==1 || version==2,"Unknown environment schema");
    QSet<QString> fields{"format","schemaVersion","projectId","revision","current","history","archivedCameras","archivedSetups","operational","selections","contentId"};if(version==2)fields.insert("registrations");keys(o,fields);auto payload=o;payload.remove("contentId");require(text(o["contentId"])==digest(payload),"Environment content hash mismatch");require(o["format"]=="visionflow.measurement-environment" ,"Unknown environment schema");MeasurementEnvironment e;e.projectId=text(o["projectId"]);e.revision=integer(o["revision"]);e.current=MeasurementSetupContract::fromJson(object(o["current"]),capacity);
    const auto history=array(o["history"]),selections=array(o["selections"]);require(history.size()<=4096 && selections.size()<=capacity,"Environment entry capacity");for(const auto& s:history)e.history.append(MeasurementSetupDescriptor::fromJson(object(s)));e.archivedCameras=idSet(o["archivedCameras"]);e.archivedSetups=idSet(o["archivedSetups"]);const auto op=object(o["operational"]);for(auto i=op.begin();i!=op.end();++i)e.operational[i.key()]=text(i.value());
    for(const auto& v:selections){const auto s=object(v);keys(s,{"target","id","contentId","version"});e.selections.append({SetupReference::fromJson(object(s["target"])),text(s["id"]),text(s["contentId"]),integer(s["version"],1)});}
    if(version==2){const auto entries=object(o["registrations"]);require(!entries.isEmpty() && entries.size()<=capacity,"Invalid registration capacity");for(auto i=entries.begin();i!=entries.end();++i){const auto r=object(i.value());keys(r,{"shortId","setupId","profileId"});e.registrations[i.key()]={text(r["shortId"]),{text(r["setupId"])},{text(r["profileId"])}};}}
    e.encode(capacity);return e;
}
MeasurementEnvironmentRepository::MeasurementEnvironmentRepository(QString p,QString id,int capacity):projectPath_(std::move(p)),projectId_(std::move(id)),capacity_(capacity){require(QFileInfo(projectPath_).isAbsolute() && capacity>0 && capacity<=4096,"Absolute project path/capacity required");CameraId{projectId_}.validate();}
void MeasurementEnvironmentRepository::validateDependencies(const MeasurementEnvironment& e) const {
    if(e.selections.isEmpty())return;auto catalog=this->catalog();const auto records=catalog.records();for(const auto& s:e.selections){bool found=false;for(const auto& r:records)if(r.contentId()==s.contentId){association(e,s,r);found=true;}require(found,"Missing selected calibration dependency");}
}
MeasurementEnvironment MeasurementEnvironmentRepository::load() const {
    MeasurementEnvironment e;e.projectId=projectId_;if(QFileInfo::exists(path()))e=MeasurementEnvironment::decode(readJson(path()),capacity_);require(e.projectId==projectId_,"Environment project isolation failure");validateDependencies(e);return e;
}
MeasurementEnvironment MeasurementEnvironmentRepository::commit(const MeasurementEnvironment& expected,MeasurementEnvironment next,Fault fault) const {
    QLockFile lock(path()+".lock");require(lock.tryLock(0),"Environment locked");QLockFile catalogLock(catalogPath()+".lock");require(catalogLock.tryLock(0),"Calibration dependency locked");const auto disk=load();require(disk.encode(capacity_)==expected.encode(capacity_),"Environment revision conflict; reload required");require(next.projectId==projectId_ && next.revision==expected.revision,"Invalid commit base");
    // No deletions or history rewriting; archive preserves referenced identities.
    for(const auto& c:disk.current.cameras){bool found=false;for(const auto& n:next.current.cameras)if(c.id==n.id){require(c.rebound(n)==n,"Invalid device revision transition");found=true;}require(found,"Camera deletion requires archive");}
    for(const auto& p:disk.current.profiles){bool found=false;for(const auto& n:next.current.profiles)if(p.id==n.id){require(p.revised(n)==n,"Invalid profile revision transition");found=true;}require(found,"Profile deletion is not supported");}
    require(next.history.size()>=disk.history.size(),"History cannot shrink");for(int i=0;i<disk.history.size();++i)require(next.history[i]==disk.history[i],"Immutable history changed");
    for(auto i=disk.registrations.cbegin();i!=disk.registrations.cend();++i)require(next.registrations.contains(i.key()),"Registration deletion is not supported");
    next.encode(capacity_);validateDependencies(next);if(next.encode(capacity_)==disk.encode(capacity_))return disk;require(disk.revision<1000000000,"Environment revision exhausted");++next.revision;writeJson(path(),next.encode(capacity_),fault);return next;
}
MeasurementEnvironment MeasurementEnvironmentRepository::saveAndRemember(const MeasurementEnvironment& expected,const CalibrationModelRecord& record,const SetupReference& target,Fault fault,bool selectOperational) const {
    RememberedCalibration selection{target,record.model().dependency().calibrationId,record.contentId(),record.model().dependency().version};association(expected,selection,record);
    auto catalog=this->catalog();bool exists=false;for(const auto& r:catalog.records())if(r.model().dependency().calibrationId==selection.id && r.model().dependency().version==selection.version){require(r.contentId()==selection.contentId,"Immutable candidate conflict");exists=true;}if(!exists)catalog.save(record);
    require(fault!=Fault::AfterCandidate,"Injected interruption after candidate");auto next=expected;if(selectOperational)next.operational[target.camera.value]=target.setup.value;bool replaced=false;for(auto& s:next.selections)if(s.target.setup==target.setup){s=selection;replaced=true;}if(!replaced)next.selections.append(selection);return commit(expected,next,fault);
}
QJsonObject MeasurementEnvironmentRepository::exportBundle() const {
    QLockFile lock(path()+".lock");require(lock.tryLock(0),"Environment locked");QLockFile catalogLock(catalogPath()+".lock");require(catalogLock.tryLock(0),"Calibration dependency locked");const auto e=load();QJsonValue catalog;if(QFileInfo::exists(catalogPath())){catalog=this->catalog().catalogView()->document;}return {{"format","visionflow.measurement-bundle"},{"schemaVersion",1},{"environment",e.encode(capacity_)},{"catalog",catalog}};
}
void MeasurementEnvironmentRepository::importBundle(const QJsonObject& bundle) const {
    keys(bundle,{"format","schemaVersion","environment","catalog"});require(bundle["format"]=="visionflow.measurement-bundle" && integer(bundle["schemaVersion"],1)==1,"Unknown bundle schema");auto e=MeasurementEnvironment::decode(object(bundle["environment"]),capacity_);const auto sourceId=e.projectId;e.projectId=projectId_;e.revision=0;const auto c=bundle["catalog"];require(c.isNull() || (c.isObject() && c.toObject()["projectId"]==sourceId),"Bundle catalog project mismatch");
    const auto imported=c.isObject() ? validateCatalog(c.toObject(),sourceId):QVector<CalibrationModelRecord>{};
    if(!c.isObject())require(e.selections.isEmpty(),"Missing bundle calibration dependency");
    for(const auto& selection:e.selections){bool found=false;for(const auto& r:imported)if(r.contentId()==selection.contentId){association(e,selection,r);found=true;}require(found,"Dangling bundle selection");}
    QLockFile lock(path()+".lock");require(lock.tryLock(0),"Destination environment locked");QLockFile catalogLock(catalogPath()+".lock");require(catalogLock.tryLock(0),"Destination catalog locked");require(!QFileInfo::exists(path()),"Destination environment exists");
    if(c.isObject()){auto catalog=c.toObject();catalog["projectId"]=projectId_;if(QFileInfo::exists(catalogPath()))require(readJson(catalogPath())==catalog,"Destination candidate conflict");else writeJson(catalogPath(),catalog,Fault::None);this->catalog().records();}
    else require(!QFileInfo::exists(catalogPath()),"Unexpected destination catalog");validateDependencies(e);writeJson(path(),e.encode(capacity_),Fault::None);
}
MeasurementEnvironmentRegistry::MeasurementEnvironmentRegistry(MeasurementEnvironmentRepository r):repository_(std::move(r)),state_(repository_.load()),thread_(QThread::currentThread()){}
void MeasurementEnvironmentRegistry::owner() const {require(thread_==QThread::currentThread(),"Registry owner thread required");}
const MeasurementEnvironment& MeasurementEnvironmentRegistry::snapshot() const {owner();return state_;}
void MeasurementEnvironmentRegistry::reload(){owner();state_=repository_.load();}
void MeasurementEnvironmentRegistry::store(MeasurementEnvironment next){owner();state_=repository_.commit(state_,std::move(next));}
void MeasurementEnvironmentRegistry::replace(MeasurementSetupContract current){owner();auto next=state_;next.current=std::move(current);for(const auto& s:next.current.setups){bool found=false;for(const auto& h:next.history)if(h.id==s.id && h.revision==s.revision){auto p=h;p.name=s.name;require(p==s,"Revision must change with geometry");found=true;}if(!found)next.history.append(s);}store(next);}
CameraRegistrationResult MeasurementEnvironmentRegistry::registerCamera(MeasurementCamera camera,const QString& shortId,int expectedRevision,MeasurementEnvironmentRepository::Fault fault){
    owner();require(state_.revision==expectedRevision,"Environment revision conflict; reload required");camera.validate();
    require(camera.device.revision==1,"New camera revision must be one");
    require(camera.physical ? camera.physical->usable():camera.device.confidence!=DeviceIdentityConfidence::Unavailable,"Exact physical identity required");
    for(const auto& c:state_.current.cameras){
        const bool same=c.physical && camera.physical ? c.physical->usable() && camera.physical->usable() && *c.physical==*camera.physical:
            !c.physical && !camera.physical && c.device.confidence!=DeviceIdentityConfidence::Unavailable && camera.device.confidence!=DeviceIdentityConfidence::Unavailable && c.device.adapter==camera.device.adapter && c.device.providerId==camera.device.providerId;
        if(same){require(!state_.archivedCameras.contains(c.id.value),"Physical camera is archived");
            // Compare disk even on an idempotent duplicate request.
            const auto existing=c.id;state_=repository_.commit(state_,state_);return {existing,true};}
    }
    require(!state_.current.cameras.contains(camera),"Duplicate camera");
    auto next=state_;QString alias=shortId.trimmed();
    if(alias.isEmpty()){int n=1;for(;;++n){alias=QString("CAM_%1").arg(n,2,10,QChar('0'));bool used=false;for(const auto& r:next.registrations)if(r.shortId.compare(alias,Qt::CaseInsensitive)==0)used=true;if(!used)break;}}
    const SetupId setup{QUuid::createUuid().toString(QUuid::WithoutBraces)};
    const ProfileId profile{QUuid::createUuid().toString(QUuid::WithoutBraces)};
    MeasurementSetupDescriptor descriptor{setup,camera.id,"Default",{},{},1,camera.device.revision,{}};descriptor.pending=true;
    AcquisitionProfile acquisition{profile,camera.id,setup,"Default",1,{}};acquisition.pending=true;
    next.current.cameras.append(camera);next.current.setups.append(descriptor);next.current.profiles.append(acquisition);next.history.append(descriptor);
    next.registrations[camera.id.value]={alias,setup,profile};
    state_=repository_.commit(state_,next,fault);return {camera.id,false};
}
void MeasurementEnvironmentRegistry::renameCamera(const CameraId& id,const QString& shortId,const QString& name,int expectedRevision){
    owner();require(state_.revision==expectedRevision,"Environment revision conflict; reload required");
    require(!state_.archivedCameras.contains(id.value),"Camera is archived");auto next=state_;bool found=false;
    for(auto& c:next.current.cameras)if(c.id==id){c.name=name.trimmed();found=true;}
    require(found,"Camera missing");
    // Legacy alias creation requires explicit default resolution; never choose the first profile.
    if(!next.registrations.contains(id.value)){
        auto resolved=next.defaults(id);require(resolved.setup.has_value() && (resolved.status==CameraDefaultsStatus::Ready || resolved.status==CameraDefaultsStatus::Pending),"Explicit default setup/profile selection required");
        if(!resolved.profile){
            // Explicit metadata edit migrates a uniquely resolved legacy webcam only.
            AcquisitionProfile profile{{QUuid::createUuid().toString(QUuid::WithoutBraces)},id,*resolved.setup,"Default",1,{}};profile.pending=true;
            next.current.profiles.append(profile);resolved.profile=profile.id;
        }
        next.registrations[id.value]={shortId.trimmed(),*resolved.setup,*resolved.profile};
    }else next.registrations[id.value].shortId=shortId.trimmed();
    store(next);
}
void MeasurementEnvironmentRegistry::selectDefaults(const CameraId& id,const QString& shortId,const SetupId& setup,const ProfileId& profile,int expectedRevision){
    owner();require(state_.revision==expectedRevision,"Environment revision conflict; reload required");
    require(!state_.archivedCameras.contains(id.value) && !state_.archivedSetups.contains(setup.value),"Archived defaults cannot be selected");
    auto next=state_;next.registrations[id.value]={shortId.trimmed(),setup,profile};next.operational[id.value]=setup.value;store(next);
}
void MeasurementEnvironmentRegistry::archiveCamera(const CameraId& id){owner();auto next=state_;next.archivedCameras.insert(id.value);next.operational.remove(id.value);store(next);}
void MeasurementEnvironmentRegistry::archiveSetup(const SetupId& id){owner();auto next=state_;next.archivedSetups.insert(id.value);for(auto i=next.operational.begin();i!=next.operational.end();)if(i.value()==id.value)i=next.operational.erase(i);else ++i;store(next);}
void MeasurementEnvironmentRegistry::setOperational(const CameraId& camera,const SetupId& id){owner();auto next=state_;next.operational[camera.value]=id.value;store(next);}
}
