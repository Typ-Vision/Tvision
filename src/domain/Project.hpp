#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include "Coordinates.hpp"
#include "CommunicationValues.hpp"
#include "SpatialTransform.hpp"
#include <optional>

namespace visionflow {
enum class ApplicationType { RobotGuidance, DefectInspection, Measurement };
enum class RecipeStatus { Draft, Valid, Invalid };
QString typeKey(ApplicationType type);
QString typeLabel(ApplicationType type);
ApplicationType parseType(const QString& key);
QString statusKey(RecipeStatus status);
RecipeStatus parseStatus(const QString& key);

enum class VariableType { Boolean, Int32, UInt32, Int64, Float64, String, Enum, SpatialTransform };
struct VariableFrameConstraint {
    FrameRef source, target;
    bool operator==(const VariableFrameConstraint&) const = default;
};
// Recipe-cycle local definition only: no persisted current/default value.
struct VariableDefinition {
    QString id, name;
    VariableType type=VariableType::Boolean;
    ScalarUnit unit=ScalarUnit::Unitless;
    std::optional<VariableFrameConstraint> frameConstraint;
    QString comment;
    QString enumDefinitionId;
    QSet<qint32> enumCodes;
    bool operator==(const VariableDefinition&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static VariableDefinition fromJson(const QJsonObject&);
};
constexpr int maxRecipeVariables=256;
QVector<VariableDefinition> recipeVariables(const QJsonObject& definition);
QJsonObject withRecipeVariables(const QJsonObject&, const QVector<VariableDefinition>&);
bool variableReferenced(const QJsonObject& definition, const QString& id);

struct Recipe {
    QString id;
    QString name;
    QString description;
    ApplicationType applicationType = ApplicationType::RobotGuidance;
    QString cameraReference;
    QString calibrationReference;
    std::optional<CalibrationDependency> calibrationBinding;
    int schemaVersion = 1;
    int recipeVersion = 1;
    RecipeStatus status = RecipeStatus::Draft;
    QDateTime createdAt;
    QDateTime modifiedAt;
    // Reserved configuration, not an executable flow in Step 2.
    QJsonObject definition;
};
struct Project {
    int schemaVersion = 1;
    QString id;
    QString name;
    QString description;
    QString cameraReference;
    QString robotType;
    QString communicationProtocol;
    QString calibrationReference;
    QDateTime createdAt;
    QDateTime modifiedAt;
    QVector<Recipe> recipes;
};
void validateProject(const Project& project);
}
