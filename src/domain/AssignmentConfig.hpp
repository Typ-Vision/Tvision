#pragma once
#include "Project.hpp"

namespace visionflow {
// Spatial counterpart of ValueEnvelope. Translation is always mm.
struct SpatialAssignmentValue {
    std::optional<SpatialTransform> value;
    ValueValidity validity=ValueValidity::Missing;
    QString reason;
    ScalarUnit unit=ScalarUnit::Millimetre;
    QString producerId,producerPort,cycleId,frameId,configId;
    QStringList dependencyIds;
};
using AssignmentValue=std::variant<ValueEnvelope,SpatialAssignmentValue>;
struct ConstantAssignmentSource { AssignmentValue value; };
struct ToolOutputAssignmentSource { QString toolId,port; int revision=0; };
struct BundleAssignmentSource { QString producerId,fieldId; int revision=0; };
using AssignmentSource=std::variant<ConstantAssignmentSource,ToolOutputAssignmentSource,BundleAssignmentSource>;
struct AssignmentRow { QString targetId; AssignmentSource source; };
struct AssignmentConfig {
    static constexpr int maxRows=64;
    QString producerId;
    int revision=0;
    QVector<AssignmentRow> rows;
};
// Returned only after every row passes; no persisted variable values or writes.
struct AssignmentBundle {
    QString producerId,cycleId,configId;
    int revision=0;
    bool committed=false;
    QMap<QString,AssignmentValue> fields;
};
// Shape codec only; catalog enum/type authority is checked by the evaluator.
QJsonObject encodeAssignmentValue(const AssignmentValue&);
AssignmentValue decodeAssignmentValue(const QJsonObject&);
QJsonObject encodeAssignmentConfig(const AssignmentConfig&);
AssignmentConfig decodeAssignmentConfig(const QJsonObject&,const QString& producerId);
QJsonObject encodeAssignmentBundle(const AssignmentBundle&);
AssignmentBundle decodeAssignmentBundle(const QJsonObject&);
}
