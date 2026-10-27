#include "RuntimeVariables.hpp"
#include "ProductionRecipeService.hpp"
#include "AssignmentExecution.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
namespace visionflow {
QByteArray runtimeVariableDependencyId(const QJsonObject& definition) {
    QJsonArray assignments;
    for(const auto& t:editorTools(definition))if(t.kind=="set_variables")
        assignments.append(QJsonObject{{"id",t.id},{"enabled",t.enabled},{"parameters",t.parameters}});
    return QCryptographicHash::hash(QJsonDocument(QJsonObject{{"catalog",withRecipeVariables({},recipeVariables(definition))},
        {"assignments",assignments},{"editor",definition["editor"]},{"engine",ExecutionService::version()}}).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();
}
std::shared_ptr<const RuntimeVariableContext> runtimeVariables(const ProductionSnapshot& snapshot,const CycleResult& result) {
    auto context=std::make_shared<RuntimeVariableContext>();context->cycleId=result.cycleId;context->configId=QString::fromLatin1(result.configId.toHex());
    context->dependencyId=snapshot.variableDependencyId();context->definitions=recipeVariables(snapshot.recipe().definition);
    for(const auto& d:context->definitions){
        if(d.type==VariableType::SpatialTransform){SpatialAssignmentValue v;v.reason="Not assigned in this cycle";v.unit=d.unit;v.cycleId=result.cycleId;v.configId=context->configId;context->values[d.id]=v;}
        else {ValueEnvelope v;v.type=ScalarType(int(d.type));v.reason="Not assigned in this cycle";v.unit=d.unit;v.cycleId=result.cycleId;v.configId=context->configId;context->values[d.id]=v;}
    }
    // Whole-cycle error/cancel exposes only unset values, even if an early producer ran.
    if(result.state!=ExecutionState::Done || result.partial)return context;
    const auto definition=snapshot.recipe().definition;const auto tools=editorTools(definition);
    const auto graph=validateGraph(tools,executionConnections(definition));
    if(!graph.valid())throw std::runtime_error("Invalid Runtime variable graph");
    QMap<QString,ToolResult> completed;for(const auto& t:result.tools)completed[t.toolId]=t;
    for(const auto& id:graph.order)for(const auto& t:tools)if(t.id==id && t.enabled && t.kind=="set_variables"){
        const auto actual=completed.value(id);if(actual.state!=ExecutionState::Done)throw std::runtime_error("Missing Runtime assignment completion");
        const auto expected=executeAssignments(t,definition,completed,result.cycleId,context->configId);
        const auto port=actual.outputs.value("bundle");
        if(!port.valid || !std::holds_alternative<AssignmentBundle>(port.data) || actual.outputs.size()!=1 ||
            encodeAssignmentBundle(std::get<AssignmentBundle>(port.data))!=encodeAssignmentBundle(std::get<AssignmentBundle>(expected.outputs["bundle"].data)))
            throw std::runtime_error("Runtime assignment differs from immutable inputs");
        const auto bundle=std::get<AssignmentBundle>(port.data);
        for(auto i=bundle.fields.cbegin();i!=bundle.fields.cend();++i)context->values[i.key()]=i.value();
    }
    return context;
}
}
