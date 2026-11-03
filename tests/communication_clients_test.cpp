#include "application/CommunicationWorkspace.hpp"
#include "application/CommunicationClientProfiles.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> QString rejection(F f){QString text;try{f();}catch(const std::exception& e){text=QString::fromUtf8(e.what());}check(!text.isEmpty(),"Expected fail-closed rejection");return text;}
QByteArray read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"Read fixture");return f.readAll();}
void write(const QString& path,const QByteArray& bytes){QFile f(path);check(f.open(QIODevice::WriteOnly|QIODevice::Truncate),"Write fixture");check(f.write(bytes)==bytes.size(),"Write bytes");}
QByteArray hex(const QUuid& id){return id.toRfc4122().toHex();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir root(app.applicationDirPath()+"/vf05-clients-XXXXXX");check(root.isValid(),"Fixture root");
    JsonStationIdentityStore identities(root.filePath("identities.json"));StationIdentityRegistry registry(identities);
    const auto catalogue=registry.initialize(QUuid::createUuid());
    CommunicationServerProfiles profiles(root.filePath("server.json"));CommunicationServerProfile listener;listener.enabled=true;listener.address="127.0.0.1";
    {QTcpServer probe;check(probe.listen(QHostAddress::LocalHost,0),"Fixture listener port");listener.port=probe.serverPort();}
    profiles.save(listener);
    StationDeliveryLedger ledger(root.filePath("production"),identities.load().installation);
    CommunicationPeerSessionService sessions(identities);sessions.setDeliveryLedger(&ledger);
    CommunicationServerLifecycle server(profiles,root.filePath("shutdown.json"),&sessions);check(server.state()==CommunicationServerState::Listening,"Real listener");
    CommunicationWorkspace workspace(server,profiles,identities);workspace.setLedger(&ledger);
    bool remote=false;workspace.setRemoteControl([&]{return remote;},[&](bool value){remote=value;});
    check(workspace.clients().isEmpty(),"No registered client on a fresh machine");
    check(rejection([&]{workspace.exportClientProfile("Simülatör istemcisi");}).contains("etkin istemci"),"Profile export requires a registered client");
    check(rejection([&]{workspace.rolloverClient();}).contains("Etkin istemci yok"),"Rollover requires a registered client");
    remote=true;check(rejection([&]{workspace.enrollClient();}).contains("Remote Enable"),"Remote Enable blocks registration");remote=false;
    const auto first=workspace.enrollClient();
    check(first.active && !first.peer.isNull() && !first.epoch.isNull(),"Registered controlling client");
    auto cat=identities.load();validateIdentityCatalogue(cat);
    check(cat.epochs.size()==1 && cat.epochs.first().peer==first.peer && cat.epochs.first().epoch==first.epoch && !cat.epochs.first().retired,"Single active epoch persisted");
    check(rejection([&]{workspace.enrollClient();}).contains("İkinci etkin kimlik"),"Second active client rejected with the required action");
    const auto profileBytes=workspace.exportClientProfile("Simülatör istemcisi");
    const auto profile=readCommunicationClientProfile(profileBytes);
    check(profile.installation==cat.installation && profile.peer==first.peer && profile.epoch==first.epoch,"Exported profile carries live catalogue identity");
    check(profile.address==listener.address && profile.port==listener.port && profile.contractRevision==CommunicationPeerSessionService::contractRevision(),"Exported profile carries listener/contract");
    workspace.checkClientProfile(profileBytes);
    const auto object=QJsonDocument::fromJson(profileBytes).object();
    auto foreign=object;foreign["installation"]=QString::fromLatin1(hex(QUuid::createUuid()));
    check(rejection([&]{workspace.checkClientProfile(QJsonDocument(foreign).toJson(QJsonDocument::Compact));}).contains("başka makine"),"Foreign installation profile rejected");
    auto stale=object;stale["epoch"]=QString::fromLatin1(hex(QUuid::createUuid()));
    check(rejection([&]{workspace.checkClientProfile(QJsonDocument(stale).toJson(QJsonDocument::Compact));}).contains("kayıtlı değil"),"Unknown epoch profile rejected");
    auto schema=object;schema["schema"]="visionflow.communication-client-profile/2";
    check(rejection([&]{workspace.checkClientProfile(QJsonDocument(schema).toJson(QJsonDocument::Compact));}).contains("şeması"),"Unknown profile schema rejected");
    const auto profilePath=root.filePath("client.json");write(profilePath,profileBytes);
    const auto runSimulator=[&](const QString& path,const QString& logPath){
        QProcess process;process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(QString::fromUtf8(VISIONFLOW_PYTHON),{QString::fromUtf8(VISIONFLOW_SIMULATOR),"--profile",path,"--log",logPath});
        if(!process.waitForStarted(5000))return false;
        QElapsedTimer timer;timer.start();
        while(process.state()!=QProcess::NotRunning && timer.elapsed()<30000){QCoreApplication::processEvents(QEventLoop::AllEvents,20);QThread::msleep(5);}
        if(process.state()!=QProcess::NotRunning){process.kill();process.waitForFinished(2000);return false;}
        return process.exitStatus()==QProcess::NormalExit && process.exitCode()==0;
    };
    const auto helloLog=root.filePath("hello.jsonl");
    check(runSimulator(profilePath,helloLog),"Independent Python client HELLO from product-exported profile");
    check(read(helloLog).contains("\"event\": \"completed\""),"Simulator completed HELLO exchange");
    auto badContract=object;badContract["contractRevision"]=QString::fromLatin1(hex(QUuid("00434634-0002-4000-8000-202610290002")));
    const auto badContractPath=root.filePath("bad-contract.json");write(badContractPath,QJsonDocument(badContract).toJson(QJsonDocument::Compact));
    check(!runSimulator(badContractPath,root.filePath("bad-contract.jsonl")),"Simulator rejects a foreign contract revision");
    auto missing=object;missing.remove("peer");
    const auto missingPath=root.filePath("missing-field.json");write(missingPath,QJsonDocument(missing).toJson(QJsonDocument::Compact));
    check(!runSimulator(missingPath,root.filePath("missing-field.jsonl")),"Simulator rejects an incomplete profile");
    const auto second=workspace.rolloverClient();
    check(second.peer==first.peer && second.epoch!=first.epoch,"Rollover keeps peer identity and rotates epoch");
    cat=identities.load();validateIdentityCatalogue(cat);
    check(cat.epochs.size()==2 && cat.epochs[0].retired && cat.epochs[0].epoch==first.epoch && !cat.epochs[1].retired && cat.epochs[1].epoch==second.epoch,"Rollover retains history and one active epoch");
    check(rejection([&]{workspace.checkClientProfile(profileBytes);}).contains("emekli"),"Retired profile rejected locally");
    const auto nextBytes=workspace.exportClientProfile("Simülatör istemcisi 2");
    const auto nextPath=root.filePath("client-2.json");write(nextPath,nextBytes);
    const auto nextLog=root.filePath("hello-2.jsonl");
    check(runSimulator(nextPath,nextLog),"New epoch profile registers with HELLO");
    check(!runSimulator(profilePath,root.filePath("hello-old.jsonl")),"Old epoch profile can no longer register");
    JsonStationIdentityStore reopened(root.filePath("identities.json"));
    check(reopened.load()==identities.load() && reopened.load().epochs.size()==2,"Epoch history survives restart");
    bool listenerClosed=false;server.beginShutdown(1000);QElapsedTimer shutdown;shutdown.start();while(!(listenerClosed=server.pollShutdown()) && shutdown.elapsed()<5000)QCoreApplication::processEvents();
    check(listenerClosed,"Bounded listener shutdown");
    // A pending delivery must fence epoch rollover even when the machine is drained.
    QTemporaryDir pendingRoot(app.applicationDirPath()+"/vf05-pending-XXXXXX");check(pendingRoot.isValid(),"Pending fixture root");
    JsonStationIdentityStore pendingIdentities(pendingRoot.filePath("identities.json"));StationIdentityRegistry pendingRegistry(pendingIdentities);
    pendingRegistry.initialize(QUuid::createUuid());
    CommunicationServerProfiles pendingProfiles(pendingRoot.filePath("server.json"));pendingProfiles.save(CommunicationServerProfile{});
    StationDeliveryLedger pendingLedger(pendingRoot.filePath("production"),pendingIdentities.load().installation);
    CommunicationServerLifecycle pendingServer(pendingProfiles,pendingRoot.filePath("shutdown.json"));
    CommunicationWorkspace pendingWorkspace(pendingServer,pendingProfiles,pendingIdentities);pendingWorkspace.setLedger(&pendingLedger);
    const auto active=pendingWorkspace.enrollClient();
    const auto station=QUuid::createUuid();CommunicationV4Frame request;
    request.header={6,1,1,QUuid::createUuid(),active.peer,active.epoch,0,0,QUuid{}};request.body=QByteArray(384,0);
    auto accepted=CommunicationV4Frame{request.header,QByteArray(896,0)};accepted.header.kind=110;
    CommunicationV4Codec::putUInt32(accepted.body,824,6);
    const V4RequestIdentity key{pendingIdentities.load().installation,active.peer,active.epoch,station,1};
    check(pendingLedger.reserve({key,{request,accepted,CommunicationV4Codec::requestFingerprint(CommunicationV4Codec::encode(request,V4Direction::Request)),false},true})==0,"Durable pending delivery");
    check(rejection([&]{pendingWorkspace.rolloverClient();}).contains("PENDING ACK"),"Pending delivery fences epoch rollover");
    const auto unchanged=pendingIdentities.load();validateIdentityCatalogue(unchanged);
    check(unchanged.epochs.size()==1 && !unchanged.epochs.first().retired && unchanged.epochs.first().epoch==active.epoch,"Rejected rollover leaves identity unchanged");
    // Even with no active identity (externally retired epoch), a pending delivery
    // fences new enrollment; the operator must ACK/reconcile first.
    pendingRegistry.retireEpoch(pendingIdentities.load().revision,active.peer,active.epoch);
    check(rejection([&]{pendingWorkspace.enrollClient();}).contains("PENDING ACK"),"Pending delivery fences new client enrollment");
    const auto retired=pendingIdentities.load();validateIdentityCatalogue(retired);
    check(retired.epochs.size()==1 && retired.epochs.first().retired,"Rejected enrollment leaves identity unchanged");
    check(rejection([&]{pendingWorkspace.exportClientProfile("Simülatör istemcisi");}).contains("etkin istemci"),"Profile export requires exactly one active client");
    pendingServer.beginShutdown(0);pendingServer.pollShutdown();
    QTextStream(stdout)<<"communication clients: "<<checks<<" checks passed; python HELLO 3 runs; skipped 0\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
