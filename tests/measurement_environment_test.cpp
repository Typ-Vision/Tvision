#include "application/MeasurementEnvironment.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLockFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
template<class F>void reject(F f,const char* s){++checks;try{f();}catch(const std::exception&){return;}throw std::runtime_error(s);}
MeasurementSetupContract fixture(int n){MeasurementSetupContract c;for(int i=0;i<n;++i){auto id=QString::number(i);c.cameras.append({{"c"+id},"Camera "+id,{"webcam","qt-"+id,DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}});c.setups.append({{"s"+id},c.cameras.last().id,"Setup "+id,"lens","plane",1,1,{{640,480},{},{},true}});}return c;}
CalibrationModelRecord model(){MeasurementSetup s{"bench","webcam",QString(64,'a'),"lens","plane",{640,480},{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};CalibrationDependency d{"scale",1,"pending","old-source",s.lensId,s.resolution,s.planeId,s.source,s.target};return CalibrationModelRecord::create(MeasurementService{}.teach(d,{"teach",{{0,0},s.source},{{400,0},s.source},10}),s,{"acquired","old-source","f",QString(64,'b')});}
QByteArray bytes(const QString& p){QFile f(p);check(f.open(QIODevice::ReadOnly),"read evidence");return f.readAll();}
void write(const QString& p,const QByteArray& b){QFile f(p);check(f.open(QIODevice::WriteOnly) && f.write(b)==b.size(),"write evidence");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/measurement-env-XXXXXX");check(dir.isValid(),"build fixture directory");
    const auto project=dir.path()+"/empty.vfproject";MeasurementEnvironmentRepository repo(project,"project-A",10);MeasurementEnvironmentRegistry registry(repo);
    check(!QFile::exists(repo.path()) && registry.snapshot().current.cameras.isEmpty(),"zero-recipe legacy load creates no file");auto empty=repo.load();check(repo.commit(empty,empty).revision==0 && !QFile::exists(repo.path()),"no-op save does not migrate");
    registry.replace(fixture(10));check(registry.snapshot().current.cameras.size()==10,"ten cameras persisted");auto first=registry.snapshot();check(repo.load().encode(10)==first.encode(10),"restart exact roundtrip");
    reject([&]{MeasurementEnvironmentRepository(project,"wrong-project").load();},"project isolation");reject([&]{MeasurementEnvironmentRepository(project,"project-A",9).load();},"capacity rejects without truncation");
    auto renamed=first.current;renamed.cameras[0].name="Renamed";renamed.setups[0].name="Renamed setup";registry.replace(renamed);check(registry.snapshot().history.size()==10 && registry.snapshot().current.setups[0].revision==1,"rename keeps geometry history");
    reject([&]{repo.commit(first,first);},"stale expected revision conflict");auto current=repo.load();auto changed=current;changed.operational["c0"]="s0";
    const auto original=bytes(repo.path());for(auto fault:{MeasurementEnvironmentRepository::Fault::BeforeWrite,MeasurementEnvironmentRepository::Fault::BeforeCommit}){reject([&]{repo.commit(current,changed,fault);},"injected atomic failure");check(bytes(repo.path())==original,"old document survives failure");}
    {QLockFile lock(repo.path()+".lock");check(lock.tryLock(0),"external lock");reject([&]{repo.commit(current,changed);},"lock conflict");}
    {QLockFile lock(repo.catalogPath()+".lock");check(lock.tryLock(0),"external dependency lock");reject([&]{repo.commit(current,changed);},"dependency lock conflict");}
    MeasurementEnvironmentRepository missing(dir.path()+"/missing/absent.vfproject","new");auto absent=missing.load(),newState=absent;newState.current=fixture(1);newState.history=newState.current.setups;reject([&]{missing.commit(absent,newState);},"unwritable destination fails");
    registry.setOperational({"c0"},{"s0"});registry.archiveSetup({"s0"});check(registry.snapshot().operational.isEmpty() && registry.snapshot().history.size()==10,"archive preserves history clears operational");registry.setOperational({"c1"},{"s1"});registry.archiveCamera({"c1"});check(registry.snapshot().operational.isEmpty(),"camera archive clears operational");reject([&]{registry.setOperational({"c1"},{"s1"});},"archived camera cannot be operational");
    const auto record=model();current=repo.load();SetupReference ref{{"c2"},{"s2"},1};reject([&]{repo.saveAndRemember(current,record,ref,MeasurementEnvironmentRepository::Fault::AfterCandidate);},"interrupted two-stage commit");check(repo.load().selections.isEmpty(),"orphan candidate grants no selection");
    auto selected=repo.saveAndRemember(current,record,ref);check(selected.selections.size()==1,"retry selects exact candidate");auto retry=repo.saveAndRemember(selected,record,ref);check(retry.encode(10)==selected.encode(10),"retry idempotent revision");ActiveCalibrationService catalog(repo.catalogPath(),"project-A",[]{return true;});check(catalog.records().size()==1 && !catalog.active() && !catalog.remembered(),"new registry never changes legacy active permission");
    MeasurementEnvironmentRegistry restart(repo);check(restart.snapshot().selections[0].contentId==record.contentId(),"restart remembers exact choice only");auto updated=restart.snapshot().current;auto s=updated.setups[2];s.opticalProfile="replacement-lens";
    updated.setups[2]=restart.snapshot().current.setups[2].revised(s);restart.replace(updated);check(restart.snapshot().history.size()==11 && restart.snapshot().selections[0].target.revision==1,"revision change preserves historical selection without permission");
    auto bundle=repo.exportBundle();MeasurementEnvironmentRepository copy(dir.path()+"/copy.vfproject","project-copy",10);copy.importBundle(bundle);check(copy.load().projectId=="project-copy" && copy.load().selections[0].contentId==record.contentId(),"Save As dependency copy");ActiveCalibrationService copiedCatalog(copy.catalogPath(),"project-copy",[]{return true;});check(!copiedCatalog.active(),"copy cannot restore permission");reject([&]{copy.importBundle(bundle);},"copy does not overwrite destination");
    MeasurementEnvironmentRepository interruptedCopy(dir.path()+"/interrupted-copy.vfproject","copy-retry",10);auto copiedCandidate=bundle["catalog"].toObject();copiedCandidate["projectId"]="copy-retry";write(interruptedCopy.catalogPath(),QJsonDocument(copiedCandidate).toJson());interruptedCopy.importBundle(bundle);check(interruptedCopy.load().selections[0].contentId==record.contentId(),"interrupted dependency copy resumes idempotently");
    MeasurementEnvironmentRepository invalidCopy(dir.path()+"/invalid-copy.vfproject","bad-copy",10);auto brokenBundle=bundle;brokenBundle["catalog"]=QJsonValue();reject([&]{invalidCopy.importBundle(brokenBundle);},"bundle missing selected dependency rejected");check(!QFile::exists(invalidCopy.path()) && !QFile::exists(invalidCopy.catalogPath()),"invalid bundle writes no dependencies");brokenBundle=bundle;auto malformedCatalog=brokenBundle["catalog"].toObject();malformedCatalog["extra"]=true;brokenBundle["catalog"]=malformedCatalog;reject([&]{invalidCopy.importBundle(brokenBundle);},"malformed catalog rejected before writing");check(!QFile::exists(invalidCopy.catalogPath()),"malformed catalog not persisted");
    const auto catalogBytes=bytes(repo.catalogPath());QFile::remove(repo.catalogPath());reject([&]{repo.load();},"dangling selected catalog rejected");write(repo.catalogPath(),catalogBytes);
    auto tampered=QJsonDocument::fromJson(catalogBytes).object();tampered["projectId"]="other";write(repo.catalogPath(),QJsonDocument(tampered).toJson());reject([&]{repo.load();},"catalog tamper/project swap rejected");write(repo.catalogPath(),catalogBytes);
    const auto valid=bytes(repo.path());for(int mutation=0;mutation<6;++mutation){auto bad=QJsonDocument::fromJson(valid).object();if(mutation==0)bad["schemaVersion"]=2;if(mutation==1)bad["revision"]=1.5;if(mutation==2)bad["extra"]=true;if(mutation==3)bad.remove("history");if(mutation==4)bad["operational"]=QJsonObject{{"c2","missing"}};if(mutation==5)bad["projectId"]="other";write(repo.path(),QJsonDocument(bad).toJson());reject([&]{repo.load();},"strict schema/tamper rejected");}write(repo.path(),valid);
    auto deletion=repo.load();deletion.current.cameras.removeLast();reject([&]{repo.commit(repo.load(),deletion);},"deletion forbidden");auto rewrite=repo.load();rewrite.history[0].name="rewritten";reject([&]{repo.commit(repo.load(),rewrite);},"immutable history cannot be rewritten");
    {// ICM-03a: industrial identity persists without a local route; remap is explicit and revisioned.
        MeasurementEnvironmentRepository plantRepo(dir.path()+"/industrial.vfproject","project-I",10);MeasurementEnvironmentRegistry plant(plantRepo);
        MeasurementSetupContract line;line.cameras.append({{"line"},"Line camera",{"gentl","",DeviceIdentityConfidence::Unavailable,1,{false,true,false,false}},PhysicalDeviceIdentity{"OECS","COE-050-M-USB-050-IR-C","SN-0001",DeviceIdentityConfidence::SerialReported}});
        line.setups.append({{"line-setup"},{"line"},"Line setup","lens","plane",1,1,{{2448,2048},QRect(0,0,2448,2048),QSize(1,1),true,QSize(1,1),GeometryMirror{}}});plant.replace(line);
        check(MeasurementEnvironmentRegistry(plantRepo).snapshot().encode(10)==plant.snapshot().encode(10),"industrial identity restart roundtrip");
        const auto stored=bytes(plantRepo.path());check(stored.contains("SN-0001") && !stored.contains("producer") && !stored.contains(".cti"),"environment stores physical identity, no local producer route");
        auto renamedLine=plant.snapshot().current;renamedLine.cameras[0].name="Renamed line";plant.replace(renamedLine);
        check(plant.snapshot().current.cameras[0].physical==line.cameras[0].physical && plant.snapshot().current.cameras[0].device.revision==1,"rename keeps industrial identity and revision");
        auto silent=plant.snapshot();silent.current.cameras[0].physical->serial="SN-0002";reject([&]{plantRepo.commit(plant.snapshot(),silent);},"silent identity swap without revision rejected");
        auto stale=plant.snapshot().current;auto staleCamera=stale.cameras[0];staleCamera.physical->serial="SN-0002";stale.cameras[0]=stale.cameras[0].rebound(staleCamera);reject([&]{plant.replace(stale);},"rebind without setup device revision rejected");
        auto rebind=stale;auto relinked=rebind.setups[0];relinked.deviceRevision=2;rebind.setups[0]=rebind.setups[0].revised(relinked);plant.replace(rebind);
        check(plant.snapshot().current.cameras[0].device.revision==2 && plant.snapshot().current.setups[0].revision==2 && plant.snapshot().history.size()==2,"explicit rebind increments device and setup revision");
        MeasurementEnvironmentRepository plantCopy(dir.path()+"/industrial-copy.vfproject","project-I-copy",10);plantCopy.importBundle(plantRepo.exportBundle());
        check(plantCopy.load().current.cameras[0].physical==plant.snapshot().current.cameras[0].physical,"Save As carries physical identity without route");
        // ICM-03b: profile persists, transitions are revisioned and Save As grants no usage.
        auto withProfile=plant.snapshot().current;const auto lineSetup=withProfile.setups[0];
        withProfile.profiles.append({{"line-profile"},{"line"},{"line-setup"},"Production",1,{"Mono8",{true,"FrameStart","Software"},AutoMode::Off,5000,AutoMode::Off,0,""}});plant.replace(withProfile);
        const auto acquisition=plant.snapshot().current.profiles[0].acquisitionId(lineSetup);
        check(MeasurementEnvironmentRegistry(plantRepo).snapshot().encode(10)==plant.snapshot().encode(10),"profile restart exact roundtrip");
        auto silentExposure=plant.snapshot();silentExposure.current.profiles[0].requested.exposureTimeUs=8000;reject([&]{plantRepo.commit(plant.snapshot(),silentExposure);},"exposure change without profile revision rejected");
        auto renamedProfile=plant.snapshot().current;renamedProfile.profiles[0].name="Renamed production";plant.replace(renamedProfile);
        check(plant.snapshot().current.profiles[0].revision==1 && plant.snapshot().current.profiles[0].acquisitionId(lineSetup)==acquisition,"profile rename keeps revision and acquisition identity");
        auto exposed=plant.snapshot().current;auto next=exposed.profiles[0];next.requested.exposureTimeUs=8000;exposed.profiles[0]=exposed.profiles[0].revised(next);plant.replace(exposed);
        check(plant.snapshot().current.profiles[0].revision==2 && plant.snapshot().current.setups[0]==lineSetup && plant.snapshot().history.size()==2 && plant.snapshot().current.profiles[0].acquisitionId(lineSetup)!=acquisition,"exposure revises profile only, setup history untouched");
        auto dropped=plant.snapshot();dropped.current.profiles.clear();reject([&]{plantRepo.commit(plant.snapshot(),dropped);},"profile deletion rejected");
        MeasurementEnvironmentRepository profileCopy(dir.path()+"/industrial-profile-copy.vfproject","project-P-copy",10);profileCopy.importBundle(plantRepo.exportBundle());const auto copied=profileCopy.load();
        check(copied.current.profiles==plant.snapshot().current.profiles && copied.current.profiles[0].acquisitionId(copied.current.setups[0])==plant.snapshot().current.profiles[0].acquisitionId(lineSetup),"Save As carries profile and acquisition identity");
        const auto copiedBytes=bytes(profileCopy.path());check(copiedBytes.contains("line-profile") && !copiedBytes.contains("producer") && !copiedBytes.contains(".cti") && !copiedBytes.contains("evidence") && !copied.current.cameras[0].device.capabilities.softwareCapture,"copied profile carries no route, readback or capture permission");}
    { // CCUX-01: registration is a single transaction, never an acquisition admission.
        MeasurementEnvironmentRepository r(dir.path()+"/ccux.vfproject","ccux",10);MeasurementEnvironmentRegistry reg(r);
        auto camera=[](int i){return MeasurementCamera{{"new-"+QString::number(i)},"Camera",{"webcam","device-"+QString::number(i),DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}};};
        const auto legacyEmpty=r.load().encode(10);
        for(auto fault:{MeasurementEnvironmentRepository::Fault::BeforeWrite,MeasurementEnvironmentRepository::Fault::BeforeCommit}){
            reject([&]{reg.registerCamera(camera(0),"CAM_01",0,fault);},"registration write failure");
            check(r.load().encode(10)==legacyEmpty && reg.snapshot().current.cameras.isEmpty() && !QFile::exists(r.path()),"failed initial registration leaves no partial relationship");
        }
        for(int i=0;i<5;++i){const auto before=reg.snapshot();const auto added=reg.registerCamera(camera(i),{},before.revision);
            check(!added.existing && added.camera==camera(i).id,"new camera identity retained");
            const auto e=reg.snapshot();const auto resolved=e.defaults(added.camera);
            check(e.current.cameras.size()==i+1 && e.current.setups.size()==i+1 && e.current.profiles.size()==i+1 && e.registrations.size()==i+1,"one and five cameras have exactly one default pair each");
            check(e.registrations[added.camera.value].shortId==QString("CAM_%1").arg(i+1,2,10,QChar('0')) && resolved.status==CameraDefaultsStatus::Pending,"automatic short ID and explicit pending state");
            check(e.current.setups.last().toJson()["geometry"].isNull() && e.current.profiles.last().toJson()["requested"].isNull(),"unknown readback is null, never invented");
            reject([&]{e.current.setups.last().geometryId();},"pending geometry cannot be used");reject([&]{e.current.profiles.last().acquisitionId(e.current.setups.last());},"pending acquisition cannot be used");
            for(int j=0;j<i;++j)check(e.current.cameras[j]==before.current.cameras[j] && e.current.setups[j]==before.current.setups[j] && e.current.profiles[j]==before.current.profiles[j],"another camera and its revisions are untouched");
        }
        const auto initial=reg.snapshot();const auto originalBytes=bytes(r.path());
        const auto duplicate=reg.registerCamera(camera(0),"OTHER",initial.revision);
        check(duplicate.existing && duplicate.camera==camera(0).id && bytes(r.path())==originalBytes,"duplicate device explicitly returns existing camera without changes");
        reject([&]{reg.registerCamera(camera(5),"cam_01",initial.revision);},"duplicate short ID is case insensitive");
        reject([&]{reg.registerCamera(camera(5),"bad id",initial.revision);},"invalid short ID");
        reject([&]{reg.registerCamera(camera(5),"CAM_06",initial.revision-1);},"stale registration revision");
        for(auto fault:{MeasurementEnvironmentRepository::Fault::BeforeWrite,MeasurementEnvironmentRepository::Fault::BeforeCommit}){
            reject([&]{reg.registerCamera(camera(5),"CAM_06",initial.revision,fault);},"registration atomic failure");check(bytes(r.path())==originalBytes,"existing registrations survive failed addition");
        }
        auto invalid=initial;invalid.registrations["new-0"].profile={"missing"};reject([&]{invalid.encode(10);},"dangling profile rejected");
        invalid=initial;invalid.registrations["new-0"].setup=initial.current.setups[1].id;reject([&]{invalid.encode(10);},"cross-camera default rejected");
        reg.renameCamera(camera(0).id,"LEFT","Left camera",initial.revision);auto renamed=reg.snapshot();
        check(renamed.current.cameras[0].device==initial.current.cameras[0].device && renamed.current.setups==initial.current.setups && renamed.current.profiles==initial.current.profiles && renamed.selections==initial.selections,"rename preserves device/setup/profile/model selection");
        const auto renameBytes=bytes(r.path());reg.renameCamera(camera(0).id,"LEFT","Left camera",renamed.revision);check(bytes(r.path())==renameBytes,"no-op rename preserves document and revision");
        reject([&]{reg.renameCamera(camera(1).id,"left","Conflict",reg.snapshot().revision);},"rename duplicate short ID");
        MeasurementEnvironmentRegistry stale(r);reg.renameCamera(camera(0).id,"LEFT","New name",reg.snapshot().revision);
        reject([&]{stale.registerCamera(camera(5),{},stale.snapshot().revision);},"disk stale revision detected under lock");
        reg.archiveCamera(camera(0).id);check(reg.snapshot().defaults(camera(0).id).status==CameraDefaultsStatus::Archived,"archived camera cannot resolve defaults");
        reject([&]{reg.registerCamera(camera(0),{},reg.snapshot().revision);},"archived physical camera cannot be registered again");
        reject([&]{reg.renameCamera(camera(0).id,"LEFT","Archived",reg.snapshot().revision);},"archived camera cannot be renamed");
        reject([&]{reg.registerCamera(camera(5),"LEFT",reg.snapshot().revision);},"archived alias remains reserved");
        reg.archiveSetup(reg.snapshot().registrations["new-1"].setup);check(reg.snapshot().defaults(camera(1).id).status==CameraDefaultsStatus::Archived,"archived default never silently falls back");
        MeasurementEnvironmentRepository copy(dir.path()+"/ccux-import.vfproject","other",10);copy.importBundle(r.exportBundle());
        check(copy.load().registrations==reg.snapshot().registrations && copy.load().current.toJson(10)==reg.snapshot().current.toJson(10) && copy.load().projectId=="other" && copy.load().operational.isEmpty(),"bundle preserves defaults/aliases/archives without admission");
        auto legacy=MeasurementEnvironment{};legacy.projectId="legacy";legacy.current=fixture(1);legacy.history=legacy.current.setups;
        const auto legacyJson=legacy.encode(10);check(MeasurementEnvironment::decode(legacyJson,10).encode(10)==legacyJson && legacyJson["schemaVersion"]==1,"legacy no-op JSON and hash preserved");
        check(legacy.defaults({"c0"}).status==CameraDefaultsStatus::Ready,"unique legacy setup resolves without migration");
        auto second=legacy.current.setups[0];second.id={"s-other"};legacy.current.setups.append(second);legacy.history.append(second);
        check(legacy.defaults({"c0"}).status==CameraDefaultsStatus::Ambiguous,"multiple legacy setups require explicit choice");legacy.operational["c0"]="s-other";
        check(legacy.defaults({"c0"}).setup==second.id,"explicit legacy operational setup preserved");
        legacy.current.cameras[0].physical=PhysicalDeviceIdentity{"Vendor","Model","serial",DeviceIdentityConfidence::SerialReported};legacy.current.cameras[0].device={"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}};
        legacy.current.profiles={{{"p1"},{"c0"},second.id,"First",1,{},true},{{"p2"},{"c0"},second.id,"Second",1,{},true}};
        check(legacy.defaults({"c0"}).status==CameraDefaultsStatus::Ambiguous,"multiple legacy profiles never select first");
        legacy.current.profiles.removeLast();check(legacy.defaults({"c0"}).profile==ProfileId{"p1"},"one matching legacy profile resolves");
        MeasurementEnvironmentRepository legacyRepo(dir.path()+"/legacy-defaults.vfproject","legacy-defaults",10);MeasurementEnvironmentRegistry legacyRegistry(legacyRepo);
        auto legacyContract=initial.current;auto extra=legacyContract.profiles[0];extra.id={"extra-profile"};legacyContract.profiles.append(extra);legacyRegistry.replace(legacyContract);
        const auto legacyBefore=legacyRegistry.snapshot();check(legacyBefore.registrations.isEmpty() && legacyBefore.defaults(camera(0).id).status==CameraDefaultsStatus::Ambiguous,"legacy multiple profiles stay unmodified and ambiguous");
        legacyRegistry.selectDefaults(camera(0).id,"CHOSEN",extra.setup,extra.id,legacyBefore.revision);
        check(legacyRegistry.snapshot().defaults(camera(0).id).profile==extra.id && legacyRegistry.snapshot().current.toJson(10)==legacyBefore.current.toJson(10),"explicit legacy selection changes metadata only");
        legacyRegistry.renameCamera(camera(0).id,"RENAMED","Renamed legacy",legacyRegistry.snapshot().revision);
        check(legacyRegistry.snapshot().defaults(camera(0).id).profile==extra.id,"legacy rename preserves explicit profile choice");
        reject([&]{legacyRegistry.selectDefaults(camera(1).id,"BAD",extra.setup,extra.id,legacyRegistry.snapshot().revision);},"explicit choice rejects cross-camera relation");
        MeasurementEnvironmentRepository singleRepo(dir.path()+"/legacy-single.vfproject","single",10);MeasurementEnvironmentRegistry single(singleRepo);single.replace(fixture(1));
        const auto singleBefore=single.snapshot();single.renameCamera({"c0"},"LEGACY","Legacy camera",singleBefore.revision);
        check(single.snapshot().current.cameras[0].id==singleBefore.current.cameras[0].id && single.snapshot().current.setups==singleBefore.current.setups && single.snapshot().current.profiles.size()==1 && single.snapshot().defaults({"c0"}).status==CameraDefaultsStatus::Pending,"explicit legacy webcam alias edit preserves UUID and setup, adds pending profile atomically");
        reject([&]{single.renameCamera({"c0"},"OLD","Stale",singleBefore.revision);},"stale rename rejected");
        // Strict pending codecs reject fields or claims that might turn defaults into evidence.
        auto setup=initial.current.setups[0];setup.geometry.resolution={640,480};reject([&]{setup.toJson();},"pending setup cannot claim a resolution");
        auto profile=initial.current.profiles[0];profile.requested.exposureTimeUs=5000;reject([&]{profile.toJson();},"pending profile cannot claim exposure");
        auto raw=initial.current.setups[0].toJson();raw["geometry"]=fixture(1).setups[0].geometry.toJson();reject([&]{MeasurementSetupDescriptor::fromJson(raw);},"pending setup codec requires null");
        raw=initial.current.profiles[0].toJson();raw["requested"]=QJsonObject{};reject([&]{AcquisitionProfile::fromJson(raw);},"pending profile codec requires null");
        auto changed=initial.current.setups[0];changed.pending=false;changed.opticalProfile="lens";changed.workingPlane="plane";changed.geometry=fixture(1).setups[0].geometry;
        check(initial.current.setups[0].revised(changed).revision==2,"explicit setup completion revises geometry");
    }
    QTextStream(stdout)<<checks<<" measurement environment checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
