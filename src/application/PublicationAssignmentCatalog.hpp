#pragma once
#include "ExecutionService.hpp"
#include "FixedSlotPublication.hpp"

namespace visionflow {
// Editor-side candidate list. The definition is authoritative for type/unit;
// options that would need an unpublished converter are listed with a reason so
// the operator sees the exact rejection instead of a silent empty list.
struct PublicationSourceOption {
  PublicationSource source;
  PublicationValueType type = PublicationValueType::Int32;
  ScalarUnit unit = ScalarUnit::Unitless;
  PublicationPoseContract pose;
  QString label, detail, scope, blockReason;
};
struct PublicationSlotRow {
  PublicationSlotKind kind = PublicationSlotKind::Number;
  int index = 0;
  QString name, contract, source, status;
  // Short badge for the narrow properties table; the full reason stays in
  // status and is shown on selection/apply.
  QString badge;
  bool required = false;
  bool assigned = false;
  bool valid = false;
  // False when no cycle value was available; a metadata match is never shown
  // as a value success.
  bool valueChecked = false;
};
struct PublicationPreview {
  QVector<PublicationSlotRow> rows;
  QStringList errors;
};
QString publicationSlotKindName(PublicationSlotKind);
QString publicationSlotDisplayName(PublicationSlotKind, int,
                                   const PublicationSlot &);
QString publicationSlotContractText(PublicationSlotKind, const PublicationSlot &);
// One-line form for the narrow properties panel; the full contract stays in the
// tooltip and in the slot detail label.
QString publicationSlotContractSummary(PublicationSlotKind, const PublicationSlot &);
QVector<PublicationSourceOption>
publicationSourceOptions(const QJsonObject &definition);
// Same immutable evaluator the Runtime uses. Values are optional: without a
// committed cycle the rows carry contract checks only.
PublicationPreview previewPublication(
    const PublicationTemplate &, const PublicationAssignments &,
    const QJsonObject &definition, const CycleResult *cycle = nullptr,
    PublicationDecision decision = PublicationDecision::NotEvaluated);
} // namespace visionflow
