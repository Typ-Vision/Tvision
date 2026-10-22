#include "MeasurementDiagnostic.hpp"
#include "ExecutionService.hpp"
#include "AcquisitionPolicy.hpp"
#include "SourceFrameIndex.hpp"
#include "ToolExecutors.hpp"
#include "ToolResultCodec.hpp"
#include "ToolDependencyResolver.hpp"
#include "PoseRoiService.hpp"
#include "ReferenceService.hpp"
#include "WorldTrackingService.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "AcquisitionService.hpp"
#include "WebcamAdapter.hpp"
#include "MeasurementSourceIdentity.hpp"
#include "IndustrialExecution.hpp"
#include <QDir>
#include <QImageReader>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QUuid>
#include <QSet>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace visionflow {
QString executionStateName(ExecutionState s) {switch(s){case ExecutionState::Ready:return "READY";case ExecutionState::Busy:return "BUSY";case ExecutionState::Done:return "DONE";case ExecutionState::Error:return "ERROR";case ExecutionState::Skipped:return "SKIPPED";}return {};}
QByteArray ExecutionService::toolSignature(const QJsonObject& definition,const QString& directory,const QString& tool,const QByteArray& scope) {
    const auto tools=editorTools(definition);const auto connections=editorConnections(definition);
    QSet<QString> required{tool};QStringList pending{tool};
    while(!pending.isEmpty()){const auto id=pending.takeFirst();for(const auto& c:connections)if(c.targetTool==id && !required.contains(c.sourceTool)){required.insert(c.sourceTool);pending.append(c.sourceTool);}}
    QCryptographicHash hash(QCryptographicHash::Sha256);hash.addData(scope);hash.addData(version().toUtf8());
    for(const auto& t:tools)if(required.contains(t.id)) {
        hash.addData(QJsonDocument(QJsonObject{{"id",t.id},{"kind",t.kind},{"enabled",t.enabled},{"parameters",t.parameters},{"version",ToolRegistry::get(t.kind).version}}).toJson(QJsonDocument::Compact));
        if(!t.dependencies.isEmpty() || !ToolRegistry::get(t.kind).requiredBackend.isEmpty()) {
            hash.addData(QJsonDocument(encodeToolDependencies(t.dependencies)).toJson(QJsonDocument::Compact));
            try {hash.addData(QJsonDocument(ToolDependencyResolver::resolve(t,directory)).toJson(QJsonDocument::Compact));}
            catch(const std::exception&){hash.addData("DEPENDENCY_UNAVAILABLE");}
        }
        if(definition["editor"].toObject()["schemaVersion"]==9 && ToolRegistry::get(t.kind).acquisitionSource){hash.addData(QJsonDocument(definition["sourceSetups"].toObject()[t.id].toObject()).toJson(QJsonDocument::Compact));if(definition["sourceSetup"].toObject()["sourceToolId"]==t.id)hash.addData(QJsonDocument(definition["sourceSetup"].toObject()).toJson(QJsonDocument::Compact));}
        if(t.kind=="image_file") {const auto path=QDir(directory).absoluteFilePath(t.parameters["path"].toString());QFile file(path);if(!file.open(QIODevice::ReadOnly))hash.addData("MISSING_ASSET");else hash.addData(QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256));QFile sidecar(path+".json");if(sidecar.open(QIODevice::ReadOnly)){hash.addData("SIDECAR");hash.addData(QCryptographicHash::hash(sidecar.readAll(),QCryptographicHash::Sha256));}}
    }
    for(const auto& c:connections)if(required.contains(c.targetTool))hash.addData(QJsonDocument(QJsonArray{c.sourceTool,c.sourcePort,c.targetTool,c.targetPort}).toJson(QJsonDocument::Compact));
    return hash.result();
}
CycleResult ExecutionService::runTool(const QJsonObject& definition,const QString& directory,const QString& recipeId,const QByteArray& configId,
    const std::shared_ptr<std::atomic_bool>& cancel,const QString& target,const TargetEvaluation& evaluation,const CalibrationResolver& resolver) const {
    const auto captured=toolSignature(definition,directory,target,evaluation.scope);
    auto result=run(definition,directory,recipeId,configId,cancel,target,{},resolver,&evaluation);
    result.configId=captured;return result;
}
CycleResult ExecutionService::run(const QJsonObject& definition,const QString& directory,const QString& recipeId,const QByteArray& configId,const std::shared_ptr<std::atomic_bool>& cancel,const QString& targetTool,const std::optional<ImageFrame>& frameSnapshot,const CalibrationResolver& resolver,const TargetEvaluation* targetOnly,const QJsonObject& acquisition,const ExecutionLimits& limits) const {
    CycleResult cycle;cycle.cycleId=QUuid::createUuid().toString(QUuid::WithoutBraces);cycle.frameId=QUuid::createUuid().toString(QUuid::WithoutBraces);cycle.recipeId=recipeId;cycle.configId=configId;
    cycle.targetToolId=targetTool;cycle.evaluationId=QUuid::createUuid().toString(QUuid::WithoutBraces);cycle.partial=targetOnly!=nullptr;
    if(targetOnly){cycle.frameId=targetOnly->frame.metadata().frameId;cycle.cycleId=targetOnly->frame.metadata().cycleId;}
    const bool multi=definition["editor"].toObject()["schemaVersion"].toInt()==9;
    if(multi && !targetTool.isEmpty())cycle.partial=true;
    if(multi && !targetOnly)cycle.frameId.clear();
    QElapsedTimer total;total.start();
    try {
        bool experimentalEditorReplay=experimentalEditorReplay_;
        if(automaticAcquisition_ && ((frameSnapshot && !frameSnapshot->metadata().experimentalAcquisition.isEmpty()) || (targetOnly && !targetOnly->frame.metadata().experimentalAcquisition.isEmpty())))experimentalEditorReplay=true;
        if(!experimentalEditorReplay && ((frameSnapshot && !frameSnapshot->metadata().experimentalAcquisition.isEmpty()) || (targetOnly && !targetOnly->frame.metadata().experimentalAcquisition.isEmpty())))throw std::runtime_error("Experimental frame requires explicit Editor replay policy");
        validateEditorDefinition(definition);
        if(multi && (limits.deadlineMs<=0 || limits.imageBytes==0))throw std::runtime_error("Invalid execution budget");
        auto tools=editorTools(definition);auto connections=editorConnections(definition);
        if(!targetTool.isEmpty()) {
            if(std::none_of(tools.begin(),tools.end(),[&](const auto& t){return t.id==targetTool;}))throw std::runtime_error("Seçili tool bulunamadı.");
            QSet<QString> required{targetTool};QStringList pending{targetTool};
            while(!pending.isEmpty()) {
                const auto id=pending.takeFirst();
                for(const auto& c:connections)if(c.targetTool==id && !required.contains(c.sourceTool)) {required.insert(c.sourceTool);pending.append(c.sourceTool);}
            }
            tools.erase(std::remove_if(tools.begin(),tools.end(),[&](const auto& t){return !required.contains(t.id);}),tools.end());
            connections.erase(std::remove_if(connections.begin(),connections.end(),[&](const auto& c){return !required.contains(c.targetTool);}),connections.end());
        }
        if(automaticAcquisition_ && !targetOnly)experimentalEditorReplay=AcquisitionPolicy::sources(tools,directory);
        auto graph=multi ? validateSourceGraph(tools,connections):validateGraph(tools,connections);
        if(!graph.valid())throw std::runtime_error(graph.errors.join("\n").toUtf8().constData());
        QSet<QString> industrialDevices;
        for(const auto& t:editorTools(definition))if(t.enabled && t.kind=="industrial_camera" && t.parameters["sourceReference"].isObject()){
            const auto ref=IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject());
            const auto identity=QString::fromUtf8(QJsonDocument(ref.physical.toJson()).toJson(QJsonDocument::Compact));
            if(industrialDevices.contains(identity))throw std::runtime_error("Same physical industrial camera assigned to multiple sources");
            industrialDevices.insert(identity);
        }
        cycle.state=ExecutionState::Busy;QMap<QString,ToolResult> completed;bool error=false,stopped=false;MeasurementExecutionContext measurement;
        QMap<QString,QString> frameIds;QMap<QString,MeasurementExecutionContext> measurements;
        quint64 retainedBytes=0;
        if(multi)for(const auto& source:graph.requiredSources)frameIds[source]=targetOnly ? targetOnly->frame.metadata().frameId:QUuid::createUuid().toString(QUuid::WithoutBraces);
        if(multi && graph.requiredSources.size()==1)cycle.frameId=frameIds.first();
        if(multi && frameSnapshot && graph.requiredSources.size()!=1)throw std::runtime_error("A single frame snapshot cannot represent multiple sources");
        if(targetOnly) {
            if(multi && (graph.requiredSources.size()!=1 || targetOnly->frame.metadata().sourceId!=graph.requiredSources.first()))throw std::runtime_error("Target raw frame source mismatch");
            if(cycle.frameId.isEmpty() || cycle.cycleId.isEmpty())throw std::runtime_error("Önce görüntü al.");
            if(resolver)measurement=resolver(targetOnly->frame);
            for(auto it=targetOnly->cache.cbegin();it!=targetOnly->cache.cend();++it)
                if(it->signature==toolSignature(definition,directory,it.key(),targetOnly->scope))completed[it.key()]=it->result;
            if(multi){
                const auto raw=completed.value(graph.requiredSources.first()).outputs.value("image");
                const auto* image=std::get_if<ImageFrame>(&raw.data);
                if(!raw.valid || !image || image->contentId()!=targetOnly->frame.contentId() ||
                   image->processingId()!=targetOnly->frame.processingId() || raw.frameId!=cycle.frameId || raw.cycleId!=cycle.cycleId)
                    throw std::runtime_error("Target raw frame differs from cached acquisition");
            }
        }
        for(const auto& id:graph.order) {
            if(targetOnly && id!=targetTool)continue;
            if(multi && !(cancel && cancel->load()) && total.elapsed()>=limits.deadlineMs)throw std::runtime_error("Multi-source total deadline exceeded");
            const auto source=graph.toolSources.value(id);
            const auto sourceAcquisition=multi ? acquisition["sources"].toObject()[source].toObject():acquisition;
            auto sourceBinding=definition["sourceSetups"].toObject()[source].toObject();if(sourceBinding.isEmpty() && definition["sourceSetup"].toObject()["sourceToolId"]==source)sourceBinding=definition["sourceSetup"].toObject();
            const QString toolFrameId=multi ? frameIds.value(source):cycle.frameId;
            if(multi && !targetOnly)measurement=measurements.value(source);
            const auto t=*std::find_if(tools.begin(),tools.end(),[&](const auto& t){return t.id==id;});ToolResult r;r.toolId=id;QElapsedTimer timer;timer.start();
            std::shared_ptr<const IndustrialExecutionEvidence> industrialEvidence;
            if(cancel && cancel->load())stopped=true;
            if(targetOnly && ToolRegistry::get(t.kind).acquisitionSource)throw std::runtime_error("Kaynak için görüntü al / Run Flow kullanın.");
            if(!t.enabled || stopped) {r.state=ExecutionState::Skipped;r.message=stopped ? "İptal edildi" : "Disabled";}
            else {
                QMap<QString,PortValue> inputs;bool unavailable=false;QStringList missing;
                for(const auto& c:connections)if(c.targetTool==id) {
                    const auto source=completed.value(c.sourceTool);const auto value=source.outputs.value(c.sourcePort);
                    if(source.state!=ExecutionState::Done || !value.valid || value.frameId!=toolFrameId || value.cycleId!=cycle.cycleId) {
                        unavailable=true;for(const auto& upstream:tools)if(upstream.id==c.sourceTool)missing.append(upstream.name);
                    }
                    else inputs[c.targetPort]=value;
                }
                if(unavailable) {r.state=ExecutionState::Skipped;r.message=targetOnly ? ("Girdi güncel değil: "+missing.join(", ")+". Önce görüntü al veya bağlı üst aracı çalıştır.") : "Upstream sonucu geçersiz/ERROR/SKIPPED";}
                else try {
                    ToolDependencyResolver::resolve(t,directory);
                    const auto declaredInputs=inputPorts(t.kind);
                    for(auto it=inputs.cbegin();it!=inputs.cend();++it) {
                        const auto expected=std::find_if(declaredInputs.begin(),declaredInputs.end(),[&](const auto& p){return p.id==it.key();});
                        if(expected==declaredInputs.end() || expected->type!=it->port.type || expected->unit!=it->port.unit || expected->frame!=it->port.frame)
                            throw std::runtime_error("Giriş port tipi/birim/frame sözleşmesi uyumsuz.");
                        auto declared=*expected;declared.id=it->port.id; // Connection maps output ID to input ID.
                        ToolResultCodec::validate(*it,declared,toolFrameId,cycle.cycleId);
                        if(multi){const auto raw=completed.value(source).outputs.value("image");SourceFrameIndex index(cycle.cycleId,{source},{raw.origin});index.validatePort(source,*it,declared);}
                    }
                    QString coordinates,sourceFrame,sourceId;
                    std::optional<CoordinateFrame> geometricFrame;
                    for(const auto& value:inputs) {
                        if(const auto frame=ToolResultCodec::coordinates(value)) {
                            if(geometricFrame && *geometricFrame!=*frame)throw std::runtime_error("Typed input geometric frames/axes differ");
                            geometricFrame=frame;
                        }
                        const auto& m=value.origin;
                        if(m.frameId.isEmpty() || m.frameId!=value.frameId || m.cycleId!=value.cycleId || m.coordinateSystemId.isEmpty())throw std::runtime_error("Giriş frame metadata eksik/uyumsuz.");
                        if(targetOnly && (m.sourceId!=targetOnly->frame.metadata().sourceId || m.coordinateSystemId!=targetOnly->frame.metadata().coordinateSystemId))throw std::runtime_error("Cached input source/coordinate identity differs from raw frame.");
                        if(!coordinates.isEmpty() && (coordinates!=m.coordinateSystemId || sourceFrame!=m.frameId || sourceId!=m.sourceId))throw std::runtime_error("Giriş frame/koordinat sistemleri uyumsuz.");
                        coordinates=m.coordinateSystemId;sourceFrame=m.frameId;sourceId=m.sourceId;
                    }
                    for(const auto& c:connections)if(c.targetTool==id && c.targetPort=="reference" && !t.parameters.contains("referenceBinding")) {
                        const auto source=std::find_if(tools.begin(),tools.end(),[&](const auto& source){return source.id==c.sourceTool;});
                        if(referenceNeedsTeaching(t,*source))throw std::runtime_error("Referans modeli/geometrisi değişti. Search ROI çizip referans ilişkisini yeniden öğretin.");
                    }
                    if(ToolRegistry::get(t.kind).acquisitionSource && frameSnapshot) {
                        if(t.kind=="industrial_camera")throw std::runtime_error("Industrial execution requires provider acquisition; snapshot substitution rejected");
                        if(frameSnapshot->size().isEmpty())throw std::runtime_error("Frame snapshot boş.");
                        auto metadata=frameSnapshot->metadata();
                        if(!metadata.sourceId.isEmpty() && metadata.sourceId!=t.id)throw std::runtime_error("Frame snapshot kaynak kimliği uyumsuz.");
                        metadata.sourceId=t.id;metadata.frameId=toolFrameId;metadata.cycleId=cycle.cycleId;
                        if(metadata.coordinateSystemId.isEmpty())metadata.coordinateSystemId=t.id+"/image";
                        if(!metadata.completedAtUtc.isValid())metadata.completedAtUtc=QDateTime::currentDateTimeUtc();
                        r.state=ExecutionState::Done;r.outputs["image"]={outputPorts(t.kind).first(),frameSnapshot->withMetadata(metadata),true,toolFrameId,cycle.cycleId,metadata};
                    } else {
                        auto effective=t;bool resolved=true;std::optional<ToolRoi> resolvedRoi;std::optional<WorldTrackingMap> worldTracking;
                        if(t.parameters.contains("referenceBinding")) {
                            const auto binding=toolReferenceBinding(t);
                            const auto source=std::find_if(tools.cbegin(),tools.cend(),[&](const auto& node){return node.id==binding.sourceTool;});
                            if(source==tools.cend())throw std::runtime_error("Reference producer missing");
                            if(binding.world){
                                const auto mapped=ReferenceService::resolveWorld(binding,*source,completed.value(source->id),std::get<ImageFrame>(inputs.value("image").data),measurement,cancel);
                                if(!mapped.map){resolved=false;r.state=mapped.state;r.message=mapped.reason;}else worldTracking=mapped.map;
                            }else {
                            const auto mapped=ReferenceService::resolve(binding,*source,completed.value(source->id),std::get<ImageFrame>(inputs.value("image").data));
                            if(!mapped.roi){resolved=false;r.state=mapped.state;r.message=mapped.reason;}
                            else {
                                effective.parameters.remove("referenceBinding");inputs.remove(referenceInput(binding));
                                if(t.kind=="edge" || t.kind=="blob" || t.kind=="code_read")resolvedRoi=mapped.roi;
                                else effective.parameters["scanRoi"]=mapped.roi->toJson();
                            }
                            }
                        }
                        if(t.parameters.contains("poseTracking")) {
                            const auto sourceId=trackingSourceTool(t);
                            const auto source=std::find_if(tools.cbegin(),tools.cend(),[&](const auto& node){return node.id==sourceId;});
                            if(source==tools.cend())throw std::runtime_error("Tracking producer missing");
                            // completed owns the full current producer result, not just the connected pose port.
                            if(isWorldPoseTracking(t)){
                                const auto mapped=PoseRoiService::resolveWorld(worldPoseTracking(t),*source,completed.value(source->id),std::get<ImageFrame>(inputs.value("image").data),measurement,cancel);
                                if(!mapped.map){resolved=false;r.state=mapped.state;r.message=mapped.reason;}else worldTracking=mapped.map;
                            }else {
                                const auto roi=PoseRoiService::resolve(poseTrackingBinding(t),*source,completed.value(source->id),std::get<ImageFrame>(inputs.value("image").data));
                                if(!roi.roi){resolved=false;r.state=roi.state;r.message=roi.reason;}
                                else {effective.parameters.remove("poseTracking");if(t.kind=="blob" || t.kind=="code_read")resolvedRoi=roi.roi;else effective.parameters["scanRoi"]=roi.roi->toJson();inputs.remove("poseReference");}
                            }
                        }
                        if(resolved){
                            cycle.executedTools.append(t.id);
                            if(ToolRegistry::get(t.kind).acquisitionSource)++cycle.acquisitions;
                            auto injected=multi && t.kind!="industrial_camera" && ToolRegistry::get(t.kind).acquisitionSource && sourceAdapterFactory_ ? sourceAdapterFactory_(t):nullptr;
                            if(t.kind=="industrial_camera"){
                                if(!industrial_)throw std::runtime_error("Industrial execution requires immutable camera/setup/profile inputs");
                                auto request=industrialExecutionRequest(*industrial_,t);
                                const SetupReference target{request.camera.id,request.setup.id,request.setup.revision};
                                if(!sourceBinding.isEmpty() && sourceBinding["target"]!=target.toJson())throw std::runtime_error("Industrial source setup snapshot mismatch");
                                const auto size=request.setup.geometry.resolution;
                                if(quint64(size.width())*size.height()*12>limits.imageBytes-retainedBytes)throw std::runtime_error("Multi-source image budget preflight rejected");
                                const auto ref=IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject());
                                const int timeout=int(std::min<qint64>(ref.timeoutMs,std::max<qint64>(1,limits.deadlineMs-total.elapsed())));
                                const auto actual=acquireIndustrial(*industrial_,request,cycle.cycleId,toolFrameId,timeout,cancel,experimentalEditorReplay);
                                r.state=ExecutionState::Done;r.outputs["image"]={outputPorts(t.kind).first(),actual.image,true,toolFrameId,cycle.cycleId,actual.image.metadata()};
                                industrialEvidence=actual.evidence;
                            }else if(injected){
                                if(injected->capabilities().sourceKind!=t.kind)throw std::runtime_error("Acquisition adapter kind mismatch");
                                const int timeout=int(std::min<qint64>(t.kind=="webcam" ? t.parameters["timeoutMs"].toInt():5000,std::max<qint64>(1,limits.deadlineMs-total.elapsed())));
                                const auto actual=AcquisitionService{}.acquire(*injected,{t.id,cycle.cycleId,toolFrameId,TriggerMode::Software,timeout,cancel});
                                if(actual.state!=AcquisitionState::Done || !actual.frame)throw std::runtime_error(actual.error.toUtf8().constData());r.state=ExecutionState::Done;r.outputs["image"]={outputPorts(t.kind).first(),actual.frame->image,true,toolFrameId,cycle.cycleId,actual.frame->image.metadata()};
                            }else if((multi || experimentalEditorReplay) && t.kind=="image_file"){
                                const auto path=QDir(directory).absoluteFilePath(t.parameters["path"].toString());const auto size=QImageReader(path).size();
                                // Conservative bound before decode: supported raw formats <= 8 B/px plus preview.
                                if(size.isValid() && quint64(size.width())*size.height()*12>limits.imageBytes-retainedBytes)throw std::runtime_error("Multi-source image budget preflight rejected");
                                const ImageFileAdapter adapter(path,experimentalEditorReplay);const auto actual=AcquisitionService{}.acquire(adapter,{t.id,cycle.cycleId,toolFrameId,TriggerMode::Software,int(std::min<qint64>(5000,limits.deadlineMs-total.elapsed())),cancel});
                                if(actual.state!=AcquisitionState::Done || !actual.frame)throw std::runtime_error(actual.error.toUtf8().constData());r.state=ExecutionState::Done;r.outputs["image"]={outputPorts(t.kind).first(),actual.frame->image,true,toolFrameId,cycle.cycleId,actual.frame->image.metadata()};
                            }else if(t.kind=="webcam" && !sourceBinding.isEmpty()){
                                if(sourceAcquisition.isEmpty())throw std::runtime_error("Scoped webcam requires owner-thread acquisition snapshot");
                                const auto binding=sourceBinding;if(binding["sourceToolId"]!=t.id || sourceAcquisition["target"]!=binding["target"])throw std::runtime_error("Scoped acquisition target mismatch");
                                const auto device=DeviceBinding::fromJson(sourceAcquisition["device"].toObject());const auto geometry=GeometryDescriptor::fromJson(sourceAcquisition["geometry"].toObject());device.resolve(WebcamAdapter::discover());
                                if(!geometry.complete || geometry.sensorRoi || geometry.binning)throw std::runtime_error("Unsupported scoped acquisition geometry");
                                WebcamAdapter adapter({0,"qt",device.providerId,geometry.resolution});const auto actual=AcquisitionService{}.acquire(adapter,{t.id,cycle.cycleId,toolFrameId,TriggerMode::Software,int(multi ? std::min<qint64>(t.parameters["timeoutMs"].toInt(),std::max<qint64>(1,limits.deadlineMs-total.elapsed())):t.parameters["timeoutMs"].toInt()),cancel});
                                if(actual.state!=AcquisitionState::Done || !actual.frame)throw std::runtime_error(actual.error.toUtf8().constData());r.state=ExecutionState::Done;r.outputs["image"]={outputPorts(t.kind).first(),actual.frame->image,true,toolFrameId,cycle.cycleId,actual.frame->image.metadata()};
                            }else {if(multi && t.kind=="webcam")effective.parameters["timeoutMs"]=int(std::min<qint64>(t.parameters["timeoutMs"].toInt(),std::max<qint64>(1,limits.deadlineMs-total.elapsed())));
                                if(multi && t.kind=="preprocess" && inputs.contains("image")){const auto& image=std::get<ImageFrame>(inputs["image"].data);if(quint64(image.size().width())*image.size().height()*12>limits.imageBytes-retainedBytes)throw std::runtime_error("Multi-source image budget preflight rejected");}
                                r=worldTracking ? WorldTrackingService::execute(t,std::get<ImageFrame>(inputs.value("image").data),*worldTracking,cancel)
                                :ToolExecutors::get(t.kind,measurement,resolvedRoi)(effective,inputs,directory,toolFrameId,cycle.cycleId,cancel);}
                        }
                    }
                    if(ToolRegistry::get(t.kind).acquisitionSource && t.kind!="industrial_camera" && !sourceBinding.isEmpty() && r.state==ExecutionState::Done){
                        const auto binding=sourceBinding;if(sourceAcquisition.isEmpty() || binding["sourceToolId"]!=t.id || sourceAcquisition["target"]!=binding["target"])throw std::runtime_error("Physical source requires exact immutable acquisition snapshot");
                        const auto image=r.outputs.value("image");const auto geometry=GeometryDescriptor::fromJson(sourceAcquisition["geometry"].toObject());if(!image.valid || !std::holds_alternative<ImageFrame>(image.data) || std::get<ImageFrame>(image.data).size()!=geometry.resolution)throw std::runtime_error("Actual frame/physical source geometry mismatch");
                    }
                    const auto declaredOutputs=outputPorts(t.kind);
                    for(const auto& value:r.outputs)if(!experimentalEditorReplay && (!value.origin.experimentalAcquisition.isEmpty() || (std::holds_alternative<ImageFrame>(value.data) && !std::get<ImageFrame>(value.data).metadata().experimentalAcquisition.isEmpty())))throw std::runtime_error("Experimental output requires explicit Editor policy");
                    const bool referenceProducer=std::any_of(declaredOutputs.begin(),declaredOutputs.end(),[&](const auto& p){return referenceSourcePort(t.kind,p.id);});
                    for(auto& value:r.outputs)if((value.valid && int(value.port.type)>int(PortType::Boolean)) || (referenceProducer && (value.port.type==PortType::Point || value.port.id=="found")) || t.kind=="pattern_pose" || t.kind=="shape_pose") {
                        if(!value.evaluationId.isEmpty() && value.evaluationId!=cycle.evaluationId)throw std::runtime_error("Output evaluation identity mismatch");
                        value.evaluationId=cycle.evaluationId;
                        if(value.method.isEmpty() && (t.kind=="template_match" || t.kind=="blob" || t.kind=="edge")) {
                            value.method=t.kind=="template_match" ? "template-ncc":t.kind=="blob" ? "connected-components":"legacy-axis-difference";value.methodVersion=1;
                        }
                    }
                    for(auto it=r.outputs.cbegin();it!=r.outputs.cend();++it) {
                        const auto expected=std::find_if(declaredOutputs.begin(),declaredOutputs.end(),[&](const auto& p){return p.id==it.key();});
                        if(expected==declaredOutputs.end())throw std::runtime_error("Undeclared output port");
                        ToolResultCodec::validate(*it,*expected,toolFrameId,cycle.cycleId);
                    }
                    if(multi && r.state==ExecutionState::Done) {
                        if(cancel && cancel->load())throw std::runtime_error("Acquisition/execution cancelled");
                        if(total.elapsed()>=limits.deadlineMs)throw std::runtime_error("Multi-source total deadline exceeded; late result rejected");
                        if(ToolRegistry::get(t.kind).acquisitionSource) {
                            const auto raw=r.outputs.value("image");
                            SourceFrameIndex index(cycle.cycleId,{t.id},{raw.origin});index.validatePort(t.id,raw,outputPorts(t.kind).first());
                        }
                        for(const auto& value:r.outputs)if(value.valid) {
                            const auto raw=completed.value(source).outputs.value("image");
                            const auto origin=ToolRegistry::get(t.kind).acquisitionSource ? value.origin:raw.origin;
                            SourceFrameIndex index(cycle.cycleId,{source},{origin});index.validatePort(source,value,value.port);
                            if(const auto* image=std::get_if<ImageFrame>(&value.data)) {
                                const quint64 bytes=quint64(image->stride())*image->size().height()+quint64(image->preview().sizeInBytes());
                                if(bytes>limits.imageBytes-retainedBytes)throw std::runtime_error("Multi-source retained image budget exceeded");
                                retainedBytes+=bytes;
                            }
                        }
                    }
                    if(r.state==ExecutionState::Error)error=true;
                    if(ToolRegistry::get(t.kind).acquisitionSource && r.state==ExecutionState::Done && resolver) {
                        try {measurement=resolver(std::get<ImageFrame>(r.outputs.value("image").data));}
                        catch(const std::exception& e){measurement={};measurement.error=QString::fromUtf8(e.what());}
                        if(multi)measurements[t.id]=measurement;
                    }
                }catch(const std::exception& e) {r.fitDiagnostic.reset();r.edgeDiagnostics.reset();r.outputs.clear();r.regions.clear();r.mask={};r.searchRegion={};r.points.clear();r.rejectedPoints.clear();r.lines.clear();r.decision=PartDecision::NotEvaluated;r.calibration.reset();r.state=ExecutionState::Error;r.message=QString::fromUtf8(e.what());
                    if(const auto* fit=dynamic_cast<const FitDiagnosticError*>(&e)) {
                        r.fitDiagnostic=fit->evidence;r.errorCode=fit->evidence.code;
                        r.message=presentMeasurementDiagnostic(*r.fitDiagnostic);
                    }
                    error=true;}
                if(r.fitDiagnostic){auto& d=*r.fitDiagnostic;d.toolId=t.id;d.sourceId=inputs.value("image").origin.sourceId;d.frameId=toolFrameId;d.cycleId=cycle.cycleId;d.evaluationId=cycle.evaluationId;}
            }
            if(r.state==ExecutionState::Done && industrialEvidence)cycle.industrialEvidence[id]=std::move(industrialEvidence);
            for(const auto& value:r.outputs)if(value.valid && std::holds_alternative<ImageFrame>(value.data) && (!multi || ToolRegistry::get(t.kind).acquisitionSource))cycle.frames.append(value.origin);
            r.milliseconds=timer.nsecsElapsed()/1e6;completed[id]=r;cycle.tools<<r;
        }
        if(cancel && cancel->load())stopped=true;
        if(multi && !stopped && total.elapsed()>=limits.deadlineMs)throw std::runtime_error("Multi-source total deadline exceeded");
        if(multi && !targetOnly && !stopped && !error && !graph.requiredSources.isEmpty()){SourceFrameIndex index(cycle.cycleId,graph.requiredSources,cycle.frames);cycle.frameId=index.legacyFrameId();}
        cycle.state=stopped ? ExecutionState::Skipped : error ? ExecutionState::Error : ExecutionState::Done;
        if(targetOnly && !stopped && cycle.tools.size()==1)cycle.state=cycle.tools.first().state;
        if(stopped) {cycle.industrialEvidence.clear();cycle.frames.clear();cycle.error="Çevrim iptal edildi; sonuç yayımlanmaz.";for(auto& r:cycle.tools) {r.state=ExecutionState::Skipped;r.message="İptal edildi";r.errorCode.clear();r.fitDiagnostic.reset();r.edgeDiagnostics.reset();r.outputs.clear();r.regions.clear();r.mask={};r.points.clear();r.rejectedPoints.clear();r.lines.clear();r.searchRegion={};r.decision=PartDecision::NotEvaluated;r.calibration.reset();}}
    } catch(const std::exception& e) {cycle.state=ExecutionState::Error;cycle.error=QString::fromUtf8(e.what());if(multi){cycle.industrialEvidence.clear();cycle.frames.clear();for(auto& r:cycle.tools){r.state=ExecutionState::Skipped;r.outputs.clear();r.regions.clear();r.points.clear();r.lines.clear();r.mask={};r.fitDiagnostic.reset();r.edgeDiagnostics.reset();r.decision=PartDecision::NotEvaluated;r.calibration.reset();}}}
    cycle.milliseconds=total.nsecsElapsed()/1e6;return cycle;
}
}


