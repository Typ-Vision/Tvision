#include "Project.hpp"
#include "ToolDefinition.hpp"
#include <QSet>
#include <QJsonArray>
#include <algorithm>
#include <limits>
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
const QStringList variableTypes{"Boolean","Int32","UInt32","Int64","Float64","String","Enum","SpatialTransform"};
const QStringList variableUnits{"unitless","mm","m","rad","degree"};
void exactKeys(const QJsonObject& o, const QSet<QString>& allowed) {
    require(o.size()==allowed.size(),"Variable fields missing/extra.");
    for(auto i=o.begin();i!=o.end();++i)require(allowed.contains(i.key()),"Unknown variable field: "+i.key());
}
QString variableText(const QJsonObject& o,const char* key) {
    require(o[key].isString(),"Variable string required.");return o[key].toString();
}
void unicodeText(const QString& text,int bytes) {
    require(text.toUtf8().size()<=bytes,"Variable text byte limit exceeded.");
    for(qsizetype i=0;i<text.size();++i) {
        if(text[i].isHighSurrogate()){require(i+1<text.size() && text[i+1].isLowSurrogate(),"Invalid Unicode.");++i;}
        else require(!text[i].isLowSurrogate(),"Invalid Unicode.");
    }
}
}
void VariableDefinition::validate() const {
    require(!QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces)==id,"Canonical variable UUID required.");
    require(validName(name),"Variable name must be 1–64 printable trimmed characters.");
    unicodeText(name,256);unicodeText(comment,4096);
    require(int(type)>=0 && int(type)<variableTypes.size(),"Illegal variable type.");
    CommunicationValues::dimension(unit);
    const bool numeric=type>=VariableType::Int32 && type<=VariableType::Float64;
    if(type==VariableType::SpatialTransform) {
        require(unit==ScalarUnit::Millimetre,"Spatial variable requires mm.");
        if(frameConstraint){frameConstraint->source.validate();frameConstraint->target.validate();}
    } else {
        require(!frameConstraint,"Frame constraint requires SpatialTransform.");
        require(numeric || unit==ScalarUnit::Unitless,"Non-numeric variable must be unitless.");
    }
    if(type==VariableType::Enum) {
        require(!enumDefinitionId.isEmpty() && enumDefinitionId==enumDefinitionId.trimmed(),"Enum definition ID required.");
        unicodeText(enumDefinitionId,256);
        require(!enumCodes.isEmpty() && enumCodes.size()<=256,"Enum needs 1–256 allowed codes.");
    } else require(enumDefinitionId.isEmpty() && enumCodes.isEmpty(),"Enum metadata requires Enum type.");
}
QJsonObject VariableDefinition::toJson() const {
    validate();QJsonArray codes;auto sorted=enumCodes.values();std::sort(sorted.begin(),sorted.end());for(auto c:sorted)codes.append(c);
    return {{"id",id},{"name",name},{"type",variableTypes[int(type)]},{"unit",variableUnits[int(unit)]},
        {"comment",comment},{"enumDefinitionId",enumDefinitionId},{"enumCodes",codes},
        {"frameConstraint",frameConstraint ? QJsonValue(QJsonObject{{"source",frameConstraint->source.toJson()},{"target",frameConstraint->target.toJson()}}) : QJsonValue(QJsonValue::Null)}};
}
VariableDefinition VariableDefinition::fromJson(const QJsonObject& o) {
    exactKeys(o,{"id","name","type","unit","comment","enumDefinitionId","enumCodes","frameConstraint"});
    VariableDefinition v;v.id=variableText(o,"id");v.name=variableText(o,"name");v.comment=variableText(o,"comment");
    const auto type=variableTypes.indexOf(variableText(o,"type")),unit=variableUnits.indexOf(variableText(o,"unit"));
    require(type>=0 && unit>=0,"Unknown variable type/unit.");v.type=VariableType(type);v.unit=ScalarUnit(unit);
    v.enumDefinitionId=variableText(o,"enumDefinitionId");
    require(o["enumCodes"].isArray() && o["enumCodes"].toArray().size()<=256,"Enum code array required.");
    for(auto c:o["enumCodes"].toArray()) {
        const double n=c.toDouble();require(c.isDouble() && n>=std::numeric_limits<qint32>::min() && n<=std::numeric_limits<qint32>::max() && n==qint64(n),"Enum code must be Int32.");
        require(!v.enumCodes.contains(qint32(n)),"Duplicate enum code.");v.enumCodes.insert(qint32(n));
    }
    if(!o["frameConstraint"].isNull()) {
        require(o["frameConstraint"].isObject(),"Frame constraint object required.");auto f=o["frameConstraint"].toObject();exactKeys(f,{"source","target"});
        require(f["source"].isObject() && f["target"].isObject(),"Frame objects required.");
        v.frameConstraint=VariableFrameConstraint{FrameRef::fromJson(f["source"].toObject()),FrameRef::fromJson(f["target"].toObject())};
    }
    v.validate();return v;
}
QVector<VariableDefinition> recipeVariables(const QJsonObject& definition) {
    if(!definition.contains("variableCatalog"))return {};
    require(definition["variableCatalog"].isObject(),"Variable catalog object required.");const auto o=definition["variableCatalog"].toObject();
    exactKeys(o,{"schemaVersion","variables"});require(o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1,"Unsupported variable catalog schema.");
    require(o["variables"].isArray() && o["variables"].toArray().size()<=maxRecipeVariables,"Variable catalog limit is 256.");
    QVector<VariableDefinition> variables;QSet<QString> ids,names;QMap<QString,QSet<qint32>> enums;
    for(auto value:o["variables"].toArray()) {
        require(value.isObject(),"Variable object required.");auto v=VariableDefinition::fromJson(value.toObject());
        require(!ids.contains(v.id) && !names.contains(v.name.toCaseFolded()),"Duplicate variable UUID/name.");
        if(v.type==VariableType::Enum){require(!enums.contains(v.enumDefinitionId) || enums[v.enumDefinitionId]==v.enumCodes,"Conflicting enum definition.");enums[v.enumDefinitionId]=v.enumCodes;}
        ids.insert(v.id);names.insert(v.name.toCaseFolded());variables.append(v);
    }
    return variables;
}
QJsonObject withRecipeVariables(const QJsonObject& definition,const QVector<VariableDefinition>& variables) {
    require(variables.size()<=maxRecipeVariables,"Variable catalog limit is 256.");QJsonArray array;for(const auto& v:variables)array.append(v.toJson());
    auto result=definition;result["variableCatalog"]=QJsonObject{{"schemaVersion",1},{"variables",array}};recipeVariables(result);return result;
}
bool variableReferenced(const QJsonObject& definition,const QString& id) {
    // Conservative UUID reference guard also covers future definition extensions.
    auto uses=[&](auto&& self,const QJsonValue& value)->bool {
        if(value.isString())return QUuid(value.toString())==QUuid(id);
        if(value.isArray())for(auto item:value.toArray())if(self(self,item))return true;
        if(value.isObject()){auto object=value.toObject();for(auto i=object.begin();i!=object.end();++i)if(QUuid(i.key())==QUuid(id) || self(self,i.value()))return true;}
        return false;
    };
    auto config=definition;config.remove("variableCatalog");return uses(uses,config);
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
        require(recipe.description.size()<=2048,"Reçete açıklaması en fazla 2048 karakter olabilir.");
        require(recipe.createdAt.isValid() && recipe.modifiedAt.isValid(),"Reçete tarihleri geçersiz.");
        typeKey(recipe.applicationType); statusKey(recipe.status);
        validateEditorDefinition(recipe.definition);
        recipeVariables(recipe.definition);
        if(recipe.definition.contains("planarSetup")) {
            require(recipe.definition["planarSetup"].isObject(),"Planar kurulum nesne olmalı.");const auto setup=recipe.definition["planarSetup"].toObject();
            require(setup.size()==2 && setup["lensId"].isString() && setup["workingPlaneId"].isString(),"Planar kurulum alanları hatalı.");
            for(const auto key:{"lensId","workingPlaneId"}) {const auto text=setup[key].toString();require(!text.trimmed().isEmpty() && text==text.trimmed() && text.size()<=256,"Lens/düzlem kimliği 1–256 karakter olmalı.");}
        }
        ids.insert(canonicalId); names.insert(recipe.name.toCaseFolded());
    }
}
}
