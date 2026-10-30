#pragma once
#include "CommunicationValues.hpp"
#include <QUuid>
#include <QVector>
#include <array>
#include <limits>

namespace visionflow {
enum class PublicationSlotKind { Number, Flag, Text, Pose };
enum class PublicationSourceKind { Variable, ToolOutput };
// Geometric outputs are deliberately distinct; no planar-to-whole-pose
// promotion.
enum class PublicationValueType {
  Boolean,
  Int32,
  UInt32,
  Int64,
  Float64,
  String,
  SpatialTransform,
  Point2D,
  Pose2D
};
struct PublicationPoseContract {
  QUuid targetFrame, targetRevision, sourceFrame, sourceRevision,
      adapterContract;
  bool operator==(const PublicationPoseContract &) const = default;
};
struct PublicationSlot {
  QString name;
  bool required = false;
  ScalarUnit unit = ScalarUnit::Unitless;
  // Wire = roundTiesToEven(source * scaleNumerator/scaleDenominator
  //                         + offsetNumerator/offsetDenominator).
  qint32 scaleNumerator = 1, scaleDenominator = 1, offsetNumerator = 0,
         offsetDenominator = 1;
  qint32 minimum = std::numeric_limits<qint32>::min(),
         maximum = std::numeric_limits<qint32>::max();
  int textMaxBytes = 64;
  PublicationPoseContract pose;
  bool operator==(const PublicationSlot &) const = default;
};
struct PublicationTemplate {
  QUuid id, revision;
  std::array<PublicationSlot, 16> numbers;
  std::array<PublicationSlot, 32> flags;
  std::array<PublicationSlot, 4> texts;
  std::array<PublicationSlot, 1> poses;
  static PublicationTemplate create(const QUuid &id, const QUuid &revision,
                                    const PublicationPoseContract &);
  void validate() const;
  QJsonObject toJson() const;
  static PublicationTemplate fromJson(const QJsonObject &);
};
struct PublicationSource {
  PublicationSourceKind kind = PublicationSourceKind::Variable;
  QString id, port;
  int revision = 0; // tool config revision; variables carry no value/revision
  bool operator==(const PublicationSource &) const = default;
};
struct SlotAssignment {
  PublicationSlotKind kind = PublicationSlotKind::Number;
  int index = 0;
  PublicationSource source;
};
// Caller supplies authoritative cycle-variable / typed-output metadata, never
// metadata copied from the assignment JSON. DAG and value provenance are
// checked by the immutable evaluator (COMM4-12).
struct PublicationSourceDefinition {
  PublicationSource source;
  PublicationValueType type = PublicationValueType::Int32;
  ScalarUnit unit = ScalarUnit::Unitless;
  PublicationPoseContract pose;
};
struct PublicationAssignments {
  QUuid templateId, templateRevision;
  QVector<SlotAssignment> rows;
  void validate(const PublicationTemplate &,
                const QVector<PublicationSourceDefinition> &) const;
  QJsonObject toJson() const;
  static PublicationAssignments fromJson(const QJsonObject &);
};
// Same revision is immutable, even when metadata edits preserve byte offsets.
void validatePublicationTemplateReplacement(const PublicationTemplate &,
                                            const PublicationTemplate &);
// Pure snapshot fence: a revised template cannot be used under an old
// activation.
void validatePublicationActivation(const PublicationTemplate &active,
                                   const PublicationTemplate &requested);
// No truncation/replacement of malformed Unicode. Empty string is valid.
void validatePublicationText(const QString &, int maxBytes = 64);
// Opt-in schema extension; absent legacy definitions remain byte-identical.
std::optional<PublicationAssignments>
recipePublicationAssignments(const QJsonObject &);
QJsonObject withRecipePublicationAssignments(QJsonObject,
                                             const PublicationAssignments &);
} // namespace visionflow
