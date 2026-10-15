#include "CalibrationService.hpp"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
#include <QMap>
#include <stdexcept>

namespace visionflow {
namespace {
constexpr qint64 byteLimit=4*1024*1024;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void requireSource(const Recipe& r,const CalibrationDependency& d) {
    int count=0;for(const auto& t:editorTools(r.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")) {
        ++count;require(t.id==d.cameraId,"Recipe acquisition source differs from calibration installation");
    }
    require(count==1,"Calibration binding requires one enabled acquisition source");
}
}
CalibrationService::CalibrationService(QString path,InstallationProvider provider):path_(std::move(path)),provider_(std::move(provider)) {
    require(QFileInfo(path_).isAbsolute(),"Calibration catalog requires an absolute path");
}
QVector<PlanarCalibration> CalibrationService::read() const {
    if(!QFileInfo::exists(path_))return {};
    QFile file(path_);require(file.open(QIODevice::ReadOnly),"Cannot open calibration catalog");
    require(file.size()<=byteLimit,"Calibration catalog exceeds 4 MiB");
    const auto bytes=file.read(byteLimit+1);require(file.error()==QFileDevice::NoError && bytes.size()<=byteLimit,"Cannot read bounded calibration catalog");
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);
    require(error.error==QJsonParseError::NoError && doc.isObject(),"Invalid calibration catalog JSON");const auto o=doc.object();
    require(o.size()==3 && o["format"].toString()=="visionflow.calibrations" && o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1 && o["records"].isArray(),"Unsupported calibration catalog");
    const auto records=o["records"].toArray();require(records.size()<=128,"Calibration history full (128 records)");
    QVector<PlanarCalibration> result;QMap<QString,int> versions;MeasurementService codec;
    for(const auto& value:records) {
        require(value.isObject(),"Invalid calibration catalog record");auto c=codec.decode(value.toObject());const auto& d=c.dependency();
        require(!c.mapping() || c.mapping()->observation().target.pattern==CalibrationGridPattern::Chessboard,"Circle grid requires shared wizard catalog; legacy recipe binding unsupported");
        require(c.validated() && d.version==versions.value(d.calibrationId)+1,"Calibration history must contain validated consecutive versions");
        versions[d.calibrationId]=d.version;result.append(c);
    }
    return result;
}
void CalibrationService::save(const PlanarCalibration& calibration) const {
    require(!calibration.mapping() || calibration.mapping()->observation().target.pattern==CalibrationGridPattern::Chessboard,"Circle grid requires shared wizard catalog; legacy recipe binding unsupported");
    require(calibration.validated(),"Cannot persist unvalidated calibration");
    // Semantic verification before acquiring the commit lock.
    MeasurementService codec;const auto record=codec.encode(calibration);codec.decode(record);
    QLockFile lock(path_+".lock");require(lock.tryLock(0),"Calibration catalog busy");
    const auto existing=read();require(existing.size()<128,"Calibration history full; automatic deletion disabled");
    int latest=0;for(const auto& c:existing)if(c.dependency().calibrationId==calibration.dependency().calibrationId)latest=c.dependency().version;
    require(latest<1000000000 && calibration.dependency().version==latest+1,"Calibration version must append consecutively");
    QJsonArray records;for(const auto& c:existing)records.append(codec.encode(c));records.append(record);
    const auto bytes=QJsonDocument(QJsonObject{{"format","visionflow.calibrations"},{"schemaVersion",1},{"records",records}}).toJson();
    require(bytes.size()<=byteLimit,"Calibration catalog exceeds 4 MiB");
    QSaveFile file(path_);file.setDirectWriteFallback(false);require(file.open(QIODevice::WriteOnly),"Cannot stage calibration catalog");
    require(file.write(bytes)==bytes.size(),"Cannot write calibration catalog");require(file.commit(),"Cannot commit calibration catalog");
}
PlanarCalibration CalibrationService::load(const QString& id,int version) const {
    for(const auto& c:read())if(c.dependency().calibrationId==id && c.dependency().version==version)return c;
    throw std::runtime_error("Calibration version not found");
}
CalibrationDependency CalibrationService::current(const QString& id) const {
    return currentFrom(read(),id);
}
CalibrationDependency CalibrationService::currentFrom(const QVector<PlanarCalibration>& records,const QString& id) const {
    require(bool(provider_),"Independent installation provider required");
    std::optional<CalibrationDependency> latest;
    for(const auto& c:records)if(c.dependency().calibrationId==id) {
        require(!c.manualProvenance() || c.manualProvenance()->origin=="acquired","File test evidence cannot resolve a production installation");
        latest=c.dependency();
    }
    require(latest.has_value(),"Current calibration not found");const auto setup=provider_(id);
    auto resolved=*latest;resolved.cameraId=setup.cameraId;resolved.lensId=setup.lensId;resolved.resolution=setup.resolution;
    resolved.workingPlaneId=setup.workingPlaneId;resolved.source=setup.source;resolved.target=setup.target;
    latest->requireCurrent(resolved);return resolved;
}
PlanarCalibration CalibrationService::resolve(const Recipe& recipe) const {
    require(recipe.calibrationBinding.has_value() && recipe.calibrationReference==recipe.calibrationBinding->calibrationId,"Recipe requires an exact calibration binding");
    const auto records=read();const auto& binding=*recipe.calibrationBinding;binding.requireCurrent(currentFrom(records,binding.calibrationId));
    requireSource(recipe,binding);
    for(const auto& c:records)if(c.dependency()==binding)return c;
    throw std::runtime_error("Bound calibration version not found");
}
void CalibrationService::bind(ProjectService& project,const QString& id) const {
    const auto dependency=current(id);require(project.activeRecipe()!=nullptr,"Select recipe before binding");
    requireSource(*project.activeRecipe(),dependency);project.bindCalibration(dependency);
}
}
