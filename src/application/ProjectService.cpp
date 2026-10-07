#include "ProjectService.hpp"
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
void ProjectService::setTools(const QVector<ToolDefinition>& tools) {
    const auto* active=activeRecipe(); if (!active) fail("Önce bir reçete açın."); const QString id=active->id;
    auto candidate=project();
    for (auto& r : candidate.recipes) if (r.id==id) {
        auto definition=withEditorTools(r.definition,tools);
        auto connections=editorConnections(definition);
        const auto previous=editorTools(r.definition);
        auto removed=[&](const QString& id){
            return std::any_of(previous.begin(),previous.end(),[&](const auto& t){return t.id==id;}) &&
                std::none_of(tools.begin(),tools.end(),[&](const auto& t){return t.id==id;});
        };
        connections.erase(std::remove_if(connections.begin(),connections.end(),[&](const auto& c){return removed(c.sourceTool) || removed(c.targetTool);}),connections.end());
        definition=withEditorConnections(definition,connections);
        if (definition==r.definition || definition==withEditorTools(r.definition,editorTools(r.definition))) return;
        if(r.calibrationBinding) {
            auto sourceConfig=[](const QVector<ToolDefinition>& list){QJsonArray sources;for(const auto& t:list)if(t.kind=="image_file" || t.kind=="webcam")sources.append(QJsonObject{{"id",t.id},{"kind",t.kind},{"enabled",t.enabled},{"parameters",t.parameters}});return sources;};
            if(sourceConfig(previous)!=sourceConfig(tools))r.calibrationBinding.reset();
        }
        r.definition=definition; r.status=RecipeStatus::Draft; validateProject(candidate);
        project_=candidate; changedRecipes_.insert(id); dirty_=true; return;
    }
}
QString ProjectService::addTool(const QString& kind) {
    auto list=tools(); const auto params=defaultParameters(kind);
    if (kind=="image_file") for (const auto& t : list) if (t.kind==kind) { return t.id; }
    int suffix=1; QString name;
    do {
        name=toolLabel(kind).replace(' ','_')+"_"+QString::number(suffix++);
    } while (std::any_of(list.begin(),list.end(),[&](const auto& t){return t.name.toCaseFolded()==name.toCaseFolded();}));
    ToolDefinition tool{newId(),kind,name,"",true,params}; list.append(tool); setTools(list); return tool.id;
}
void ProjectService::updateTool(const ToolDefinition& tool) {
    auto list=tools();
    for (auto& t : list) if (t.id==tool.id) {
        if (t.kind!=tool.kind) fail("Tool türü değiştirilemez.");
        t=tool; setTools(list); return;
    }
    fail("Tool bulunamadı.");
}
void ProjectService::removeTool(const QString& id) {
    auto list=tools();
    const auto it=std::find_if(list.begin(),list.end(),[&](const auto& t){return t.id==id;});
    if (it==list.end()) fail("Tool bulunamadı."); list.erase(it); setTools(list);
}
QString ProjectService::setImageFile(const QString& absolutePath) {
    if (!QFileInfo(absolutePath).isAbsolute() || !QFileInfo(absolutePath).isFile()) fail("Görüntü dosyası yolu mevcut ve mutlak olmalı.");
    auto list=tools();
    auto it=std::find_if(list.begin(),list.end(),[](const auto& t){return t.kind=="image_file";});
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
void ProjectService::clearCalibrationBinding() {
    if(!activeRecipe())fail("Önce reçete açın.");if(!activeRecipe()->calibrationBinding)return;
    const auto id=activeRecipe()->id;auto candidate=project();
    for(auto& r:candidate.recipes)if(r.id==id) {
        r.calibrationBinding.reset();r.calibrationReference.clear();r.status=RecipeStatus::Draft;
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
