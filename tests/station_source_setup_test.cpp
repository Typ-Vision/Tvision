#include "application/StationSourceSetup.hpp"
#include "application/MachineSetup.hpp"
#include "application/CommunicationStationComposition.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QLockFile>
#include <QJsonDocument>
#include <QJsonObject>
using namespace visionflow;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 int checks=0;auto check=[&](bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);};
 auto rejects=[&](auto f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"Expected rejection");};
 auto write=[](QString p,QByteArray b){QFile f(p);if(!f.open(QIODevice::WriteOnly)||f.write(b)!=b.size())throw std::runtime_error("write fixture");};
 auto bytes=[](QString p){QFile f(p);if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("read fixture");return f.readAll();};
 QTemporaryDir d;check(d.isValid(),"temporary directory");MachineSetup(d.path()).start();
 const auto path=d.filePath("communication-identities-v4.json"),image=d.filePath("image.pgm"),other=d.filePath("other.pgm");
 write(image,QByteArray("P5\n2 2\n255\n")+QByteArray(4,'\x42'));write(other,bytes(image));
 JsonStationIdentityStore ids(path);StationSourceSetup setup(ids);const auto legacy=ids.load();const auto legacyBytes=bytes(path);
 check(JsonStationIdentityStore::encode(JsonStationIdentityStore::decode(legacyBytes))==legacyBytes,"Legacy exact roundtrip");
 const auto station=QUuid::createUuid(),camera=QUuid::createUuid();
 rejects([&]{setup.save(legacy.revision,station,"Station 1",camera,"Camera 1",d.filePath("missing.png"));});check(bytes(path)==legacyBytes,"Missing source leaves catalogue unchanged");
 write(d.filePath("bad.png"),"not an image");rejects([&]{setup.save(legacy.revision,station,"Station 1",camera,"Camera 1",d.filePath("bad.png"));});check(bytes(path)==legacyBytes,"Bad image leaves catalogue unchanged");
 rejects([&]{setup.save(legacy.revision,station,"",camera,"Camera 1",image);});check(bytes(path)==legacyBytes,"Invalid name leaves catalogue unchanged");
 const auto source=setup.save(legacy.revision,station,"Station 1",camera,"Camera 1",image);const auto first=ids.load();
 check(first.stationSources==QVector<StationFileSource>{source} && first.entries.size()==4,"Atomic enrollment and complete source");
 check(first.installation==legacy.installation && first.entries[0]==legacy.entries[0] && first.entries[1]==legacy.entries[1],"Templates and installation preserved");
 check(source.station==station && source.camera==camera && source.path==image && !source.revision.isNull(),"Independent machine source");
 check(JsonStationIdentityStore(path).load()==first,"Restart persistence");
 check(QJsonDocument::fromJson(bytes(path)).object()["payload"].toObject()["schemaVersion"]==2,"Explicit schema v2");
 const auto firstBytes=bytes(path);setup.save(first.revision,station,"Station 1",camera,"Camera 1",image);check(bytes(path)==firstBytes,"Identical save exact no-op");
 rejects([&]{setup.save(legacy.revision,station,"Stale",camera,"Camera 1",other);});check(bytes(path)==firstBytes,"Stale CAS preserves winner");
 auto renamed=setup.save(first.revision,station,"Renamed",camera,"Camera 1",image);check(renamed==source && ids.load().entries.size()==4,"Rename stable camera/source revision/keys");
 const auto renamedCat=ids.load();auto changed=setup.save(renamedCat.revision,station,"Renamed",camera,"Camera 1",other);
 check(changed.camera==camera && changed.revision!=source.revision && changed.path==other,"File update new source revision");
 const auto current=ids.load();const auto currentBytes=bytes(path);
 rejects([&]{setup.save(current.revision,station,"Renamed",QUuid::createUuid(),"Other",other);});
 rejects([&]{setup.save(current.revision,QUuid::createUuid(),"Station 2",camera,"Camera 1",other);});check(bytes(path)==currentBytes,"No camera replacement or shared station camera");
 QLockFile lock(path+".lock");check(lock.tryLock(),"Concurrent writer lock");rejects([&]{setup.save(current.revision,station,"Locked",camera,"Camera 1",image);});lock.unlock();check(bytes(path)==currentBytes,"Locked write unchanged");
 for(const auto& boundary:{QString("PREPARED"),QString("HALF_WRITTEN"),QString("WRITTEN")}){
  JsonStationIdentityStore failing(path,[&](const QString& at){if(at==boundary)throw std::runtime_error("power loss");});
  rejects([&]{StationSourceSetup(failing).save(current.revision,station,"Interrupted",camera,"Camera 1",image);});check(bytes(path)==currentBytes,"Interrupted update atomically retains prior record");
 }
 // Power loss during first enrollment must not leave an identity-only station.
 QTemporaryDir fresh;MachineSetup(fresh.path()).start();const auto freshPath=fresh.filePath("communication-identities-v4.json");const auto freshBytes=bytes(freshPath);
 JsonStationIdentityStore failing(freshPath,[](const QString& at){if(at=="HALF_WRITTEN")throw std::runtime_error("power loss");});
 rejects([&]{StationSourceSetup(failing).save(failing.load().revision,QUuid::createUuid(),"First",QUuid::createUuid(),"Camera",image);});check(bytes(freshPath)==freshBytes,"Interrupted first enrollment has no orphan identities");
 // Legacy station/activation records stay separate. No mapping is guessed.
 const auto legacyStation=StationIdentityRegistry(ids).bind(ids.load().revision,{CatalogueCategory::Station,QUuid::createUuid(),{},{}},"Legacy station");
 CommunicationServerProfiles profiles(d.filePath("server.json"));CommunicationPeerSessionService sessions(ids);CommunicationServerLifecycle server(profiles,d.filePath("shutdown"));CommunicationWorkspace w(server,profiles,ids);
 {CommunicationStationComposition composition(d.path(),{},ids,sessions);composition.attach(server,w);
  const auto rows=w.stations();check(rows.size()==2 && !w.remoteEnabled(),"Normal composition restores all stations without permissions");
  for(const auto& row:rows){check(row.runtime.mode==StationMode::Stopped && !row.automatic && !row.canRun && !row.canAuto,"Stopped without recipe on reopen");if(row.station.id==station)check(row.source==changed && row.cameraOwners.contains(other),"UI source restored");if(row.station.id==legacyStation.identity.entity)check(!row.source,"Legacy mapping explicitly unresolved");}
  QFile::remove(other);const auto missing=w.stations();for(const auto& row:missing)if(row.station.id==station)check(row.source==changed && row.unavailable.contains(QString::fromUtf8("Kaynak okunamıyor")),"Lost file retained with repair action reason");write(other,bytes(image));
  w.saveStationSource(ids.load().revision,station,"Renamed again",camera,"Camera 1",image);check(w.listener().contains(QString::fromUtf8("yeniden başlatın")),"Explicit composition restart notice");rejects([&]{w.command(station,{},"run");});rejects([&]{w.setRemoteEnabled(true);});
  server.beginShutdown(0);check(server.pollShutdown(),"Shutdown drained");
 }
 const auto valid=bytes(path);write(path,"corrupt");rejects([&]{ids.load();});check(bytes(path)=="corrupt","Corruption preserved");write(path,valid);
 QTextStream(stdout)<<"PASS "<<checks<<" station source checks\n";return 0;
 }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
