#pragma once
#include "domain/ToolGraph.hpp"
#include "vision/metrology/FitDiagnostic.hpp"
#include "ImageFileService.hpp"
#include "AcquisitionService.hpp"
#include "domain/Coordinates.hpp"
#include "domain/ToolValues.hpp"
#include "domain/AssignmentConfig.hpp"
#include "DecisionEvaluator.hpp"
#include "PublicationService.hpp"
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
using PortData=std::variant<ImageFrame,QRect,QPointF,double,bool,QString,CodeResults,PointSet,Profile,Line2D,Circle2D,Arc2D,Pose2D,InspectionResults,AssignmentBundle>;
struct PortValue {PortDefinition port; PortData data=false; bool valid=false; QString frameId,cycleId; FrameMetadata origin;
    QString evaluationId,method;int methodVersion=0;bool diagnostic=false;QString invalidReason;std::optional<ImageGeometry> imageGeometry;
};
enum class PartDecision { NotEvaluated, Pass, Fail };
struct RegionMask {QByteArray pixels; QSize size; QPoint origin;};
// Bounded live diagnostics; terminal evidence remains the typed output ports.
struct EdgeProfilePoint { double distance=0,raw=0; std::optional<double> smooth; };
struct EdgeDisplayCandidate { QPointF point; double distance=0,strength=0; bool accepted=false,selected=false; QString reason,refinement; };
struct EdgeDiagnostics { QLineF scan; double width=0,length=0; int sampleCount=0,derivativeCount=0,candidateCount=0; QVector<EdgeProfilePoint> profile; QVector<QPointF> derivative; QVector<EdgeDisplayCandidate> candidates; };
struct ToolOverlayGeometry { FrameMetadata origin; QSize rawSize; QVector<QVector<QPointF>> outlines; QVector<QLineF> guides,resultLines; QString reason; };
struct ToolResult {std::optional<ToolOverlayGeometry> overlayGeometry;std::optional<FitDiagnostic> fitDiagnostic;std::optional<EdgeDiagnostics> edgeDiagnostics;RegionMask mask;QString toolId; ExecutionState state=ExecutionState::Ready; QString message; QMap<QString,PortValue> outputs; QVector<QRect> regions; QVector<QPointF> points,rejectedPoints; QVector<QLineF> lines; QRect searchRegion; double milliseconds=0; PartDecision decision=PartDecision::NotEvaluated; QString errorCode; std::optional<CalibrationDependency> calibration;};
class PlanarCalibration;
struct MeasurementExecutionContext {
    std::shared_ptr<const PlanarCalibration> calibration;
    std::optional<CalibrationDependency> current;
    QSize rawSize;
    QString error;
};
using CalibrationResolver=std::function<MeasurementExecutionContext(const ImageFrame&)>;
class ProductionSnapshot;
struct RuntimeVariableContext;
struct IndustrialExecutionInputs;
struct IndustrialExecutionEvidence;
// Legacy frameId denotes the sole acquired source. In the multi-source contract
// it must be empty; SourceFrameIndex/frames provide the source-specific IDs.
// Schema9 execution keeps one frame identity per source.
struct CycleResult {QString cycleId,frameId,recipeId,evaluationId,targetToolId; bool partial=false; QStringList executedTools; int acquisitions=0; QByteArray configId; ExecutionState state=ExecutionState::Ready; QVector<ToolResult> tools; double milliseconds=0; QString error; QVector<FrameMetadata> frames;
    QVector<PublicationIntent> publicationIntents;
    ResultDecision decision() const;
    const std::shared_ptr<const RuntimeVariableContext>& variables() const {return variables_;}
    // Editor diagnostics only; this does not issue Runtime completion authority.
    QMap<QString,std::shared_ptr<const IndustrialExecutionEvidence>> industrialEvidence;
private:
    friend class ExecutionService;
    friend class RuntimeCoordinator;
    QString acquisitionFailureSourceId_; // diagnostic policy preflight context; no completion authority
    friend class RuntimeService;
    friend class ProductionRecipeService;
    friend class MultiSourceRuntimeService;
    friend class ExperimentalRuntime;
    std::shared_ptr<const RuntimeVariableContext> variables_;
    QByteArray experimentalDigest_;
    quint64 experimentalTicket_=0;
    std::shared_ptr<const ProductionSnapshot> experimentalSnapshot_;
    // In-process capability issued only after Runtime executes its immutable lease.
    // Disk records and Editor results cannot recreate this authority.
    QByteArray completionDigest_;
    quint64 completionTicket_=0;
    std::shared_ptr<const ProductionSnapshot> completionSnapshot_;
};
struct CachedToolResult {QByteArray signature;ToolResult result;};
struct TargetEvaluation {ImageFrame frame;QByteArray scope;QMap<QString,CachedToolResult> cache;};
struct ExecutionLimits { qint64 deadlineMs=30000; quint64 imageBytes=512ull*1024*1024; };
class ExecutionService {
public:
    // Optional application adapter factory for deterministic acquisition contracts.
    // Production/default callers use the normal file/Qt webcam adapters.
    using SourceAdapterFactory=std::function<std::unique_ptr<AcquisitionAdapter>(const ToolDefinition&)>;
    explicit ExecutionService(SourceAdapterFactory factory={}):sourceAdapterFactory_(std::move(factory)){}
    explicit ExecutionService(const IndustrialExecutionInputs&,SourceAdapterFactory factory={});
    // Ephemeral unverified acquisition policy shared with ExperimentalRuntime.
    // Never persisted and never issues production completion authority.
    ExecutionService withExperimentalEditorReplay(bool enabled=true) const {auto service=*this;service.experimentalEditorReplay_=enabled;service.automaticAcquisition_=false;return service;}
    // Bump when execution semantics change; validation records bind this identity.
    static QString version() { return QStringLiteral("visionflow.execution/35"); }
    ExecutionService withPublicationSnapshot(std::shared_ptr<const PublicationSnapshot> s) const {auto copy=*this;copy.publication_=std::move(s);return copy;}
    ExecutionService withCycleId(QString id) const {auto copy=*this;copy.cycleId_=std::move(id);return copy;}
    static QByteArray toolSignature(const QJsonObject&,const QString& directory,const QString& tool,const QByteArray& scope);
    CycleResult runTool(const QJsonObject& definition,const QString& directory,const QString& recipeId,const QByteArray& configId,
        const std::shared_ptr<std::atomic_bool>& cancel,const QString& target,const TargetEvaluation& evaluation,const CalibrationResolver& resolver={}) const;
    CycleResult run(const QJsonObject& definition,const QString& projectDirectory,const QString& recipeId,
                    const QByteArray& configId,const std::shared_ptr<std::atomic_bool>& cancel,
                    const QString& targetTool={},const std::optional<ImageFrame>& frameSnapshot={},const CalibrationResolver& resolver={},const TargetEvaluation* targetOnly=nullptr,const QJsonObject& acquisition={},const ExecutionLimits& limits={}) const;
private:
    QString cycleId_;
    std::shared_ptr<const PublicationSnapshot> publication_;
    SourceAdapterFactory sourceAdapterFactory_;
    std::shared_ptr<const IndustrialExecutionInputs> industrial_;
    bool experimentalEditorReplay_=false;
    bool automaticAcquisition_=true;
};
}
