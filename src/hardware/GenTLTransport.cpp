#include "GenTLTransport.hpp"
#include <cstdint>
#define GCTLI_NO_DECLSPEC_STATEMENTS
#include <GenTL.h>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QElapsedTimer>
#include <QScopeGuard>
#include <mutex>
#include <stdexcept>
namespace visionflow {
namespace {
std::mutex producerMutex;
void fail(const QString& message) {throw std::runtime_error(message.toUtf8().constData());}
template<class F> F resolve(QLibrary& library,const char* name) {
    auto function=library.resolve(name);
    if(!function)fail(QString("GenTL export eksik: %1 — %2").arg(name,library.errorString()));
    return reinterpret_cast<F>(function);
}
// GenTL returns strings including their NUL. Guard both size and termination.
template<class F> QByteArray text(F read) {
    size_t size=0;
    auto code=read(nullptr,&size);
    if(code!=GenTL::GC_ERR_SUCCESS && code!=GenTL::GC_ERR_BUFFER_TOO_SMALL)fail(QString("GenTL string size hatası: %1").arg(code));
    if(!size || size>65536)fail("GenTL string boyutu geçersiz.");
    QByteArray bytes(static_cast<qsizetype>(size),'\0');
    code=read(bytes.data(),&size);
    if(code!=GenTL::GC_ERR_SUCCESS)fail(QString("GenTL string okuma hatası: %1").arg(code));
    if(!size || size>size_t(bytes.size()) || bytes[int(size)-1]!='\0')fail("GenTL string sonlandırması geçersiz.");
    bytes.resize(static_cast<qsizetype>(size)-1);return bytes;
}
}
struct GenTLTransport::Impl {
    std::unique_lock<std::mutex> lock{producerMutex,std::try_to_lock};
    QLibrary library;
    QString path;
    bool initialized=false;
    GenTL::TL_HANDLE tl=nullptr;
    GenTL::IF_HANDLE iface=nullptr;
    GenTL::DEV_HANDLE device=nullptr;
    GenTL::PGCInitLib init{};GenTL::PGCCloseLib finish{};GenTL::PGCGetLastError lastError{};
    GenTL::PTLOpen tlOpen{};GenTL::PTLClose tlClose{};GenTL::PTLUpdateInterfaceList updateInterfaces{};
    GenTL::PTLGetNumInterfaces numInterfaces{};GenTL::PTLGetInterfaceID interfaceId{};GenTL::PTLOpenInterface openInterface{};
    GenTL::PIFClose ifClose{};GenTL::PIFUpdateDeviceList updateDevices{};GenTL::PIFGetNumDevices numDevices{};
    GenTL::PIFGetDeviceID deviceId{};GenTL::PIFGetDeviceInfo deviceInfo{};GenTL::PIFOpenDevice openDevice{};GenTL::PDevClose devClose{};
    explicit Impl(const QString& file):path(QFileInfo(file).canonicalFilePath()) {
        if(!lock.owns_lock())fail("Başka bir GenTL oturumu çalışıyor; tamamlanınca tekrar deneyin.");
        if(path.isEmpty() || QFileInfo(path).suffix().compare("cti",Qt::CaseInsensitive)!=0)fail("Mevcut bir .cti producer dosyası seçin.");
        library.setFileName(path);library.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        if(!library.load())fail("GenTL producer yüklenemedi: "+library.errorString());
        init=resolve<GenTL::PGCInitLib>(library,"GCInitLib");finish=resolve<GenTL::PGCCloseLib>(library,"GCCloseLib");lastError=resolve<GenTL::PGCGetLastError>(library,"GCGetLastError");
        tlOpen=resolve<GenTL::PTLOpen>(library,"TLOpen");tlClose=resolve<GenTL::PTLClose>(library,"TLClose");
        updateInterfaces=resolve<GenTL::PTLUpdateInterfaceList>(library,"TLUpdateInterfaceList");numInterfaces=resolve<GenTL::PTLGetNumInterfaces>(library,"TLGetNumInterfaces");
        interfaceId=resolve<GenTL::PTLGetInterfaceID>(library,"TLGetInterfaceID");openInterface=resolve<GenTL::PTLOpenInterface>(library,"TLOpenInterface");
        ifClose=resolve<GenTL::PIFClose>(library,"IFClose");updateDevices=resolve<GenTL::PIFUpdateDeviceList>(library,"IFUpdateDeviceList");
        numDevices=resolve<GenTL::PIFGetNumDevices>(library,"IFGetNumDevices");deviceId=resolve<GenTL::PIFGetDeviceID>(library,"IFGetDeviceID");
        deviceInfo=resolve<GenTL::PIFGetDeviceInfo>(library,"IFGetDeviceInfo");openDevice=resolve<GenTL::PIFOpenDevice>(library,"IFOpenDevice");devClose=resolve<GenTL::PDevClose>(library,"DevClose");
    }
    ~Impl() {
        if(device)devClose(device);
        if(iface)ifClose(iface);
        if(tl)tlClose(tl);
        if(initialized)finish();
    }
    void check(GenTL::GC_ERROR code,const char* operation) const {
        if(code==GenTL::GC_ERR_SUCCESS)return;
        char message[2048]{};size_t size=sizeof(message);GenTL::GC_ERROR detail=0;
        lastError(&detail,message,&size);message[sizeof(message)-1]='\0';
        fail(QString("%1: GenTL %2 — %3").arg(operation).arg(code).arg(QString::fromUtf8(message)));
    }
    QString info(GenTL::IF_HANDLE handle,const QByteArray& id,GenTL::DEVICE_INFO_CMD command) {
        GenTL::INFO_DATATYPE type=GenTL::INFO_DATATYPE_UNKNOWN;
        // Optional metadata need not be implemented by older producers.
        size_t size=0;auto code=deviceInfo(handle,id.constData(),command,&type,nullptr,&size);
        if(code==GenTL::GC_ERR_NOT_IMPLEMENTED || code==GenTL::GC_ERR_NOT_AVAILABLE)return {};
        if(code!=GenTL::GC_ERR_BUFFER_TOO_SMALL)check(code,"IFGetDeviceInfo(size)");
        if(type!=GenTL::INFO_DATATYPE_STRING)fail("GenTL metadata tipi STRING değil.");
        const auto value=text([&](char* data,size_t* count){return deviceInfo(handle,id.constData(),command,&type,data,count);});
        if(type!=GenTL::INFO_DATATYPE_STRING)fail("GenTL metadata tipi STRING değil.");
        return QString::fromUtf8(value);
    }
};
GenTLTransport::GenTLTransport(const QString& path):impl_(std::make_unique<Impl>(path)) {
    impl_->check(impl_->init(),"GCInitLib");impl_->initialized=true;
    impl_->check(impl_->tlOpen(&impl_->tl),"TLOpen");
    if(!impl_->tl)fail("GenTL producer boş system handle döndürdü.");
}
GenTLTransport::~GenTLTransport()=default;
QVector<CameraDevice> GenTLTransport::discover(int timeoutMs,const std::shared_ptr<std::atomic_bool>& cancel) {
    if(timeoutMs<=0)fail("Keşif timeout değeri pozitif olmalı.");
    if(isDeviceOpen())fail("Keşiften önce cihazı kapatın.");
    QElapsedTimer timer;timer.start();
    auto remaining=[&]() -> uint64_t {
        if(cancel && cancel->load())fail("Kamera keşfi iptal edildi.");
        const auto left=timeoutMs-timer.elapsed();if(left<=0)fail("Kamera keşfi zaman aşımı.");return uint64_t(left);
    };
    auto& p=*impl_;bool8_t changed=false;
    p.check(p.updateInterfaces(p.tl,&changed,remaining()),"TLUpdateInterfaceList");remaining();
    uint32_t count=0;p.check(p.numInterfaces(p.tl,&count),"TLGetNumInterfaces");if(count>4096)fail("GenTL interface sayısı sınırı aşıyor.");
    QVector<CameraDevice> devices;
    for(uint32_t i=0;i<count;++i) {
        remaining();const auto ifaceId=text([&](char* data,size_t* size){return p.interfaceId(p.tl,i,data,size);});
        GenTL::IF_HANDLE iface=nullptr;p.check(p.openInterface(p.tl,ifaceId.constData(),&iface),"TLOpenInterface");
        if(!iface)fail("GenTL producer boş interface handle döndürdü.");
        auto cleanup=qScopeGuard([&]{p.ifClose(iface);});
        p.check(p.updateDevices(iface,&changed,remaining()),"IFUpdateDeviceList");remaining();
        uint32_t n=0;p.check(p.numDevices(iface,&n),"IFGetNumDevices");if(n>4096 || devices.size()+n>4096)fail("GenTL cihaz sayısı sınırı aşıyor.");
        for(uint32_t j=0;j<n;++j) {
            remaining();const auto id=text([&](char* data,size_t* size){return p.deviceId(iface,j,data,size);});
            if(id.isEmpty() || ifaceId.isEmpty() || id.contains('\0') || ifaceId.contains('\0'))fail("GenTL cihaz/interface kimliği geçersiz.");
            devices.append({p.path,QString::fromUtf8(ifaceId),QString::fromUtf8(id),p.info(iface,id,GenTL::DEVICE_INFO_VENDOR),p.info(iface,id,GenTL::DEVICE_INFO_MODEL),p.info(iface,id,GenTL::DEVICE_INFO_SERIAL_NUMBER),p.info(iface,id,GenTL::DEVICE_INFO_TLTYPE)});
        }
        remaining();p.check(p.ifClose(iface),"IFClose");cleanup.dismiss();
    }
    remaining();return devices;
}
void GenTLTransport::openReadOnly(const CameraDevice& device) {
    auto& p=*impl_;
    if(isDeviceOpen())fail("Cihaz zaten açık.");
    if(QFileInfo(device.producerPath).canonicalFilePath()!=p.path || device.interfaceId.isEmpty() || device.deviceId.isEmpty() || device.interfaceId.contains(QChar(0)) || device.deviceId.contains(QChar(0)))fail("Cihaz/producer referansı geçersiz.");
    const auto iface=device.interfaceId.toUtf8(),id=device.deviceId.toUtf8();
    p.check(p.openInterface(p.tl,iface.constData(),&p.iface),"TLOpenInterface");
    if(!p.iface)fail("GenTL producer boş interface handle döndürdü.");
    try {p.check(p.openDevice(p.iface,id.constData(),GenTL::DEVICE_ACCESS_READONLY,&p.device),"IFOpenDevice(readonly)");if(!p.device)fail("GenTL producer boş device handle döndürdü.");}
    catch(...) {p.ifClose(p.iface);p.iface=nullptr;throw;}
}
void GenTLTransport::closeDevice() {
    auto& p=*impl_;
    if(p.device){p.check(p.devClose(p.device),"DevClose");p.device=nullptr;}
    if(p.iface){p.check(p.ifClose(p.iface),"IFClose");p.iface=nullptr;}
}
bool GenTLTransport::isDeviceOpen() const {return impl_->device!=nullptr;}
QStringList genTLProducerPaths(const QStringList& directories) {
    QStringList dirs=directories;dirs.append(qEnvironmentVariable("GENICAM_GENTL64_PATH").split(QDir::listSeparator(),Qt::SkipEmptyParts));
    QStringList paths;
    for(const auto& directory:dirs) {
        if(directory.trimmed().isEmpty())continue;
        const QDir dir(directory);
        for(const auto& file:dir.entryInfoList(QDir::Files|QDir::Readable,QDir::Name)) {
            if(file.suffix().compare("cti",Qt::CaseInsensitive)!=0)continue;
            const auto path=file.canonicalFilePath();if(!path.isEmpty() && !paths.contains(path,Qt::CaseInsensitive))paths.append(path);
        }
    }
    return paths;
}
}
