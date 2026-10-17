#pragma once
#include "hardware/CameraFeatures.hpp"
#include <QString>
#include <QVector>
#include <memory>

namespace visionflow {
struct OecsDeviceInfo {
    QString vendor,model,serial,userName,deviceVersion;
    quint16 usbVendorId=0,usbProductId=0;
    quint32 deviceNumber=0;
};
// Borrowed until releaseBuffer. No vendor struct escapes this boundary.
struct OecsBuffer {
    const void* data=nullptr;
    quint64 bytes=0,stride=0,hardwareFrameId=0,deviceTicks=0;
    quint32 width=0,height=0,pixelFormat=0;
    bool complete=true,overflow=false,chunks=false;
    quint64 triggerGeneration=0;
    quint32 triggerCounter=0;
    bool triggerEvidence=false;
};
// Backend evidence from a quiescent, trigger-ready stream. Baselines must be actual
// device counters in this epoch, never inferred from arrival order or pixel hashes.
struct OecsTriggerArm {
    quint64 generation=0;
    quint32 frameCounter=0,triggerCounter=0;
};
struct OecsStreamLimits {
    quint32 buffers=4;
    quint64 poolBytes=128ULL*1024*1024;
    int timeoutMs=1000;
};
// MV_* status values the session interprets; copied by value, no SDK header needed.
namespace oecs {
constexpr int ok=0;
constexpr int errorUnsupported=int(0x80000001u);
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
    virtual int readFeature(void*,const QString&,CameraFeature&) {return oecs::errorLoadLibrary;}
    virtual int writeFeature(void*,const QString&,const FeatureValue&) {return oecs::errorLoadLibrary;}
    // prepare must drain/quiesce old exposures, verify stream-ready and establish an
    // actual counter epoch. waitBuffer may tag generation ONLY from that epoch's
    // hardware event evidence. Echoing the caller generation on any next frame is invalid.
    // Native OECS currently cannot prove this; fail closed before sending a command.
    virtual int prepareSoftwareTrigger(void*,quint64,int,OecsTriggerArm&) {return oecs::errorUnsupported;}
    virtual int executeSoftwareTrigger(void*) {return oecs::errorUnsupported;}
    virtual int setBufferCount(void*,quint32) {return oecs::errorLoadLibrary;}
    virtual int startStream(void*) {return oecs::errorLoadLibrary;}
    virtual int waitBuffer(void*,int,OecsBuffer&) {return oecs::errorLoadLibrary;}
    virtual int releaseBuffer(void*) {return oecs::errorLoadLibrary;}
    virtual int stopStream(void*) {return oecs::errorLoadLibrary;}
    virtual int flushStream(void*) {return oecs::errorLoadLibrary;}
};
// Dynamic MvCameraControl binding when built with VISIONFLOW_OECS_INCLUDE_DIR, otherwise an
// implementation whose load() always fails with an explicit reason.
std::unique_ptr<OecsCApi> makeOecsCApi();
bool oecsSdkBindingCompiled();
}
