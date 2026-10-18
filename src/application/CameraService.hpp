#pragma once
#include "hardware/GenTLTransport.hpp"
#include "domain/MeasurementSetupContract.hpp"
namespace visionflow {
// Offline preparation DTOs. No SDK enumeration, open, lease or capability grant.
struct IndustrialDiscoverySnapshot {
    QVector<DiscoveredDevice> devices;
    QStringList errors;
    bool cancelled=false;
};
struct IndustrialDeviceGroup {
    PhysicalDeviceIdentity identity;
    QVector<DiscoveredDevice> routes;
    bool ambiguous=false;
};
struct CameraDiscovery {
    QStringList producers,errors;
    QVector<CameraDevice> devices;
    bool cancelled=false;
};
struct CameraDiscoveryEntry {
    std::optional<DeviceBinding> webcam;
    std::optional<IndustrialDeviceGroup> industrial;
};
struct CameraInventory {
    QVector<CameraDiscoveryEntry> devices;
    IndustrialDiscoverySnapshot industrial;
    QStringList producers,errors;
    bool cancelled=false;
};
class CameraService {
public:
    CameraInventory inventory(const QStringList& producerFiles={},int timeoutMs=1000,
                             const std::shared_ptr<std::atomic_bool>& cancel={}) const;
    static CameraInventory combine(const QVector<DeviceBinding>& webcams,const IndustrialDiscoverySnapshot&);
    static IndustrialDiscoverySnapshot industrialSnapshot(const CameraDiscovery&);
    // Explicit producer files override environment discovery. Each producer has its own budget.
    CameraDiscovery discover(const QStringList& producerFiles={},int timeoutMs=1000,
                             const std::shared_ptr<std::atomic_bool>& cancel={},int scanWaitMs=250) const;
    void checkConnection(const CameraDevice& device) const;
    static QVector<IndustrialDeviceGroup> groupIndustrial(const IndustrialDiscoverySnapshot&);
    // Resolve afresh by portable physical identity; route hints never select another
    // device. Multiple routes require an explicit local choice, never first/index.
    static DiscoveredDevice resolveIndustrial(const MeasurementCamera&,const IndustrialDiscoverySnapshot&,
                                              const std::optional<TransportLocator>& localRoute={});
};
}
