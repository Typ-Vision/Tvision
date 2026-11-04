#include "PylonSession.hpp"
#include "RuntimeLibrary.hpp"
#include <QLibrary>
#include <QJsonArray>
#include <QDebug>
#include <atomic>
#include <thread>
#include <stdexcept>
#ifdef VISIONFLOW_WITH_PYLON_SDK
#include "PylonHeaders.hpp"
#include <cstddef>
#include <type_traits>
// Only the ABI actually crossing this read-only boundary is admitted.
static_assert(sizeof(void*)==8 && sizeof(size_t)==8 && sizeof(GENAPIC_RESULT)==4);
static_assert(sizeof(_Bool)==1 && sizeof(EGenApiNodeType)==4 && sizeof(EGenApiAccessMode)==4);
static_assert(PYLONC_PACKING==8 && GENAPIC_PACKING==8 && FILENAME_MAX==260);
static_assert(sizeof(PylonDeviceInfo_t)==708 && alignof(PylonDeviceInfo_t)==1);
static_assert(offsetof(PylonDeviceInfo_t,VendorName)==324 && offsetof(PylonDeviceInfo_t,SerialNumber)==452);
static_assert(std::is_same_v<decltype(&PylonDeviceOpen),GENAPIC_RESULT(__stdcall*)(PYLON_DEVICE_HANDLE,int)>);
#endif

namespace visionflow {
namespace {
void require(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
#ifdef VISIONFLOW_WITH_PYLON_SDK
std::atomic_flag sdkBusy=ATOMIC_FLAG_INIT;
template<size_t N> QString text(const char (&s)[N]){
    size_t n=0;while(n<N && s[n])++n;
    require(n<N,"Unterminated pylon identity");return QString::fromUtf8(s,int(n));
}
QJsonObject identity(const PylonDeviceInfo_t& d){return {{"vendor",text(d.VendorName)},{"model",text(d.ModelName)},
    {"serial",text(d.SerialNumber)},{"deviceClass",text(d.DeviceClass)},{"deviceVersion",text(d.DeviceVersion)}};}
bool matches(const QJsonObject& d,const PylonSelector& s){return d["vendor"]==s.vendor && d["model"]==s.model && d["serial"]==s.serial && d["deviceClass"]=="BaslerUsb";}
#define PYLON_FUNCTIONS(X) \
    X(PylonInitialize) X(PylonTerminate) X(PylonEnumerateDevices) X(PylonGetDeviceInfo) \
    X(PylonCreateDeviceByIndex) X(PylonDestroyDevice) X(PylonDeviceOpen) X(PylonDeviceClose) \
    X(PylonDeviceGetDeviceInfo) X(PylonDeviceGetNodeMap) X(GenApiGetLastErrorDetail) \
    X(GenApiNodeMapGetNode) X(GenApiNodeGetAccessMode) X(GenApiNodeGetType) X(GenApiNodeToString) X(GenApiNodeFromString) X(GenApiBooleanGetValue) \
    X(GenApiIntegerGetMin) X(GenApiIntegerGetMax) X(GenApiIntegerGetInc) \
    X(GenApiFloatGetMin) X(GenApiFloatGetMax) X(GenApiFloatGetUnit) \
    X(GenApiEnumerationGetNumEntries) X(GenApiEnumerationGetEntryByIndex) X(GenApiEnumerationEntryGetSymbolic)
#endif
}
struct PylonSession::Impl {
    std::thread::id owner=std::this_thread::get_id();
#ifdef VISIONFLOW_WITH_PYLON_SDK
    bool claimed=false;
    RuntimeLibrary runtime;
    QLibrary library;
    bool initialized=false,opened=false;
    PYLON_DEVICE_HANDLE device=nullptr;
#define DECLARE(name) decltype(&::name) name=nullptr;
    PYLON_FUNCTIONS(DECLARE)
#undef DECLARE
    void check(GENAPIC_RESULT result,const char* stage){
        if(result==GENAPI_E_OK)return;
        char buffer[8192]{};size_t size=sizeof(buffer);
        const auto detail=GenApiGetLastErrorDetail(buffer,&size);
        throw std::runtime_error(QString("%1: pylon rc=0x%2: %3").arg(stage).arg(quint32(result),8,16,QChar('0'))
            .arg(detail==GENAPI_E_OK?QString::fromUtf8(buffer,int(qstrnlen(buffer,sizeof(buffer)))):QString("error detail unavailable")).toStdString());
    }
    template<class F> QString string(F&& get,const char* stage){
        size_t size=0;check(get(nullptr,&size),stage);
        require(size>0 && size<=1024*1024,"Invalid pylon string length");
        QByteArray buffer(qsizetype(size),'\0');check(get(buffer.data(),&size),stage);
        require(size>0 && size<=size_t(buffer.size()) && buffer[int(size)-1]=='\0',"Invalid pylon string result");
        return QString::fromUtf8(buffer.constData(),int(size)-1);
    }
    QJsonObject node(NODEMAP_HANDLE map,const char* name){
        QJsonObject out{{"name",name}};NODE_HANDLE h=nullptr;
        const auto result=GenApiNodeMapGetNode(map,name,&h);
        if(result==GENAPI_E_NODE_NOT_FOUND || (result==GENAPI_E_OK && !h)){out["access"]="missing";return out;}
        check(result,"GenApiNodeMapGetNode");
        EGenApiAccessMode access;EGenApiNodeType type;
        check(GenApiNodeGetAccessMode(h,&access),"GenApiNodeGetAccessMode");
        check(GenApiNodeGetType(h,&type),"GenApiNodeGetType");
        const char* types[]={"integer","boolean","float","command","string","enumeration","enumEntry","category"};
        const char* accesses[]={"NI","NA","WO","RO","RW"};
        require(type>=IntegerNode && type<=Category && access>=NI && access<=RW,"Invalid pylon node metadata");
        out["type"]=types[int(type)];out["access"]=accesses[int(access)];
        if(access!=RO && access!=RW)return out;
        out["value"]=string([&](char* b,size_t* n){return GenApiNodeToString(h,b,n);},"GenApiNodeToString");
        if(type==IntegerNode){
            int64_t min,max,inc;check(GenApiIntegerGetMin(h,&min),"GenApiIntegerGetMin");
            check(GenApiIntegerGetMax(h,&max),"GenApiIntegerGetMax");check(GenApiIntegerGetInc(h,&inc),"GenApiIntegerGetInc");
            out["min"]=QString::number(min);out["max"]=QString::number(max);out["increment"]=QString::number(inc);
        }else if(type==FloatNode){
            double min,max;check(GenApiFloatGetMin(h,&min),"GenApiFloatGetMin");check(GenApiFloatGetMax(h,&max),"GenApiFloatGetMax");
            out["min"]=min;out["max"]=max;
            out["unit"]=string([&](char* b,size_t* n){return GenApiFloatGetUnit(h,b,n);},"GenApiFloatGetUnit");
        }else if(type==EnumerationNode){
            size_t count=0;check(GenApiEnumerationGetNumEntries(h,&count),"GenApiEnumerationGetNumEntries");
            require(count<=4096,"Invalid pylon enum size");QJsonArray entries;
            for(size_t i=0;i<count;++i){NODE_HANDLE entry=nullptr;check(GenApiEnumerationGetEntryByIndex(h,i,&entry),"GenApiEnumerationGetEntryByIndex");
                EGenApiAccessMode a;check(GenApiNodeGetAccessMode(entry,&a),"GenApiNodeGetAccessMode");
                entries.append(QJsonObject{{"symbol",string([&](char* b,size_t* n){return GenApiEnumerationEntryGetSymbolic(entry,b,n);},"GenApiEnumerationEntryGetSymbolic")},{"available",a==RO || a==RW}});
            }out["entries"]=entries;
        }
        return out;
    }
#endif
};
PylonSession::PylonSession():impl_(std::make_unique<Impl>()){}
PylonSession::~PylonSession(){
    if(!impl_)return;
    try{close();}catch(const std::exception& e){qCritical()<<"Pylon cleanup failed; runtime/session pinned:"<<e.what();impl_.release();}
}
bool PylonSession::available(){
#ifdef VISIONFLOW_WITH_PYLON_SDK
    return true;
#else
    return false;
#endif
}
QJsonObject PylonSession::abi(){
#ifdef VISIONFLOW_WITH_PYLON_SDK
    return {{"headerVersion",PYLON_C_VERSIONSTRING},{"pointerBytes",int(sizeof(void*))},{"resultBytes",int(sizeof(GENAPIC_RESULT))},
        {"boolBytes",int(sizeof(_Bool))},{"deviceInfoBytes",int(sizeof(PylonDeviceInfo_t))},{"packing",PYLONC_PACKING},{"callingConvention","header __stdcall; Windows x64"}};
#else
    return {{"available",false}};
#endif
}
QJsonObject PylonSession::open(const QString& path,const QStringList& dependencies,const PylonSelector& selector){
    auto& p=*impl_;require(p.owner==std::this_thread::get_id(),"Pylon owner thread required");
    require(!selector.vendor.trimmed().isEmpty() && !selector.model.trimmed().isEmpty() && !selector.serial.trimmed().isEmpty(),"Exact vendor/model/serial required");
#ifdef VISIONFLOW_WITH_PYLON_SDK
    require(!p.claimed,"Pylon session already active");
    require(!sdkBusy.test_and_set(),"Pylon SDK busy; close the active native session");p.claimed=true;
    try{
        QString error;require(p.runtime.load(path,dependencies,error),error);
        p.library.setFileName(path);require(p.library.load(),p.library.errorString());
#define BIND(name) p.name=reinterpret_cast<decltype(p.name)>(p.library.resolve(#name));require(p.name!=nullptr,"Missing pylon export: " #name);
        PYLON_FUNCTIONS(BIND)
#undef BIND
        p.check(p.PylonInitialize(),"PylonInitialize");p.initialized=true;
        size_t count=0;p.check(p.PylonEnumerateDevices(&count),"PylonEnumerateDevices");require(count<=4096,"Invalid pylon device count");
        QJsonArray devices;size_t selected=0,matchesCount=0;
        for(size_t i=0;i<count;++i){PylonDeviceInfo_t d{};p.check(p.PylonGetDeviceInfo(i,&d),"PylonGetDeviceInfo");
            auto id=identity(d);devices.append(id);if(matches(id,selector)){selected=i;++matchesCount;}}
        require(matchesCount==1,matchesCount?"Ambiguous exact pylon identity":"Exact pylon USB camera not found");
        // Enumeration is kept alive and serialized through creation; index is never a fallback.
        p.check(p.PylonCreateDeviceByIndex(selected,&p.device),"PylonCreateDeviceByIndex");require(p.device!=nullptr,"Null pylon device handle");
        p.check(p.PylonDeviceOpen(p.device,PYLONC_ACCESS_MODE_CONTROL|PYLONC_ACCESS_MODE_EXCLUSIVE),"PylonDeviceOpen (exclusive/busy)");p.opened=true;
        PylonDeviceInfo_t actual{};p.check(p.PylonDeviceGetDeviceInfo(p.device,&actual),"PylonDeviceGetDeviceInfo");
        auto actualId=identity(actual);require(matches(actualId,selector),"Pylon identity mismatch after open");
        NODEMAP_HANDLE map=nullptr;p.check(p.PylonDeviceGetNodeMap(p.device,&map),"PylonDeviceGetNodeMap");require(map!=nullptr,"Null pylon node map");
        QJsonArray nodes;
        for(const char* name:{"DeviceVendorName","DeviceModelName","DeviceSerialNumber","DeviceID","DeviceFirmwareVersion","DeviceVersion",
            "Width","Height","OffsetX","OffsetY","WidthMax","HeightMax","SensorWidth","SensorHeight","PixelFormat",
            "BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical","ReverseX","ReverseY",
            "ExposureTime","ExposureTimeAbs","ExposureAuto","Gain","GainRaw","GainAbs","GainAuto",
            "TriggerSelector","TriggerMode","TriggerSource","AcquisitionStatusSelector","AcquisitionStatus","SoftwareTriggerIsBusy",
            "CounterSelector","CounterValue","ChunkModeActive","ChunkSelector","ChunkFramecounter","ChunkFrameID","TimestampLatchValue",
            "AcquisitionAbort","AcquisitionStop","DeviceReset"})nodes.append(p.node(map,name));
        // Verify remote identity too; feature naming is observed, never inferred from another model.
        for(const auto pair:{std::pair{"DeviceVendorName",selector.vendor},std::pair{"DeviceModelName",selector.model}}){
            const auto n=p.node(map,pair.first);require(n["value"]==pair.second,"Pylon remote identity mismatch");}
        auto serial=p.node(map,"DeviceSerialNumber");if(!serial.contains("value"))serial=p.node(map,"DeviceID");
        require(serial["value"]==selector.serial,"Pylon remote serial mismatch");
        return {{"identity",actualId},{"enumerated",devices},{"nodes",nodes},{"abi",abi()},{"exclusive",true},{"featureWrites",false},{"acquisitionImplemented",false}};
    }catch(...){const auto failure=std::current_exception();close();std::rethrow_exception(failure);}
#else
    Q_UNUSED(path);Q_UNUSED(dependencies);throw std::runtime_error("Pylon C binding unavailable; configure VISIONFLOW_PYLON_INCLUDE_DIR");
#endif
}
CameraFeature PylonSession::feature(const QString& name) const {
    auto& p=*impl_;require(p.owner==std::this_thread::get_id(),"Pylon owner thread required");
    CameraFeature f;f.name=name;
#ifdef VISIONFLOW_WITH_PYLON_SDK
    require(p.opened,"Pylon session not open");NODEMAP_HANDLE map=nullptr;
    p.check(p.PylonDeviceGetNodeMap(p.device,&map),"PylonDeviceGetNodeMap");
    const auto n=p.node(map,name.toUtf8().constData());const auto a=n["access"].toString();
    f.access=a=="missing"?FeatureAccess::Missing:a=="RW"?FeatureAccess::ReadWrite:a=="RO"?FeatureAccess::ReadOnly:
        a=="WO"?FeatureAccess::WriteOnly:FeatureAccess::Unavailable;
    const auto type=n["type"].toString(),value=n["value"].toString();
    f.type=type=="integer"?FeatureType::Integer:type=="float"?FeatureType::Float:type=="boolean"?FeatureType::Boolean:
        type=="enumeration"?FeatureType::Enumeration:type=="string"?FeatureType::String:FeatureType::Unsupported;
    if(!f.readable())return f;
    bool ok=true;
    if(f.type==FeatureType::Integer){f.value=value.toLongLong(&ok);f.minimum=n["min"].toString().toDouble();f.maximum=n["max"].toString().toDouble();f.increment=n["increment"].toString().toDouble();}
    else if(f.type==FeatureType::Float){f.value=value.toDouble(&ok);f.minimum=n["min"].toDouble();f.maximum=n["max"].toDouble();f.unit=n["unit"].toString();}
    else if(f.type==FeatureType::Boolean){NODE_HANDLE h=nullptr;_Bool actual=false;
        p.check(p.GenApiNodeMapGetNode(map,name.toUtf8().constData(),&h),"GenApiNodeMapGetNode");
        p.check(p.GenApiBooleanGetValue(h,&actual),"GenApiBooleanGetValue");f.value=bool(actual);}
    else f.value=value;
    require(ok,"Invalid pylon numeric readback");
    for(const auto entry:n["entries"].toArray()){const auto e=entry.toObject();if(e["available"].toBool())f.entries.insert(e["symbol"].toString(),quint32(f.entries.size()));}
#else
    throw std::runtime_error("Pylon C binding unavailable");
#endif
    return f;
}
void PylonSession::writeFeature(const QString& name,const FeatureValue& value){
    require(QStringList{"ExposureTime","Gain","ExposureAuto","GainAuto"}.contains(name),"Unsupported pylon basic setting");
    const auto f=feature(name);validateFeatureWrite(f,value);if(featureEquivalent(f,value))return;
#ifdef VISIONFLOW_WITH_PYLON_SDK
    auto& p=*impl_;NODEMAP_HANDLE map=nullptr;NODE_HANDLE node=nullptr;
    p.check(p.PylonDeviceGetNodeMap(p.device,&map),"PylonDeviceGetNodeMap");
    p.check(p.GenApiNodeMapGetNode(map,name.toUtf8().constData(),&node),"GenApiNodeMapGetNode");
    const auto text=std::visit([](const auto& v)->QString{using T=std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T,QString>)return v;
        else if constexpr(std::is_same_v<T,double>)return QString::number(v,'g',17);
        else return QString::number(v);},value).toUtf8();
    p.check(p.GenApiNodeFromString(node,text.constData()),"GenApiNodeFromString");
    require(featureEquivalent(feature(name),value),"Pylon settings readback mismatch");
#endif
}
void PylonSession::close(){
    auto& p=*impl_;require(p.owner==std::this_thread::get_id(),"Pylon owner thread required");
#ifdef VISIONFLOW_WITH_PYLON_SDK
    if(p.opened){p.check(p.PylonDeviceClose(p.device),"PylonDeviceClose");p.opened=false;}
    if(p.device){p.check(p.PylonDestroyDevice(p.device),"PylonDestroyDevice");p.device=nullptr;}
    if(p.initialized){p.check(p.PylonTerminate(true),"PylonTerminate");p.initialized=false;}
    // QLibrary returns false while another binding still references the same DLL.
    if(p.library.isLoaded())p.library.unload();
    p.runtime.unload();if(p.claimed){p.claimed=false;sdkBusy.clear();}
#endif
}
}
