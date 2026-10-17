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
    QTextStream(stdout)<<checks<<" measurement environment checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
