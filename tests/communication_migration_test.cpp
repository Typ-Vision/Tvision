#include "application/CommunicationMigration.hpp"
#include "application/CommunicationStationComposition.hpp"
#include "application/ProjectService.hpp"
#include "application/CommunicationProfiles.hpp"
#include "application/DeliveryStore.hpp"
#include "application/StationDeliveryLedger.hpp"
#include "application/CommunicationListenerOwnership.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include <QTextStream>
#include <cstdlib>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* m){++checks;if(!b)throw std::runtime_error(m);}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,"Expected rejection");}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
QByteArray read(const QString& p){QFile f(p);check(f.open(QIODevice::ReadOnly),"read");return f.readAll();}
void write(const QString& p,const QByteArray& b){QFile f(p);check(f.open(QIODevice::WriteOnly|QIODevice::Truncate) && f.write(b)==b.size(),"write");}
CommunicationMigrationInput load(const QString& root){
    const auto o=QJsonDocument::fromJson(read(root+"/input.json")).object();
    CommunicationMigrationInput in;in.projectPath=root+"/project.json";in.profilesPath=root+"/profiles.json";in.registryPath=root+"/registry.json";in.binaryPath=root+"/legacy.exe";in.archiveDirectory=root+"/archive";
    in.station=QUuid(o["station"].toString());in.publication=PublicationTemplate::fromJson(o["template"].toObject());return in;
}
struct Fixture {
    QTemporaryDir root{QCoreApplication::applicationDirPath()+"/migration-XXXXXX"};
    CommunicationMigrationInput in;QString variable,producer;
    Fixture(VariableType type=VariableType::Boolean,int groups=1){
        check(root.isValid(),"temp");check(QDir().mkdir(root.filePath("archive")),"archive");check(QDir().mkdir(root.filePath("workspace")),"workspace");
        auto p=PublicationTemplate::create(QUuid::createUuid(),QUuid::createUuid(),{QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid()});p.flags[0].required=type==VariableType::Boolean;
        write(root.filePath("input.json"),QJsonDocument(QJsonObject{{"station",id()},{"template",p.toJson()}}).toJson());in=load(root.path());
        IdentityCatalogue catalogue;catalogue.installation=QUuid::createUuid();catalogue.revision=QUuid::createUuid();JsonStationIdentityStore(in.registryPath).create(catalogue);
        write(in.binaryPath,"legacy binary fixture v3\0exact bytes");
        EndpointProfile profile;profile.id=id();profile.enabled=true;profile.fieldMapping={{"accepted","legacy/address"}};CommunicationProfiles(in.profilesPath).save({{profile.id,profile}});
        ProjectService project;project.createProject("Migration","",in.projectPath,"");project.createRecipe("Part","",ApplicationType::DefectInspection);
        VariableDefinition v;v.name="Accepted";v.type=type;if(type==VariableType::SpatialTransform)v.unit=ScalarUnit::Millimetre;variable=project.addVariable(v);producer=project.addTool("set_variables");
        ValueEnvelope value;value.validity=ValueValidity::Valid;
        if(type==VariableType::Boolean){value.type=ScalarType::Boolean;value.value=false;}
        else if(type==VariableType::String){value.type=ScalarType::String;value.value=QString(65,'x');}
        else {value.type=ScalarType::Int32;value.value=qint32(0);}
        for(auto t:project.tools())if(t.id==producer){
            AssignmentValue constant=value;if(type==VariableType::SpatialTransform){
                SpatialAssignmentValue spatial;spatial.validity=ValueValidity::Valid;
                spatial.value=SpatialTransform{{"camera","","",id(),1},{"","robot","",id(),1},{1,2,3},{}};constant=spatial;
            }
            t.parameters=encodeAssignmentConfig({producer,1,{{variable,ConstantAssignmentSource{constant}}}});project.updateTool(t);
        }
        QVector<OutputGroup> output;for(int n=0;n<groups;++n)output.append({id(),profile.id,1,true,"vf.fields/1",producer,1,{{"accepted",variable}}});
        project.setOutputGroups(output);const auto publisher=project.addTool("publish_data");
        QStringList ids;for(const auto& g:output)ids.append(g.id);
        for(auto t:project.tools())if(t.id==publisher){t.parameters=encodePublishConfig({ids});t.enabled=true;project.updateTool(t);}project.save();
    }
    QString workspace() const{return root.filePath("workspace");}
};
void crash(const Fixture& f,const QString& workspace,const QString& package,const QString& phase,const QString& point){
    QProcess p;p.start(QCoreApplication::applicationFilePath(),{"--crash",f.root.path(),workspace,package,phase,point});
    check(p.waitForFinished(30000),"Crash child finished");
    if(p.exitCode()!=73)QTextStream(stderr)<<p.readAllStandardError();
    check(p.exitCode()==73,"Fault boundary reached");
}
bool reason(const CommunicationMigrationPlan& p,const QString& s){return QJsonDocument(p.report).toJson().contains(s.toUtf8());}
void basic(){
    Fixture f;const auto bytes=read(f.in.projectPath),profiles=read(f.in.profilesPath),registry=read(f.in.registryPath);
    auto plan=CommunicationMigrationPlanner(f.in).analyze();check(plan.supported(),"Bool mapping supported");check(plan.digest()==CommunicationMigrationPlanner(f.in).analyze().digest(),"Read-only plan deterministic");
    check(read(f.in.projectPath)==bytes && read(f.in.profilesPath)==profiles && read(f.in.registryPath)==registry,"No-op exact bytes");
    check(!QFileInfo::exists(f.workspace()+"/active.json"),"Analyze no activation");
    CommunicationMigrationTransaction tx(f.in,f.workspace());const auto package=id();auto manifest=tx.prepare(package,plan.digest());
    rejects([&]{tx.prepare(package,plan.digest());});rejects([&]{CommunicationMigrationTransaction other(f.in,f.workspace());});
    rejects([&]{ProductionStore(f.in.archiveDirectory).append("events",id(),{{"action","test"}});});
    QTcpServer listener;check(listener.listen(QHostAddress::LocalHost,0),"Listener preflight fixture");CommunicationListenerOwnership::observeLegacy(listener);
    rejects([&]{CommunicationMigrationTransaction blocked(f.in,f.root.path());});listener.close();
    rejects([&]{tx.activate(package,"bad",{});});tx.activate(package,manifest,{});check(tx.active()["mode"]=="v4","Explicit activation");
    const auto pointer=tx.pointerDigest();rejects([&]{tx.rollback({},tx.ledgerDigest());});rejects([&]{tx.rollback(pointer,"bad");});
    tx.rollback(pointer,tx.ledgerDigest());check(tx.active()["mode"]=="legacy","Rollback");check(tx.pointerDigest()!=pointer,"Rollback new revision");
    check(read(f.in.projectPath)==bytes && read(f.in.profilesPath)==profiles && read(f.in.registryPath)==registry,"Rollback preserves legacy bytes");
    auto staged=ProjectRepository{}.load(f.workspace()+"/"+package+"/project.json");auto a=recipePublicationAssignments(staged.recipes[0].definition);
    check(a && a->rows.size()==1 && a->rows[0].source.id==f.variable,"Real serializer assignment");
    check(staged.recipes[0].recipeVersion==ProjectRepository{}.load(f.in.projectPath).recipes[0].recipeVersion+1,"Recipe revision changed explicitly");
    const auto evidence=qEnvironmentVariable("VISIONFLOW_MIGRATION_EVIDENCE");if(!evidence.isEmpty()){
        QDir().mkpath(evidence);QFile::copy(f.workspace()+"/"+package+"/manifest.json",evidence+"/example-manifest.json");
        write(evidence+"/example-report.json",QJsonDocument(plan.report).toJson());
    }
}
void unsupported(){
    Fixture text(VariableType::String);auto p=CommunicationMigrationPlanner(text.in).analyze();check(!p.supported() && reason(p,"TEXT_EXCEEDS_64_BYTES"),"65-byte text reported");
    CommunicationMigrationTransaction tx(text.in,text.workspace());auto package=id();auto m=tx.prepare(package,p.digest());rejects([&]{tx.activate(package,m,{});});
    Fixture fan(VariableType::Boolean,2);check(reason(CommunicationMigrationPlanner(fan.in).analyze(),"MULTI_ENDPOINT_FANOUT"),"Fanout blocked");
    Fixture pose(VariableType::SpatialTransform,2);check(reason(CommunicationMigrationPlanner(pose.in).analyze(),"MULTIPLE_WHOLE_POSES"),"Multiple poses blocked");
    Fixture numeric(VariableType::Int32);numeric.in.publication.numbers[0].required=true;
    check(CommunicationMigrationPlanner(numeric.in).analyze().supported(),"Int32 zero preserves type");
    auto project=ProjectRepository{}.load(fan.in.projectPath);project.communicationProtocol="vf.socket/2";ProjectRepository{}.save(fan.in.projectPath,project);
    check(reason(CommunicationMigrationPlanner(fan.in).analyze(),"V2_SCALAR"),"V2 scalar explicit draft");
}
void staleAndPending(){
    Fixture f;auto p=CommunicationMigrationPlanner(f.in).analyze();QString package=id();QByteArray m;
    {CommunicationMigrationTransaction t(f.in,f.workspace());m=t.prepare(package,p.digest());}
    const auto original=read(f.in.projectPath);write(f.in.projectPath,original+"\n");
    {CommunicationMigrationTransaction t(f.in,f.workspace());rejects([&]{t.activate(package,m,{});});}write(f.in.projectPath,original);
    DeliveryRequest r{"cell","project","endpoint","peer","request","fingerprint"};DeliveryStore(f.in.archiveDirectory).request(r);
    p=CommunicationMigrationPlanner(f.in).analyze();check(!p.supported() && reason(p,"RECOVERY_REQUIRED"),"Legacy unsealed intent blocks migration");
    Fixture v2;ProductionStore store(v2.in.archiveDirectory);store.append("events",id(),{{"action","PLC_REQUEST"},{"sessionId","session"},{"requestId","request"}});
    check(!CommunicationMigrationPlanner(v2.in).analyze().supported(),"V2 pending event blocks");
    store.append("events",id(),{{"action","PLC_ACK"},{"sessionId","session"},{"requestId","request"}});
    check(CommunicationMigrationPlanner(v2.in).analyze().supported(),"V2 drained evidence accepted");
    Fixture v4;auto plan=CommunicationMigrationPlanner(v4.in).analyze();auto pkg=id();
    {CommunicationMigrationTransaction t(v4.in,v4.workspace());t.activate(pkg,t.prepare(pkg,plan.digest()),{});}
    auto c=JsonStationIdentityStore(v4.in.registryPath).load();V4RequestIdentity key{c.installation,QUuid::createUuid(),QUuid::createUuid(),v4.in.station,1};
    CommunicationV4Frame req{{10,1,1,QUuid::createUuid(),key.peer,key.epoch,1,1,QUuid::createUuid()},QByteArray(384,0)};
    auto accepted=req;accepted.header.kind=110;accepted.body=QByteArray(896,0);CommunicationV4Codec::putUInt32(accepted.body,100,1);CommunicationV4Codec::putUInt32(accepted.body,824,10);
    StationDeliveryLedger ledger(v4.in.archiveDirectory,c.installation);ledger.reserve({key,{req,accepted,CommunicationV4Codec::requestFingerprint(CommunicationV4Codec::encode(req,V4Direction::Request)),false},false});
    auto sealed=accepted;sealed.header.kind=111;sealed.body.replace(0,16,QUuid::createUuid().toRfc4122());sealed.body.replace(16,16,QUuid::createUuid().toRfc4122());
    CommunicationV4Codec::putUInt32(sealed.body,64,1);CommunicationV4Codec::putUInt32(sealed.body,72,1);CommunicationV4Codec::putUInt32(sealed.body,100,3);
    auto raw=CommunicationV4Codec::encode(accepted,V4Direction::Response);CommunicationV4Codec::putUInt32(raw,8,111);raw.replace(128,896,sealed.body);sealed.body.replace(32,32,CommunicationV4Codec::terminalDigest(raw));ledger.seal(key,sealed);
    {CommunicationMigrationTransaction t(v4.in,v4.workspace());rejects([&]{t.rollback(t.pointerDigest(),t.ledgerDigest());});}
    // A sealed terminal survives restart and is drained by its exact ACK.
    StationDeliveryLedger restarted(v4.in.archiveDirectory,c.installation);auto terminal=restarted.recovery().first().value.replay;auto ack=terminal;ack.header.kind=11;ack.body=QByteArray(384,0);ack.body.replace(0,16,terminal.body.first(16));ack.body.replace(16,32,terminal.body.sliced(32,32));restarted.acknowledge(key,ack);
    const auto records=ProductionStore(v4.in.archiveDirectory).all("station_delivery");
    {CommunicationMigrationTransaction t(v4.in,v4.workspace());t.rollback(t.pointerDigest(),t.ledgerDigest());check(t.active()["mode"]=="legacy","Drained v4 rollback");}
    check(ProductionStore(v4.in.archiveDirectory).all("station_delivery")==records,"Rollback keeps all v4 tombstones/dedup");
}
void faults(){
    Fixture f;const auto original=read(f.in.projectPath);const auto plan=CommunicationMigrationPlanner(f.in).analyze();
    QStringList preparePoints{"package-created","manifest:half","manifest:written","manifest-committed"};
    for(const auto& file:plan.files.keys())preparePoints<<file+":half"<<file+":written"<<file;
    int n=0;
    for(const auto& point:preparePoints){auto w=f.root.filePath("prepare-"+QString::number(n++));QDir().mkdir(w);auto package=id();crash(f,w,package,"prepare",point);
        CommunicationMigrationTransaction t(f.in,w);check(t.active()["mode"]=="legacy" && t.pointerDigest().isEmpty(),"Staging crash keeps legacy active");check(read(f.in.projectPath)==original,"Crash preserves original");}
    for(const auto& phase:QStringList{"activate","rollback"})for(const auto& point:QStringList{phase=="activate"?"validated":"rollback-validated","before-pointer","pointer:half","pointer:written","after-pointer"}){
        auto w=f.root.filePath("swap-"+QString::number(n++));QDir().mkdir(w);auto package=id();
        {CommunicationMigrationTransaction t(f.in,w);auto m=t.prepare(package,plan.digest());if(phase=="rollback")t.activate(package,m,{});}
        crash(f,w,package,phase,point);CommunicationMigrationTransaction t(f.in,w);auto mode=t.active()["mode"].toString();
        const auto expected=(point=="after-pointer")?(phase=="activate"?"v4":"legacy"):(phase=="activate"?"legacy":"v4");
        check(mode==expected,"Crash atomic old/new consistent selection");check(read(f.in.projectPath)==original,"Swap never rewrites original");
    }
    auto w=f.root.filePath("tamper");QDir().mkdir(w);CommunicationMigrationTransaction t(f.in,w);auto pkg=id();auto m=t.prepare(pkg,plan.digest());
    write(w+"/"+pkg+"/project.json",original);rejects([&]{t.activate(pkg,m,{});});
    write(w+"/active.json","{");rejects([&]{t.active();});
}
void startupAuthority(){
    Fixture f;const auto authority=f.root.filePath("communication-identities-v4.json");check(QFile::copy(f.in.registryPath,authority),"Machine authority fixture");f.in.registryPath=authority;
    const auto work=f.root.filePath("migration");check(QDir().mkdir(work),"Machine migration directory");const auto package=id();
    {CommunicationMigrationTransaction t(f.in,work);const auto plan=CommunicationMigrationPlanner(f.in).analyze();const auto digest=t.prepare(package,plan.digest());t.activate(package,digest,{});}
    const auto selected=CommunicationStationComposition::restoreSelection(f.root.path());check(selected["mode"]=="v4","Normal startup consumes reviewed pointer");
    JsonStationIdentityStore store(authority);const auto seed=store.load();check(seed==JsonStationIdentityStore::decode(read(selected["registry"].toString())),"Startup adopts exact migrated catalogue with CAS");
    StationIdentityRegistry registry(store);const auto camera=registry.bind(store.load().revision,{CatalogueCategory::Camera,QUuid::createUuid(),{},{}},"New camera");registry.retire(store.load().revision,camera.key);const auto peer=QUuid::createUuid(),epoch=QUuid::createUuid();registry.enrollEpoch(store.load().revision,peer,epoch);registry.retireEpoch(store.load().revision,peer,epoch);const auto live=read(authority);
    CommunicationStationComposition::restoreSelection(f.root.path());check(read(authority)==live,"Restart preserves new keys/retired/epoch history");
    {CommunicationMigrationTransaction t(f.in,work);t.rollback(t.pointerDigest(),t.ledgerDigest());}
    check(CommunicationStationComposition::restoreSelection(f.root.path())["mode"]=="legacy" && read(authority)==live,"Rollback never restores stale catalogue seed");
    write(work+"/active.json","{");rejects([&]{CommunicationStationComposition::restoreSelection(f.root.path());});check(read(authority)==live,"Corrupt pointer fails closed without rewriting live authority");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    if(app.arguments().value(1)=="--crash"){
        auto a=app.arguments();auto in=load(a[2]);CommunicationMigrationTransaction t(in,a[3],[&](const QString& p){if(p==a[6])std::_Exit(73);});
        if(a[5]=="prepare")t.prepare(a[4],CommunicationMigrationPlanner(in).analyze().digest());
        else if(a[5]=="activate"){
            const auto bytes=read(a[3]+"/"+a[4]+"/manifest.json");auto digest=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex();t.activate(a[4],digest,{});
        }else t.rollback(t.pointerDigest(),t.ledgerDigest());return 2;
    }
    basic();unsupported();staleAndPending();faults();startupAuthority();QTextStream(stdout)<<"communication migration checks="<<checks<<"; crash boundaries=38\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}}
