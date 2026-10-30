#include "application/StationDeliveryScheduler.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QElapsedTimer>
#include <QTextStream>
using namespace visionflow;
namespace {
using C=CommunicationV4Codec;int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
QUuid id(){return QUuid::createUuid();}
template<class F>void until(F ready){QElapsedTimer t;t.start();while(!ready() && t.elapsed()<5000)QCoreApplication::processEvents(QEventLoop::AllEvents,5);check(ready(),"Socket deadline");}
struct Fixture {
 QTemporaryDir root{QCoreApplication::applicationDirPath()+"/scheduler-XXXXXX"};
 QUuid installation=id(),peer=id(),epoch=id();
 StationDeliveryLedger ledger{root.path(),installation};
 QVector<V4RequestIdentity> keys;QVector<CommunicationV4Frame> terminals;
 void seal(quint32 station,QUuid stationId={},quint32 kind=10){
  V4RequestIdentity key{installation,peer,epoch,stationId.isNull()?id():stationId,quint64(keys.size()+1)};
  CommunicationV4Frame request{{kind,station,key.request,id(),peer,epoch,31,41,id()},QByteArray(384,0)};
  auto accepted=request;accepted.header.kind=110;accepted.body=QByteArray(896,0);C::putUInt32(accepted.body,824,kind);
  check(ledger.reserve({key,{request,accepted,C::requestFingerprint(C::encode(request,V4Direction::Request)),false},kind==6})==0,"Reserve once");
  auto terminal=accepted;terminal.header.kind=111;terminal.body.replace(0,16,id().toRfc4122());C::putUInt32(terminal.body,100,3);
  auto raw=C::encode(accepted,V4Direction::Response);C::putUInt32(raw,8,111);raw.replace(128,896,terminal.body);terminal.body.replace(32,32,C::terminalDigest(raw));
  ledger.seal(key,terminal);keys.append(key);terminals.append(terminal);
 }
 void ack(int n){auto a=terminals[n];a.header.kind=11;a.body=QByteArray(384,0);a.body.replace(0,16,terminals[n].body.first(16));a.body.replace(16,32,terminals[n].body.sliced(32,32));ledger.acknowledge(keys[n],a);}
};
QByteArray drain(StationDeliveryScheduler& s,qint64 now,int chunk=37){QByteArray bytes;while(true){const auto view=s.pendingBytes();if(view.isEmpty())break;const auto n=qMin<qsizetype>(chunk,view.size());bytes.append(view.first(n));s.consumeWritten(n,now);}return bytes;}
void fairness(){
 StationDeliveryScheduler s;auto session=id();auto f=CommunicationV4Frame{{102,1,1,session,id(),id(),0,0,{}},QByteArray(896,0)};
 for(int n=0;n<8;++n){f.header.requestId=n+1;check(s.enqueue(f),"Queue S1 burst");}
 f.header.stationKey=2;check(s.enqueue(f),"Queue S2");f.header.stationKey=3;check(s.enqueue(f),"Queue S3");
 const auto prefix=s.pendingBytes().first(13).toByteArray();s.consumeWritten(13,0);
 check(s.pendingBytes(65536).isEmpty() && !s.canRead(65536),"Highwater pauses writes and admission");
 auto bytes=prefix+drain(s,0,1);QVector<quint32> stations;
 for(int o=0;o<bytes.size();o+=1024)stations.append(C::decode(QByteArrayView(bytes).sliced(o,1024),V4Direction::Response).header.stationKey);
 check(stations[0]==1 && stations[1]==2 && stations[2]==3 && stations[3]==1,"Round robin at frame boundaries; no partial byte interleave");
 for(int n=0;n<64;++n)check(s.enqueue(f),"Bounded queue accepts capacity");
 check(!s.enqueue(f) && s.queuedFrames()==64 && s.queuedBytes()==65536 && !s.canRead(0),"Full is explicit, never evicts");
 check(s.pendingBytes(65530).size()==6,"Socket remaining capacity enforced");
 drain(s,0);check(s.canRead(0),"Read resumes after drain");
}
void retry(){
 Fixture f;for(int n=1;n<=3;++n)f.seal(n);f.seal(1,f.keys[0].station,6);
 StationDeliveryScheduler s(&f.ledger);const auto session=id();auto authorized=[](const auto&){return true;};
 s.tick(0,session,authorized);check(s.queuedFrames()==4,"One normal plus independent control reserve per station");
 auto first=drain(s,0);check(first.size()==4096,"All stations progress before any ACK");
 f.ack(1);f.ack(2);f.ack(3);
 s.tick(1999,session,authorized);check(s.queuedFrames()==0,"No early retry");
 for(int attempt=1;attempt<=3;++attempt){s.tick(attempt*2000,session,authorized);auto bytes=drain(s,attempt*2000);check(bytes.size()==1024 && C::decode(bytes,V4Direction::Response).body==f.terminals[0].body,"Retry only S1 exact sealed body");}
 s.tick(8000,session,authorized);check(s.timedOut(f.keys[0]) && s.queuedFrames()==0 && f.ledger.pending(f.keys[0].station),"2 seconds and three retries leave pending alarm");
 s.tick(90000,session,authorized);check(s.timedOut(f.keys[0]) && drain(s,90000).isEmpty(),"Timeout is sticky, no unbounded retry");
 const auto fresh=id();s.tick(90001,fresh,authorized);auto replay=C::decode(drain(s,90001),V4Direction::Response);
 check(replay.header.session==fresh && replay.body==f.terminals[0].body,"Reconnect session-only envelope");
 check(f.ledger.recovery().size()==4,"Retry/reconnect never creates intents or acquisition");
 f.ack(0);s.tick(90002,fresh,authorized);check(!s.timedOut(f.keys[0]) && s.queuedFrames()==0,"Exact ACK clears pending only");
}
void saturation(){
 Fixture f;for(int n=1;n<=32;++n){f.seal(n);f.seal(n,f.keys.last().station,6);}
 StationDeliveryScheduler s(&f.ledger);auto session=id();auto auth=[](const auto&){return true;};
 s.tick(0,session,auth,65536);check(s.queuedFrames()==0,"Slow socket leaves disk pending");
 s.tick(0,session,auth,1024);check(s.queuedFrames()==63 && s.queuedBytes()+1024==65536,"RAM plus socket bounded to 64 responses");
 s.tick(100000,session,auth,1024);check(s.queuedFrames()==63 && !s.timedOut(f.keys[0]),"Unwritten backlog spends no retry budget");
 auto partial=s.pendingBytes().first(19).toByteArray();s.consumeWritten(19,100000);
 bool fenced=false;try{s.bindSession(id());}catch(...){fenced=true;}check(fenced,"HELLO cannot splice a partial response");
 s.reset();check(s.queuedFrames()==0 && f.ledger.recovery().size()==64,"Disconnect discards only transport offsets");
 session=id();s.tick(100001,session,auth);check(drain(s,100001).size()==64*1024,"All 64 pending records replay after partial disconnect");
}
void sockets(){
 Fixture f;JsonStationIdentityStore store(f.root.filePath("registry.json"));StationIdentityRegistry registry(store);
 auto catalogue=registry.initialize(f.installation);registry.enrollEpoch(catalogue.revision,f.peer,f.epoch);
 for(int n=0;n<3;++n){catalogue=store.load();auto station=id();const auto entry=registry.bind(catalogue.revision,{CatalogueCategory::Station,station,{},{}},QString::number(n));f.seal(entry.key,station);}
 CommunicationPeerSessionService service(store);service.setDeliveryLedger(&f.ledger);
 QTcpServer server;check(server.listen(QHostAddress::LocalHost,0),"Listen");
 QObject::connect(&server,&QTcpServer::newConnection,&server,[&]{while(server.hasPendingConnections()){auto socket=server.nextPendingConnection();QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);attachCommunicationPeer(*socket,service);}});
 CommunicationV4Frame hello{{1,0,1,{},f.peer,f.epoch,0,0,{}},QByteArray(384,0)};hello.body.replace(0,16,CommunicationPeerSessionService::contractRevision().toRfc4122());
 auto connect=[&](QTcpSocket& client){client.connectToHost(QHostAddress::LocalHost,server.serverPort());until([&]{return client.state()==QAbstractSocket::ConnectedState;});client.write(C::encode(hello,V4Direction::Request));until([&]{return client.bytesAvailable()>=4096;});auto h=C::decode(client.read(1024),V4Direction::Response);check(h.header.kind==101,"HELLO precedes replay");for(int n=0;n<3;++n){auto t=C::decode(client.read(1024),V4Direction::Response);check(t.header.kind==111 && t.header.session==h.header.session,"Socket auto replays all stations without new capture");}return h.header.session;};
 QTcpSocket client;client.setReadBufferSize(4096);auto old=connect(client);
 // A slow reader and a hot STATUS stream must resume without closing or losing correlation.
 QByteArray burst;CommunicationV4Frame status{{2,0,100,old,f.peer,f.epoch,0,0,{}},QByteArray(384,0)};
 for(int n=0;n<160;++n){status.header.requestId=100+n;burst+=C::encode(status,V4Direction::Request);}client.write(burst);
 QElapsedTimer slow;slow.start();while(slow.elapsed()<150)QCoreApplication::processEvents(QEventLoop::AllEvents,5);
 for(int n=0;n<160;++n){until([&]{return client.bytesAvailable()>=1024;});auto r=C::decode(client.read(1024),V4Direction::Response);check(r.header.kind==102 && r.header.requestId==quint64(100+n),"Slow reader frame stream resumes in order");}
 client.disconnectFromHost();until([&]{return service.session().isNull();});
 QTcpSocket reconnected;const auto fresh=connect(reconnected);check(fresh!=old,"Fresh transport session");
 auto stale=f.ledger.inspect(f.keys[0])->value.request;stale.header.session=old;reconnected.write(C::encode(stale,V4Direction::Request));until([&]{return reconnected.bytesAvailable()>=1024;});check(C::uint32(C::decode(reconnected.read(1024),V4Direction::Response).body,76)==5,"Stale session rejected on real socket");
 catalogue=store.load();registry.retireEpoch(catalogue.revision,f.peer,f.epoch);stale.header.session=fresh;reconnected.write(C::encode(stale,V4Direction::Request));until([&]{return reconnected.bytesAvailable()>=1024;});check(C::uint32(C::decode(reconnected.read(1024),V4Direction::Response).body,76)==19,"Retired epoch trigger rejected");
 reconnected.disconnectFromHost();until([&]{return service.session().isNull();});
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{fairness();retry();saturation();sockets();QTextStream(stdout)<<"PASS "<<checks<<" scheduler assertions\n";return 0;}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
