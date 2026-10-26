#include "AssignmentEvaluator.hpp"
#include <stdexcept>

namespace visionflow {
namespace {
void validateValue(const AssignmentValue& value,const EnumDefinitions& enums) {
    std::visit([&](const auto& e) {
        using T=std::decay_t<decltype(e)>;
        if constexpr(std::is_same_v<T,ValueEnvelope>)CommunicationValues::validate(e,enums);
        else {
            if(e.unit!=ScalarUnit::Millimetre || !e.value)throw std::runtime_error("Spatial value requires mm and a transform");
            e.value->validate();
            ValueEnvelope metadata;
            metadata.value=true;metadata.validity=e.validity;metadata.reason=e.reason;
            metadata.producerId=e.producerId;metadata.producerPort=e.producerPort;
            metadata.cycleId=e.cycleId;metadata.frameId=e.frameId;metadata.configId=e.configId;
            metadata.dependencyIds=e.dependencyIds;
            CommunicationValues::validate(metadata);
        }
    },value);
}
}
QString assignmentReasonKey(AssignmentReason r) {
    switch(r) {
    case AssignmentReason::None:return "none";
    case AssignmentReason::InvalidConfig:return "invalid_config";
    case AssignmentReason::DuplicateTarget:return "duplicate_target";
    case AssignmentReason::ReadOnlyTarget:return "read_only_target";
    case AssignmentReason::UnknownTarget:return "unknown_target";
    case AssignmentReason::MissingSource:return "missing_source";
    case AssignmentReason::StaleSource:return "stale_source";
    case AssignmentReason::CycleMismatch:return "cycle_mismatch";
    case AssignmentReason::ConfigMismatch:return "config_mismatch";
    case AssignmentReason::RevisionMismatch:return "revision_mismatch";
    case AssignmentReason::InvalidUpstream:return "invalid_upstream";
    case AssignmentReason::TypeMismatch:return "type_mismatch";
    case AssignmentReason::UnitMismatch:return "unit_mismatch";
    case AssignmentReason::FrameMismatch:return "frame_mismatch";
    case AssignmentReason::ProducerMismatch:return "producer_mismatch";
    }
    return "invalid_config";
}
AssignmentEvaluation AssignmentEvaluator::evaluate(const AssignmentConfig& config,const AssignmentInputView& input) {
    int row=-1;
    auto fail=[&](AssignmentReason reason,const QString& detail=QString{}) {
        return AssignmentEvaluation{reason,row,detail.isEmpty()?assignmentReasonKey(reason):detail,std::nullopt};
    };
    if(config.producerId.trimmed().isEmpty() || config.revision<=0 || config.rows.isEmpty() ||
       config.rows.size()>AssignmentConfig::maxRows || input.cycleId.isEmpty() || input.configId.isEmpty())
        return fail(AssignmentReason::InvalidConfig);
    QMap<QString,VariableDefinition> targets;
    EnumDefinitions enums;
    try {
        // Reuse the catalog's UUID/name/enum and capacity validation.
        withRecipeVariables({},input.variables);
        for(const auto& v:input.variables) {
            targets.insert(v.id,v);
            if(v.type==VariableType::Enum)enums.insert(v.enumDefinitionId,v.enumCodes);
        }
    } catch(const std::exception& e) { return fail(AssignmentReason::InvalidConfig,QString::fromUtf8(e.what())); }
    QSet<QString> seen;
    for(const auto& assignment:config.rows) {
        ++row;
        if(input.systemTargetIds.contains(assignment.targetId))return fail(AssignmentReason::ReadOnlyTarget);
        if(!targets.contains(assignment.targetId))return fail(AssignmentReason::UnknownTarget);
        if(seen.contains(assignment.targetId))return fail(AssignmentReason::DuplicateTarget);
        seen.insert(assignment.targetId);
    }
    AssignmentBundle candidate{config.producerId,input.cycleId,input.configId,config.revision,false,{}};
    row=-1;
    for(const auto& assignment:config.rows) {
        ++row;
        const auto& target=targets[assignment.targetId];
        AssignmentValue value;
        QString expectedProducer,expectedPort;
        const bool constant=std::holds_alternative<ConstantAssignmentSource>(assignment.source);
        if(const auto* c=std::get_if<ConstantAssignmentSource>(&assignment.source))value=c->value;
        else if(const auto* s=std::get_if<ToolOutputAssignmentSource>(&assignment.source)) {
            if(s->toolId.isEmpty() || s->port.isEmpty() || s->revision<=0)return fail(AssignmentReason::InvalidConfig);
            const auto producer=input.toolOutputs.constFind(s->toolId);
            if(producer==input.toolOutputs.cend())return fail(AssignmentReason::MissingSource);
            const auto output=producer->constFind(s->port);
            if(output==producer->cend())return fail(AssignmentReason::MissingSource);
            if(output->revision!=s->revision)return fail(AssignmentReason::RevisionMismatch);
            value=output->value;expectedProducer=s->toolId;expectedPort=s->port;
        } else {
            const auto& binding=std::get<BundleAssignmentSource>(assignment.source);
            if(binding.producerId.isEmpty() || binding.fieldId.isEmpty() || binding.revision<=0)return fail(AssignmentReason::InvalidConfig);
            const auto bundle=input.bundles.constFind(binding.producerId);
            if(bundle==input.bundles.cend())return fail(AssignmentReason::MissingSource);
            if(!bundle->committed)return fail(AssignmentReason::InvalidUpstream);
            if(bundle->producerId!=binding.producerId)return fail(AssignmentReason::ProducerMismatch);
            if(bundle->cycleId!=input.cycleId)return fail(AssignmentReason::CycleMismatch);
            if(bundle->configId!=input.configId)return fail(AssignmentReason::ConfigMismatch);
            if(bundle->revision!=binding.revision)return fail(AssignmentReason::RevisionMismatch);
            const auto field=bundle->fields.constFind(binding.fieldId);
            if(field==bundle->fields.cend())return fail(AssignmentReason::MissingSource);
            value=*field;expectedProducer=binding.producerId;expectedPort=binding.fieldId;
        }
        AssignmentReason reason=AssignmentReason::None;
        try { std::visit([&](const auto& e) {
            if(e.validity==ValueValidity::Missing)reason=AssignmentReason::MissingSource;
            else if(e.validity==ValueValidity::Stale)reason=AssignmentReason::StaleSource;
            else if(e.validity!=ValueValidity::Valid || !e.value)reason=AssignmentReason::InvalidUpstream;
            else if(constant && (!e.cycleId.isEmpty() || !e.configId.isEmpty() || !e.producerId.isEmpty() || !e.producerPort.isEmpty() || !e.frameId.isEmpty() || !e.dependencyIds.isEmpty()))reason=AssignmentReason::InvalidConfig;
            else if(!constant && e.cycleId!=input.cycleId)reason=AssignmentReason::CycleMismatch;
            else if(!constant && e.configId!=input.configId)reason=AssignmentReason::ConfigMismatch;
            else if(!constant && (e.producerId!=expectedProducer || e.producerPort!=expectedPort))reason=AssignmentReason::ProducerMismatch;
            else if(e.unit!=target.unit)reason=AssignmentReason::UnitMismatch;
            else {
                using T=std::decay_t<decltype(e)>;
                if constexpr(std::is_same_v<T,ValueEnvelope>) {
                    if(target.type==VariableType::SpatialTransform || int(e.type)!=int(target.type))reason=AssignmentReason::TypeMismatch;
                    else if(e.type==ScalarType::Enum && (!std::holds_alternative<EnumValue>(*e.value) || std::get<EnumValue>(*e.value).definitionId!=target.enumDefinitionId))reason=AssignmentReason::TypeMismatch;
                    else CommunicationValues::validate(e,enums);
                } else {
                    if(target.type!=VariableType::SpatialTransform)reason=AssignmentReason::TypeMismatch;
                    else if(!e.reason.isEmpty())reason=AssignmentReason::InvalidUpstream;
                    else {
                        e.value->validate();
                        if(target.frameConstraint && (e.value->source!=target.frameConstraint->source || e.value->target!=target.frameConstraint->target))reason=AssignmentReason::FrameMismatch;
                    }
                }
            }
        },value); } catch(const std::exception& e) {
            return fail(AssignmentReason::InvalidUpstream,QString::fromUtf8(e.what()));
        }
        if(reason!=AssignmentReason::None)return fail(reason);
        try { validateValue(value,enums); }
        catch(const std::exception& e) {return fail(AssignmentReason::InvalidUpstream,QString::fromUtf8(e.what()));}
        // Rebind the result to this bundle; original dependencies remain explicit.
        std::visit([&](auto& e) {
            if(!constant) {
                const auto dependency=expectedProducer+":"+expectedPort;
                if(!e.dependencyIds.contains(dependency))e.dependencyIds.append(dependency);
            }
            e.producerId=config.producerId;e.producerPort=assignment.targetId;
            e.cycleId=input.cycleId;e.configId=input.configId;
        },value);
        try { validateValue(value,enums); }
        catch(const std::exception& e) {return fail(AssignmentReason::InvalidUpstream,QString::fromUtf8(e.what()));}
        candidate.fields.insert(assignment.targetId,std::move(value));
    }
    candidate.committed=true;
    return {AssignmentReason::None,-1,{},std::move(candidate)};
}
}
