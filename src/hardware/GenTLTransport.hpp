#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <memory>

namespace visionflow {
struct CameraDevice {
    QString producerPath,interfaceId,deviceId,vendor,model,serialNumber,transport;
};
// One worker owns a transport. The process serializes all producer sessions.
class GenTLTransport {
public:
    explicit GenTLTransport(const QString& producerPath);
    ~GenTLTransport();
    GenTLTransport(const GenTLTransport&)=delete;
    GenTLTransport& operator=(const GenTLTransport&)=delete;
    QVector<CameraDevice> discover(int timeoutMs,const std::shared_ptr<std::atomic_bool>& cancel={});
    void openReadOnly(const CameraDevice& device);
    void closeDevice();
    bool isDeviceOpen() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
// Searches only supplied directories and GENICAM_GENTL64_PATH; no recursive disk scan.
QStringList genTLProducerPaths(const QStringList& directories={});
}
