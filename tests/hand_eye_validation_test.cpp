#include "application/HandEyeValidationService.hpp"
#include "application/ProductionStore.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <thread>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
QUuid uid(int n){return QUuid(QString("00000000-0000-0000-0000-%1").arg(n,12,16,QChar('0')));}
QString id(const QUuid& u){return u.toString(QUuid::WithoutBraces);}
QJsonObject read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"read JSON");return QJsonDocument::fromJson(f.readAll()).object();}
void write(const QString& path,const QJsonObject& o){QFile f(path);check(f.open(QIODevice::WriteOnly),"write JSON");const auto data=QJsonDocument(o).toJson();check(f.write(data)==data.size(),"full JSON write");}
HandEyeFixtureRequest fixture(const QString& mode){
 auto o=read(QString(TEST_DATA_DIR)+"/"+mode+".json");HandEyeFixtureRequest r;r.evidence=EvidenceState::Synthetic;r.session=uid(900);r.plateDiagonalMm=100;
 for(auto value:o["samples"].toArray()){auto s=value.toObject();r.samples.push_back({QUuid(s["sample"].toString()),QUuid(s["frame"].toString()),s["holdout"].toBool(),SpatialTransform::fromJson(s["baseTFlange"].toObject()),SpatialTransform::fromJson(s["cameraTPlate"].toObject()),.1,.2,.3});}
 r.base=r.samples[0].baseTFlange.target;r.flange=r.samples[0].baseTFlange.source;r.camera=r.samples[0].cameraTPlate.target;r.plate=r.samples[0].cameraTPlate.source;
 auto& d=r.dependencies;d.mode=mode=="eye-in-hand"?HandEyeMode::EyeInHand:HandEyeMode::EyeToHand;d.cameraKey=1;d.cameraRevision=uid(10);d.intrinsicRevisionKey=2;d.intrinsicRevision=uid(11);d.plateRevisionKey=3;d.plateRevision=uid(12);d.robotAdapterRevisionKey=4;d.robotAdapterRevision=uid(13);d.acquisitionProfileRevisionKey=5;d.acquisitionProfileRevision=uid(14);d.modelRevisionKey=6;d.modelRevision=uid(15);d.pose={QUuid(r.base.frameUuid),uid(16),QUuid(r.flange.frameUuid),uid(17),2};d.cameraId="fixture-camera";d.physical={"fixture","camera","synthetic",DeviceIdentityConfidence::SerialReported};return r;
}
struct Boundary:HandEyeActivationBoundary {
 HandEyeActivationSnapshot current;std::mutex mutex;
 void withExclusiveSnapshot(const std::function<void(const HandEyeActivationSnapshot&)>& f) override {std::lock_guard lock(mutex);f(current);}
};
HandEyeValidationRequest validation(const HandEyeFixtureRequest& f,const QJsonObject& c,const QString& mode){
 HandEyeValidationRequest r;r.session=f.session;r.candidate=QUuid(c["candidate"].toString());r.candidateRevision=QUuid(c["candidateRevision"].toString());r.referenceRevision=uid(700);r.referenceSource="independent committed golden fixedTruth; not candidate fixedTransform";r.evidence=EvidenceState::Synthetic;r.tolerance={uid(701),1e-6,1e-6,1e-6,1e-7,1e-7,1e-7};
 const auto expected=SpatialTransform::fromJson(read(QString(TEST_DATA_DIR)+"/"+mode+".json")["fixedTruth"].toObject());
 for(const auto& s:f.samples)if(s.holdout){r.holdout.push_back(s);r.references.push_back({s.sample,expected});}return r;
}
HandEyeActivationRequest activation(const HandEyeValidationResult& v){const auto& o=v.report;return {uid(800),QUuid(o["session"].toString()),QUuid(o["candidate"].toString()),QUuid(o["candidateRevision"].toString()),QUuid(o["report"].toString()),{},o["dependencyDigest"].toString()};}
void set(Boundary& b,const HandEyeActivationRequest& a,const CalibrationDependencySnapshot& d){b.current={a.station,a.session,a.candidate,a.candidateRevision,d,true,true,uid(700),uid(701)};}
void run(const QString& mode){
 auto f=fixture(mode);QTemporaryDir archive;check(archive.isValid(),"archive");ProductionStore store(archive.path());HandEyeValidationService service(archive.path());HandEyeActivationService activator(archive.path());
 const auto solved=HandEyeCandidateSolver{}.solveFixture(f,archive.path());check(solved.accepted,"solve");const auto c=solved.report;auto r=validation(f,c,mode);const auto passed=service.validateFixture(r);check(passed.accepted,"independent golden validation");
 check(passed.report["state"]=="VALIDATED"&&passed.report["physicalEligible"]==false&&passed.report["productionPermitted"]==false,"software only");
 check(store.get("hand_eye_validation",passed.report["report"].toString())==passed.report,"immutable full report");
 auto output=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS");if(!output.isEmpty()){check(QDir().mkpath(output),"artifact directory");write(output+"/"+mode+"-validation.json",passed.report);}
 auto a=activation(passed);Boundary b;set(b,a,f.dependencies);
 check(activator.remembered(a.station).isEmpty(),"validate never activates");check(!activator.activateProduction(a).accepted,"production remains blocked");
 auto bad=[&](HandEyeValidationRequest input,const char* why){auto result=service.validateFixture(input);check(!result.accepted,why);check(!result.reason.isEmpty(),"reason retained");check(activator.remembered(a.station).isEmpty(),"bad validation preserves absent active");return result;};
 auto q=r;q.holdout[0].sample=f.samples[0].sample;q.references[0].sample=q.holdout[0].sample;bad(q,"sample overlap");q=r;q.holdout[0].frame=f.samples[0].frame;bad(q,"frame overlap");
 q=r;q.holdout[0].holdout=false;bad(q,"solve role");q=r;q.holdout[1].sample=q.holdout[0].sample;bad(q,"duplicate holdout");q=r;q.holdout[1].frame=q.holdout[0].frame;bad(q,"duplicate holdout frame");
 q=r;q.holdout.resize(4);q.references.resize(4);bad(q,"less than five");q=r;q.holdout.resize(257);bad(q,"bounded holdout");q=r;q.references.clear();bad(q,"no independent references");
 q=r;q.referenceRevision=QUuid{};bad(q,"missing reference revision");q=r;q.referenceSource.clear();bad(q,"missing source");q=r;q.tolerance.revision=QUuid{};bad(q,"missing tolerance");q=r;q.tolerance.maxMm=0;bad(q,"zero tolerance");q=r;q.tolerance.rmsMm=std::numeric_limits<double>::quiet_NaN();bad(q,"nonfinite tolerance");
 q=r;q.evidence=EvidenceState::PhysicalVerified;bad(q,"physical evidence enum cannot bypass gate");q=r;q.evidence=EvidenceState::Missing;bad(q,"missing evidence");
 q=r;q.candidateRevision=uid(999);bad(q,"stale candidate revision");q=r;q.session=uid(998);bad(q,"stale session");q=r;q.candidate=uid(997);bad(q,"unknown candidate");
 q=r;q.references[0].expectedPlate.source.revision++;bad(q,"reference frame revision");q=r;q.holdout[0].baseTFlange.target.revision++;bad(q,"robot base revision");q=r;q.holdout[0].cameraTPlate.target.revision++;bad(q,"camera revision");
 q=r;q.references[1].sample=q.references[0].sample;bad(q,"duplicate reference");q=r;q.references[0].sample=uid(996);bad(q,"unmatched reference");
 q=r;q.holdout[0].rmsPx=.51;bad(q,"plate quality");q=r;q.holdout[0].maxPx=std::numeric_limits<double>::infinity();bad(q,"nonfinite quality");q=r;q.holdout[0].cameraTPlate.translationMm[0]=std::numeric_limits<double>::quiet_NaN();bad(q,"nonfinite holdout");
 // Bias the independent reference while training and holdout closure stay tiny.
 q=r;for(auto& ref:q.references)ref.expectedPlate.translationMm[0]+=3;auto rejected=bad(q,"independent systematic error");
 check(rejected.report["metrics"].toObject()["mm"].toObject()["max"].toDouble()>2.99,"independent mm measured");check(rejected.report["closureMetrics"].toObject()["mm"].toObject()["max"].toDouble()<1e-6,"closure cannot prove physical accuracy");
 auto failedActivation=activation(rejected);set(b,failedActivation,f.dependencies);check(!activator.activateFixture(failedActivation,b).accepted,"failed report cannot activate");set(b,a,f.dependencies);
 // Known independent errors 1..5mm give max/p95=5, RMS=sqrt(11).
 q=r;q.tolerance={uid(702),10,10,10,1,1,1};for(size_t i=0;i<q.references.size();++i)q.references[i].expectedPlate.translationMm[0]+=i+1;
 const auto measured=service.validateFixture(q);check(measured.accepted,"loose independent tolerance");auto mm=measured.report["metrics"].toObject()["mm"].toObject();check(std::abs(mm["max"].toDouble()-5)<1e-8,"known max");check(std::abs(mm["rms"].toDouble()-std::sqrt(11.))<1e-8,"known RMS");check(std::abs(mm["p95"].toDouble()-5)<1e-8,"nearest rank p95");
 for(int i=0;i<3;++i){auto limits=q;limits.tolerance.maxMm=limits.tolerance.rmsMm=limits.tolerance.p95Mm=10;if(i==0)limits.tolerance.maxMm=4.9;if(i==1)limits.tolerance.rmsMm=3.3;if(i==2)limits.tolerance.p95Mm=4.9;bad(limits,"each translation limit independently rejects");}
 q=r;const double theta=.02;for(auto& ref:q.references){SpatialTransform spin{ref.expectedPlate.target,ref.expectedPlate.target,{0,0,0},{std::cos(theta/2),0,0,std::sin(theta/2)}};ref.expectedPlate.rotation=ref.expectedPlate.then(spin).rotation;}
 q.tolerance={uid(703),10,10,10,1,1,1};const auto rotated=service.validateFixture(q);check(rotated.accepted,"loose rotation");auto rad=rotated.report["metrics"].toObject()["rad"].toObject();check(std::abs(rad["max"].toDouble()-theta)<1e-10,"known rotation error");
 for(int i=0;i<3;++i){auto limits=q;if(i==0)limits.tolerance.maxRad=.019;if(i==1)limits.tolerance.rmsRad=.019;if(i==2)limits.tolerance.p95Rad=.019;bad(limits,"each rotation limit independently rejects");}
 // Exact thresholds pass; one representable value below max fails.
 q.tolerance.maxRad=rad["max"].toDouble();q.tolerance.rmsRad=rad["rms"].toDouble();q.tolerance.p95Rad=rad["p95"].toDouble();check(service.validateFixture(q).accepted,"equal tolerance accepted");q.tolerance.maxRad=std::nextafter(q.tolerance.maxRad,0);bad(q,"adjacent tolerance boundary");
 const auto first=activator.activateFixture(a,b);check(first.accepted,"atomic first fixture activation");check(first.active["productionPermitted"]==false,"no production permission");const auto original=activator.remembered(a.station);check(original==first.active,"restart remembers exact selection");
 auto rejectActivation=[&](HandEyeActivationRequest input,const char* why){auto result=activator.activateFixture(input,b);check(!result.accepted,why);check(activator.remembered(a.station)==original,"failed activation preserves old active");};
 rejectActivation(a,"stale expected active null");a.expectedActiveRevision=QUuid(original["revision"].toString());
 auto altered=a;altered.candidateRevision=uid(9001);rejectActivation(altered,"request candidate revision stale");altered=a;altered.validationReport=uid(9002);rejectActivation(altered,"unknown report");altered=a;altered.dependenciesDigest=QString(64,'a');rejectActivation(altered,"wrong dependency digest");altered=a;altered.expectedActiveRevision=uid(9003);rejectActivation(altered,"stale active revision");
 b.current.idle=false;rejectActivation(a,"busy station");b.current.idle=true;b.current.cameraDrained=false;rejectActivation(a,"undrained camera");b.current.cameraDrained=true;
 b.current.candidateRevision=uid(9004);rejectActivation(a,"authoritative candidate stale");set(b,a,f.dependencies);b.current.session=uid(9005);rejectActivation(a,"authoritative session stale");set(b,a,f.dependencies);
 b.current.referenceRevision=uid(9020);rejectActivation(a,"independent reference revision stale");set(b,a,f.dependencies);b.current.toleranceRevision=uid(9021);rejectActivation(a,"tolerance revision stale");set(b,a,f.dependencies);
 // All geometric dependency categories invalidate; returning to old geometry
 // gets a NEW revision, so A->B->A never resurrects the report.
 for(int i=0;i<10;++i){auto d=f.dependencies;switch(i){case 0:d.cameraRevision=uid(9100);break;case 1:d.intrinsicRevision=uid(9101);break;case 2:d.plateRevision=uid(9102);break;case 3:d.flangeTcpRevisionKey=9;d.flangeTcpRevision=uid(9103);break;case 4:d.robotAdapterRevision=uid(9104);break;case 5:d.acquisitionProfileRevision=uid(9105);break;case 6:d.modelRevision=uid(9106);break;case 7:d.pose.targetFrameRevision=uid(9107);break;case 8:d.pose.sourceFrameRevision=uid(9108);break;case 9:d.cameraRevision=uid(9109);break;}b.current.dependencies=d;rejectActivation(a,"dependency invalidation");}
 set(b,a,f.dependencies);auto candidate2=HandEyeCandidateSolver{}.solveFixture(f,archive.path());check(candidate2.accepted,"second candidate");auto validate2=service.validateFixture(validation(f,candidate2.report,mode));check(validate2.accepted,"second validated candidate");b.current.candidate=QUuid(candidate2.report["candidate"].toString());b.current.candidateRevision=QUuid(candidate2.report["candidateRevision"].toString());rejectActivation(a,"new current candidate retires old validation");
 auto a2=activation(validate2);a2.expectedActiveRevision=a.expectedActiveRevision;set(b,a2,f.dependencies);auto cross=a2;cross.validationReport=a.validationReport;rejectActivation(cross,"report from another candidate");
 // Injected storage failures at every precommit boundary preserve exact old.
 for(const QString phase:{"after-open","after-partial-write","after-write","before-commit"}){auto result=activator.activateFixture(a2,b,[&](const QString& p){if(p==phase)throw std::runtime_error("injected-write-failure");});check(!result.accepted,"write failure surfaced");check(activator.remembered(a.station)==original,"write failure preserves old");}
 // Two independent service instances and one exact expected revision: only one
 // wins. The file lock fences concurrent CAS even across service instances.
 HandEyeActivationResult race1,race2;std::thread t1([&]{race1=activator.activateFixture(a2,b);});std::thread t2([&]{race2=HandEyeActivationService(archive.path()).activateFixture(a2,b);});t1.join();t2.join();check(int(race1.accepted)+int(race2.accepted)==1,"single CAS winner");
 auto post=a2;post.expectedActiveRevision=QUuid(activator.remembered(a.station)["revision"].toString());auto postResult=activator.activateFixture(post,b,[](const QString& phase){if(phase=="after-commit")throw std::runtime_error("notification failure after durable commit");});check(postResult.accepted,"postcommit observer error cannot report rollback");
 check(HandEyeActivationService(archive.path()).remembered(a.station)["candidate"]==id(a2.candidate),"new full active visible");check(!activator.activateProduction(a2).accepted,"fixture never opens physical");
 // ProductionStore refuses an attempted overwrite of the immutable report.
 auto tampered=passed.report;tampered["state"]="FAILED";bool refused=false;try{store.append("hand_eye_validation",passed.report["report"].toString(),tampered);}catch(...){refused=true;}check(refused,"immutable report overwrite refused");
}
void crashes(const QString& mode){
 auto f=fixture(mode);
 for(bool initial:{true,false})for(const QString phase:{"after-open","after-partial-write","after-write","before-commit","after-commit"}){
  QTemporaryDir dir;auto c=HandEyeCandidateSolver{}.solveFixture(f,dir.path());check(c.accepted,"crash fixture solve");auto v=HandEyeValidationService(dir.path()).validateFixture(validation(f,c.report,mode));check(v.accepted,"crash fixture validate");auto a=activation(v);Boundary b;set(b,a,f.dependencies);HandEyeActivationService service(dir.path());
  QJsonObject old;if(!initial){auto first=service.activateFixture(a,b);check(first.accepted,"old active");old=first.active;a.expectedActiveRevision=QUuid(old["revision"].toString());}
  write(dir.path()+"/crash-request.json",{{"candidate",id(a.candidate)},{"candidateRevision",id(a.candidateRevision)},{"report",id(a.validationReport)},{"expected",a.expectedActiveRevision.isNull()?QString():id(a.expectedActiveRevision)},{"dependency",a.dependenciesDigest}});
  QProcess process;process.start(QCoreApplication::applicationFilePath(),{"--crash",dir.path(),mode,phase});check(process.waitForStarted(10000)&&process.waitForFinished(30000),"crash child completes");check(process.exitCode()==91,"child reached intended crash boundary");
  auto restored=HandEyeActivationService(dir.path()).remembered(a.station);
  if(phase=="after-commit"){check(!restored.isEmpty()&&restored["revision"]!=old["revision"]&&restored["candidate"]==id(a.candidate),"postcommit crash complete new state");}
  else check(restored==old,"precommit crash exact old or absent state");
  // Actual crash leaves the lock file; retry must safely reclaim the dead PID.
  a.expectedActiveRevision=restored.isEmpty()?QUuid():QUuid(restored["revision"].toString());check(service.activateFixture(a,b).accepted,"crash recovery CAS");
 }
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 if(argc==5&&QString(argv[1])=="--crash"){const QString dir=QString::fromLocal8Bit(argv[2]),mode=QString::fromLocal8Bit(argv[3]),phase=QString::fromLocal8Bit(argv[4]);auto f=fixture(mode);auto o=read(dir+"/crash-request.json");HandEyeActivationRequest a{uid(800),f.session,QUuid(o["candidate"].toString()),QUuid(o["candidateRevision"].toString()),QUuid(o["report"].toString()),QUuid(o["expected"].toString()),o["dependency"].toString()};Boundary b;set(b,a,f.dependencies);auto result=HandEyeActivationService(dir).activateFixture(a,b,[&](const QString& p){if(p==phase)std::_Exit(91);});QTextStream(stderr)<<result.reason;return 2;}
 for(const QString mode:{"eye-in-hand","eye-to-hand"}){run(mode);crashes(mode);}QTextStream(stdout)<<checks<<" checks passed\n";return 0;
 }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}}
