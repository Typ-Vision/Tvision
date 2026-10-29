#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLockFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <cstdlib>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int assertions=0;
void check(bool b,const char* message) {++assertions;if(!b) throw std::runtime_error(message);}
template<class F> void rejects(F f) {bool rejected=false;try {f();} catch(const std::exception&) {rejected=true;}check(rejected,"Expected rejection");}
QUuid id() {return QUuid::createUuid();}
QByteArray read(const QString& path) {QFile f(path);check(f.open(QIODevice::ReadOnly),"Read fixture");return f.readAll();}
void write(const QString& path,const QByteArray& bytes) {QFile f(path);check(f.open(QIODevice::WriteOnly|QIODevice::Truncate),"Write fixture");check(f.write(bytes)==bytes.size(),"Write bytes");}
CatalogueIdentity station() {return {CatalogueCategory::Station,id(),{}, {}};}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        if(app.arguments().value(1)=="--crash") {
            auto path=app.arguments().at(2),boundary=app.arguments().at(3);
            JsonStationIdentityStore store(path,[&](const QString& point){if(point==boundary) std::_Exit(73);});
            StationIdentityRegistry registry(store);registry.bind(store.load().revision,station(),"crash candidate");return 2;
        }
        if(app.arguments().value(1)=="--race") {
            JsonStationIdentityStore store(app.arguments().at(2));StationIdentityRegistry registry(store);
            registry.bind(QUuid(app.arguments().at(3)),station(),"Concurrent writer");return 0;
        }
        QTemporaryDir root(app.applicationDirPath()+"/identity-XXXXXX");check(root.isValid(),"Temp root");
        JsonStationIdentityStore store(root.filePath("registry.json"));StationIdentityRegistry registry(store);
        rejects([&]{store.load();});auto c=registry.initialize(id());const auto initialBytes=read(root.filePath("registry.json"));
        rejects([&]{registry.initialize(id());});check(read(root.filePath("registry.json"))==initialBytes,"No automatic overwrite");
        auto s=station();auto first=registry.bind(c.revision,s,"Station 1");c=store.load();
        auto second=registry.bind(c.revision,station(),"Station 2");check(first.key!=second.key && first.key==1 && second.key==2,"Unique keys");
        rejects([&]{registry.snapshot(c.revision);});c=store.load();
        auto duplicate=registry.bind(c.revision,s,"Station 1");check(duplicate.key==first.key && store.load()==c,"Idempotent bind");
        registry.rename(c.revision,first.key,"Renamed");c=store.load();check(c.entries.first().key==first.key && c.entries.first().name=="Renamed","Rename preserves key");
        const auto recipe=id();auto r1=registry.bind(c.revision,{CatalogueCategory::RecipeRevision,recipe,id(),{}},"Recipe");c=store.load();
        auto r2=registry.bind(c.revision,{CatalogueCategory::RecipeRevision,recipe,id(),{}},"Recipe");check(r1.key!=r2.key,"Revision allocates new key");c=store.load();
        registry.retire(c.revision,first.key);c=store.load();rejects([&]{registry.bind(c.revision,s,"Renamed");});
        auto newer=registry.bind(c.revision,station(),"New station");check(newer.key>r2.key,"Retired key not reused");c=store.load();
        JsonStationIdentityStore reopened(root.filePath("registry.json"));check(reopened.load()==c,"Restart exact snapshot");
        const auto before=read(root.filePath("registry.json"));auto bad=c;bad.revision=id();bad.entries[1].key=first.key;
        rejects([&]{store.compareExchange(c.revision,bad);});bad=c;bad.revision=id();bad.entries[0].retired=false;
        rejects([&]{store.compareExchange(c.revision,bad);});bad=c;bad.revision=id();bad.entries.removeFirst();
        rejects([&]{store.compareExchange(c.revision,bad);});bad=c;bad.revision=id();bad.entries[1].identity.entity=id();
        rejects([&]{store.compareExchange(c.revision,bad);});bad=c;bad.revision=id();bad.nextKey=1;
        rejects([&]{store.compareExchange(c.revision,bad);});
        auto stale=c;registry.rename(c.revision,second.key,"Updated");stale.revision=id();rejects([&]{store.compareExchange(c.revision,stale);});c=store.load();
        QLockFile lock(root.filePath("registry.json.lock"));check(lock.tryLock(),"Acquire fixture lock");
        rejects([&]{registry.bind(c.revision,station(),"Locked");});lock.unlock();check(store.load()==c,"Lock failure unchanged");
        const auto peer=id(),epoch=id();registry.enrollEpoch(c.revision,peer,epoch);c=store.load();
        rejects([&]{registry.enrollEpoch(c.revision,peer,id());});registry.retireEpoch(c.revision,peer,epoch);c=store.load();
        rejects([&]{registry.enrollEpoch(c.revision,peer,epoch);});registry.enrollEpoch(c.revision,peer,id());c=store.load();
        check(reopened.load().epochs.first().retired && reopened.load().epochs.size()==2,"Retired epochs persist");
        bad=c;bad.revision=id();bad.epochs.removeFirst();rejects([&]{store.compareExchange(c.revision,bad);});
        for(std::uint32_t category=1;category<=13;++category) {
            registry.bind(c.revision,{CatalogueCategory(category),id(),category==1 || category==4?QUuid{}:id(),{}},"Category");c=store.load();
        }
        check(store.load().entries.size()==18,"All 13 frozen categories supported");
        auto raceRevision=c.revision;QProcess writer1,writer2;
        const QStringList raceArgs{"--race",root.filePath("registry.json"),raceRevision.toString()};
        writer1.start(app.applicationFilePath(),raceArgs);writer2.start(app.applicationFilePath(),raceArgs);
        check(writer1.waitForStarted() && writer2.waitForStarted(),"Concurrent writers started");
        check(writer1.waitForFinished(15000) && writer2.waitForFinished(15000),"Concurrent writers finished");
        check((writer1.exitCode()==0)+(writer2.exitCode()==0)==1,"Exactly one competing writer commits");
        check(store.load().entries.size()==c.entries.size()+1 && store.load().nextKey==c.nextKey+1,"No lost update or double allocation");c=store.load();
        const QUuid golden("00112233-4455-6677-8899-aabbccddeeff");
        check(exportIdentityUuid(golden).toHex()=="00112233445566778899aabbccddeeff","Opaque16 RFC order");
        check(importIdentityUuid(QByteArray::fromHex("00112233445566778899aabbccddeeff"))==golden,"Opaque16 import");
        rejects([&]{importIdentityUuid(QByteArray(15,'x'));});rejects([&]{importIdentityUuid(QByteArray(16,0));});
        rejects([&]{registry.bind(c.revision,{CatalogueCategory::Camera,id(),id(),{}},"Invalid revision");});
        rejects([&]{registry.bind(c.revision,{CatalogueCategory::TemplateRevision,id(),{},{}},"Missing revision");});
        rejects([&]{registry.bind(c.revision,station(),QString(65,'x'));});
        // Independently authored checked-in JSON schema/opaque identity fixture.
        const auto fixture=JsonStationIdentityStore::decode(read(app.arguments().at(1)));
        check(fixture.entries.size()==3 && fixture.entries[0].identity.entity==golden && fixture.entries[2].retired,"Schema fixture identities");
        check(JsonStationIdentityStore::decode(JsonStationIdentityStore::encode(fixture))==fixture,"Schema round trip");
        // Even a recomputed checksum cannot hide a structural key/identity collision.
        auto envelope=QJsonDocument::fromJson(JsonStationIdentityStore::encode(fixture)).object();auto p=envelope["payload"].toObject();
        auto entries=p["entries"].toArray();auto collision=entries[1].toObject();collision["key"]=1;entries[1]=collision;p["entries"]=entries;
        envelope["payload"]=p;envelope["sha256"]=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(p).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
        rejects([&]{JsonStationIdentityStore::decode(QJsonDocument(envelope).toJson());});
        auto limit=fixture;limit.nextKey=std::numeric_limits<std::uint32_t>::max();
        JsonStationIdentityStore limitStore(root.filePath("limit.json"));limitStore.create(limit);StationIdentityRegistry limited(limitStore);
        const auto last=limited.bind(limit.revision,station(),"Last key");check(last.key==std::numeric_limits<std::uint32_t>::max(),"Full unsigned key supported");
        const auto exhausted=limitStore.load();check(exhausted.nextKey==std::uint64_t(last.key)+1,"No wrap");
        rejects([&]{limited.bind(exhausted.revision,station(),"Overflow");});check(limitStore.load()==exhausted,"Exhaustion fail closed");
        // Actual process termination without unwinding at each atomic file boundary.
        for(const QString boundary:{"PREPARED","HALF_WRITTEN","WRITTEN","COMMITTED"}) {
            auto path=root.filePath(boundary+".json");JsonStationIdentityStore crashStore(path);crashStore.create(fixture);
            QProcess process;process.start(app.applicationFilePath(),{"--crash",path,boundary});
            check(process.waitForStarted() && process.waitForFinished(15000) && process.exitCode()==73,"Crash child reached boundary");
            auto recovered=crashStore.load();check(recovered.entries.size()==fixture.entries.size()+(boundary=="COMMITTED"?1:0),"Restart old or new coherent snapshot");
            check(boundary=="COMMITTED" || recovered==fixture,"Precommit byte state unchanged");
            StationIdentityRegistry next(crashStore);auto e=next.bind(recovered.revision,station(),"After restart");
            check(e.key==recovered.nextKey,"Stale process lock recovered without key collision");
        }
        for(const auto& bytes:QVector<QByteArray>{QByteArray("{"),before.left(before.size()/2),before+"garbage"}) {
            auto path=root.filePath("corrupt.json");write(path,bytes);JsonStationIdentityStore corrupt(path);
            rejects([&]{corrupt.load();});rejects([&]{corrupt.create(fixture);});check(read(path)==bytes,"Corrupt store not repaired");
        }
        auto corrupted=initialBytes;corrupted.replace("\"nextKey\":\"1\"","\"nextKey\":\"2\"");rejects([&]{JsonStationIdentityStore::decode(corrupted);});
        // Real filesystem failure: directory at the destination cannot be replaced.
        auto obstruction=root.filePath("directory");QDir().mkpath(obstruction);JsonStationIdentityStore unavailable(obstruction);
        rejects([&]{unavailable.create(fixture);});
        QTextStream(stdout)<<"station identity registry: "<<assertions<<" assertions passed; crash boundaries 4; skipped 0\n";return 0;
    } catch(const std::exception& e) {QTextStream(stderr)<<e.what()<<'\n';return 1;}
}
