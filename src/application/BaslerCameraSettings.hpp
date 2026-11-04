#pragma once
#include "IndustrialCameraProvider.hpp"
namespace visionflow {
// Settings only; never supplies acquisition admission.
struct BaslerCameraSettings {
    static QString unavailableReason(const DiscoveredDevice&,const QString& runtime);
    static std::unique_ptr<IndustrialCameraBackend> create(const DiscoveredDevice&,const QString&,const QStringList& dependencies={});
};
}
