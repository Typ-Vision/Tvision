#include "Project.hpp"
#include "ToolDefinition.hpp"
#include <QSet>
#include <QUuid>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool valid, const QString& message) {
    if (!valid) throw std::runtime_error(message.toUtf8().constData());
}
bool validName(const QString& name) {
    if (name.isEmpty() || name.size()>64 || name!=name.trimmed()) return false;
    for (QChar c : name) if (!c.isPrint()) return false;
    return true;
}
}
QString typeKey(ApplicationType type) {
    switch (type) {
    case ApplicationType::RobotGuidance: return "robot_guidance";
    case ApplicationType::DefectInspection: return "defect_inspection";
    case ApplicationType::Measurement: return "measurement";
    }
    throw std::runtime_error("Geçersiz uygulama türü.");
}
QString typeLabel(ApplicationType type) {
    switch (type) {
    case ApplicationType::RobotGuidance: return "Robot Guidance";
    case ApplicationType::DefectInspection: return "Defect Inspection";
    case ApplicationType::Measurement: return "Measurement";
    }
    throw std::runtime_error("Geçersiz uygulama türü.");
}
ApplicationType parseType(const QString& key) {
    if (key=="robot_guidance") return ApplicationType::RobotGuidance;
    if (key=="defect_inspection") return ApplicationType::DefectInspection;
    if (key=="measurement") return ApplicationType::Measurement;
    throw std::runtime_error("Desteklenmeyen applicationType.");
}
QString statusKey(RecipeStatus status) {
    switch (status) {
    case RecipeStatus::Draft: return "DRAFT";
    case RecipeStatus::Valid: return "VALID";
    case RecipeStatus::Invalid: return "INVALID";
    }
    throw std::runtime_error("Geçersiz reçete durumu.");
}
RecipeStatus parseStatus(const QString& key) {
    if (key=="DRAFT") return RecipeStatus::Draft;
    if (key=="VALID") return RecipeStatus::Valid;
    if (key=="INVALID") return RecipeStatus::Invalid;
    throw std::runtime_error("Desteklenmeyen reçete durumu.");
}
void validateProject(const Project& project) {
    require(project.schemaVersion==1,"Desteklenmeyen proje şema sürümü.");
    require(!QUuid(project.id).isNull(),"Proje ID geçersiz.");
    require(validName(project.name),"Proje adı 1–64 yazdırılabilir karakter olmalı.");
    require(!project.cameraReference.trimmed().isEmpty(),"Varsayılan görüntü kaynağı gerekli.");
    require(project.description.size()<=2048,"Proje açıklaması en fazla 2048 karakter olabilir.");
    require(project.createdAt.isValid() && project.modifiedAt.isValid(),"Proje tarihleri geçersiz.");
    QSet<QString> ids, names;
    for (const auto& recipe : project.recipes) {
        require(recipe.schemaVersion==1 || recipe.schemaVersion==2,"Desteklenmeyen reçete şema sürümü.");
        if(recipe.calibrationBinding) {
            recipe.calibrationBinding->validate();
            require(recipe.schemaVersion==2 && recipe.calibrationReference==recipe.calibrationBinding->calibrationId,"Kalibrasyon sürüm bağlantısı uyumsuz.");
        }
        const auto canonicalId = QUuid(recipe.id).toString(QUuid::WithoutBraces);
        require(!QUuid(recipe.id).isNull() && !ids.contains(canonicalId),"Reçete ID geçersiz veya tekrar ediyor.");
        require(validName(recipe.name),"Reçete adı 1–64 yazdırılabilir karakter olmalı.");
        require(!names.contains(recipe.name.toCaseFolded()),"Reçete adı bu projede zaten var.");
        require(recipe.recipeVersion>=1 && recipe.recipeVersion<1000000000,"Reçete sürümü geçersiz.");
        require(!recipe.cameraReference.trimmed().isEmpty(),"Reçete görüntü kaynağı gerekli.");
        require(recipe.description.size()<=2048,"Reçete açıklaması en fazla 2048 karakter olabilir.");
        require(recipe.createdAt.isValid() && recipe.modifiedAt.isValid(),"Reçete tarihleri geçersiz.");
        typeKey(recipe.applicationType); statusKey(recipe.status);
        validateEditorDefinition(recipe.definition);
        if(recipe.definition.contains("planarSetup")) {
            require(recipe.definition["planarSetup"].isObject(),"Planar kurulum nesne olmalı.");const auto setup=recipe.definition["planarSetup"].toObject();
            require(setup.size()==2 && setup["lensId"].isString() && setup["workingPlaneId"].isString(),"Planar kurulum alanları hatalı.");
            for(const auto key:{"lensId","workingPlaneId"}) {const auto text=setup[key].toString();require(!text.trimmed().isEmpty() && text==text.trimmed() && text.size()<=256,"Lens/düzlem kimliği 1–256 karakter olmalı.");}
        }
        ids.insert(canonicalId); names.insert(recipe.name.toCaseFolded());
    }
}
}
