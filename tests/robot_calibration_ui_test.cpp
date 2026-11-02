#include "app/RobotCalibrationPanel.hpp"
#include "application/HandEyeValidationService.hpp"
#include "application/RobotCalibrationSessionService.hpp"
#include "persistence/RobotCalibrationSessionStore.hpp"
#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <QKeyEvent>
#include <mutex>
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

using A=RobotCalibrationAction;
struct Dependencies:CalibrationDependencyProvider {
    CalibrationDependencySnapshot value;
    std::optional<CalibrationDependencySnapshot> current(const QUuid&,quint32 key) const override {return key==value.cameraKey?std::optional(value):std::nullopt;}
};
struct Drain:CalibrationDrainPort {bool ready=false;int calls=0;bool drain(const QUuid&,const QString&) override {++calls;return ready;}};
CommunicationV4Frame begin(const HandEyeFixtureRequest& f){
    const auto& d=f.dependencies;using C=CommunicationV4Codec;
    CommunicationV4Frame r;r.header={20,1,1,uid(90),uid(91),uid(92),0,0,{}};r.body=QByteArray(384,0);
    auto& b=r.body;b.replace(0,16,f.session.toRfc4122());C::putUInt32(b,16,d.mode==HandEyeMode::EyeToHand?1:2);C::putUInt32(b,20,d.cameraKey);b.replace(24,16,d.cameraRevision.toRfc4122());C::putUInt32(b,40,d.intrinsicRevisionKey);C::putUInt32(b,44,d.plateRevisionKey);C::putUInt32(b,48,d.flangeTcpRevisionKey);C::putUInt32(b,52,d.robotAdapterRevisionKey);
    b.replace(112,16,d.pose.targetFrame.toRfc4122());b.replace(128,16,d.pose.targetFrameRevision.toRfc4122());b.replace(144,16,d.pose.sourceFrame.toRfc4122());b.replace(160,16,d.pose.sourceFrameRevision.toRfc4122());C::putUInt32(b,176,2);return r;
}
void run(const QString& mount,QApplication& app,const QString& artifacts){
    auto f=fixture(mount);QTemporaryDir root;check(root.isValid(),"fixture directory");
    Dependencies deps;deps.value=f.dependencies;Drain drain;FileRobotCalibrationSessionStore store(root.filePath("sessions.json"));RobotCalibrationSessionService sessions(store,deps,drain);
    HandEyeValidationService validator(root.path());HandEyeActivationService activator(root.path());Boundary boundary;
    RobotCalibrationView state;state.station=uid(800);state.mode=f.dependencies.mode;state.revision=1;state.connected=true;state.fixture=true;state.physical="PHYSICAL BLOCKED • Synthetic golden fixture; üretim izni yok";
    state.frameReadiness="Fixture exact base/flange UUID + revision";state.tcpReadiness="Actual flange; TCP dönüşümü uygulanmıyor";state.lensReadiness="Synthetic pinhole fixture; fiziksel lens bekleniyor";state.actualPose="Actual pose yok";state.camera=f.dependencies.cameraId;state.leaseOwner="yok";state.drain="Edinim yok";
    state.commands={{A::Begin,{}},{A::SampleSolve,{}},{A::SampleHoldout,{}},{A::Solve,"Önce BEGIN"},{A::Validate,"Candidate yok"},{A::Activate,"Validation yok"},{A::Abort,"Session yok"},{A::Recover,"Recovery gerekmiyor"}};
    RobotCalibrationWorkspace workspace;QMap<A,int> dispatched;QJsonObject candidate;HandEyeValidationResult validated;auto request=begin(f);
    const V4RequestIdentity key{uid(93),uid(91),uid(92),state.station,1};
    workspace.bind([&](const QUuid& station,HandEyeMode mode){auto v=state;v.station=station;v.mode=mode;if(station!=state.station){v.commands.clear();v.connected=false;v.session=QUuid{};v.leaseOwner="Station 2 production owner • bağımsız";}return v;},[&](const RobotCalibrationCommand& c){
        ++dispatched[c.action];++state.revision;
        if(c.action==A::Begin){check(sessions.begin(key,request).accepted,"Real BEGIN service");state.session=f.session;state.state=CalibrationState::Created;state.commands[A::Begin]="Oturum zaten açık";state.commands[A::Solve]={};state.commands[A::Abort]={};}
        else if(c.action==A::Solve){const auto solved=HandEyeCandidateSolver{}.solveFixture(f,root.path());check(solved.accepted,"Real two-geometry candidate solve");candidate=solved.report;state.candidate=QUuid(candidate["candidate"].toString());state.candidateRevision=QUuid(candidate["candidateRevision"].toString());state.state=CalibrationState::Candidate;state.commands[A::Validate]={};check(activator.remembered(state.station).isEmpty(),"Candidate never changes active selection");}
        else if(c.action==A::Validate){validated=validator.validateFixture(validation(f,candidate,mount));check(validated.accepted,"Real independent golden holdout validation");state.validation=QUuid(validated.report["report"].toString());state.state=CalibrationState::Validated;state.commands[A::Activate]={};}
        else if(c.action==A::Activate){auto a=activation(validated);set(boundary,a,f.dependencies);const auto result=activator.activateFixture(a,boundary);check(result.accepted,"Real fixture atomic CAS activation");state.active=state.candidate;state.activeRevision=QUuid(result.active["revision"].toString());state.state=CalibrationState::Active;check(!activator.activateProduction(a).accepted,"UI fixture cannot open production activation");}
        else if(c.action==A::Abort){auto abort=request;abort.header.kind=25;abort.header.requestId=2;abort.body=QByteArray(384,0);abort.body.replace(0,16,f.session.toRfc4122());CommunicationV4Codec::putUInt32(abort.body,16,1);auto ak=key;ak.request=2;const auto result=sessions.abort(ak,abort);const auto record=sessions.session(f.session);check(record.has_value(),"Durable ABORT record");state.draining=record->abortRequested;state.recoveryRequired=record->recoveryRequired;state.state=record->state;state.drain=state.draining?"DRAINING • gerçek worker kapanışı bekleniyor":"ABORTED • drain tamamlandı";if(drain.ready)check(result.accepted && !sessions.ownsCamera(f.dependencies.cameraId),"Lease released only after real drain completion");else check(!result.accepted && sessions.ownsCamera(f.dependencies.cameraId),"Incomplete drain retains actual lease");}
        else if(c.action==A::Recover){check(sessions.recover(f.session).accepted,"Explicit real session recovery");state.recoveryRequired=false;}
        else throw std::runtime_error("SAMPLE fixture absent: actual pose and capture adapter required");
        state.leaseOwner=sessions.ownsCamera(f.dependencies.cameraId)?"calibration:"+id(f.session):"yok";return QString("Application komutu tamamlandı; fixture fiziksel kabul değildir");
    });
    QWidget host;host.setWindowTitle("VisionFlow COMM4-24 • "+mount+" • synthetic software acceptance");auto* layout=new QVBoxLayout(&host);auto* stationHint=new QLabel("Station 2 • bağımsız production durumu Communication tablosunda erişilebilir");stationHint->setWordWrap(true);layout->addWidget(stationHint);auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);layout->addWidget(scroll);auto* panel=new RobotCalibrationPanel;scroll->setWidget(panel);panel->bind(&workspace);
    auto* modes=panel->findChild<QComboBox*>("robotCalibrationMode");modes->setCurrentIndex(mount=="eye-in-hand"?0:1);panel->selectStation(state.station);host.show();QApplication::processEvents();
    auto button=[&](A a){return panel->findChild<QPushButton*>("robotCalibrationAction"+QString::number(int(a)));};
    auto click=[&](A a){check(button(a)->isEnabled(),"Application command enabled");button(a)->click();check(panel->findChild<QLabel*>("robotCalibrationMessage")->text().contains("Application"),"Application command success visible");};
    for(const auto& s:f.samples)state.samples.append({s.sample,s.frame,s.holdout?"holdout":"solve","GOLDEN INPUT",{},"Committed fixture UUID pair; live token yok"});panel->refresh();check(panel->findChild<QTableWidget*>("robotCalibrationSamples")->item(0,0)->text()==id(f.samples[0].sample),"Exact sample UUID displayed");
    auto screenshot=[&](const QString& phase){QApplication::processEvents();check(host.grab().save(artifacts+"/"+mount+"-"+phase+".png"),"Viewport screenshot");};
    host.resize(1400,900);QApplication::processEvents();check(host.size()==QSize(1400,900),"Normal viewport");
    check(!button(A::SampleSolve)->isEnabled() && !button(A::SampleHoldout)->isEnabled(),"No actual pose disables both SAMPLE roles");check(button(A::SampleSolve)->accessibleDescription().contains("Actual pose"),"Disabled reason accessible");
    screenshot("normal-missing-pose");click(A::Begin);check(state.active.isNull(),"BEGIN does not activate");
    // SAMPLE remains unavailable even if a provider advertises it; GUI cannot
    // substitute an identity pose. Exact capture service acceptance is COMM4-21.
    const auto stale=workspace.view(state.station,state.mode);++state.revision;bool rejected=false;try{workspace.command({A::Solve,stale});}catch(...){rejected=true;}check(rejected && dispatched[A::Solve]==0,"Stale screen cannot dispatch");panel->refresh();
    click(A::Solve);check(state.active.isNull() && panel->findChild<QLabel*>("robotCalibrationModels")->text().contains("Active: yok"),"Real candidate separated from active");screenshot("candidate");
    click(A::Validate);check(state.active.isNull(),"Real validation preserves active");click(A::Activate);check(!state.active.isNull(),"Explicit fixture activation");screenshot("validated-active-fixture");
    state.connected=false;state.recoveryRequired=true;++state.revision;panel->refresh();check(!button(A::SampleSolve)->isEnabled(),"Disconnect never recaptures");check(button(A::Abort)->isEnabled(),"ABORT available while disconnected");screenshot("reconnect-recovery");
    state.connected=true;state.recoveryRequired=false;++state.revision;panel->refresh();click(A::Abort);check(state.draining && drain.calls==1,"Real drain pending visible");check(panel->findChild<QLabel*>("robotCalibrationProgress")->text().contains("DRAINING"),"Actual drain progress projection");screenshot("abort-draining");
    drain.ready=true;click(A::Abort);check(!state.draining && state.state==CalibrationState::Aborted,"Real ABORT drain completes");
    host.resize(850,600);QApplication::processEvents();QTextStream(stdout)<<"small actual="<<host.width()<<"x"<<host.height()<<" panelMin="<<panel->minimumSizeHint().width()<<"x"<<panel->minimumSizeHint().height()<<"\n";check(host.size()==QSize(850,600),"Small viewport");screenshot("small-aborted");scroll->ensureWidgetVisible(button(A::Abort));button(A::Abort)->setFocus();QApplication::processEvents();check(button(A::Abort)->hasFocus(),"Small viewport keyboard focus");QKeyEvent tab(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);QApplication::sendEvent(button(A::Abort),&tab);check(QApplication::focusWidget()!=button(A::Abort),"Tab traversal");screenshot("small-keyboard");
    panel->selectStation(uid(801));check(panel->findChild<QLabel*>("robotCalibrationReadiness")->text().contains("Station 2"),"Independent station selection available");panel->selectStation(state.station);
    check(dispatched[A::SampleSolve]==0 && dispatched[A::SampleHoldout]==0,"No fabricated pose/capture");
    QTextStream(stdout)<<mount<<" platform="<<QApplication::platformName()<<" DPR="<<host.devicePixelRatioF()<<" normal=1400x900 small=850x600\n";
    if(app.arguments().contains("--preview") || app.arguments().contains("--preview-normal")){if(app.arguments().contains("--preview-normal"))host.resize(1400,900);host.show();host.raise();host.activateWindow();app.exec();}
}
}
int main(int argc,char** argv){QApplication app(argc,argv);QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");app.setFont(QFont("Segoe UI",10));app.setStyle("Fusion");try{const auto artifacts=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS","D:/TVision/build/comm4/COMM4-24/ui");QDir().mkpath(artifacts);run("eye-in-hand",app,artifacts);if(!app.arguments().contains("--single-mode"))run("eye-to-hand",app,artifacts);QTextStream(stdout)<<"PASS "<<checks<<" robot calibration UI / real application service checks\n";return 0;}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
