#pragma once
#include "domain/RobotCalibrationSession.hpp"
#include <QMap>
#include <QThread>
#include <functional>

namespace visionflow {
enum class RobotCalibrationAction { Begin, SampleSolve, SampleHoldout, Solve, Validate, Activate, Abort, Recover };
struct RobotCalibrationSampleView {
    QUuid sample, frame;
    QString role, status, reason, pairing;
};
// Immutable application projection. No numeric pose defaults, inferred PASS or
// GUI-owned lease/progress flags. Providers recheck exact identities at execution.
struct RobotCalibrationView {
    QUuid station, session, candidate, candidateRevision, validation, active, activeRevision;
    quint64 revision=0;
    HandEyeMode mode=HandEyeMode::EyeInHand;
    CalibrationState state=CalibrationState::Created;
    QString camera, frameReadiness, tcpReadiness, lensReadiness, actualPose, leaseOwner, drain, physical;
    bool connected=false, actualPoseReady=false, recoveryRequired=false, draining=false, fixture=false, physicalReady=false;
    QVector<RobotCalibrationSampleView> samples;
    // Nonempty reason disables each operation. Missing operation is unavailable.
    QMap<RobotCalibrationAction,QString> commands;
};
struct RobotCalibrationCommand {
    RobotCalibrationAction action;
    RobotCalibrationView expected;
};
class RobotCalibrationWorkspace final {
public:
    using Read=std::function<RobotCalibrationView(const QUuid&,HandEyeMode)>;
    using Execute=std::function<QString(const RobotCalibrationCommand&)>;
    void bind(Read read,Execute execute);
    RobotCalibrationView view(const QUuid&,HandEyeMode) const;
    static QString disabled(const RobotCalibrationView&,RobotCalibrationAction);
    QString command(const RobotCalibrationCommand&);
private:
    void owner() const;
    Read read_;
    Execute execute_;
    QThread* owner_=QThread::currentThread();
};
}
