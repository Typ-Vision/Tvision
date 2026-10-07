#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include "Coordinates.hpp"
#include <optional>

namespace visionflow {
enum class ApplicationType { RobotGuidance, DefectInspection, Measurement };
enum class RecipeStatus { Draft, Valid, Invalid };
QString typeKey(ApplicationType type);
QString typeLabel(ApplicationType type);
ApplicationType parseType(const QString& key);
QString statusKey(RecipeStatus status);
RecipeStatus parseStatus(const QString& key);

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
