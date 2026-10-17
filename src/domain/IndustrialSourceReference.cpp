#include "IndustrialSourceReference.hpp"
#include <QSet>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int integer(const QJsonObject& o,const char* key){
    const auto v=o[key];require(v.isDouble() && v.toDouble()>=1 && v.toDouble()<=1000000000 && v.toDouble()==v.toInt(),"Invalid industrial source integer");return v.toInt();
}
QString text(const QJsonObject& o,const char* key){require(o[key].isString(),"Invalid industrial source string");return o[key].toString();}
struct Basis {MeasurementCamera camera;MeasurementSetupDescriptor setup;AcquisitionProfile profile;};
Basis basis(const MeasurementSetupContract& contract,const ProfileId& id){
    contract.validate();id.validate();
    for(const auto& p:contract.profiles)if(p.id==id)
        for(const auto& c:contract.cameras)if(c.id==p.camera)
            for(const auto& s:contract.setups)if(s.id==p.setup){
                require(c.physical && c.physical->usable(),"Industrial source requires exact physical identity");
                require(s.geometry.complete && s.geometry.critical(),"Industrial source requires complete geometry");
                const auto& t=p.requested.trigger;
                require(t.enabled && t.source=="Software" && (t.selector=="FrameStart" || t.selector=="FrameBurstStart"),"Industrial source requires explicit supported software trigger");
                return {c,s,p};
            }
    throw std::runtime_error("Industrial source profile missing");
}
}
void IndustrialSourceReference::validate() const {
    camera.validate();setup.validate();profile.validate();physical.validate();
    require(physical.usable(),"Industrial source physical identity unavailable");
    for(int revision:{deviceRevision,setupRevision,profileRevision})require(revision>=1 && revision<=1000000000,"Invalid industrial source revision");
    require(timeoutMs>=1 && timeoutMs<=600000,"Industrial source timeout outside 1..600000 ms");
    require(acquisitionId.size()==64,"Industrial source acquisition hash required");
    for(auto c:acquisitionId)require((c>='0' && c<='9') || (c>='a' && c<='f'),"Invalid industrial source acquisition hash");
}
QJsonObject IndustrialSourceReference::toJson() const {
    validate();return {{"format","visionflow.industrial-source-reference"},{"schemaVersion",1},{"kind","industrial_camera"},
        {"cameraId",camera.value},{"setupId",setup.value},{"profileId",profile.value},
        {"deviceRevision",deviceRevision},{"setupRevision",setupRevision},{"profileRevision",profileRevision},
        {"physical",physical.toJson()},{"acquisitionId",acquisitionId},{"timeoutMs",timeoutMs}};
}
IndustrialSourceReference IndustrialSourceReference::fromJson(const QJsonObject& o){
    const QSet<QString> keys{"format","schemaVersion","kind","cameraId","setupId","profileId","deviceRevision","setupRevision","profileRevision","physical","acquisitionId","timeoutMs"};
    require(o.size()==keys.size(),"Missing/unknown industrial source fields");
    for(auto i=o.begin();i!=o.end();++i)require(keys.contains(i.key()),"Unknown industrial source field");
    require(o["format"]=="visionflow.industrial-source-reference" && integer(o,"schemaVersion")==1 && o["kind"]=="industrial_camera","Unsupported industrial source format/kind/version");
    require(o["physical"].isObject(),"Industrial source physical object required");
    IndustrialSourceReference r{{text(o,"cameraId")},{text(o,"setupId")},{text(o,"profileId")},
        integer(o,"deviceRevision"),integer(o,"setupRevision"),integer(o,"profileRevision"),
        PhysicalDeviceIdentity::fromJson(o["physical"].toObject()),text(o,"acquisitionId"),integer(o,"timeoutMs")};
    r.validate();return r;
}
IndustrialSourceReference IndustrialSourceReference::bind(const MeasurementSetupContract& contract,const ProfileId& profile,int timeout){
    const auto b=basis(contract,profile);
    IndustrialSourceReference r{b.camera.id,b.setup.id,b.profile.id,b.camera.device.revision,b.setup.revision,b.profile.revision,
        *b.camera.physical,b.profile.acquisitionId(b.setup),timeout};
    r.validateAgainst(contract);return r;
}
void IndustrialSourceReference::validateAgainst(const MeasurementSetupContract& contract) const {
    validate();const auto b=basis(contract,profile);
    require(camera==b.camera.id && setup==b.setup.id && physical==*b.camera.physical,"Industrial source camera/setup/physical binding changed");
    require(deviceRevision==b.camera.device.revision && setupRevision==b.setup.revision && profileRevision==b.profile.revision,"Stale industrial source revision; explicit rebind required");
    require(acquisitionId==b.profile.acquisitionId(b.setup),"Industrial source acquisition settings/geometry changed");
    require(double(timeoutMs)*1000>b.profile.requested.exposureTimeUs,"Industrial source timeout must exceed exposure; transfer budget also required");
}
}
