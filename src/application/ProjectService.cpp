#include "application/EditorObservation.hpp"
#include "ProjectService.hpp"
#include "BackendCapabilities.hpp"
#include "domain/ToolRegistry.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QLockFile>
#include <QUuid>
#include <stdexcept>
#include <algorithm>

namespace visionflow {
namespace {
QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
void fail(const QString& message) { throw std::runtime_error(message.toUtf8().constData()); }
QByteArray readBytes(const QString& path) {
    QFile file(path); if (!file.open(QIODevice::ReadOnly)) fail("Proje dosyası kontrol edilemiyor: " + file.errorString());
    if (file.size()>16*1024*1024) fail("Proje dosyası 16 MB sınırını aşıyor.");
    const auto bytes=file.readAll(); if (file.error()!=QFileDevice::NoError) fail("Proje dosyası okunamadı."); return bytes;
}
}
const Project& ProjectService::project() const {
    if (!project_) fail("Önce bir proje oluşturun veya açın."); return *project_;
}
const Recipe* ProjectService::activeRecipe() const {
    if (project_) for (const auto& recipe : project_->recipes) if (recipe.id==activeId_) return &recipe;
    return nullptr;
}
void ProjectService::createProject(QString name, QString description, const QString& path, QString camera,
                                   QString robot, QString protocol, QString calibration) {
    if (dirty_) fail("Mevcut değişiklikleri kaydetmeden proje değiştirilemez.");
    if (path.trimmed().isEmpty() || QFileInfo::exists(path)) fail("Yeni proje için mevcut olmayan bir dosya yolu seçin.");
    if (!QFileInfo(QFileInfo(path).absolutePath()).isDir()) fail("Proje klasörü mevcut olmalı.");
    Project p; p.id=newId(); p.name=name.trimmed(); p.description=description;
    p.cameraReference=camera.trimmed(); p.robotType=robot.trimmed(); p.communicationProtocol=protocol.trimmed();
    p.calibrationReference=calibration.trimmed(); p.createdAt=p.modifiedAt=QDateTime::currentDateTimeUtc();
    validateProject(p);
    project_=p; savedProject_.reset(); path_=QFileInfo(path).absoluteFilePath(); activeId_.clear();
    dirty_=true; changedRecipes_.clear(); newRecipes_.clear(); diskSnapshot_.clear();
}
void ProjectService::openProject(const QString& path) {
    if (dirty_) fail("Mevcut değişiklikleri kaydetmeden proje değiştirilemez.");
    const auto bytes=readBytes(path);
    const auto candidate=repository_.load(path);
    if (bytes!=readBytes(path)) fail("Proje açılırken dosya başka bir süreç tarafından değiştirildi. Tekrar açın.");
    project_=candidate; savedProject_=candidate; path_=QFileInfo(path).absoluteFilePath(); activeId_.clear();
    dirty_=false; changedRecipes_.clear(); newRecipes_.clear(); diskSnapshot_=bytes;
}
void ProjectService::updateProject(QString name, QString description, QString camera, QString robot, QString protocol, QString calibration) {
    auto candidate=project(); candidate.name=name.trimmed(); candidate.description=description; candidate.cameraReference=camera.trimmed();
    candidate.robotType=robot.trimmed(); candidate.communicationProtocol=protocol.trimmed(); candidate.calibrationReference=calibration.trimmed();
    validateProject(candidate);
    if (ProjectRepository::encode(candidate)==ProjectRepository::encode(*project_)) return;
    project_=candidate; dirty_=true;
}
QString ProjectService::createRecipe(QString name, QString description, ApplicationType type, QString camera, QString calibration) {
    auto candidate=project(); Recipe recipe; recipe.id=newId(); recipe.name=name.trimmed(); recipe.description=description;
    recipe.applicationType=type; recipe.cameraReference=camera.trimmed(); recipe.calibrationReference=calibration.trimmed();
    recipe.createdAt=recipe.modifiedAt=QDateTime::currentDateTimeUtc();
    candidate.recipes.append(recipe); validateProject(candidate);
    project_=candidate; activeId_=recipe.id; changedRecipes_.insert(recipe.id); newRecipes_.insert(recipe.id); dirty_=true;
    return recipe.id;
}
void ProjectService::editRecipeMetadata(const QString& id, QString name, QString description) {
    // Metadata edits must not clear legacy calibration or source-scoped bindings.
    auto candidate=project();
    for (auto& recipe : candidate.recipes) if (recipe.id==id) {
        if (recipe.name==name.trimmed() && recipe.description==description) return;
        recipe.name=name.trimmed(); recipe.description=description; recipe.status=RecipeStatus::Draft;
        validateProject(candidate);
        project_=candidate; changedRecipes_.insert(id); dirty_=true; return;
    }
    fail("Reçete bulunamadı.");
}
void ProjectService::editRecipe(const QString& id, QString name, QString description, QString camera, QString calibration) {
    auto candidate=project();
    for (auto& recipe : candidate.recipes) if (recipe.id==id) {
        if (recipe.name==name.trimmed() && recipe.description==description && recipe.cameraReference==camera.trimmed() && recipe.calibrationReference==calibration.trimmed()) return;
        if(recipe.cameraReference!=camera.trimmed() || recipe.calibrationReference!=calibration.trimmed())recipe.calibrationBinding.reset();
        recipe.name=name.trimmed(); recipe.description=description; recipe.cameraReference=camera.trimmed(); recipe.calibrationReference=calibration.trimmed();
        recipe.status=RecipeStatus::Draft; validateProject(candidate);
        project_=candidate; changedRecipes_.insert(id); dirty_=true; return;
    }
    fail("Reçete bulunamadı.");
}
void ProjectService::selectRecipe(const QString& id) {
    for (const auto& r : project().recipes) if (r.id==id) { activeId_=id; return; }
    fail("Reçete bulunamadı.");
}
void ProjectService::save() {
    editor_observation::Scope observation(editor_observation::ProjectSave);
    project();
    if (!dirty_) return;
    QLockFile lock(path_+".lock");
    if (!lock.tryLock(0)) fail("Proje başka bir VisionFlow oturumu tarafından kaydediliyor.");
    if (diskSnapshot_.isEmpty()) {
        if (QFileInfo::exists(path_)) fail("Proje yolu artık mevcut. Dosya korunarak kayıt iptal edildi.");
    } else if (readBytes(path_)!=diskSnapshot_) fail("Dosya dışarıdan değiştirilmiş. Değişiklikleriniz bellekte korunuyor; dosyayı yeniden açın.");
    auto candidate=*project_; const auto now=QDateTime::currentDateTimeUtc(); candidate.modifiedAt=now;
    for (auto& r : candidate.recipes) if (changedRecipes_.contains(r.id)) {
        if (!newRecipes_.contains(r.id)) ++r.recipeVersion;
        r.modifiedAt=now;
    }
    // Publish new revisions and clear dirty flags only after atomic commit succeeds.
    repository_.save(path_,candidate);
    project_=candidate; savedProject_=candidate; diskSnapshot_=ProjectRepository::encode(candidate);
    dirty_=false; changedRecipes_.clear(); newRecipes_.clear();
}
void ProjectService::discardChanges() {
    project_=savedProject_; dirty_=false; changedRecipes_.clear(); newRecipes_.clear();
    if (!activeRecipe()) activeId_.clear();
    if (!project_) { path_.clear(); diskSnapshot_.clear(); }
}
QVector<ToolDefinition> ProjectService::tools() const {
    const auto* recipe=activeRecipe(); if (!recipe) fail("Önce bir reçete açın."); return editorTools(recipe->definition);
}
QVector<VariableDefinition> ProjectService::variables() const {
    if(!activeRecipe())fail("Open a recipe first.");return recipeVariables(activeRecipe()->definition);
}
void ProjectService::setVariables(const QVector<VariableDefinition>& variables) {
    if(!activeRecipe())fail("Open a recipe first.");
    const auto previous=this->variables();if(previous==variables)return;
    const auto definition=withRecipeVariables(activeRecipe()->definition,variables);
    for(const auto& old:previous) {
        auto next=std::find_if(variables.begin(),variables.end(),[&](const auto& v){return v.id==old.id;});
        const bool contractChanged=next==variables.end() || next->type!=old.type || next->unit!=old.unit ||
            next->frameConstraint!=old.frameConstraint || next->enumDefinitionId!=old.enumDefinitionId || next->enumCodes!=old.enumCodes;
        if(contractChanged && variableReferenced(activeRecipe()->definition,old.id))
            fail("Variable is referenced; remove its bindings before deleting or changing type/unit/frame/enum.");
    }
    auto candidate=project();const auto id=activeRecipe()->id;
    for(auto& recipe:candidate.recipes)if(recipe.id==id){recipe.definition=definition;recipe.status=RecipeStatus::Draft;}
    validateProject(candidate);project_=candidate;changedRecipes_.insert(id);dirty_=true;
}
QString ProjectService::addVariable(VariableDefinition definition) {
    auto list=variables();if(!definition.id.isEmpty())fail("New variable UUID is generated by the service.");
    definition.id=newId();list.append(definition);setVariables(list);return definition.id;
}
void ProjectService::updateVariable(const VariableDefinition& definition) {
    auto list=variables();for(auto& v:list)if(v.id==definition.id){v=definition;setVariables(list);return;}
    fail("Variable not found.");
}
void ProjectService::renameVariable(const QString& id,QString name) {
    for(auto v:variables())if(v.id==id){v.name=name;updateVariable(v);return;}
    fail("Variable not found.");
}
void ProjectService::removeVariable(const QString& id) {
    auto list=variables();for(auto i=list.begin();i!=list.end();++i)if(i->id==id){list.erase(i);setVariables(list);return;}
    fail("Variable not found.");
}
void ProjectService::setTools(const QVector<ToolDefinition>& tools,const QVector<ToolConnection>* bindings) {
    const auto* active=activeRecipe(); if (!active) fail("Önce bir reçete açın."); const QString id=active->id;
    auto candidate=project();
    for (auto& r : candidate.recipes) if (r.id==id) {
        auto base=r.definition;if(base.contains("sourceSetup") && std::none_of(tools.begin(),tools.end(),[&](const auto& t){return t.id==base["sourceSetup"].toObject()["sourceToolId"].toString();}))base.remove("sourceSetup");if(base.contains("sourceSetups")){auto bindings=base["sourceSetups"].toObject();for(auto i=bindings.begin();i!=bindings.end();)if(std::none_of(tools.begin(),tools.end(),[&](const auto& t){return t.id==i.key();}))i=bindings.erase(i);else ++i;base["sourceSetups"]=bindings;}
        auto definition=withEditorTools(base,tools);if(definition.contains("sourceSetup")){const auto source=definition["sourceSetup"].toObject()["sourceToolId"].toString();if(std::none_of(tools.begin(),tools.end(),[&](const auto& t){return t.id==source;}))definition.remove("sourceSetup");}
        auto connections=bindings ? *bindings:editorConnections(definition);
        const auto previous=editorTools(r.definition);
        auto removed=[&](const QString& id){
            return std::any_of(previous.begin(),previous.end(),[&](const auto& t){return t.id==id;}) &&
                std::none_of(tools.begin(),tools.end(),[&](const auto& t){return t.id==id;});
        };
        connections.erase(std::remove_if(connections.begin(),connections.end(),[&](const auto& c){return removed(c.sourceTool) || removed(c.targetTool);}),connections.end());
        definition=withEditorConnections(definition,connections);
        if (definition==r.definition || definition==withEditorTools(r.definition,editorTools(r.definition))) return;
        if(r.calibrationBinding) {
            auto sourceConfig=[](const QVector<ToolDefinition>& list){QJsonArray sources;for(const auto& t:list)if(ToolRegistry::get(t.kind).acquisitionSource)sources.append(QJsonObject{{"id",t.id},{"kind",t.kind},{"enabled",t.enabled},{"parameters",t.parameters}});return sources;};
            if(sourceConfig(previous)!=sourceConfig(tools))r.calibrationBinding.reset();
        }
        r.definition=definition; r.status=RecipeStatus::Draft; validateProject(candidate);
        project_=candidate; changedRecipes_.insert(id); dirty_=true; return;
    }
}
void ProjectService::enableMultiSource() {
    if(!activeRecipe())fail("Recipe required");const auto definition=withMultiSourceSchema(activeRecipe()->definition);
    if(definition==activeRecipe()->definition)return;
    for(auto& r:project_->recipes)if(r.id==activeId_){r.definition=definition;r.status=RecipeStatus::Draft;}
    dirty_=true;changedRecipes_.insert(activeId_);
}
QString ProjectService::addTool(const QString& kind) {
    auto list=tools(); auto params=defaultParameters(kind);
    // Creation policy only; registry defaults remain compatible with pixel records.
    if(kind=="circle_fit") {
        params["mode"]="calibrated";
        for(const auto* key:{"scanWidth","inlierDistance","maxResidual","unitsPerSample"})params[key]=.1;
        params["maxRms"]=.035;
    }
    if (kind=="image_file" && activeRecipe()->definition["editor"].toObject()["schemaVersion"].toInt()!=9) for (const auto& t : list) if (t.kind==kind) { return t.id; }
    int suffix=1; QString name;
    do {
        name=toolLabel(kind).replace(' ','_')+"_"+QString::number(suffix++);
    } while (std::any_of(list.begin(),list.end(),[&](const auto& t){return t.name.toCaseFolded()==name.toCaseFolded();}));
    ToolDefinition tool{newId(),kind,name,"",kind!="result",params};
    const auto backend=ToolRegistry::get(kind).requiredBackend;
    if(!backend.isEmpty()){const auto capability=BackendCapabilities::get(backend);if(!capability.available)fail(capability.reason);tool.dependencies.append({ToolDependencyKind::Backend,backend,capability.version,capability.contentId,{}});}
    auto connections=editorConnections(activeRecipe()->definition);
    const auto inputs=inputPorts(kind);
    const auto image=std::find_if(inputs.begin(),inputs.end(),[](const auto& port){return port.id=="image" && port.type==PortType::Image;});
    if(image!=inputs.end()) {
        QVector<QString> sources;
        for(const auto& t:list)if(ToolRegistry::get(t.kind).acquisitionSource)sources.append(t.id);
        // Only an unambiguous acquisition chain can supply a creation default.
        QString producer;
        if(sources.size()==1) {
            QSet<QString> reachable;
            for(const auto& t:list)if(t.id==sources.first() && t.enabled)reachable.insert(t.id);
            bool changed=true;
            while(changed) {
                changed=false;
                for(const auto& t:list)if(t.enabled && t.kind=="preprocess" && !reachable.contains(t.id)) {
                    const auto input=[&](const ToolConnection& c){return c.targetTool==t.id && c.targetPort=="image";};
                    if(std::count_if(connections.begin(),connections.end(),input)!=1)continue;
                    for(const auto& c:connections)if(input(c) && reachable.contains(c.sourceTool) && c.sourcePort=="image") {reachable.insert(t.id);changed=true;break;}
                }
            }
            for(const auto& t:list)if(reachable.contains(t.id)) {
                for(const auto& output:outputPorts(t.kind))if(output.id=="image" && output.type==image->type && output.unit==image->unit && output.frame==image->frame)
                    if(t.kind=="preprocess" || producer.isEmpty())producer=t.id;
            }
        }
        if(!producer.isEmpty())connections.append({producer,"image",tool.id,"image"});
    }
    list.append(tool);
    const auto baseline=validateGraph(list,editorConnections(activeRecipe()->definition));
    const auto graph=validateGraph(list,connections);
    for(const auto& error:graph.errors)if(!baseline.errors.contains(error))fail(error);
    setTools(list,&connections);return tool.id;
}
void ProjectService::updateTool(const ToolDefinition& tool) {
    auto list=tools();
    for (auto& t : list) if (t.id==tool.id) {
        if (t.kind!=tool.kind) fail("Tool türü değiştirilemez.");
        if(tool.kind=="result") {
            const auto before=validateGraph(list,editorConnections(activeRecipe()->definition));auto changed=list;
            for(auto& candidate:changed)if(candidate.id==tool.id)candidate=tool;
            for(const auto& error:validateGraph(changed,editorConnections(activeRecipe()->definition)).errors)if(!before.errors.contains(error))fail(error);
        }
        t=tool; setTools(list); return;
    }
    fail("Tool bulunamadı.");
}
void ProjectService::bindIndustrialSource(const QString& id,const MeasurementSetupContract& contract,const ProfileId& profile,int timeoutMs) {
    const auto reference=IndustrialSourceReference::bind(contract,profile,timeoutMs);
    for(auto tool:tools())if(tool.id==id){
        if(tool.kind!="industrial_camera")fail("Industrial source tool required");
        tool.parameters={{"sourceReference",reference.toJson()}};updateTool(tool);return;
    }
    fail("Tool bulunamadı.");
}
void ProjectService::updateToolConnections(const ToolDefinition& tool,const QVector<ToolConnection>& connections) {
    if(!activeRecipe())fail("Önce reçete açın.");
    auto updated=tools();bool found=false;
    for(auto& t:updated)if(t.id==tool.id){if(t.kind!=tool.kind)fail("Tool türü değiştirilemez.");t=tool;found=true;}
    if(!found)fail("Tool bulunamadı.");
    const auto definition=withEditorConnections(withEditorTools(activeRecipe()->definition,updated),connections);
    validateEditorDefinition(definition);
    const auto graph=validateGraph(updated,connections);
    // An unfinished editor may have other unconnected tools. This edit must not
    // introduce any new graph failure, including tracking/region conflicts.
    const auto previous=editorConnections(activeRecipe()->definition);
    const auto before=validateGraph(tools(),previous);
    const auto imageInput=[&](const ToolConnection& c){return c.targetTool==tool.id && c.targetPort=="image";};
    const bool imageDisconnected=std::any_of(previous.begin(),previous.end(),imageInput) &&
        std::none_of(connections.begin(),connections.end(),imageInput);
    for(const auto& error:graph.errors) {
        // Explicitly clearing this image selector leaves an unfinished draft.
        // Execution still rejects the missing input; structural/teaching gates remain.
        if(imageDisconnected && error==tool.name+": eksik giriş image")continue;
        if(error.contains("DAG") || error.contains("uyumsuz") || !before.errors.contains(error))fail(error);
    }
    if(definition==activeRecipe()->definition)return;
    auto candidate=project();const auto id=activeRecipe()->id;
    for(auto& r:candidate.recipes)if(r.id==id){r.definition=definition;r.status=RecipeStatus::Draft;}
    validateProject(candidate);project_=candidate;changedRecipes_.insert(id);dirty_=true;
}
bool ProjectService::moveTool(const QString& id,int direction) {
    if(direction!=-1 && direction!=1)fail("Taşıma yönü -1 veya 1 olmalı.");
    auto list=tools();
    const auto it=std::find_if(list.begin(),list.end(),[&](const auto& t){return t.id==id;});
    if(it==list.end())fail("Tool bulunamadı.");
    const auto index=std::distance(list.begin(),it),target=index+direction;
    if(target<0 || target>=list.size())return false;
    std::swap(list[index],list[target]);
    setTools(list);return true;
}
void ProjectService::bindReference(const QString& target,const ReferenceBinding& binding) {
    auto list=tools();auto it=std::find_if(list.begin(),list.end(),[&](const auto& t){return t.id==target;});
    if(it==list.end())fail("Reference target missing");
    const auto source=std::find_if(list.begin(),list.end(),[&](const auto& t){return t.id==binding.sourceTool;});
    if(source==list.end() || binding.geometryId!=referenceGeometryIdentity(*source,binding.sourcePort))fail("Reference teaching geometry/source differs");
    auto t=*it;t.parameters.remove("poseTracking");t.parameters["referenceBinding"]=binding.toJson();
    auto links=editorConnections(activeRecipe()->definition);
    links.erase(std::remove_if(links.begin(),links.end(),[&](const auto& c){return c.targetTool==target && (c.targetPort=="reference" || c.targetPort=="poseReference");}),links.end());
    links.append({binding.sourceTool,binding.sourcePort,target,referenceInput(binding)});
    updateToolConnections(t,links);
}
void ProjectService::clearReference(const QString& target) {
    auto list=tools();auto it=std::find_if(list.begin(),list.end(),[&](const auto& t){return t.id==target;});
    if(it==list.end())fail("Reference target missing");
    auto t=*it;
    const bool world=t.parameters.contains("referenceBinding") ? toolReferenceBinding(t).world:isWorldPoseTracking(t);
    if(world && t.kind!="circle_fit"){t.parameters["scanRoi"]=QJsonObject{};if(t.kind!="edge_point")t.parameters["mode"]="pixel";}
    t.parameters.remove("referenceBinding");t.parameters.remove("poseTracking");
    auto links=editorConnections(activeRecipe()->definition);
    links.erase(std::remove_if(links.begin(),links.end(),[&](const auto& c){return c.targetTool==target && (c.targetPort=="reference" || c.targetPort=="poseReference");}),links.end());
    updateToolConnections(t,links);
}
void ProjectService::removeTool(const QString& id) {
    auto list=tools();
    const auto it=std::find_if(list.begin(),list.end(),[&](const auto& t){return t.id==id;});
    if (it==list.end()) fail("Tool bulunamadı."); list.erase(it); setTools(list);
}
QString ProjectService::setImageFile(const QString& absolutePath,const QString& source) {
    if (!QFileInfo(absolutePath).isAbsolute() || !QFileInfo(absolutePath).isFile()) fail("Görüntü dosyası yolu mevcut ve mutlak olmalı.");
    auto list=tools();
    auto it=std::find_if(list.begin(),list.end(),[&](const auto& t){return t.kind=="image_file" && (source.isEmpty() || t.id==source);});
    if(!source.isEmpty() && it==list.end())fail("Selected image source missing");
    if (it==list.end()) {
        ToolDefinition source{newId(),"image_file","Image_File_1","",true,defaultParameters("image_file")};
        int suffix=1;
        while (std::any_of(list.begin(),list.end(),[&](const auto& t){return t.name.toCaseFolded()==source.name.toCaseFolded();})) source.name="Image_File_"+QString::number(++suffix);
        list.append(source); it=list.end()-1;
    }
    it->parameters["path"]=QDir(QFileInfo(path_).absolutePath()).relativeFilePath(QFileInfo(absolutePath).absoluteFilePath());
    it->enabled=true; const auto id=it->id; setTools(list); return id;
}
void ProjectService::setConnections(const QVector<ToolConnection>& connections) {
    if (!activeRecipe()) fail("Önce reçete açın.");
    const auto id=activeRecipe()->id; auto candidate=project();
    for (auto& r:candidate.recipes) if (r.id==id) {
        auto updated=tools();const auto previous=editorConnections(r.definition);
        for(const auto& c:connections)if(c.targetPort=="reference" && !previous.contains(c))
            for(const auto& source:updated)if(source.id==c.sourceTool)
                for(auto& target:updated)if(target.id==c.targetTool && target.kind=="edge" && target.parameters["referenceGeometry"].toString().isEmpty())
                    target.parameters["referenceGeometry"]=referenceGeometry(source);
        auto definition=withEditorConnections(withEditorTools(r.definition,updated),connections);
        validateEditorDefinition(definition);
        if (definition==withEditorConnections(withEditorTools(r.definition,tools()),editorConnections(r.definition))) return;
        r.definition=definition; r.status=RecipeStatus::Draft; validateProject(candidate);
        project_=candidate; changedRecipes_.insert(id); dirty_=true; return;
    }
}
QString ProjectService::imageFilePath() const {
    if (!activeRecipe()) return {};
    for (const auto& t : tools()) if (t.kind=="image_file" && t.enabled) {
        const auto relative=t.parameters["path"].toString();
        if (relative.isEmpty()) return {};
        return QDir::cleanPath(QDir(QFileInfo(path_).absolutePath()).absoluteFilePath(relative));
    }
    return {};
}
void ProjectService::bindCalibration(const CalibrationDependency& dependency) {
    dependency.validate();if(!activeRecipe())fail("Önce reçete açın.");
    const auto id=activeRecipe()->id;auto candidate=project();
    for(auto& r:candidate.recipes)if(r.id==id) {
        if(r.calibrationBinding==dependency)return;
        r.schemaVersion=2;r.calibrationReference=dependency.calibrationId;r.calibrationBinding=dependency;r.status=RecipeStatus::Draft;
        validateProject(candidate);project_=candidate;changedRecipes_.insert(id);dirty_=true;return;
    }
}
void ProjectService::bindSourceSetup(const QString& source,const QJsonObject& target){
    if(!activeRecipe())fail("Önce reçete açın.");auto definition=activeRecipe()->definition;const auto binding=QJsonObject{{"schemaVersion",1},{"sourceToolId",source},{"target",target}};
    if(definition["editor"].toObject()["schemaVersion"]==9){auto bindings=definition["sourceSetups"].toObject();const auto legacy=definition.value("sourceSetup").toObject();if(!legacy.isEmpty()){bindings[legacy["sourceToolId"].toString()]=legacy;definition.remove("sourceSetup");}bindings[source]=binding;definition["sourceSetups"]=bindings;}else definition["sourceSetup"]=binding;definition["calibrationPolicy"]="scoped-source/1";validateEditorDefinition(definition);
    if(definition==activeRecipe()->definition)return;auto candidate=project();const auto id=activeRecipe()->id;
    for(auto& r:candidate.recipes)if(r.id==id){r.definition=definition;r.calibrationBinding.reset();r.calibrationReference.clear();r.status=RecipeStatus::Draft;}
    validateProject(candidate);project_=candidate;changedRecipes_.insert(id);dirty_=true;
}
void ProjectService::clearSourceSetup(const QString& source){
    if(activeRecipe() && activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9){auto d=activeRecipe()->definition;auto bindings=d.value("sourceSetups").toObject();if(!source.isEmpty() && !bindings.contains(source) && d.value("sourceSetup").toObject()["sourceToolId"]!=source)return;if(source.isEmpty())bindings={};else bindings.remove(source);d["sourceSetups"]=bindings;if(source.isEmpty() || d.value("sourceSetup").toObject()["sourceToolId"]==source)d.remove("sourceSetup");if(d==activeRecipe()->definition)return;for(auto& r:project_->recipes)if(r.id==activeId_){r.definition=d;r.status=RecipeStatus::Draft;}dirty_=true;changedRecipes_.insert(activeId_);return;}
if(!activeRecipe())fail("Önce reçete açın.");if(!activeRecipe()->definition.contains("sourceSetup"))return;auto candidate=project();const auto id=activeRecipe()->id;for(auto& r:candidate.recipes)if(r.id==id){r.definition.remove("sourceSetup");r.calibrationBinding.reset();r.calibrationReference.clear();r.status=RecipeStatus::Draft;}project_=candidate;changedRecipes_.insert(id);dirty_=true;}
void ProjectService::clearCalibrationBinding() {
    if(!activeRecipe())fail("Önce reçete açın.");if(!activeRecipe()->calibrationBinding)return;
    const auto id=activeRecipe()->id;auto candidate=project();
    for(auto& r:candidate.recipes)if(r.id==id) {
        r.calibrationBinding.reset();r.calibrationReference.clear();r.status=RecipeStatus::Draft;
        project_=candidate;changedRecipes_.insert(id);dirty_=true;return;
    }
}
void ProjectService::setRuntimeOptions(QString trigger,QString measurementToolId) {
    if(!activeRecipe())fail("Open a recipe first.");const auto id=activeRecipe()->id;auto candidate=project();
    QJsonObject options{{"trigger",trigger},{"measurementToolId",measurementToolId}};
    for(auto& r:candidate.recipes)if(r.id==id) {
        if(r.definition.value("runtime")==options)return;
        r.definition["runtime"]=options;r.status=RecipeStatus::Draft;validateProject(candidate);
        project_=candidate;changedRecipes_.insert(id);dirty_=true;return;
    }
}
void ProjectService::setCalibrationSetup(QString lensId,QString workingPlaneId) {
    if(!activeRecipe())fail("Önce reçete açın.");const auto id=activeRecipe()->id;auto candidate=project();
    const QJsonObject setup{{"lensId",lensId.trimmed()},{"workingPlaneId",workingPlaneId.trimmed()}};
    for(auto& r:candidate.recipes)if(r.id==id) {
        if(r.definition.value("planarSetup")==setup)return;
        r.definition["planarSetup"]=setup;r.calibrationBinding.reset();r.status=RecipeStatus::Draft;
        validateProject(candidate);project_=candidate;changedRecipes_.insert(id);dirty_=true;return;
    }
}
}
