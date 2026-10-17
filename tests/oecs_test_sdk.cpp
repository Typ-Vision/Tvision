// Test implementation only; ABI declarations come from the optional local SDK.
#define MV_CAMCTRL_EXPORTS
#include <cstdint>
#include <MvCameraControl.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

namespace {
MV_CC_DEVICE_INFO sharedInfo{};
std::mutex bufferMutex;
std::atomic<int> active{0},overlap{0},sequence{0};
}
extern "C" {
MV_CAMCTRL_API unsigned int __stdcall MV_CC_GetSDKVersion(){return 0x3050200;}
MV_CAMCTRL_API int __stdcall MV_CC_EnumDevices(unsigned int,MV_CC_DEVICE_INFO_LIST* list){
    if(active.fetch_add(1)!=0)++overlap;
    {
        const std::lock_guard lock(bufferMutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        sharedInfo={};sharedInfo.nTLayerType=MV_USB_DEVICE;
        sharedInfo.nMacAddrHigh=0x12345678; // Also verify the complete ABI record is copied.
        auto& usb=sharedInfo.SpecialInfo.stUsb3VInfo;
        std::strcpy(reinterpret_cast<char*>(usb.chVendorName),"VF");
        std::strcpy(reinterpret_cast<char*>(usb.chModelName),"CAM");
        const auto serial=std::to_string(++sequence);
        std::strcpy(reinterpret_cast<char*>(usb.chSerialNumber),serial.c_str());
        *list={};list->nDeviceNum=3;
        list->pDeviceInfo[0]=nullptr;
        list->pDeviceInfo[1]=&sharedInfo;
        list->pDeviceInfo[2]=nullptr;
    }
    --active;return MV_OK;
}
MV_CAMCTRL_API int __stdcall MV_CC_CreateHandleWithoutLog(void** handle,const MV_CC_DEVICE_INFO* info){
    *handle=new MV_CC_DEVICE_INFO(*info);return MV_OK;
}
MV_CAMCTRL_API int __stdcall MV_CC_OpenDevice(void*,unsigned int,unsigned short){return MV_OK;}
MV_CAMCTRL_API int __stdcall MV_CC_CloseDevice(void*){return MV_OK;}
MV_CAMCTRL_API int __stdcall MV_CC_DestroyHandle(void* handle){delete static_cast<MV_CC_DEVICE_INFO*>(handle);return MV_OK;}
__declspec(dllexport) const char* testHandleSerial(void* handle){return reinterpret_cast<const char*>(static_cast<MV_CC_DEVICE_INFO*>(handle)->SpecialInfo.stUsb3VInfo.chSerialNumber);}
__declspec(dllexport) unsigned int testHandleMarker(void* handle){return static_cast<MV_CC_DEVICE_INFO*>(handle)->nMacAddrHigh;}
__declspec(dllexport) int testEnumerationOverlap(){return overlap.load();}
}
