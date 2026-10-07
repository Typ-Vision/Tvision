#include "ExecutionService.hpp"
#include "ToolExecutors.hpp"
#include "domain/ToolRegistry.hpp"
#include "AcquisitionService.hpp"
#include <QDir>
#include <QElapsedTimer>
#include <QUuid>
#include <QSet>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace visionflow {
QString executionStateName(ExecutionState s) {switch(s){case ExecutionState::Ready:return "READY";case ExecutionState::Busy:return "BUSY";case ExecutionState::Done:return "DONE";case ExecutionState::Error:return "ERROR";case ExecutionState::Skipped:return "SKIPPED";}return {};}
CycleResult ExecutionService::run(const QJsonObject& definition,const QString& directory,const QString& recipeId,const QByteArray& configId,const std::shared_ptr<std::atomic_bool>& cancel,const QString& targetTool,const std::optional<ImageFrame>& frameSnapshot,const CalibrationResolver& resolver) const {
    CycleResult cycle;cycle.cycleId=QUuid::createUuid().toString(QUuid::WithoutBraces);cycle.frameId=QUuid::createUuid().toString(QUuid::WithoutBraces);cycle.recipeId=recipeId;cycle.configId=configId;
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
        for(const auto& id:graph.order) {
            const auto t=*std::find_if(tools.begin(),tools.end(),[&](const auto& t){return t.id==id;});ToolResult r;r.toolId=id;QElapsedTimer timer;timer.start();
            if(cancel && cancel->load())stopped=true;
            if(!t.enabled || stopped) {r.state=ExecutionState::Skipped;r.message=stopped ? "İptal edildi" : "Disabled";}
            else {
                QMap<QString,PortValue> inputs;bool unavailable=false;
                for(const auto& c:connections)if(c.targetTool==id) {
                    const auto source=completed.value(c.sourceTool);const auto value=source.outputs.value(c.sourcePort);
                    if(source.state!=ExecutionState::Done || !value.valid || value.frameId!=cycle.frameId || value.cycleId!=cycle.cycleId)unavailable=true;
                    else inputs[c.targetPort]=value;
                }
                if(unavailable) {r.state=ExecutionState::Skipped;r.message="Upstream sonucu geçersiz/ERROR/SKIPPED";}
                else try {
                    const auto declaredInputs=inputPorts(t.kind);
                    for(auto it=inputs.cbegin();it!=inputs.cend();++it) {
                        const auto expected=std::find_if(declaredInputs.begin(),declaredInputs.end(),[&](const auto& p){return p.id==it.key();});
                        if(expected==declaredInputs.end() || expected->type!=it->port.type || expected->unit!=it->port.unit || expected->frame!=it->port.frame)
                            throw std::runtime_error("Giriş port tipi/birim/frame sözleşmesi uyumsuz.");
                    }
                    QString coordinates,sourceFrame,sourceId;
                    for(const auto& value:inputs) {
                        const auto& m=value.origin;
                        if(m.frameId.isEmpty() || m.frameId!=value.frameId || m.cycleId!=value.cycleId || m.coordinateSystemId.isEmpty())throw std::runtime_error("Giriş frame metadata eksik/uyumsuz.");
                        if(!coordinates.isEmpty() && (coordinates!=m.coordinateSystemId || sourceFrame!=m.frameId || sourceId!=m.sourceId))throw std::runtime_error("Giriş frame/koordinat sistemleri uyumsuz.");
                        coordinates=m.coordinateSystemId;sourceFrame=m.frameId;sourceId=m.sourceId;
                    }
                    for(const auto& c:connections)if(c.targetTool==id && c.targetPort=="reference") {
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
                    } else r=ToolExecutors::get(t.kind,measurement)(t,inputs,directory,cycle.frameId,cycle.cycleId,cancel);
                    if(r.state==ExecutionState::Error)error=true;
                    if(ToolRegistry::get(t.kind).acquisitionSource && r.state==ExecutionState::Done && resolver) {
                        try {measurement=resolver(std::get<ImageFrame>(r.outputs.value("image").data));}
                        catch(const std::exception& e){measurement={};measurement.error=QString::fromUtf8(e.what());}
                    }
                }catch(const std::exception& e) {r.state=ExecutionState::Error;r.message=QString::fromUtf8(e.what());error=true;}
            }
            for(const auto& value:r.outputs)if(value.valid && std::holds_alternative<ImageFrame>(value.data))cycle.frames.append(value.origin);
            r.milliseconds=timer.nsecsElapsed()/1e6;completed[id]=r;cycle.tools<<r;
        }
        if(cancel && cancel->load())stopped=true;
        cycle.state=stopped ? ExecutionState::Skipped : error ? ExecutionState::Error : ExecutionState::Done;
        if(stopped) {cycle.frames.clear();cycle.error="Çevrim iptal edildi; sonuç yayımlanmaz.";for(auto& r:cycle.tools) {r.outputs.clear();r.regions.clear();r.points.clear();r.lines.clear();r.searchRegion={};r.decision=PartDecision::NotEvaluated;r.calibration.reset();}}
    } catch(const std::exception& e) {cycle.state=ExecutionState::Error;cycle.error=QString::fromUtf8(e.what());}
    cycle.milliseconds=total.nsecsElapsed()/1e6;return cycle;
}
}


