#include "ExperimentalCalibrationEvidence.hpp"
#include <QJsonDocument>
#include <QSet>
#include <stdexcept>

namespace visionflow {
namespace { void require(bool b,const char* why){if(!b)throw std::runtime_error(why);} }
void ExperimentalCalibrationEvidence::validate() const {
    require(origin==SourceOrigin::CameraAcquired || origin==SourceOrigin::SetupFile,"Experimental calibration requires exact live/replay target");
    MeasurementSetupContract{{camera},{setup},{profile}}.validate();
    require(camera.physical && camera.physical->usable() && setup.geometry.critical(),"Experimental physical identity/geometry required");
    const auto& p=acquisition;
    require(p["schema"]=="visionflow.oecs-experimental1" && p["policy"]=="UnverifiedNextArrival" &&
        p["productionAdmission"]==false && p["triggerAssociationVerified"]==false && p["embeddedMetadataEnabled"]==false,
        "Experimental calibration provenance required");
    require(p["vendor"]==camera.physical->vendor && p["model"]==camera.physical->model && p["serial"]==camera.physical->serial &&
        camera.physical->vendor=="Opto Engineering Srl" && camera.physical->model=="COE-050-M-USB-050-IR-C","Experimental camera identity mismatch");
    require(p["profileSnapshot"].toObject()==profile.toJson() && p["profileId"]==profile.id.value &&
        p["profileRevision"]==double(profile.revision) && !p["acquisitionId"].toString().isEmpty(),"Experimental profile mismatch");
    const auto actual=AcquisitionReadback::fromJson(p["readback"].toObject());
    auto requested=profile.requested;
    if(actual.settings.exposureTimeUs==double(float(requested.exposureTimeUs)))requested.exposureTimeUs=actual.settings.exposureTimeUs;
    if(actual.settings.gain==double(float(requested.gain)))requested.gain=actual.settings.gain;
    require(actual.geometry.descriptor()==setup.geometry && actual.settings==requested &&
        actual.settings.trigger==TriggerSettings{true,"FrameBurstStart","Software"},"Experimental setup/readback mismatch");
    require(setup.geometry.resolution==QSize(2448,2048) && actual.geometry.offset==QPoint(0,0) &&
        actual.geometry.binning==QSize(1,1) && actual.geometry.decimation==QSize(1,1) && actual.geometry.reverse==GeometryMirror{} &&
        p["width"]==2448 && p["height"]==2048 && p["originX"]==0 && p["originY"]==0 && p["pixelFormat"]=="Mono8",
        "Experimental raster geometry mismatch");
    const auto hash=p["contentId"].toString();require(hash.size()==64,"Experimental raster hash required");
    for(auto c:hash)require((c>='0' && c<='9') || (c>='a' && c<='f'),"Invalid experimental raster hash");
    require(!p["frameId"].toString().isEmpty() && !p["sourceId"].toString().isEmpty() &&
        QDateTime::fromString(p["completedAtUtc"].toString(),Qt::ISODateWithMs).isValid(),"Experimental original frame metadata required");
    require(QJsonDocument(p).toJson(QJsonDocument::Compact).size()<=1024*1024,"Experimental provenance too large");
}
void ExperimentalCalibrationEvidence::verify(const ImageFrame& image) const {
    validate();require(image.processing().isEmpty() && image.format()==PixelFormat::Gray8 && image.size()==setup.geometry.resolution &&
        image.metadata().experimentalAcquisition==acquisition && acquisition["contentId"]==QString::fromLatin1(image.contentId().toHex()),
        "Experimental calibration raster/provenance mismatch");
}
QJsonObject ExperimentalCalibrationEvidence::toJson() const {
    validate();return {{"schemaVersion",1},{"origin",sourceOriginName(origin)},{"camera",camera.toJson()},{"setup",setup.toJson()},
        {"profile",profile.toJson()},{"acquisition",acquisition},{"physicalAcceptance",false}};
}
ExperimentalCalibrationEvidence ExperimentalCalibrationEvidence::fromJson(const QJsonObject& o){
    const QSet<QString> fields{"schemaVersion","origin","camera","setup","profile","acquisition","physicalAcceptance"};
    require(o.size()==fields.size(),"Missing experimental calibration evidence");for(auto i=o.begin();i!=o.end();++i)require(fields.contains(i.key()),"Unknown experimental calibration field");
    require(o["schemaVersion"]==1 && o["physicalAcceptance"]==false,"Unsupported experimental calibration evidence");
    ExperimentalCalibrationEvidence e{sourceOriginFromName(o["origin"].toString()),MeasurementCamera::fromJson(o["camera"].toObject()),
        MeasurementSetupDescriptor::fromJson(o["setup"].toObject()),AcquisitionProfile::fromJson(o["profile"].toObject()),o["acquisition"].toObject()};
    e.validate();require(e.toJson()==o,"Experimental calibration evidence mismatch");return e;
}
}
