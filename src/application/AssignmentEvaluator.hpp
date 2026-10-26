#pragma once
#include "domain/AssignmentConfig.hpp"

namespace visionflow {
struct AssignmentToolOutput { int revision=0; AssignmentValue value; };
struct AssignmentInputView {
    QString cycleId,configId;
    QVector<VariableDefinition> variables;
    QSet<QString> systemTargetIds;
    QMap<QString,QMap<QString,AssignmentToolOutput>> toolOutputs;
    QMap<QString,AssignmentBundle> bundles;
};
enum class AssignmentReason {
    None, InvalidConfig, DuplicateTarget, ReadOnlyTarget, UnknownTarget,
    MissingSource, StaleSource, CycleMismatch, ConfigMismatch, RevisionMismatch,
    InvalidUpstream, TypeMismatch, UnitMismatch, FrameMismatch, ProducerMismatch
};
QString assignmentReasonKey(AssignmentReason);
struct AssignmentEvaluation {
    AssignmentReason reason=AssignmentReason::None;
    int row=-1;
    QString detail;
    std::optional<AssignmentBundle> bundle;
};
class AssignmentEvaluator {
public:
    // Pure: all RHS read the same const input. Failure returns no partial bundle.
    static AssignmentEvaluation evaluate(const AssignmentConfig&,const AssignmentInputView&);
};
}
