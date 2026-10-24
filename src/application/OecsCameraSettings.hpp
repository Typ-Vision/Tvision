#pragma once
#include "IndustrialCameraProvider.hpp"
#include "hardware/OecsSession.hpp"

namespace visionflow {
// Settings-only product path. Never supplies an acquisition provider/admission.
struct OecsCameraSettings {
    using ApiFactory=std::function<std::unique_ptr<OecsCApi>()>;
    static QString installedRuntime();
    static bool available(const DiscoveredDevice&,const QString& runtime);
    static std::unique_ptr<IndustrialCameraBackend> create(const DiscoveredDevice&,const QString&,ApiFactory);
};
}
