#include "MeasurementSetupContract.hpp"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void text(const QString& s){require(!s.trimmed().isEmpty() && s==s.trimmed() && s.size()<=512 && !s.contains(QChar::Null),"Invalid measurement identity/text");}
void requireRevision(int n){require(n>0 && n<=1000000000,"Invalid measurement revision");}
void keys(const QJsonObject& o,const QSet<QString>& expected){require(o.size()==expected.size(),"Missing/unknown measurement fields");for(auto i=o.begin();i!=o.end();++i)require(expected.contains(i.key()),"Unknown measurement field");}
QString string(const QJsonObject& o,const char* k){require(o[k].isString(),"Expected measurement string");return o[k].toString();}
int integer(const QJsonObject& o,const char* k,int minimum=1){const auto v=o[k];require(v.isDouble() && v.toDouble()>=minimum && v.toDouble()<=1000000000 && v.toDouble()==v.toInt(),"Expected bounded measurement integer");return v.toInt();}
bool boolean(const QJsonObject& o,const char* k){require(o[k].isBool(),"Expected measurement boolean");return o[k].toBool();}
QJsonObject object(const QJsonObject& o,const char* k){require(o[k].isObject(),"Expected measurement object");return o[k].toObject();}
void schema(const QJsonObject& o){require(integer(o,"schemaVersion")==1,"Unsupported measurement schema");}
QString digest(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
QString confidenceName(DeviceIdentityConfidence c){switch(c){case DeviceIdentityConfidence::Unavailable:return "unavailable";case DeviceIdentityConfidence::ProviderReported:return "provider_reported";case DeviceIdentityConfidence::SerialReported:return "serial_reported";}throw std::runtime_error("Unknown device identity confidence");}
DeviceIdentityConfidence parseConfidence(const QString& s){for(auto c:{DeviceIdentityConfidence::Unavailable,DeviceIdentityConfidence::ProviderReported,DeviceIdentityConfidence::SerialReported})if(confidenceName(c)==s)return c;throw std::runtime_error("Unknown device identity confidence");}
}
void CameraId::validate() const {text(value);}
void SetupId::validate() const {text(value);}
QJsonObject DeviceCapabilities::toJson() const {return {{"softwareCapture",softwareCapture},{"resolution",resolution},{"sensorRoi",sensorRoi},{"binning",binning}};}
DeviceCapabilities DeviceCapabilities::fromJson(const QJsonObject& o){keys(o,{"softwareCapture","resolution","sensorRoi","binning"});return {boolean(o,"softwareCapture"),boolean(o,"resolution"),boolean(o,"sensorRoi"),boolean(o,"binning")};}
void DeviceBinding::validate() const {
    require(adapter=="webcam" || adapter=="gentl","Unknown physical adapter");requireRevision(this->revision);confidenceName(this->confidence);
    if(confidence==DeviceIdentityConfidence::Unavailable)require(providerId.isEmpty(),"Unavailable identity cannot claim provider ID");else text(providerId);
    require(adapter!="gentl" || !capabilities.softwareCapture,"Industrial grab not supported");
}
QJsonObject DeviceBinding::toJson() const {validate();return {{"schemaVersion",1},{"adapter",adapter},{"providerId",providerId},{"confidence",confidenceName(confidence)},{"revision",this->revision},{"capabilities",capabilities.toJson()}};}
DeviceBinding DeviceBinding::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","adapter","providerId","confidence","revision","capabilities"});schema(o);DeviceBinding d{string(o,"adapter"),string(o,"providerId"),parseConfidence(string(o,"confidence")),integer(o,"revision"),DeviceCapabilities::fromJson(object(o,"capabilities"))};d.validate();return d;}
int DeviceBinding::resolve(const QVector<DeviceBinding>& discovered) const {
    validate();require(confidence!=DeviceIdentityConfidence::Unavailable,"Device must be explicitly remapped");int found=-1;
    for(int i=0;i<discovered.size();++i){const auto& d=discovered[i];d.validate();if(d.adapter==adapter && d.providerId==providerId && d.confidence==confidence){require(found==-1,"Ambiguous device identity");found=i;}}
    require(found!=-1,"Device offline or unmapped");require(discovered[found].capabilities==capabilities,"Device capabilities changed; remap required");return found;
}
DeviceBinding DeviceBinding::rebound(const DeviceBinding& next) const {validate();next.validate();auto result=next;result.revision=this->revision;if(adapter!=next.adapter || providerId!=next.providerId || confidence!=next.confidence || capabilities!=next.capabilities){require(this->revision<1000000000,"Device revision exhausted");++result.revision;}return result;}
void GeometryDescriptor::validate() const {
    require(resolution.width()>0 && resolution.height()>0 && resolution.width()<=1000000 && resolution.height()<=1000000,"Invalid raw geometry resolution");
    if(sensorRoi)require(sensorRoi->x()>=0 && sensorRoi->y()>=0 && sensorRoi->width()>0 && sensorRoi->height()>0 && qint64(sensorRoi->x())+sensorRoi->width()<=1000000000 && qint64(sensorRoi->y())+sensorRoi->height()<=1000000000,"Invalid sensor ROI");
    if(binning)require(binning->width()>0 && binning->height()>0 && binning->width()<=1024 && binning->height()<=1024,"Invalid binning");
    if(sensorRoi && binning)require(qint64(resolution.width())*binning->width()==sensorRoi->width() && qint64(resolution.height())*binning->height()==sensorRoi->height(),"Inconsistent sensor ROI/binning geometry");
}
QJsonObject GeometryDescriptor::toJson() const {
    validate();QJsonValue roi,bin;
    if(sensorRoi)roi=QJsonObject{{"x",sensorRoi->x()},{"y",sensorRoi->y()},{"width",sensorRoi->width()},{"height",sensorRoi->height()}};
    if(binning)bin=QJsonObject{{"x",binning->width()},{"y",binning->height()}};
    return {{"schemaVersion",1},{"width",resolution.width()},{"height",resolution.height()},{"sensorRoi",roi},{"binning",bin},{"complete",complete}};
}
GeometryDescriptor GeometryDescriptor::fromJson(const QJsonObject& o){
    keys(o,{"schemaVersion","width","height","sensorRoi","binning","complete"});schema(o);GeometryDescriptor g{{integer(o,"width"),integer(o,"height")},{},{},boolean(o,"complete")};
    if(!o["sensorRoi"].isNull()){const auto r=object(o,"sensorRoi");keys(r,{"x","y","width","height"});g.sensorRoi=QRect(integer(r,"x",0),integer(r,"y",0),integer(r,"width"),integer(r,"height"));}
    if(!o["binning"].isNull()){const auto b=object(o,"binning");keys(b,{"x","y"});g.binning=QSize(integer(b,"x"),integer(b,"y"));}g.validate();return g;
}
void MeasurementCamera::validate() const {id.validate();text(name);device.validate();}
QJsonObject MeasurementCamera::toJson() const {validate();return {{"schemaVersion",1},{"id",id.value},{"name",name},{"device",device.toJson()}};}
MeasurementCamera MeasurementCamera::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","id","name","device"});schema(o);MeasurementCamera c{{string(o,"id")},string(o,"name"),DeviceBinding::fromJson(object(o,"device"))};c.validate();return c;}
void MeasurementSetupDescriptor::validate() const {id.validate();camera.validate();text(name);text(opticalProfile);text(workingPlane);requireRevision(this->revision);requireRevision(deviceRevision);geometry.validate();}
QJsonObject MeasurementSetupDescriptor::toJson() const {validate();return {{"schemaVersion",1},{"id",id.value},{"cameraId",camera.value},{"name",name},{"opticalProfile",opticalProfile},{"workingPlane",workingPlane},{"revision",this->revision},{"deviceRevision",deviceRevision},{"geometry",geometry.toJson()}};}
MeasurementSetupDescriptor MeasurementSetupDescriptor::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","id","cameraId","name","opticalProfile","workingPlane","revision","deviceRevision","geometry"});schema(o);MeasurementSetupDescriptor s{{string(o,"id")},{string(o,"cameraId")},string(o,"name"),string(o,"opticalProfile"),string(o,"workingPlane"),integer(o,"revision"),integer(o,"deviceRevision"),GeometryDescriptor::fromJson(object(o,"geometry"))};s.validate();return s;}
MeasurementSetupDescriptor MeasurementSetupDescriptor::revised(const MeasurementSetupDescriptor& next) const {
    validate();next.validate();require(id==next.id && camera==next.camera,"Setup cannot silently change camera/identity");require(next.deviceRevision>=deviceRevision,"Setup cannot restore an older device mapping");auto result=next;result.revision=this->revision;
    if(opticalProfile!=next.opticalProfile || workingPlane!=next.workingPlane || deviceRevision!=next.deviceRevision || geometry!=next.geometry){require(this->revision<1000000000,"Setup revision exhausted");++result.revision;}return result;
}
QString MeasurementSetupDescriptor::geometryId() const {validate();require(geometry.complete,"Unknown critical geometry is not compatible");auto o=toJson();o.remove("name");return digest(o);}
QString MeasurementSetupDescriptor::cacheId(const QJsonObject& settings) const {return digest({{"geometryId",geometryId()},{"acquisitionSettings",settings}});}
void MeasurementSetupContract::validate(int capacity) const {
    require(capacity>0 && capacity<=4096 && cameras.size()<=capacity && setups.size()<=capacity,"Measurement capacity exceeded");QSet<QString> ids,setupIds,devices;
    for(const auto& c:cameras){c.validate();require(!ids.contains(c.id.value),"Duplicate camera ID");ids.insert(c.id.value);if(c.device.confidence!=DeviceIdentityConfidence::Unavailable){const auto key=digest({{"adapter",c.device.adapter},{"providerId",c.device.providerId}});require(!devices.contains(key),"Ambiguous physical camera mapping");devices.insert(key);}}
    for(const auto& s:setups){s.validate();require(!setupIds.contains(s.id.value) && ids.contains(s.camera.value),"Duplicate/dangling setup ID");setupIds.insert(s.id.value);for(const auto& c:cameras)if(c.id==s.camera){require(s.deviceRevision==c.device.revision,"Stale setup device revision");require(!s.geometry.complete || c.device.capabilities.resolution,"Unverified adapter resolution");require((!s.geometry.sensorRoi || c.device.capabilities.sensorRoi) && (!s.geometry.binning || c.device.capabilities.binning),"Unsupported adapter geometry");}}
}
QJsonObject MeasurementSetupContract::toJson(int capacity) const {validate(capacity);QJsonArray cs,ss;for(const auto& c:cameras)cs.append(c.toJson());for(const auto& s:setups)ss.append(s.toJson());return {{"format","visionflow.measurement-setup-contract"},{"schemaVersion",1},{"cameras",cs},{"setups",ss}};}
MeasurementSetupContract MeasurementSetupContract::fromJson(const QJsonObject& o,int capacity){keys(o,{"format","schemaVersion","cameras","setups"});schema(o);require(o["format"]=="visionflow.measurement-setup-contract" && o["cameras"].isArray() && o["setups"].isArray(),"Invalid measurement contract");require(capacity>0 && capacity<=4096 && o["cameras"].toArray().size()<=capacity && o["setups"].toArray().size()<=capacity,"Measurement capacity exceeded");MeasurementSetupContract result;for(const auto& c:o["cameras"].toArray()){require(c.isObject(),"Invalid camera entry");result.cameras.append(MeasurementCamera::fromJson(c.toObject()));}for(const auto& s:o["setups"].toArray()){require(s.isObject(),"Invalid setup entry");result.setups.append(MeasurementSetupDescriptor::fromJson(s.toObject()));}result.validate(capacity);return result;}
QString sourceOriginName(SourceOrigin o){switch(o){case SourceOrigin::CameraAcquired:return "camera_acquired";case SourceOrigin::SetupFile:return "setup_file";case SourceOrigin::TrialFile:return "trial_file";case SourceOrigin::LegacyRecord:return "legacy_record";}throw std::runtime_error("Unknown source origin");}
SourceOrigin sourceOriginFromName(const QString& s){for(auto o:{SourceOrigin::CameraAcquired,SourceOrigin::SetupFile,SourceOrigin::TrialFile,SourceOrigin::LegacyRecord})if(sourceOriginName(o)==s)return o;throw std::runtime_error("Unknown source origin");}
}
