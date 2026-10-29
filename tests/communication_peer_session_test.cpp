#include "application/CommunicationPeerSessionService.hpp"
#include "application/CommunicationServerLifecycle.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTextStream>
#include <QFile>
#include <QHostAddress>
#include <limits>
#include <future>
using namespace visionflow;
namespace {
using C=CommunicationV4Codec;int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F>void until(F ready){QElapsedTimer t;t.start();while(!ready() && t.elapsed()<4000)QCoreApplication::processEvents(QEventLoop::AllEvents,5);check(ready(),"Async timeout");}
void putUuid(QByteArray& b,int o,const QUuid& id){b.replace(o,16,id.toRfc4122());}
quint32 reason(const CommunicationV4Frame& f){return C::uint32(f.body,f.header.kind==103?788:76);}
CommunicationV4Frame request(quint32 kind,const QUuid& peer,const QUuid& epoch,const QUuid& session={},quint64 id=9007199254740993ULL){CommunicationV4Frame f;f.header.kind=kind;f.header.requestId=id;f.header.peer=peer;f.header.epoch=epoch;f.header.session=session;f.body=QByteArray(384,0);return f;}
CommunicationV4Frame hello(const QUuid& peer,const QUuid& epoch,QUuid contract=CommunicationPeerSessionService::contractRevision()){auto f=request(1,peer,epoch);putUuid(f.body,0,contract);return f;}
CommunicationV4Frame exchange(QTcpSocket& socket,const CommunicationV4Frame& f,bool split=false){const auto bytes=C::encode(f,V4Direction::Request);if(split){socket.write(bytes.first(127));socket.flush();QCoreApplication::processEvents();socket.write(bytes.sliced(127));}else socket.write(bytes);socket.flush();until([&]{return socket.bytesAvailable()>=1024;});return C::decode(socket.read(1024),V4Direction::Response);}
QByteArray read(const QString& path){QFile file(path);check(file.open(QIODevice::ReadOnly),"Read fixture");return file.readAll();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir root(app.applicationDirPath()+"/peer-XXXXXX");check(root.isValid(),"Fixture root");
    const auto path=root.filePath("identities.json");JsonStationIdentityStore store(path);StationIdentityRegistry registry(store);
    const auto installation=QUuid::createUuid(),peer=QUuid("ffffffff-ffff-4fff-8fff-ffffffffffff"),epoch=QUuid::createUuid(),station=QUuid::createUuid();
    auto catalogue=registry.initialize(installation);registry.enrollEpoch(catalogue.revision,peer,epoch);catalogue=store.load();
    const auto stationEntry=registry.bind(catalogue.revision,{CatalogueCategory::Station,station,{},{}},"Station");catalogue=store.load();
    const auto entry2=registry.bind(catalogue.revision,{CatalogueCategory::Station,QUuid::createUuid(),{},{}},"Station2");catalogue=store.load();
    auto metadata=[](const IdentityCatalogue&,const CatalogueEntry&){QByteArray m(256,0);C::putUInt32(m,0,1);C::putUInt32(m,4,256);m.replace(8,32,QByteArray(32,'c'));m.replace(40,32,QByteArray(32,'d'));return m;};
    CommunicationPeerSessionService sessions(store,metadata);const auto connection=QUuid::createUuid(),other=QUuid::createUuid();
    check(sessions.open(connection) && !sessions.open(other),"One controlling connection");
    auto r=sessions.handle(connection,hello(peer,epoch,{}));check(reason(r)==0 && !r.header.session.isNull(),"Contract0 discovery");const auto discovery=r.header.session;
    auto trigger=request(10,peer,epoch,discovery);trigger.header.stationKey=stationEntry.key;
    check(sessions.authorize(connection,trigger)==24 && reason(sessions.handle(connection,trigger))==24,"Discovery cannot mutate");
    check(reason(sessions.handle(other,hello(peer,epoch)))==18,"Second peer busy");
    r=sessions.handle(connection,hello(peer,epoch));const auto exact=r.header.session;check(reason(r)==0 && exact!=discovery,"Exact HELLO replaces discovery session");
    check(sessions.authorize(connection,trigger)==5,"Previous session stale");trigger.header.session=exact;
    check(sessions.authorize(connection,trigger)==0 && reason(sessions.handle(connection,trigger))==20,"Authority does not dispatch unimplemented station command");
    const auto key=sessions.requestIdentity(connection,trigger);check(key==V4RequestIdentity{installation,peer,epoch,station,9007199254740993ULL},"Opaque UUID and >2^53 request namespace exact");
    const auto fingerprint=C::requestFingerprint(C::encode(trigger,V4Direction::Request));
    sessions.close(other);check(sessions.session()==exact,"Foreign close cannot release owner");sessions.close(connection);check(sessions.open(other),"Reconnect slot released");
    r=sessions.handle(other,hello(peer,epoch));check(reason(r)==0 && r.header.session!=exact,"Reconnect new session");trigger.header.session=r.header.session;
    check(sessions.requestIdentity(other,trigger)==key && C::requestFingerprint(C::encode(trigger,V4Direction::Request))==fingerprint,"Reconnect same key and fingerprint");
    trigger.header.requestId=std::numeric_limits<quint64>::max();check(sessions.requestIdentity(other,trigger).request==std::numeric_limits<quint64>::max(),"uint64 max exact");trigger.header.requestId=key.request;
    auto status=request(2,peer,epoch,trigger.header.session);r=sessions.handle(other,status);check(r.header.kind==102 && reason(r)==0 && C::uint32(r.body,756)==0,"Global status no fabricated ready/result");C::encode(r,V4Direction::Response);
    auto get=request(3,peer,epoch,trigger.header.session);C::putUInt32(get.body,0,1);r=sessions.handle(other,get);check(r.header.kind==103 && C::uint32(r.body,20)==stationEntry.key && C::uint32(r.body,784)==entry2.key,"Read-only catalogue first and next");C::encode(r,V4Direction::Response);
    C::putUInt32(get.body,4,entry2.key);putUuid(get.body,8,catalogue.revision);r=sessions.handle(other,get);check(reason(r)==0 && C::uint32(r.body,784)==0,"Exact catalogue continuation");
    putUuid(get.body,8,QUuid::createUuid());check(reason(sessions.handle(other,get))==4,"Stale catalogue fence");putUuid(get.body,8,{});C::putUInt32(get.body,4,12345);r=sessions.handle(other,get);check(reason(r)==10 && C::uint32(r.body,20)==12345,"Unknown key echoes query");C::encode(r,V4Direction::Response);
    auto spoof=trigger;spoof.header.peer=QUuid::createUuid();check(sessions.authorize(other,spoof)==5,"Peer spoof fenced");spoof=trigger;spoof.header.epoch=QUuid::createUuid();check(sessions.authorize(other,spoof)==5,"Epoch spoof fenced");
    r=sessions.handle(other,hello(peer,epoch,QUuid::createUuid()));check(reason(r)==24 && sessions.session().isNull() && sessions.authorize(other,trigger)==5,"Wrong contract HELLO destroys old authority");
    check(reason(sessions.handle(other,hello(QUuid::createUuid(),epoch)))==24,"Unknown peer never enrolled");
    sessions.handle(other,hello(peer,epoch));registry.retireEpoch(catalogue.revision,peer,epoch);catalogue=store.load();
    trigger.header.session=sessions.session();check(sessions.authorize(other,trigger)==19 && reason(sessions.handle(other,hello(peer,epoch)))==19,"Retired epoch blocks live and reconnect");
    const auto newEpoch=QUuid::createUuid();registry.enrollEpoch(catalogue.revision,peer,newEpoch);catalogue=store.load();sessions.close(other);
    check(sessions.open(other),"Open after retirement");r=sessions.handle(other,hello(peer,newEpoch));trigger.header.epoch=newEpoch;trigger.header.session=r.header.session;
    const auto foreignPeer=QUuid::createUuid(),foreignEpoch=QUuid::createUuid();registry.enrollEpoch(catalogue.revision,foreignPeer,foreignEpoch);catalogue=store.load();
    check(sessions.authorize(other,trigger)==24 && reason(sessions.handle(other,hello(foreignPeer,foreignEpoch)))==24,"Multiple local active peers are ambiguous, never first-caller enrollment");
    registry.retireEpoch(catalogue.revision,foreignPeer,foreignEpoch);catalogue=store.load();sessions.close(other);
    // Real listener composition: parser, HELLO, busy rejection, STATUS/CATALOG,
    // reconnect and persistent registry byte preservation, without Runtime dispatch.
    const auto baseline=read(path);CommunicationServerProfiles profiles(root.filePath("server.json"));CommunicationServerProfile profile;QTcpServer reservation;check(reservation.listen(QHostAddress::LocalHost,0),"Port fixture");profile.port=reservation.serverPort();reservation.close();profile.enabled=true;profiles.save(profile);
    {
        CommunicationServerLifecycle server(profiles,root.filePath("checkpoint.json"),&sessions);
        QTcpSocket client;client.connectToHost(QHostAddress::LocalHost,server.port());until([&]{return server.peer()!=nullptr && client.state()==QAbstractSocket::ConnectedState;});
        r=exchange(client,hello(peer,newEpoch,{}),true);check(reason(r)==0,"Localhost fragmented discovery");
        auto networkTrigger=request(10,peer,newEpoch,r.header.session);networkTrigger.header.stationKey=stationEntry.key;check(reason(exchange(client,networkTrigger))==24,"Discovery trigger rejected over socket");
        r=exchange(client,hello(peer,newEpoch));const auto firstSession=r.header.session;check(reason(r)==0,"Localhost exact HELLO");
        QTcpSocket extra;extra.connectToHost(QHostAddress::LocalHost,server.port());until([&]{return extra.state()==QAbstractSocket::ConnectedState;});check(reason(exchange(extra,hello(QUuid::createUuid(),QUuid::createUuid())))==18,"Localhost second peer PEER_BUSY");until([&]{return extra.state()==QAbstractSocket::UnconnectedState;});check(sessions.session()==firstSession,"Busy peer did not steal authority");
        status=request(2,peer,newEpoch,firstSession);get=request(3,peer,newEpoch,firstSession);C::putUInt32(get.body,0,1);
        // Coalesced reads preserve correlation and response order.
        status.header.requestId=std::numeric_limits<quint64>::max();client.write(C::encode(status,V4Direction::Request)+C::encode(get,V4Direction::Request));client.flush();until([&]{return client.bytesAvailable()>=2048;});r=C::decode(client.read(1024),V4Direction::Response);check(r.header.kind==102 && r.header.requestId==status.header.requestId,"Coalesced status uint64 max");r=C::decode(client.read(1024),V4Direction::Response);check(r.header.kind==103 && reason(r)==0,"Coalesced catalogue");
        networkTrigger.header.session=firstSession;check(reason(exchange(client,networkTrigger))==20,"Exact trigger has no dispatcher yet");
        // >4 requests must be continued without requiring another client write.
        QByteArray burst;for(int n=0;n<9;++n){status.header.requestId=quint64(n+1);burst+=C::encode(status,V4Direction::Request);}client.write(burst);client.flush();until([&]{return client.bytesAvailable()>=9*1024;});
        for(int n=0;n<9;++n){r=C::decode(client.read(1024),V4Direction::Response);check(r.header.requestId==quint64(n+1) && reason(r)==0,"Bounded batches continue in order");}
        client.disconnectFromHost();until([&]{return !server.peer() && sessions.session().isNull();});
        QTcpSocket reconnect;reconnect.connectToHost(QHostAddress::LocalHost,server.port());until([&]{return reconnect.state()==QAbstractSocket::ConnectedState;});r=exchange(reconnect,hello(peer,newEpoch));check(reason(r)==0 && r.header.session!=firstSession,"Localhost reconnect fresh session");
        check(reason(exchange(reconnect,networkTrigger))==5,"Old socket session stale");networkTrigger.header.session=r.header.session;check(reason(exchange(reconnect,networkTrigger))==20,"New session passes only session gate");
        reconnect.write(C::encode(status,V4Direction::Request).first(127));reconnect.flush();until([&]{return reconnect.state()==QAbstractSocket::UnconnectedState;});check(sessions.session().isNull(),"Idle partial deadline closes and releases ownership");
        server.beginShutdown(0);check(server.pollShutdown(),"Shutdown");
    }
    check(read(path)==baseline,"Handshake never rewrites registry or epoch history");
    {
        CommunicationPeerSessionService restart(store);const auto id=QUuid::createUuid();restart.open(id);r=restart.handle(id,hello(peer,newEpoch));check(reason(r)==0 && r.header.session!=exact,"Process service restart fresh session and persisted namespace");
        auto get2=request(3,peer,newEpoch,r.header.session);C::putUInt32(get2.body,0,1);check(reason(restart.handle(id,get2))==20,"Missing immutable metadata cannot fabricate catalogue");
        check(std::async(std::launch::async,[&]{try{restart.session();return false;}catch(...){return true;}}).get(),"Owner thread enforced");
        QFile corrupt(path);check(corrupt.open(QIODevice::WriteOnly|QIODevice::Truncate),"Corrupt fixture");corrupt.write("bad");corrupt.close();check(reason(restart.handle(id,hello(peer,newEpoch)))==8,"Corrupt store fail closed");
    }
    JsonStationIdentityStore missing(root.filePath("missing.json"));CommunicationPeerSessionService unconfigured(missing);const auto id=QUuid::createUuid();unconfigured.open(id);check(reason(unconfigured.handle(id,hello(peer,newEpoch)))==8 && !QFile::exists(root.filePath("missing.json")),"Missing store never implicitly enrolled");
    QTextStream(stdout)<<"PASS "<<checks<<" peer session assertions\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
