// Synthetic GenTL C ABI fixture. Never linked into the product or installed as a driver.
#include <cstdint>
#define GCTLIDLL
#include <GenTL.h>
#include <cstring>
#include <thread>
#include <chrono>
namespace {
#ifdef VISIONFLOW_EMPTY_GENTL_FIXTURE
int mode=4;
#else
int mode=0;
#endif
int initializations=0,systems=0,interfaces=0,devices=0;
int tlToken,ifToken,devToken;
uint64_t maxScanWait=0;
void scan(uint64_t timeout) {
    if(timeout>maxScanWait)maxScanWait=timeout;
    if(mode==14 || mode==15)std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
}
GenTL::GC_ERROR string(const char* text,void* data,size_t* size) {
    if(!size)return GenTL::GC_ERR_INVALID_PARAMETER;
    const auto required=std::strlen(text)+1;
    if(!data){*size=required;return GenTL::GC_ERR_SUCCESS;}
    if(*size<required){*size=required;return GenTL::GC_ERR_BUFFER_TOO_SMALL;}
    std::memcpy(data,text,required);*size=required;return GenTL::GC_ERR_SUCCESS;
}
}
extern "C" __declspec(dllexport) int FixtureLiveHandles(){return initializations+systems+interfaces+devices;}
extern "C" __declspec(dllexport) void FixtureMode(int value){mode=value;maxScanWait=0;}
extern "C" __declspec(dllexport) uint64_t FixtureMaxScanWait(){return maxScanWait;}
namespace GenTL {
GC_API GCInitLib() {if(mode==6)return GC_ERR_IO;if(initializations)return GC_ERR_RESOURCE_IN_USE;++initializations;return GC_ERR_SUCCESS;}
GC_API GCCloseLib() {if(systems || interfaces || devices)return GC_ERR_RESOURCE_IN_USE;--initializations;return GC_ERR_SUCCESS;}
GC_API GCGetLastError(GC_ERROR* error,char* message,size_t* size){*error=GC_ERR_IO;return string("fixture transport failure",message,size);}
GC_API TLOpen(TL_HANDLE* handle){if(mode==7)return GC_ERR_IO;if(mode==12){*handle=nullptr;return GC_ERR_SUCCESS;}*handle=&tlToken;++systems;return GC_ERR_SUCCESS;}
GC_API TLClose(TL_HANDLE){if(interfaces || devices)return GC_ERR_RESOURCE_IN_USE;--systems;return GC_ERR_SUCCESS;}
GC_API TLUpdateInterfaceList(TL_HANDLE,bool8_t* changed,uint64_t timeout){scan(timeout);*changed=true;if(mode==1)return GC_ERR_IO;if(mode==8)std::this_thread::sleep_for(std::chrono::milliseconds(20));return GC_ERR_SUCCESS;}
GC_API TLGetNumInterfaces(TL_HANDLE,uint32_t* count){*count=mode==4 ? 0 : mode==14 ? 2 : 1;return GC_ERR_SUCCESS;}
GC_API TLGetInterfaceID(TL_HANDLE,uint32_t,char* data,size_t* size){if(mode==2){*size=1000000;return GC_ERR_SUCCESS;}return string("interface-1",data,size);}
GC_API TLOpenInterface(TL_HANDLE,const char* id,IF_HANDLE* handle){if(std::strcmp(id,"interface-1"))return GC_ERR_INVALID_ID;if(mode==13){*handle=nullptr;return GC_ERR_SUCCESS;}*handle=&ifToken;++interfaces;return GC_ERR_SUCCESS;}
GC_API IFClose(IF_HANDLE){if(devices)return GC_ERR_RESOURCE_IN_USE;--interfaces;return GC_ERR_SUCCESS;}
GC_API IFUpdateDeviceList(IF_HANDLE,bool8_t* changed,uint64_t timeout){scan(timeout);*changed=true;return GC_ERR_SUCCESS;}
GC_API IFGetNumDevices(IF_HANDLE,uint32_t* count){*count=1;return GC_ERR_SUCCESS;}
GC_API IFGetDeviceID(IF_HANDLE,uint32_t,char* data,size_t* size){return string("device-1",data,size);}
GC_API IFGetDeviceInfo(IF_HANDLE,const char*,DEVICE_INFO_CMD cmd,INFO_DATATYPE* type,void* data,size_t* size){
    if(mode==15)std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if(mode==5 && cmd==DEVICE_INFO_SERIAL_NUMBER)return GC_ERR_NOT_IMPLEMENTED;
    *type=mode==9 ? INFO_DATATYPE_INT32 : INFO_DATATYPE_STRING;
    const char* value="";
    switch(cmd){case DEVICE_INFO_VENDOR:value="Synthetic test producer";break;case DEVICE_INFO_MODEL:value="Fixture only";break;case DEVICE_INFO_SERIAL_NUMBER:value="TEST-001";break;case DEVICE_INFO_TLTYPE:value="Custom";break;default:return GC_ERR_NOT_IMPLEMENTED;}
    // Exercise BUFFER_TOO_SMALL on a null metadata buffer.
    auto code=string(value,data,size);return !data && code==GC_ERR_SUCCESS ? GC_ERR_BUFFER_TOO_SMALL : code;
}
GC_API IFOpenDevice(IF_HANDLE,const char* id,DEVICE_ACCESS_FLAGS flags,DEV_HANDLE* handle){
    if(mode==3)return GC_ERR_ACCESS_DENIED;
    if(mode==11){*handle=nullptr;return GC_ERR_SUCCESS;}
    if(flags!=DEVICE_ACCESS_READONLY)return GC_ERR_ACCESS_DENIED;
    if(std::strcmp(id,"device-1"))return GC_ERR_INVALID_ID;
    *handle=&devToken;++devices;return GC_ERR_SUCCESS;
}
GC_API DevClose(DEV_HANDLE){--devices;return GC_ERR_SUCCESS;}
}
