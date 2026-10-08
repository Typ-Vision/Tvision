#pragma once
#include "domain/ToolGraph.hpp"
#include "ImageFileService.hpp"
#include "domain/Coordinates.hpp"
#include <functional>
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
enum class PartDecision { NotEvaluated, Pass, Fail };
struct RegionMask {QByteArray pixels; QSize size; QPoint origin;};
struct ToolResult {RegionMask mask;QString toolId; ExecutionState state=ExecutionState::Ready; QString message; QMap<QString,PortValue> outputs; QVector<QRect> regions; QVector<QPointF> points; QVector<QLineF> lines; QRect searchRegion; double milliseconds=0; PartDecision decision=PartDecision::NotEvaluated; QString errorCode; std::optional<CalibrationDependency> calibration;};
class PlanarCalibration;
struct MeasurementExecutionContext {
    std::shared_ptr<const PlanarCalibration> calibration;
    std::optional<CalibrationDependency> current;
    QSize rawSize;
    QString error;
};
using CalibrationResolver=std::function<MeasurementExecutionContext(const ImageFrame&)>;
struct CycleResult {QString cycleId,frameId,recipeId,evaluationId,targetToolId; bool partial=false; QStringList executedTools; int acquisitions=0; QByteArray configId; ExecutionState state=ExecutionState::Ready; QVector<ToolResult> tools; double milliseconds=0; QString error; QVector<FrameMetadata> frames;};
struct CachedToolResult {QByteArray signature;ToolResult result;};
struct TargetEvaluation {ImageFrame frame;QByteArray scope;QMap<QString,CachedToolResult> cache;};
class ExecutionService {
public:
    // Bump when execution semantics change; validation records bind this identity.
    static QString version() { return QStringLiteral("visionflow.execution/6"); }
    static QByteArray toolSignature(const QJsonObject&,const QString& directory,const QString& tool,const QByteArray& scope);
    CycleResult runTool(const QJsonObject& definition,const QString& directory,const QString& recipeId,const QByteArray& configId,
        const std::shared_ptr<std::atomic_bool>& cancel,const QString& target,const TargetEvaluation& evaluation,const CalibrationResolver& resolver={}) const;
    CycleResult run(const QJsonObject& definition,const QString& projectDirectory,const QString& recipeId,
                    const QByteArray& configId,const std::shared_ptr<std::atomic_bool>& cancel,
                    const QString& targetTool={},const std::optional<ImageFrame>& frameSnapshot={},const CalibrationResolver& resolver={},const TargetEvaluation* targetOnly=nullptr) const;
};
}
