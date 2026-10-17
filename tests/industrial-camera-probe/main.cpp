// Opt-in local ABI experiment. No SDK sources/binaries are distributed with this probe.
// No configuration writes, streaming, trigger, or grab calls are made.
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QFileInfo>
#include <QScopeGuard>
#include <MvCameraControl.h>
#include <cstddef>
#include <iostream>
#include <stdexcept>

template<class T> T symbol(QLibrary& lib, const char* name) {
    auto p = lib.resolve(name);
    if (!p) throw std::runtime_error(std::string("missing export: ") + name);
    return reinterpret_cast<T>(p);
}
QString code(int value) { return QString("0x%1").arg(quint32(value), 8, 16, QChar('0')); }
template<size_t N> QString bounded(const unsigned char (&text)[N]) {
    size_t n=0; while(n<N && text[n]) ++n;
    return QString::fromLatin1(reinterpret_cast<const char*>(text), int(n));
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QJsonObject out{{"schema", "visionflow.backend-experiment1"},
                    {"physicalAcceptance", false}, {"grabAttempted", false}};
    int exitCode=0;
    try {
        if(argc!=3) throw std::runtime_error("usage: industrial_backend_probe --oecs|--gentl absolute-dll-or-cti");
        const QString mode=QString::fromLocal8Bit(argv[1]);
        const auto path=QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
        out["path"]=path;
        if(!QFileInfo(QString::fromLocal8Bit(argv[2])).isAbsolute())
            throw std::runtime_error("explicit absolute library path required");
        QLibrary lib(path);
        if(!lib.load()) { out["status"]="UNAVAILABLE"; throw std::runtime_error(lib.errorString().toStdString()); }
        if(mode=="--gentl") {
            // Presence only; discovery/open is independently recorded by camera_probe.
            QJsonArray exports;
            for(const char* name : {"GCInitLib", "GCCloseLib", "TLOpen", "TLClose",
                "DevGetPort", "GCGetPortURL", "GCGetPortURLInfo", "GCReadPort", "GCWritePort",
                "DevOpenDataStream", "DSClose", "DSAnnounceBuffer", "DSQueueBuffer",
                "DSStartAcquisition", "DSStopAcquisition", "DSRevokeBuffer", "DSFlushQueue",
                "GCRegisterEvent", "GCUnregisterEvent", "EventGetData", "EventKill"}) {
                const bool present=lib.resolve(name)!=nullptr;
                exports.append(QJsonObject{{"name",name},{"present",present}});
                if(!present) exitCode=1;
            }
            out["exports"]=exports;
            out["status"]=exitCode ? "MISSING_EXPORTS" : "EXPORTS_PRESENT_NOT_NODEMAP_ACCEPTANCE";
        } else if(mode=="--oecs") {
#define API(name) const auto name = symbol<decltype(&::name)>(lib, #name)
            API(MV_CC_GetSDKVersion); API(MV_CC_EnumDevices); API(MV_CC_CreateHandleWithoutLog);
            API(MV_CC_OpenDevice); API(MV_CC_CloseDevice); API(MV_CC_DestroyHandle);
            API(MV_CC_GetIntValueEx); API(MV_CC_GetFloatValue); API(MV_CC_GetEnumValue);
            API(MV_CC_GetBoolValue); API(MV_CC_GetStringValue); API(MV_XML_GetGenICamXML);
            API(MV_XML_GetNodeAccessMode); API(MV_XML_GetNodeInterfaceType);
            QJsonArray exports;
            for(const char* name : {"MV_CC_StartGrabbing", "MV_CC_StopGrabbing", "MV_CC_GetImageBuffer",
                "MV_CC_FreeImageBuffer", "MV_CC_SetCommandValue", "MV_CC_SetIntValueEx",
                "MV_CC_SetFloatValue", "MV_CC_SetEnumValue", "MV_CC_SetBoolValue",
                "MV_CC_RegisterEventCallBackEx", "MV_CC_RegisterExceptionCallBack"}) {
                const bool present=lib.resolve(name)!=nullptr;
                exports.append(QJsonObject{{"name",name},{"present",present}});
                if(!present) exitCode=1;
            }
            out["exports"]=exports;
            out["sdkVersionRaw"]=code(int(MV_CC_GetSDKVersion()));
            out["abi"]=QJsonObject{{"pointer",int(sizeof(void*))},
                {"deviceInfo",int(sizeof(MV_CC_DEVICE_INFO))},
                {"deviceList",int(sizeof(MV_CC_DEVICE_INFO_LIST))},
                {"listPointerOffset",int(offsetof(MV_CC_DEVICE_INFO_LIST,pDeviceInfo))},
                {"frameOut",int(sizeof(MV_FRAME_OUT))}, {"frameInfo",int(sizeof(MV_FRAME_OUT_INFO_EX))},
                {"intValue",int(sizeof(MVCC_INTVALUE_EX))}, {"floatValue",int(sizeof(MVCC_FLOATVALUE))}};
            MV_CC_DEVICE_INFO_LIST list{};
            const auto result=MV_CC_EnumDevices(MV_USB_DEVICE, &list);
            out["enumCode"]=code(result);
            if(result!=MV_OK) throw std::runtime_error("USB enumeration failed");
            if(list.nDeviceNum>MV_MAX_DEVICE_NUM) throw std::runtime_error("invalid device count");
            out["deviceCount"]=int(list.nDeviceNum);
            QJsonArray devices;
            for(unsigned i=0;i<list.nDeviceNum;++i) {
                auto* info=list.pDeviceInfo[i];
                if(!info || info->nTLayerType!=MV_USB_DEVICE) throw std::runtime_error("invalid USB device info");
                QJsonObject device{{"model",bounded(info->SpecialInfo.stUsb3VInfo.chModelName)},
                    {"serial",bounded(info->SpecialInfo.stUsb3VInfo.chSerialNumber)}};
                // U3V ignores access mode per local SDK header. No preemption mode or writes.
                void* handle=nullptr;
                auto created=MV_CC_CreateHandleWithoutLog(&handle, info);
                device["createCode"]=code(created);
                bool opened=false;
                auto cleanup=qScopeGuard([&] {
                    if(opened) MV_CC_CloseDevice(handle);
                    if(handle) MV_CC_DestroyHandle(handle);
                });
                if(created==MV_OK && handle) {
                    auto open=MV_CC_OpenDevice(handle, MV_ACCESS_Exclusive, 0);
                    device["openCode"]=code(open); opened=open==MV_OK;
                    if(opened) {
                        QJsonArray features;
                        for(const char* name : {"Width", "Height", "OffsetX", "OffsetY", "PayloadSize"}) {
                            MVCC_INTVALUE_EX value{}; auto rc=MV_CC_GetIntValueEx(handle,name,&value);
                            features.append(QJsonObject{{"name",name},{"type","integer"},{"code",code(rc)},
                                {"value",rc==MV_OK?QJsonValue(QString::number(value.nCurValue)):QJsonValue()}});
                        }
                        for(const char* name : {"ExposureTime", "Gain"}) {
                            MVCC_FLOATVALUE value{}; auto rc=MV_CC_GetFloatValue(handle,name,&value);
                            features.append(QJsonObject{{"name",name},{"type","float"},{"code",code(rc)},
                                {"value",rc==MV_OK?QJsonValue(value.fCurValue):QJsonValue()}});
                        }
                        for(const char* name : {"PixelFormat", "TriggerMode", "TriggerSource", "ExposureAuto", "GainAuto"}) {
                            MVCC_ENUMVALUE value{}; auto rc=MV_CC_GetEnumValue(handle,name,&value);
                            features.append(QJsonObject{{"name",name},{"type","enum"},{"code",code(rc)},
                                {"value",rc==MV_OK?QJsonValue(double(value.nCurValue)):QJsonValue()}});
                        }
                        device["features"]=features;
                        unsigned length=0;
                        device["xmlSizeCode"]=code(MV_XML_GetGenICamXML(handle,nullptr,0,&length));
                        device["xmlRequiredBytes"]=double(length);
                        auto closed=MV_CC_CloseDevice(handle);
                        device["closeCode"]=code(closed);
                        if(closed==MV_OK) opened=false;
                    } else exitCode=1;
                    const auto destroyed=MV_CC_DestroyHandle(handle);
                    device["destroyCode"]=code(destroyed);
                    if(destroyed==MV_OK) { handle=nullptr; opened=false; }
                    if(device.value("destroyCode")!=code(MV_OK) ||
                       (device.contains("closeCode") && device.value("closeCode")!=code(MV_OK))) exitCode=1;
                } else exitCode=1;
                devices.append(device);
            }
            out["devices"]=devices;
            out["status"]=exitCode?"DEVICE_CHECK_FAILED":(list.nDeviceNum==0?"NO_DEVICES":"READBACK_RECORDED_REQUIRES_REVIEW");
        } else throw std::runtime_error("unknown mode");
    } catch(const std::exception& error) {
        if(!out.contains("status")) out["status"]="ERROR";
        out["error"]=error.what(); exitCode=1;
    }
    out["exitCode"]=exitCode;
    std::cout << QJsonDocument(out).toJson().constData();
    return exitCode;
}
