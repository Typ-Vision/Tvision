#include "CalibrationTargetLibrary.hpp"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QLockFile>
#include <QJsonDocument>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}}
QMap<QString,CalibrationGridTarget> CalibrationTargetLibrary::read() const {
    require(QFileInfo(path_).isAbsolute(),"Absolute target library path required");if(!QFileInfo::exists(path_))return {};
    QFile file(path_);require(file.open(QIODevice::ReadOnly) && file.size()<=1024*1024,"Cannot read bounded target library");QJsonParseError error;const auto d=QJsonDocument::fromJson(file.readAll(),&error);require(error.error==QJsonParseError::NoError && d.isObject(),"Invalid target library JSON");const auto o=d.object();require(o.size()==2 && o["schemaVersion"]==1 && o["targets"].isObject(),"Invalid target library schema");const auto entries=o["targets"].toObject();require(entries.size()<=64,"Target library full");QMap<QString,CalibrationGridTarget> result;
    for(auto i=entries.begin();i!=entries.end();++i){require(!i.key().trimmed().isEmpty() && i.key()==i.key().trimmed() && i.key().size()<=128 && i.value().isObject(),"Invalid target name");result.insert(i.key(),CalibrationGridTarget::fromJson(i.value().toObject()));}return result;
}
void CalibrationTargetLibrary::save(QString name,CalibrationGridTarget target) const {
    require(!name.trimmed().isEmpty() && name==name.trimmed() && name.size()<=128,"Target name required");target.validate();QLockFile lock(path_+".lock");require(lock.tryLock(0),"Target library busy");auto all=read();all.insert(name,target);require(all.size()<=64,"Target library full");QJsonObject entries;for(auto i=all.begin();i!=all.end();++i)entries[i.key()]=i.value().toJson();const auto bytes=QJsonDocument(QJsonObject{{"schemaVersion",1},{"targets",entries}}).toJson();require(bytes.size()<=1024*1024,"Target library too large");QSaveFile file(path_);file.setDirectWriteFallback(false);require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size() && file.commit(),"Cannot save target library");
}
}
