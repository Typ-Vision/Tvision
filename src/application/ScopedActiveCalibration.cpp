#include "ScopedActiveCalibration.hpp"
#include <stdexcept>
#include <QFile>
#include <QCryptographicHash>
namespace visionflow {
namespace {QJsonObject setupProof(const MeasurementEnvironment& e,const SetupReference& r){for(auto s:e.current.setups)if(s.id==r.setup && s.revision==r.revision){s.name=s.id.value;return s.toJson();}throw std::runtime_error("Missing setup proof");}
QString sGeometryId(const MeasurementEnvironment& e,const SetupReference& r){for(const auto& s:e.current.setups)if(s.id==r.setup && s.revision==r.revision)return s.geometryId();throw std::runtime_error("Missing setup geometry");}
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}}
ScopedActiveCalibrationService::ScopedActiveCalibrationService(MeasurementEnvironmentRepository r,ActiveCalibrationService::ChangeGate g)
    :repository_(std::move(r)),gate_(std::move(g)),thread_(QThread::currentThread()) {require(bool(gate_),"Change gate required");repository_.load();}
void ScopedActiveCalibrationService::owner() const {require(thread_==QThread::currentThread(),"Scoped calibration owner thread required");}
MeasurementEnvironment ScopedActiveCalibrationService::environment() const {owner();return repository_.load();}
MeasurementSetup ScopedActiveCalibrationService::installation(const MeasurementEnvironment& e,const SetupReference& ref) const {
    ref.validate();require(!e.archivedCameras.contains(ref.camera.value) && !e.archivedSetups.contains(ref.setup.value),"Archived installation");
    for(const auto& s:e.current.setups)if(s.id==ref.setup && s.camera==ref.camera && s.revision==ref.revision){
        for(const auto& c:e.current.cameras)if(c.id==ref.camera){
            require(c.device.adapter=="webcam" && s.geometry.complete && !s.geometry.sensorRoi && !s.geometry.binning && c.device.confidence!=DeviceIdentityConfidence::Unavailable,"Unsupported/unknown installation geometry or identity");
            return {s.id.value+"/"+QString::number(s.revision),c.device.adapter,CalibrationWizardSession::configurationId({{"geometryId",s.geometryId()},{"device",c.device.toJson()}}),s.opticalProfile,s.workingPlane,s.geometry.resolution,{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{s.workingPlane+"/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
        }
    }throw std::runtime_error("Current exact installation required");
}
QVector<CalibrationModelRecord> ScopedActiveCalibrationService::records(const SetupReference& ref) const {
    const auto e=environment();const auto expected=installation(e,ref);QVector<CalibrationModelRecord> out;
    ActiveCalibrationService catalog(repository_.catalogPath(),e.projectId,gate_);
    for(const auto& r:catalog.records())if(r.setup()==expected)out.append(r);return out;
}
void ScopedActiveCalibrationService::save(const SetupReference& ref,const CalibrationModelRecord& r){
    const auto e=environment();require(gate_(),"Runtime/worker/pending record/ACK busy");require(r.setup()==installation(e,ref),"Exact model installation mismatch");
    ActiveCalibrationService(repository_.catalogPath(),e.projectId,gate_).save(r);
}
QVector<CalibrationModelRecord> ScopedActiveCalibrationService::historyRecords(const SetupId& id) const {
    const auto e=environment();id.validate();QVector<CalibrationModelRecord> out;
    ActiveCalibrationService catalog(repository_.catalogPath(),e.projectId,gate_);
    for(const auto& r:catalog.records())for(const auto& s:e.history)
        if(s.id==id && r.setup().id==s.id.value+"/"+QString::number(s.revision) && r.setup().resolution==s.geometry.resolution && r.setup().lensId==s.opticalProfile && r.setup().planeId==s.workingPlane){out.append(r);break;}
    return out; // Historical display only; never an activation/resolve path.
}
ActiveCalibrationSnapshot ScopedActiveCalibrationService::candidate(const SetupReference& ref,const QString& content) const {
    const auto e=environment();for(const auto& r:records(ref))if(r.contentId()==content){const auto& d=r.model().dependency();for(const auto& c:e.current.cameras)if(c.id==ref.camera)return {r,{d.calibrationId,r.contentId(),d.version,1},{{"schemaVersion",1},{"target",ref.toJson()},{"device",c.device.toJson()},{"geometryId",sGeometryId(e,ref)},{"setup",setupProof(e,ref)}}};}throw std::runtime_error("Exact candidate not found");
}
int ScopedActiveCalibrationService::migrateLegacy(const SetupReference& ref,const QVector<CalibrationModelRecord>& originals,int failAfter){
    const auto target=setup(ref);require(gate_(),"Runtime/worker/pending record/ACK busy");require(!originals.isEmpty(),"Legacy history empty");QVector<CalibrationModelRecord> pending;const auto existing=records(ref);const auto id=originals.first().model().dependency().calibrationId;const auto at=QDateTime::currentDateTimeUtc();int version=0;
    for(const auto& original:originals){require(original.model().dependency().calibrationId==id && original.model().dependency().version==++version,"Complete consecutive original history required");auto mapped=CalibrationModelRecord::mapLegacy(original,target,original.name().left(128),at);bool found=false;
        for(const auto& r:existing)if(r.model().dependency().calibrationId==mapped.model().dependency().calibrationId && r.model().dependency().version==version){require(r.originalContentId()==original.contentId() && r.setup()==target,"Legacy migration prefix conflict");found=true;break;}if(!found)pending.append(mapped);
    }
    int saved=0;for(const auto& r:pending){if(failAfter>=0 && saved==failAfter)throw std::runtime_error("Injected migration interruption; retry resumes immutable prefix");save(ref,r);++saved;}return saved;
}
void ScopedActiveCalibrationService::activate(const SetupReference& ref,const QString& content,const IndependentCalibrationSession& probe,MeasurementEnvironmentRepository::Fault fault){
    const auto e=environment();require(gate_(),"Runtime/worker/pending record/ACK busy");require(revision_<1000000000,"Activation revision exhausted");
    const auto expected=installation(e,ref);const auto proof=probe.evidence();
    require(!probe.busy() && proof && proof->origin==SourceOrigin::CameraAcquired && proof->target==std::optional(ref) && probe.acceptedSetup()==std::optional(expected),"Accepted actual camera probe for exact installation required");
    require(!usedProbes_.contains(proof->frameId),"Acquire a new activation probe");
    require(e.operational.value(ref.camera.value)==ref.setup.value,"Select operational setup explicitly first");
    for(const auto& r:records(ref))if(r.contentId()==content){
        require(r.activatable() && (r.image().origin=="acquired" || r.image().origin=="setup_file" || (r.image().origin=="legacy_record" && r.encode()["schemaVersion"]==5)),"Trial/legacy/rejected control cannot activate");
        const auto committed=repository_.saveAndRemember(e,r,ref,fault);
        for(const auto& selection:committed.selections)if(selection.target==ref){permissions_.insert(ref.setup.value,{selection,++revision_});usedProbes_.insert(proof->frameId);return;}
    }throw std::runtime_error("Exact calibration content not found");
}
void ScopedActiveCalibrationService::deactivate(const SetupId& id){
    const auto e=environment();id.validate();require(gate_(),"Runtime/worker/pending record/ACK busy");auto next=e;
    for(int i=next.selections.size()-1;i>=0;--i)if(next.selections[i].target.setup==id)next.selections.removeAt(i);
    repository_.commit(e,next);permissions_.remove(id.value);
}
void ScopedActiveCalibrationService::setOperational(const CameraId& camera,const SetupId& setup){
    const auto e=environment();require(gate_(),"Runtime/worker/pending record/ACK busy");auto next=e;next.operational[camera.value]=setup.value;
    repository_.commit(e,next);if(e.operational.value(camera.value)!=setup.value)disconnect(camera);
}
void ScopedActiveCalibrationService::disconnect(const CameraId& camera){owner();for(auto i=permissions_.begin();i!=permissions_.end();)if(i->selection.target.camera==camera)i=permissions_.erase(i);else ++i;}
void ScopedActiveCalibrationService::refreshAvailability(const QVector<DeviceBinding>& devices){
    const auto e=environment();for(const auto& c:e.current.cameras)try{c.device.resolve(devices);}catch(const std::exception&){disconnect(c.id);}
}
QVector<ScopedCalibrationSnapshot> ScopedActiveCalibrationService::snapshots(const QVector<SetupReference>& refs,std::function<void()> beforeVerify) const {
    auto fingerprint=[&]{QFile f(repository_.catalogPath());if(!f.exists())return QByteArray("missing");require(f.open(QIODevice::ReadOnly) && f.size()<=4*1024*1024,"Catalog snapshot bounded read failed");const auto bytes=f.read(4*1024*1024+1);require(f.error()==QFileDevice::NoError && bytes.size()<=4*1024*1024,"Catalog snapshot read failed");return QCryptographicHash::hash(bytes,QCryptographicHash::Sha256);};
    const auto before=fingerprint();const auto e=environment();const auto permissions=permissions_;
    const auto models=ActiveCalibrationService(repository_.catalogPath(),e.projectId,gate_).records();QVector<ScopedCalibrationSnapshot> out;QSet<QString> seen;
    for(const auto& ref:refs){require(!seen.contains(ref.setup.value),"Duplicate snapshot target");seen.insert(ref.setup.value);
        const auto expected=installation(e,ref);require(e.operational.value(ref.camera.value)==ref.setup.value && permissions_.contains(ref.setup.value),"Installation requires explicit activation");
        const auto p=permissions_.value(ref.setup.value);require(p.selection.target==ref && e.selections.contains(p.selection),"Selection changed; reactivate");bool found=false;
        for(const auto& r:models)if(r.contentId()==p.selection.contentId && r.activatable() && r.setup()==expected){
            for(const auto& c:e.current.cameras)if(c.id==ref.camera)out.append({ref,c.device,{r,{p.selection.id,p.selection.contentId,p.selection.version,p.revision},{{"schemaVersion",1},{"target",ref.toJson()},{"device",c.device.toJson()},{"geometryId",sGeometryId(e,ref)},{"setup",setupProof(e,ref)}}}});found=true;
        }require(found,"Calibration dependency unavailable");
    }if(beforeVerify)beforeVerify();require(before==fingerprint() && e.encode()==environment().encode() && permissions==permissions_,"Calibration snapshot set changed; retry whole admission");return out;
}
}
