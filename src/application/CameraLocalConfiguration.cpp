#include "CameraLocalConfiguration.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSettings>
#include <stdexcept>
namespace visionflow {
QString CameraLocalConfiguration::key(const PhysicalDeviceIdentity& id) const {
    id.validate();if(!id.usable())throw std::runtime_error("Exact identity required for local route.");
    return "camera/routes/"+QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(id.toJson()).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
QStringList CameraLocalConfiguration::producers() const {QSettings s(path_,QSettings::IniFormat);return s.value("camera/producers").toStringList();}
void CameraLocalConfiguration::setProducers(const QStringList& files) const {
    QStringList canonical;for(const auto& f:files){const auto path=QFileInfo(f).canonicalFilePath();if(path.isEmpty())throw std::runtime_error("Producer file unavailable.");canonical.append(path);}canonical.removeDuplicates();
    QDir().mkpath(QFileInfo(path_).absolutePath());QSettings s(path_,QSettings::IniFormat);s.setValue("camera/producers",canonical);s.sync();if(s.status()!=QSettings::NoError)throw std::runtime_error("Local camera configuration write failed.");
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
