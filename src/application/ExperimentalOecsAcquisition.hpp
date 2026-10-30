#pragma once
#include "IndustrialCameraProvider.hpp"
#include "hardware/OecsCApi.hpp"

namespace visionflow {
// Explicit preview/calibration entries. Cannot supply a production factory or Runtime lease.
class ExperimentalOecsAcquisition {
public:
    using ApiFactory=std::function<std::unique_ptr<OecsCApi>()>;
    static QString notice();
    static QString installedRuntime();
    static bool available(const DiscoveredDevice&,const QString& dll);
    static std::unique_ptr<CameraWorkerSession> open(const IndustrialCameraRequest&,const QString& dll,
                                                   ApiFactory api=makeOecsCApi,CameraLeaseArbiter::Ticket lease={});
    static std::unique_ptr<CameraWorkerSession> openCalibration(const IndustrialCameraRequest&,const QString& dll,
                                                              ApiFactory api=makeOecsCApi,CameraLeaseArbiter::Ticket lease={});
    // Writes lossless PGM plus a sidecar. Refuses overwrite; evidence commits first.
    static void save(const AcquiredFrame&,const QString& absolutePgm);
};
}
