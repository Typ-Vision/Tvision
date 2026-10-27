#pragma once
#include "application/ProjectService.hpp"
#include "application/CommunicationProfiles.hpp"
#include <QWidget>
class QComboBox; class QCheckBox; class QTableWidget; class QLabel;
namespace visionflow {
class PublishDataEditor final:public QWidget {
public:
    PublishDataEditor(const ToolDefinition&,const QJsonObject&,QString profilePath);
    QJsonObject parameters() const;
    QVector<OutputGroup> groups() const;
    void error(const QString&);
    void validateDraft(const ToolDefinition&) const;
private:
    ToolDefinition tool_;QJsonObject definition_;CommunicationProfiles profiles_;
    QVector<OutputGroup> groups_;QStringList selected_;QString current_;
    QComboBox *group_,*producer_,*endpoint_;QCheckBox* required_;QTableWidget* fields_;QLabel *status_,*fieldInfo_;
    void load();void store();void refresh(const QString&);void addField(const PublicationField& = {});void preview();
};
}
