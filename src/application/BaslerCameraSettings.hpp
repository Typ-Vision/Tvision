#pragma once
#include "IndustrialCameraProvider.hpp"
namespace visionflow {
// Settings only; never supplies acquisition admission.
struct BaslerCameraSettings {
    static QString unavailableReason(const DiscoveredDevice&,const QString& runtime);
    static std::unique_ptr<IndustrialCameraBackend> create(const DiscoveredDevice&,const QString&,const QStringList& dependencies={});
};
// Product preview plus manual trigger diagnostics; G1/production stays closed.
struct BaslerAcquisition {
    static std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest&,const QString&,const QStringList& dependencies={});
};
}
