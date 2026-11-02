#include "application/CalibrationSampleCoordinator.hpp"
#include "application/ProductionStore.hpp"
#include <QImage>
#include "persistence/RobotCalibrationSessionStore.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QSaveFile>
#include <QTcpServer>
#include <QJsonArray>
#include "persistence/StationIdentityStore.hpp"

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
} // namespace
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2,"Fixture directory");QDir root(QString::fromLocal8Bit(argv[1]));QDir().mkpath(root.path());
    JsonStationIdentityStore ids(root.filePath("identities.json"));StationIdentityRegistry registry(ids);
    registry.initialize(kInstallation);registry.enrollEpoch(ids.load().revision,kPeer,kEpoch);
    const auto station=registry.bind(ids.load().revision,{CatalogueCategory::Station,kStation,{},{ }},"Calibration fixture");
    Provider provider;Drain drain;auto d=dependency("socket",31);provider.set(kStation,d);
    FileRobotCalibrationSessionStore store(root.filePath("calibration-sessions.json"));
    RobotCalibrationSessionService calibrations(store,provider,drain);
    StationDeliveryLedger ledger(root.filePath("production"),kInstallation);
    CalibrationSampleCoordinator capture(calibrations,provider,ledger,true);
    auto request=captureRequest(d.cameraId);int captures=0,dispatches=0,lateCallbacks=0;
    CommunicationPeerSessionService sessions(ids);sessions.setDeliveryLedger(&ledger);
    sessions.setCalibrationHandler([&](const V4RequestIdentity& key,const CommunicationV4Frame& f){
        if(f.header.kind==20){
            // Test-only mode selection is local to this executable. No normal main
            // dependency provider or softwareFixture authorization is installed.
            provider.entries[0].snapshot.mode=C::uint32(f.body,16)==1?HandEyeMode::EyeToHand:HandEyeMode::EyeInHand;
            return calibrations.begin(key,f).response;
        }
        if(f.header.kind==21){
            auto a=capture.sample(key,f,request);
            if(a.job){++dispatches;const auto job=*a.job;
                QTimer::singleShot(100,&app,[&,job]{
                    auto result=CalibrationSampleCoordinator::run(job,backendFor(request,captures));
                    if(!capture.complete(job,result))++lateCallbacks;
                });
            }
            return a.response;
        }
        if(f.header.kind==25){
            if(const auto prior=ledger.inspect(key);prior && prior->value.replay.header.kind==111)return ledger.replay(key,f.header.session);
            CommunicationV4Frame accepted{f.header,QByteArray(896,0)};accepted.header.kind=110;C::putUInt32(accepted.body,100,1);C::putUInt32(accepted.body,824,25);
            if(!ledger.inspect(key)){
                const auto reason=ledger.reserve({key,{f,accepted,C::requestFingerprint(C::encode(f,V4Direction::Request)),false},true});
                if(reason){accepted.header.kind=190;C::putUInt32(accepted.body,76,reason);return accepted;}
            }
            drain.clean=!QFile::exists(root.filePath("fail-drain"));
            auto result=calibrations.abort(key,f).response;
            if(result.header.kind==111){
                result.body.replace(0,16,QUuid::createUuid().toRfc4122());
                auto raw=C::encode(accepted,V4Direction::Response);C::putUInt32(raw,8,111);raw.replace(128,896,result.body);
                result.body.replace(32,32,C::terminalDigest(raw));ledger.seal(key,result);
            }
            return result;
        }
        CommunicationV4Frame reject{f.header,QByteArray(896,0)};reject.header.kind=190;C::putUInt32(reject.body,76,17);C::putUInt32(reject.body,824,f.header.kind);return reject;
    });
    QTcpServer listener;check(listener.listen(QHostAddress::LocalHost,0),"Fixture listen");
    QObject::connect(&listener,&QTcpServer::newConnection,&app,[&]{while(listener.hasPendingConnections()){auto socket=listener.nextPendingConnection();QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);attachCommunicationPeer(*socket,sessions);}});
    auto save=[&](QString name,QJsonObject object){QSaveFile f(root.filePath(name));check(f.open(QIODevice::WriteOnly),"report open");f.write(QJsonDocument(object).toJson());check(f.commit(),"report commit");};
    save("ready.json",{{"port",int(listener.serverPort())},{"peer",kPeer.toString(QUuid::WithoutBraces)},{"epoch",kEpoch.toString(QUuid::WithoutBraces)},{"station",int(station.key)},{"camera",int(d.cameraKey)},{"cameraRevision",d.cameraRevision.toString(QUuid::WithoutBraces)},{"intrinsic",int(d.intrinsicRevisionKey)},{"plate",int(d.plateRevisionKey)},{"adapter",int(d.robotAdapterRevisionKey)},{"profile",int(d.acquisitionProfileRevisionKey)},{"target",d.pose.targetFrame.toString(QUuid::WithoutBraces)},{"targetRevision",d.pose.targetFrameRevision.toString(QUuid::WithoutBraces)},{"source",d.pose.sourceFrame.toString(QUuid::WithoutBraces)},{"sourceRevision",d.pose.sourceFrameRevision.toString(QUuid::WithoutBraces)},{"physical","BLOCKED"}});
    QJsonObject lastCounters;QTimer timer;timer.setInterval(20);QObject::connect(&timer,&QTimer::timeout,&app,[&]{
        const QJsonObject counters{{"captures",captures},{"dispatches",dispatches},{"lateCallbacks",lateCallbacks},{"leases",calibrations.activeCount()}};
        if(counters!=lastCounters){save("counters.json",counters);lastCounters=counters;}
        if(QFile::exists(root.filePath("quit"))){save("server-result.json",{{"captures",captures},{"dispatches",dispatches},{"physical","BLOCKED"}});app.quit();}
    });timer.start();QTimer::singleShot(90000,&app,[&]{app.exit(5);});return app.exec();
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
