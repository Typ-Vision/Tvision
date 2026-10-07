#pragma once
#include "domain/ToolGraph.hpp"
#include "ImageFileService.hpp"
#include <QMap>
#include <QPointF>
#include <QRect>
#include <QLineF>
#include <atomic>
#include <memory>
#include <optional>
#include <variant>
namespace visionflow {
enum class ExecutionState { Ready, Busy, Done, Error, Skipped };
QString executionStateName(ExecutionState state);
using PortData=std::variant<ImageFrame,QRect,QPointF,double,bool>;
struct PortValue {PortDefinition port; PortData data=false; bool valid=false; QString frameId,cycleId; FrameMetadata origin;};
struct ToolResult {QString toolId; ExecutionState state=ExecutionState::Ready; QString message; QMap<QString,PortValue> outputs; QVector<QRect> regions; QVector<QPointF> points; QVector<QLineF> lines; QRect searchRegion; double milliseconds=0;};
struct CycleResult {QString cycleId,frameId,recipeId; QByteArray configId; ExecutionState state=ExecutionState::Ready; QVector<ToolResult> tools; double milliseconds=0; QString error; QVector<FrameMetadata> frames;};
class ExecutionService {
public:
    // Bump when execution semantics change; validation records bind this identity.
    static QString version() { return QStringLiteral("visionflow.execution/1"); }
    CycleResult run(const QJsonObject& definition,const QString& projectDirectory,const QString& recipeId,
                    const QByteArray& configId,const std::shared_ptr<std::atomic_bool>& cancel,
                    const QString& targetTool={},const std::optional<ImageFrame>& frameSnapshot={}) const;
};
}
