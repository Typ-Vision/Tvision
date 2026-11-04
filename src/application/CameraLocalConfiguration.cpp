#include "CameraLocalConfiguration.hpp"
#include "hardware/GenTLTransport.hpp"
#include "hardware/RuntimeLibrary.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSettings>
#include <stdexcept>
namespace visionflow {
namespace {
QString canonical(const QString& file){const QFileInfo info(file);const auto path=info.canonicalFilePath();return path.isEmpty()?info.absoluteFilePath():path;}
QString backendKey(const QString& backend){if(backend!="oecs" && backend!="basler")throw std::runtime_error("Unknown camera runtime backend.");return "camera/runtime/"+backend;}
void sync(QSettings& s){s.sync();if(s.status()!=QSettings::NoError)throw std::runtime_error("Local camera configuration write failed.");}
}
QStringList CameraLocalConfiguration::automaticProducers() const {
    QSettings s(path_,QSettings::IniFormat);auto files=producers();files.append(genTLProducerPaths());
    const auto disabled=s.value("camera/disabledProducers").toStringList();QStringList result;
    for(const auto& file:files){const auto path=canonical(file);if(!disabled.contains(path,Qt::CaseInsensitive) && !result.contains(path,Qt::CaseInsensitive))result.append(path);}
    return result;
}
void CameraLocalConfiguration::setProducerEnabled(const QString& file,bool enabled) const {
    QDir().mkpath(QFileInfo(path_).absolutePath());QSettings s(path_,QSettings::IniFormat);auto disabled=s.value("camera/disabledProducers").toStringList();
    const auto path=canonical(file);for(int i=disabled.size()-1;i>=0;--i)if(disabled[i].compare(path,Qt::CaseInsensitive)==0)disabled.removeAt(i);
    if(!enabled)disabled.append(path);s.setValue("camera/disabledProducers",disabled);sync(s);
}
void CameraLocalConfiguration::removeProducer(const QString& file) const {
    auto files=producers();const auto path=canonical(file);
    for(int i=files.size()-1;i>=0;--i)if(canonical(files[i]).compare(path,Qt::CaseInsensitive)==0)files.removeAt(i);
    QSettings s(path_,QSettings::IniFormat);s.setValue("camera/producers",files);sync(s);
    setProducerEnabled(path,false);
}
CameraRuntimeLocation CameraLocalConfiguration::runtime(const QString& backend,const QString& explicitPath) const {
    const auto key=backendKey(backend);CameraRuntimeLocation result;
    if(!path_.isEmpty()){QSettings s(path_,QSettings::IniFormat);result.path=s.value(key+"/path").toString();result.dependencies=s.value(key+"/dependencies").toStringList();}
    if(!explicitPath.isEmpty())result.path=explicitPath;
    if(result.path.isEmpty()){
        QStringList candidates;
        const auto add=[&](const QString& path){if(QFileInfo(path).isFile()){const auto value=canonical(path);if(!candidates.contains(value,Qt::CaseInsensitive))candidates.append(value);}};
        const auto env=qEnvironmentVariable(backend=="oecs"?"OECS_RUNTIME_DIR":"PYLON_DEV_DIR");
        const auto scan=[&](const QString& directory){if(directory.isEmpty())return;
            if(backend=="oecs")add(QDir(directory).filePath("MvCameraControl.dll"));
            else for(const auto& file:QDir(directory).entryList({"PylonC_v*.dll"},QDir::Files))add(QDir(directory).filePath(file));};
        if(backend=="oecs")scan(env);
        else if(!env.isEmpty())scan(QDir(env).filePath("../Runtime/x64"));
        // A valid vendor environment is preferred to known installation locations.
        if(candidates.isEmpty())scan(backend=="oecs"?"C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64":"C:/Program Files/Basler/pylon/Runtime/x64");
        if(candidates.size()!=1){result.error=candidates.isEmpty()?"Runtime bulunamadı; DLL seçin.":"Birden fazla runtime bulundu; DLL seçin: "+candidates.join("; ");return result;}
        result.path=candidates.first();
    }
    result.error=runtimeBinaryError(result.path);
    for(const auto& dir:result.dependencies)if(!QFileInfo(dir).isAbsolute() || !QFileInfo(dir).isDir())result.error="Bağımlılık dizini yok; yeniden seçin: "+dir;
    if(result.error.isEmpty())result.path=canonical(result.path);
    return result;
}
void CameraLocalConfiguration::setRuntime(const QString& backend,const QString& path,const QStringList& dependencies) const {
    const auto error=runtimeBinaryError(path);if(!error.isEmpty())throw std::runtime_error(error.toStdString());
    QStringList dirs;for(const auto& dir:dependencies){if(!QFileInfo(dir).isAbsolute() || !QFileInfo(dir).isDir())throw std::runtime_error("Runtime dependency directory unavailable.");dirs.append(QFileInfo(dir).canonicalFilePath());}dirs.removeDuplicates();
    QDir().mkpath(QFileInfo(path_).absolutePath());QSettings s(path_,QSettings::IniFormat);const auto key=backendKey(backend);
    s.setValue(key+"/path",canonical(path));s.setValue(key+"/dependencies",dirs);sync(s);
}
QString CameraLocalConfiguration::defaultRuntime(const QString& backend){
    const auto config=qEnvironmentVariable("VISIONFLOW_CAMERA_CONFIGURATION");
    const auto value=CameraLocalConfiguration(config).runtime(backend);
    // Preserve a broken explicit selection for the binding's visible error; never switch SDK.
    return value.path;
}
QString CameraLocalConfiguration::key(const PhysicalDeviceIdentity& id) const {
    id.validate();if(!id.usable())throw std::runtime_error("Exact identity required for local route.");
    return "camera/routes/"+QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(id.toJson()).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
QStringList CameraLocalConfiguration::producers() const {QSettings s(path_,QSettings::IniFormat);return s.value("camera/producers").toStringList();}
void CameraLocalConfiguration::setProducers(const QStringList& files) const {
    const auto previous=producers();QStringList paths;
    for(const auto& f:files){const auto path=canonical(f);if(!QFileInfo(path).isFile() && !previous.contains(path,Qt::CaseInsensitive))throw std::runtime_error("Producer file unavailable.");if(!paths.contains(path,Qt::CaseInsensitive))paths.append(path);}
    QDir().mkpath(QFileInfo(path_).absolutePath());QSettings s(path_,QSettings::IniFormat);s.setValue("camera/producers",paths);sync(s);
}
std::optional<TransportLocator> CameraLocalConfiguration::route(const PhysicalDeviceIdentity& id) const {
    QSettings s(path_,QSettings::IniFormat);const auto bytes=s.value(key(id)).toByteArray();if(bytes.isEmpty())return {};
    return TransportLocator::fromJson(QJsonDocument::fromJson(bytes).object());
}
void CameraLocalConfiguration::setRoute(const PhysicalDeviceIdentity& id,const TransportLocator& route) const {
    route.validate();QDir().mkpath(QFileInfo(path_).absolutePath());QSettings s(path_,QSettings::IniFormat);
    s.setValue(key(id),QJsonDocument(route.toJson()).toJson(QJsonDocument::Compact));s.sync();if(s.status()!=QSettings::NoError)throw std::runtime_error("Local camera route write failed.");
}
}
