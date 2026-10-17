#pragma once
#include <QString>
#include <QVector>
#include <memory>

namespace visionflow {
struct OecsDeviceInfo {
    QString vendor,model,serial,userName,deviceVersion;
    quint16 usbVendorId=0,usbProductId=0;
    quint32 deviceNumber=0;
};
// MV_* status values the session interprets; copied by value, no SDK header needed.
namespace oecs {
constexpr int ok=0;
constexpr int errorHandle=int(0x80000000u),errorParameter=int(0x80000004u),errorLoadLibrary=int(0x8000000Cu);
constexpr int errorAccessDenied=int(0x80000203u),errorBusy=int(0x80000204u);
}
// Vendor C boundary. One worker thread calls an instance. Destruction never calls the SDK or unloads.
class OecsCApi {
public:
    virtual ~OecsCApi()=default;
    // Absolute path; resolves every required export or stays unloaded and fills reason.
    virtual bool load(const QString& dllPath,QString& reason)=0;
    // Refuses (false) while any created handle is not destroyed.
    virtual bool unload()=0;
    virtual quint32 sdkVersion()=0;
    virtual int enumerateUsb(QVector<OecsDeviceInfo>& devices)=0;
    // index refers to the last enumerateUsb result; handle stays null on error.
    virtual int createHandle(int index,void** handle)=0;
    virtual int openExclusive(void* handle)=0;
    virtual int closeDevice(void* handle)=0;
    virtual int destroyHandle(void* handle)=0;
};
// Dynamic MvCameraControl binding when built with VISIONFLOW_OECS_INCLUDE_DIR, otherwise an
// implementation whose load() always fails with an explicit reason.
std::unique_ptr<OecsCApi> makeOecsCApi();
bool oecsSdkBindingCompiled();
}
