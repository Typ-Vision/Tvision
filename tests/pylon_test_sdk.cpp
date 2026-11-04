#include "hardware/PylonHeaders.hpp"
#include <cstring>
#include <string>
namespace {
int mode=0,initialized=0,handles=0,opened=0,selected=-1;
std::string lastError="fixture vendor error",node;
GENAPIC_RESULT fail(){return GENAPI_E_FAIL;}
GENAPIC_RESULT str(const std::string& value,char* b,size_t* n){
    const auto size=value.size()+1;if(!b){*n=size;return GENAPI_E_OK;}
    if(*n<size){*n=size;return GENAPI_E_INSUFFICIENT_BUFFER;}
    std::memcpy(b,value.c_str(),size);*n=size;return GENAPI_E_OK;
}
void info(PylonDeviceInfo_t* d,bool target){
    std::memset(d,0,sizeof(*d));std::strcpy(d->VendorName,"Basler");std::strcpy(d->ModelName,"acA1920-40um");
    std::strcpy(d->SerialNumber,target?"23102780":"OTHER");std::strcpy(d->DeviceClass,mode==13?"BaslerGigE":"BaslerUsb");std::strcpy(d->DeviceVersion,"fixture-fw");
}
bool integer(){return node=="Width" || node=="Height" || node=="GainRaw";}
bool floating(){return node=="ExposureTime";}
bool enumeration(){return node=="PixelFormat";}
}
extern "C" __declspec(dllexport) void fixtureMode(int v){mode=v;}
extern "C" __declspec(dllexport) int fixtureResources(){return initialized+handles+opened;}
extern "C" __declspec(dllexport) int fixtureSelected(){return selected;}
extern "C" {
GENAPIC_RESULT __stdcall PylonInitialize(){if(mode==11)return fail();++initialized;return 0;}
GENAPIC_RESULT __stdcall PylonTerminate(_Bool logging){if(mode==10 || !logging)return fail();--initialized;return 0;}
GENAPIC_RESULT __stdcall PylonEnumerateDevices(size_t* n){if(mode==1)return fail();*n=mode==7?0:2;return 0;}
GENAPIC_RESULT __stdcall PylonGetDeviceInfo(size_t i,PylonDeviceInfo_t* d){info(d,i==1 || mode==6);return 0;}
GENAPIC_RESULT __stdcall PylonCreateDeviceByIndex(size_t i,PYLON_DEVICE_HANDLE* h){
    if(mode==2)return fail();if(mode==12){*h=nullptr;return 0;}
    selected=int(i);*h=reinterpret_cast<PYLON_DEVICE_HANDLE>(new int(17));++handles;return 0;
}
GENAPIC_RESULT __stdcall PylonDestroyDevice(PYLON_DEVICE_HANDLE h){if(mode==9)return fail();delete reinterpret_cast<int*>(h);--handles;return 0;}
GENAPIC_RESULT __stdcall PylonDeviceOpen(PYLON_DEVICE_HANDLE,int access){
    if(mode==3 || access!=(PYLONC_ACCESS_MODE_CONTROL|PYLONC_ACCESS_MODE_EXCLUSIVE))return fail();++opened;return 0;
}
GENAPIC_RESULT __stdcall PylonDeviceClose(PYLON_DEVICE_HANDLE){if(mode==8)return fail();--opened;return 0;}
GENAPIC_RESULT __stdcall PylonDeviceGetDeviceInfo(PYLON_DEVICE_HANDLE,PylonDeviceInfo_t* d){info(d,mode!=4);return 0;}
GENAPIC_RESULT __stdcall PylonDeviceGetNodeMap(PYLON_DEVICE_HANDLE,NODEMAP_HANDLE* h){*h=reinterpret_cast<NODEMAP_HANDLE>(1);return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiGetLastErrorDetail(char* b,size_t* n){return str(lastError,b,n);}
GENAPIC_RESULT GENAPIC_CC GenApiNodeMapGetNode(NODEMAP_HANDLE,const char* name,NODE_HANDLE* h){
    if(mode==5)return fail();node=name;
    if(!integer() && !floating() && !enumeration() && node!="DeviceVendorName" && node!="DeviceModelName" && node!="DeviceSerialNumber" && node!="DeviceFirmwareVersion" && node!="AcquisitionAbort")return GENAPI_E_NODE_NOT_FOUND;
    *h=reinterpret_cast<NODE_HANDLE>(1);return 0;
}
GENAPIC_RESULT GENAPIC_CC GenApiNodeGetAccessMode(NODE_HANDLE,EGenApiAccessMode* a){*a=node=="AcquisitionAbort"?WO:RW;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiNodeGetType(NODE_HANDLE,EGenApiNodeType* t){*t=integer()?IntegerNode:floating()?FloatNode:enumeration()?EnumerationNode:node=="AcquisitionAbort"?CommandNode:StringNode;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiNodeToString(NODE_HANDLE,char* b,size_t* n){
    if(mode==15 && !b){*n=0;return 0;}
    return str(node=="DeviceVendorName"?"Basler":node=="DeviceModelName"?"acA1920-40um":node=="DeviceSerialNumber"?(mode==14?"WRONG":"23102780"):
        integer()?"1920":floating()?"123.5":enumeration()?"Mono8":"fixture-fw",b,n);
}
GENAPIC_RESULT GENAPIC_CC GenApiIntegerGetMin(NODE_HANDLE,int64_t* v){*v=1;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiIntegerGetMax(NODE_HANDLE,int64_t* v){*v=1936;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiIntegerGetInc(NODE_HANDLE,int64_t* v){*v=2;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiFloatGetMin(NODE_HANDLE,double* v){*v=10;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiFloatGetMax(NODE_HANDLE,double* v){*v=100000;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiFloatGetUnit(NODE_HANDLE,char* b,size_t* n){return str("us",b,n);}
GENAPIC_RESULT GENAPIC_CC GenApiEnumerationGetNumEntries(NODE_HANDLE,size_t* n){*n=1;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiEnumerationGetEntryByIndex(NODE_HANDLE,size_t,NODE_HANDLE* h){*h=reinterpret_cast<NODE_HANDLE>(2);return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiEnumerationEntryGetSymbolic(NODE_HANDLE,char* b,size_t* n){return str("Mono8",b,n);}
}
