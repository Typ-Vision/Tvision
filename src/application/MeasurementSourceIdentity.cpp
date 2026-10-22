#include "MeasurementSourceIdentity.hpp"
#include <QDir>
#include <QSet>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void keys(const QJsonObject& o,const QSet<QString>& expected){require(o.size()==expected.size(),"Missing/unknown source identity fields");for(auto i=o.begin();i!=o.end();++i)require(expected.contains(i.key()),"Unknown source identity field");}
QString text(const QJsonObject& o,const char* k){require(o[k].isString(),"Expected source identity text");return o[k].toString();}
int integer(const QJsonObject& o,const char* k,int minimum=1){const auto n=o[k];require(n.isDouble() && n.toDouble()>=minimum && n.toDouble()<=1000000000 && n.toDouble()==n.toInt(),"Invalid source revision/generation");return n.toInt();}
void hash(const QString& s){require(s.size()==64,"Invalid raster hash");for(auto c:s)require((c>='0' && c<='9') || (c>='a' && c<='f'),"Invalid raster hash");}
void identity(const QString& s){CameraId{s}.validate();}
QString kindName(SourceIdentityKind k){switch(k){case SourceIdentityKind::RecipeTool:return "recipe_tool";case SourceIdentityKind::CalibrationSession:return "calibration_session";}throw std::runtime_error("Unknown source identity kind");}
SourceIdentityKind kind(const QString& s){if(s=="recipe_tool")return SourceIdentityKind::RecipeTool;if(s=="calibration_session")return SourceIdentityKind::CalibrationSession;throw std::runtime_error("Unknown source identity kind");}
}
void SetupReference::validate() const {camera.validate();setup.validate();require(revision>0 && revision<=1000000000,"Invalid target revision");}
QJsonObject SetupReference::toJson() const {validate();return {{"schemaVersion",1},{"cameraId",camera.value},{"setupId",setup.value},{"revision",revision}};}
SetupReference SetupReference::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","cameraId","setupId","revision"});require(integer(o,"schemaVersion")==1,"Unknown target schema");SetupReference r{{text(o,"cameraId")},{text(o,"setupId")},integer(o,"revision")};r.validate();return r;}
void MeasurementSourceEvidence::validate() const {
    sourceOriginName(origin);kindName(sourceKind);identity(sourceId);require(generation>=0 && generation<=1000000000,"Invalid source generation");if(target)target->validate();
    if(experimental){experimental->validate();require(experimental->origin==origin && target==std::optional(SetupReference{experimental->camera.id,experimental->setup.id,experimental->setup.revision}) && experimental->acquisition["contentId"]==pixelHash,"Experimental source target/hash mismatch");}
    if(origin==SourceOrigin::LegacyRecord){require(generation==0 && filePath.isEmpty(),"Legacy evidence cannot invent an acquisition generation/path");if(!frameId.isEmpty())identity(frameId);if(!pixelHash.isEmpty())hash(pixelHash);require(cycleId.isEmpty() && coordinates.isEmpty(),"Legacy acquisition metadata not recorded by this schema");return;}
    require(generation>0,"Missing source generation");identity(frameId);identity(cycleId);identity(coordinates);hash(pixelHash);
    if(origin==SourceOrigin::CameraAcquired)require(target.has_value() && filePath.isEmpty(),"Camera evidence requires target and cannot be a file");
    else {require(QDir::isAbsolutePath(filePath) && filePath.size()<=32768 && !filePath.contains(QChar::Null),"File evidence requires absolute path");require((origin==SourceOrigin::SetupFile)==target.has_value(),"Trial/setup file target mismatch");}
}
QJsonObject MeasurementSourceEvidence::toJson() const {validate();QJsonObject base{{"schemaVersion",1},{"origin",sourceOriginName(origin)},{"sourceKind",kindName(sourceKind)},{"sourceId",sourceId},{"frameId",frameId},{"cycleId",cycleId},{"coordinates",coordinates},{"pixelHash",pixelHash},{"filePath",filePath},{"generation",generation},{"target",target ? QJsonValue(target->toJson()):QJsonValue()}};if(experimental)return {{"schemaVersion",2},{"source",base},{"experimental",experimental->toJson()}};return base;}
MeasurementSourceEvidence MeasurementSourceEvidence::fromJson(const QJsonObject& o){if(o["schemaVersion"]==2){keys(o,{"schemaVersion","source","experimental"});require(o["source"].isObject() && o["experimental"].isObject() && o["source"].toObject()["schemaVersion"]==1,"Invalid experimental source envelope");auto e=fromJson(o["source"].toObject());e.experimental=ExperimentalCalibrationEvidence::fromJson(o["experimental"].toObject());e.validate();return e;}keys(o,{"schemaVersion","origin","sourceKind","sourceId","frameId","cycleId","coordinates","pixelHash","filePath","generation","target"});require(integer(o,"schemaVersion")==1,"Unknown source evidence schema");MeasurementSourceEvidence e{sourceOriginFromName(text(o,"origin")),text(o,"sourceId"),text(o,"frameId"),text(o,"cycleId"),text(o,"coordinates"),text(o,"pixelHash"),text(o,"filePath"),integer(o,"generation",0),{},kind(text(o,"sourceKind"))};if(!o["target"].isNull()){require(o["target"].isObject(),"Invalid source target");e.target=SetupReference::fromJson(o["target"].toObject());}e.validate();return e;}
}
