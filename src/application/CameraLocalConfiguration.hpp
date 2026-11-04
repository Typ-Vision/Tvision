#pragma once
#include "domain/MeasurementSetupContract.hpp"
namespace visionflow {
struct CameraRuntimeLocation {
    QString path;
    QStringList dependencies;
    QString error;
};
// Machine-local producer/runtime/route settings. Never exported in project data.
class CameraLocalConfiguration {
public:
    explicit CameraLocalConfiguration(QString path):path_(std::move(path)){}
    QStringList producers() const;
    void setProducers(const QStringList&) const;
    QStringList automaticProducers() const;
    void setProducerEnabled(const QString&,bool) const;
    void removeProducer(const QString&) const;
    CameraRuntimeLocation runtime(const QString& backend,const QString& explicitPath={}) const;
    void setRuntime(const QString& backend,const QString& path,const QStringList& dependencies={}) const;
    static QString defaultRuntime(const QString& backend);
    std::optional<TransportLocator> route(const PhysicalDeviceIdentity&) const;
    void setRoute(const PhysicalDeviceIdentity&,const TransportLocator&) const;
private:
    QString key(const PhysicalDeviceIdentity&) const;
    QString path_;
};
}
