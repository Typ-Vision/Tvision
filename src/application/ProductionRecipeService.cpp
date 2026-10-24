#include "MeasurementDiagnostic.hpp"
#include "CircleToolService.hpp"
#include "GeometryMeasurementService.hpp"
#include "ProductionRecipeService.hpp"
#include "MatchService.hpp"
#include "ToolResultCodec.hpp"
#include "ProductionResultCodec.hpp"
#include "ToolDependencyResolver.hpp"
#include "persistence/ProjectRepository.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QUuid>
#include <QSet>
#include <QDataStream>
#include "domain/ReferenceConfig.hpp"
#include "SystemCalibration.hpp"
#include "SourceFrameIndex.hpp"
#include "IndustrialRuntime.hpp"
#include <cmath>
#include <algorithm>

namespace visionflow {
namespace {
[[noreturn]] void fail(ProductionError code,const QString& text) {throw ProductionException(code,text);}
QByteArray hash(const QJsonObject& object) {return QCryptographicHash::hash(QJsonDocument(object).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();}
QByteArray content(const Recipe& r,const QString& directory) {
    return hash({{"recipeId",r.id},{"recipeVersion",r.recipeVersion},{"schemaVersion",r.schemaVersion},
        {"name",r.name},{"description",r.description},{"type",typeKey(r.applicationType)},
        {"camera",r.cameraReference},{"calibration",r.calibrationReference},
        {"calibrationBinding",r.calibrationBinding ? QJsonValue(r.calibrationBinding->toJson()) : QJsonValue(QJsonValue::Null)},
        {"definition",r.definition},{"projectDirectory",QDir(directory).absolutePath()}});
}
bool sha256(const QString& s) {return s.size()==64 && std::all_of(s.begin(),s.end(),[](QChar c){return (c>='0' && c<='9') || (c>='a' && c<='f');});}
bool trustedCompletionRequired(const QVector<ToolDefinition>& tools) {
    return std::any_of(tools.cbegin(),tools.cend(),[](const auto& t){return t.enabled &&
        (t.parameters.contains("referenceBinding") || t.kind=="pattern_pose" || t.kind=="shape_pose");});
}
bool terminal(ExecutionState s) {return s==ExecutionState::Done || s==ExecutionState::Error || s==ExecutionState::Skipped;}
}
QByteArray ProductionRecipeService::recipeContent(const Recipe& r,const QString& directory){return content(r,directory);}
QJsonObject ProductionRecipeService::validateMultiSourceCompletion(const ProductionCycle& lease,const CycleResult& r,const QMap<QString,std::shared_ptr<const ProductionSnapshot>>& branches){
    QJsonObject evidence;
    auto invalid=[&](bool ok,const char* text){if(!ok)fail(ProductionError::InvalidCompletion,text);};
    invalid(lease.snapshot && lease.ticket && r.completionSnapshot_==lease.snapshot && r.completionTicket_==lease.ticket && !r.completionDigest_.isEmpty() && r.completionDigest_==completionDigest(r),"Missing/changed/foreign multi-source trusted completion");
    invalid((lease.snapshot->validation().scope=="multi_source_in_memory/1" || lease.snapshot->validation().scope=="multi_source_runtime/1") && !r.partial && r.targetToolId.isEmpty() && terminal(r.state) && r.recipeId==lease.snapshot->recipe().id && r.configId==lease.snapshot->validation().contentId && !r.cycleId.isEmpty(),"Invalid multi-source completion scope/identity");
    const auto configured=editorTools(lease.snapshot->recipe().definition);const auto graph=validateSourceGraph(configured,editorConnections(lease.snapshot->recipe().definition));invalid(graph.valid() && branches.size()==graph.requiredSources.size(),"Snapshot graph/source set mismatch");
    QStringList acquired;for(const auto& frame:r.frames){invalid(graph.requiredSources.contains(frame.sourceId) && !acquired.contains(frame.sourceId),"Duplicate/foreign acquired source");acquired.append(frame.sourceId);}
    if(!r.frames.isEmpty()){SourceFrameIndex index(r.cycleId,acquired,r.frames);for(const auto& tool:r.tools)for(const auto& value:tool.outputs)if(value.valid)index.validatePort(graph.toolSources.value(tool.toolId),value,value.port);}
    else for(const auto& tool:r.tools)for(const auto& value:tool.outputs)invalid(!value.valid,"Output without acquired frame");
    if(r.state==ExecutionState::Done){invalid(acquired.size()==graph.requiredSources.size(),"DONE missing required source");SourceFrameIndex index(r.cycleId,graph.requiredSources,r.frames);invalid(r.frameId==index.legacyFrameId(),"Wrong compatibility frame ID");}
    else if(graph.requiredSources.size()>1)invalid(r.frameId.isEmpty(),"Multi-source error invented shared frame ID");
    for(auto i=r.industrialEvidence.cbegin();i!=r.industrialEvidence.cend();++i){invalid(i.value() && branches.contains(i.key()) && branches[i.key()]->dependencies().contains("industrialSource"),"Foreign industrial evidence");bool matched=false;for(const auto& t:r.tools)if(t.toolId==i.key())for(const auto& v:t.outputs)if(v.valid && std::holds_alternative<ImageFrame>(v.data)){const auto& image=std::get<ImageFrame>(v.data);invalid(image.contentId()==i.value()->pixelHash && image.metadata().frameId==i.value()->execution.frameId && image.metadata().cycleId==i.value()->execution.cycleId,"Industrial pixels/frame differ from acquisition");matched=true;}invalid(matched,"Industrial evidence without actual raster");}
    QSet<QString> seen;for(const auto& tool:r.tools){invalid(!seen.contains(tool.toolId) && std::any_of(configured.cbegin(),configured.cend(),[&](const auto& t){return t.id==tool.toolId;}),"Duplicate/foreign completed tool");seen.insert(tool.toolId);if(!graph.requiredSources.contains(graph.toolSources.value(tool.toolId)))invalid(tool.state==ExecutionState::Skipped && tool.outputs.isEmpty() && tool.decision==PartDecision::NotEvaluated && !tool.calibration,"Disabled branch carried output");}
    if(r.state==ExecutionState::Done)invalid(seen.size()==configured.size(),"DONE missing tool results");
    for(const auto& source:graph.requiredSources){const auto snapshot=branches.value(source);invalid(bool(snapshot) && snapshot->dependencies()==lease.snapshot->dependencies()["sourceSet"].toObject()[source].toObject(),"Branch dependency set mismatch");
        // Frozen dependency accessor, no live model resolution during completion.
        ProductionRecipeService verifier([](const QString&,ProductionAction){return true;});verifier.active_=snapshot;verifier.cycle_={lease.ticket,snapshot};verifier.cyclePrincipal_=snapshot->validation().principal;verifier.cycleBeginId_=QUuid::createUuid().toString(QUuid::WithoutBraces);verifier.cycleStarted_=QDateTime::currentDateTimeUtc();
        verifier.completionResolver_=[snapshot](const ImageFrame& image){if(snapshot->dependencies().contains("systemCalibration"))return SystemCalibration::resolve(SystemCalibration::decodeEvidence(snapshot->dependencies()["systemCalibration"].toObject()),snapshot->recipe(),snapshot->projectDirectory(),image);MeasurementExecutionContext c;c.rawSize=image.size();c.error="Raw-only immutable source";return c;};
        auto part=r;part.industrialEvidence.clear();if(r.industrialEvidence.contains(source))part.industrialEvidence[source]=r.industrialEvidence[source];part.configId=snapshot->validation().contentId;part.frames.clear();part.tools.clear();part.executedTools.clear();part.frameId.clear();
        for(const auto& frame:r.frames)if(frame.sourceId==source){part.frames.append(frame);part.frameId=frame.frameId;}
        for(const auto& tool:r.tools)if(graph.toolSources.value(tool.toolId)==source)part.tools.append(tool);
        for(const auto& id:r.executedTools)if(graph.toolSources.value(id)==source)part.executedTools.append(id);part.acquisitions=part.executedTools.contains(source) ? 1:0;
        part.completionSnapshot_=snapshot;part.completionTicket_=lease.ticket;part.completionDigest_=completionDigest(part);verifier.finishCycle(lease.ticket,part);
        evidence[source]=verifier.cycleRecords().last();
    }
    return evidence;
}
QByteArray ProductionRecipeService::completionDigest(const CycleResult& r) {
    QByteArray bytes;QDataStream out(&bytes,QIODevice::WriteOnly);
    out<<r.cycleId<<r.frameId<<r.recipeId<<r.evaluationId<<r.targetToolId<<r.partial<<r.executedTools
       <<r.acquisitions<<r.configId<<int(r.state)<<r.milliseconds<<r.error;
    auto metadata=[&](const FrameMetadata& m){out<<m.sourceId<<m.frameId<<m.cycleId<<m.coordinateSystemId<<m.completedAtUtc
        <<bool(m.exposureAtUtc)<<m.exposureAtUtc.value_or(QDateTime{})<<bool(m.deviceTimestampTicks)
        <<quint64(m.deviceTimestampTicks.value_or(0))<<m.deviceClockId<<QJsonDocument(m.experimentalAcquisition).toJson(QJsonDocument::Compact);};
    out<<r.industrialEvidence.size();for(auto it=r.industrialEvidence.cbegin();it!=r.industrialEvidence.cend();++it){out<<it.key()<<bool(it.value());if(it.value())out<<QJsonDocument(industrialCompletionEvidence(*it.value())).toJson(QJsonDocument::Compact);}
    out<<r.frames.size();for(const auto& f:r.frames)metadata(f);
    out<<r.tools.size();for(const auto& t:r.tools) {
        out<<t.toolId<<int(t.state)<<t.message<<t.milliseconds<<int(t.decision)<<t.errorCode
           <<QJsonDocument(t.calibration ? t.calibration->toJson():QJsonObject{}).toJson(QJsonDocument::Compact)
           <<t.regions<<t.points<<t.rejectedPoints<<t.lines<<t.searchRegion<<t.mask.pixels<<t.mask.size<<t.mask.origin;
        out<<t.outputs.size();for(auto it=t.outputs.cbegin();it!=t.outputs.cend();++it) {
            out<<it.key()<<QJsonDocument(ProductionResultCodec::output(*it)).toJson(QJsonDocument::Compact);
            metadata(it->origin);
            if(it->valid && std::holds_alternative<ImageFrame>(it->data)) {
                const auto& image=std::get<ImageFrame>(it->data);metadata(image.metadata());
                out<<image.contentId()<<image.processingId()<<image.size()<<int(image.format());
            }
        }
        out<<bool(t.fitDiagnostic);if(t.fitDiagnostic)out<<QJsonDocument(encodeMeasurementDiagnostic(*t.fitDiagnostic)).toJson(QJsonDocument::Compact);
        out<<bool(t.edgeDiagnostics);if(t.edgeDiagnostics) {
            const auto& d=*t.edgeDiagnostics;out<<d.scan<<d.width<<d.length<<d.sampleCount<<d.derivativeCount<<d.candidateCount<<d.derivative;
            out<<d.profile.size();for(const auto& v:d.profile)out<<v.distance<<v.raw<<bool(v.smooth)<<v.smooth.value_or(0);
            out<<d.candidates.size();for(const auto& v:d.candidates)out<<v.point<<v.distance<<v.strength<<v.accepted<<v.selected<<v.reason<<v.refinement;
        }
    }
    return QCryptographicHash::hash(bytes,QCryptographicHash::Sha256);
}
ProductionException::ProductionException(ProductionError code,const QString& message):std::runtime_error(message.toUtf8().constData()),code_(code) {}
ProductionRecipeService::ProductionRecipeService(Authorize authorize,Dependencies dependencies,int limit,Persist persist,CalibrationResolver completionResolver)
    :authorize_(std::move(authorize)),dependencies_(std::move(dependencies)),persist_(std::move(persist)),completionResolver_(std::move(completionResolver)),historyLimit_(limit) {
    if(limit<1 || limit>256)fail(ProductionError::InvalidConfiguration,"Snapshot sınırı 1–256 olmalı.");
}
void ProductionRecipeService::persist(const QString& kind,const QString& id,const QJsonObject& object) const {
    if(!persist_)return;
    try {persist_(kind,id,object);}catch(const std::exception& e){fail(ProductionError::PersistenceFailure,QString::fromUtf8(e.what()));}
}
void ProductionRecipeService::transition(const QString& principal,const QString& action,const ProductionSnapshot& s) const {
    const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    persist("events",id,{{"schemaVersion",1},{"eventId",id},{"action",action},{"principal",principal},
        {"validationId",s.validation().id},{"contentId",QString::fromLatin1(s.validation().contentId)},
        {"atUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
}
void ProductionRecipeService::owner() const {if(QThread::currentThread()!=ownerThread_)fail(ProductionError::WrongThread,"Production controller owner thread gerektirir.");}
void ProductionRecipeService::permission(const QString& principal,ProductionAction action) const {
    owner();if(principal.isEmpty() || !authorize_ || !authorize_(principal,action))fail(ProductionError::Unauthorized,"Servis yetkisi reddedildi.");
}
void ProductionRecipeService::idle() const {if(cycle_.ticket)fail(ProductionError::Busy,"Aktif çevrim bitmeden sürüm değiştirilemez.");}
QJsonObject ProductionRecipeService::dependencies(const Recipe& recipe,const QString& directory) const {
    ProductionDependencies current;
    try {current=dependencies_ ? dependencies_(recipe) : ProductionDependencies{};}
    catch(const std::exception& e){fail(ProductionError::DependencyChanged,QString::fromUtf8(e.what()));}
    if(current.engineVersion!=ExecutionService::version())fail(ProductionError::DependencyChanged,"Engine sürümü uyumsuz.");
    QJsonObject tools,models,assets,references;
    for(const auto& tool:editorTools(recipe.definition)) {
        tools[tool.id]=QJsonObject{{"kind",tool.kind},{"version",ToolRegistry::get(tool.kind).version}};
        if(!tool.dependencies.isEmpty() || !ToolRegistry::get(tool.kind).requiredBackend.isEmpty()) {
            try {auto entry=tools[tool.id].toObject();entry["dependencies"]=ToolDependencyResolver::resolve(tool,directory);tools[tool.id]=entry;}
            catch(const std::exception& e){fail(ProductionError::DependencyChanged,QString::fromUtf8(e.what()));}
        }
        if(tool.kind=="template_match")models[tool.id]=referenceGeometry(tool);
        if(tool.kind=="pattern_pose" || tool.kind=="shape_pose")models[tool.id]=referenceGeometryIdentity(tool,"pose");
        if(tool.parameters.contains("referenceBinding"))references[tool.id]=toolReferenceBinding(tool).toJson();
    }
    if(current.assetHashes.size()>256)fail(ProductionError::InvalidConfiguration,"En fazla 256 varlık bağımlılığı.");
    for(auto it=current.assetHashes.cbegin();it!=current.assetHashes.cend();++it) {
        if(it.key().isEmpty() || it.key().size()>256 || !sha256(it.value()))fail(ProductionError::InvalidConfiguration,"Varlık içerik kimliği geçersiz.");
        assets[it.key()]=it.value();
    }
    QJsonObject calibration;
    if(current.calibration) {
        current.calibration->validate();
        if(recipe.calibrationReference!=current.calibration->calibrationId)fail(ProductionError::DependencyChanged,"Kalibrasyon referansı uyumsuz.");
        if(recipe.calibrationBinding && *recipe.calibrationBinding!=*current.calibration)fail(ProductionError::DependencyChanged,"Reçetenin kalibrasyon sürümü güncel değil.");
        calibration=current.calibration->toJson();
    } else if(!recipe.calibrationReference.isEmpty())fail(ProductionError::DependencyChanged,"Kalibrasyon bağımlılığı çözümlenmedi.");
    QJsonObject result{{"engine",current.engineVersion},{"tools",tools},{"models",models},{"assets",assets},{"calibration",calibration}};
    if(!references.isEmpty())result["referenceBindings"]=references;
    if(!current.systemCalibration.isEmpty()){SystemCalibration::decodeEvidence(current.systemCalibration);if(current.systemCalibration["executionDependency"].toObject()!=calibration)fail(ProductionError::DependencyChanged,"System binding differs from execution calibration");result["systemCalibration"]=current.systemCalibration;}
    if(SystemCalibration::source(recipe).kind=="industrial_camera"){
        if(!industrialAdmission_)fail(ProductionError::InvalidConfiguration,"Industrial production terminal acceptance pending ICM-08b");
        const auto request=decodeIndustrialRuntimeDependency(SystemCalibration::source(recipe),current.industrialSource);
        if(!current.measurementSource.isEmpty())fail(ProductionError::DependencyChanged,"Industrial source cannot use webcam evidence envelope");
        if(!current.systemCalibration.isEmpty()){
            const auto model=SystemCalibration::decodeEvidence(current.systemCalibration);
            const auto scope=model.scope;
            const auto profile=AcquisitionProfile::fromJson(scope["profile"].toObject());
            if(profile.id!=request.profile.id || profile.revision!=request.profile.revision || profile.acquisitionId(request.setup)!=request.profile.acquisitionId(request.setup))fail(ProductionError::DependencyChanged,"Industrial model/admission profile differs");
            if(scope["schemaVersion"]!=2 || scope["physical"]!=request.device.identity.toJson() || scope["device"]!=request.camera.device.toJson() || scope["target"]!=SetupReference{request.camera.id,request.setup.id,request.setup.revision}.toJson() || scope["geometryId"]!=request.setup.geometryId())fail(ProductionError::DependencyChanged,"Industrial model/admission geometry differs");
        }
        result["industrialSource"]=current.industrialSource;
    }else if(!current.industrialSource.isEmpty())fail(ProductionError::DependencyChanged,"Unexpected industrial source snapshot");
    if(recipe.definition.contains("sourceSetup") && SystemCalibration::source(recipe).kind!="industrial_camera"){

        const auto source=current.measurementSource;const auto acquisition=source["acquisition"].toObject();const auto ref=SystemCalibration::setupReference(recipe);
        if(source.size()!=4 || source["schemaVersion"]!=1 || source["binding"]!=recipe.definition["sourceSetup"] || !ref || acquisition.size()!=3 || acquisition["target"]!=ref->toJson())fail(ProductionError::DependencyChanged,"Physical source snapshot mismatch");
        const auto device=DeviceBinding::fromJson(acquisition["device"].toObject());const auto geometry=GeometryDescriptor::fromJson(acquisition["geometry"].toObject());
        if(!geometry.complete || geometry.sensorRoi || geometry.binning || device.adapter!="webcam" || source["origin"]!=(SystemCalibration::source(recipe).kind=="image_file" ? "offline_replay":"camera_acquired"))fail(ProductionError::DependencyChanged,"Unsupported physical source snapshot");
        if(!current.systemCalibration.isEmpty()){const auto s=SystemCalibration::decodeEvidence(current.systemCalibration);if(s.scope["target"]!=acquisition["target"] || s.scope["device"]!=acquisition["device"] || s.scope["setup"].toObject()["geometry"]!=acquisition["geometry"])fail(ProductionError::DependencyChanged,"Calibration/acquisition physical scope differs");}
        result["measurementSource"]=source;
    }else if(!current.measurementSource.isEmpty())fail(ProductionError::DependencyChanged,"Unexpected physical source snapshot");
    return result;
}
void ProductionRecipeService::requireCurrent(const ProductionSnapshot& s) const {
    if(hash(dependencies(s.recipe(),s.projectDirectory()))!=s.validation().dependencyId)fail(ProductionError::DependencyChanged,"Doğrulama bağımlılığı değişti; yeniden doğrulama gerekir.");
}
RecipeValidation ProductionRecipeService::validate(const QString& principal,const Recipe& draft,const QString& directory) {
    permission(principal,ProductionAction::Validate);
    if(validated_.size()>=historyLimit_)fail(ProductionError::HistoryFull,"Snapshot geçmişi dolu; otomatik silme yok.");
    try {
        if(QJsonDocument(draft.definition).toJson(QJsonDocument::Compact).size()>16*1024*1024)throw std::runtime_error("Snapshot definition sınırı 16 MB.");
        if(directory.isEmpty() || !QDir(directory).isAbsolute() || !QDir(directory).exists())throw std::runtime_error("Mevcut mutlak proje klasörü gerekir.");
        Project p;p.id=QUuid::createUuid().toString();p.name="Validation";p.cameraReference=draft.cameraReference;
        p.createdAt=p.modifiedAt=QDateTime::currentDateTimeUtc();p.recipes={draft};validateProject(p);
        validateEditorDefinition(draft.definition);
        if(draft.definition["editor"].toObject()["schemaVersion"].toInt()==9 && !industrialAdmission_)throw std::runtime_error("Editor9 requires whole source-set Runtime preparation");
        const auto tools=editorTools(draft.definition);const auto connections=editorConnections(draft.definition);
        const auto graph=validateGraph(tools,connections);
        if(!graph.valid())throw std::runtime_error(graph.errors.join("\n").toUtf8().constData());
        int sources=0;
        for(const auto& tool:tools)if(tool.enabled) {
            if((tool.kind=="pattern_pose" || tool.kind=="shape_pose") && std::none_of(tools.cbegin(),tools.cend(),[&](const auto& t){
                return t.enabled && t.parameters.contains("referenceBinding") && toolReferenceBinding(t).sourceTool==tool.id;
            }))throw std::runtime_error("Üretim sözleşmesinde desteklenmeyen tool: standalone pose corpus acceptance pending T8.4.2b.3c");
            if(tool.parameters.contains("poseTracking"))throw std::runtime_error("Legacy pose production corpus acceptance pending T8.4.2b.3c");
            if(tool.parameters.contains("referenceBinding")) {
                const auto b=toolReferenceBinding(tool);
                const auto producer=std::find_if(tools.cbegin(),tools.cend(),[&](const auto& t){return t.id==b.sourceTool;});
                if(producer==tools.cend() || b.geometryId!=referenceGeometryIdentity(*producer,b.sourcePort))throw std::runtime_error("Reference geometry changed; reteach");
                const auto acquisition=std::find_if(tools.cbegin(),tools.cend(),[&](const auto& t){return t.enabled && t.id==b.imageSource && ToolRegistry::get(t.kind).acquisitionSource;});
                if(acquisition==tools.cend())throw std::runtime_error("Reference acquisition identity differs from recipe");
                if(b.world && (!draft.calibrationBinding || b.calibration!=draft.calibrationBinding))throw std::runtime_error("Reference calibration differs from recipe");
            }
            if(!(industrialAdmission_ && tool.kind=="industrial_camera") && !QStringList{"image_file","template_match","blob","edge","calculate","webcam","measurement","roi","preprocess","code_read","edge_point","edge_pair","line_fit","circle_fit","geometry_measurement","pattern_pose","shape_pose"}.contains(tool.kind))throw std::runtime_error("Üretim sözleşmesinde desteklenmeyen tool.");
            if(tool.kind=="webcam" || (industrialAdmission_ && tool.kind=="industrial_camera"))++sources;
            if((ToolRegistry::get(tool.kind).usesCalibration || ((tool.kind=="edge_pair" || tool.kind=="line_fit" || tool.kind=="circle_fit" || tool.kind=="geometry_measurement") && tool.parameters["mode"]=="calibrated")) && !draft.calibrationBinding)throw std::runtime_error("Metric tool requires an exact calibration binding.");
            if(tool.kind=="image_file") {++sources;if(tool.parameters["path"].toString().isEmpty())throw std::runtime_error("Kaynak yolu boş.");}
            if(tool.kind=="template_match" || tool.kind=="pattern_pose" || tool.kind=="shape_pose")MatchService::validateModel(MatchService::parameters(tool.parameters));
            if((tool.kind=="edge_point" || tool.kind=="edge_pair" || tool.kind=="line_fit" || tool.kind=="circle_fit") && tool.parameters["scanRoi"].toObject().isEmpty())throw std::runtime_error("Edge Point scan ROI is not taught.");
            for(const auto& c:connections)if(c.targetTool==tool.id) {
                const auto source=std::find_if(tools.begin(),tools.end(),[&](const auto& t){return t.id==c.sourceTool;});
                if(!source->enabled)throw std::runtime_error("Enabled tool disabled kaynağa bağlı.");
                if(c.targetPort=="reference" && !tool.parameters.contains("referenceBinding") && (referenceNeedsTeaching(tool,*source) || tool.parameters["searchRoi"].toObject().isEmpty() || tool.parameters["referenceOrigin"].toObject().isEmpty()))throw std::runtime_error("Tracking ROI yeniden öğretim gerektirir.");
            }
        }
        if(sources!=1)throw std::runtime_error("Tek enabled edinim kaynağı gerekir.");
        auto s=std::make_shared<ProductionSnapshot>();s->recipe_=draft;s->directory_=QDir(directory).absolutePath();s->dependencies_=dependencies(draft,directory);
        s->validation_={QUuid::createUuid().toString(QUuid::WithoutBraces),principal,content(draft,directory),hash(s->dependencies_),QDateTime::currentDateTimeUtc()};
        Project archived;archived.id=QUuid::createUuid().toString();archived.name="Production archive";
        archived.cameraReference=draft.cameraReference;archived.createdAt=archived.modifiedAt=s->validation_.checkedAtUtc;archived.recipes={draft};
        persist("snapshots",s->validation_.id,{{"schemaVersion",1},{"validationId",s->validation_.id},{"principal",principal},
            {"scope",s->validation_.scope},{"checkedAtUtc",s->validation_.checkedAtUtc.toString(Qt::ISODateWithMs)},
            {"contentId",QString::fromLatin1(s->validation_.contentId)},{"dependencyId",QString::fromLatin1(s->validation_.dependencyId)},
            {"projectDirectory",s->directory_},{"dependencies",s->dependencies_},
            {"project",QJsonDocument::fromJson(ProjectRepository::encode(archived)).object()}});
        validated_.insert(s->validation_.id,s);return s->validation_;
    } catch(const ProductionException&) {throw;}
      catch(const std::exception& e) {fail(ProductionError::InvalidConfiguration,QString::fromUtf8(e.what()));}
}
void ProductionRecipeService::activate(const QString& principal,const QString& id,const Recipe& draft,const QString& directory) {
    permission(principal,ProductionAction::Activate);idle();
    const auto s=validated_.value(id);if(!s)fail(ProductionError::UnknownValidation,"Serviste oluşturulmuş doğrulama bulunamadı.");
    if(content(draft,directory)!=s->validation().contentId)fail(ProductionError::DependencyChanged,"Taslak içeriği veya proje yolu değişti.");
    requireCurrent(*s);transition(principal,"ACTIVATE",*s);active_=s;if(!activated_.contains(id))activated_.append(id);
}
void ProductionRecipeService::rollback(const QString& principal,const QString& id) {
    permission(principal,ProductionAction::Rollback);idle();
    if(!activated_.contains(id))fail(ProductionError::UnknownValidation,"Daha önce etkinleşmiş snapshot bulunamadı.");
    const auto s=validated_.value(id);requireCurrent(*s);transition(principal,"ROLLBACK",*s);active_=s;
}
std::shared_ptr<const ProductionSnapshot> ProductionRecipeService::active() const {owner();return active_;}
bool ProductionRecipeService::busy() const {owner();return cycle_.ticket!=0;}
QVector<QJsonObject> ProductionRecipeService::cycleRecords() const {owner();return records_;}
void ProductionRecipeService::requireReady(const QString& principal) const {
    permission(principal,ProductionAction::Run);idle();if(!active_)fail(ProductionError::UnknownValidation,"Aktif snapshot yok.");requireCurrent(*active_);
}
ProductionCycle ProductionRecipeService::beginCycle(const QString& principal,const QJsonObject& communication) {
    permission(principal,ProductionAction::Run);idle();if(!active_)fail(ProductionError::UnknownValidation,"Aktif snapshot yok.");
    requireCurrent(*active_);const auto started=QDateTime::currentDateTimeUtc();const auto beginId=QUuid::createUuid().toString(QUuid::WithoutBraces);
    persist("events",beginId,{{"schemaVersion",1},{"eventId",beginId},{"action","CYCLE_BEGIN"},{"principal",principal},
        {"validationId",active_->validation().id},{"configId",QString::fromLatin1(active_->validation().contentId)},
        {"ticket",QString::number(serial_+1)},{"communication",communication},{"atUtc",started.toString(Qt::ISODateWithMs)}});
    cycle_={++serial_,active_};cyclePrincipal_=principal;cycleStarted_=started;cycleBeginId_=beginId;cycleCommunication_=communication;pendingRecord_={};return cycle_;
}
void ProductionRecipeService::finishCycle(quint64 ticket,const CycleResult& r) {
    owner();if(r.partial)fail(ProductionError::InvalidCompletion,"Editor partial evaluation cannot finish a production cycle.");
    if(!ticket || ticket!=cycle_.ticket)fail(ProductionError::InvalidCompletion,"Eski veya yabancı çevrim ticket.");
    if(!terminal(r.state) || r.cycleId.isEmpty() || r.recipeId!=cycle_.snapshot->recipe().id || r.configId!=cycle_.snapshot->validation().contentId || !std::isfinite(r.milliseconds) || r.milliseconds<0)fail(ProductionError::InvalidCompletion,"Çevrim sonucu snapshot ile uyuşmuyor.");
    for(const auto& f:r.frames)if(f.cycleId!=r.cycleId || f.frameId.isEmpty() || f.sourceId.isEmpty())fail(ProductionError::InvalidCompletion,"Çevrim frame kökeni uyumsuz.");
    for(const auto& log:records_)if(log["cycleId"]==r.cycleId)fail(ProductionError::InvalidCompletion,"Çevrim kimliği daha önce tamamlandı.");
    const auto configured=editorTools(cycle_.snapshot->recipe().definition);QSet<QString> seen;
    const bool trusted=bool(r.completionSnapshot_) || std::any_of(r.tools.cbegin(),r.tools.cend(),[](const auto& t){return t.fitDiagnostic.has_value();}) || trustedCompletionRequired(configured) || cycle_.snapshot->dependencies().contains("systemCalibration") || cycle_.snapshot->dependencies().contains("measurementSource") || cycle_.snapshot->dependencies().contains("industrialSource");
    if(trusted)try {
        if(r.completionSnapshot_!=cycle_.snapshot || r.completionTicket_!=ticket || r.completionDigest_.isEmpty() ||
           r.completionDigest_!=completionDigest(r))fail(ProductionError::InvalidCompletion,"Runtime completion evidence missing, changed or foreign to lease");
    }catch(const ProductionException&){throw;}catch(const std::exception& e){fail(ProductionError::InvalidCompletion,QString::fromUtf8(e.what()));}
    for(const auto& t:r.tools) {
        const auto tool=std::find_if(configured.cbegin(),configured.cend(),[&](const auto& v){return v.id==t.toolId;});
        if(tool==configured.cend() || seen.contains(t.toolId) || !std::isfinite(t.milliseconds) || t.milliseconds<0)fail(ProductionError::InvalidCompletion,"Yabancı/tekrarlı tool sonucu veya süre.");
        seen.insert(t.toolId);
        if(tool->kind=="geometry_measurement" && t.state==ExecutionState::Done && r.state!=ExecutionState::Skipped) {
            try {
                QMap<QString,PortValue> inputs;
                for(const auto& c:editorConnections(cycle_.snapshot->recipe().definition))if(c.targetTool==tool->id){
                    const auto source=std::find_if(r.tools.cbegin(),r.tools.cend(),[&](const auto& v){return v.toolId==c.sourceTool;});
                    if(source==r.tools.cend() || source->state!=ExecutionState::Done || !source->outputs.contains(c.sourcePort))throw std::runtime_error("Geometry replay upstream missing");
                    inputs[c.targetPort]=source->outputs[c.sourcePort];
                }
                const auto image=std::get<ImageFrame>(inputs.value("image").data);
                const bool world=tool->parameters["mode"]=="calibrated";
                if(world && !completionResolver_)throw std::runtime_error("Geometry completion requires trusted calibration resolver");
                const auto replay=GeometryMeasurementService::execute(*tool,inputs,r.frameId,r.cycleId,{},world ? completionResolver_(image):MeasurementExecutionContext{});
                if(t.decision!=PartDecision::NotEvaluated || replay.calibration!=t.calibration || t.outputs.size()!=replay.outputs.size())throw std::runtime_error("Geometry completion mode/ports/calibration mismatch");
                for(auto it=replay.outputs.cbegin();it!=replay.outputs.cend();++it){
                    auto expected=it.value();const auto actual=t.outputs.value(it.key());
                    if(expected.valid && int(expected.port.type)>int(PortType::Boolean))expected.evaluationId=r.evaluationId;
                    if(ToolResultCodec::encode(expected)!=ToolResultCodec::encode(actual))throw std::runtime_error("Geometry completion differs from exact replay");
                }
                if(t.points!=replay.points || t.lines!=replay.lines || !t.regions.isEmpty() || !t.rejectedPoints.isEmpty())throw std::runtime_error("Geometry helper overlay differs from replay");
            }catch(const std::exception& e){fail(ProductionError::InvalidCompletion,QString::fromUtf8(e.what()));}
        }
        if(tool->kind=="circle_fit" && !tool->parameters.contains("referenceBinding") && t.state==ExecutionState::Done && r.state!=ExecutionState::Skipped)CircleToolService::validateCompletion(*tool,t,cycle_.snapshot->recipe().calibrationBinding);
        if((tool->kind=="edge_pair" || tool->kind=="line_fit") && t.state==ExecutionState::Done && r.state!=ExecutionState::Skipped) {
            const auto found=t.outputs.value("found"),accepted=t.outputs.value("accepted");
            if(!found.valid || !std::holds_alternative<bool>(found.data))fail(ProductionError::InvalidCompletion,"Metrology requires found Boolean");
            if(std::get<bool>(found.data)) {
                const bool world=tool->parameters["mode"]=="calibrated";
                const auto metric=t.outputs.value(tool->kind=="edge_pair" ? (world ? "widthMm":"widthPx"):(world ? "spanMm":"spanPx"));
                if(!metric.valid || !std::holds_alternative<double>(metric.data) || !accepted.valid || !std::holds_alternative<bool>(accepted.data) || metric.diagnostic!=!world || accepted.diagnostic!=!world)fail(ProductionError::InvalidCompletion,"Metrology metric/Boolean/mode mismatch");
                if(world && (!t.calibration || !cycle_.snapshot->recipe().calibrationBinding || *t.calibration!=*cycle_.snapshot->recipe().calibrationBinding))fail(ProductionError::InvalidCompletion,"Metrology calibration differs from snapshot");
                const double value=std::get<double>(metric.data);bool within=value>=tool->parameters["lower"].toDouble() && value<=tool->parameters["upper"].toDouble();
                if(tool->kind=="line_fit"){const auto angle=t.outputs.value(world ? "angleMm":"anglePx");if(!angle.valid || !std::holds_alternative<double>(angle.data))fail(ProductionError::InvalidCompletion,"Metrology angle missing");const double a=std::get<double>(angle.data);within=within && a>=tool->parameters["angleLower"].toDouble() && a<=tool->parameters["angleUpper"].toDouble();}
                if(std::get<bool>(accepted.data)!=within)fail(ProductionError::InvalidCompletion,"Metrology tolerance differs from snapshot");
            }else if(accepted.valid || t.calibration)fail(ProductionError::InvalidCompletion,"No-found metrology cannot carry tolerance/calibration result");
        }
        if(tool->kind=="measurement" && t.state==ExecutionState::Done && r.state!=ExecutionState::Skipped && t.decision==PartDecision::NotEvaluated)fail(ProductionError::InvalidCompletion,"DONE Measurement gerçek karar gerektirir.");
        if(t.decision!=PartDecision::NotEvaluated) {
            const auto value=t.outputs.value("value"),pass=t.outputs.value("pass");
            if(value.diagnostic || pass.diagnostic)fail(ProductionError::InvalidCompletion,"Diagnostic outputs cannot establish a production decision.");
            if(t.state!=ExecutionState::Done || !t.calibration || !cycle_.snapshot->recipe().calibrationBinding || *t.calibration!=*cycle_.snapshot->recipe().calibrationBinding || !value.valid || !pass.valid || value.port.unit!="mm" || !std::holds_alternative<double>(value.data) || !std::isfinite(std::get<double>(value.data)) || !std::holds_alternative<bool>(pass.data) || std::get<bool>(pass.data)!=(t.decision==PartDecision::Pass))fail(ProductionError::InvalidCompletion,"Measurement kararı/kalibrasyonu/çıktısı uyumsuz.");
            const auto mm=std::get<double>(value.data);const bool accepted=mm>=tool->parameters["lowerMm"].toDouble() && mm<=tool->parameters["upperMm"].toDouble();
            if(tool->kind!="measurement" || accepted!=(t.decision==PartDecision::Pass))fail(ProductionError::InvalidCompletion,"Karar aktif toleransla uyuşmuyor.");
        }
        if((r.state==ExecutionState::Skipped || t.state!=ExecutionState::Done) && (!t.outputs.isEmpty() || t.decision!=PartDecision::NotEvaluated || t.calibration))fail(ProductionError::InvalidCompletion,"Terminal hata/iptal ölçüm çıktısı taşıyamaz.");
        if(!terminal(t.state))fail(ProductionError::InvalidCompletion,"Tool sonucu terminal değil.");
        for(auto it=t.outputs.cbegin();it!=t.outputs.cend();++it) {
            const auto ports=outputPorts(tool->kind);const auto expected=std::find_if(ports.begin(),ports.end(),[&](const auto& p){return p.id==it.key();});
            if(expected==ports.end())fail(ProductionError::InvalidCompletion,"Undeclared completion port");
            if(it->valid && std::none_of(r.frames.cbegin(),r.frames.cend(),[&](const auto& f){return f.frameId==it->frameId && f.cycleId==it->cycleId && f.sourceId==it->origin.sourceId && f.coordinateSystemId==it->origin.coordinateSystemId;}))fail(ProductionError::InvalidCompletion,"Output origin does not match an acquired frame.");
            if(it->valid && int(it->port.type)>int(PortType::Boolean) && (r.evaluationId.isEmpty() || it->evaluationId!=r.evaluationId))fail(ProductionError::InvalidCompletion,"Output evaluation identity differs from cycle");
            try {ToolResultCodec::validate(*it,*expected,r.frameId,r.cycleId);}catch(const std::exception& e){fail(ProductionError::InvalidCompletion,QString::fromUtf8(e.what()));}
        }
        for(const auto& value:t.outputs)if(value.valid && (value.cycleId!=r.cycleId || value.frameId!=r.frameId || value.origin.cycleId!=r.cycleId || value.origin.frameId!=r.frameId))fail(ProductionError::InvalidCompletion,"Çıktı kökeni çevrimle uyuşmuyor.");
    }
    if(r.state==ExecutionState::Done && seen.size()!=configured.size())fail(ProductionError::InvalidCompletion,"Tam çevrim tool sonuçları eksik.");
    if(!pendingRecord_.isEmpty() && pendingRecord_["cycleId"].toString()!=r.cycleId)fail(ProductionError::InvalidCompletion,"Kayıt bekleyen çevrim değiştirilemez.");
    record(&r,{});cycle_={};
}
void ProductionRecipeService::abandonCycle(quint64 ticket,const QString& reason) {
    owner();if(!ticket || ticket!=cycle_.ticket || reason.isEmpty())fail(ProductionError::InvalidCompletion,"Çevrim iptal ticket/reason geçersiz.");
    if(!pendingRecord_.isEmpty())fail(ProductionError::PersistenceFailure,"Gerçek çevrim kaydı bekliyor; önce aynı sonucu tekrar kaydedin.");
    record(nullptr,reason);cycle_={};
}
void ProductionRecipeService::record(const CycleResult* r,const QString& error) {
    const auto& s=*cycle_.snapshot;QJsonArray frames,tools;
    if(r) {
        for(const auto& f:r->frames)frames.append(QJsonObject{{"frameId",f.frameId},{"cycleId",f.cycleId},{"sourceId",f.sourceId},{"coordinateSystemId",f.coordinateSystemId},{"completedAtUtc",f.completedAtUtc.toString(Qt::ISODateWithMs)}});
        for(const auto& t:r->tools) {
            QJsonObject outputs;
            for(auto it=t.outputs.cbegin();it!=t.outputs.cend();++it)
                outputs[it.key()]=ProductionResultCodec::output(*it);
            if(s.dependencies().contains("industrialSource"))for(auto it=t.outputs.begin();it!=t.outputs.end();++it)if(it->valid && std::holds_alternative<ImageFrame>(it->data)){auto o=outputs[it.key()].toObject(), image=o["imageInfo"].toObject();image["contentId"]=QString::fromLatin1(std::get<ImageFrame>(it->data).contentId().toHex());o["imageInfo"]=image;outputs[it.key()]=o;}
            QString name,kind;for(const auto& configured:editorTools(s.recipe().definition))if(configured.id==t.toolId){name=configured.name;kind=configured.kind;}
            tools.append(QJsonObject{{"toolId",t.toolId},{"name",name},{"kind",kind},{"state",executionStateName(t.state)},{"message",t.message},{"milliseconds",t.milliseconds},{"outputs",outputs},{"errorCode",t.errorCode},
                {"decision",t.decision==PartDecision::Pass ? "PASS" : t.decision==PartDecision::Fail ? "FAIL" : "NOT_EVALUATED"},
                {"calibration",t.calibration ? QJsonValue(t.calibration->toJson()) : QJsonValue(QJsonValue::Null)}});
            if(t.fitDiagnostic){auto item=tools.last().toObject();item["measurementDiagnostic"]=encodeMeasurementDiagnostic(*t.fitDiagnostic);tools.replace(tools.size()-1,item);}
        }
    }
    if(pendingRecord_.isEmpty())pendingRecord_=QJsonObject{{"schemaVersion",s.dependencies().contains("industrialSource") ? 8:s.dependencies().contains("measurementSource") ? 6:s.dependencies().contains("systemCalibration") ? 5:s.dependencies().contains("referenceBindings") ? 4:3},{"evaluationId",r ? r->evaluationId : QString{}},{"recordId",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"beginEventId",cycleBeginId_},{"ticket",QString::number(cycle_.ticket)},{"principal",cyclePrincipal_},
        {"recipeId",s.recipe().id},{"recipeName",s.recipe().name},{"applicationType",typeKey(s.recipe().applicationType)},{"recipeVersion",s.recipe().recipeVersion},{"validationId",s.validation().id},
        {"configId",QString::fromLatin1(s.validation().contentId)},{"dependencyId",QString::fromLatin1(s.validation().dependencyId)},
        {"dependencies",s.dependencies()},{"startedAtUtc",cycleStarted_.toString(Qt::ISODateWithMs)},
        {"completedAtUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"cycleId",r ? r->cycleId : QString{}},{"milliseconds",r ? QJsonValue(r->milliseconds) : QJsonValue(QJsonValue::Null)},{"state",r ? executionStateName(r->state) : "ERROR"},
        {"error",r ? r->error : error},{"frames",frames},{"tools",tools},{"communicationState",cycleCommunication_.isEmpty() ? "NOT_REQUESTED" : "RESULT_PENDING"},{"communication",cycleCommunication_}};
    if(s.dependencies().contains("referenceBindings"))pendingRecord_["referenceBindings"]=s.dependencies()["referenceBindings"];
    if(s.dependencies().contains("measurementSource"))pendingRecord_["measurementSource"]=s.dependencies()["measurementSource"];
    if(s.dependencies().contains("systemCalibration"))pendingRecord_["systemCalibration"]=s.dependencies()["systemCalibration"];
    if(s.dependencies().contains("industrialSource")){
        const auto admission=s.dependencies()["industrialSource"].toObject();const auto source=admission["sourceToolId"].toString();pendingRecord_["industrialSource"]=admission;
        QString reason=r ? r->error:error,code; if(r)for(const auto& t:r->tools)if(t.toolId==source){if(!t.message.isEmpty())reason=t.message;code=t.errorCode;}
        pendingRecord_["industrialAcquisition"]=r && r->industrialEvidence.contains(source) && r->industrialEvidence[source] ? industrialCompletionEvidence(*r->industrialEvidence[source]):QJsonObject{{"schemaVersion",1},{"sourceToolId",source},{"status","NOT_ACQUIRED"},{"reason",reason},{"errorCode",code}};
    }
    ProductionResultCodec::validate(pendingRecord_);
    persist("cycles",pendingRecord_["recordId"].toString(),pendingRecord_);
    records_.append(pendingRecord_);pendingRecord_={};
    if(records_.size()>128)records_.removeFirst();
}
}
