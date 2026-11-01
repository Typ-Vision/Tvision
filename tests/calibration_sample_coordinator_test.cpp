#include "application/CalibrationSampleCoordinator.hpp"
#include "application/ProductionStore.hpp"
#include "IndustrialProviderFixture.hpp"
#include "persistence/RobotCalibrationSessionStore.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <vector>

using namespace visionflow;
using C = CommunicationV4Codec;

namespace {
int checks = 0;
quint64 nextRequest = 1;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

QUuid kInstallation(QStringLiteral("11111111-1111-4111-8111-111111111111"));
QUuid kPeer(QStringLiteral("22222222-2222-4222-8222-222222222222"));
QUuid kEpoch(QStringLiteral("33333333-3333-4333-8333-333333333333"));
QUuid kStation(QStringLiteral("44444444-4444-4444-8444-444444444444"));
QUuid kConnection(QStringLiteral("55555555-5555-4555-8555-555555555555"));
QUuid kOtherStation(QStringLiteral("66666666-6666-4666-8666-666666666666"));

struct Provider final : CalibrationDependencyProvider {
    struct Entry {
        QUuid station;
        quint32 cameraKey = 0;
        CalibrationDependencySnapshot snapshot;
    };
    std::vector<Entry> entries;
    bool fail = false;
    std::optional<CalibrationDependencySnapshot> current(const QUuid& station, quint32 cameraKey) const override {
        if (fail) throw std::runtime_error("synthetic dependency provider failure");
        for (const auto& entry : entries)
            if (entry.station == station && entry.cameraKey == cameraKey) return entry.snapshot;
        return std::nullopt;
    }
    void set(const QUuid& station, const CalibrationDependencySnapshot& snapshot) {
        entries.push_back({station, snapshot.cameraKey, snapshot});
    }
};

struct Drain final : CalibrationDrainPort {
    int calls = 0;
    bool clean = true;
    std::function<void()> during;
    bool drain(const QUuid&, const QString&) override {
        ++calls;
        if (during) during();
        return clean;
    }
};

struct MemoryStore final : CalibrationSessionStore {
    std::vector<CalibrationSessionRecord> sessions;
    std::vector<CalibrationCommandRecord> commands;
    int commitCalls = 0;
    int failAtCommit = -1;

    std::optional<CalibrationCommandRecord> findCommand(const CalibrationCommandKey& key) const override {
        for (const auto& record : commands)
            if (record.key == key) return record;
        return std::nullopt;
    }
    std::optional<CalibrationSessionRecord> findSession(const QUuid& id) const override {
        for (const auto& record : sessions)
            if (record.session == id) return record;
        return std::nullopt;
    }
    std::vector<CalibrationSessionRecord> loadAll() const override { return sessions; }
    void commit(const CalibrationSessionRecord& record,
                const std::optional<CalibrationCommandRecord>& command = std::nullopt) override {
        ++commitCalls;
        if (failAtCommit == commitCalls) {
            failAtCommit = -1;
            throw std::runtime_error("synthetic persistence failure");
        }
        bool replaced = false;
        for (auto& existing : sessions)
            if (existing.session == record.session) { existing = record; replaced = true; break; }
        if (!replaced) sessions.push_back(record);
        if (command) {
            bool found = false;
            for (auto& existing : commands)
                if (existing.key == command->key) { existing = *command; found = true; break; }
            if (!found) commands.push_back(*command);
        }
    }
    void failNext() { failAtCommit = commitCalls + 1; }
    void failAfter(int commits) { failAtCommit = commitCalls + commits; }
};

CalibrationDependencySnapshot dependency(const QString& tag, quint32 cameraKey,
                                         HandEyeMode mode = HandEyeMode::EyeInHand) {
    auto uuid = [] { return QUuid::createUuid(); };
    CalibrationDependencySnapshot snapshot;
    snapshot.mode = mode;
    snapshot.cameraKey = cameraKey;
    snapshot.cameraRevision = uuid();
    snapshot.intrinsicRevisionKey = 100 + cameraKey;
    snapshot.intrinsicRevision = uuid();
    snapshot.plateRevisionKey = 200 + cameraKey;
    snapshot.plateRevision = uuid();
    snapshot.flangeTcpRevisionKey = 0;
    snapshot.flangeTcpRevision = QUuid{};
    snapshot.robotAdapterRevisionKey = 300 + cameraKey;
    snapshot.robotAdapterRevision = uuid();
    snapshot.acquisitionProfileRevisionKey = 400 + cameraKey;
    snapshot.acquisitionProfileRevision = uuid();
    snapshot.modelRevisionKey = 500 + cameraKey;
    snapshot.modelRevision = uuid();
    snapshot.pose = CalibrationPoseContract{uuid(), uuid(), uuid(), uuid(), 2};
    snapshot.cameraId = QStringLiteral("camera-") + tag;
    snapshot.physical = {QStringLiteral("VF"), QStringLiteral("CAL"), tag,
                         DeviceIdentityConfidence::SerialReported};
    check(snapshot.complete(), "fixture dependency snapshot is complete");
    return snapshot;
}

struct Wire {
    QUuid installation = kInstallation;
    QUuid peer = kPeer;
    QUuid epoch = kEpoch;
    QUuid station = kStation;
    QUuid connection = kConnection;
    quint64 request = 0;
    quint32 stationKey = 1;
    QUuid session;
    HandEyeMode mode = HandEyeMode::EyeInHand;
    quint32 cameraKey = 0;
    QUuid cameraRevision;
    quint32 intrinsic = 0;
    quint32 plate = 0;
    quint32 flangeTcp = 0;
    quint32 adapter = 0;
    CalibrationPoseContract pose;
};

Wire wireFor(const CalibrationDependencySnapshot& snapshot) {
    Wire wire;
    wire.request = nextRequest++;
    wire.session = QUuid::createUuid();
    wire.mode = snapshot.mode;
    wire.cameraKey = snapshot.cameraKey;
    wire.cameraRevision = snapshot.cameraRevision;
    wire.intrinsic = snapshot.intrinsicRevisionKey;
    wire.plate = snapshot.plateRevisionKey;
    wire.flangeTcp = snapshot.flangeTcpRevisionKey;
    wire.adapter = snapshot.robotAdapterRevisionKey;
    wire.pose = snapshot.pose;
    return wire;
}

V4RequestIdentity identity(const Wire& wire) {
    return {wire.installation, wire.peer, wire.epoch, wire.station, wire.request};
}

V4RequestIdentity identity(const CommunicationV4Frame& frame) {
    return {kInstallation, frame.header.peer, frame.header.epoch, kStation, frame.header.requestId};
}

V4RequestIdentity abortIdentity(const CommunicationV4Frame& frame, const QUuid& station) {
    return {kInstallation, frame.header.peer, frame.header.epoch, station, frame.header.requestId};
}

CommunicationV4Frame beginFrame(const Wire& wire) {
    CommunicationV4Frame frame;
    frame.header.kind = 20;
    frame.header.stationKey = wire.stationKey;
    frame.header.requestId = wire.request;
    frame.header.session = wire.connection;
    frame.header.peer = wire.peer;
    frame.header.epoch = wire.epoch;
    frame.body = QByteArray(384, 0);
    auto& body = frame.body;
    body.replace(0, 16, wire.session.toRfc4122());
    C::putUInt32(body, 16, wire.mode == HandEyeMode::EyeToHand ? 1 : 2);
    C::putUInt32(body, 20, wire.cameraKey);
    body.replace(24, 16, wire.cameraRevision.toRfc4122());
    C::putUInt32(body, 40, wire.intrinsic);
    C::putUInt32(body, 44, wire.plate);
    C::putUInt32(body, 48, wire.flangeTcp);
    C::putUInt32(body, 52, wire.adapter);
    body.replace(112, 16, wire.pose.targetFrame.toRfc4122());
    body.replace(128, 16, wire.pose.targetFrameRevision.toRfc4122());
    body.replace(144, 16, wire.pose.sourceFrame.toRfc4122());
    body.replace(160, 16, wire.pose.sourceFrameRevision.toRfc4122());
    C::putUInt32(body, 176, wire.pose.convention);
    return frame;
}

}
namespace {
CommunicationV4Frame sampleFrame(const Wire& w,quint64 request,quint32 role=1){
    auto f=beginFrame(w);f.header.kind=21;f.header.requestId=request;f.body.fill(0);
    f.body.replace(0,16,w.session.toRfc4122());f.body.replace(16,16,QUuid::createUuid().toRfc4122());
    C::putUInt32(f.body,32,w.cameraKey);C::putUInt32(f.body,36,1);f.body.replace(40,16,w.cameraRevision.toRfc4122());
    C::putUInt64(f.body,56,request);C::putUInt64(f.body,64,123);C::putUInt32(f.body,72,1);C::putUInt32(f.body,76,1);
    C::putFloat64(f.body,80,42);C::putFloat64(f.body,104,1);
    f.body.replace(136,16,w.pose.targetFrame.toRfc4122());f.body.replace(152,16,w.pose.targetFrameRevision.toRfc4122());
    f.body.replace(168,16,w.pose.sourceFrame.toRfc4122());f.body.replace(184,16,w.pose.sourceFrameRevision.toRfc4122());
    C::putUInt32(f.body,200,1);C::putUInt32(f.body,208,400+w.cameraKey);C::putUInt32(f.body,212,role);return f;
}
IndependentCaptureRequest captureRequest(const QString& camera){
    IndependentCaptureRequest r;r.origin=SourceOrigin::CameraAcquired;
    r.camera=MeasurementCamera{{camera},"Fixture camera",{"webcam","7174",DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}};
    r.setup=MeasurementSetupDescriptor{{"setup-A"},r.camera->id,"Fixture setup","lens","plane",1,1,{{64,32},{},{},true}};return r;
}
class RasterAdapter final:public AcquisitionAdapter {
public:
    int* captures;bool fail=false;
    explicit RasterAdapter(int& n,bool f=false):captures(&n),fail(f){}
    AcquisitionCapabilities capabilities() const override{return {"webcam",true};}
    ImageFrame acquire(const AcquisitionRequest&) const override {
        ++*captures;if(fail)throw std::runtime_error("fixture capture failure");
        QImage image(64,32,QImage::Format_Grayscale8);image.fill(42);
        return ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),64,32,image.bytesPerLine(),PixelFormat::Gray8);
    }
};
IndependentCaptureBackend backendFor(const IndependentCaptureRequest& r,int& n,bool fail=false){
    IndependentCaptureBackend b;b.discover=[r]{return QVector<DeviceBinding>{r.camera->device};};
    b.create=[&n,fail](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<RasterAdapter>(n,fail);};return b;
}
void ack(StationDeliveryLedger& ledger,const V4RequestIdentity& key,const CommunicationV4Frame& t){
    CommunicationV4Frame a{t.header,QByteArray(384,0)};a.header.kind=11;
    a.body.replace(0,16,t.body.first(16));a.body.replace(16,32,t.body.sliced(32,32));ledger.acknowledge(key,a);
}
bool release(const CommunicationV4Frame& f){return f.header.kind==111 && (C::uint32(f.body,100)==6 || C::uint32(f.body,100)==7);}
void pairing(){
    QTemporaryDir dir;MemoryStore store;Provider provider;Drain drain;
    auto d=dependency("pair",31);provider.set(kStation,d);auto w=wireFor(d);
    RobotCalibrationSessionService sessions(store,provider,drain);check(sessions.begin(identity(w),beginFrame(w)).accepted,"begin");
    StationDeliveryLedger ledger(dir.path(),kInstallation);CalibrationSampleCoordinator c(sessions,provider,ledger,true);
    auto request=captureRequest(d.cameraId);int captures=0;auto backend=backendFor(request,captures);
    auto f=sampleFrame(w,100);auto key=identity(f);auto admitted=c.sample(key,f,request);
    check(admitted.job && !release(admitted.response) && captures==0,"ACCEPTED no release no hardware before worker");
    check(sessions.session(w.session)->state==CalibrationState::Collecting,"durable COLLECTING");
    for(int i=0;i<100;++i){auto duplicate=c.sample(key,f,request);check(!duplicate.job && duplicate.response.header.kind==110,"100 duplicates no acquisition");}
    auto changed=f;C::putFloat64(changed.body,80,43);check(C::uint32(c.sample(key,changed,request).response.body,76)==6,"same ID different pose conflict");
    auto reconnect=f;reconnect.header.session=QUuid::createUuid();check(!c.sample(key,reconnect,request).job,"reconnect no additional job");
    auto result=CalibrationSampleCoordinator::run(*admitted.job,backend);check(captures==1,"exactly one acquisition");
    auto t=c.complete(*admitted.job,result);check(t && release(*t) && C::uint32(t->body,100)==6,"CAPTURED release");
    check(C::uint32(t->body,820)==31 && C::uint32(t->body,824)==21 && t->body.sliced(120,16)==f.body.sliced(16,16)
        && !QUuid::fromRfc4122(t->body.sliced(136,16)).isNull(),"exact camera origin sample frame echo");
    check(!c.complete(*admitted.job,result),"single use callback");
    auto replay=c.sample(key,reconnect,request);check(!replay.job && replay.response.body==t->body && captures==1,"sealed exact terminal replay");
    auto records=ProductionStore(dir.path()).all("calibration_sample");check(records.size()==2,"durable intent and frame pairing evidence");
    for(const auto& record:records)if(record["rasterBase64"].isString()){
        const auto bytes=QByteArray::fromBase64(record["rasterBase64"].toString().toLatin1());
        auto saved=ImageFrame::fromBuffer(bytes.data(),bytes.size(),record["width"].toInt(),record["height"].toInt(),record["stride"].toInt(),PixelFormat(record["pixelFormat"].toInt()));
        check(QString::fromLatin1(saved.contentId().toHex())==record["pixelHash"].toString(),"durable raw raster reproduces exact owned pixel hash");
    }
    ack(ledger,key,*t);
    auto sameSample=f;sameSample.header.requestId=101;check(C::uint32(c.sample(identity(sameSample),sameSample,request).response.body,76)==6,"sample UUID cannot rebind after ACK");
    auto holdout=sampleFrame(w,102,2);auto h=c.sample(identity(holdout),holdout,request);check(h.job && h.job->token()>admitted.job->token(),"holdout new token");
    auto hr=CalibrationSampleCoordinator::run(*h.job,backend);check(!c.complete(*h.job,result),"late previous raster cannot consume new ticket");
    auto ht=c.complete(*h.job,hr);check(ht && release(*ht) && captures==2,"own holdout completes");ack(ledger,identity(holdout),*ht);
    for(int mode=0;mode<7;++mode){auto bad=sampleFrame(w,200+mode);auto target=request;
        if(mode==0)C::putUInt32(bad.body,32,99);
        if(mode==1)bad.body.replace(40,16,QUuid::createUuid().toRfc4122());
        if(mode==2)bad.body.replace(136,16,QUuid::createUuid().toRfc4122());
        if(mode==3)bad.body.replace(0,16,QUuid::createUuid().toRfc4122());
        if(mode==4)C::putUInt32(bad.body,208,999);
        if(mode==5)bad.header.activation=QUuid::createUuid();
        if(mode==6)target.camera->id={"different"};
        auto no=c.sample(identity(bad),bad,target);check(!no.job && no.response.header.kind==190 && !release(no.response),"mismatch reject before capture");}
    check(captures==2,"rejections acquisition zero");
    auto timed=sampleFrame(w,300);auto a=c.sample(identity(timed),timed,request);auto late=CalibrationSampleCoordinator::run(*a.job,backend);
    check(c.timeout(w.session) && !c.complete(*a.job,late),"timeout fences late completed raster");
    check(!c.sample(identity(timed),timed,request).job,"timeout duplicate never recaptures");
    check(!c.sample(identity(sampleFrame(w,301)),sampleFrame(w,301),request).job,"new sample fenced until drain");
    drain.clean=false;check(!c.recover(w.session,drain),"failed drain no release");drain.clean=true;
    auto failed=c.recover(w.session,drain);check(failed && release(*failed) && C::uint32(failed->body,100)==7 && C::uint32(failed->body,72)==0 && C::uint64(failed->body,748)==0,"drained failure terminal release no success");
    ack(ledger,identity(timed),*failed);
    CalibrationSampleCoordinator production(sessions,provider,ledger);auto unsupported=sampleFrame(w,302);
    check(C::uint32(production.sample(identity(unsupported),unsupported,request).response.body,76)==17,"fixture not default physical authorization");
}
void sharedIndustrialLease(){
    QTemporaryDir dir;MemoryStore store;Provider provider;Drain drain;
    const auto source=industrial_provider_fixture::request();auto state=std::make_shared<industrial_provider_fixture::State>();
    auto d=dependency("industrial",61);d.cameraId=source.camera.id.value;d.physical=source.device.identity;provider.set(kStation,d);auto w=wireFor(d);
    RobotCalibrationSessionService sessions(store,provider,drain);check(sessions.begin(identity(w),beginFrame(w)).accepted,"industrial begin holds shared lease");
    StationDeliveryLedger ledger(dir.path(),kInstallation);CalibrationSampleCoordinator c(sessions,provider,ledger);
    IndependentCaptureRequest request;request.origin=SourceOrigin::CameraAcquired;request.camera=source.camera;request.setup=source.setup;request.profile=source.profile;
    IndependentCaptureBackend backend;backend.factory=CameraProviderFactory::forTests(std::make_shared<industrial_provider_fixture::Provider>(state));backend.industrialDiscover=[source]{return IndustrialDiscoverySnapshot{{source.device},{},false};};
    auto f=sampleFrame(w,800);auto a=c.sample(identity(f),f,request);check(a.job.has_value(),"industrial exact physical request admitted");
    auto result=CalibrationSampleCoordinator::run(*a.job,backend);check(result.error().isEmpty(),"existing calibration lease consumed by real industrial worker");
    auto terminal=c.complete(*a.job,result);check(terminal && release(*terminal),"industrial provider fixture reaches CAPTURED without second lease registry");
    check(sessions.ownsCamera(d.cameraId),"session retains lease after capture worker drains");
    auto reservation=CameraLeaseArbiter::reserve("other",CameraLeaseUse::Runtime,{{d.cameraId,d.physical}});check(reservation.reason==3,"production still blocked by calibration owner");
}
void candidateAndFailures(){
    QTemporaryDir dir;MemoryStore store;Provider provider;Drain drain;
    auto d=dependency("candidate",51);provider.set(kStation,d);auto w=wireFor(d);
    {RobotCalibrationSessionService initial(store,provider,drain);check(initial.begin(identity(w),beginFrame(w)).accepted,"candidate fixture begin");}
    store.sessions.front().state=CalibrationState::Candidate;
    RobotCalibrationSessionService sessions(store,provider,drain);check(sessions.recover(w.session).accepted,"candidate explicit session recovery");
    StationDeliveryLedger ledger(dir.path(),kInstallation);CalibrationSampleCoordinator c(sessions,provider,ledger,true);
    auto request=captureRequest(d.cameraId);int captures=0;
    auto solve=sampleFrame(w,700,1);check(!c.sample(identity(solve),solve,request).job,"candidate cannot append training data");
    auto holdout=sampleFrame(w,701,2);auto a=c.sample(identity(holdout),holdout,request);check(a.job.has_value(),"candidate accepts fresh holdout");
    auto failed=CalibrationSampleCoordinator::run(*a.job,backendFor(request,captures,true));
    check(!c.complete(*a.job,failed) && captures==1,"worker failure fences until actual drain");
    auto terminal=c.recover(w.session,drain);check(terminal && release(*terminal) && C::uint32(terminal->body,100)==7,"worker failure durable CAPTURE_FAILED");
    ack(ledger,identity(holdout),*terminal);
    auto next=sampleFrame(w,702,2);auto b=c.sample(identity(next),next,request);check(b.job.has_value(),"holdout retry is a new explicit sample");
    auto result=CalibrationSampleCoordinator::run(*b.job,backendFor(request,captures));auto captured=c.complete(*b.job,result);check(captured.has_value(),"candidate holdout captured");
    bool found=false;for(const auto& record:ProductionStore(dir.path()).all("calibration_sample"))if(record["frame"].isString())found=record["role"].toInt()==2;
    check(found && sessions.session(w.session)->state==CalibrationState::Candidate,"holdout role durable and candidate unchanged");
    ack(ledger,identity(next),*captured);
    auto last=sampleFrame(w,703,2);auto late=c.sample(identity(last),last,request);auto lateResult=CalibrationSampleCoordinator::run(*late.job,backendFor(request,captures));
    auto abort=beginFrame(w);abort.header.kind=25;abort.header.requestId=704;abort.body.fill(0);abort.body.replace(0,16,w.session.toRfc4122());C::putUInt32(abort.body,16,1);
    check(sessions.abort(identity(abort),abort).accepted && !c.complete(*late.job,lateResult),"ABORT prevents late pairing");
}
void restartAndFaults(){
    for(const QString fault:{QString(),QStringLiteral("before:INTENT"),QStringLiteral("after:INTENT"),QStringLiteral("after:BEGIN"),QStringLiteral("before:TERMINAL"),QStringLiteral("after:TERMINAL"),QStringLiteral("after:PENDING")}){
        QTemporaryDir dir;MemoryStore store;Provider provider;Drain drain;auto d=dependency("restart",41);provider.set(kStation,d);auto w=wireFor(d);
        RobotCalibrationSessionService sessions(store,provider,drain);check(sessions.begin(identity(w),beginFrame(w)).accepted,"restart fixture begin");
        bool armed=true;StationDeliveryLedger ledger(dir.path(),kInstallation,[&](const QString& point){if(armed && point==fault){armed=false;throw std::runtime_error("injected journal boundary");}});
        auto f=sampleFrame(w,500);auto key=identity(f);auto request=captureRequest(d.cameraId);int captures=0;
        {
            CalibrationSampleCoordinator c(sessions,provider,ledger,true);auto a=c.sample(key,f,request);
            if(a.job){auto result=CalibrationSampleCoordinator::run(*a.job,backendFor(request,captures));
                if(fault.isEmpty()){check(c.timeout(w.session),"crash simulation fences capture");}
                else c.complete(*a.job,result);
            }
        }
        StationDeliveryLedger reopened(dir.path(),kInstallation);CalibrationSampleCoordinator restored(sessions,provider,reopened,true);
        auto prior=reopened.inspect(key);
        if(!prior){check(captures==0,"before intent no capture");continue;}
        auto duplicate=restored.sample(key,f,request);check(!duplicate.job,"restart no acquisition");
        if(prior->value.replay.header.kind==111){check(release(duplicate.response),"durable terminal survives seal fault");continue;}
        check(!release(duplicate.response),"unresolved restart no release");
        drain.clean=false;check(!restored.recover(w.session,drain),"restart drain required");drain.clean=true;
        auto failure=restored.recover(w.session,drain);check(failure && release(*failure),"restart explicit drain seals failure without capture");
        auto old=reopened.replay(key,QUuid::createUuid());check(old.body==failure->body,"recovery terminal exact replay");
    }
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{pairing();sharedIndustrialLease();candidateAndFailures();restartAndFaults();QTextStream(stdout)<<checks<<" SAMPLE checks passed; software camera fixture, physical BLOCKED\n";return 0;}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
