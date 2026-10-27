#pragma once
#include "ExecutionService.hpp"
namespace visionflow {
// Frozen per worker cycle. Missing values have no payload; never seeded from Editor/cache.
struct RuntimeVariableContext {
    QString cycleId, configId;
    QByteArray dependencyId;
    QVector<VariableDefinition> definitions;
    QMap<QString,AssignmentValue> values;
};
QByteArray runtimeVariableDependencyId(const QJsonObject& definition);
std::shared_ptr<const RuntimeVariableContext> runtimeVariables(const ProductionSnapshot&,const CycleResult&);
}
