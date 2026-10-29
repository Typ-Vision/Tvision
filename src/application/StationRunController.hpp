#pragma once
#include "StationRuntimeBoundary.hpp"

namespace visionflow {
// Owner-thread process state only. No socket or wire dispatch; never persisted as permission.
class StationRunController final {
public:
    StationRunController(StationRuntimeBoundary&,QUuid station);
    void run(const std::shared_ptr<const StationSnapshot>&);
    void setAuto(bool);
    void stop();
    void poll();
    StationRuntimeState state() const;
    bool drained() const;
    bool automatic() const;
    QUuid station() const {return station_;}
    QUuid activation() const {return activation_;}
    quint64 controlGeneration() const {owner();return controlGeneration_;}
private:
    friend class StationRuntimeAdmissionBoundary;
    void owner() const;
    StationRuntimeBoundary& boundary_;
    QUuid station_,activation_;
    bool auto_=false;
    quint64 controlGeneration_=0;
    QThread* owner_=QThread::currentThread();
};
}
