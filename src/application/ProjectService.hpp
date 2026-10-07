#pragma once
#include "persistence/ProjectRepository.hpp"
#include "domain/ToolDefinition.hpp"
#include "domain/ToolGraph.hpp"
#include <QSet>
#include <optional>

namespace visionflow {
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
    QString createRecipe(QString name, QString description, ApplicationType type, QString camera, QString calibration = {});
    void editRecipe(const QString& id, QString name, QString description, QString camera, QString calibration);
    void selectRecipe(const QString& id);
    void save();
    void discardChanges();
    QVector<ToolDefinition> tools() const;
    QString addTool(const QString& kind);
    void updateTool(const ToolDefinition& tool);
    void removeTool(const QString& id);
    QString setImageFile(const QString& absolutePath);
    QString imageFilePath() const;
    void setConnections(const QVector<ToolConnection>& connections);
private:
    void setTools(const QVector<ToolDefinition>& tools);
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
