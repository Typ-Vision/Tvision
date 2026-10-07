#pragma once
#include "hardware/GenTLTransport.hpp"
namespace visionflow {
struct CameraDiscovery {
    QStringList producers,errors;
    QVector<CameraDevice> devices;
    bool cancelled=false;
};
class CameraService {
public:
    // Explicit producer files override environment discovery. Each producer has its own budget.
    CameraDiscovery discover(const QStringList& producerFiles={},int timeoutMs=1000,
                             const std::shared_ptr<std::atomic_bool>& cancel={},int scanWaitMs=250) const;
    void checkConnection(const CameraDevice& device) const;
};
}
