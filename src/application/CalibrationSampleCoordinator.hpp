#pragma once
#include "RobotCalibrationSessionService.hpp"
#include "StationDeliveryLedger.hpp"
#include "IndependentCalibrationCapture.hpp"
#include <QElapsedTimer>
#include <QSet>
#include <memory>

namespace visionflow {
// Owner-thread coordinator. Worker jobs contain the real independent capture job,
// never a preview raster. The durable COMM4-13 ledger is the request authority.
class CalibrationSampleCoordinator {
public:
    class Job {
        friend class CalibrationSampleCoordinator;
        V4RequestIdentity key_;
        QUuid session_, sample_, cameraSession_;
        quint64 token_=0, generation_=0;
        std::optional<IndependentCaptureJob> capture_;
    public:
        quint64 token() const { return token_; }
        QUuid cameraSession() const { return cameraSession_; }
    };
    struct Admission { CommunicationV4Frame response; std::optional<Job> job; };
    CalibrationSampleCoordinator(RobotCalibrationSessionService&, CalibrationDependencyProvider&,
        StationDeliveryLedger&, bool softwareFixture=false);
    Admission sample(const V4RequestIdentity&, const CommunicationV4Frame&, IndependentCaptureRequest);
    static IndependentCaptureResult run(const Job&, const IndependentCaptureBackend& = {});
    // nullopt means a foreign/retired/late completion; it never touches another sample.
    std::optional<CommunicationV4Frame> complete(const Job&, const IndependentCaptureResult&);
    // Timeout is a fence, not robot release. Explicit recovery must drain the
    // worker/hardware before a failure terminal can be sealed.
    bool timeout(const QUuid& session);
    std::optional<CommunicationV4Frame> recover(const QUuid& session, CalibrationDrainPort&);
private:
    struct Pending {
        Job job;
        CommunicationV4Frame request;
        std::shared_ptr<IndependentCalibrationSession> capture;
        bool fenced=false;
    };
    void owner() const;
    CommunicationV4Frame response(const CommunicationV4Frame&, quint32 kind, quint32 reason) const;
    CommunicationV4Frame finish(Pending&, const QUuid& frame, quint32 reason);
    RobotCalibrationSessionService& sessions_;
    CalibrationDependencyProvider& dependencies_;
    StationDeliveryLedger& ledger_;
    QMap<QUuid, Pending> pending_;
    QSet<QUuid> fenced_;
    QMap<QUuid, Pending> restored_;
    quint64 nextToken_=1;
    bool softwareFixture_=false;
    QElapsedTimer clock_;
    QThread* owner_=QThread::currentThread();
};
}
