// Manual, exact-device experiment. Raw metadata/arrival observations are NOT
// trigger admission evidence. Never linked to the product or registered in CTest.
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <MvCameraControl.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

template<class T> T resolve(QLibrary& lib,const char* name) {
    const auto p=lib.resolve(name);
    if(!p)throw std::runtime_error(std::string("missing export: ")+name);
    return reinterpret_cast<T>(p);
}
template<size_t N> QString bounded(const unsigned char (&v)[N]) {
    size_t n=0;while(n<N && v[n])++n;
    return QString::fromLatin1(reinterpret_cast<const char*>(v),qsizetype(n));
}
QString code(int v){return QString("0x%1").arg(quint32(v),8,16,QChar('0'));}
void check(int v,const char* stage){if(v!=MV_OK)throw std::runtime_error((QString(stage)+":"+code(v)).toStdString());}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    QJsonObject out{{"schema","visionflow.trigger-diagnostic1"},{"productionAdmission",false},
        {"readyProven",false},{"counterEpochProven",false},{"settingsRestored",false},
        {"commandsAttempted",0},{"framesReceived",0}};
    int exitCode=0;
    try {
        const bool frameSpec=argc==6 && QString::fromLocal8Bit(argv[5])=="--frame-spec";
        if((argc!=5 && !frameSpec) || !QFileInfo(QString::fromLocal8Bit(argv[1])).isAbsolute())
            throw std::runtime_error("usage: oecs_trigger_diagnostic absolute-dll exact-vendor exact-model exact-serial [--frame-spec]");
        out["frameSpecExperiment"]=frameSpec;
        QLibrary lib(QString::fromLocal8Bit(argv[1]));
        if(!lib.load())throw std::runtime_error(lib.errorString().toStdString());
#define API(name) const auto name=resolve<decltype(&::name)>(lib,#name)
        API(MV_CC_EnumDevices);API(MV_CC_CreateHandleWithoutLog);API(MV_CC_OpenDevice);
        API(MV_CC_CloseDevice);API(MV_CC_DestroyHandle);API(MV_CC_GetEnumValue);
        API(MV_CC_SetEnumValue);API(MV_CC_GetIntValueEx);API(MV_CC_SetIntValueEx);
        API(MV_CC_GetBoolValue);API(MV_CC_SetBoolValue);API(MV_CC_SetCommandValue);
        API(MV_CC_SetImageNodeNum);API(MV_CC_StartGrabbing);API(MV_CC_StopGrabbing);
        API(MV_CC_GetImageBuffer);API(MV_CC_FreeImageBuffer);API(MV_CC_ClearImageBuffer);
        API(MV_XML_GetNodeAccessMode);
        MV_CC_DEVICE_INFO_LIST list{};check(MV_CC_EnumDevices(MV_USB_DEVICE,&list),"enumerate");
        if(list.nDeviceNum>MV_MAX_DEVICE_NUM)throw std::runtime_error("invalid device count");
        std::vector<MV_CC_DEVICE_INFO> matches;
        for(unsigned n=0;n<list.nDeviceNum;++n) {
            const auto* info=list.pDeviceInfo[n];if(!info || info->nTLayerType!=MV_USB_DEVICE)continue;
            const auto& usb=info->SpecialInfo.stUsb3VInfo;
            if(bounded(usb.chVendorName)==QString::fromLocal8Bit(argv[2]) &&
               bounded(usb.chModelName)==QString::fromLocal8Bit(argv[3]) &&
               bounded(usb.chSerialNumber)==QString::fromLocal8Bit(argv[4])) matches.push_back(*info);
        }
        if(matches.size()!=1)throw std::runtime_error("exact device absent or ambiguous");
        out["vendor"]=QString::fromLocal8Bit(argv[2]);out["model"]=QString::fromLocal8Bit(argv[3]);
        out["serial"]=QString::fromLocal8Bit(argv[4]);
        void* handle=nullptr;bool opened=false,stream=false,held=false,modified=false,snapshot=false;
        unsigned originalMode=0,originalSource=0;int64_t originalBurst=0;bool originalCache=false;
        unsigned originalSpecSelector=0;
        bool specSelectorSaved=false,specTouched=false;
        std::vector<std::pair<unsigned,bool>> specSnapshot;
        MV_FRAME_OUT frame{}; // One borrowed slot survives every exceptional path.
        QJsonArray samples,cleanup;
        const auto enumValue=[&](const char* name){MVCC_ENUMVALUE v{};check(MV_CC_GetEnumValue(handle,name,&v),name);return v;};
        const auto intValue=[&](const char* name){MVCC_INTVALUE_EX v{};check(MV_CC_GetIntValueEx(handle,name,&v),name);return v.nCurValue;};
        const auto boolValue=[&](const char* name){bool v=false;check(MV_CC_GetBoolValue(handle,name,&v),name);return v;};
        try {
            check(MV_CC_CreateHandleWithoutLog(&handle,&matches.front()),"create");
            check(MV_CC_OpenDevice(handle,MV_ACCESS_Exclusive,0),"open");opened=true;
            const auto selector=enumValue("TriggerSelector");
            if(selector.nCurValue!=6 || selector.nSupportedNum!=1 || selector.nSupportValue[0]!=6)
                throw std::runtime_error("experiment requires sole FrameBurstStart bank");
            originalMode=enumValue("TriggerMode").nCurValue;
            originalSource=enumValue("TriggerSource").nCurValue;
            originalBurst=intValue("AcquisitionBurstFrameCount");originalCache=boolValue("TriggerCacheEnable");
            out["originalSettings"]=QJsonObject{{"mode",double(originalMode)},{"source",double(originalSource)},
                {"burst",QString::number(originalBurst)},{"cache",originalCache},{"selector",6}};
            if(originalMode!=0 || enumValue("AcquisitionMode").nCurValue!=2 ||
               enumValue("PixelFormat").nCurValue!=0x01080001)
                throw std::runtime_error("requires trigger Off, Continuous, Mono8");
            const auto width=intValue("Width"),height=intValue("Height"),payload=intValue("PayloadSize");
            if(width<=0 || height<=0 || width>2448 || height>2048 || payload!=width*height)
                throw std::runtime_error("requires bounded tight Mono8 payload");
            if(frameSpec) {
                const auto selector=enumValue("FrameSpecInfoSelector");
                originalSpecSelector=selector.nCurValue;specSelectorSaved=true;
                for(unsigned bank:{0u,5u,6u}) {
                    bool supported=false;
                    for(unsigned n=0;n<selector.nSupportedNum && n<MV_MAX_XML_SYMBOLIC_NUM;++n)
                        supported|=selector.nSupportValue[n]==bank;
                    if(!supported)throw std::runtime_error("required frame spec bank unsupported");
                }
                // Snapshot every target bank before changing its enable value.
                for(unsigned bank:{0u,5u,6u}) {
                    check(MV_CC_SetEnumValue(handle,"FrameSpecInfoSelector",bank),"select frame spec snapshot");
                    if(enumValue("FrameSpecInfoSelector").nCurValue!=bank)throw std::runtime_error("frame spec selector mismatch");
                    specSnapshot.emplace_back(bank,boolValue("FrameSpecInfo"));
                }
                QJsonArray saved;
                for(const auto& [bank,enabled]:specSnapshot) {
                    saved.append(QJsonObject{{"bank",double(bank)},{"enabled",enabled}});
                    check(MV_CC_SetEnumValue(handle,"FrameSpecInfoSelector",bank),"select frame spec enable");
                    if(enumValue("FrameSpecInfoSelector").nCurValue!=bank)throw std::runtime_error("frame spec enable selector mismatch");
                    specTouched=true;
                    check(MV_CC_SetBoolValue(handle,"FrameSpecInfo",true),"enable frame spec");
                    if(!boolValue("FrameSpecInfo"))throw std::runtime_error("frame spec enable mismatch");
                }
                out["frameSpecSnapshot"]=saved;
                check(MV_CC_SetEnumValue(handle,"FrameSpecInfoSelector",originalSpecSelector),"restore active frame spec selector");
                if(enumValue("FrameSpecInfoSelector").nCurValue!=originalSpecSelector)throw std::runtime_error("active frame spec selector mismatch");
                if(intValue("PayloadSize")!=payload)throw std::runtime_error("frame spec changed payload; raster contract unsupported");
            }
            snapshot=true;modified=true;
            check(MV_CC_SetEnumValue(handle,"TriggerSource",7),"set Software");
            check(MV_CC_SetIntValueEx(handle,"AcquisitionBurstFrameCount",1),"set burst 1");
            check(MV_CC_SetBoolValue(handle,"TriggerCacheEnable",false),"disable cache");
            check(MV_CC_SetEnumValue(handle,"TriggerMode",1),"set trigger On");
            if(enumValue("TriggerSource").nCurValue!=7 || enumValue("TriggerMode").nCurValue!=1 ||
               intValue("AcquisitionBurstFrameCount")!=1 || boolValue("TriggerCacheEnable"))
                throw std::runtime_error("configuration readback mismatch");
            check(MV_CC_SetImageNodeNum(handle,4),"pool 4");
            check(MV_CC_StartGrabbing(handle),"start");stream=true;
            check(MV_CC_ClearImageBuffer(handle),"initial clear");
            QJsonArray armedAccess;
            for(const char* name:{"AcquisitionStatusSelector","AcquisitionStatus","TriggerSoftware"}) {
                MV_XML_AccessMode access=AM_Undefined;
                const auto rc=MV_XML_GetNodeAccessMode(handle,name,&access);
                QJsonObject node{{"name",name},{"code",code(rc)}};
                if(rc==MV_OK)node["accessModeRaw"]=int(access);
                armedAccess.append(node);
            }
            out["armedNodeAccess"]=armedAccess;
            const auto requireQuiet=[&] {
                frame={};
                const int rc=MV_CC_GetImageBuffer(handle,&frame,50);
                if(rc==MV_OK){held=true;throw std::runtime_error("unsolicited/extra frame in quiet window");}
                if(rc!=int(MV_E_NODATA))check(rc,"quiet wait");
            };
            requireQuiet(); // Observation only: timeout is not a ready oracle.
            for(int n=0;n<100;++n) {
                QElapsedTimer timer;timer.start();out["commandsAttempted"]=n+1;
                check(MV_CC_SetCommandValue(handle,"TriggerSoftware"),"command");
                const auto left=1000-timer.elapsed();
                if(left<=0)throw std::runtime_error("command exceeded diagnostic deadline");
                frame={};check(MV_CC_GetImageBuffer(handle,&frame,unsigned(left)),"trigger wait");held=true;
                const auto& f=frame.stFrameInfo;
                if(timer.elapsed()>=1000 || !frame.pBufAddr || f.nFrameLen!=payload ||
                   f.nWidth!=width || f.nHeight!=height || unsigned(f.enPixelType)!=0x01080001)
                    throw std::runtime_error("late or invalid diagnostic frame");
                samples.append(QJsonObject{{"sample",n+1},{"frameNumber",double(f.nFrameNum)},
                    {"frameCounter",double(f.nFrameCounter)},{"triggerIndex",double(f.nTriggerIndex)},
                    {"deviceTicks",QString::number((quint64(f.nDevTimeStampHigh)<<32)|f.nDevTimeStampLow)},
                    {"unparsedChunks",double(f.nUnparsedChunkNum)},{"elapsedMs",double(timer.elapsed())},
                    {"sha256",QString::fromLatin1(QCryptographicHash::hash(
                        QByteArrayView(reinterpret_cast<const char*>(frame.pBufAddr),f.nFrameLen),QCryptographicHash::Sha256).toHex())}});
                if(frameSpec) {
                    auto sample=samples.last().toObject();
                    sample["rasterPrefixHex"]=QString::fromLatin1(QByteArray(
                        reinterpret_cast<const char*>(frame.pBufAddr),int(std::min(f.nFrameLen,64u))).toHex());
                    samples.replace(samples.size()-1,sample);
                }
                check(MV_CC_FreeImageBuffer(handle,&frame),"free");held=false;
                out["framesReceived"]=n+1;requireQuiet();
            }
            out["status"]="OBSERVED_100_COMMAND_FRAME_PAIRS_NOT_ADMISSION";
        } catch(const std::exception& e){out["error"]=e.what();out["status"]="ERROR";exitCode=1;}
        // Attempt every cleanup/restore operation even if an earlier operation failed.
        bool cleanupOk=true;
        const auto record=[&](const char* stage,int rc){cleanup.append(QJsonObject{{"stage",stage},{"code",code(rc)}});if(rc!=MV_OK){cleanupOk=false;exitCode=1;}};
        if(held)record("free outstanding",MV_CC_FreeImageBuffer(handle,&frame));
        if(stream){record("clear",MV_CC_ClearImageBuffer(handle));record("stop",MV_CC_StopGrabbing(handle));}
        if(opened && specSelectorSaved) {
            bool specRestored=true;
            if(specTouched)for(const auto& [bank,enabled]:specSnapshot) {
                const int selected=MV_CC_SetEnumValue(handle,"FrameSpecInfoSelector",bank);
                record("restore spec select",selected);
                if(selected!=MV_OK){specRestored=false;continue;}
                // Never write a bank if selector readback could not be confirmed.
                try {
                    if(enumValue("FrameSpecInfoSelector").nCurValue!=bank)throw std::runtime_error("restore selector mismatch");
                    const int rc=MV_CC_SetBoolValue(handle,"FrameSpecInfo",enabled);record("restore spec enable",rc);
                    if(rc!=MV_OK || boolValue("FrameSpecInfo")!=enabled)specRestored=false;
                }catch(const std::exception& e){out["frameSpecRestoreError"]=e.what();specRestored=false;}
            }
            record("restore spec selector",MV_CC_SetEnumValue(handle,"FrameSpecInfoSelector",originalSpecSelector));
            try{specRestored=enumValue("FrameSpecInfoSelector").nCurValue==originalSpecSelector && specRestored;}
            catch(const std::exception& e){out["frameSpecRestoreError"]=e.what();specRestored=false;}
            out["frameSpecRestored"]=specRestored;
            if(!specRestored){cleanupOk=false;exitCode=1;}
        }
        if(opened && modified && snapshot) {
            record("restore Off",MV_CC_SetEnumValue(handle,"TriggerMode",0));
            record("restore source",MV_CC_SetEnumValue(handle,"TriggerSource",originalSource));
            record("restore burst",MV_CC_SetIntValueEx(handle,"AcquisitionBurstFrameCount",originalBurst));
            record("restore cache",MV_CC_SetBoolValue(handle,"TriggerCacheEnable",originalCache));
            record("restore mode",MV_CC_SetEnumValue(handle,"TriggerMode",originalMode));
            try {
                const bool restored=enumValue("TriggerMode").nCurValue==originalMode &&
                    enumValue("TriggerSource").nCurValue==originalSource &&
                    intValue("AcquisitionBurstFrameCount")==originalBurst && boolValue("TriggerCacheEnable")==originalCache;
                out["settingsRestored"]=restored;if(!restored){cleanupOk=false;exitCode=1;}
            } catch(const std::exception& e){out["restoreError"]=e.what();cleanupOk=false;exitCode=1;}
        }
        if(opened)record("close",MV_CC_CloseDevice(handle));
        if(handle)record("destroy",MV_CC_DestroyHandle(handle));
        out["samples"]=samples;out["cleanup"]=cleanup;
        if(!cleanupOk)out["status"]="CLEANUP_OR_RESTORE_FAILED";
    } catch(const std::exception& e){out["error"]=e.what();out["status"]="ERROR";exitCode=1;}
    out["exitCode"]=exitCode;std::cout<<QJsonDocument(out).toJson().constData();return exitCode;
}
