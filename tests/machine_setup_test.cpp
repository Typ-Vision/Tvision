#include "application/MachineSetup.hpp"
#include "application/CommunicationWorkspace.hpp"
#include "application/CommunicationStationComposition.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "persistence/PublicationTemplateStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QLockFile>
#include <QTextStream>
#include <QJsonDocument>
using namespace visionflow;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    int checks=0;auto check=[&](bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);};
    auto rejects=[&](auto f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Expected safe rejection");};
    QTemporaryDir root;check(root.isValid(),"Temporary directory");
    auto bytes=[](QString p){QFile f(p);if(!f.open(QIODevice::ReadOnly))return QByteArray{};return f.readAll();};
    auto write=[](QString p,QByteArray b){QFile f(p);if(!f.open(QIODevice::WriteOnly) || f.write(b)!=b.size())throw std::runtime_error("Fixture write");};
    const auto profilePath=root.filePath("communication-server-v4.json");CommunicationServerProfiles profiles(profilePath);CommunicationServerProfile p;p.address="127.0.0.1";p.port=53963;p.enabled=true;profiles.save(p);const auto profileBytes=bytes(profilePath);
    MachineSetup setup(root.path());check(setup.status().contains("yapılmadı"),"Missing differs from corruption");
    const auto c=setup.start();check(c.entries.size()==2 && !c.installation.isNull(),"First enrollment");
    const auto saved=bytes(root.filePath("communication-identities-v4.json"));
    const auto templates=PublicationTemplateStore(root.filePath("publication-templates-v4")).load();check(templates.size()==2,"Two manifests");
    check(MachineSetup::defaults(c.installation)[1].numbers[0].required,"Measurement required");
    check(MachineSetup::defaults(c.installation)[1].numbers[0].unit==ScalarUnit::Unitless,"No false mm");
    setup.start();check(bytes(root.filePath("communication-identities-v4.json"))==saved,"Repeat exact catalogue no-op");
    check(bytes(profilePath)==profileBytes && profiles.load()->enabled,"Listener untouched");
    MachineSetup reopened(root.path());check(reopened.start()==c && reopened.status().contains("Hazır"),"Restart same IDs/revision/keys");
    QLockFile lock(root.filePath("machine-setup-v4.lock"));check(lock.tryLock(),"Hold concurrent installer lock");rejects([&]{setup.start();});lock.unlock();check(setup.start()==c,"Retry after concurrency");
    // Preserve real catalogue history including a custom template, station and retired peer epoch.
    JsonStationIdentityStore store(root.filePath("communication-identities-v4.json"));StationIdentityRegistry registry(store);
    const auto station=registry.bind(store.load().revision,{CatalogueCategory::Station,QUuid::createUuid(),{},{}},"Existing station");
    auto custom=MachineSetup::defaults(QUuid::createUuid()).first();registry.bind(store.load().revision,{CatalogueCategory::TemplateRevision,custom.id,custom.revision,{}},"Custom");
    write(root.filePath("publication-templates-v4/custom.json"),QJsonDocument(custom.toJson()).toJson());
    QDir().mkpath(root.filePath("production"));write(root.filePath("production/history"),"history sentinel");write(root.filePath("recipes"),"recipe sentinel");
    const auto prior=store.load();setup.start();check(store.load()==prior && station.key>2,"Existing station and custom key unchanged");check(bytes(root.filePath("production/history"))=="history sentinel" && bytes(root.filePath("recipes"))=="recipe sentinel","Historical/recipe bytes unchanged");
    for(const auto& t:templates)check(bytes(t.path)==QJsonDocument(t.value.toJson()).toJson(),"Immutable manifest bytes");
    // Every write boundary, including the second manifest/key, can fail and resume.
    for(int stop=1;stop<=9;++stop){QTemporaryDir partial;int n=0;MachineSetup s(partial.path());
        try{s.start([&](const QString&){if(++n==stop)throw std::runtime_error("Power loss");});}catch(const std::exception&){}
        JsonStationIdentityStore ids(partial.filePath("communication-identities-v4.json"));const auto installation=ids.load().installation;
        const auto done=s.start();check(done.installation==installation && done.entries.size()==2,"Interrupted install resumes without duplicates");check(s.start()==done,"Resumed install is no-op");
    }
    const auto bad=root.filePath("publication-templates-v4/broken.json");write(bad,"broken");rejects([&]{setup.start();});check(store.load()==prior && bytes(bad)=="broken","Corrupt manifest retained");QFile::remove(bad);
    write(root.filePath("communication-identities-v4.json"),"broken");rejects([&]{setup.start();});check(bytes(root.filePath("communication-identities-v4.json"))=="broken" && setup.status().contains("Bozuk"),"Corrupt catalogue never replaced");
    QTemporaryDir lost;QDir().mkpath(lost.filePath("production"));rejects([&]{MachineSetup(lost.path()).start();});check(!QFile::exists(lost.filePath("communication-identities-v4.json")),"Lost identity with history is not fresh install");
    // Ordinary workspace action and normal composition after restart share the records.
    QTemporaryDir normal;JsonStationIdentityStore ids(normal.filePath("communication-identities-v4.json"));CommunicationServerProfiles ps(normal.filePath("communication-server-v4.json"));CommunicationServerLifecycle server(ps,normal.filePath("shutdown"));CommunicationWorkspace w(server,ps,ids);w.setMachinePaths(normal.filePath("migration"),normal.filePath("communication-identities-v4.json"));w.startSetup();check(w.stations().isEmpty() && !w.remoteEnabled(),"No stations or permissions synthesized");CommunicationPeerSessionService sessions(ids);CommunicationStationComposition composition(normal.path(),{},ids,sessions);composition.attach(server,w);check(!w.remoteEnabled() && w.stations().isEmpty(),"Restart remains stopped");server.beginShutdown(0);check(server.pollShutdown(),"Clean shutdown");
    QTextStream(stdout)<<"PASS "<<checks<<" machine setup checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
