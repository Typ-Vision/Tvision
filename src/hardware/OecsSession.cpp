#include "hardware/OecsSession.hpp"
#include <QDebug>
#include <QFileInfo>
#include <QSet>
#include <mutex>

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
    handle_=handle;
    stage_=OecsSessionStage::Open;
    return info;
}

void OecsSession::close() {
    if(std::this_thread::get_id()!=owner_)throw OecsSessionError(OecsFailure::WrongThread,"thread",0,"OECS oturumu yalnız sahibi olan iş parçacığından kapatılabilir");
    if(stage_!=OecsSessionStage::Open)return;
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
        release(claimKey_);
        claimKey_.clear();
        if(api_->unload())stage_=OecsSessionStage::Unloaded;
        else {note("unload",0);pinned_=true;stage_=OecsSessionStage::Faulted;}
    }
    if(!firstStage.isEmpty())throw OecsSessionError(OecsFailure::CleanupFailed,firstStage,firstCode,QString("OECS oturumu temiz kapatılamadı (%1): %2").arg(firstStage,hex(firstCode)));
}
}
