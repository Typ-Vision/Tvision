#include "RuntimeLibrary.hpp"
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QDir>
#include <mutex>
#include <QtEndian>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace visionflow {
QString runtimeBinaryError(const QString& path){
    const QFileInfo info(path);
    if(!info.isAbsolute() || !info.isFile())return "Runtime dosyası yok; mutlak dosya yolunu yeniden seçin: "+path;
    QFile file(path);if(!file.open(QIODevice::ReadOnly))return "Runtime dosyası okunamıyor: "+path;
#ifdef Q_OS_WIN
    const auto header=file.read(64);
    if(header.size()!=64 || header.left(2)!="MZ")return "Geçerli Windows PE dosyası değil: "+path;
    const auto offset=qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(header.constData()+60));
    if(!file.seek(offset))return "Geçersiz PE başlığı: "+path;
    const auto pe=file.read(26);
    if(pe.size()!=26 || pe.left(4)!=QByteArray("PE\0\0",4))return "Geçersiz PE başlığı: "+path;
    if(qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(pe.constData()+4))!=0x8664 ||
       qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(pe.constData()+24))!=0x20b)return "Yanlış mimari; Windows x64 PE gerekli: "+path;
    if(!(qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(pe.constData()+22)) & 0x2000))return "Windows DLL dosyası gerekli: "+path;
#endif
    return {};
}
RuntimeLibrary::~RuntimeLibrary(){
    unload();
}
void RuntimeLibrary::unload(){
#ifdef Q_OS_WIN
    if(handle_)FreeLibrary(static_cast<HMODULE>(handle_));
#endif
    handle_=nullptr;
}
bool RuntimeLibrary::load(const QString& path,const QStringList& dependencies,QString& error){
    error=runtimeBinaryError(path);if(!error.isEmpty())return false;
#ifdef Q_OS_WIN
    static std::mutex loaderMutex;const std::lock_guard lock(loaderMutex);
    if(handle_){error="Runtime zaten yüklü; Stop/close ve yeniden açma gerekli.";return false;}
    auto dirs=dependencies;
    const auto config=qEnvironmentVariable("VISIONFLOW_CAMERA_CONFIGURATION");
    if(!config.isEmpty()){
        QSettings settings(config,QSettings::IniFormat);
        for(const auto& backend:QStringList{"oecs","basler"}){const auto key="camera/runtime/"+backend;
            if(QFileInfo(settings.value(key+"/path").toString()).canonicalFilePath()==QFileInfo(path).canonicalFilePath())dirs.append(settings.value(key+"/dependencies").toStringList());}
    }
    dirs.removeDuplicates();QVector<DLL_DIRECTORY_COOKIE> cookies;
    wchar_t systemDirectory[32768]{};const auto systemSize=GetSystemDirectoryW(systemDirectory,32768);
    const auto systemPath=systemSize && systemSize<32768?QFileInfo(QString::fromWCharArray(systemDirectory,int(systemSize))).canonicalFilePath():QString{};
    auto checkDirs=dirs;checkDirs.append(QFileInfo(path).absolutePath());
    for(const auto& dir:checkDirs)for(const auto& file:QDir(dir).entryInfoList({"*.dll"},QDir::Files)){
        const auto module=GetModuleHandleW(reinterpret_cast<LPCWSTR>(file.fileName().utf16()));
        if(module){wchar_t loaded[32768]{};const auto size=GetModuleFileNameW(module,loaded,32768);
            if(size && QFileInfo(QString::fromWCharArray(loaded,int(size))).canonicalFilePath().compare(file.canonicalFilePath(),Qt::CaseInsensitive)==0)continue;
            // Shared Windows dependencies can already be loaded centrally (e.g. MSVCR100).
            if(size && !systemPath.isEmpty() && file.canonicalFilePath()!=QFileInfo(path).canonicalFilePath() &&
               QFileInfo(QString::fromWCharArray(loaded,int(size))).canonicalPath().compare(systemPath,Qt::CaseInsensitive)==0)continue;
            error="Başka kurulumdan aynı adlı DLL yüklü; uygulamayı yeniden başlatın: "+file.fileName();return false;}
    }
    for(const auto& dir:dirs){
        if(!QFileInfo(dir).isAbsolute() || !QFileInfo(dir).isDir()){error="Bağımlılık dizini yok: "+dir;break;}
        const auto nativeDir=QDir::toNativeSeparators(dir);
        const auto cookie=AddDllDirectory(reinterpret_cast<LPCWSTR>(nativeDir.utf16()));
        if(!cookie){error="Bağımlılık dizini yükleyiciye eklenemedi: "+dir;break;}cookies.append(cookie);
    }
    if(error.isEmpty()){
        // DLL_LOAD_DIR needs native separators to resolve sibling dependencies.
        const auto nativePath=QDir::toNativeSeparators(path);
        handle_=LoadLibraryExW(reinterpret_cast<LPCWSTR>(nativePath.utf16()),nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS|LOAD_LIBRARY_SEARCH_USER_DIRS);
        if(!handle_)error=QString("Runtime/bağımlılık yüklenemedi (Windows %1): %2").arg(GetLastError()).arg(path);
    }
    for(auto cookie:cookies)RemoveDllDirectory(cookie);
    return handle_!=nullptr;
#else
    Q_UNUSED(dependencies);
    return true;
#endif
}
}
