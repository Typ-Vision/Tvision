#pragma once
#include "domain/MeasurementSetupContract.hpp"
namespace visionflow {
// Machine-local route hints only. Never exported in MeasurementEnvironment/project.
class CameraLocalConfiguration {
public:
    explicit CameraLocalConfiguration(QString path):path_(std::move(path)){}
    QStringList producers() const;
    void setProducers(const QStringList&) const;
    std::optional<TransportLocator> route(const PhysicalDeviceIdentity&) const;
    void setRoute(const PhysicalDeviceIdentity&,const TransportLocator&) const;
private:
    QString key(const PhysicalDeviceIdentity&) const;
    QString path_;
};
}
