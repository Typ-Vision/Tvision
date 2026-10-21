#include "MeasurementSetupContract.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <cmath>
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
void industrialBackend(const QString& s){require(s=="oecs" || s=="gentl","Unknown industrial backend");}
double number(const QJsonObject& o,const char* k){require(o[k].isDouble() && std::isfinite(o[k].toDouble()),"Expected finite measurement number");return o[k].toDouble();}
QString autoName(AutoMode m){switch(m){case AutoMode::Off:return "off";case AutoMode::Once:return "once";case AutoMode::Continuous:return "continuous";}throw std::runtime_error("Unknown auto mode");}
AutoMode parseAuto(const QString& s){for(auto m:{AutoMode::Off,AutoMode::Once,AutoMode::Continuous})if(autoName(m)==s)return m;throw std::runtime_error("Unknown auto mode");}
QString evidenceName(GeometryEvidence e){switch(e){case GeometryEvidence::Reported:return "reported";case GeometryEvidence::NotApplicable:return "not_applicable";case GeometryEvidence::Unknown:return "unknown";}throw std::runtime_error("Unknown geometry evidence");}
GeometryEvidence parseEvidence(const QString& s){for(auto e:{GeometryEvidence::Reported,GeometryEvidence::NotApplicable,GeometryEvidence::Unknown})if(evidenceName(e)==s)return e;throw std::runtime_error("Unknown geometry evidence");}
template<class T> QJsonObject feature(GeometryEvidence e,T x,T y){QJsonObject o{{"evidence",evidenceName(e)}};if(e==GeometryEvidence::Reported){o["x"]=x;o["y"]=y;}return o;}
GeometryEvidence featureEvidence(const QJsonObject& f){const auto e=parseEvidence(string(f,"evidence"));if(e==GeometryEvidence::Reported)keys(f,{"evidence","x","y"});else keys(f,{"evidence"});return e;}
// Profile rows of the invalidation table, shared by acquisitionChange and AcquisitionProfile::revised.
void settingsRows(const AcquisitionSettings& a,const AcquisitionSettings& b,AcquisitionChange& c){c.pixelFormat=a.pixelFormat!=b.pixelFormat;auto same=a;same.pixelFormat=b.pixelFormat;c.acquisition=same!=b;}
// Field changes generate a revision; compatibility also compares that revision.
bool setupFieldsChanged(const MeasurementSetupDescriptor& x,const MeasurementSetupDescriptor& y){
    return x.id!=y.id || x.camera!=y.camera || x.opticalProfile!=y.opticalProfile || x.workingPlane!=y.workingPlane || x.deviceRevision!=y.deviceRevision;
}
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
void PhysicalDeviceIdentity::validate() const {
    confidenceName(confidence);if(confidence==DeviceIdentityConfidence::Unavailable){require(vendor.isEmpty() && model.isEmpty() && serial.isEmpty(),"Unavailable identity cannot claim device values");return;}
    text(vendor);text(model);if(confidence==DeviceIdentityConfidence::SerialReported)text(serial);else require(serial.isEmpty(),"Reported serial requires serial confidence");
}
QJsonObject PhysicalDeviceIdentity::toJson() const {validate();return {{"schemaVersion",1},{"vendor",vendor},{"model",model},{"serial",serial},{"confidence",confidenceName(confidence)}};}
PhysicalDeviceIdentity PhysicalDeviceIdentity::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","vendor","model","serial","confidence"});schema(o);PhysicalDeviceIdentity p{string(o,"vendor"),string(o,"model"),string(o,"serial"),parseConfidence(string(o,"confidence"))};p.validate();return p;}
void TransportLocator::validate() const {
    industrialBackend(backend);text(deviceId);if(!interfaceId.isEmpty())text(interfaceId);
    if(backend=="oecs")require(producer.isEmpty(),"Direct SDK route has no producer");else{text(producer);require(QDir::isAbsolutePath(producer) && producer==QDir::cleanPath(producer),"Producer route must be canonical absolute path");}
}
QJsonObject TransportLocator::toJson() const {validate();return {{"schemaVersion",1},{"backend",backend},{"producer",producer},{"interfaceId",interfaceId},{"deviceId",deviceId}};}
TransportLocator TransportLocator::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","backend","producer","interfaceId","deviceId"});schema(o);TransportLocator t{string(o,"backend"),string(o,"producer"),string(o,"interfaceId"),string(o,"deviceId")};t.validate();return t;}
void BackendIdentity::validate() const {
    industrialBackend(adapter);requireRevision(adapterVersion);text(runtimeVersion);require(runtimeSha256.size()==64,"Backend runtime hash required");for(auto c:runtimeSha256)require((c>='0' && c<='9') || (c>='a' && c<='f'),"Backend runtime hash must be lowercase SHA-256");
}
QJsonObject BackendIdentity::toJson() const {validate();return {{"schemaVersion",1},{"adapter",adapter},{"adapterVersion",adapterVersion},{"runtimeVersion",runtimeVersion},{"runtimeSha256",runtimeSha256}};}
BackendIdentity BackendIdentity::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","adapter","adapterVersion","runtimeVersion","runtimeSha256"});schema(o);BackendIdentity b{string(o,"adapter"),string(o,"runtimeVersion"),string(o,"runtimeSha256"),integer(o,"adapterVersion")};b.validate();return b;}
QString BackendIdentity::fingerprint() const {return digest(toJson());}
void DiscoveredDevice::validate() const {identity.validate();route.validate();backend.validate();require(route.backend==backend.adapter,"Route/backend mismatch");}
QVector<int> resolveRoutes(const PhysicalDeviceIdentity& wanted,const QVector<DiscoveredDevice>& discovered){
    wanted.validate();require(wanted.usable(),"Device must be explicitly remapped");QVector<int> found;QSet<QString> enumerations;
    for(int i=0;i<discovered.size();++i){const auto& d=discovered[i];d.validate();if(d.identity==wanted){const auto key=d.route.backend+QChar(0)+d.route.producer;require(!enumerations.contains(key),"Ambiguous device identity");enumerations.insert(key);found.append(i);}}
    require(!found.isEmpty(),"Device offline or unmapped");return found;
}
void GeometryDescriptor::validate() const {
    require(resolution.width()>0 && resolution.height()>0 && resolution.width()<=1000000 && resolution.height()<=1000000,"Invalid raw geometry resolution");
    if(sensorRoi)require(sensorRoi->x()>=0 && sensorRoi->y()>=0 && sensorRoi->width()>0 && sensorRoi->height()>0 && qint64(sensorRoi->x())+sensorRoi->width()<=1000000000 && qint64(sensorRoi->y())+sensorRoi->height()<=1000000000,"Invalid sensor ROI");
    if(binning)require(binning->width()>0 && binning->height()>0 && binning->width()<=1024 && binning->height()<=1024,"Invalid binning");
    require(decimation.has_value()==reverse.has_value() && (!decimation || (sensorRoi && binning)),"Industrial geometry must state every critical feature");
    if(decimation)require(decimation->width()>0 && decimation->height()>0 && decimation->width()<=1024 && decimation->height()<=1024,"Invalid decimation");
    const auto dx=decimation ? decimation->width():1,dy=decimation ? decimation->height():1;
    if(sensorRoi && binning)require(qint64(resolution.width())*binning->width()*dx==sensorRoi->width() && qint64(resolution.height())*binning->height()*dy==sensorRoi->height(),"Inconsistent sensor ROI/binning geometry");
}
QJsonObject GeometryDescriptor::toJson() const {
    validate();QJsonValue roi,bin;
    if(sensorRoi)roi=QJsonObject{{"x",sensorRoi->x()},{"y",sensorRoi->y()},{"width",sensorRoi->width()},{"height",sensorRoi->height()}};
    if(binning)bin=QJsonObject{{"x",binning->width()},{"y",binning->height()}};
    QJsonObject o{{"schemaVersion",critical() ? 2:1},{"width",resolution.width()},{"height",resolution.height()},{"sensorRoi",roi},{"binning",bin},{"complete",complete}};
    if(critical()){o["decimation"]=QJsonObject{{"x",decimation->width()},{"y",decimation->height()}};o["reverse"]=QJsonObject{{"x",reverse->x},{"y",reverse->y}};}return o;
}
GeometryDescriptor GeometryDescriptor::fromJson(const QJsonObject& o){
    const int version=integer(o,"schemaVersion");require(version==1 || version==2,"Unsupported measurement schema");
    if(version==1)keys(o,{"schemaVersion","width","height","sensorRoi","binning","complete"});else keys(o,{"schemaVersion","width","height","sensorRoi","binning","complete","decimation","reverse"});
    GeometryDescriptor g{{integer(o,"width"),integer(o,"height")},{},{},boolean(o,"complete")};
    if(!o["sensorRoi"].isNull()){const auto r=object(o,"sensorRoi");keys(r,{"x","y","width","height"});g.sensorRoi=QRect(integer(r,"x",0),integer(r,"y",0),integer(r,"width"),integer(r,"height"));}
    if(!o["binning"].isNull()){const auto b=object(o,"binning");keys(b,{"x","y"});g.binning=QSize(integer(b,"x"),integer(b,"y"));}
    if(version==2){const auto d=object(o,"decimation"),r=object(o,"reverse");keys(d,{"x","y"});keys(r,{"x","y"});g.decimation=QSize(integer(d,"x"),integer(d,"y"));g.reverse=GeometryMirror{boolean(r,"x"),boolean(r,"y")};}g.validate();return g;
}
void MeasurementCamera::validate() const {
    id.validate();text(name);device.validate();
    if(physical){physical->validate();require(device.adapter=="gentl" && device.confidence==DeviceIdentityConfidence::Unavailable,"Industrial camera identity belongs to physical identity");}
}
QJsonObject MeasurementCamera::toJson() const {validate();QJsonObject o{{"schemaVersion",physical ? 2:1},{"id",id.value},{"name",name},{"device",device.toJson()}};if(physical)o["physical"]=physical->toJson();return o;}
MeasurementCamera MeasurementCamera::fromJson(const QJsonObject& o){
    const int version=integer(o,"schemaVersion");require(version==1 || version==2,"Unsupported measurement schema");
    if(version==1)keys(o,{"schemaVersion","id","name","device"});else keys(o,{"schemaVersion","id","name","device","physical"});
    MeasurementCamera c{{string(o,"id")},string(o,"name"),DeviceBinding::fromJson(object(o,"device"))};if(version==2)c.physical=PhysicalDeviceIdentity::fromJson(object(o,"physical"));c.validate();return c;
}
MeasurementCamera MeasurementCamera::rebound(const MeasurementCamera& next) const {
    validate();next.validate();require(id==next.id,"Camera identity cannot change");auto result=next;result.device.revision=device.revision;auto before=device;before.revision=next.device.revision;
    if(before!=next.device || physical!=next.physical){require(device.revision<1000000000,"Device revision exhausted");++result.device.revision;}return result;
}
void MeasurementSetupDescriptor::validate() const {
    id.validate();camera.validate();text(name);requireRevision(revision);requireRevision(deviceRevision);
    if(pending){require(opticalProfile.isEmpty() && workingPlane.isEmpty() && geometry==GeometryDescriptor{},"Pending setup cannot claim physical geometry");return;}
    text(opticalProfile);text(workingPlane);geometry.validate();
}
QJsonObject MeasurementSetupDescriptor::toJson() const {validate();return {{"schemaVersion",pending ? 2:1},{"id",id.value},{"cameraId",camera.value},{"name",name},{"opticalProfile",opticalProfile},{"workingPlane",workingPlane},{"revision",revision},{"deviceRevision",deviceRevision},{"geometry",pending ? QJsonValue():QJsonValue(geometry.toJson())}};}
MeasurementSetupDescriptor MeasurementSetupDescriptor::fromJson(const QJsonObject& o){
    keys(o,{"schemaVersion","id","cameraId","name","opticalProfile","workingPlane","revision","deviceRevision","geometry"});
    const int version=integer(o,"schemaVersion");require(version==1 || version==2,"Unsupported setup schema");
    MeasurementSetupDescriptor s{{string(o,"id")},{string(o,"cameraId")},string(o,"name"),string(o,"opticalProfile"),string(o,"workingPlane"),integer(o,"revision"),integer(o,"deviceRevision"),{}};
    s.pending=version==2;if(s.pending)require(o["geometry"].isNull(),"Pending geometry must be null");else s.geometry=GeometryDescriptor::fromJson(object(o,"geometry"));s.validate();return s;
}
MeasurementSetupDescriptor MeasurementSetupDescriptor::revised(const MeasurementSetupDescriptor& next) const {
    validate();next.validate();require(id==next.id && camera==next.camera,"Setup cannot silently change camera/identity");require(next.deviceRevision>=deviceRevision,"Setup cannot restore an older device mapping");auto result=next;result.revision=this->revision;
    if(pending!=next.pending || setupFieldsChanged(*this,next) || geometry!=next.geometry){require(this->revision<1000000000,"Setup revision exhausted");++result.revision;}return result;
}
QString MeasurementSetupDescriptor::geometryId() const {validate();require(!pending && geometry.complete,"Unknown critical geometry is not compatible");auto o=toJson();o.remove("name");return digest(o);}
QString MeasurementSetupDescriptor::cacheId(const QJsonObject& settings) const {return digest({{"geometryId",geometryId()},{"acquisitionSettings",settings}});}
void ProfileId::validate() const {text(value);}
void AcquisitionSettings::validate() const {
    text(pixelFormat);autoName(exposureAuto);autoName(gainAuto);if(!illumination.isEmpty())text(illumination);
    if(trigger.enabled){text(trigger.selector);text(trigger.source);}else require(trigger.selector.isEmpty() && trigger.source.isEmpty(),"Disabled trigger cannot claim selector/source");
    require(std::isfinite(exposureTimeUs) && exposureTimeUs>0 && exposureTimeUs<=100000000,"Invalid exposure time");require(std::isfinite(gain) && gain>=-1000 && gain<=1000,"Invalid gain");
}
QJsonObject AcquisitionSettings::toJson() const {validate();return {{"schemaVersion",1},{"pixelFormat",pixelFormat},{"trigger",QJsonObject{{"enabled",trigger.enabled},{"selector",trigger.selector},{"source",trigger.source}}},{"exposureAuto",autoName(exposureAuto)},{"exposureTimeUs",exposureTimeUs},{"gainAuto",autoName(gainAuto)},{"gain",gain},{"illumination",illumination}};}
AcquisitionSettings AcquisitionSettings::fromJson(const QJsonObject& o){
    keys(o,{"schemaVersion","pixelFormat","trigger","exposureAuto","exposureTimeUs","gainAuto","gain","illumination"});schema(o);const auto t=object(o,"trigger");keys(t,{"enabled","selector","source"});
    AcquisitionSettings s{string(o,"pixelFormat"),{boolean(t,"enabled"),string(t,"selector"),string(t,"source")},parseAuto(string(o,"exposureAuto")),number(o,"exposureTimeUs"),parseAuto(string(o,"gainAuto")),number(o,"gain"),string(o,"illumination")};s.validate();return s;
}
void AcquisitionProfile::validate() const {
    id.validate();camera.validate();setup.validate();text(name);requireRevision(this->revision);
    if(pending){require(requested==AcquisitionSettings{},"Pending profile cannot claim settings");return;}
    requested.validate();
    require(requested.pixelFormat=="Mono8","First industrial profile is Mono8 only");require(requested.exposureAuto==AutoMode::Off && requested.gainAuto==AutoMode::Off,"Industrial profile requires manual exposure/gain");
}
QJsonObject AcquisitionProfile::toJson() const {validate();return {{"schemaVersion",pending ? 2:1},{"id",id.value},{"cameraId",camera.value},{"setupId",setup.value},{"name",name},{"revision",this->revision},{"requested",pending ? QJsonValue():QJsonValue(requested.toJson())}};}
AcquisitionProfile AcquisitionProfile::fromJson(const QJsonObject& o){
    keys(o,{"schemaVersion","id","cameraId","setupId","name","revision","requested"});const int version=integer(o,"schemaVersion");require(version==1 || version==2,"Unsupported profile schema");
    AcquisitionProfile p{{string(o,"id")},{string(o,"cameraId")},{string(o,"setupId")},string(o,"name"),integer(o,"revision"),{}};p.pending=version==2;
    if(p.pending)require(o["requested"].isNull(),"Pending settings must be null");else p.requested=AcquisitionSettings::fromJson(object(o,"requested"));p.validate();return p;
}
AcquisitionProfile AcquisitionProfile::revised(const AcquisitionProfile& next) const {
    validate();next.validate();require(id==next.id && camera==next.camera && setup==next.setup,"Profile cannot silently change camera/setup/identity");auto result=next;result.revision=this->revision;
    AcquisitionChange change;settingsRows(requested,next.requested,change);if(pending!=next.pending || !change.cacheCompatible()){require(this->revision<1000000000,"Profile revision exhausted");++result.revision;}return result;
}
QString AcquisitionProfile::acquisitionId(const MeasurementSetupDescriptor& s) const {validate();require(!pending,"Acquisition settings await readback");require(s.id==setup && s.camera==camera,"Profile/setup mismatch");return s.cacheId(requested.toJson());}
void GeometryReadback::validate() const {
    require(resolution.width()>0 && resolution.height()>0 && resolution.width()<=1000000 && resolution.height()<=1000000,"Invalid readback resolution");
    for(auto e:{offsetEvidence,binningEvidence,decimationEvidence,reverseEvidence})evidenceName(e);
    require(offset.x()>=0 && offset.y()>=0 && offset.x()<=1000000000 && offset.y()<=1000000000,"Invalid readback offset");
    for(const auto& s:{binning,decimation})require(s.width()>0 && s.height()>0 && s.width()<=1024 && s.height()<=1024,"Invalid readback binning/decimation");
    require((offsetEvidence==GeometryEvidence::Reported || offset==QPoint()) && (binningEvidence==GeometryEvidence::Reported || binning==QSize(1,1)) && (decimationEvidence==GeometryEvidence::Reported || decimation==QSize(1,1)) && (reverseEvidence==GeometryEvidence::Reported || reverse==GeometryMirror{}),"Unreported geometry cannot claim a value");
    require(qint64(resolution.width())*binning.width()*decimation.width()+offset.x()<=1000000000 && qint64(resolution.height())*binning.height()*decimation.height()+offset.y()<=1000000000,"Readback geometry overflow");
}
QJsonObject GeometryReadback::toJson() const {
    validate();return {{"schemaVersion",1},{"width",resolution.width()},{"height",resolution.height()},{"offset",feature(offsetEvidence,offset.x(),offset.y())},{"binning",feature(binningEvidence,binning.width(),binning.height())},{"decimation",feature(decimationEvidence,decimation.width(),decimation.height())},{"reverse",feature(reverseEvidence,reverse.x,reverse.y)}};
}
GeometryReadback GeometryReadback::fromJson(const QJsonObject& o){
    keys(o,{"schemaVersion","width","height","offset","binning","decimation","reverse"});schema(o);GeometryReadback g;g.resolution={integer(o,"width"),integer(o,"height")};
    const auto offset=object(o,"offset"),bin=object(o,"binning"),dec=object(o,"decimation"),rev=object(o,"reverse");
    if((g.offsetEvidence=featureEvidence(offset))==GeometryEvidence::Reported)g.offset={integer(offset,"x",0),integer(offset,"y",0)};
    if((g.binningEvidence=featureEvidence(bin))==GeometryEvidence::Reported)g.binning={integer(bin,"x"),integer(bin,"y")};
    if((g.decimationEvidence=featureEvidence(dec))==GeometryEvidence::Reported)g.decimation={integer(dec,"x"),integer(dec,"y")};
    if((g.reverseEvidence=featureEvidence(rev))==GeometryEvidence::Reported)g.reverse={boolean(rev,"x"),boolean(rev,"y")};g.validate();return g;
}
GeometryDescriptor GeometryReadback::descriptor() const {
    validate();for(auto e:{offsetEvidence,binningEvidence,decimationEvidence,reverseEvidence})require(e!=GeometryEvidence::Unknown,"Unknown critical geometry is not compatible");
    GeometryDescriptor g{resolution,QRect(offset,QSize(resolution.width()*binning.width()*decimation.width(),resolution.height()*binning.height()*decimation.height())),binning,true,decimation,reverse};g.validate();return g;
}
void AcquisitionReadback::validate() const {geometry.validate();settings.validate();}
QJsonObject AcquisitionReadback::toJson() const {validate();return {{"schemaVersion",1},{"geometry",geometry.toJson()},{"settings",settings.toJson()}};}
AcquisitionReadback AcquisitionReadback::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","geometry","settings"});schema(o);AcquisitionReadback r{GeometryReadback::fromJson(object(o,"geometry")),AcquisitionSettings::fromJson(object(o,"settings"))};r.validate();return r;}
AcquisitionChange acquisitionChange(const AcquisitionBasis& before,const AcquisitionBasis& after){
    AcquisitionChange change;for(const auto* b:{&before,&after}){b->setup.validate();if(b->profile){b->profile->requested.validate();require(b->profile->setup==b->setup.id && b->profile->camera==b->setup.camera,"Profile/setup mismatch");}}
    const auto& x=before.setup;const auto& y=after.setup;
    change.setup=setupFieldsChanged(x,y) || x.revision!=y.revision;
    change.geometry=x.geometry!=y.geometry;
    if(before.profile && after.profile)settingsRows(before.profile->requested,after.profile->requested,change);else if(before.profile || after.profile)change.pixelFormat=change.acquisition=true;
    change.backend=before.backend!=after.backend;return change;
}
QString verifyApplied(const MeasurementSetupDescriptor& setup,const AcquisitionProfile& profile,const AcquisitionReadback& readback){
    const auto id=profile.acquisitionId(setup);readback.validate();AcquisitionBasis applied{setup,profile};applied.setup.geometry=readback.geometry.descriptor();applied.profile->requested=readback.settings;
    // OECS float ABI rounds to binary32; keep actual values in the report and only
    // canonicalize comparison to the exact binary32 representation. Integers remain exact upstream.
    const auto represented=[](double a,double b){return a==b || a==double(float(b));};
    if(represented(readback.settings.exposureTimeUs,profile.requested.exposureTimeUs))applied.profile->requested.exposureTimeUs=profile.requested.exposureTimeUs;
    if(represented(readback.settings.gain,profile.requested.gain))applied.profile->requested.gain=profile.requested.gain;

    require(acquisitionChange({setup,profile},applied).cacheCompatible(),"Applied acquisition differs from requested profile");return id;
}
void MeasurementSetupContract::validate(int capacity) const {
    require(capacity>0 && capacity<=4096 && cameras.size()<=capacity && setups.size()<=capacity && profiles.size()<=capacity,"Measurement capacity exceeded");QSet<QString> ids,setupIds,devices,profileIds;
    for(const auto& c:cameras){c.validate();require(!ids.contains(c.id.value),"Duplicate camera ID");ids.insert(c.id.value);if(c.device.confidence!=DeviceIdentityConfidence::Unavailable){const auto key=digest({{"adapter",c.device.adapter},{"providerId",c.device.providerId}});require(!devices.contains(key),"Ambiguous physical camera mapping");devices.insert(key);}
        if(c.physical && c.physical->usable()){const auto key=digest(c.physical->toJson());require(!devices.contains(key),"Ambiguous physical camera mapping");devices.insert(key);}}
    for(const auto& s:setups){s.validate();require(!setupIds.contains(s.id.value) && ids.contains(s.camera.value),"Duplicate/dangling setup ID");setupIds.insert(s.id.value);for(const auto& c:cameras)if(c.id==s.camera){require(s.deviceRevision==c.device.revision,"Stale setup device revision");
            // Industrial geometry is proven by readback evidence, not by webcam adapter capabilities.
            if(s.pending)continue;
            if(c.physical){require(!s.geometry.complete || s.geometry.critical(),"Industrial geometry must state every critical feature");continue;}
            require(!s.geometry.critical(),"Legacy setup geometry is schema 1");require(!s.geometry.complete || c.device.capabilities.resolution,"Unverified adapter resolution");require((!s.geometry.sensorRoi || c.device.capabilities.sensorRoi) && (!s.geometry.binning || c.device.capabilities.binning),"Unsupported adapter geometry");}}
    for(const auto& p:profiles){p.validate();require(!profileIds.contains(p.id.value),"Duplicate profile ID");profileIds.insert(p.id.value);bool bound=false;for(const auto& c:cameras)for(const auto& s:setups)if(c.id==p.camera && s.id==p.setup && s.camera==c.id && (c.physical || p.pending))bound=true;require(bound,"Profile requires an industrial camera setup");}
}
QJsonObject MeasurementSetupContract::toJson(int capacity) const {
    validate(capacity);QJsonArray cs,ss,ps;for(const auto& c:cameras)cs.append(c.toJson());for(const auto& s:setups)ss.append(s.toJson());for(const auto& p:profiles)ps.append(p.toJson());
    QJsonObject o{{"format","visionflow.measurement-setup-contract"},{"schemaVersion",profiles.isEmpty() ? 1:2},{"cameras",cs},{"setups",ss}};if(!profiles.isEmpty())o["profiles"]=ps;return o;
}
MeasurementSetupContract MeasurementSetupContract::fromJson(const QJsonObject& o,int capacity){
    const int version=integer(o,"schemaVersion");require(version==1 || version==2,"Unsupported measurement schema");
    if(version==1)keys(o,{"format","schemaVersion","cameras","setups"});else keys(o,{"format","schemaVersion","cameras","setups","profiles"});
    require(o["format"]=="visionflow.measurement-setup-contract" && o["cameras"].isArray() && o["setups"].isArray() && (version==1 || (o["profiles"].isArray() && !o["profiles"].toArray().isEmpty())),"Invalid measurement contract");
    require(capacity>0 && capacity<=4096 && o["cameras"].toArray().size()<=capacity && o["setups"].toArray().size()<=capacity && o["profiles"].toArray().size()<=capacity,"Measurement capacity exceeded");MeasurementSetupContract result;
    for(const auto& c:o["cameras"].toArray()){require(c.isObject(),"Invalid camera entry");result.cameras.append(MeasurementCamera::fromJson(c.toObject()));}for(const auto& s:o["setups"].toArray()){require(s.isObject(),"Invalid setup entry");result.setups.append(MeasurementSetupDescriptor::fromJson(s.toObject()));}
    for(const auto& p:o["profiles"].toArray()){require(p.isObject(),"Invalid profile entry");result.profiles.append(AcquisitionProfile::fromJson(p.toObject()));}result.validate(capacity);return result;
}
QString sourceOriginName(SourceOrigin o){switch(o){case SourceOrigin::CameraAcquired:return "camera_acquired";case SourceOrigin::SetupFile:return "setup_file";case SourceOrigin::TrialFile:return "trial_file";case SourceOrigin::LegacyRecord:return "legacy_record";}throw std::runtime_error("Unknown source origin");}
SourceOrigin sourceOriginFromName(const QString& s){for(auto o:{SourceOrigin::CameraAcquired,SourceOrigin::SetupFile,SourceOrigin::TrialFile,SourceOrigin::LegacyRecord})if(sourceOriginName(o)==s)return o;throw std::runtime_error("Unknown source origin");}
}
