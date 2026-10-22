#include "SystemCalibration.hpp"
#include "ScopedActiveCalibration.hpp"
#include "domain/ReferenceConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "domain/IndustrialSourceReference.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QString digest(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
void valid(const ActiveCalibrationSnapshot& snapshot){const auto r=CalibrationModelRecord::decode(snapshot.record.encode());const auto& d=r.model().dependency();require(snapshot.selection.id==d.calibrationId && snapshot.selection.version==d.version && snapshot.selection.contentId==r.contentId() && snapshot.selection.revision>0 && snapshot.selection.revision<=1000000000 && r.activatable(),"Invalid active model-ready selection/evidence");
    if(!snapshot.scope.isEmpty()){
        const auto o=snapshot.scope;const bool industrial=o["schemaVersion"]==2;
        require(o.size()==(industrial?7:5) && o["setup"].isObject() && (industrial || o["schemaVersion"]==1) && o["target"].isObject() && o["device"].isObject() && o["geometryId"].isString(),"Invalid scoped calibration envelope");
        const auto ref=SetupReference::fromJson(o["target"].toObject());const auto device=DeviceBinding::fromJson(o["device"].toObject());const auto setup=MeasurementSetupDescriptor::fromJson(o["setup"].toObject());
        require(setup.camera==ref.camera && setup.id==ref.setup && setup.revision==ref.revision && setup.deviceRevision==device.revision && setup.geometryId()==o["geometryId"].toString(),"Scope descriptor mismatch");
        MeasurementCamera camera{ref.camera,"scope",device};
        if(industrial){
            require(o["physical"].isObject() && (o["profile"].isNull() || o["profile"].isObject()),"Missing industrial scope identity/profile");
            camera.physical=PhysicalDeviceIdentity::fromJson(o["physical"].toObject());
            if(o["profile"].isObject()){
                const auto profile=AcquisitionProfile::fromJson(o["profile"].toObject());
                require(profile.camera==ref.camera && profile.setup==ref.setup && profile.requested.trigger.enabled && profile.requested.trigger.source=="Software","Industrial scope profile mismatch");
            }
        }
        require(r.setup()==IndependentCaptureRequest::installation(camera,setup),"Scoped physical installation/evidence mismatch");
    }
    if(!snapshot.acquisition.isEmpty()){
        const auto a=snapshot.acquisition;
        require(!snapshot.scope.isEmpty() && a.size()==3 && a["schemaVersion"]==1 && a["probe"].isObject(),"Invalid session acquisition envelope");
        const auto probe=MeasurementSourceEvidence::fromJson(a["probe"].toObject());
        require(!probe.experimental && probe.origin==SourceOrigin::CameraAcquired && probe.sourceKind==SourceIdentityKind::CalibrationSession && probe.target && probe.target->toJson()==snapshot.scope["target"].toObject(),"Session acquisition target/origin mismatch");
        if(snapshot.scope["schemaVersion"]==2){require(a["backend"].isObject() && snapshot.scope["profile"].isObject(),"Missing session backend/profile");BackendIdentity::fromJson(a["backend"].toObject());}
        else require(a["backend"].isNull(),"Webcam cannot claim industrial backend");
    }}

QJsonObject selection(const ActiveCalibrationSelection& s){return {{"id",s.id},{"version",s.version},{"contentId",s.contentId},{"revision",s.revision}};}
}
Recipe SystemCalibration::branch(const Recipe& recipe,const QString& source) {
    auto r=recipe;const auto tools=editorTools(r.definition);const auto connections=editorConnections(r.definition);const auto graph=validateGraph(tools,connections);
    QJsonArray array;QVector<ToolConnection> links;QSet<QString> ids;
    for(const auto& t:tools)if(t.id==source || graph.toolSources.value(t.id)==source){array.append(encodeEditorTool(t,9));ids.insert(t.id);}
    for(const auto& c:connections)if(ids.contains(c.sourceTool) && ids.contains(c.targetTool))links.append(c);
    r.definition["editor"]=QJsonObject{{"schemaVersion",9},{"tools",array},{"connections",QJsonArray{}}};r.definition=withEditorConnections(r.definition,links);
    auto binding=r.definition["sourceSetups"].toObject()[source].toObject();const auto legacy=r.definition["sourceSetup"].toObject();if(binding.isEmpty() && legacy["sourceToolId"]==source)binding=legacy;
    r.definition.remove("sourceSetups");r.definition.remove("sourceSetup");if(!binding.isEmpty())r.definition["sourceSetup"]=binding;
    r.definition["calibrationPolicy"]="scoped-source/1";r.calibrationBinding.reset();return r;
}
QJsonObject SystemCalibration::sourceBinding(const QString& source,const SetupReference& target){target.validate();return {{"schemaVersion",1},{"sourceToolId",source},{"target",target.toJson()}};}
std::optional<SetupReference> SystemCalibration::setupReference(const Recipe& r){if(!r.definition.contains("sourceSetup"))return {};const auto o=r.definition["sourceSetup"].toObject();require(o.size()==3 && o["schemaVersion"]==1 && o["sourceToolId"]==source(r).id && o["target"].isObject(),"Invalid source/setup binding");return SetupReference::fromJson(o["target"].toObject());}
ToolDefinition SystemCalibration::source(const Recipe& recipe){std::optional<ToolDefinition> source;for(const auto& t:editorTools(recipe.definition))if(t.enabled && ToolRegistry::get(t.kind).acquisitionSource){require(!source,"Tek enabled edinim kaynağı gerekir.");source=t;}require(source.has_value(),"Enabled edinim kaynağı gerekli.");return *source;}
bool SystemCalibration::requestsCalibration(const Recipe& recipe,bool optionalPoints){for(const auto& t:editorTools(recipe.definition))if(t.enabled && (ToolRegistry::get(t.kind).usesCalibration || t.parameters["mode"]=="calibrated" || (optionalPoints && t.kind=="edge_point") || (t.parameters.contains("referenceBinding") && toolReferenceBinding(t).world) || t.parameters["poseTracking"].toObject()["schemaVersion"]==2))return true;return false;}
CalibrationDependency SystemCalibration::binding(const ActiveCalibrationSnapshot& s,const QString& tool,const QString& coordinates){
    valid(s);require(!tool.trimmed().isEmpty() && !tool.startsWith("calibration-test:") && (coordinates=="image" || coordinates==tool+"/image"),"Invalid actual execution source/frame");
    auto d=s.record.model().dependency();d.cameraId=tool;d.source.id=coordinates;
    QJsonObject identity{{"policy","system-model-ready/1"},{"recordContentId",s.record.contentId()},{"selection",selection(s.selection)},{"sourceToolId",tool},{"sourceCoordinates",coordinates}};if(!s.scope.isEmpty())identity["scope"]=s.scope;if(!s.acquisition.isEmpty())identity["acquisition"]=s.acquisition;d.contentId=digest(identity);d.validate();return d;
}
PlanarCalibration SystemCalibration::executionModel(const ActiveCalibrationSnapshot& s,const QString& tool,const QString& coordinates){
    // Ephemeral geometry adapter: exact raw raster coordinate domain and actual
    // acquisition source are bound separately from immutable teaching evidence.
    auto c=s.record.model();c.dependency_=binding(s,tool,coordinates);c.readyRecordContentId_=s.record.contentId();return c;
}
QJsonObject SystemCalibration::evidence(const ActiveCalibrationSnapshot& s,const QString& tool,const QString& coordinates){QJsonObject result{{"schemaVersion",!s.acquisition.isEmpty() ? 3:(s.scope.isEmpty() ? 1:2)},{"policy","system-model-ready/1"},{"record",s.record.encode()},{"selection",selection(s.selection)},{"executionDependency",binding(s,tool,coordinates).toJson()}};if(!s.scope.isEmpty())result["scope"]=s.scope;if(!s.acquisition.isEmpty())result["acquisition"]=s.acquisition;return result;}
ActiveCalibrationSnapshot SystemCalibration::decodeEvidence(const QJsonObject& o){
    const bool session=o["schemaVersion"]==3;const bool scoped=session || o["schemaVersion"]==2;require(o.size()==(session ? 7:(scoped ? 6:5)) && (scoped || o["schemaVersion"]==1) && o["policy"]=="system-model-ready/1" && o["record"].isObject() && o["selection"].isObject() && o["executionDependency"].isObject(),"Invalid system calibration envelope");
    const auto selection=o["selection"].toObject();require(selection.size()==4 && selection["id"].isString() && selection["contentId"].isString() && selection["version"].isDouble() && selection["version"].toDouble()==selection["version"].toInt() && selection["revision"].isDouble() && selection["revision"].toDouble()==selection["revision"].toInt(),"Invalid system selection schema");
    ActiveCalibrationSnapshot s{CalibrationModelRecord::decode(o["record"].toObject()),{selection["id"].toString(),selection["contentId"].toString(),selection["version"].toInt(),selection["revision"].toInt()},scoped ? o["scope"].toObject():QJsonObject{},session ? o["acquisition"].toObject():QJsonObject{}};require((!scoped || !s.scope.isEmpty()) && (!session || !s.acquisition.isEmpty()),"Missing physical scope/session evidence");valid(s);const auto d=CalibrationDependency::fromJson(o["executionDependency"].toObject());require(evidence(s,d.cameraId,d.source.id)==o,"System calibration binding/content mismatch");return s;
}
void SystemCalibration::compatible(const ActiveCalibrationSnapshot& s,const Recipe& recipe,const QString& directory,const QSize& size){
    valid(s);const auto tool=source(recipe);const auto& setup=s.record.setup();
    // Configuration preparation compares requirements only. No raster or
    // acquisition/measurement is fabricated here.
    if(!s.scope.isEmpty()){
        const auto ref=setupReference(recipe);require(ref && s.scope["target"].toObject()==ref->toJson(),"Recipe requires a different physical setup");
        bool adapter=tool.kind=="image_file" || tool.kind==setup.adapter;
        if(tool.kind=="industrial_camera"){
            require(s.scope["schemaVersion"]==2,"Industrial source requires industrial calibration scope");
            const auto requested=IndustrialSourceReference::fromJson(tool.parameters["sourceReference"].toObject());
            const auto device=DeviceBinding::fromJson(s.scope["device"].toObject());
            require(requested.physical.toJson()==s.scope["physical"].toObject() && requested.deviceRevision==device.revision &&
                requested.camera==ref->camera && requested.setup==ref->setup && requested.setupRevision==ref->revision,
                "Industrial calibration physical identity/revision mismatch");
            adapter=true;
        }
        require(adapter && size==setup.resolution,"Scoped source geometry mismatch");
    }else {require(!setupReference(recipe),"Scoped recipe cannot use legacy global calibration");const auto actual=CalibrationCapture::configuration(tool,directory);
        require(tool.kind==setup.adapter && actual==setup.configId && size==setup.resolution,"Aktif kalibrasyon bu görüntü kaynağına/çözünürlüğüne uygun değil.");}
    const auto declared=recipe.definition["planarSetup"].toObject();if(!declared.isEmpty())require(declared["lensId"]==setup.lensId && declared["workingPlaneId"]==setup.planeId,"Eski reçetenin lens/düzlem beyanı ortak kurulumdan farklı; açıkça düzenleyin.");
}
MeasurementExecutionContext SystemCalibration::resolve(const ActiveCalibrationSnapshot& s,const Recipe& recipe,const QString& directory,const ImageFrame& image){
    MeasurementExecutionContext result;result.rawSize=image.size();try{compatible(s,recipe,directory,image.size());const auto tool=source(recipe);const auto& m=image.metadata();require(m.sourceId==tool.id && !m.frameId.isEmpty() && !m.cycleId.isEmpty() && m.completedAtUtc.isValid(),"System calibration actual frame provenance mismatch");auto c=executionModel(s,tool.id,m.coordinateSystemId);result.current=c.dependency();result.calibration=std::make_shared<const PlanarCalibration>(std::move(c));}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;
}
CalibrationDependency SystemCalibration::experimentalBinding(const ExperimentalScopedCalibrationSnapshot& typed,const QString& tool,const QString& coordinates){
    const auto& s=typed.snapshot.calibration;const auto record=CalibrationModelRecord::decode(s.record.encode());const auto& original=record.model().dependency();
    require(record.experimentallyActivatable() && s.selection.contentId==record.contentId() && s.selection.id==original.calibrationId && s.selection.version==original.version && s.selection.revision>0,"Invalid experimental selection");
    const auto a=s.acquisition;require(a["schemaVersion"]==2 && a["policy"]=="experimental-calibration/1" && a["productionAdmission"]==false && a["triggerAssociationVerified"]==false && a["physicalAcceptance"]==false,"Experimental session evidence required");
    const auto proof=MeasurementSourceEvidence::fromJson(a["probe"].toObject());require(proof.experimental && proof.origin==SourceOrigin::CameraAcquired && proof.target && *proof.target==typed.snapshot.target && proof.target->toJson()==s.scope["target"].toObject(),"Experimental live permission target mismatch");
    auto descriptor=MeasurementSetupDescriptor::fromJson(s.scope["setup"].toObject());descriptor.name=proof.experimental->setup.name;
    require(s.scope["device"].toObject()==proof.experimental->camera.device.toJson() && s.scope["schemaVersion"]==2 && proof.experimental->camera.physical->toJson()==s.scope["physical"].toObject() && proof.experimental->setup==descriptor && proof.experimental->profile.toJson()==s.scope["profile"].toObject() && record.setup()==IndependentCaptureRequest::installation(proof.experimental->camera,proof.experimental->setup),"Experimental permission installation mismatch");
    require(!tool.isEmpty() && !tool.startsWith("calibration-session:") && (coordinates=="image" || coordinates==tool+"/image"),"Invalid experimental execution source");
    auto d=original;d.cameraId=tool;d.source.id=coordinates;
    d.contentId=digest({{"policy","experimental-editor-model/1"},{"record",record.contentId()},{"selection",selection(s.selection)},{"scope",s.scope},{"acquisition",a},{"source",tool},{"coordinates",coordinates}});d.validate();return d;
}
MeasurementExecutionContext SystemCalibration::resolveExperimental(const ExperimentalScopedCalibrationSnapshot& typed,const Recipe& recipe,const ImageFrame& image){
    MeasurementExecutionContext result;result.rawSize=image.size();try{
        const auto& s=typed.snapshot.calibration;const auto tool=source(recipe);experimentalBinding(typed,tool.id,image.metadata().coordinateSystemId);const auto ref=setupReference(recipe);require(ref && *ref==typed.snapshot.target,"Experimental source requires exact setup binding");
        const auto probe=MeasurementSourceEvidence::fromJson(s.acquisition["probe"].toObject());const auto& expected=*probe.experimental;
        ExperimentalCalibrationEvidence actual{tool.kind=="image_file" ? SourceOrigin::SetupFile:SourceOrigin::CameraAcquired,expected.camera,expected.setup,expected.profile,image.metadata().experimentalAcquisition};actual.validate();
        require(image.size()==expected.setup.geometry.resolution && image.metadata().sourceId==tool.id && !image.metadata().frameId.isEmpty() && !image.metadata().cycleId.isEmpty() && image.metadata().completedAtUtc.isValid(),"Experimental execution raster/source mismatch");
        if(image.processing().isEmpty())actual.verify(image);
        require(tool.kind=="image_file" || tool.kind=="industrial_camera","Experimental source adapter mismatch");
        if(tool.kind=="industrial_camera"){
            const auto request=IndustrialSourceReference::fromJson(tool.parameters["sourceReference"].toObject());request.validateAgainst(MeasurementSetupContract{{expected.camera},{expected.setup},{expected.profile}});
        }
        const auto declared=recipe.definition["planarSetup"].toObject();require(declared.isEmpty() || (declared["lensId"]==s.record.setup().lensId && declared["workingPlaneId"]==s.record.setup().planeId),"Recipe optical declaration differs");
        auto model=s.record.model();model.dependency_=experimentalBinding(typed,tool.id,image.metadata().coordinateSystemId);model.readyRecordContentId_=s.record.contentId();result.current=model.dependency();result.calibration=std::make_shared<const PlanarCalibration>(std::move(model));
    }catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;
}
QStringList SystemCalibration::impact(const ActiveCalibrationSnapshot& s,const Recipe& recipe,const QString& directory){
    QStringList notes;try{compatible(s,recipe,directory,s.record.setup().resolution);notes<<recipe.name+": kaynak/kurulum uyumlu";}catch(const std::exception& e){notes<<recipe.name+": "+QString::fromUtf8(e.what());return notes;}
    for(const auto& t:editorTools(recipe.definition))if(t.enabled){if(QStringList{"edge_pair","line_fit","circle_fit","geometry_measurement"}.contains(t.kind) && t.parameters["mode"]=="pixel")notes<<t.name+": px tanısı/toleransı korunur; mm için açık birim düzenlemesi gerekir";
        if(t.parameters.contains("referenceBinding") && toolReferenceBinding(t).world)notes<<t.name+": exact world ROI/referansı yeni binding ile yeniden öğretilmeli";
        if(t.kind=="circle_fit" && t.parameters["mode"]=="calibrated" && !t.parameters["scanRoi"].toObject().isEmpty())notes<<t.name+": nominal mm Circle ROI exact worldCalibration bağıyla denetlenir; model değişiminde yeniden öğretin";
        if(t.parameters.contains("poseTracking"))notes<<t.name+": eski pose/world ilişkisi yeniden öğretim incelemesi gerektirir";}
    return notes;
}
Recipe SystemCalibration::prepareRecipe(const ActiveCalibrationSnapshot& s,const Recipe& recipe,const QString& directory){compatible(s,recipe,directory,s.record.setup().resolution);auto result=recipe;const auto tool=source(recipe);result.calibrationBinding=binding(s,tool.id,tool.id+"/image");result.calibrationReference=result.calibrationBinding->calibrationId;result.schemaVersion=2;
    for(const auto& t:editorTools(recipe.definition))if(t.enabled && t.kind=="circle_fit" && t.parameters["mode"]=="calibrated" && !t.parameters.contains("referenceBinding") && !t.parameters["scanRoi"].toObject().isEmpty()){require(t.parameters["worldCalibration"].isObject(),"Eski nominal mm Circle ROI ortak modelde yeniden öğretilmeli.");CalibrationDependency::fromJson(t.parameters["worldCalibration"].toObject()).requireCurrent(*result.calibrationBinding);}return result;}
}
