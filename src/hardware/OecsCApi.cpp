#include "hardware/OecsCApi.hpp"
#include <QFileInfo>
#include <QLibrary>
#include <mutex>
#ifdef VISIONFLOW_WITH_OECS_SDK
#include <MvCameraControl.h>
#endif

namespace visionflow {
namespace {
#ifdef VISIONFLOW_WITH_OECS_SDK
// Every in-process SDK enumeration must hold this through the owned copy.
// SDK device-list storage is shared even between separate binding instances.
std::mutex enumerationMutex;
template<size_t N> QString bounded(const unsigned char (&text)[N]) {
    size_t n=0;
    while(n<N&&text[n])++n;
    return QString::fromLatin1(reinterpret_cast<const char*>(text),qsizetype(n));
}
class OecsDynamicApi final : public OecsCApi {
public:
    bool load(const QString& dllPath,QString& reason) override {
        if(library_.isLoaded()){reason="OECS kütüphanesi zaten yüklü";return false;}
        if(!QFileInfo(dllPath).isAbsolute()){reason="OECS DLL yolu mutlak olmalı";return false;}
        library_.setFileName(dllPath);
        if(!library_.load()){reason=library_.errorString();return false;}
        const auto fail=[&](const char* name){unbind();library_.unload();reason=QString("OECS export eksik: %1").arg(name);return false;};
        if(!bind(version_,"MV_CC_GetSDKVersion"))return fail("MV_CC_GetSDKVersion");
        if(!bind(enum_,"MV_CC_EnumDevices"))return fail("MV_CC_EnumDevices");
        if(!bind(create_,"MV_CC_CreateHandleWithoutLog"))return fail("MV_CC_CreateHandleWithoutLog");
        if(!bind(open_,"MV_CC_OpenDevice"))return fail("MV_CC_OpenDevice");
        if(!bind(close_,"MV_CC_CloseDevice"))return fail("MV_CC_CloseDevice");
        if(!bind(destroy_,"MV_CC_DestroyHandle"))return fail("MV_CC_DestroyHandle");
        return true;
    }
    bool unload() override {
        if(liveHandles_>0)return false;
        unbind();
        deviceInfos_.clear();
        library_.unload();
        return true;
    }
    quint32 sdkVersion() override {return version_?quint32(version_()):0;}
    int enumerateUsb(QVector<OecsDeviceInfo>& devices) override {
        devices.clear();
        deviceInfos_.clear();
        if(!enum_)return oecs::errorLoadLibrary;
        const std::lock_guard lock(enumerationMutex);
        MV_CC_DEVICE_INFO_LIST list{};
        const int status=enum_(MV_USB_DEVICE,&list);
        if(status!=MV_OK)return status;
        for(unsigned i=0;i<list.nDeviceNum&&i<MV_MAX_DEVICE_NUM;++i) {
            const MV_CC_DEVICE_INFO* info=list.pDeviceInfo[i];
            if(!info||info->nTLayerType!=MV_USB_DEVICE)continue;
            const MV_USB3_DEVICE_INFO& usb=info->SpecialInfo.stUsb3VInfo;
            devices.push_back({bounded(usb.chVendorName),bounded(usb.chModelName),bounded(usb.chSerialNumber),bounded(usb.chUserDefinedName),bounded(usb.chDeviceVersion),quint16(usb.idVendor),quint16(usb.idProduct),quint32(usb.nDeviceNumber)});
            deviceInfos_.push_back(*info);
        }
        return MV_OK;
    }
    int createHandle(int index,void** handle) override {
        if(!create_)return oecs::errorLoadLibrary;
        if(index<0||index>=deviceInfos_.size())return oecs::errorParameter;
        const int status=create_(handle,&deviceInfos_[index]);
        if(status==MV_OK)++liveHandles_;
        else *handle=nullptr;
        return status;
    }
    int openExclusive(void* handle) override {return open_?open_(handle,MV_ACCESS_Exclusive,0):oecs::errorLoadLibrary;}
    int closeDevice(void* handle) override {return close_?close_(handle):oecs::errorLoadLibrary;}
    int destroyHandle(void* handle) override {
        if(!destroy_)return oecs::errorLoadLibrary;
        const int status=destroy_(handle);
        if(status==MV_OK&&liveHandles_>0)--liveHandles_;
        return status;
    }
private:
    template<class F> bool bind(F& pointer,const char* name){pointer=reinterpret_cast<F>(library_.resolve(name));return pointer!=nullptr;}
    void unbind(){version_=nullptr;enum_=nullptr;create_=nullptr;open_=nullptr;close_=nullptr;destroy_=nullptr;}
    QLibrary library_;
    decltype(&::MV_CC_GetSDKVersion) version_=nullptr;
    decltype(&::MV_CC_EnumDevices) enum_=nullptr;
    decltype(&::MV_CC_CreateHandleWithoutLog) create_=nullptr;
    decltype(&::MV_CC_OpenDevice) open_=nullptr;
    decltype(&::MV_CC_CloseDevice) close_=nullptr;
    decltype(&::MV_CC_DestroyHandle) destroy_=nullptr;
    QVector<MV_CC_DEVICE_INFO> deviceInfos_;
    int liveHandles_=0;
};
#else
class OecsUnavailableApi final : public OecsCApi {
public:
    bool load(const QString&,QString& reason) override {reason="OECS SDK başlıkları olmadan derlendi; OECS C API kullanılamaz";return false;}
    bool unload() override {return true;}
    quint32 sdkVersion() override {return 0;}
    int enumerateUsb(QVector<OecsDeviceInfo>&) override {return oecs::errorLoadLibrary;}
    int createHandle(int,void**) override {return oecs::errorLoadLibrary;}
    int openExclusive(void*) override {return oecs::errorLoadLibrary;}
    int closeDevice(void*) override {return oecs::errorLoadLibrary;}
    int destroyHandle(void*) override {return oecs::errorLoadLibrary;}
};
#endif
}
std::unique_ptr<OecsCApi> makeOecsCApi() {
#ifdef VISIONFLOW_WITH_OECS_SDK
    return std::make_unique<OecsDynamicApi>();
#else
    return std::make_unique<OecsUnavailableApi>();
#endif
}
bool oecsSdkBindingCompiled() {
#ifdef VISIONFLOW_WITH_OECS_SDK
    return true;
#else
    return false;
#endif
}
}
