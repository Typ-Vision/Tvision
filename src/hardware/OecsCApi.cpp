#include "hardware/OecsCApi.hpp"
#include <QFileInfo>
#include <QLibrary>
#include <mutex>
#include <QXmlStreamReader>
#include <cmath>
#ifdef VISIONFLOW_WITH_OECS_SDK
#include <MvCameraControl.h>
#endif

namespace visionflow {
namespace {
#ifdef VISIONFLOW_WITH_OECS_SDK
// Every in-process SDK enumeration must hold this through the owned copy.
// SDK device-list storage is shared even between separate binding instances.
std::mutex enumerationMutex;
template<class C,size_t N> QString bounded(const C (&text)[N]) {
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
        if(!bind(accessMode_,"MV_XML_GetNodeAccessMode"))return fail("MV_XML_GetNodeAccessMode");
        if(!bind(interfaceType_,"MV_XML_GetNodeInterfaceType"))return fail("MV_XML_GetNodeInterfaceType");
        if(!bind(getXml_,"MV_XML_GetGenICamXML"))return fail("MV_XML_GetGenICamXML");
        if(!bind(getInt_,"MV_CC_GetIntValueEx"))return fail("MV_CC_GetIntValueEx");
        if(!bind(setInt_,"MV_CC_SetIntValueEx"))return fail("MV_CC_SetIntValueEx");
        if(!bind(getFloat_,"MV_CC_GetFloatValue"))return fail("MV_CC_GetFloatValue");
        if(!bind(setFloat_,"MV_CC_SetFloatValue"))return fail("MV_CC_SetFloatValue");
        if(!bind(getEnum_,"MV_CC_GetEnumValue"))return fail("MV_CC_GetEnumValue");
        if(!bind(setEnum_,"MV_CC_SetEnumValue"))return fail("MV_CC_SetEnumValue");
        if(!bind(getBool_,"MV_CC_GetBoolValue"))return fail("MV_CC_GetBoolValue");
        if(!bind(setBool_,"MV_CC_SetBoolValue"))return fail("MV_CC_SetBoolValue");
        if(!bind(getString_,"MV_CC_GetStringValue"))return fail("MV_CC_GetStringValue");
        return true;
    }
    bool unload() override {
        if(liveHandles_>0)return false;
        unbind();
        xmlCache_.clear();
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
        if(status==MV_OK){xmlCache_.remove(handle);if(liveHandles_>0)--liveHandles_;}
        return status;
    }
    int readFeature(void* h,const QString& name,CameraFeature& out) override {
        out={};out.name=name;if(!accessMode_)return oecs::errorLoadLibrary;const auto key=name.toLatin1();
        MV_XML_AccessMode access=AM_Undefined;
        int status=accessMode_(h,key.constData(),&access);if(status!=MV_OK)return status;
        switch(access){case AM_NI:out.access=FeatureAccess::Missing;return 0;case AM_NA:out.access=FeatureAccess::Unavailable;return 0;case AM_WO:out.access=FeatureAccess::WriteOnly;return 0;case AM_RO:out.access=FeatureAccess::ReadOnly;break;case AM_RW:out.access=FeatureAccess::ReadWrite;break;default:return oecs::errorParameter;}
        MV_XML_InterfaceType type=IFT_IBase;status=interfaceType_(h,key.constData(),&type);if(status!=MV_OK)return status;
        if(type==IFT_IInteger){MVCC_INTVALUE_EX v{};status=getInt_(h,key.constData(),&v);if(status!=0)return status;
            // Numeric validation uses doubles: reject lossy ranges rather than round int64 bounds.
            constexpr qint64 exact=9007199254740991LL;
            if(v.nMin < -exact || v.nMax > exact || v.nInc<=0)return oecs::errorParameter;
            out.type=FeatureType::Integer;out.value=qint64(v.nCurValue);out.minimum=double(v.nMin);out.maximum=double(v.nMax);out.increment=double(v.nInc);
        }else if(type==IFT_IFloat){MVCC_FLOATVALUE v{};status=getFloat_(h,key.constData(),&v);if(status!=0)return status;out.type=FeatureType::Float;out.value=double(v.fCurValue);out.minimum=v.fMin;out.maximum=v.fMax;
        }else if(type==IFT_IBoolean){bool v=false;status=getBool_(h,key.constData(),&v);if(status!=0)return status;out.type=FeatureType::Boolean;out.value=v;
        }else if(type==IFT_IString){MVCC_STRINGVALUE v{};status=getString_(h,key.constData(),&v);if(status!=0)return status;out.type=FeatureType::String;out.value=bounded(v.chCurValue);
        }else if(type==IFT_IEnumeration){out.type=FeatureType::Enumeration;MVCC_ENUMVALUE v{};status=getEnum_(h,key.constData(),&v);if(status!=0)return status;
            QMap<QString,QMap<QString,QString>> xml;status=xmlMetadata(h,xml);if(status!=0)return status;
            const auto refs=xml.value(name).value("pEnumEntry").split('\n',Qt::SkipEmptyParts);
            QString current;
            for(const auto& ref:refs){const auto entry=xml.value(ref);bool ok=false;const auto raw=entry.value("Value").toUInt(&ok,0);const auto symbol=entry.value("Symbolic",ref);
                if(!ok||symbol.isEmpty())continue;
                for(unsigned i=0;i<v.nSupportedNum&&i<MV_MAX_XML_SYMBOLIC_NUM;++i)if(v.nSupportValue[i]==raw){out.entries.insert(symbol,raw);if(v.nCurValue==raw)current=symbol;}
            }
            if(current.isEmpty())return oecs::errorParameter;out.value=current;
        }else return oecs::errorParameter;
        if(out.type==FeatureType::Integer || out.type==FeatureType::Float){
            QMap<QString,QMap<QString,QString>> xml;status=xmlMetadata(h,xml);if(status!=0)return status;const auto props=xml.value(name);out.unit=props.value("Unit");
            // Float ABI has no increment. Preserve a literal XML increment if declared.
            if(out.type==FeatureType::Float && props.contains("Inc")){bool ok=false;double inc=props.value("Inc").toDouble(&ok);if(!ok||!std::isfinite(inc)||inc<=0)return oecs::errorParameter;out.increment=inc;}
            if(out.type==FeatureType::Float && props.contains("pInc"))return oecs::errorParameter;
        }
        return 0;
    }
    int writeFeature(void* h,const QString& name,const FeatureValue& value) override {
        // No command/string writes, including UserSetSave, cross this boundary.
        if(!acquisitionFeatureNames().contains(name))return oecs::errorParameter;
        CameraFeature f;const int status=readFeature(h,name,f);if(status!=0)return status;
        try{validateFeatureWrite(f,value);}catch(...){return oecs::errorParameter;}
        if(featureEquivalent(f,value))return 0;
        const auto key=name.toLatin1();
        switch(f.type){case FeatureType::Integer:return setInt_(h,key.constData(),std::get<qint64>(value));case FeatureType::Float:return setFloat_(h,key.constData(),float(std::get<double>(value)));case FeatureType::Boolean:return setBool_(h,key.constData(),std::get<bool>(value));case FeatureType::Enumeration:return setEnum_(h,key.constData(),f.entries.value(std::get<QString>(value)));default:return oecs::errorParameter;}
    }
private:
    int xmlMetadata(void* h,QMap<QString,QMap<QString,QString>>& result){
        if(xmlCache_.contains(h)){result=xmlCache_.value(h);return 0;}
        // Bound allocation even if a faulty device reports an excessive XML size.
        unsigned length=0;getXml_(h,nullptr,0,&length);
        if(length==0||length>8*1024*1024)return oecs::errorParameter;
        QByteArray bytes(qsizetype(length),'\0');unsigned actual=0;int status=getXml_(h,reinterpret_cast<unsigned char*>(bytes.data()),length,&actual);
        if(status!=0)return status;if(actual==0||actual>length)return oecs::errorParameter;bytes.resize(actual);
        while(bytes.endsWith('\0'))bytes.chop(1);
        QXmlStreamReader xml(bytes);QString node;int depth=0,nodeDepth=-1;
        while(!xml.atEnd()){
            xml.readNext();if(xml.isStartElement()){
                ++depth;const auto attr=xml.attributes().value("Name");if(!attr.isEmpty()){node=attr.toString();nodeDepth=depth;}
                else if(!node.isEmpty() && depth==nodeDepth+1){const auto prop=xml.name().toString();const auto value=xml.readElementText();--depth;
                    auto& target=result[node][prop];if(!target.isEmpty())target+='\n';target+=value;}
            }else if(xml.isEndElement()){if(depth==nodeDepth){node.clear();nodeDepth=-1;}--depth;}
        }
        if(xml.hasError())return oecs::errorParameter;
        xmlCache_.insert(h,result);return 0;
    }
    template<class F> bool bind(F& pointer,const char* name){pointer=reinterpret_cast<F>(library_.resolve(name));return pointer!=nullptr;}
    void unbind(){version_=nullptr;enum_=nullptr;create_=nullptr;open_=nullptr;close_=nullptr;destroy_=nullptr;accessMode_=nullptr;interfaceType_=nullptr;getXml_=nullptr;getInt_=nullptr;setInt_=nullptr;getFloat_=nullptr;setFloat_=nullptr;getEnum_=nullptr;setEnum_=nullptr;getBool_=nullptr;setBool_=nullptr;getString_=nullptr;}
    decltype(&::MV_XML_GetNodeAccessMode) accessMode_=nullptr;
    decltype(&::MV_XML_GetNodeInterfaceType) interfaceType_=nullptr;
    decltype(&::MV_XML_GetGenICamXML) getXml_=nullptr;
    decltype(&::MV_CC_GetIntValueEx) getInt_=nullptr;
    decltype(&::MV_CC_SetIntValueEx) setInt_=nullptr;
    decltype(&::MV_CC_GetFloatValue) getFloat_=nullptr;
    decltype(&::MV_CC_SetFloatValue) setFloat_=nullptr;
    decltype(&::MV_CC_GetEnumValue) getEnum_=nullptr;
    decltype(&::MV_CC_SetEnumValue) setEnum_=nullptr;
    decltype(&::MV_CC_GetBoolValue) getBool_=nullptr;
    decltype(&::MV_CC_SetBoolValue) setBool_=nullptr;
    decltype(&::MV_CC_GetStringValue) getString_=nullptr;
    QMap<void*,QMap<QString,QMap<QString,QString>>> xmlCache_;
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
