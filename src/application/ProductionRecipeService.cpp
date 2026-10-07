#include "ProductionRecipeService.hpp"
#include "MatchService.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QUuid>
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
        {"definition",r.definition},{"projectDirectory",QDir(directory).absolutePath()}});
}
bool sha256(const QString& s) {return s.size()==64 && std::all_of(s.begin(),s.end(),[](QChar c){return (c>='0' && c<='9') || (c>='a' && c<='f');});}
bool terminal(ExecutionState s) {return s==ExecutionState::Done || s==ExecutionState::Error || s==ExecutionState::Skipped;}
}
ProductionException::ProductionException(ProductionError code,const QString& message):std::runtime_error(message.toUtf8().constData()),code_(code) {}
ProductionRecipeService::ProductionRecipeService(Authorize authorize,Dependencies dependencies,int limit)
    :authorize_(std::move(authorize)),dependencies_(std::move(dependencies)),historyLimit_(limit) {
    if(limit<1 || limit>256)fail(ProductionError::InvalidConfiguration,"Snapshot sınırı 1–256 olmalı.");
}
void ProductionRecipeService::owner() const {if(QThread::currentThread()!=ownerThread_)fail(ProductionError::WrongThread,"Production controller owner thread gerektirir.");}
void ProductionRecipeService::permission(const QString& principal,ProductionAction action) const {
    owner();if(principal.isEmpty() || !authorize_ || !authorize_(principal,action))fail(ProductionError::Unauthorized,"Servis yetkisi reddedildi.");
}
void ProductionRecipeService::idle() const {if(cycle_.ticket)fail(ProductionError::Busy,"Aktif çevrim bitmeden sürüm değiştirilemez.");}
QJsonObject ProductionRecipeService::dependencies(const Recipe& recipe) const {
    const auto current=dependencies_ ? dependencies_(recipe) : ProductionDependencies{};
    if(current.engineVersion!=ExecutionService::version())fail(ProductionError::DependencyChanged,"Engine sürümü uyumsuz.");
    QJsonObject tools,models,assets;
    for(const auto& tool:editorTools(recipe.definition)) {
        tools[tool.id]=QJsonObject{{"kind",tool.kind},{"version",ToolRegistry::get(tool.kind).version}};
        if(tool.kind=="template_match")models[tool.id]=referenceGeometry(tool);
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
        calibration=current.calibration->toJson();
    } else if(!recipe.calibrationReference.isEmpty())fail(ProductionError::DependencyChanged,"Kalibrasyon bağımlılığı çözümlenmedi.");
    return {{"engine",current.engineVersion},{"tools",tools},{"models",models},{"assets",assets},{"calibration",calibration}};
}
void ProductionRecipeService::requireCurrent(const ProductionSnapshot& s) const {
    if(hash(dependencies(s.recipe()))!=s.validation().dependencyId)fail(ProductionError::DependencyChanged,"Doğrulama bağımlılığı değişti; yeniden doğrulama gerekir.");
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
        const auto tools=editorTools(draft.definition);const auto connections=editorConnections(draft.definition);
        const auto graph=validateGraph(tools,connections);
        if(!graph.valid())throw std::runtime_error(graph.errors.join("\n").toUtf8().constData());
        int sources=0;
        for(const auto& tool:tools)if(tool.enabled) {
            if(!QStringList{"image_file","template_match","blob","edge","calculate"}.contains(tool.kind))throw std::runtime_error("Üretim sözleşmesinde desteklenmeyen tool.");
            if(tool.kind=="image_file") {++sources;if(tool.parameters["path"].toString().isEmpty())throw std::runtime_error("Kaynak yolu boş.");}
            if(tool.kind=="template_match")MatchService::validateModel(MatchService::parameters(tool.parameters));
            for(const auto& c:connections)if(c.targetTool==tool.id) {
                const auto source=std::find_if(tools.begin(),tools.end(),[&](const auto& t){return t.id==c.sourceTool;});
                if(!source->enabled)throw std::runtime_error("Enabled tool disabled kaynağa bağlı.");
                if(c.targetPort=="reference" && (referenceNeedsTeaching(tool,*source) || tool.parameters["searchRoi"].toObject().isEmpty() || tool.parameters["referenceOrigin"].toObject().isEmpty()))throw std::runtime_error("Tracking ROI yeniden öğretim gerektirir.");
            }
        }
        if(sources!=1)throw std::runtime_error("Tek enabled Image File kaynağı gerekir.");
        auto s=std::make_shared<ProductionSnapshot>();s->recipe_=draft;s->directory_=QDir(directory).absolutePath();s->dependencies_=dependencies(draft);
        s->validation_={QUuid::createUuid().toString(QUuid::WithoutBraces),principal,content(draft,directory),hash(s->dependencies_),QDateTime::currentDateTimeUtc()};
        validated_.insert(s->validation_.id,s);return s->validation_;
    } catch(const ProductionException&) {throw;}
      catch(const std::exception& e) {fail(ProductionError::InvalidConfiguration,QString::fromUtf8(e.what()));}
}
void ProductionRecipeService::activate(const QString& principal,const QString& id,const Recipe& draft,const QString& directory) {
    permission(principal,ProductionAction::Activate);idle();
    const auto s=validated_.value(id);if(!s)fail(ProductionError::UnknownValidation,"Serviste oluşturulmuş doğrulama bulunamadı.");
    if(content(draft,directory)!=s->validation().contentId)fail(ProductionError::DependencyChanged,"Taslak içeriği veya proje yolu değişti.");
    requireCurrent(*s);active_=s;if(!activated_.contains(id))activated_.append(id);
}
void ProductionRecipeService::rollback(const QString& principal,const QString& id) {
    permission(principal,ProductionAction::Rollback);idle();
    if(!activated_.contains(id))fail(ProductionError::UnknownValidation,"Daha önce etkinleşmiş snapshot bulunamadı.");
    const auto s=validated_.value(id);requireCurrent(*s);active_=s;
}
std::shared_ptr<const ProductionSnapshot> ProductionRecipeService::active() const {owner();return active_;}
bool ProductionRecipeService::busy() const {owner();return cycle_.ticket!=0;}
QVector<QJsonObject> ProductionRecipeService::cycleRecords() const {owner();return records_;}
ProductionCycle ProductionRecipeService::beginCycle(const QString& principal) {
    permission(principal,ProductionAction::Run);idle();if(!active_)fail(ProductionError::UnknownValidation,"Aktif snapshot yok.");
    requireCurrent(*active_);cycle_={++serial_,active_};cyclePrincipal_=principal;cycleStarted_=QDateTime::currentDateTimeUtc();return cycle_;
}
void ProductionRecipeService::finishCycle(quint64 ticket,const CycleResult& r) {
    owner();if(!ticket || ticket!=cycle_.ticket)fail(ProductionError::InvalidCompletion,"Eski veya yabancı çevrim ticket.");
    if(!terminal(r.state) || r.cycleId.isEmpty() || r.recipeId!=cycle_.snapshot->recipe().id || r.configId!=cycle_.snapshot->validation().contentId || !std::isfinite(r.milliseconds) || r.milliseconds<0)fail(ProductionError::InvalidCompletion,"Çevrim sonucu snapshot ile uyuşmuyor.");
    for(const auto& f:r.frames)if(f.cycleId!=r.cycleId || f.frameId.isEmpty() || f.sourceId.isEmpty())fail(ProductionError::InvalidCompletion,"Çevrim frame kökeni uyumsuz.");
    for(const auto& log:records_)if(log["cycleId"]==r.cycleId)fail(ProductionError::InvalidCompletion,"Çevrim kimliği daha önce tamamlandı.");
    for(const auto& t:r.tools) {
        if(!terminal(t.state))fail(ProductionError::InvalidCompletion,"Tool sonucu terminal değil.");
        for(const auto& value:t.outputs)if(value.valid && (value.cycleId!=r.cycleId || value.frameId!=r.frameId || value.origin.cycleId!=r.cycleId || value.origin.frameId!=r.frameId))fail(ProductionError::InvalidCompletion,"Çıktı kökeni çevrimle uyuşmuyor.");
    }
    record(&r,{});cycle_={};
}
void ProductionRecipeService::abandonCycle(quint64 ticket,const QString& reason) {
    owner();if(!ticket || ticket!=cycle_.ticket || reason.isEmpty())fail(ProductionError::InvalidCompletion,"Çevrim iptal ticket/reason geçersiz.");
    record(nullptr,reason);cycle_={};
}
void ProductionRecipeService::record(const CycleResult* r,const QString& error) {
    const auto& s=*cycle_.snapshot;QJsonArray frames,tools;
    if(r) {
        for(const auto& f:r->frames)frames.append(QJsonObject{{"frameId",f.frameId},{"cycleId",f.cycleId},{"sourceId",f.sourceId},{"coordinateSystemId",f.coordinateSystemId},{"completedAtUtc",f.completedAtUtc.toString(Qt::ISODateWithMs)}});
        for(const auto& t:r->tools) {
            QJsonObject outputs;
            for(auto it=t.outputs.cbegin();it!=t.outputs.cend();++it)if(it->valid) {
                QJsonObject output{{"type",portTypeName(it->port.type)},{"unit",it->port.unit},{"frameId",it->frameId}};
                if(const auto p=std::get_if<double>(&it->data))output["value"]=*p;
                else if(const auto p=std::get_if<bool>(&it->data))output["value"]=*p;
                else if(const auto p=std::get_if<QPointF>(&it->data))output["value"]=QJsonObject{{"x",p->x()},{"y",p->y()}};
                else if(const auto p=std::get_if<QRect>(&it->data))output["value"]=MatchService::rectangle(*p);
                // Image bytes belong to the frame store, never the cycle trace.
                outputs[it.key()]=output;
            }
            tools.append(QJsonObject{{"toolId",t.toolId},{"state",executionStateName(t.state)},{"message",t.message},{"milliseconds",t.milliseconds},{"outputs",outputs}});
        }
    }
    records_.append({{"schemaVersion",1},{"ticket",QString::number(cycle_.ticket)},{"principal",cyclePrincipal_},
        {"recipeId",s.recipe().id},{"recipeVersion",s.recipe().recipeVersion},{"validationId",s.validation().id},
        {"configId",QString::fromLatin1(s.validation().contentId)},{"dependencyId",QString::fromLatin1(s.validation().dependencyId)},
        {"dependencies",s.dependencies()},{"startedAtUtc",cycleStarted_.toString(Qt::ISODateWithMs)},
        {"completedAtUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"cycleId",r ? r->cycleId : QString{}},{"state",r ? executionStateName(r->state) : "ERROR"},
        {"error",r ? r->error : error},{"frames",frames},{"tools",tools},{"communicationState","NOT_REQUESTED"}});
    if(records_.size()>128)records_.removeFirst();
}
}
