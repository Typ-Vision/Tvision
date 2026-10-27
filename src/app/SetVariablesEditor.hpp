#pragma once
#include "domain/AssignmentConfig.hpp"
#include "application/ExecutionService.hpp"
#include <QWidget>
#include <QJsonArray>
class QListWidget;
class QComboBox;
class QLineEdit;
class QLabel;
namespace visionflow {
class SetVariablesEditor : public QWidget {
public:
    SetVariablesEditor(const ToolDefinition&, const QJsonObject&, const CycleResult*);
    QJsonObject parameters() const;
    void validateDraft() const;
    static void validateConfiguration(const ToolDefinition&,const QVector<VariableDefinition>&,const QVector<ToolDefinition>&);
    void error(const QString&);
private:
    void selectRow();
    void sources(const QJsonObject& selected={});
    void storeRow();
    void refreshRows(int selected);
    QString description(const VariableDefinition&) const;
    ToolDefinition tool_;
    QVector<ToolDefinition> tools_;
    QVector<VariableDefinition> variables_;
    QJsonArray rows_;
    QListWidget* list_;
    QComboBox *target_,*source_;
    QLineEdit* constant_;
    QLabel *error_,*validity_;
    bool loading_=false;
};
}
