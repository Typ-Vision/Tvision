#pragma once
#include "StationAdmissionService.hpp"
#include "StationRunController.hpp"

namespace visionflow {
// Adapter over the existing per-station Coordinator. Worker scheduling and
// physical leases remain injected application responsibilities (COMM4-09/10).
class StationRuntimeAdmissionBoundary final:public StationAdmissionBoundary {
public:
    using Manifest=std::function<std::optional<StationAdmissionSnapshot>(const QUuid&)>;
    using Reserve=std::function<StationAdmissionReservation(const StationAdmissionSnapshot&)>;
    using Dispatch=std::function<void(const StationAdmissionIntent&,const ProductionCycle&,const std::shared_ptr<void>&)>;
    StationRuntimeAdmissionBoundary(StationRuntimeBoundary&,Manifest,Reserve,Dispatch);
    StationRuntimeAdmissionBoundary(const StationRuntimeAdmissionBoundary&)=delete;
    StationRuntimeAdmissionBoundary& operator=(const StationRuntimeAdmissionBoundary&)=delete;
    void bind(StationRunController&);
    std::optional<StationAdmissionSnapshot> snapshot(const QUuid&) const override;
    StationAdmissionBarriers barriers(const QUuid&) const override;
    StationAdmissionReservation reserve(const StationAdmissionSnapshot&) override;
    void begin(const StationAdmissionIntent&,const std::shared_ptr<void>&) override;
    bool drained(const QUuid&) const override;
private:
    void owner() const;
    StationRuntimeBoundary& runtime_;
    Manifest manifest_;Reserve reserve_;Dispatch dispatch_;
    QMap<QUuid,StationRunController*> controllers_;
    QThread* owner_=QThread::currentThread();
};
}
