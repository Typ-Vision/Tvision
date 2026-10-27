#pragma once
#include "domain/ToolDefinition.hpp"
#include <QWidget>
class QComboBox;
class QListWidget;
class QLabel;
namespace visionflow {
class ResultEditor : public QWidget {
public:
    ResultEditor(const ToolDefinition&,const QJsonObject&);
    QJsonObject parameters() const;
    void validateDraft() const;
    void error(const QString&);
private:
    QComboBox* operation_;
    QListWidget* sources_;
    QLabel* error_;
};
}
