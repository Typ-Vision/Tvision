#pragma once
#include "application/ProjectService.hpp"
#include "application/CommunicationProfiles.hpp"
#include "application/ExecutionService.hpp"
#include "application/PublicationAssignmentCatalog.hpp"
#include "domain/PublicationTemplate.hpp"
#include <QWidget>
#include <optional>
class QComboBox; class QCheckBox; class QTableWidget; class QLabel;
namespace visionflow {
// Bundle (v3 OutputGroup) editor plus the v4 fixed-slot assignment editor.
// The machine template manifest is read-only here; only the recipe-level
// source binding is edited.
class PublishDataEditor final:public QWidget {
public:
    PublishDataEditor(const ToolDefinition&,const QJsonObject&,QString profilePath,
                      QString templateDirectory={},const CycleResult* cycle=nullptr);
    QJsonObject parameters() const;
    QVector<OutputGroup> groups() const;
    std::optional<PublicationAssignments> assignments() const;
    bool assignmentsEdited() const{return assignmentsEdited_;}
    void validateAssignments() const;
    void error(const QString&);
    void validateDraft(const ToolDefinition&) const;
private:
    ToolDefinition tool_;QJsonObject definition_;CommunicationProfiles profiles_;
    QVector<OutputGroup> groups_;QStringList selected_;QString current_;
    QComboBox *group_,*producer_,*endpoint_;QCheckBox* required_;QTableWidget* fields_;QLabel *status_,*fieldInfo_;
    void load();void store();void refresh(const QString&);void addField(const PublicationField& = {});void preview();
    // v4 slot assignment
    void loadTemplates();void startAssignment(int templateIndex);void refreshSlots();
    void slotChanged(int row,int column);void slotSelectionChanged();void slotPreview();
    void clearAssignment();void updateSlotStatus();
    PublicationPreview previewNow() const;
    static QString sharedTemplateLabel(const PublicationTemplate&);
    static PublicationSlot slotFor(const PublicationTemplate&,PublicationSlotKind,int);
    QJsonObject candidateDefinition() const;
    PublicationSlotKind rowKind(int row) const;int rowIndex(int row) const;
    QString templateDirectory_,templateError_;
    QVector<PublicationTemplate> templates_;
    std::optional<PublicationAssignments> assignment_;
    PublicationTemplate selectedTemplate_;
    bool hasTemplate_=false,assignmentsEdited_=false,loadingSlots_=false;
    const CycleResult* cycle_=nullptr;
    QComboBox *templateSelector_=nullptr,*slotFilter_=nullptr;
    QTableWidget* slotTable_=nullptr;
    QLabel *templateInfo_=nullptr,*slotInfo_=nullptr,*slotStatus_=nullptr;
};
}
