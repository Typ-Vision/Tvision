#include "EditorSession.hpp"
#include "MeasurementService.hpp"
#include <QJsonDocument>
#include <QFile>
#include <QJsonArray>
#include <QFileInfo>
#include <QUuid>
#include "domain/ToolRegistry.hpp"
#include "PoseMatchService.hpp"
#include "ShapeMatchService.hpp"
#include "PoseRoiService.hpp"
#include "ToolResultCodec.hpp"
#include <QCryptographicHash>
#include <algorithm>
#include <stdexcept>
namespace visionflow {
JobSession::~JobSession() {for(const auto& t:tokens_)t->store(true);for(auto& f:futures_)f.waitForFinished();}
bool JobSession::busy(SessionJob job) const {return std::any_of(active_.begin(),active_.end(),[job](auto value){return value==job;});}
SessionState JobSession::state() const {if(!busy())return SessionState::Idle;for(const auto& job:active_)if(token(job) && token(job)->load())return SessionState::Cancelling;return SessionState::Working;}
std::shared_ptr<std::atomic_bool> JobSession::token(SessionJob job) const {return tokens_.value(job);}
void JobSession::stop(SessionJob job) {if(busy(job) && token(job))token(job)->store(true);}
void JobSession::requestShutdown(int budgetMs) {
    if(budgetMs<=0)throw std::runtime_error("Kapanış bütçesi pozitif olmalı.");
    if(!shutdownDeadline_)shutdownDeadline_=Deadline(budgetMs);
    for(const auto& token:tokens_)token->store(true);
}
ShutdownState JobSession::shutdownState() const {
    if(!shutdownDeadline_)return ShutdownState::NotRequested;
    if(!busy() && std::all_of(futures_.begin(),futures_.end(),[](const auto& f){return f.isFinished();}))return ShutdownState::Ready;
    return shutdownDeadline_->expired() ? ShutdownState::BudgetExceeded : ShutdownState::Pending;
}
quint64 JobSession::begin(SessionJob job) {
    if(shutdownDeadline_)throw std::runtime_error("Kapanış istenmiş oturum yeni iş kabul etmez.");
    if(job!=SessionJob::Image && busy())throw std::runtime_error("Oturumda başka iş sürüyor.");
    if(job==SessionJob::Image && (busy(SessionJob::Execution) || busy(SessionJob::Teaching)))throw std::runtime_error("Editor worker sürerken görüntü değiştirilemez.");
    if(!busy(job))tokens_[job]=std::make_shared<std::atomic_bool>(false);
    futures_.erase(std::remove_if(futures_.begin(),futures_.end(),[](const auto& f){return f.isFinished();}),futures_.end());
    const auto ticket=++serial_;active_[ticket]=job;return ticket;
}
void JobSession::finish(quint64 ticket) {active_.remove(ticket);}
QByteArray EditorSession::signature(const EditorContext& c) const {
    if(c.recipeId.isEmpty())return {};
    auto bytes=c.projectId.toUtf8()+c.path.toUtf8()+c.recipeId.toUtf8()+QByteArray::number(generation)+QJsonDocument(c.definition).toJson(QJsonDocument::Compact)+QJsonDocument(c.calibrationBinding).toJson(QJsonDocument::Compact);
    if(!c.systemCalibration.isEmpty())bytes+=QJsonDocument(c.systemCalibration).toJson(QJsonDocument::Compact);return QCryptographicHash::hash(bytes,QCryptographicHash::Sha256);
}
QByteArray EditorSession::referenceSignature(const EditorContext& c,const QString& tool) const {
    if(c.recipeId.isEmpty() || !frame)return {};
    auto connections=editorConnections(c.definition);QSet<QString> required{tool};QStringList pending{tool};
    while(!pending.isEmpty()){auto id=pending.takeFirst();for(const auto& b:connections)if(b.targetTool==id && !required.contains(b.sourceTool)){required.insert(b.sourceTool);pending.append(b.sourceTool);}}
    auto tools=editorTools(c.definition);tools.erase(std::remove_if(tools.begin(),tools.end(),[&](const auto& t){return !required.contains(t.id);}),tools.end());
    connections.erase(std::remove_if(connections.begin(),connections.end(),[&](const auto& b){return !required.contains(b.targetTool);}),connections.end());
    auto upstream=c;upstream.definition=withEditorConnections(withEditorTools({},tools),connections);return signature(upstream);
}
std::optional<QPointF> EditorSession::referencePoint(const EditorContext& c,const QString& source) const {
    if(cycle && accepts(cycle->configId,c))for(const auto& r:cycle->tools)if(r.toolId==source) {auto p=r.outputs.value("point");if(p.valid && std::holds_alternative<QPointF>(p.data))return std::get<QPointF>(p.data);return {};}
    for(const auto& t:editorTools(c.definition))if(t.id==source && t.kind=="template_match") {
        auto cached=references.constFind(source);if(cached!=references.cend() && cached->first==referenceSignature(c,source) && t.enabled)return cached->second;
        auto roi=MatchService::rectangle(t.parameters["modelRoi"].toObject());if(!roi.isEmpty())return QPointF(roi.x()+(roi.width()-1)/2.0,roi.y()+(roi.height()-1)/2.0);
    }return {};
}
QByteArray EditorSession::evaluationScope(const EditorContext& c) const {
    if(!frame)return {};
    const auto& m=frame->metadata();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(QJsonDocument(QJsonArray{c.projectId,c.path,c.recipeId,QString::number(generation),m.sourceId,m.frameId,m.cycleId,m.coordinateSystemId}).toJson(QJsonDocument::Compact));
    hash.addData(frame->contentId());hash.addData(frame->processingId());hash.addData(QJsonDocument(c.calibrationBinding).toJson(QJsonDocument::Compact));
    if(!c.systemCalibration.isEmpty())hash.addData(QJsonDocument(c.systemCalibration).toJson(QJsonDocument::Compact));
    QFile catalog(c.path+".calibrations.json");if(catalog.open(QIODevice::ReadOnly))hash.addData(QCryptographicHash::hash(catalog.readAll(),QCryptographicHash::Sha256));
    return hash.result();
}
void EditorSession::cacheResult(const EditorContext& c,const ToolResult& r) {
    cacheContext_=c;
    cache_[r.toolId]={ExecutionService::toolSignature(c.definition,QFileInfo(c.path).absolutePath(),r.toolId,evaluationScope(c)),r};
}
void EditorSession::adoptFrame(const EditorContext& c) {
    cache_.clear();if(!frame)return;
    for(const auto& t:editorTools(c.definition))if(ToolRegistry::get(t.kind).acquisitionSource && t.enabled) {
        auto m=frame->metadata();if(!m.sourceId.isEmpty() && m.sourceId!=t.id)return;
        m.sourceId=t.id;if(m.frameId.isEmpty())m.frameId=QUuid::createUuid().toString(QUuid::WithoutBraces);
        if(m.cycleId.isEmpty())m.cycleId=QUuid::createUuid().toString(QUuid::WithoutBraces);
        if(m.coordinateSystemId.isEmpty())m.coordinateSystemId=t.id+"/image";
        if(!m.completedAtUtc.isValid())m.completedAtUtc=QDateTime::currentDateTimeUtc();
        frame=frame->withMetadata(m);ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;
        r.outputs["image"]={outputPorts(t.kind).first(),*frame,true,m.frameId,m.cycleId,m};cacheResult(c,r);return;
    }
}
TargetEvaluation EditorSession::evaluation(const EditorContext& c) const {
    if(!frame)throw std::runtime_error("Önce görüntü al.");
    TargetEvaluation e{*frame,evaluationScope(c),{}};
    for(auto it=cache_.cbegin();it!=cache_.cend();++it)if(it->signature==ExecutionService::toolSignature(c.definition,QFileInfo(c.path).absolutePath(),it.key(),e.scope))e.cache[it.key()]=*it;
    return e;
}
void EditorSession::reconcile(const EditorContext& c) {
    if(!frame){cache_.clear();if(cycle && cycle->state==ExecutionState::Done)cycle.reset();return;}
    const auto e=evaluation(c);cache_=e.cache;
    if(cycle) {
        cycle->tools.erase(std::remove_if(cycle->tools.begin(),cycle->tools.end(),[&](const auto& r){return !cache_.contains(r.toolId);}),cycle->tools.end());
        if(cycle->tools.isEmpty())cycle.reset();else cycle->configId=signature(c);
    }
}
void EditorSession::invalidateTool(const EditorContext& c,const QString& tool) {
    QSet<QString> removed{tool};QStringList pending{tool};const auto connections=editorConnections(c.definition);
    while(!pending.isEmpty()){const auto id=pending.takeFirst();for(const auto& link:connections)if(link.sourceTool==id && !removed.contains(link.targetTool)){removed.insert(link.targetTool);pending.append(link.targetTool);}}
    for(const auto& id:removed){cache_.remove(id);references.remove(id);}
    if(cycle)cycle->tools.erase(std::remove_if(cycle->tools.begin(),cycle->tools.end(),[&](const auto& r){return removed.contains(r.toolId);}),cycle->tools.end());
}
bool EditorSession::acceptTool(CycleResult result,const QByteArray& captured,const EditorContext& current) {
    if(!accepts(captured,current))return false;
    if(result.targetToolId.isEmpty() || result.configId!=ExecutionService::toolSignature(current.definition,QFileInfo(current.path).absolutePath(),result.targetToolId,evaluationScope(current)))return false;
    reconcile(current);invalidateTool(current,result.targetToolId);
    if(token(SessionJob::Execution) && token(SessionJob::Execution)->load())suppressCancelled(result);
    for(const auto& r:result.tools)cacheResult(current,r);
    auto presentation=result;presentation.tools.clear();
    for(const auto& t:editorTools(current.definition))if(cache_.contains(t.id))presentation.tools.append(cache_[t.id].result);
    presentation.configId=signature(current);cycle=std::move(presentation);return true;
}
void EditorSession::invalidateResults() {cycle.reset();matchOutcome.reset();resultSignature.clear();}
void EditorSession::calibrationChanged(const EditorContext& c) {
    if(busy())throw std::runtime_error("Calibration cache change while worker running");
    if(!frame || cacheContext_.projectId!=c.projectId || cacheContext_.path!=c.path || cacheContext_.recipeId!=c.recipeId || cacheContext_.definition!=c.definition){cache_.clear();invalidateResults();return;}
    cache_=evaluation(cacheContext_).cache;const auto tools=editorTools(c.definition);
    for(const auto& t:tools)if(ToolRegistry::get(t.kind).usesCalibration || t.parameters["mode"]=="calibrated" || (t.parameters.contains("referenceBinding") && toolReferenceBinding(t).world) || isWorldPoseTracking(t))invalidateTool(cacheContext_,t.id);
    for(const auto& t:tools)if(t.kind=="edge_point" && cache_.contains(t.id)){auto& r=cache_[t.id].result;r.outputs.remove("pointMm");r.calibration.reset();}
    const auto scope=evaluationScope(c);for(auto i=cache_.begin();i!=cache_.end();++i)i->signature=ExecutionService::toolSignature(c.definition,QFileInfo(c.path).absolutePath(),i.key(),scope);
    cacheContext_=c;references.clear();matchOutcome.reset();resultSignature.clear();if(cycle){cycle->configId=signature(c);cycle->tools.erase(std::remove_if(cycle->tools.begin(),cycle->tools.end(),[&](const auto& r){return !cache_.contains(r.toolId);}),cycle->tools.end());for(auto& r:cycle->tools)r=cache_.value(r.toolId).result;}
}
bool EditorSession::acceptCycle(CycleResult result,const QByteArray& captured,const EditorContext& current) {
    if(token(SessionJob::Execution) && token(SessionJob::Execution)->load())suppressCancelled(result);
    if(!accepts(captured,current)){invalidateResults();return false;}
    cycle=std::move(result);frame.reset();references.clear();
    if(cycle->state!=ExecutionState::Skipped)for(const auto& r:cycle->tools)for(const auto& t:editorTools(current.definition))if(t.id==r.toolId && ToolRegistry::get(t.kind).acquisitionSource)for(const auto& v:r.outputs)if(v.valid)if(auto* image=std::get_if<ImageFrame>(&v.data))frame=*image;
    cache_.clear();if(frame)for(const auto& r:cycle->tools)cacheResult(current,r);
    if(frame)for(const auto& r:cycle->tools) {
        auto point=r.outputs.value("point");if(!point.valid || !std::holds_alternative<QPointF>(point.data))continue;
        for(const auto& t:editorTools(current.definition))if(t.id==r.toolId && t.kind=="template_match")references[t.id]={referenceSignature(current,t.id),std::get<QPointF>(point.data)};
    }return true;
}
ImageLoad EditorSession::load(const QString& path) {try{return {ImageFileService{}.load(path),{}};}catch(const std::exception& e){return {{},QString::fromUtf8(e.what())};}}
ModelTeaching EditorSession::teach(const ImageFrame& frame,const QRect& roi) {try{return {MatchService{}.teach(frame,roi),{}};}catch(const std::exception& e){return {{},QString::fromUtf8(e.what())};}}
PoseTeaching EditorSession::teachPose(const ToolDefinition& tool,const ImageFrame& image,const QRect& roi,QPointF anchor) {
    try {
        if(tool.kind!="pattern_pose" && tool.kind!="shape_pose")throw std::runtime_error("Pose teaching requires a pose tool");
        return {(tool.kind=="shape_pose" ? ShapeMatchService::teach:PoseMatchService::teach)(image,roi,anchor,tool.parameters),{}};
    }catch(const std::exception& e){return {{},QString::fromUtf8(e.what())};}
}
ImageFrame EditorSession::toolImage(const EditorContext& c,const QString& tool) const {
    const auto e=evaluation(c);
    for(const auto& link:editorConnections(c.definition))if(link.targetTool==tool && link.targetPort=="image") {
        const auto cached=e.cache.constFind(link.sourceTool);
        if(cached==e.cache.cend() || cached->result.state!=ExecutionState::Done)break;
        const auto port=cached->result.outputs.value(link.sourcePort);
        ToolResultCodec::validate(port,{"image",PortType::Image,"px","image"},e.frame.metadata().frameId,e.frame.metadata().cycleId);
        if(port.valid && std::holds_alternative<ImageFrame>(port.data))return std::get<ImageFrame>(port.data);
    }
    throw std::runtime_error("Güncel image girişi yok. Görüntü kaynağını bağlayıp önce kaynak/Preprocess çalıştırın.");
}
ToolDefinition EditorSession::taughtReference(const EditorContext& c,const ToolDefinition& consumer,const QString& producer,const QString& port,QPointF offset,double angle) const {
    if(token(SessionJob::Execution) && token(SessionJob::Execution)->load())throw std::runtime_error("Reference teaching cancelled");
    const auto e=evaluation(c);const auto cached=e.cache.constFind(producer);
    if(cached==e.cache.cend())throw std::runtime_error("Run current reference source first");
    for(const auto& source:editorTools(c.definition))if(source.id==producer)
        return ReferenceService::teach(consumer,source,port,cached->result,toolImage(c,consumer.id),offset,angle);
    throw std::runtime_error("Reference producer missing");
}
TrackingTeachingInput EditorSession::trackingTeachingInput(const EditorContext& c,const ToolDefinition& consumer,const QString& producer,double mmPerSample,const MeasurementExecutionContext& calibration) const {
    const auto e=evaluation(c);const auto image=toolImage(c,consumer.id);
    for(const auto& source:editorTools(c.definition))if(source.id==producer) {
        const auto cached=e.cache.constFind(producer);
        if(cached==e.cache.cend())throw std::runtime_error("Güncel pose sonucu yok. Önce referans aracını Run Tool ile çalıştırın.");
        return {consumer,source,cached->result,image,mmPerSample,calibration};
    }
    throw std::runtime_error("Pose referansı bulunamadı.");
}
TrackingTeaching EditorSession::teachTracking(const TrackingTeachingInput& input,const std::shared_ptr<std::atomic_bool>& cancel) {
    try {
        if(cancel && cancel->load())throw std::runtime_error("Teaching cancelled");
        auto unbound=input.consumer;unbound.parameters.remove("poseTracking");
        const auto roi=input.consumer.parameters["scanRoi"].toObject();
        if(isWorldPoseTracking(input.consumer) || input.consumer.parameters["mode"]=="calibrated" || (!roi.isEmpty() && ToolRoi::fromJson(roi).frame.unit==LengthUnit::Millimetre))return {PoseRoiService::teachWorldTool(unbound,ToolRoi::fromJson(roi),input.mmPerSample,input.source,input.reference,input.image,input.calibration,cancel),{}};
        return {PoseRoiService::teachTool(unbound,{input.source.id+"/model-local",LengthUnit::Pixel,AxisConvention::XRightYDown},input.source,input.reference,input.image),{}};
    }catch(const std::exception& e){return {{},QString::fromUtf8(e.what())};}
}
TrackingTeaching EditorSession::teachReference(const TrackingTeachingInput& input,const QString& port,QPointF offset,double angle,const std::shared_ptr<std::atomic_bool>& cancel){
    try{
        if(cancel && cancel->load())throw std::runtime_error("Teaching cancelled");
        const auto json=input.consumer.parameters["scanRoi"].toObject();
        const bool world=!json.isEmpty() && ToolRoi::fromJson(json).frame.unit==LengthUnit::Millimetre;
        auto taught=world ? ReferenceService::teachWorld(input.consumer,ToolRoi::fromJson(json),input.mmPerSample,input.source,port,input.reference,input.image,input.calibration,offset,angle,cancel)
            :ReferenceService::teach(input.consumer,input.source,port,input.reference,input.image,offset,angle);
        if(taught.kind=="circle_fit" && input.calibration.calibration && input.calibration.calibration->systemBound() && input.calibration.current)taught.parameters["worldCalibration"]=input.calibration.current->toJson();
        if(cancel && cancel->load())throw std::runtime_error("Teaching cancelled");return {taught,{}};
    }catch(const std::exception& e){return {{},QString::fromUtf8(e.what())};}
}
ToolDefinition EditorSession::taughtTracking(const EditorContext& c,const ToolDefinition& consumer,const QString& producer,double mmPerSample,const MeasurementExecutionContext& calibration) const {
    const auto taught=teachTracking(trackingTeachingInput(c,consumer,producer,mmPerSample,calibration));
    if(!taught.tool)throw std::runtime_error(taught.error.toUtf8().constData());return *taught.tool;
}
QJsonObject EditorSession::taughtSearch(const EditorContext& c,const ToolDefinition& tool,const QRect& roi) const {
    auto p=tool.parameters;
    if(tool.kind=="edge")for(const auto& b:editorConnections(c.definition))if(b.targetTool==tool.id && b.targetPort=="reference") {
        auto point=referencePoint(c,b.sourceTool);if(!point)throw std::runtime_error("Referans bulunamadı. Önce geçerli match sonucu üretin.");
        p["referenceOrigin"]=QJsonObject{{"x",point->x()},{"y",point->y()}};
        for(const auto& source:editorTools(c.definition))if(source.id==b.sourceTool)p["referenceGeometry"]=referenceGeometry(source);
    }p["searchRoi"]=MatchService::rectangle(roi);return p;
}
void EditorSession::suppressCancelled(CycleResult& c) {c.frames.clear();c.state=ExecutionState::Skipped;c.error="Çevrim iptal edildi.";for(auto& r:c.tools){r.edgeDiagnostics.reset();r.outputs.clear();r.regions.clear();r.mask={};r.points.clear();r.rejectedPoints.clear();r.lines.clear();r.searchRegion={};r.decision=PartDecision::NotEvaluated;r.calibration.reset();}}
}
