#pragma once
#include "AssignmentEvaluator.hpp"
#include "domain/PublicationTemplate.hpp"
#include <memory>

namespace visionflow {
// Authoritative activation metadata, never supplied by assignment JSON or values.
// Variable bindings resolve to committed Set Variables bundle fields. Direct
// bindings resolve to typed tool ports. Dependencies form the publication DAG.
struct PublicationBinding {
  PublicationSourceDefinition definition;
  QString producerId, port;
  int revision = 0;
  QVector<PublicationSource> dependencies;
  std::optional<FrameRef> targetFrame, sourceFrame;
};
enum class PublicationExecution { Unknown, Done, Error, Canceled };
enum class PublicationDecision { NotEvaluated, Ok, Nok };
struct PublicationCycleView {
  AssignmentInputView inputs;
  // Exact source-specific acquired frame per producer; empty for non-image data.
  QMap<QString, QString> producerFrames;
  PublicationExecution execution = PublicationExecution::Unknown;
  PublicationDecision decision = PublicationDecision::NotEvaluated;
};
struct FixedSlotResult {
  std::array<qint32, 16> numbers{};
  std::array<QByteArray, 4> texts{};
  std::optional<SpatialTransform> pose;
  PublicationPoseContract poseContract;
  quint32 numberValid = 0, flagValid = 0, flagValue = 0, textValid = 0;
  bool resultValid = false;
  PublicationExecution execution = PublicationExecution::Unknown;
  PublicationDecision decision = PublicationDecision::NotEvaluated;
  QString reason;
  QMap<QString, QString> diagnostics;
};
class FixedSlotPublication {
public:
  // Copies and validates all metadata. No mutable snapshot alias escapes.
  static std::shared_ptr<const FixedSlotPublication>
  capture(PublicationTemplate, PublicationAssignments,
          QVector<PublicationBinding>, QString configId);
  FixedSlotResult evaluate(const PublicationCycleView &) const;
private:
  PublicationTemplate template_;
  PublicationAssignments assignments_;
  QVector<PublicationBinding> bindings_;
  QString configId_;
};
} // namespace visionflow
