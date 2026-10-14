#include "ExecutionService.hpp"
#include "ToolExecutors.hpp"
#include "ToolResultCodec.hpp"
#include "ToolDependencyResolver.hpp"
#include "PoseRoiService.hpp"
#include "ReferenceService.hpp"
#include "WorldTrackingService.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "AcquisitionService.hpp"
#include <QDir>
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
        if(t.kind=="image_file") {QFile file(QDir(directory).absoluteFilePath(t.parameters["path"].toString()));if(!file.open(QIODevice::ReadOnly))hash.addData("MISSING_ASSET");else hash.addData(QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256));}
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
CycleResult ExecutionService::run(const QJsonObject& definition,const QString& directory,const QString& recipeId,const QByteArray& configId,const std::shared_ptr<std::atomic_bool>& cancel,const QString& targetTool,const std::optional<ImageFrame>& frameSnapshot,const CalibrationResolver& resolver,const TargetEvaluation* targetOnly) const {
    CycleResult cycle;cycle.cycleId=QUuid::createUuid().toString(QUuid::WithoutBraces);cycle.frameId=QUuid::createUuid().toString(QUuid::WithoutBraces);cycle.recipeId=recipeId;cycle.configId=configId;
    cycle.targetToolId=targetTool;cycle.evaluationId=QUuid::createUuid().toString(QUuid::WithoutBraces);cycle.partial=targetOnly!=nullptr;
    if(targetOnly){cycle.frameId=targetOnly->frame.metadata().frameId;cycle.cycleId=targetOnly->frame.metadata().cycleId;}
    QElapsedTimer total;total.start();
    try {
        validateEditorDefinition(definition);auto tools=editorTools(definition);auto connections=editorConnections(definition);
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
        auto graph=validateGraph(tools,connections);
        if(!graph.valid())throw std::runtime_error(graph.errors.join("\n").toUtf8().constData());
        cycle.state=ExecutionState::Busy;QMap<QString,ToolResult> completed;bool error=false,stopped=false;MeasurementExecutionContext measurement;
        if(targetOnly) {
            if(cycle.frameId.isEmpty() || cycle.cycleId.isEmpty())throw std::runtime_error("Önce görüntü al.");
            if(resolver)measurement=resolver(targetOnly->frame);
            for(auto it=targetOnly->cache.cbegin();it!=targetOnly->cache.cend();++it)
                if(it->signature==toolSignature(definition,directory,it.key(),targetOnly->scope))completed[it.key()]=it->result;
        }
        for(const auto& id:graph.order) {
            if(targetOnly && id!=targetTool)continue;
            const auto t=*std::find_if(tools.begin(),tools.end(),[&](const auto& t){return t.id==id;});ToolResult r;r.toolId=id;QElapsedTimer timer;timer.start();
            if(cancel && cancel->load())stopped=true;
            if(targetOnly && ToolRegistry::get(t.kind).acquisitionSource)throw std::runtime_error("Kaynak için görüntü al / Run Flow kullanın.");
            if(!t.enabled || stopped) {r.state=ExecutionState::Skipped;r.message=stopped ? "İptal edildi" : "Disabled";}
            else {
                QMap<QString,PortValue> inputs;bool unavailable=false;QStringList missing;
                for(const auto& c:connections)if(c.targetTool==id) {
                    const auto source=completed.value(c.sourceTool);const auto value=source.outputs.value(c.sourcePort);
                    if(source.state!=ExecutionState::Done || !value.valid || value.frameId!=cycle.frameId || value.cycleId!=cycle.cycleId) {
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
                        ToolResultCodec::validate(*it,declared,cycle.frameId,cycle.cycleId);
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
                        if(frameSnapshot->size().isEmpty())throw std::runtime_error("Frame snapshot boş.");
                        auto metadata=frameSnapshot->metadata();
                        if(!metadata.sourceId.isEmpty() && metadata.sourceId!=t.id)throw std::runtime_error("Frame snapshot kaynak kimliği uyumsuz.");
                        metadata.sourceId=t.id;metadata.frameId=cycle.frameId;metadata.cycleId=cycle.cycleId;
                        if(metadata.coordinateSystemId.isEmpty())metadata.coordinateSystemId=t.id+"/image";
                        if(!metadata.completedAtUtc.isValid())metadata.completedAtUtc=QDateTime::currentDateTimeUtc();
                        r.state=ExecutionState::Done;r.outputs["image"]={outputPorts(t.kind).first(),frameSnapshot->withMetadata(metadata),true,cycle.frameId,cycle.cycleId,metadata};
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
                            r=worldTracking ? WorldTrackingService::execute(t,std::get<ImageFrame>(inputs.value("image").data),*worldTracking,cancel)
                                :ToolExecutors::get(t.kind,measurement,resolvedRoi)(effective,inputs,directory,cycle.frameId,cycle.cycleId,cancel);
                        }
                    }
                    const auto declaredOutputs=outputPorts(t.kind);
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
                        ToolResultCodec::validate(*it,*expected,cycle.frameId,cycle.cycleId);
                    }
                    if(r.state==ExecutionState::Error)error=true;
                    if(ToolRegistry::get(t.kind).acquisitionSource && r.state==ExecutionState::Done && resolver) {
                        try {measurement=resolver(std::get<ImageFrame>(r.outputs.value("image").data));}
                        catch(const std::exception& e){measurement={};measurement.error=QString::fromUtf8(e.what());}
                    }
                }catch(const std::exception& e) {r.edgeDiagnostics.reset();r.outputs.clear();r.points.clear();r.rejectedPoints.clear();r.lines.clear();r.decision=PartDecision::NotEvaluated;r.calibration.reset();r.state=ExecutionState::Error;r.message=QString::fromUtf8(e.what());error=true;}
            }
            for(const auto& value:r.outputs)if(value.valid && std::holds_alternative<ImageFrame>(value.data))cycle.frames.append(value.origin);
            r.milliseconds=timer.nsecsElapsed()/1e6;completed[id]=r;cycle.tools<<r;
        }
        if(cancel && cancel->load())stopped=true;
        cycle.state=stopped ? ExecutionState::Skipped : error ? ExecutionState::Error : ExecutionState::Done;
        if(targetOnly && !stopped && cycle.tools.size()==1)cycle.state=cycle.tools.first().state;
        if(stopped) {cycle.frames.clear();cycle.error="Çevrim iptal edildi; sonuç yayımlanmaz.";for(auto& r:cycle.tools) {r.edgeDiagnostics.reset();r.outputs.clear();r.regions.clear();r.mask={};r.points.clear();r.rejectedPoints.clear();r.lines.clear();r.searchRegion={};r.decision=PartDecision::NotEvaluated;r.calibration.reset();}}
    } catch(const std::exception& e) {cycle.state=ExecutionState::Error;cycle.error=QString::fromUtf8(e.what());}
    cycle.milliseconds=total.nsecsElapsed()/1e6;return cycle;
}
}


