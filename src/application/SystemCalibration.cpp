#include "SystemCalibration.hpp"
#include "domain/ReferenceConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QSet>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QString digest(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
void valid(const ActiveCalibrationSnapshot& snapshot){const auto r=CalibrationModelRecord::decode(snapshot.record.encode());const auto& d=r.model().dependency();require(snapshot.selection.id==d.calibrationId && snapshot.selection.version==d.version && snapshot.selection.contentId==r.contentId() && snapshot.selection.revision>0 && snapshot.selection.revision<=1000000000 && r.activatable(),"Invalid active model-ready selection/evidence");
    if(!snapshot.scope.isEmpty()){const auto o=snapshot.scope;require(o.size()==5 && o["setup"].isObject() && o["schemaVersion"]==1 && o["target"].isObject() && o["device"].isObject() && o["geometryId"].isString(),"Invalid scoped calibration envelope");const auto ref=SetupReference::fromJson(o["target"].toObject());const auto device=DeviceBinding::fromJson(o["device"].toObject());const auto setup=MeasurementSetupDescriptor::fromJson(o["setup"].toObject());require(setup.camera==ref.camera && setup.id==ref.setup && setup.revision==ref.revision && setup.deviceRevision==device.revision && setup.geometryId()==o["geometryId"].toString() && setup.opticalProfile==r.setup().lensId && setup.workingPlane==r.setup().planeId && setup.geometry.resolution==r.setup().resolution,"Scope descriptor mismatch");require(r.setup().id==ref.setup.value+"/"+QString::number(ref.revision) && r.setup().adapter==device.adapter && r.setup().configId==CalibrationWizardSession::configurationId({{"geometryId",o["geometryId"]},{"device",device.toJson()}}),"Scoped physical installation/evidence mismatch");}}
QJsonObject selection(const ActiveCalibrationSelection& s){return {{"id",s.id},{"version",s.version},{"contentId",s.contentId},{"revision",s.revision}};}
}
QJsonObject SystemCalibration::sourceBinding(const QString& source,const SetupReference& target){target.validate();return {{"schemaVersion",1},{"sourceToolId",source},{"target",target.toJson()}};}
std::optional<SetupReference> SystemCalibration::setupReference(const Recipe& r){if(!r.definition.contains("sourceSetup"))return {};const auto o=r.definition["sourceSetup"].toObject();require(o.size()==3 && o["schemaVersion"]==1 && o["sourceToolId"]==source(r).id && o["target"].isObject(),"Invalid source/setup binding");return SetupReference::fromJson(o["target"].toObject());}
ToolDefinition SystemCalibration::source(const Recipe& recipe){std::optional<ToolDefinition> source;for(const auto& t:editorTools(recipe.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){require(!source,"Tek enabled edinim kaynağı gerekir.");source=t;}require(source.has_value(),"Image File veya Webcam kaynağı gerekli.");return *source;}
bool SystemCalibration::requestsCalibration(const Recipe& recipe,bool optionalPoints){for(const auto& t:editorTools(recipe.definition))if(t.enabled && (ToolRegistry::get(t.kind).usesCalibration || t.parameters["mode"]=="calibrated" || (optionalPoints && t.kind=="edge_point") || (t.parameters.contains("referenceBinding") && toolReferenceBinding(t).world) || t.parameters["poseTracking"].toObject()["schemaVersion"]==2))return true;return false;}
CalibrationDependency SystemCalibration::binding(const ActiveCalibrationSnapshot& s,const QString& tool,const QString& coordinates){
    valid(s);require(!tool.trimmed().isEmpty() && !tool.startsWith("calibration-test:") && (coordinates=="image" || coordinates==tool+"/image"),"Invalid actual execution source/frame");
    auto d=s.record.model().dependency();d.cameraId=tool;d.source.id=coordinates;
    QJsonObject identity{{"policy","system-model-ready/1"},{"recordContentId",s.record.contentId()},{"selection",selection(s.selection)},{"sourceToolId",tool},{"sourceCoordinates",coordinates}};if(!s.scope.isEmpty())identity["scope"]=s.scope;d.contentId=digest(identity);d.validate();return d;
}
PlanarCalibration SystemCalibration::executionModel(const ActiveCalibrationSnapshot& s,const QString& tool,const QString& coordinates){
    // Ephemeral geometry adapter: exact raw raster coordinate domain and actual
    // acquisition source are bound separately from immutable teaching evidence.
    auto c=s.record.model();c.dependency_=binding(s,tool,coordinates);c.readyRecordContentId_=s.record.contentId();return c;
}
QJsonObject SystemCalibration::evidence(const ActiveCalibrationSnapshot& s,const QString& tool,const QString& coordinates){QJsonObject result{{"schemaVersion",s.scope.isEmpty() ? 1:2},{"policy","system-model-ready/1"},{"record",s.record.encode()},{"selection",selection(s.selection)},{"executionDependency",binding(s,tool,coordinates).toJson()}};if(!s.scope.isEmpty())result["scope"]=s.scope;return result;}
ActiveCalibrationSnapshot SystemCalibration::decodeEvidence(const QJsonObject& o){
    const bool scoped=o["schemaVersion"]==2;require(o.size()==(scoped ? 6:5) && (scoped || o["schemaVersion"]==1) && o["policy"]=="system-model-ready/1" && o["record"].isObject() && o["selection"].isObject() && o["executionDependency"].isObject(),"Invalid system calibration envelope");
    const auto selection=o["selection"].toObject();require(selection.size()==4 && selection["id"].isString() && selection["contentId"].isString() && selection["version"].isDouble() && selection["version"].toDouble()==selection["version"].toInt() && selection["revision"].isDouble() && selection["revision"].toDouble()==selection["revision"].toInt(),"Invalid system selection schema");
    ActiveCalibrationSnapshot s{CalibrationModelRecord::decode(o["record"].toObject()),{selection["id"].toString(),selection["contentId"].toString(),selection["version"].toInt(),selection["revision"].toInt()},scoped ? o["scope"].toObject():QJsonObject{}};require(!scoped || !s.scope.isEmpty(),"Missing physical scope");valid(s);const auto d=CalibrationDependency::fromJson(o["executionDependency"].toObject());require(evidence(s,d.cameraId,d.source.id)==o,"System calibration binding/content mismatch");return s;
}
void SystemCalibration::compatible(const ActiveCalibrationSnapshot& s,const Recipe& recipe,const QString& directory,const QSize& size){
    valid(s);const auto tool=source(recipe);const auto& setup=s.record.setup();
    // Configuration preparation compares requirements only. No raster or
    // acquisition/measurement is fabricated here.
    if(!s.scope.isEmpty()){
        const auto ref=setupReference(recipe);require(ref && s.scope["target"].toObject()==ref->toJson(),"Recipe requires a different physical setup");
        require((tool.kind=="image_file" || tool.kind==setup.adapter) && size==setup.resolution,"Scoped source geometry mismatch");
    }else {require(!setupReference(recipe),"Scoped recipe cannot use legacy global calibration");const auto actual=CalibrationCapture::configuration(tool,directory);
        require(tool.kind==setup.adapter && actual==setup.configId && size==setup.resolution,"Aktif kalibrasyon bu görüntü kaynağına/çözünürlüğüne uygun değil.");}
    const auto declared=recipe.definition["planarSetup"].toObject();if(!declared.isEmpty())require(declared["lensId"]==setup.lensId && declared["workingPlaneId"]==setup.planeId,"Eski reçetenin lens/düzlem beyanı ortak kurulumdan farklı; açıkça düzenleyin.");
}
MeasurementExecutionContext SystemCalibration::resolve(const ActiveCalibrationSnapshot& s,const Recipe& recipe,const QString& directory,const ImageFrame& image){
    MeasurementExecutionContext result;result.rawSize=image.size();try{compatible(s,recipe,directory,image.size());const auto tool=source(recipe);const auto& m=image.metadata();require(m.sourceId==tool.id && !m.frameId.isEmpty() && !m.cycleId.isEmpty() && m.completedAtUtc.isValid(),"System calibration actual frame provenance mismatch");auto c=executionModel(s,tool.id,m.coordinateSystemId);result.current=c.dependency();result.calibration=std::make_shared<const PlanarCalibration>(std::move(c));}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;
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
