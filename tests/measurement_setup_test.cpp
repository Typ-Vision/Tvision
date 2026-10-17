#include "domain/MeasurementSetupContract.hpp"
#include "application/MeasurementSourceIdentity.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){++checks;try{f();}catch(const std::exception&){return;}throw std::runtime_error(why);}
MeasurementSetupContract fixture(int count){
    MeasurementSetupContract c;
    for(int i=0;i<count;++i){const auto suffix=QString::number(i);c.cameras.append({{"camera-"+suffix},"Camera "+suffix,{"webcam","provider-"+suffix,DeviceIdentityConfidence::ProviderReported,1,{true,true,true,true}}});
        c.setups.append({{"setup-"+suffix},c.cameras.last().id,"Setup "+suffix,"lens-A","plane-A",1,1,{{640,480},QRect(10,20,1280,960),QSize(2,2),true}});}
    return c;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    // Golden bytes/hash computed outside VisionFlow from the pre-ICM-03a v1 encoding.
    const QByteArray golden=R"({"cameras":[{"device":{"adapter":"webcam","capabilities":{"binning":true,"resolution":true,"sensorRoi":true,"softwareCapture":true},"confidence":"provider_reported","providerId":"provider-0","revision":1,"schemaVersion":1},"id":"camera-0","name":"Camera 0","schemaVersion":1}],"format":"visionflow.measurement-setup-contract","schemaVersion":1,"setups":[{"cameraId":"camera-0","deviceRevision":1,"geometry":{"binning":{"x":2,"y":2},"complete":true,"height":480,"schemaVersion":1,"sensorRoi":{"height":960,"width":1280,"x":10,"y":20},"width":640},"id":"setup-0","name":"Setup 0","opticalProfile":"lens-A","revision":1,"schemaVersion":1,"workingPlane":"plane-A"}]})";
    check(QJsonDocument(fixture(1).toJson()).toJson(QJsonDocument::Compact)==golden,"legacy v1 contract bytes unchanged");
    check(QJsonDocument(MeasurementSetupContract::fromJson(QJsonDocument::fromJson(golden).object()).toJson()).toJson(QJsonDocument::Compact)==golden,"legacy golden no-op roundtrip");
    check(fixture(1).setups[0].geometryId()=="7cbe135ab59e652f345cf792535607d04163e81121a17b6606d02c8b6672a60a","legacy geometry hash unchanged");
    for(int n:{0,1,2,5,8,10}){
        const auto c=fixture(n);const auto json=c.toJson();const auto bytes=QJsonDocument(json).toJson(QJsonDocument::Compact);
        const auto decoded=MeasurementSetupContract::fromJson(QJsonDocument::fromJson(bytes).object());
        check(decoded.cameras==c.cameras && decoded.setups==c.setups && decoded.toJson()==json,"N-camera exact roundtrip");
    }
    const auto original=fixture(2);auto renamed=original;
    renamed.cameras[0].name="Renamed camera";auto next=renamed.setups[0];next.name="Renamed setup";renamed.setups[0]=renamed.setups[0].revised(next);
    check(renamed.cameras[0].id==original.cameras[0].id && renamed.cameras[0].device==original.cameras[0].device,"camera rename preserves physical identity");
    check(renamed.setups[0].revision==1 && renamed.setups[0].geometryId()==original.setups[0].geometryId(),"setup rename preserves revision/hash");
    for(int change=0;change<5;++change){next=original.setups[0];if(change==0)next.opticalProfile="lens-B";if(change==1)next.workingPlane="plane-B";if(change==2){next.geometry.resolution={320,240};next.geometry.sensorRoi=QRect(10,20,640,480);}if(change==3)next.geometry.sensorRoi=QRect(11,20,1280,960);if(change==4)next.deviceRevision=2;
        next=original.setups[0].revised(next);check(next.revision==2 && next.geometryId()!=original.setups[0].geometryId(),"physical change increments revision/hash");}
    const auto& setup=original.setups[0];
    check(setup.cacheId({{"exposure",100}})!=setup.cacheId({{"exposure",200}}) && setup.geometryId()==original.setups[0].geometryId(),"exposure affects cache, not geometric compatibility");
    next=setup;next.camera={"camera-1"};reject([&]{setup.revised(next);},"silent camera reassignment rejected");
    next=setup;next.geometry.complete=false;reject([&]{next.geometryId();},"unknown critical geometry cannot be compatible");
    next=setup;next.geometry.binning=QSize(3,2);reject([&]{next.validate();},"inconsistent binning rejected");
    auto device=original.cameras[0].device;auto physical=device;physical.providerId="replacement";physical=device.rebound(physical);
    check(physical.revision==2 && device.rebound(device).revision==1,"remapping increments device revision, no-op preserves it");
    check(device.resolve({original.cameras[1].device,device})==1,"device discovery resolves exact provider identity");
    reject([&]{device.resolve({device,device});},"ambiguous discovery rejected");reject([&]{device.resolve({});},"offline device rejected");
    auto unknown=device;unknown.providerId.clear();unknown.confidence=DeviceIdentityConfidence::Unavailable;
    check(DeviceBinding::fromJson(unknown.toJson())==unknown,"unavailable identity remains explicit");reject([&]{unknown.resolve({device});},"index fallback forbidden");
    auto changedCaps=device;changedCaps.capabilities.softwareCapture=false;reject([&]{device.resolve({changedCaps});},"changed actual capabilities require remap");
    next=setup;next.deviceRevision=2;next=setup.revised(next);auto rollback=next;rollback.deviceRevision=1;reject([&]{next.revised(rollback);},"device revision rollback rejected");
    auto exhausted=setup;exhausted.revision=1000000000;next=exhausted;next.opticalProfile="new-lens";reject([&]{exhausted.revised(next);},"setup revision exhaustion rejected");
    auto exhaustedDevice=device;exhaustedDevice.revision=1000000000;reject([&]{exhaustedDevice.rebound(physical);},"device revision exhaustion rejected");
    next=setup;next.geometry.binning=QSize(1,1);next.geometry.sensorRoi=QRect(10,20,640,480);check(setup.revised(next).revision==2,"valid binning change increments revision");
    for(int mutation=0;mutation<9;++mutation){auto c=original;
        if(mutation==0)c.cameras.append(c.cameras[0]);if(mutation==1)c.setups.append(c.setups[0]);if(mutation==2)c.setups[0].camera={"missing"};if(mutation==3)c.cameras[1].device=c.cameras[0].device;
        if(mutation==4)c.setups[0].deviceRevision=2;if(mutation==5)c.cameras[0].device.capabilities.sensorRoi=false;if(mutation==6)c.cameras[0].device.capabilities.binning=false;if(mutation==7)c.cameras[0].device.adapter="unknown";if(mutation==8)c.setups[0].revision=0;
        reject([&]{c.toJson();},"invalid contract rejected");}
    auto ten=fixture(10);check(MeasurementSetupContract::fromJson(ten.toJson(10),10).cameras.size()==10,"explicit capacity accepts ten");reject([&]{ten.toJson(9);},"capacity overflow rejected without truncation");reject([&]{MeasurementSetupContract::fromJson(ten.toJson(),9);},"decode capacity overflow rejected");reject([&]{ten.toJson(0);},"invalid capacity rejected");
    // Mutations exercise every nested strict schema and typed input boundary.
    auto json=original.toJson();for(int level=0;level<6;++level)for(int kind=0;kind<4;++kind){auto bad=json;auto cs=bad["cameras"].toArray(),ss=bad["setups"].toArray();auto camera=cs[0].toObject(),s=ss[0].toObject(),d=camera["device"].toObject(),g=s["geometry"].toObject(),caps=d["capabilities"].toObject();
        QJsonObject* o=level==0 ? &bad:level==1 ? &camera:level==2 ? &d:level==3 ? &s:level==4 ? &g:&caps;
        if(kind==0)(*o)["unexpected"]=true;
        if(kind==1)o->remove(o->begin().key());
        if(kind==2){if(level==5)(*o)["softwareCapture"]=1;else (*o)["schemaVersion"]=99;}
        if(kind==3){if(level==5)(*o)["binning"]="true";else (*o)["schemaVersion"]=1.5;}
        if(level>=5)d["capabilities"]=caps;if(level==2 || level==5)camera["device"]=d;if(level>=4 && level!=5)s["geometry"]=g;if(level==1 || level==2 || level==5){cs[0]=camera;bad["cameras"]=cs;}if(level==3 || level==4){ss[0]=s;bad["setups"]=ss;}
        reject([&]{MeasurementSetupContract::fromJson(bad);},"unknown/missing/type/schema fields rejected");}
    MeasurementSourceEvidence evidence{SourceOrigin::CameraAcquired,"calibration-session-A","frame-A","cycle-A","raw-A",QString(64,'a'),"",1,SetupReference{{"camera-0"},{"setup-0"},1}};
    for(auto origin:{SourceOrigin::CameraAcquired,SourceOrigin::SetupFile,SourceOrigin::TrialFile,SourceOrigin::LegacyRecord}){
        auto e=evidence;e.origin=origin;
        if(origin==SourceOrigin::SetupFile || origin==SourceOrigin::TrialFile)e.filePath="C:/fixtures/reference.png";
        if(origin==SourceOrigin::TrialFile)e.target.reset();
        if(origin==SourceOrigin::LegacyRecord){e.generation=0;e.frameId.clear();e.cycleId.clear();e.coordinates.clear();e.pixelHash.clear();}
        check(MeasurementSourceEvidence::fromJson(e.toJson())==e,"origin exact roundtrip without invented raster");}
    for(int mutation=0;mutation<12;++mutation){auto e=evidence;
        if(mutation==0)e.target.reset();if(mutation==1)e.filePath="C:/file.png";if(mutation==2)e.generation=0;if(mutation==3)e.pixelHash="fake";if(mutation==4)e.frameId.clear();if(mutation==5)e.cycleId.clear();if(mutation==6)e.coordinates.clear();if(mutation==7)e.target->revision=0;
        if(mutation==8){e.origin=SourceOrigin::TrialFile;e.filePath="C:/file.png";}if(mutation==9){e.origin=SourceOrigin::SetupFile;e.filePath="relative.png";}if(mutation==10)e.origin=static_cast<SourceOrigin>(42);if(mutation==11)e.sourceId.clear();
        reject([&]{e.toJson();},"invalid origin/evidence rejected");}
    auto source=evidence.toJson();for(const auto& key:source.keys()){auto bad=source;bad.remove(key);reject([&]{MeasurementSourceEvidence::fromJson(bad);},"missing source field rejected");}
    auto recipeEvidence=evidence;recipeEvidence.sourceKind=SourceIdentityKind::RecipeTool;check(MeasurementSourceEvidence::fromJson(recipeEvidence.toJson())==recipeEvidence && recipeEvidence.toJson()!=source,"recipe tool and session source namespaces remain distinct");
    auto wrongKind=source;wrongKind["sourceKind"]="camera";reject([&]{MeasurementSourceEvidence::fromJson(wrongKind);},"camera ID is not an acquisition source kind");
    for(int mutation=0;mutation<3;++mutation){auto target=source["target"].toObject();if(mutation==0)target["schemaVersion"]=2;if(mutation==1)target["revision"]=1.1;if(mutation==2)target["extra"]=true;auto bad=source;bad["target"]=target;reject([&]{MeasurementSourceEvidence::fromJson(bad);},"strict target schema/type/fields");}
    auto bad=source;bad["origin"]="acquired";reject([&]{MeasurementSourceEvidence::fromJson(bad);},"legacy origin cannot be silently relabeled");bad=source;bad["schemaVersion"]=2;reject([&]{MeasurementSourceEvidence::fromJson(bad);},"unknown source schema rejected");bad=source;bad["generation"]=1.5;reject([&]{MeasurementSourceEvidence::fromJson(bad);},"fractional generation rejected");
    // ICM-03a identity corpus: portable physical identity, local route/backend, still no industrial capture.
    const PhysicalDeviceIdentity coe{"OECS","COE-050-M-USB-050-IR-C","SN-0001",DeviceIdentityConfidence::SerialReported};
    const PhysicalDeviceIdentity noSerial{"OECS","COE-050-M-USB-050-IR-C",{},DeviceIdentityConfidence::ProviderReported};
    const BackendIdentity sdk{"oecs","3.5.2.0",QString(64,'a'),1},cti{"gentl","1.0.0.5",QString(64,'b'),1};
    const QString producer="C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvProducerU3V.cti";
    const DiscoveredDevice direct{coe,{"oecs","","usb-0","dev-7"},sdk},viaGentl{coe,{"gentl",producer,"if-0","dev-1"},cti};
    for(const auto& p:{coe,noSerial,PhysicalDeviceIdentity{}})check(PhysicalDeviceIdentity::fromJson(p.toJson())==p,"physical identity exact roundtrip");
    for(const auto& t:{direct.route,viaGentl.route})check(TransportLocator::fromJson(t.toJson())==t,"route exact roundtrip");
    check(BackendIdentity::fromJson(sdk.toJson())==sdk && BackendIdentity::fromJson(cti.toJson())==cti,"backend exact roundtrip");
    auto moved=viaGentl;moved.route.producer="D:/Copied/MvProducerU3V.cti";check(moved.backend.fingerprint()==viaGentl.backend.fingerprint() && moved.route!=viaGentl.route,"moved binary keeps backend fingerprint, changes route");
    auto replacedBinary=cti;replacedBinary.runtimeSha256=QString(64,'c');check(replacedBinary.fingerprint()!=cti.fingerprint(),"replaced binary changes backend fingerprint");
    check(resolveRoutes(coe,{direct,viaGentl})==QVector<int>{0,1},"same device via two producers is one device with two routes");
    auto twin=direct;twin.route.deviceId="dev-8";reject([&]{resolveRoutes(coe,{direct,twin});},"duplicate serial in one enumeration rejected");
    auto otherVendor=direct;otherVendor.identity.vendor="Other";check(resolveRoutes(coe,{otherVendor,viaGentl})==QVector<int>{1},"same serial from another vendor is another device");reject([&]{resolveRoutes(coe,{otherVendor});},"no serial-only fallback");
    auto otherModel=direct;otherModel.identity.model="Other";reject([&]{resolveRoutes(coe,{otherModel});},"model mismatch rejected");
    reject([&]{resolveRoutes(coe,{});},"offline device rejected without next-device fallback");
    auto unserial=direct;unserial.identity=noSerial;reject([&]{resolveRoutes(noSerial,{unserial});},"missing serial requires explicit remap");
    check(resolveRoutes(coe,{unserial,direct})==QVector<int>{1},"serial-less discovery never matches a serial identity");
    reject([&]{resolveRoutes(PhysicalDeviceIdentity{},{direct});},"unavailable identity cannot resolve");
    auto mismatch=direct;mismatch.backend=cti;reject([&]{resolveRoutes(coe,{mismatch});},"route/backend mismatch rejected");
    for(int mutation=0;mutation<6;++mutation){auto p=coe;if(mutation==0)p.serial.clear();if(mutation==1)p.vendor.clear();if(mutation==2)p.model=" padded";if(mutation==3)p.confidence=DeviceIdentityConfidence::ProviderReported;if(mutation==4){p=PhysicalDeviceIdentity{};p.serial="1";}if(mutation==5)p.confidence=static_cast<DeviceIdentityConfidence>(9);
        reject([&]{p.toJson();},"invalid physical identity rejected");}
    for(int mutation=0;mutation<6;++mutation){auto t=viaGentl.route;if(mutation==0)t.producer="relative/MvProducerU3V.cti";if(mutation==1)t.producer="C:/OECS/../OECS/x.cti";if(mutation==2)t.producer.clear();if(mutation==3)t.deviceId.clear();if(mutation==4)t.backend="usb";if(mutation==5)t.backend="oecs";
        reject([&]{t.toJson();},"invalid route rejected");}
    for(int mutation=0;mutation<4;++mutation){auto b=sdk;if(mutation==0)b.runtimeSha256=QString(64,'A');if(mutation==1)b.runtimeSha256="abc";if(mutation==2)b.adapterVersion=0;if(mutation==3)b.adapter="webcam";reject([&]{b.toJson();},"invalid backend identity rejected");}
    auto strict=[&](const QJsonObject& value,auto decode){for(int kind=0;kind<4;++kind){auto bad=value;if(kind==0)bad["unexpected"]=true;if(kind==1)bad.remove(bad.begin().key());if(kind==2)bad["schemaVersion"]=2;if(kind==3)bad["schemaVersion"]=1.5;reject([&]{decode(bad);},"strict identity codec");}};
    strict(coe.toJson(),PhysicalDeviceIdentity::fromJson);strict(direct.route.toJson(),TransportLocator::fromJson);strict(sdk.toJson(),BackendIdentity::fromJson);
    {auto bad=coe.toJson();bad["serial"]=1;reject([&]{PhysicalDeviceIdentity::fromJson(bad);},"typed serial");bad=sdk.toJson();bad["adapterVersion"]="1";reject([&]{BackendIdentity::fromJson(bad);},"typed adapter version");}
    const MeasurementCamera industrial{{"camera-industrial"},"Line camera",{"gentl","",DeviceIdentityConfidence::Unavailable,1,{false,true,false,false}},coe};
    const auto industrialJson=industrial.toJson();const auto industrialBytes=QJsonDocument(industrialJson).toJson(QJsonDocument::Compact);
    check(MeasurementCamera::fromJson(industrialJson)==industrial && industrialJson["schemaVersion"]==2,"industrial camera explicit schema 2 roundtrip");
    check(!industrialBytes.contains("producer") && !industrialBytes.contains("Program Files") && !industrialBytes.contains("dev-7") && !industrialBytes.contains(sdk.runtimeSha256.toLatin1()),"project camera carries no local route or backend fingerprint");
    check(original.cameras[0].rebound(renamed.cameras[0])==renamed.cameras[0],"legacy rename through camera rebind");
    auto renamedIndustrial=industrial;renamedIndustrial.name="Renamed line camera";check(industrial.rebound(renamedIndustrial)==renamedIndustrial,"rename keeps physical identity and device revision");
    auto remapped=industrial;remapped.physical->serial="SN-0002";remapped=industrial.rebound(remapped);check(remapped.device.revision==2 && remapped.physical->serial=="SN-0002" && remapped.rebound(remapped)==remapped,"explicit rebind increments revision; no-op keeps it");
    auto lost=industrial;lost.physical=PhysicalDeviceIdentity{};check(industrial.rebound(lost).device.revision==2,"losing identity is a revisioned remap");
    auto converted=industrial;converted.id=original.cameras[0].id;check(original.cameras[0].rebound(converted).device.revision==2,"webcam to industrial is a revisioned remap");
    auto otherCamera=industrial;otherCamera.id={"other"};reject([&]{industrial.rebound(otherCamera);},"rebind cannot change camera ID");
    auto exhaustedCamera=industrial;exhaustedCamera.device.revision=1000000000;reject([&]{exhaustedCamera.rebound(remapped);},"camera revision exhaustion rejected");
    for(int mutation=0;mutation<4;++mutation){auto c=industrial;if(mutation==0)c.device.adapter="webcam";if(mutation==1){c.device.providerId="x";c.device.confidence=DeviceIdentityConfidence::ProviderReported;}if(mutation==2)c.device.capabilities.softwareCapture=true;if(mutation==3)c.physical->serial.clear();
        reject([&]{c.toJson();},"industrial camera cannot claim capture or provider route");}
    for(int mutation=0;mutation<4;++mutation){auto bad=industrialJson;if(mutation==0)bad.remove("physical");if(mutation==1)bad["schemaVersion"]=1;if(mutation==2)bad["schemaVersion"]=3;if(mutation==3)bad["route"]=direct.route.toJson();
        reject([&]{MeasurementCamera::fromJson(bad);},"strict camera schema 1/2 boundary");}
    auto mixed=original;mixed.cameras.append(industrial);auto mixedSetup=original.setups[0];mixedSetup.id={"setup-industrial"};mixedSetup.camera=industrial.id;mixedSetup.geometry={{2448,2048},{},{},true};mixed.setups.append(mixedSetup);
    const auto mixedJson=mixed.toJson();check(MeasurementSetupContract::fromJson(mixedJson).cameras==mixed.cameras && mixedJson["cameras"].toArray()[0].toObject()["schemaVersion"]==1,"mixed contract keeps legacy cameras at schema 1");
    auto duplicate=mixed;auto second=industrial;second.id={"camera-industrial-2"};duplicate.cameras.append(second);reject([&]{duplicate.toJson();},"same physical identity cannot back two cameras");
    second.physical->vendor="Other";duplicate.cameras.last()=second;check(MeasurementSetupContract::fromJson(duplicate.toJson()).cameras.size()==4,"same serial from another vendor is a distinct camera");
    QTextStream(stdout)<<checks<<" measurement setup checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
