#include "hardware/OecsSession.hpp"
#include <QDebug>
#include <QFileInfo>
#include <QSet>
#include <mutex>
#include <exception>
#include <atomic>
#include <limits>

namespace visionflow {
namespace {
QString hex(int code){return QString("0x%1").arg(quint32(code),8,16,QChar('0'));}
// Process-local owner registry: one session per exact camera identity.
std::mutex claimMutex;
QSet<QString>& claims(){static QSet<QString> set;return set;}
bool claim(const QString& key){std::lock_guard<std::mutex> lock(claimMutex);if(claims().contains(key))return false;claims().insert(key);return true;}
void release(const QString& key){std::lock_guard<std::mutex> lock(claimMutex);claims().remove(key);}
}
OecsSessionError::OecsSessionError(OecsFailure failure,QString stage,int code,const QString& message):std::runtime_error(message.toStdString()),failure_(failure),stage_(std::move(stage)),code_(code) {}
OecsFailure OecsSessionError::failure() const {return failure_;}
QString OecsSessionError::stage() const {return stage_;}
int OecsSessionError::code() const {return code_;}

OecsSession::OecsSession(std::unique_ptr<OecsCApi> api,QString dllPath):api_(std::move(api)),dllPath_(std::move(dllPath)),owner_(std::this_thread::get_id()) {}
OecsSession::~OecsSession() {
    if(std::this_thread::get_id()==owner_) {try{close();}catch(...){}return;}
    if(stage_==OecsSessionStage::Open) {
        pinned_=true;
        qCritical()<<"OECS oturumu sahip olmayan iş parçacığında yok edildi; kaynaklar bilerek sızdırıldı";
    }
}
OecsSessionStage OecsSession::stage() const {return stage_;}
bool OecsSession::libraryPinned() const {return pinned_;}
quint32 OecsSession::sdkVersion() const {return sdkVersion_;}

OecsDeviceInfo OecsSession::open(const OecsDeviceSelector& selector) {
    using F=OecsFailure;
    if(std::this_thread::get_id()!=owner_)throw OecsSessionError(F::WrongThread,"thread",0,"OECS oturumu yalnız sahibi olan iş parçacığından açılabilir");
    if(stage_!=OecsSessionStage::Unloaded)throw OecsSessionError(F::InvalidState,"state",0,"OECS oturumu açılamaz: oturum boş değil veya arızalı");
    if(selector.vendor.isEmpty()||selector.model.isEmpty()||selector.serial.isEmpty())throw OecsSessionError(F::InvalidSelector,"select",0,"OECS seçici üretici, model ve seri numarası içermeli");
    if(!QFileInfo(dllPath_).isAbsolute())throw OecsSessionError(F::Unavailable,"load",0,"OECS DLL yolu mutlak olmalı");
    QString reason;
    if(!api_->load(dllPath_,reason))throw OecsSessionError(F::Unavailable,"load",oecs::errorLoadLibrary,reason);
    sdkVersion_=api_->sdkVersion();
    // Undoes completed stages in reverse order; a failed destroy or unload pins the library and faults the session.
    const auto fail=[&](F failure,const QString& stage,int code,QString message,void* handle,bool claimed)->OecsSessionError {
        bool pin=false;
        if(handle) {
            const int status=api_->destroyHandle(handle);
            if(status!=oecs::ok){pin=true;message+=QString(" (cleanup: destroy %1)").arg(hex(status));}
        }
        if(!pin) {
            if(claimed)release(claimKey_);
            if(!api_->unload()){pin=true;message+=" (cleanup: unload)";}
        }
        if(pin){pinned_=true;stage_=OecsSessionStage::Faulted;}
        if(!pin||handle==nullptr)claimKey_.clear();
        return OecsSessionError(failure,stage,code,message);
    };
    QVector<OecsDeviceInfo> devices;
    int status=api_->enumerateUsb(devices);
    if(status!=oecs::ok)throw fail(F::ApiError,"enumerate",status,QString("OECS cihaz listesi alınamadı: %1").arg(hex(status)),nullptr,false);
    int index=-1,count=0;
    for(int i=0;i<devices.size();++i)
        if(devices[i].vendor==selector.vendor&&devices[i].model==selector.model&&devices[i].serial==selector.serial){index=i;++count;}
    if(count==0)throw fail(F::NotFound,"select",0,"OECS kamera bulunamadı",nullptr,false);
    if(count>1)throw fail(F::Ambiguous,"select",0,"OECS kamera kimliği birden fazla cihazla eşleşiyor",nullptr,false);
    const OecsDeviceInfo info=devices[index];
    const QString key=selector.vendor+"\n"+selector.model+"\n"+selector.serial;
    if(!claim(key))throw fail(F::Busy,"owner",0,"OECS kamera bu süreçte başka bir oturuma ait",nullptr,false);
    claimKey_=key;
    void* handle=nullptr;
    status=api_->createHandle(index,&handle);
    if(status!=oecs::ok)throw fail(F::ApiError,"create",status,QString("OECS tutamacı oluşturulamadı: %1").arg(hex(status)),nullptr,true);
    status=api_->openExclusive(handle);
    if(status==oecs::errorBusy||status==oecs::errorAccessDenied)throw fail(F::Busy,"open",status,QString("OECS kamera meşgul veya erişim reddedildi (başka uygulama sahibi olabilir): %1").arg(hex(status)),handle,true);
    if(status!=oecs::ok)throw fail(F::ApiError,"open",status,QString("OECS kamera açılamadı: %1").arg(hex(status)),handle,true);
    deviceInfo_=info;
    handle_=handle;
    stage_=OecsSessionStage::Open;
    return info;
}

void OecsSession::close() {
    if(std::this_thread::get_id()!=owner_)throw OecsSessionError(OecsFailure::WrongThread,"thread",0,"OECS oturumu yalnız sahibi olan iş parçacığından kapatılabilir");
    if(probing_)throw OecsSessionError(OecsFailure::InvalidState,"stream",0,"OECS probe is in flight");
    if(stage_!=OecsSessionStage::Open && !configurationFault_)return;
    configurationFault_=false;
    QString firstStage;
    int firstCode=0;
    const auto note=[&](const char* stage,int code){if(firstStage.isEmpty()){firstStage=stage;firstCode=code;}};
    int status=api_->closeDevice(handle_);
    if(status!=oecs::ok)note("close",status);
    status=api_->destroyHandle(handle_);
    if(status!=oecs::ok) {
        note("destroy",status);
        pinned_=true;
        stage_=OecsSessionStage::Faulted;
    } else {
        handle_=nullptr;
        streaming_=bufferHeld_=streamDirty_=false;
        release(claimKey_);
        claimKey_.clear();
        if(api_->unload())stage_=OecsSessionStage::Unloaded;
        else {note("unload",0);pinned_=true;stage_=OecsSessionStage::Faulted;}
    }
    if(!firstStage.isEmpty())throw OecsSessionError(OecsFailure::CleanupFailed,firstStage,firstCode,QString("OECS oturumu temiz kapatılamadı (%1): %2").arg(firstStage,hex(firstCode)));
}
}

namespace visionflow {
void OecsSession::requireOpen() const {
    if(std::this_thread::get_id()!=owner_)throw OecsSessionError(OecsFailure::WrongThread,"thread",0,"OECS feature access requires owner worker");
    if(stage_!=OecsSessionStage::Open||streamDirty_||probing_)throw OecsSessionError(OecsFailure::InvalidState,"feature",0,"OECS session is not idle/open");
}
CameraFeature OecsSession::feature(const QString& name){
    requireOpen();CameraFeature f;f.name=name;
    const int status=api_->readFeature(handle_,name,f);
    if(status!=oecs::ok)throw OecsSessionError(OecsFailure::ApiError,"read:"+name,status,"OECS feature read failed");
    return f;
}
QVector<CameraFeature> OecsSession::features(){requireOpen();QVector<CameraFeature> result;for(const auto& name:acquisitionFeatureNames())result.push_back(feature(name));return result;}
void OecsSession::writeFeature(const QString& name,const FeatureValue& value){
    const auto f=feature(name);validateFeatureWrite(f,value);
    if(featureEquivalent(f,value))return;
    const int status=api_->writeFeature(handle_,name,value);
    if(status!=oecs::ok)throw OecsSessionError(OecsFailure::ApiError,"write:"+name,status,"OECS feature write failed");
    if(!featureEquivalent(feature(name),value))throw OecsSessionError(OecsFailure::ConfigurationFailed,"readback:"+name,0,"OECS actual differs from requested");
}
void OecsSession::configurationFault(){requireOpen();configurationFault_=true;stage_=OecsSessionStage::Faulted;}
OecsDeviceInfo OecsSession::deviceInfo() const {requireOpen();return deviceInfo_;}
OecsTriggerCorrelation OecsSession::softwareTriggerCorrelation() const {
    requireOpen();return api_->softwareTriggerCorrelation();
}

QString OecsSession::cleanupStream(){
    QStringList failures;
    if(bufferHeld_){const int rc=api_->releaseBuffer(handle_);if(rc==0)bufferHeld_=false;else failures<<"release:"+hex(rc);}
    if(streamDirty_){const int rc=api_->flushStream(handle_);if(rc!=0)failures<<"flush:"+hex(rc);}
    // ClearImageBuffer requires acquisition still running on the physical OECS SDK.
    // Stop remains mandatory even after a release/flush error.
    if(streaming_){const int rc=api_->stopStream(handle_);if(rc==0)streaming_=false;else failures<<"stop:"+hex(rc);}
    if(failures.isEmpty())streamDirty_=false;
    return failures.join(", ");
}
void OecsSession::probeFrame(const OecsStreamLimits& limits,const std::function<void(const OecsBuffer&)>& copy){
    captureFrame(limits,copy);
}
OecsTriggerArm OecsSession::softwareFrame(const OecsStreamLimits& limits,const std::function<int()>& remaining,
                                        const std::function<void(const OecsBuffer&)>& copy){
    requireOpen();
    if(!remaining)throw OecsSessionError(OecsFailure::ConfigurationFailed,"deadline",0,"Monotonic budget required");
    if(softwareTriggerCorrelation()!=OecsTriggerCorrelation::ActualCounterEpoch)
        throw OecsSessionError(OecsFailure::Unavailable,"trigger-correlation",oecs::errorUnsupported,
            "Backend cannot prove trigger-ready state and actual frame/trigger counter epoch");
    // Process-wide generations survive close/reopen and different session instances.
    static std::atomic<quint64> next{1};
    quint64 generation=next.load();
    do {
        if(generation==std::numeric_limits<quint64>::max())throw OecsSessionError(OecsFailure::InvalidState,"generation",0,"Generation exhausted");
    }while(!next.compare_exchange_weak(generation,generation+1));
    for(const auto& pair: {std::pair{"TriggerSelector","FrameStart"},std::pair{"TriggerMode","On"},std::pair{"TriggerSource","Software"}}){
        const auto f=feature(pair.first);
        if((f.access!=FeatureAccess::ReadOnly&&f.access!=FeatureAccess::ReadWrite)||f.type!=FeatureType::Enumeration||
           !std::holds_alternative<QString>(f.value)||std::get<QString>(f.value)!=pair.second)
            throw OecsSessionError(OecsFailure::ConfigurationFailed,"trigger-settings",0,"Actual FrameStart/On/Software required");
    }
    const auto budget=[&]{
        const int ms=remaining();
        if(ms<=0)throw OecsSessionError(OecsFailure::ApiError,"deadline",0,"Expired trigger budget");
        return ms;
    };
    OecsTriggerArm arm;
    const auto check=[](int rc,const char* stage){if(rc!=0)throw OecsSessionError(OecsFailure::ApiError,stage,rc,"Software trigger correlation unavailable or failed");};
    captureFrame(limits,copy,budget,[&]{
        // A flush alone is insufficient: backend must also quiesce in-flight old exposures.
        check(api_->flushStream(handle_),"trigger-flush");
        check(api_->prepareSoftwareTrigger(handle_,generation,budget(),arm),"trigger-ready");
        budget();
        if(arm.generation!=generation)throw OecsSessionError(OecsFailure::ApiError,"trigger-epoch",0,"Unproven trigger epoch");
        check(api_->executeSoftwareTrigger(handle_),"trigger-command");
        budget();
    },[&](const OecsBuffer& f){
        // Exact modulo-32 successor supports wrap; reset/gaps/duplicates are errors.
        if(!f.triggerEvidence||f.triggerGeneration!=generation||f.hardwareFrameId!=quint32(arm.frameCounter+quint32(1))||
           f.triggerCounter!=quint32(arm.triggerCounter+quint32(1)))
            throw OecsSessionError(OecsFailure::ApiError,"trigger-frame",0,"Frame cannot be attributed to this trigger");
    });
    return arm;
}
void OecsSession::captureFrame(const OecsStreamLimits& limits,const std::function<void(const OecsBuffer&)>& copy,
                              const std::function<int()>& remaining,const std::function<void()>& ready,
                              const std::function<void(const OecsBuffer&)>& validate){
    requireOpen();
    const auto invalid=[](const char* message){throw OecsSessionError(OecsFailure::ConfigurationFailed,"stream-validation",0,message);};
    if(!copy||limits.buffers<1||limits.buffers>64||limits.poolBytes==0||limits.poolBytes>512ULL*1024*1024||limits.timeoutMs<1||limits.timeoutMs>30000)invalid("Invalid stream limits");
    if(remaining)remaining();
    // Read actual payload before the SDK can allocate its internal pool.
    const auto payload=feature("PayloadSize"),width=feature("Width"),height=feature("Height"),format=feature("PixelFormat");
    const auto integer=[&](const CameraFeature& f)->quint64 {
        if(f.type!=FeatureType::Integer||!std::holds_alternative<qint64>(f.value)||std::get<qint64>(f.value)<=0)invalid("Invalid actual stream dimensions/payload");
        if(f.access!=FeatureAccess::ReadOnly&&f.access!=FeatureAccess::ReadWrite)invalid("Unreadable stream feature");
        return quint64(std::get<qint64>(f.value));
    };
    const quint64 bytes=integer(payload),w=integer(width),h=integer(height);
    if(w>100000000||h>100000000||w*h>100000000||bytes<w*h||bytes>limits.poolBytes/limits.buffers)invalid("Stream pool budget or raster size exceeded");
    if(format.type!=FeatureType::Enumeration||!std::holds_alternative<QString>(format.value)||std::get<QString>(format.value)!="Mono8"||(format.access!=FeatureAccess::ReadOnly&&format.access!=FeatureAccess::ReadWrite))invalid("Only actual Mono8 is supported");
    const auto call=[](int rc,const char* stage){if(rc!=0)throw OecsSessionError(OecsFailure::ApiError,stage,rc,"OECS stream operation failed");};
    std::exception_ptr failure;
    probing_=true;
    try {
        if(remaining)remaining();
        call(api_->setBufferCount(handle_,limits.buffers),"pool");
        if(remaining)remaining();
        // Start may partially succeed. Always attempt stop/flush after entering it.
        streaming_=streamDirty_=true;
        call(api_->startStream(handle_),"start");
        if(remaining)remaining();
        if(ready)ready();
        OecsBuffer frame;
        const int wait=remaining?std::min(limits.timeoutMs,remaining()):limits.timeoutMs;
        if(wait<=0)throw OecsSessionError(OecsFailure::ApiError,"deadline",0,"Expired wait budget");
        call(api_->waitBuffer(handle_,wait,frame),"wait");
        bufferHeld_=true;
        if(remaining)remaining();
        if(validate)validate(frame);
        if(!frame.data||!frame.complete||frame.overflow||frame.chunks||frame.pixelFormat!=0x01080001||frame.width!=w||frame.height!=h||frame.stride<w||frame.stride>bytes/h||frame.bytes!=frame.stride*h||frame.bytes>bytes)invalid("Invalid/incomplete/unsupported Mono8 buffer");
        copy(frame); // Owned copy finishes before release, including exception paths.
        if(remaining)remaining();
    }catch(...){failure=std::current_exception();}
    const auto cleanup=cleanupStream();
    probing_=false;
    if(!cleanup.isEmpty()){
        configurationFault_=true;stage_=OecsSessionStage::Faulted;
        QString message="OECS stream cleanup failed: "+cleanup;
        if(failure)try{std::rethrow_exception(failure);}catch(const std::exception& e){message+="; acquisition: "+QString::fromUtf8(e.what());}catch(...){message+="; acquisition failed";}
        throw OecsSessionError(OecsFailure::CleanupFailed,"stream-cleanup",0,message);
    }
    if(!failure&&remaining)try{remaining();}catch(...){failure=std::current_exception();}
    if(failure){
        configurationFault_=true;stage_=OecsSessionStage::Faulted;
        std::rethrow_exception(failure);
    }
}
}
