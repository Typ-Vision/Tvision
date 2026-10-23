#pragma once
#include "IndustrialCameraProvider.hpp"
#include "domain/IndustrialSourceReference.hpp"

namespace visionflow {
class CameraAcquisitionSessions;
// Captured by value before launching execution. Discovery runs inside its budget.
// No provider, permission or local route is decoded from recipe JSON.
struct IndustrialExecutionInputs {
    MeasurementSetupContract contract;
    QMap<QString,TransportLocator> routes; // CameraId -> explicit local route
    CameraProviderFactory factory;
    QMap<QString,DiscoveredDevice> admittedDevices; // Runtime: exact resolved device/backend at preparation
    CameraLeaseUse leaseUse=CameraLeaseUse::Editor;
    std::function<IndustrialDiscoverySnapshot(int,const std::shared_ptr<std::atomic_bool>&)> discover;
    // Ephemeral prepared context capability. Never serialized or inferred on open.
    std::shared_ptr<CameraAcquisitionSessions> sessions;
    QString sessionContext;
    quint64 sessionGeneration=0;
};
struct IndustrialExecutionEvidence {
    IndustrialCameraRequest request;
    AcquisitionReadback actual;
    FrameMetadata acquired; // Source-bound acquisition metadata; original frame/cycle
    FrameMetadata execution;
    QByteArray pixelHash;
    std::optional<FrameMetadata> workerAcquired; // Original worker metadata when sharing a physical camera
};
struct IndustrialExecutionFrame {
    ImageFrame image;
    std::shared_ptr<const IndustrialExecutionEvidence> evidence;
};
IndustrialCameraRequest industrialExecutionRequest(const IndustrialExecutionInputs&,const ToolDefinition&);
IndustrialExecutionFrame acquireIndustrial(const IndustrialExecutionInputs&,IndustrialCameraRequest,
    const QString& cycle,const QString& frame,int timeout,const std::shared_ptr<std::atomic_bool>&,bool experimentalEditor=false);
}
