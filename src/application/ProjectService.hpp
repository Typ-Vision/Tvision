#pragma once
#include "persistence/ProjectRepository.hpp"
#include "domain/ToolDefinition.hpp"
#include "domain/ToolGraph.hpp"
#include <QSet>
#include <optional>
#include "domain/ReferenceConfig.hpp"
#include "domain/IndustrialSourceReference.hpp"


namespace visionflow {
struct MeasurementEnvironment;
class ProjectService {
public:
    bool hasProject() const { return project_.has_value(); }
    bool dirty() const { return dirty_; }
    const Project& project() const;
    const QString& path() const { return path_; }
    const Recipe* activeRecipe() const;
    bool recipeDirty(const QString& id) const { return changedRecipes_.contains(id); }
    void createProject(QString name, QString description, const QString& path, QString cameraReference,
                       QString robotType = {}, QString protocol = {}, QString calibration = {});
    void openProject(const QString& path);
    void updateProject(QString name, QString description, QString cameraReference,
                       QString robotType, QString protocol, QString calibration);
    QString createRecipe(QString name, QString description, ApplicationType type, QString camera = {}, QString calibration = {});
    void editRecipeMetadata(const QString& id, QString name, QString description);
    // Explicit legacy reference edit; normal UI edits use editRecipeMetadata.
    void editRecipe(const QString& id, QString name, QString description, QString camera, QString calibration);
    void selectRecipe(const QString& id);
    void save();
    void discardChanges();
    QVector<ToolDefinition> tools() const;
    void enableMultiSource();
    QString addTool(const QString& kind);
    void bindIndustrialSource(const QString& toolId,const MeasurementSetupContract&,const ProfileId&,int timeoutMs);
    void bindCameraSource(const QString& toolId,const MeasurementEnvironment&,const CameraId&,int timeoutMs=5000);
    void refreshCameraSources(const MeasurementEnvironment&);
    void refreshEditorCameraSources(const MeasurementEnvironment&);
    void updateTool(const ToolDefinition& tool);
    // Commit teaching and its reference connection as one validated edit.
    void updateToolConnections(const ToolDefinition&,const QVector<ToolConnection>&);
    void bindReference(const QString& target,const ReferenceBinding&);
    void clearReference(const QString& target);
    void removeTool(const QString& id);
    bool moveTool(const QString& id,int direction);
    QString setImageFile(const QString& absolutePath,const QString& source={});
    QString imageFilePath() const;
    void setConnections(const QVector<ToolConnection>& connections);
    void bindSourceSetup(const QString& sourceToolId,const QJsonObject& target);
    void clearSourceSetup(const QString& source={});
    void bindCalibration(const CalibrationDependency&);
    void clearCalibrationBinding();
    void setRuntimeOptions(QString trigger,QString measurementToolId);
    void setCalibrationSetup(QString lensId,QString workingPlaneId);
    QVector<VariableDefinition> variables() const;
    QString addVariable(VariableDefinition definition);
    void renameVariable(const QString& id,QString name);
    void updateVariable(const VariableDefinition& definition);
    void removeVariable(const QString& id);
private:
    void setVariables(const QVector<VariableDefinition>&);
    void setTools(const QVector<ToolDefinition>& tools,const QVector<ToolConnection>* bindings=nullptr);
    ProjectRepository repository_;
    std::optional<Project> project_;
    std::optional<Project> savedProject_;
    QString path_;
    QString activeId_;
    bool dirty_ = false;
    QSet<QString> changedRecipes_;
    QSet<QString> newRecipes_;
    QByteArray diskSnapshot_;
};
}
