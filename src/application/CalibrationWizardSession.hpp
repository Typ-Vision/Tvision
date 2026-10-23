#pragma once
#include "ActiveCalibrationService.hpp"
#include "EdgePointService.hpp"
#include "EdgePairService.hpp"

namespace visionflow {
struct WizardPoint {
    QString id,name,scanId,detectionId,pairId;
    QPointF pixel;
    CalibrationImageEvidence image;
    ToolRoi roi;
    QJsonObject parameters;
    bool refined=false;
};
// Immutable worker input and opaque completion. No worker retains session/UI refs.
class WizardScanJob {
    friend class CalibrationWizardSession;
    WizardScanJob()=default;
    ImageFrame frame_;
    ToolRoi roi_;
    QJsonObject parameters_;
    std::optional<EdgePairOptions> pair_;
    QString scanId_,ticket_;
    std::shared_ptr<std::atomic_bool> cancel_;
public:
    void cancel() const {cancel_->store(true);}
};
class WizardScanResult {
    friend class CalibrationWizardSession;
    WizardScanResult()=default;
    QString ticket_,scanId_,error_;
    ToolRoi roi_;
    QJsonObject parameters_;
    QVector<QPair<QPointF,bool>> candidates_;
    QVector<QPair<int,int>> pairs_;
    std::optional<int> selected_;
public:
    const QString& error() const {return error_;}
    const QVector<QPair<QPointF,bool>>& candidates() const {return candidates_;}
    const QVector<QPair<int,int>>& pairs() const {return pairs_;}
    std::optional<int> selected() const {return selected_;}
};
class WizardPlateJob {
    friend class CalibrationWizardSession;
    WizardPlateJob()=default;
    ImageFrame frame_;
    CalibrationGridTarget target_;
    QString ticket_;
    std::shared_ptr<std::atomic_bool> cancel_;
public:
    void cancel() const {cancel_->store(true);}
};
class WizardPlateResult {
    friend class CalibrationWizardSession;
    WizardPlateResult()=default;
    QString ticket_,error_;
    std::optional<CalibrationGridObservation> observation_;
    PlateDetectionDiagnostics diagnostics_;
    std::optional<RawPlateObservation> rawPlate_;
public:
    const QString& error() const {return error_;}
    const std::optional<RawPlateObservation>& rawPlate() const {return rawPlate_;}
    const PlateDetectionDiagnostics& diagnostics() const {return diagnostics_;}
    const std::optional<CalibrationGridObservation>& observation() const {return observation_;}
};
// Owner-thread draft only. It never edits a recipe, saves or activates a model.
class CalibrationWizardSession {
public:
    CalibrationWizardSession()=default;
    CalibrationWizardSession(const CalibrationWizardSession&)=delete;
    CalibrationWizardSession& operator=(const CalibrationWizardSession&)=delete;
    ~CalibrationWizardSession(){cancel();}
    static QString configurationId(const QJsonObject& relevantAdapterConfiguration);
    void useFrame(ImageFrame,MeasurementSetup,QString origin,std::optional<ExperimentalCalibrationEvidence> = {});
    void importFile(QString path,MeasurementSetup,bool declaredSameSetup);
    void clear();
    void cancel();
    bool busy() const {return !ticket_.isEmpty();}
    const std::optional<ImageFrame>& frame() const {return frame_;}
    const QVector<WizardPoint>& points() const {return points_;}
    const std::optional<CalibrationModelRecord>& calculated() const {return model_;}
    WizardScanJob scanEdge(QString scanId,ToolRoi,QJsonObject edgeParameters);
    WizardScanJob scanPair(QString scanId,ToolRoi,EdgePairOptions = {});
    static WizardScanResult run(const WizardScanJob&);
    bool accept(const WizardScanResult&);
    WizardPlateJob scanPlate(CalibrationGridTarget);
    static WizardPlateResult run(const WizardPlateJob&);
    bool accept(const WizardPlateResult&);
    void confirmPlate();
    void invalidatePlate(){cancel();plate_.reset();rawPlate_.reset();plateConfirmed_=false;model_.reset();}
    const std::optional<RawPlateObservation>& rawPlate() const {return rawPlate_;}
    const std::optional<CalibrationGridObservation>& plate() const {return plate_;}
    CalibrationModelRecord calculatePlate(const CalibrationControl& = {});
    // Candidate index is from this scan's actual accepted detections, never XY input.
    void chooseCandidate(const QString& scanId,int candidate);
    void removeScan(QString scanId);
    void renamePoint(const QString& pointId,const QString& name);
    void selectPair(const QString& firstId,const QString& secondId);
    void setKnownMillimetres(double);
    std::optional<double> distancePixels() const;
    CalibrationModelRecord calculate(const CalibrationControl& = {});
private:
    WizardScanJob begin(QString,ToolRoi,QJsonObject,std::optional<EdgePairOptions>);
    void populate(const WizardScanResult&,int);
    const WizardPoint& point(const QString&) const;
    void invalidateSelection();
    std::optional<ImageFrame> frame_;
    MeasurementSetup setup_;
    CalibrationImageEvidence image_;
    QVector<WizardPoint> points_;
    QMap<QString,WizardScanResult> scans_;
    QString ticket_,first_,second_,modelId_;
    std::shared_ptr<std::atomic_bool> cancel_;
    double knownMm_=0;
    std::optional<CalibrationModelRecord> model_;
    std::optional<CalibrationGridObservation> plate_;
    std::optional<RawPlateObservation> rawPlate_;
    bool plateConfirmed_=false;
};
}
