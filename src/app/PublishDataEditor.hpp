#pragma once
#include "application/ProjectService.hpp"
#include "application/ExecutionService.hpp"
#include "application/PublicationAssignmentCatalog.hpp"
#include "domain/PublicationTemplate.hpp"
#include <QWidget>
#include <optional>
class QComboBox; class QTableWidget; class QLabel;
namespace visionflow {
// V4 recipe bindings; machine templates and legacy publication data stay unchanged.
class PublishDataEditor final:public QWidget {
public:
    PublishDataEditor(const ToolDefinition&,const QJsonObject&,QString profilePath,
                      QString templateDirectory={},const CycleResult* cycle=nullptr);
    QJsonObject parameters() const;
    std::optional<PublicationAssignments> assignments() const;
    bool assignmentsEdited() const{return assignmentsEdited_;}
    void validateAssignments() const;
    void error(const QString&);
    void validateDraft(const ToolDefinition&) const;
private:
    ToolDefinition tool_;QJsonObject definition_;
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
