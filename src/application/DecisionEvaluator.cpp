#include "DecisionEvaluator.hpp"
#include <stdexcept>

namespace visionflow {
QString decisionReasonKey(DecisionReason r) {
    switch(r) {
    case DecisionReason::None:return "none";
    case DecisionReason::InvalidConfig:return "invalid_config";
    case DecisionReason::ExecutionUnavailable:return "execution_unavailable";
    case DecisionReason::MissingSource:return "missing_source";
    case DecisionReason::StaleSource:return "stale_source";
    case DecisionReason::SkippedSource:return "skipped_source";
    case DecisionReason::InvalidSource:return "invalid_source";
    case DecisionReason::ErrorSource:return "error_source";
    case DecisionReason::CycleMismatch:return "cycle_mismatch";
    case DecisionReason::ConfigMismatch:return "config_mismatch";
    case DecisionReason::RevisionMismatch:return "revision_mismatch";
    case DecisionReason::ProducerMismatch:return "producer_mismatch";
    case DecisionReason::TypeMismatch:return "type_mismatch";
    }
    return "invalid_config";
}
DecisionEvaluation DecisionEvaluator::evaluate(const ResultConfig& c,const DecisionInputView& input) {
    int row=-1;
    auto fail=[&](DecisionReason reason){return DecisionEvaluation{ResultDecision::NotEvaluated,reason,row};};
    try {validateResultConfig(c);}catch(const std::exception&){return fail(DecisionReason::InvalidConfig);}
    if(input.cycleId.isEmpty() || input.configId.isEmpty())return fail(DecisionReason::InvalidConfig);
    if(input.execution!=ResultExecutionState::Done)return fail(DecisionReason::ExecutionUnavailable);
    bool all=true,any=false;
    for(const auto& binding:c.required) {
        ++row;
        const auto producer=input.toolOutputs.constFind(binding.toolId);
        if(producer==input.toolOutputs.cend())return fail(DecisionReason::MissingSource);
        const auto output=producer->constFind(binding.port);
        if(output==producer->cend())return fail(DecisionReason::MissingSource);
        if(output->revision!=binding.revision)return fail(DecisionReason::RevisionMismatch);
        const auto& e=output->value;
        switch(e.validity) {
        case ValueValidity::Missing:return fail(DecisionReason::MissingSource);
        case ValueValidity::Stale:return fail(DecisionReason::StaleSource);
        case ValueValidity::Skipped:return fail(DecisionReason::SkippedSource);
        case ValueValidity::Error:return fail(DecisionReason::ErrorSource);
        case ValueValidity::Invalid:return fail(DecisionReason::InvalidSource);
        case ValueValidity::Valid:break;
        default:return fail(DecisionReason::InvalidSource);
        }
        if(e.cycleId!=input.cycleId)return fail(DecisionReason::CycleMismatch);
        if(e.configId!=input.configId)return fail(DecisionReason::ConfigMismatch);
        if(e.producerId!=binding.toolId || e.producerPort!=binding.port)return fail(DecisionReason::ProducerMismatch);
        if(e.type!=ScalarType::Boolean || e.unit!=ScalarUnit::Unitless)return fail(DecisionReason::TypeMismatch);
        try {CommunicationValues::validate(e);}catch(const std::exception&){return fail(DecisionReason::InvalidSource);}
        const bool value=std::get<bool>(*e.value);
        all=all && value;any=any || value;
        // Never return a Boolean decision here: later missing data wins.
    }
    const bool accepted=c.operation==ResultOperator::All?all:any;
    return {accepted?ResultDecision::Ok:ResultDecision::Nok,DecisionReason::None,-1};
}
ExportDecision DecisionEvaluator::forExport(const std::optional<DecisionEvaluation>& result,
    ResultExecutionState execution,bool requiredFieldsValid) {
    if(execution!=ResultExecutionState::Done)return {};
    const auto decision=result && result->reason==DecisionReason::None?result->decision:ResultDecision::NotEvaluated;
    return {decision,requiredFieldsValid};
}
}
