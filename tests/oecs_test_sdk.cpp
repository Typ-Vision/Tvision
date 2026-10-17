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
unsigned char raster[6]={1,2,3,4,5,6};
int streamMode=0,borrowed=0,running=0;
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
// Small ABI feature oracle, independent of the application node-map fixture.
MV_CAMCTRL_API int __stdcall MV_XML_GetNodeAccessMode(void*,const char* n,MV_XML_AccessMode* v){*v=std::strcmp(n,"Missing")==0?AM_NI:AM_RW;return 0;}
MV_CAMCTRL_API int __stdcall MV_XML_GetNodeInterfaceType(void*,const char* n,MV_XML_InterfaceType* v){
    *v=std::strcmp(n,"ExposureTime")==0?IFT_IFloat:std::strcmp(n,"PixelFormat")==0?IFT_IEnumeration:std::strcmp(n,"ReverseX")==0?IFT_IBoolean:std::strcmp(n,"DeviceModelName")==0?IFT_IString:IFT_IInteger;return 0;}
MV_CAMCTRL_API int __stdcall MV_XML_GetGenICamXML(void*,unsigned char* data,unsigned int size,unsigned int* length){
    const char* xml="<RegisterDescription><Float Name=\"ExposureTime\"><Unit>us</Unit></Float><Enumeration Name=\"PixelFormat\"><pEnumEntry>Mono8</pEnumEntry></Enumeration><EnumEntry Name=\"Mono8\"><Value>17301505</Value></EnumEntry></RegisterDescription>";
    *length=unsigned(std::strlen(xml));if(!data||size<*length)return int(0x80000006u);std::memcpy(data,xml,*length);return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_GetIntValueEx(void*,const char*,MVCC_INTVALUE_EX* v){*v={};v->nCurValue=640;v->nMin=1;v->nMax=2048;v->nInc=1;return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_SetIntValueEx(void*,const char*,int64_t v){return v==641?0:int(0x80000004u);}
MV_CAMCTRL_API int __stdcall MV_CC_GetFloatValue(void*,const char*,MVCC_FLOATVALUE* v){*v={};v->fCurValue=123.5f;v->fMin=1;v->fMax=1000;return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_SetFloatValue(void*,const char*,float v){return v==124.5f?0:int(0x80000004u);}
MV_CAMCTRL_API int __stdcall MV_CC_GetEnumValue(void*,const char*,MVCC_ENUMVALUE* v){*v={};v->nCurValue=17301505;v->nSupportedNum=1;v->nSupportValue[0]=v->nCurValue;return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_SetEnumValue(void*,const char*,unsigned int){return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_GetBoolValue(void*,const char*,bool* v){*v=false;return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_SetBoolValue(void*,const char*,bool v){return v?0:int(0x80000004u);}
MV_CAMCTRL_API int __stdcall MV_CC_GetStringValue(void*,const char*,MVCC_STRINGVALUE* v){*v={};std::strcpy(v->chCurValue,"CAM");return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_SetImageNodeNum(void*,unsigned int n){return n==4?0:MV_E_PARAMETER;}
MV_CAMCTRL_API int __stdcall MV_CC_StartGrabbing(void*){running=1;return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_StopGrabbing(void*){running=0;return 0;}
MV_CAMCTRL_API int __stdcall MV_CC_ClearImageBuffer(void*){return borrowed?MV_E_PARAMETER:0;}
MV_CAMCTRL_API int __stdcall MV_CC_GetImageBuffer(void*,MV_FRAME_OUT* frame,unsigned int timeout){
    if(!running||borrowed||timeout!=37)return MV_E_PARAMETER;
    *frame={};frame->pBufAddr=raster;auto& f=frame->stFrameInfo;
    f.nWidth=3;f.nHeight=2;f.nFrameLen=6;f.enPixelType=PixelType_Gvsp_Mono8;
    f.nFrameNum=0xFFFFFFFFu;f.nDevTimeStampHigh=0x12345678;f.nDevTimeStampLow=0xABCDEF01;
    if(streamMode==1)f.nFrameLen=5;
    if(streamMode==2)f.nLostPacket=1;
    if(streamMode==3)f.nUnparsedChunkNum=1;
    if(streamMode==4)f.enPixelType=PixelType_Gvsp_Mono12_Packed;
    if(streamMode==5)f.nFrameLen=8; // No stride field: never guess padding.
    borrowed=1;return 0;
}
MV_CAMCTRL_API int __stdcall MV_CC_FreeImageBuffer(void*,MV_FRAME_OUT* frame){
    if(!borrowed||frame->pBufAddr!=raster||frame->stFrameInfo.nFrameNum!=0xFFFFFFFFu)return MV_E_PARAMETER;
    borrowed=0;std::memset(raster,99,sizeof(raster));return 0;
}
__declspec(dllexport) void testStreamMode(int mode){streamMode=mode;}
__declspec(dllexport) int testStreamResources(){return borrowed+running;}
__declspec(dllexport) const char* testHandleSerial(void* handle){return reinterpret_cast<const char*>(static_cast<MV_CC_DEVICE_INFO*>(handle)->SpecialInfo.stUsb3VInfo.chSerialNumber);}
__declspec(dllexport) unsigned int testHandleMarker(void* handle){return static_cast<MV_CC_DEVICE_INFO*>(handle)->nMacAddrHigh;}
__declspec(dllexport) int testEnumerationOverlap(){return overlap.load();}
}
