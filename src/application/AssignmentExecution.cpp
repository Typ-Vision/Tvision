#include "AssignmentExecution.hpp"
#include "AssignmentEvaluator.hpp"
#include "domain/ToolRegistry.hpp"
#include <stdexcept>
namespace visionflow {
ToolResult executeAssignments(const ToolDefinition& t,const QJsonObject& definition,
    const QMap<QString,ToolResult>& completed,const QString& cycleId,const QString& configId) {
    const auto config=decodeAssignmentConfig(t.parameters,t.id);
    AssignmentInputView input;input.cycleId=cycleId;input.configId=configId;input.variables=recipeVariables(definition);
    const auto tools=editorTools(definition);
    for(const auto& row:config.rows) {
        if(auto binding=std::get_if<BundleAssignmentSource>(&row.source)) {
            const auto result=completed.value(binding->producerId);const auto port=result.outputs.value("bundle");
            if(result.state==ExecutionState::Done && port.valid)if(auto b=std::get_if<AssignmentBundle>(&port.data))input.bundles[binding->producerId]=*b;
        } else if(auto binding=std::get_if<ToolOutputAssignmentSource>(&row.source)) {
            const auto result=completed.value(binding->toolId);const auto port=result.outputs.value(binding->port);
            if(result.state!=ExecutionState::Done || !port.valid)continue;
            ValueEnvelope value;value.validity=ValueValidity::Valid;value.producerId=binding->toolId;value.producerPort=binding->port;value.cycleId=port.cycleId;value.frameId=port.frameId;value.configId=configId;
            const auto unit=port.port.unit;
            if(unit.isEmpty() || unit=="ratio" || unit=="count")value.unit=ScalarUnit::Unitless;
            else if(unit=="mm")value.unit=ScalarUnit::Millimetre;
            else if(unit=="m")value.unit=ScalarUnit::Metre;
            else if(unit=="rad")value.unit=ScalarUnit::Radian;
            else if(unit=="degree" || unit=="deg")value.unit=ScalarUnit::Degree;
            else throw std::runtime_error("Assignment output unit requires an explicit converter");
            if(auto p=std::get_if<double>(&port.data)){value.type=ScalarType::Float64;value.value=*p;}
            else if(auto p=std::get_if<bool>(&port.data)){value.type=ScalarType::Boolean;value.value=*p;}
            else if(auto p=std::get_if<QString>(&port.data)){value.type=ScalarType::String;value.value=*p;}
            else throw std::runtime_error("Assignment output type requires an explicit converter");
            for(const auto& producer:tools)if(producer.id==binding->toolId)input.toolOutputs[binding->toolId][binding->port]={ToolRegistry::get(producer.kind).version,value};
        }
    }
    const auto evaluated=AssignmentEvaluator::evaluate(config,input);
    if(!evaluated.bundle)throw std::runtime_error(("Assignment "+assignmentReasonKey(evaluated.reason)+": "+evaluated.detail).toUtf8().constData());
    ToolResult result;result.toolId=t.id;result.state=ExecutionState::Done;
    PortValue port{outputPorts(t.kind).first(),*evaluated.bundle,true,{},cycleId,{}};
    port.method="atomic-assignment";port.methodVersion=1;result.outputs["bundle"]=port;return result;
}
}
