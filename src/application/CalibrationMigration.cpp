#include "CalibrationMigration.hpp"
#include <stdexcept>
namespace visionflow {
int CalibrationMigration::adopt(const CalibrationService& legacy,ActiveCalibrationService& target,const QString& id,const ToolDefinition& source,const QString& directory,const ImageFrame& actual,const QString& installation){
    auto require=[](bool b,const char* s){if(!b)throw std::runtime_error(s);};QVector<PlanarCalibration> history;for(const auto& m:legacy.records())if(m.dependency().calibrationId==id)history.append(m);require(!history.isEmpty(),"Legacy calibration not found");
    const auto& chosen=history.last().dependency();require(actual.metadata().sourceId==source.id && !actual.metadata().frameId.isEmpty() && actual.processing().isEmpty() && actual.metadata().completedAtUtc.isValid(),"Actual raw source acquisition required for legacy setup association");
    const auto existing=target.records();int saved=0;const auto at=QDateTime::currentDateTimeUtc();
    // Preflight the whole history before any write. A commit failure may still
    // leave a consecutive prefix; retry verifies and resumes that exact prefix.
    QVector<CalibrationModelRecord> pending;
    for(const auto& m:history){const auto& d=m.dependency();require(d.lensId==chosen.lensId && d.workingPlaneId==chosen.workingPlaneId && d.resolution==chosen.resolution,"Legacy history changed lens/plane/resolution; cannot assert one shared setup");auto setup=CalibrationCapture::setup(source,directory,actual,installation,d.lensId,d.workingPlaneId);require(setup.resolution==d.resolution,"Actual resolution differs from legacy evidence");setup.source=d.source;setup.target=d.target;
        auto record=CalibrationModelRecord::adoptLegacy(m,setup,(id+QString(" v%1").arg(d.version)).left(128),at);bool present=false;
        for(const auto& old:existing)if(old.model().dependency().calibrationId==id && old.model().dependency().version==d.version){require(MeasurementService{}.encode(old.model())==MeasurementService{}.encode(m) && old.setup()==setup && old.image().origin=="legacy_record","Legacy adoption conflicts with existing shared history");present=true;break;}if(!present)pending.append(record);
    }
    for(const auto& r:pending){target.save(r);++saved;}return saved;
}
}
