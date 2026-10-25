#pragma once
#include "ScopedActiveCalibration.hpp"
#include "WebcamAdapter.hpp"
#include "SystemCalibration.hpp"
#include "CalibrationService.hpp"
#include "MultiSourceRuntimeService.hpp"
#include "IndustrialCameraProvider.hpp"
#include "IndustrialExecution.hpp"
#include "CameraSettingsService.hpp"
#include <QUuid>
#include <stdexcept>
#include <QFileInfo>
namespace visionflow {
// MainWindow owns this project workspace; views only issue application commands.
class MeasurementWorkspace {
public:
    MeasurementWorkspace(QString path,QString project,ActiveCalibrationService::ChangeGate gate,std::function<QVector<DeviceBinding>()> discovery=WebcamAdapter::discover)
        :path_(path),project_(project),repository_(path,project),registry_(repository_),active_(repository_,gate),legacy_(path+".active-calibrations.json",project,gate,repository_.catalogCache()),gate_(std::move(gate)),discovery_(std::move(discovery)){}
    MeasurementEnvironment environment(){registry_.reload();return registry_.snapshot();}
    ActiveCalibrationService& legacyCalibrations(){return legacy_;}
    const ActiveCalibrationService& legacyCalibrations() const{return legacy_;}
    ScopedActiveCalibrationService& calibrations(){return active_;}
    CalibrationSaveUseResult saveAndUseCalibration(const CalibrationSaveUseRequest& request,const CalibrationModelRecord& record,
        const IndependentCalibrationSession& probe,std::function<bool()> currentTarget){
        return active_.saveAndUse(request,record,probe,std::move(currentTarget));
    }
    void notifyOpticalChange(const CameraId& camera,int expectedRevision){active_.notifyOpticalChange(camera,expectedRevision);registry_.reload();}
    QVector<DeviceBinding> discover(){auto devices=discovery_();active_.refreshAvailability(devices);discovered_=devices;discoveryDone_=true;return devices;}
    const QVector<DeviceBinding>& discovered() const{return discovered_;}
    void observeInventory(CameraInventory snapshot){
        inventory_=std::move(snapshot);inventoryDone_=true;
        QVector<DeviceBinding> webcams;for(const auto& d:inventory_.devices)if(d.webcam)webcams.append(*d.webcam);
        active_.refreshAvailability(webcams,inventory_.industrial);discovered_=webcams;discoveryDone_=!inventory_.cancelled;
    }
    CameraId registerCamera(const CameraDiscoveryEntry& entry,const QString& name){
        const auto revision=environment().revision;return registerCamera(entry,{},name,revision).camera;
    }
    CameraRegistrationResult registerCamera(const CameraDiscoveryEntry& entry,const QString& shortId,const QString& name,int expectedRevision){
        if(!QFileInfo(path_).isFile())throw std::runtime_error("Saved project required");
        if(!gate_())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul");
        environment();MeasurementCamera camera;
        camera.name=name.trimmed().isEmpty() ? (shortId.trimmed().isEmpty() ? QString("Camera"):shortId.trimmed()):name.trimmed();
        camera.id={QUuid::createUuid().toString(QUuid::WithoutBraces)};
        if(entry.webcam && !entry.industrial){camera.device=*entry.webcam;camera.device.resolve(discovered_);}
        else if(entry.industrial && !entry.webcam){
            camera.device={"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}};camera.physical=entry.industrial->identity;
            if(!inventoryDone_ || inventory_.cancelled || !inventory_.errors.isEmpty() || entry.industrial->ambiguous)
                throw std::runtime_error("Tam ve belirsiz olmayan keşif gerekli.");
            resolveRoutes(*camera.physical,inventory_.industrial.devices);
        }else throw std::runtime_error("Tek keşif kaydı gerekli.");
        camera.validate();return registry_.registerCamera(camera,shortId,expectedRevision);
    }
    void renameCamera(const CameraId& id,const QString& shortId,const QString& name,int expectedRevision){
        if(!gate_())throw std::runtime_error("Workspace busy");
        registry_.reload();registry_.renameCamera(id,shortId,name,expectedRevision);
    }
    void selectCameraDefaults(const CameraId& id,const QString& shortId,const SetupId& setup,const ProfileId& profile,int expectedRevision){
        if(!gate_())throw std::runtime_error("Workspace busy");
        registry_.reload();const auto before=registry_.snapshot();const auto previous=before.defaults(id);
        registry_.selectDefaults(id,shortId,setup,profile,expectedRevision);
        if(before.operational.value(id.value)!=setup.value || previous.setup!=setup || previous.profile!=profile)active_.disconnect(id);
    }
    CameraDefaults cameraDefaults(const CameraId& id){return environment().defaults(id);}
    std::unique_ptr<CameraSettingsSession> cameraSettings(const CameraId& id,int expectedRevision,
        const CameraLocalConfiguration& local,const CameraProviderFactory& factory,std::function<bool()> ownerGate={},
        QString runtime=OecsCameraSettings::installedRuntime(),OecsCameraSettings::ApiFactory api=makeOecsCApi){
        // Opening uses the full workspace gate. The owning controller may supply
        // an in-flight gate that excludes only its own CameraSession reservation.
        if(!gate_())throw std::runtime_error("Workspace busy");
        if(!QFileInfo(path_).isFile())throw std::runtime_error("Saved project required");
        if(!inventoryDone_ || inventory_.cancelled || !inventory_.errors.isEmpty())throw std::runtime_error("Complete discovery required");
        return CameraSettingsService::open(repository_,id,expectedRevision,
            inventory_.industrial,local,factory,ownerGate?std::move(ownerGate):gate_,[this,id]{active_.disconnect(id);},
            MeasurementEnvironmentRepository::Fault::None,std::move(runtime),std::move(api));
    }
    QString availability(const MeasurementCamera& c) const {
        if(c.physical){
            if(!inventoryDone_)return "Keşif yapılmadı • kayıtlı endüstriyel kamera";
            if(inventory_.cancelled || !inventory_.errors.isEmpty())return "Keşif eksik / SDK kullanılamıyor";
            try{const auto routes=resolveRoutes(*c.physical,inventory_.industrial.devices);
                return routes.size()>1 ? "Keşfedildi • yerel rota seçimi gerekli" : "Keşfedildi";
            }catch(const std::exception&){return "Çevrimdışı / belirsiz • otomatik eşleme yok";}
        }
        if(c.device.adapter!="webcam")return "Desteklenmeyen edinim adapter'ı";
        if(c.device.confidence==DeviceIdentityConfidence::Unavailable)return "Cihaz eşlemesi gerekli";
        if(!discoveryDone_)return "Keşif yapılmadı";
        try{c.device.resolve(discovered_);return "Keşfedildi • kullanım için kare denetimi gerekli";}
        catch(const std::exception& e){return QString::fromUtf8(e.what());}
    }
    void replace(MeasurementSetupContract c,std::optional<int> expectedRevision={}){if(!gate_())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul");if(expectedRevision && registry_.snapshot().revision!=*expectedRevision)throw std::runtime_error("Kurulum belgesi değişti; formu yeniden açın.");registry_.replace(std::move(c));}
    void archive(const CameraId& id){if(!gate_())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul");registry_.reload();registry_.archiveCamera(id);active_.disconnect(id);}
    void archiveSetup(const SetupId& id){if(!gate_())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul");registry_.reload();auto e=registry_.snapshot();registry_.archiveSetup(id);for(const auto& s:e.current.setups)if(s.id==id)active_.disconnect(s.camera);}
    std::optional<ActiveCalibrationSnapshot> forRecipe(const Recipe& r) const {const auto ref=SystemCalibration::setupReference(r);if(!ref){if(r.definition.contains("calibrationPolicy"))throw std::runtime_error("Scoped recipe requires explicit setup binding");return {};}return active_.snapshots({*ref}).first().calibration;}
    std::optional<ActiveCalibrationSnapshot> configurationForRecipe(const Recipe& r) const {
        const auto ref=SystemCalibration::setupReference(r);if(!ref)throw std::runtime_error("Scoped recipe requires explicit setup binding");
        return active_.configuration(*ref);
    }
    MultiSourceDependencies configurationSources(const Recipe& r,const std::optional<IndustrialExecutionInputs>& industrial={},bool experimental=false) const {
        return sourceDependencies(r,industrial,experimental,true);
    }
    MultiSourceDependencies productionSources(const Recipe& r,const std::optional<IndustrialExecutionInputs>& industrial={},bool experimental=false) const {
        return sourceDependencies(r,industrial,experimental,false);
    }
private:
    MultiSourceDependencies sourceDependencies(const Recipe& recipe,const std::optional<IndustrialExecutionInputs>& industrial,bool experimental,bool configurationOnly) const {
        const auto before=active_.environment();const auto graph=validateSourceGraph(editorTools(recipe.definition),editorConnections(recipe.definition));if(!graph.valid() || graph.requiredSources.isEmpty())throw std::runtime_error("Invalid required source graph");
        QVector<SetupReference> refs;QMap<QString,Recipe> branches;
        for(const auto& source:graph.requiredSources){const auto branch=SystemCalibration::branch(recipe,source);branches[source]=branch;if(SystemCalibration::requestsCalibration(branch,false)){const auto ref=SystemCalibration::setupReference(branch);if(!ref)throw std::runtime_error("Metric source requires exact setup binding");if(!refs.contains(*ref))refs.append(*ref);}}
        QVector<ScopedCalibrationSnapshot> models;QVector<ExperimentalScopedCalibrationSnapshot> experimentalModels;
        if(configurationOnly){for(const auto& ref:refs){auto m=active_.configuration(ref,experimental);DeviceBinding device;for(const auto& c:before.current.cameras)if(c.id==ref.camera)device=c.device;
            if(experimental)experimentalModels.append(ExperimentalScopedCalibrationSnapshot{{ref,device,m}});else models.append(ScopedCalibrationSnapshot{ref,device,m});}}
        else if(experimental && !refs.isEmpty())experimentalModels=active_.experimentalSnapshots(refs);
        else models=active_.snapshots(refs);
        MultiSourceDependencies set;set.configurationOnly=configurationOnly;
        for(const auto& source:graph.requiredSources)if(SystemCalibration::source(branches[source]).kind=="industrial_camera"){
            if(!industrial)throw std::runtime_error("Industrial Runtime unavailable: admitted native backend composition required");
            auto frozen=*industrial;frozen.contract=before.current;frozen.admittedDevices.clear();frozen.leaseUse=experimental ? CameraLeaseUse::Editor:CameraLeaseUse::Runtime;
            set.industrial=std::make_shared<const IndustrialExecutionInputs>(std::move(frozen));break;
        }
        for(const auto& source:graph.requiredSources){const auto branch=branches[source];ProductionDependencies d;const auto ref=SystemCalibration::setupReference(branch);
            const auto tool=SystemCalibration::source(branch);
            if(tool.kind=="industrial_camera"){
                const auto reference=IndustrialSourceReference::fromJson(tool.parameters["sourceReference"].toObject());reference.validateAgainst(before.current);
                if(before.archivedCameras.contains(reference.camera.value) || before.archivedSetups.contains(reference.setup.value))throw std::runtime_error("Archived industrial Runtime source");
            }
            if(ref && tool.kind!="industrial_camera"){bool found=false;for(const auto& c:before.current.cameras)if(c.id==ref->camera)for(const auto& s:before.current.setups)if(s.id==ref->setup && s.revision==ref->revision && s.camera==c.id){if(before.archivedCameras.contains(c.id.value) || before.archivedSetups.contains(s.id.value))throw std::runtime_error("Archived required source");d.measurementSource={{"schemaVersion",1},{"binding",branch.definition["sourceSetup"]},{"acquisition",QJsonObject{{"target",ref->toJson()},{"device",c.device.toJson()},{"geometry",s.geometry.toJson()}}},{"origin",SystemCalibration::source(branch).kind=="image_file" ? "offline_replay":"camera_acquired"}};found=true;}if(!found)throw std::runtime_error("Required source setup changed");}
            for(const auto& m:models)if(ref && SystemCalibration::requestsCalibration(branch,false) && m.target==*ref){d.calibration=SystemCalibration::binding(m.calibration,source,source+"/image");d.systemCalibration=SystemCalibration::evidence(m.calibration,source,source+"/image");}
            for(const auto& m:experimentalModels)if(ref && SystemCalibration::requestsCalibration(branch,false) && m.snapshot.target==*ref)set.experimentalCalibrations[source]=std::make_shared<const ExperimentalScopedCalibrationSnapshot>(m);
            set.sources.insert(source,d);
        }
        if(before.encode()!=active_.environment().encode())throw std::runtime_error("Workspace changed during source-set admission");return set;
    }
public:
    QJsonObject acquisition(const Recipe& r) const {const auto ref=SystemCalibration::setupReference(r);if(!ref)return {};const auto e=active_.environment();
        if(e.archivedCameras.contains(ref->camera.value) || e.archivedSetups.contains(ref->setup.value))throw std::runtime_error("Archived source setup");
        for(const auto& c:e.current.cameras)if(c.id==ref->camera)for(const auto& s:e.current.setups)if(s.id==ref->setup && s.camera==c.id && s.revision==ref->revision){IndependentCaptureRequest::installation(c,s);return {{"target",ref->toJson()},{"device",c.device.toJson()},{"geometry",s.geometry.toJson()}};}
        throw std::runtime_error("Source setup changed; bind explicitly again");
    }
    void observeAcquisition(const Recipe& r,const CycleResult& cycle){const auto ref=SystemCalibration::setupReference(r);if(!ref)return;const auto source=SystemCalibration::source(r);if(source.kind!="webcam")return;for(const auto& result:cycle.tools)if(result.toolId==source.id && result.state==ExecutionState::Error){active_.disconnect(ref->camera,{{"reason",result.message},{"errorCode",result.errorCode},{"sourceId",source.id},{"cycleId",cycle.cycleId},{"origin","acquisition"}});return;}}
    QMap<QString,QVector<CalibrationModelRecord>> legacyHistories() const {
        QMap<QString,QVector<CalibrationModelRecord>> out;const auto catalog=repository_.catalog(gate_).records();
        for(const auto& r:catalog)if(r.encode()["schemaVersion"].toInt()<=3)out["Sistem • "+r.model().dependency().calibrationId].append(r);
        const auto at=QDateTime::currentDateTimeUtc();for(const auto& m:CalibrationService(path_+".calibrations.json").records()){
            const auto& d=m.dependency();std::optional<CalibrationModelRecord> original;for(const auto& r:catalog)if(r.encode()["schemaVersion"]==5){const auto old=r.encode()["original"].toObject();if(old["schemaVersion"]==3 && old["model"].toObject()==MeasurementService{}.encode(m)){original=CalibrationModelRecord::decode(old);break;}}
            if(!original){MeasurementSetup setup{"legacy:"+d.calibrationId,"image_file",CalibrationWizardSession::configurationId({{"legacyId",d.calibrationId}}),d.lensId,d.workingPlaneId,d.resolution,d.source,d.target};original=CalibrationModelRecord::adoptLegacy(m,setup,(d.calibrationId+QString(" v%1").arg(d.version)).left(128),at);}
            out["Eski matematik • "+d.calibrationId].append(*original);
        }return out;
    }
    QJsonObject exportDependencies() const {return repository_.exportBundle();}
    void importDependencies(const QJsonObject& bundle){if(!gate_())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul");repository_.importBundle(bundle);registry_.reload();}
    QString projectPath() const {return path_;}
    QString projectId() const {return project_;}
    bool changeAllowed() const {return gate_();}
    QString directory() const {return QFileInfo(path_).absolutePath();}
    QVector<CalibrationModelRecord> trials() const {QVector<CalibrationModelRecord> out;for(const auto& r:repository_.catalog(gate_).records())if(r.image().origin=="trial_file")out.append(r);return out;}
    void saveTrial(const CalibrationModelRecord& r){if(!gate_() || r.image().origin!="trial_file")throw std::runtime_error("Trial save unavailable");repository_.catalog(gate_).save(r);}
    std::function<CalibrationModelRecord()> associationJob(const CalibrationModelRecord& original,const SetupReference& ref,const QString& path){
        if(!gate_())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul");auto e=environment();IndependentCaptureRequest request;request.origin=SourceOrigin::SetupFile;request.filePath=path;
        for(const auto& c:e.current.cameras)if(c.id==ref.camera)request.camera=c;
        for(const auto& s:e.current.setups)if(s.id==ref.setup && s.revision==ref.revision)request.setup=s;
        request.validate();return [request,original]{IndependentCalibrationSession session;auto job=session.begin(request);auto result=IndependentCalibrationSession::run(job);
            if(!session.accept(result) || session.evidence()->pixelHash!=original.image().pixelHash)throw std::runtime_error("Özgün raster eksik/değişmiş veya kurulum uyumsuz.");
            return CalibrationModelRecord::associate(original,*session.acceptedSetup(),(original.name()+" • ilişkilendirme").left(128),original.recordedAt());};
    }
    CalibrationModelRecord saveAssociation(const SetupReference& ref,const CalibrationModelRecord& derived){
        if(!gate_())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul");
        for(const auto& existing:active_.records(ref))if(existing.contentId()==derived.contentId())return existing;
        active_.save(ref,derived);return derived;
    }
    CalibrationModelRecord associate(const CalibrationModelRecord& original,const SetupReference& ref,const QString& path){return saveAssociation(ref,associationJob(original,ref,path)());}

private:
    QString path_,project_;
    MeasurementEnvironmentRepository repository_;
    MeasurementEnvironmentRegistry registry_;
    ScopedActiveCalibrationService active_;
    ActiveCalibrationService legacy_;
    ActiveCalibrationService::ChangeGate gate_;
    QVector<DeviceBinding> discovered_;
    bool discoveryDone_=false;
    CameraInventory inventory_;
    bool inventoryDone_=false;
    std::function<QVector<DeviceBinding>()> discovery_;
};
}
