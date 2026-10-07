#include "EditorSession.hpp"
#include <QJsonDocument>
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
    return QCryptographicHash::hash(c.projectId.toUtf8()+c.path.toUtf8()+c.recipeId.toUtf8()+QByteArray::number(generation)+QJsonDocument(c.definition).toJson(QJsonDocument::Compact)+QJsonDocument(c.calibrationBinding).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);
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
void EditorSession::invalidateResults() {cycle.reset();matchOutcome.reset();resultSignature.clear();}
bool EditorSession::acceptCycle(CycleResult result,const QByteArray& captured,const EditorContext& current) {
    if(token(SessionJob::Execution) && token(SessionJob::Execution)->load())suppressCancelled(result);
    if(!accepts(captured,current)){invalidateResults();return false;}
    cycle=std::move(result);frame.reset();references.clear();
    if(cycle->state!=ExecutionState::Skipped)for(const auto& r:cycle->tools)for(const auto& v:r.outputs)if(v.valid)if(auto* image=std::get_if<ImageFrame>(&v.data))frame=*image;
    if(frame)for(const auto& r:cycle->tools) {
        auto point=r.outputs.value("point");if(!point.valid || !std::holds_alternative<QPointF>(point.data))continue;
        for(const auto& t:editorTools(current.definition))if(t.id==r.toolId && t.kind=="template_match")references[t.id]={referenceSignature(current,t.id),std::get<QPointF>(point.data)};
    }return true;
}
ImageLoad EditorSession::load(const QString& path) {try{return {ImageFileService{}.load(path),{}};}catch(const std::exception& e){return {{},QString::fromUtf8(e.what())};}}
ModelTeaching EditorSession::teach(const ImageFrame& frame,const QRect& roi) {try{return {MatchService{}.teach(frame,roi),{}};}catch(const std::exception& e){return {{},QString::fromUtf8(e.what())};}}
QJsonObject EditorSession::taughtSearch(const EditorContext& c,const ToolDefinition& tool,const QRect& roi) const {
    auto p=tool.parameters;
    if(tool.kind=="edge")for(const auto& b:editorConnections(c.definition))if(b.targetTool==tool.id && b.targetPort=="reference") {
        auto point=referencePoint(c,b.sourceTool);if(!point)throw std::runtime_error("Referans bulunamadı. Önce geçerli match sonucu üretin.");
        p["referenceOrigin"]=QJsonObject{{"x",point->x()},{"y",point->y()}};
        for(const auto& source:editorTools(c.definition))if(source.id==b.sourceTool)p["referenceGeometry"]=referenceGeometry(source);
    }p["searchRoi"]=MatchService::rectangle(roi);return p;
}
void EditorSession::suppressCancelled(CycleResult& c) {c.frames.clear();c.state=ExecutionState::Skipped;c.error="Çevrim iptal edildi.";for(auto& r:c.tools){r.outputs.clear();r.regions.clear();r.points.clear();r.lines.clear();r.searchRegion={};r.decision=PartDecision::NotEvaluated;r.calibration.reset();}}
}
