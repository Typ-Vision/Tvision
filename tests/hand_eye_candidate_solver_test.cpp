#include "application/HandEyeCandidateSolver.hpp"
#include "application/ProductionStore.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTextStream>
#include <cmath>
#include <limits>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
QUuid uid(int n){return QUuid(QString("00000000-0000-0000-0000-%1").arg(n,12,16,QChar('0')));}
QJsonObject read(const QString& p){QFile f(p);check(f.open(QIODevice::ReadOnly),"open golden");return QJsonDocument::fromJson(f.readAll()).object();}
HandEyeFixtureRequest request(const QJsonObject& o) {
 HandEyeFixtureRequest r;r.evidence=EvidenceState::Synthetic;r.session=uid(900);r.plateDiagonalMm=100;
 for(auto value:o["samples"].toArray()) {auto s=value.toObject();r.samples.push_back({QUuid(s["sample"].toString()),QUuid(s["frame"].toString()),s["holdout"].toBool(),SpatialTransform::fromJson(s["baseTFlange"].toObject()),SpatialTransform::fromJson(s["cameraTPlate"].toObject()),.1,.2,.3});}
 r.base=r.samples[0].baseTFlange.target;r.flange=r.samples[0].baseTFlange.source;r.camera=r.samples[0].cameraTPlate.target;r.plate=r.samples[0].cameraTPlate.source;
 auto& d=r.dependencies;d.mode=o["mode"]=="eye-in-hand"?HandEyeMode::EyeInHand:HandEyeMode::EyeToHand;d.cameraKey=1;d.cameraRevision=uid(10);d.intrinsicRevisionKey=2;d.intrinsicRevision=uid(11);d.plateRevisionKey=3;d.plateRevision=uid(12);d.robotAdapterRevisionKey=4;d.robotAdapterRevision=uid(13);d.acquisitionProfileRevisionKey=5;d.acquisitionProfileRevision=uid(14);d.modelRevisionKey=6;d.modelRevision=uid(15);d.pose={QUuid(r.base.frameUuid),uid(16),QUuid(r.flange.frameUuid),uid(17),2};d.cameraId="fixture-camera";d.physical={"fixture","camera","synthetic",DeviceIdentityConfidence::SerialReported};return r;
}
void near(const SpatialTransform& a,const SpatialTransform& b){check(a.source==b.source&&a.target==b.target,"output direction");double e=0;for(int k=0;k<3;++k)e+=std::pow(a.translationMm[k]-b.translationMm[k],2);check(std::sqrt(e)<=1e-6,"ground truth mm");check(a.rotation.equivalent(b.rotation,5e-8),"ground truth rotation");}
void run(const QString& mode) {
 auto golden=read(QString(TEST_DATA_DIR)+"/"+mode+".json");auto r=request(golden);QTemporaryDir archive;check(archive.isValid(),"archive");HandEyeCandidateSolver solver;
 // Active evidence is deliberately pre-existing and must survive all outcomes.
 ProductionStore store(archive.path());store.append("snapshots",uid(999).toString(QUuid::WithoutBraces),{{"sentinel",123}});const auto active=store.get("snapshots",uid(999).toString(QUuid::WithoutBraces));
 auto result=solver.solveFixture(r,archive.path());if(!result.accepted)throw std::runtime_error(result.reason.toStdString());check(result.accepted,"golden accepted");
 auto out=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS");if(!out.isEmpty()){check(QDir().mkpath(out),"evidence directory");QFile evidence(out+"/"+mode+"-candidate.json");check(evidence.open(QIODevice::WriteOnly),"evidence file");evidence.write(QJsonDocument(result.report).toJson());}
 auto x=SpatialTransform::fromJson(result.report["transform"].toObject());auto fixed=SpatialTransform::fromJson(result.report["fixedTransform"].toObject());near(x,SpatialTransform::fromJson(golden["groundTruth"].toObject()));near(fixed,SpatialTransform::fromJson(golden["fixedTruth"].toObject()));
 // Independent domain quaternion composition reconstructs S7, including holdout;
 // it does not reuse OpenCV matrix helpers or the solver's reported residual.

 for(const auto& sample:r.samples){auto y=r.dependencies.mode==HandEyeMode::EyeInHand ? sample.cameraTPlate.then(x).then(sample.baseTFlange) : sample.cameraTPlate.then(x).then(sample.baseTFlange.inverse());near(y,fixed);}
 check(result.report["physicalEligible"]==false,"fixture is not physical");
 check(result.report["dependencyDigest"].toString().size()==64,"dependency digest");
 check(store.get("hand_eye_candidate",result.report["candidate"].toString())==result.report,"durable full provenance");
 auto expectReject=[&](HandEyeFixtureRequest bad,const char* why){auto count=store.all("hand_eye_candidate").size();auto failed=solver.solveFixture(bad,archive.path());if(failed.accepted)throw std::runtime_error(why);check(!failed.reason.isEmpty(),"explicit reason");check(store.all("hand_eye_candidate").size()==count,"reject archives no candidate");check(store.get("snapshots",uid(999).toString(QUuid::WithoutBraces))==active,"active survives failure");return failed;};
 auto bad=r;bad.evidence=EvidenceState::PhysicalVerified;expectReject(bad,"physical gate");bad=r;bad.evidence=EvidenceState::Missing;expectReject(bad,"missing evidence");
 bad=r;bad.samples.resize(11);expectReject(bad,"too few");
 bad=r;for(auto& sample:bad.samples)sample.baseTFlange=r.samples[0].baseTFlange;expectReject(bad,"same pose");
 bad=r;for(size_t i=0;i<bad.samples.size();++i){double a=.15*i;bad.samples[i].baseTFlange.rotation={std::cos(a/2),0,0,std::sin(a/2)};}expectReject(bad,"one axis");
 bad=r;for(auto& sample:bad.samples){sample.baseTFlange.translationMm={0,0,0};}expectReject(bad,"translation coverage");
 bad=r;for(auto& sample:bad.samples){sample.baseTFlange.rotation={std::cos(.001),std::sin(.001),0,0};}expectReject(bad,"low diversity");
 bad=r;bad.samples[0].cameraTPlate=bad.samples[0].cameraTPlate.inverse();expectReject(bad,"inverse direction");
 bad=r;for(auto& sample:bad.samples){auto inverse=sample.cameraTPlate.inverse();inverse.source=r.plate;inverse.target=r.camera;sample.cameraTPlate=inverse;}expectReject(bad,"wrong convention silently relabeled");
 bad=r;bad.dependencies.mode=bad.dependencies.mode==HandEyeMode::EyeInHand?HandEyeMode::EyeToHand:HandEyeMode::EyeInHand;expectReject(bad,"wrong setup mode");
 bad=r;for(auto& sample:bad.samples){auto inverse=sample.baseTFlange.inverse();inverse.source=r.flange;inverse.target=r.base;sample.baseTFlange=inverse;}expectReject(bad,"robot inverse relabeled");
 bad=r;bad.samples[3].baseTFlange.translationMm[0]+=30;expectReject(bad,"wrong TCP offset");
 bad=r;bad.samples[0].rmsPx=.501;auto quality=expectReject(bad,"plate RMS");check(!quality.report["rejectedSamples"].toArray().isEmpty(),"rejected sample reason retained");
 bad=r;bad.samples[0].maxPx=1.501;expectReject(bad,"plate max");bad=r;bad.samples[0].ambiguityMarginPx=.199;expectReject(bad,"ambiguity");
 bad=r;bad.samples[0].rmsPx=std::numeric_limits<double>::quiet_NaN();expectReject(bad,"NaN");
 bad=r;bad.samples.back().sample=bad.samples[0].sample;expectReject(bad,"holdout overlap");bad=r;bad.samples.back().frame=bad.samples[0].frame;expectReject(bad,"frame overlap");
 bad=r;bad.dependencies.intrinsicRevision=QUuid{};expectReject(bad,"missing dependency");bad=r;bad.policy.minSolveSamples=3;expectReject(bad,"policy tamper");
 bad=r;bad.samples[0].baseTFlange.source.revision++;expectReject(bad,"frame revision");
 // A holdout's numbers never influence candidate or training dataset digest.
 auto held=r;held.samples.back().cameraTPlate.translationMm[0]+=10000;auto heldResult=solver.solveFixture(held,archive.path());check(heldResult.accepted,"holdout excluded");check(heldResult.report["sampleSetDigest"]==result.report["sampleSetDigest"],"holdout excluded from digest");
 auto dep=r;dep.dependencies.intrinsicRevision=uid(55);auto depResult=solver.solveFixture(dep,archive.path());check(depResult.accepted,"new dependency snapshot");check(depResult.report["dependencyDigest"]!=result.report["dependencyDigest"],"dependency changes digest");
 auto boundary=r;for(auto& sample:boundary.samples){sample.rmsPx=.5;sample.maxPx=1.5;sample.ambiguityMarginPx=.2;}check(solver.solveFixture(boundary,archive.path()).accepted,"quality exact boundaries");
 QFile blocker(archive.path()+"/file");check(blocker.open(QIODevice::WriteOnly),"blocker");blocker.write("blocked");blocker.close();check(!solver.solveFixture(r,blocker.fileName()).accepted,"archive failure");
 check(store.get("snapshots",uid(999).toString(QUuid::WithoutBraces))==active,"active unchanged after successes and failures");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{run("eye-in-hand");run("eye-to-hand");QTextStream(stdout)<<checks<<" checks passed\n";return 0;}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}}
