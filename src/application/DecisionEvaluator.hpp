#pragma once
#include "domain/ResultDecision.hpp"

namespace visionflow {
enum class ResultExecutionState { Incomplete, Done, Error, Canceled };
struct DecisionToolOutput { int revision=0; ValueEnvelope value; };
struct DecisionInputView {
    QString cycleId, configId;
    ResultExecutionState execution=ResultExecutionState::Incomplete;
    QMap<QString,QMap<QString,DecisionToolOutput>> toolOutputs;
};
enum class DecisionReason {
    None, InvalidConfig, ExecutionUnavailable, MissingSource, StaleSource,
    SkippedSource, InvalidSource, ErrorSource, CycleMismatch, ConfigMismatch,
    RevisionMismatch, ProducerMismatch, TypeMismatch
};
QString decisionReasonKey(DecisionReason);
struct DecisionEvaluation {
    ResultDecision decision=ResultDecision::NotEvaluated;
    DecisionReason reason=DecisionReason::None;
    int row=-1;
};
struct ExportDecision {
    ResultDecision decision=ResultDecision::NotEvaluated;
    bool resultValid=false;
};
class DecisionEvaluator {
public:
    // Pure, immutable input. Every required field must be valid before ALL/ANY.
    static DecisionEvaluation evaluate(const ResultConfig&,const DecisionInputView&);
    // Publication validity belongs to its required fields, not existence of a rule.
    // Error/cancel always suppress both decision and publication validity.
    static ExportDecision forExport(const std::optional<DecisionEvaluation>&,
        ResultExecutionState,bool requiredFieldsValid);
};
}
