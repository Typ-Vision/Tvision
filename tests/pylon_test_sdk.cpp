#include "hardware/PylonHeaders.hpp"
#include <cstring>
#include <string>
#include <map>
#include <cstdlib>
#include <limits>
namespace {
int mode=0,initialized=0,handles=0,opened=0,selected=-1;
std::string lastError="fixture vendor error",node;
std::map<std::string,std::string> values;int writes=0;bool failed=false;
bool settings(){return mode>=20;}
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
bool integer(){if(settings())return node=="Width" || node=="Height" || node=="SensorWidth" || node=="SensorHeight" || node=="OffsetX" || node=="OffsetY" || node=="BinningHorizontal" || node=="BinningVertical" || node=="GainRaw";return node=="Width" || node=="Height" || node=="GainRaw";}
bool floating(){return node=="ExposureTime" || (settings() && node=="Gain");}
bool enumeration(){return node=="PixelFormat" || (settings() && (node=="ExposureAuto" || node=="GainAuto" || node=="TriggerMode" || node=="TriggerSelector" || node=="TriggerSource"));}
void* image=nullptr;size_t capacity=0;const void* context=nullptr;
bool streamOpen=false,prepared=false,registered=false,queued=false,acquiring=false,delivered=false,flushing=false;
uint64_t block=0;int triggers=0;
}
extern "C" __declspec(dllexport) void fixtureMode(int v){mode=v;writes=0;failed=false;block=0;triggers=0;delivered=flushing=false;values={{"Width","1920"},{"Height","1200"},{"SensorWidth","1936"},{"SensorHeight","1216"},{"OffsetX","8"},{"OffsetY","8"},{"BinningHorizontal","1"},{"BinningVertical","1"},{"ExposureTime","3000"},{"Gain","0"},{"GainRaw","0"},{"ExposureAuto",(v==30 || v==31)?"Continuous":"Off"},{"GainAuto","Off"},{"TriggerMode","Off"},{"TriggerSelector","FrameStart"},{"TriggerSource","Line1"},{"PixelFormat","Mono8"},{"ReverseX","false"},{"ReverseY","false"}};
    if(v>=40){values["AcquisitionMode"]="Continuous";values["AcquisitionStatusSelector"]="FrameTriggerWait";values["AcquisitionStatus"]="true";values["ChunkModeActive"]=v==55?"true":"false";}}
extern "C" __declspec(dllexport) int fixtureWrites(){return writes;}
extern "C" __declspec(dllexport) int fixtureResources(){return initialized+handles+opened+int(streamOpen)+int(prepared)+int(registered)+int(acquiring);}
extern "C" __declspec(dllexport) int fixtureTriggers(){return triggers;}
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
    if((mode==60 || mode==61) && (node=="DecimationHorizontal" || node=="DecimationVertical")){*h=reinterpret_cast<NODE_HANDLE>(1);return 0;}
    if(settings() && ((mode==21 && node=="Gain") || (mode==29 && node=="Gain") || (mode==27 && (node=="BinningHorizontal" || node=="BinningVertical")) || (mode==28 && node=="BinningVertical")))return GENAPI_E_NODE_NOT_FOUND;
    if(settings() && values.count(node)){*h=reinterpret_cast<NODE_HANDLE>(1);return 0;}
    if(!integer() && !floating() && !enumeration() && node!="DeviceVendorName" && node!="DeviceModelName" && node!="DeviceSerialNumber" && node!="DeviceFirmwareVersion" && node!="AcquisitionAbort")return GENAPI_E_NODE_NOT_FOUND;
    *h=reinterpret_cast<NODE_HANDLE>(1);return 0;
}
GENAPIC_RESULT GENAPIC_CC GenApiNodeGetAccessMode(NODE_HANDLE,EGenApiAccessMode* a){*a=(mode==60 || mode==61) && node.find("Decimation")==0?(mode==60?NI:NA):node=="AcquisitionAbort"?WO:(settings() && ((mode==22 && node=="Gain") || (node=="ExposureTime" && values["ExposureAuto"]!="Off")))?RO:RW;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiNodeGetType(NODE_HANDLE,EGenApiNodeType* t){*t=integer()?IntegerNode:floating()?FloatNode:enumeration()?EnumerationNode:settings() && (node=="ReverseX" || node=="ReverseY" || node=="AcquisitionStatus" || node=="ChunkModeActive")?BooleanNode:node=="AcquisitionAbort"?CommandNode:StringNode;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiNodeToString(NODE_HANDLE,char* b,size_t* n){
    if(settings() && values.count(node))return str(values[node],b,n);
    if(mode==15 && !b){*n=0;return 0;}
    return str(node=="DeviceVendorName"?"Basler":node=="DeviceModelName"?"acA1920-40um":node=="DeviceSerialNumber"?(mode==14?"WRONG":"23102780"):
        integer()?"1920":floating()?"123.5":enumeration()?"Mono8":"fixture-fw",b,n);
}
GENAPIC_RESULT GENAPIC_CC GenApiIntegerGetMin(NODE_HANDLE,int64_t* v){*v=settings() && (node=="OffsetX" || node=="OffsetY" || node=="GainRaw")?0:1;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiIntegerGetMax(NODE_HANDLE,int64_t* v){*v=1936;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiIntegerGetInc(NODE_HANDLE,int64_t* v){*v=settings()?1:2;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiFloatGetMin(NODE_HANDLE,double* v){*v=settings() && node=="Gain"?0:10;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiFloatGetMax(NODE_HANDLE,double* v){*v=settings() && node=="Gain"?36:100000;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiFloatGetUnit(NODE_HANDLE,char* b,size_t* n){return str(settings() && node=="Gain"?(mode==23?"raw":"dB"):"us",b,n);}
GENAPIC_RESULT GENAPIC_CC GenApiBooleanGetValue(NODE_HANDLE,_Bool* value){*value=values[node]=="true" && !(mode==53 && node=="AcquisitionStatus");return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiEnumerationGetNumEntries(NODE_HANDLE,size_t* n){*n=settings() && (node=="ExposureAuto" || node=="GainAuto" || (mode>=40 && (node=="TriggerMode" || node=="TriggerSource")))?2:1;return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiEnumerationGetEntryByIndex(NODE_HANDLE,size_t i,NODE_HANDLE* h){*h=reinterpret_cast<NODE_HANDLE>(2+i);return 0;}
GENAPIC_RESULT GENAPIC_CC GenApiEnumerationEntryGetSymbolic(NODE_HANDLE h,char* b,size_t* n){
    if(mode>=40 && node=="TriggerMode")return str(reinterpret_cast<size_t>(h)==2?"Off":"On",b,n);
    if(mode>=40 && node=="TriggerSource")return str(reinterpret_cast<size_t>(h)==2?"Line1":"Software",b,n);
    return str(settings()?(node=="ExposureAuto" || node=="GainAuto"?(reinterpret_cast<size_t>(h)==2?"Off":"Continuous"):values[node]):"Mono8",b,n);}
}

extern "C" GENAPIC_RESULT GENAPIC_CC GenApiNodeFromString(NODE_HANDLE,const char* value){
    if(!settings())return fail();++writes;
    if((mode==26 || mode==64) && failed)return fail();
    values[node]=value;
    if((mode==25 || mode==26 || mode==31 || mode==63 || mode==64) && node=="Gain" && !failed){failed=true;return fail();}
    if(mode==24 && node=="ExposureTime" && !failed){failed=true;values[node]="3501";}
    return 0;
}

extern "C" {
GENAPIC_RESULT __stdcall PylonDeviceFeatureFromString(PYLON_DEVICE_HANDLE,const char* name,const char* value){values[name]=value;return 0;}
GENAPIC_RESULT __stdcall PylonDeviceGetStreamGrabber(PYLON_DEVICE_HANDLE,size_t,PYLON_STREAMGRABBER_HANDLE* h){*h=reinterpret_cast<PYLON_STREAMGRABBER_HANDLE>(3);return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberOpen(PYLON_STREAMGRABBER_HANDLE){streamOpen=true;return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberClose(PYLON_STREAMGRABBER_HANDLE){if(mode==51)return fail();streamOpen=false;return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberGetWaitObject(PYLON_STREAMGRABBER_HANDLE,PYLON_WAITOBJECT_HANDLE* h){*h=reinterpret_cast<PYLON_WAITOBJECT_HANDLE>(4);return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberSetMaxNumBuffer(PYLON_STREAMGRABBER_HANDLE,size_t n){return n==1?0:fail();}
GENAPIC_RESULT __stdcall PylonStreamGrabberSetMaxBufferSize(PYLON_STREAMGRABBER_HANDLE,size_t){return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberGetPayloadSize(PYLON_DEVICE_HANDLE,PYLON_STREAMGRABBER_HANDLE,size_t* n){*n=1920*1200;return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberPrepareGrab(PYLON_STREAMGRABBER_HANDLE){prepared=true;flushing=false;block=0;return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberFinishGrab(PYLON_STREAMGRABBER_HANDLE){prepared=false;return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberRegisterBuffer(PYLON_STREAMGRABBER_HANDLE,void* p,size_t n,PYLON_STREAMBUFFER_HANDLE* h){image=p;capacity=n;*h=reinterpret_cast<PYLON_STREAMBUFFER_HANDLE>(5);registered=true;return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberDeregisterBuffer(PYLON_STREAMGRABBER_HANDLE,PYLON_STREAMBUFFER_HANDLE){std::memset(image,0xee,capacity);registered=false;return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberQueueBuffer(PYLON_STREAMGRABBER_HANDLE,PYLON_STREAMBUFFER_HANDLE,const void* c){if(mode==54 && delivered)return fail();context=c;queued=true;delivered=false;return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberStartStreamingIfMandatory(PYLON_STREAMGRABBER_HANDLE){return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberStopStreamingIfMandatory(PYLON_STREAMGRABBER_HANDLE){return 0;}
GENAPIC_RESULT __stdcall PylonStreamGrabberFlushBuffersToOutput(PYLON_STREAMGRABBER_HANDLE){flushing=true;return 0;}
GENAPIC_RESULT __stdcall PylonWaitObjectWait(PYLON_WAITOBJECT_HANDLE,uint32_t,_Bool* ready){*ready=mode!=50;return 0;}
GENAPIC_RESULT __stdcall PylonDeviceExecuteCommandFeature(PYLON_DEVICE_HANDLE,const char* name){
    if(std::string(name)=="AcquisitionStart"){acquiring=true;return 0;}
    if(std::string(name)=="AcquisitionStop"){if(mode==59)return fail();acquiring=false;return 0;}
    if(mode==49)return fail();++triggers;delivered=true;return 0;
}
GENAPIC_RESULT __stdcall PylonStreamGrabberRetrieveResult(PYLON_STREAMGRABBER_HANDLE,PylonGrabResult_t* r,_Bool* ready){
    *ready=queued && (flushing || (acquiring && (delivered || values["TriggerMode"]=="Off" || mode==52)));
    if(!*ready)return 0;queued=false;
    std::memset(image,0x2a,capacity);*r={};r->Context=context;r->hBuffer=reinterpret_cast<PYLON_STREAMBUFFER_HANDLE>(5);r->pBuffer=image;
    r->Status=flushing?Canceled:mode==45?Failed:Grabbed;r->PayloadType=PayloadType_Image;r->PixelType=mode==47?PixelType_Mono12packed:PixelType_Mono8;
    r->SizeX=mode==44?1919:1920;r->SizeY=1200;r->OffsetX=mode==56?0:8;r->OffsetY=8;r->PaddingX=mode==57?1:0;
    r->PayloadSize=mode==48?std::numeric_limits<uint64_t>::max():capacity;r->BlockID=mode==43?std::numeric_limits<uint64_t>::max():mode==41?0:mode==42 && block>0?0:block;
    r->TimeStamp=block++;if(mode==46)r->pBuffer=nullptr;if(mode==58)r->Context=nullptr;return 0;
}
}
