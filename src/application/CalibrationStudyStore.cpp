#include "CalibrationStudyStore.hpp"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QLockFile>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}constexpr qint64 limit=4*1024*1024;}
void CalibrationStudyStore::save(const QString& path,const CalibrationTestImage& image,const PlanarCalibration& c) {
    require(!c.mapping() || c.mapping()->observation().target.pattern==CalibrationGridPattern::Chessboard,"Circle grid uses shared wizard records; legacy study UI is chessboard-only");
    require(QFileInfo(path).isAbsolute() && QFileInfo(path).absoluteFilePath()!=QFileInfo(image.frame.path()).absoluteFilePath(),"Study path must be absolute and distinct from source image");
    require(c.mapping() && c.dependency().cameraId.startsWith("calibration-test:") && c.dependency().cameraId==image.frame.metadata().sourceId && c.mapping()->observation().source.frameId==image.frame.metadata().frameId,"Study requires exact test image/fit provenance");
    const auto record=MeasurementService{}.encode(c);MeasurementService{}.decode(record);
    // Check that the external evidence asset still matches the owned fit image.
    const auto current=ChessboardService::importImage(image.frame.path());require(current.contentSha256==image.contentSha256,"Source image changed since import");
    const auto bytes=QJsonDocument(QJsonObject{{"format","visionflow.calibration-study"},{"schemaVersion",1},{"imagePath",image.frame.path()},{"decodedSha256",image.contentSha256},{"calibration",record}}).toJson();require(bytes.size()<=limit,"Study exceeds 4 MiB");
    QLockFile lock(path+".lock");require(lock.tryLock(0),"Study file busy");QSaveFile file(path);file.setDirectWriteFallback(false);require(file.open(QIODevice::WriteOnly),"Cannot stage calibration study");require(file.write(bytes)==bytes.size() && file.commit(),"Cannot commit calibration study");
}
CalibrationStudy CalibrationStudyStore::load(const QString& path) {
    require(QFileInfo(path).isAbsolute(),"Study requires absolute path");QFile file(path);require(file.open(QIODevice::ReadOnly) && file.size()<=limit,"Study missing or exceeds 4 MiB");const auto bytes=file.read(limit+1);require(file.error()==QFileDevice::NoError && bytes.size()<=limit,"Cannot read bounded study");QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);require(error.error==QJsonParseError::NoError && doc.isObject(),"Invalid study JSON");const auto j=doc.object();require(j.size()==5 && j["format"].toString()=="visionflow.calibration-study" && j["schemaVersion"].isDouble() && j["schemaVersion"].toDouble()==1 && j["imagePath"].isString() && j["decodedSha256"].isString() && j["calibration"].isObject(),"Unsupported study format");
    auto c=MeasurementService{}.decode(j["calibration"].toObject());require(c.mapping() && c.mapping()->observation().target.pattern==CalibrationGridPattern::Chessboard && c.dependency().cameraId.startsWith("calibration-test:"),"Legacy study requires chessboard test evidence");
    auto image=ChessboardService::importImage(j["imagePath"].toString());require(image.contentSha256==j["decodedSha256"].toString() && image.frame.metadata().sourceId==c.dependency().cameraId && image.frame.size()==c.dependency().resolution,"Study image missing/changed/content mismatch");image.frame=image.frame.withMetadata(c.mapping()->observation().source);return {image,c};
}
}
