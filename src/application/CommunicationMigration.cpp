#include "CommunicationMigration.hpp"
#include "CommunicationProfiles.hpp"
#include "CommunicationListenerOwnership.hpp"
#include "DeliveryStore.hpp"
#include "StationDeliveryLedger.hpp"
#include "ProductionStore.hpp"
#include "persistence/ProjectRepository.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "domain/PublicationConfig.hpp"
#include "domain/ToolDefinition.hpp"
#include "domain/ToolRegistry.hpp"
#include "domain/IndustrialSourceReference.hpp"
#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
#include <limits>
#include <algorithm>

namespace visionflow {
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QByteArray json(const QJsonObject& o){return QJsonDocument(o).toJson(QJsonDocument::Compact);}
QByteArray hash(const QByteArray& b){return QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex();}
QString uid(const QUuid& u){return u.toString(QUuid::WithoutBraces);}
// Paths are canonicalized, not merely string-prefix checked. Symlink ancestors
// and missing inputs are rejected, including redirected archive directories.
QString path(const QString& p,bool directory=false){
    QFileInfo f(p);need(f.isAbsolute() && f.exists() && (directory?f.isDir():f.isFile()),"Migration input missing/absolute path required");
    for(auto a=f.absoluteFilePath();;a=QFileInfo(a).absolutePath()){
        need(!QFileInfo(a).isSymLink(),"Migration symlink refused");
        if(QFileInfo(a).absolutePath()==a)break;
    }
    return f.canonicalFilePath();
}
QByteArray read(const QString& p,qint64 limit=20*1024*1024){
    QFile f(path(p));need(f.open(QIODevice::ReadOnly) && f.size()<=limit,"Migration input too large/unreadable");
    auto b=f.readAll();need(f.error()==QFileDevice::NoError,"Migration read failed");return b;
}
void write(const QString& p,const QByteArray& b,const std::function<void(const QString&)>& fault={}){
    QSaveFile f(p);f.setDirectWriteFallback(false);
    need(f.open(QIODevice::WriteOnly),"Migration atomic open failed");
    const auto half=b.size()/2;
    need(f.write(b.first(half))==half,"Migration first write failed");if(fault)fault("half");
    need(f.write(b.sliced(half))==b.size()-half,"Migration final write failed");if(fault)fault("written");
    need(f.commit(),"Migration atomic commit failed");
}
QJsonObject object(const QByteArray& b){
    QJsonParseError e;auto d=QJsonDocument::fromJson(b,&e);
    need(e.error==QJsonParseError::NoError && d.isObject(),"Migration JSON invalid");return d.object();
}
QJsonObject inventory(const QString& directory){
    QJsonObject out;QDir root(path(directory,true));qint64 total=0;int count=0;
    // All persistent archive evidence, not lock/temp files. Verify supported
    // archive namespaces via their real readers below as well as raw SHA256.
    QDirIterator i(root.path(),QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while(i.hasNext()){
        i.next();need(!i.fileInfo().isSymLink(),"Archive symlink refused");
        if(i.fileInfo().isDir() || !i.fileName().endsWith(".json"))continue;
        total+=i.fileInfo().size();need(++count<=10000 && total<=512LL*1024*1024,"Migration archive limit");
        out[root.relativeFilePath(i.filePath())]=QString::fromLatin1(hash(read(i.filePath())));
    }return out;
}
QJsonArray pending(const CommunicationMigrationInput& in,const IdentityCatalogue& catalogue){
    QJsonArray a;
    for(const auto& r:DeliveryStore(in.archiveDirectory).recovery())if(r.status.state!="ACK_COMMITTED")
        a.append(QJsonObject{{"protocol","v3"},{"workcell",r.request.workcell},{"project",r.request.project},{"endpoint",r.request.endpoint},{"peer",r.request.peer},{"request",r.request.requestId},{"state",r.status.state}});
    for(const auto& r:StationDeliveryLedger(in.archiveDirectory,catalogue.installation).recovery())if(!r.value.acknowledged)
        a.append(QJsonObject{{"protocol","v4"},{"station",uid(r.key.station)},{"peer",uid(r.key.peer)},{"epoch",uid(r.key.epoch)},{"request",QString::number(r.key.request)},{"state",r.state}});
    QSet<QByteArray> requests,acks;
    for(const auto& e:ProductionStore(in.archiveDirectory).all("events")){
        const auto kind=e["action"].toString();
        if(kind!="PLC_REQUEST" && kind!="PLC_RESULT" && kind!="PLC_ACK")continue;
        auto session=e["sessionId"].toString(),request=e["requestId"].toString();
        need(!session.isEmpty() && !request.isEmpty(),"Invalid legacy PLC evidence");
        auto key=json({{"session",session},{"request",request}});
        if(kind=="PLC_ACK")acks.insert(key);else requests.insert(key);
    }
    auto ordered=requests.values();std::sort(ordered.begin(),ordered.end());
    for(const auto& k:ordered)if(!acks.contains(k)){auto o=object(k);o["protocol"]="v2";o["state"]="RECONCILIATION_REQUIRED";a.append(o);}
    return a;
}
QUuid derived(const QByteArray& b){return QUuid::fromRfc4122(QCryptographicHash::hash(b,QCryptographicHash::Sha256).left(16));}
CatalogueEntry bind(IdentityCatalogue& c,const CatalogueIdentity& id,const QString& name){
    for(const auto& e:c.entries)if(e.identity==id){need(!e.retired,"Migration identity retired");return e;}
    need(c.nextKey<=std::numeric_limits<quint32>::max(),"Catalogue key exhaustion");
    CatalogueEntry e{quint32(c.nextKey++),id,name,false};c.entries.append(e);return e;
}
QJsonObject description(const QString& p,const QByteArray& b){return {{"path",path(p)},{"sha256",QString::fromLatin1(hash(b))},{"bytes",double(b.size())}};}
void safeName(const QString& s){need(!QUuid(s).isNull() && uid(QUuid(s))==s,"Package must be canonical UUID");}
}
bool CommunicationMigrationPlan::supported() const{return report["unsupported"].toArray().isEmpty() && report["pending"].toArray().isEmpty();}
QByteArray CommunicationMigrationPlan::digest() const{return hash(json(report));}
CommunicationMigrationPlan CommunicationMigrationPlanner::analyze() const {
    const auto& in=input_;CommunicationServerProfiles::validate(in.server);
    need(!in.server.enabled && !in.station.isNull(),"Migration requires disabled server and explicit station");in.publication.validate();
    auto projectBytes=read(in.projectPath),profileBytes=read(in.profilesPath),registryBytes=read(in.registryPath),binaryBytes=read(in.binaryPath,256*1024*1024);
    auto project=ProjectRepository{}.load(in.projectPath);const auto profiles=CommunicationProfiles(in.profilesPath).load();
    const auto oldCatalogue=JsonStationIdentityStore::decode(registryBytes);auto catalogue=oldCatalogue;
    QJsonArray unsupported,mappings;auto issue=[&](const QString& where,const QString& reason){unsupported.append(QJsonObject{{"location",where},{"reason",reason}});};
    if(project.communicationProtocol=="vf.socket/2" || project.communicationProtocol=="v2")issue(project.id,"V2_SCALAR_CONVERSION_REQUIRES_EXPLICIT_DRAFT");
    const auto station=bind(catalogue,{CatalogueCategory::Station,in.station,{},{}},"Migrated station");
    const auto templ=bind(catalogue,{CatalogueCategory::TemplateRevision,in.publication.id,in.publication.revision,{}},"Publication template");
    for(auto& recipe:project.recipes){
        const auto groups=recipeOutputGroups(recipe.definition);auto tools=editorTools(recipe.definition);
        for(const auto& t:tools)if(t.enabled && ToolRegistry::get(t.kind).acquisitionSource){
            const auto camera=t.kind=="industrial_camera"?QUuid(IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject()).camera.value):QUuid(t.id);
            need(!camera.isNull(),"Saved acquisition source identity required");
            bind(catalogue,{CatalogueCategory::Camera,camera,{},{}},t.name);
        }
        validatePublicationBindings(recipe.definition);
        if(groups.size()>1)issue(recipe.id,"MULTI_ENDPOINT_FANOUT");
        if(recipePublicationAssignments(recipe.definition))issue(recipe.id,"ALREADY_HAS_V4_ASSIGNMENTS");
        PublicationAssignments assignments;assignments.templateId=in.publication.id;assignments.templateRevision=in.publication.revision;
        QVector<PublicationSourceDefinition> definitions;
        QMap<QString,VariableDefinition> vars;for(const auto& v:recipeVariables(recipe.definition))vars[v.id]=v;
        int numbers=0,flags=0,poses=0;
        for(const auto& g:groups){
            int publishers=0;
            for(const auto& t:tools)if(t.kind=="publish_data" && t.enabled && decodePublishConfig(t.parameters).groups.contains(g.id))++publishers;
            if(publishers!=1)issue(g.id,"INACTIVE_PUBLICATION_REQUIRES_EXPLICIT_DRAFT");
            if(!g.required)issue(g.id,"OPTIONAL_DELIVERY_REQUIRES_EXPLICIT_POLICY_DRAFT");
            if(!profiles.contains(g.profileId) || profiles[g.profileId].contractRevision!=g.contractRevision)issue(g.id,"PROFILE_UNRESOLVED_OR_STALE");
            const auto profile=profiles.value(g.profileId);
            if(!profile.credentialRef.isEmpty())issue(g.id,"CREDENTIAL_POLICY_REQUIRES_EXPLICIT_DRAFT");
            for(const auto& f:g.fields){
                const auto v=vars.value(f.fieldId);const auto where=recipe.id+"/"+g.id+"/"+f.name;
                if(!vars.contains(f.fieldId)){issue(where,"VARIABLE_UNRESOLVED");continue;}
                // Legacy publishes a particular producer's committed bundle,
                // whereas v4 reads cycle variables. A second writer could make
                // those values differ even though the variable UUID is equal.
                for(const auto& t:tools)if(t.kind=="set_variables" && t.id!=g.producerId)
                    for(const auto& row:decodeAssignmentConfig(t.parameters,t.id).rows)
                        if(row.targetId==v.id)issue(where,"VARIABLE_OVERWRITE_REQUIRES_EXPLICIT_DRAFT");
                if(!profile.fieldMapping.contains(f.name))issue(where,"LEGACY_FIELD_MAPPING_UNRESOLVED");
                PublicationSlotKind kind;int index;PublicationValueType type;
                if(v.type==VariableType::Boolean){kind=PublicationSlotKind::Flag;index=flags++;type=PublicationValueType::Boolean;}
                else if(v.type==VariableType::Int32){kind=PublicationSlotKind::Number;index=numbers++;type=PublicationValueType::Int32;}
                else {
                    if(v.type==VariableType::SpatialTransform){++poses;issue(where,"POSE_REQUIRES_EXPLICIT_FRAME_ADAPTER_DRAFT");}
                    else if(v.type==VariableType::String){
                        issue(where,"TEXT_UNBOUNDED_LEGACY_STRING_REQUIRES_64_BYTE_DRAFT");
                        for(const auto& t:tools)if(t.kind=="set_variables")for(const auto& row:decodeAssignmentConfig(t.parameters,t.id).rows)
                            if(row.targetId==v.id)if(auto c=std::get_if<ConstantAssignmentSource>(&row.source))if(auto value=std::get_if<ValueEnvelope>(&c->value))
                                if(value->value)if(auto text=std::get_if<QString>(&*value->value))if(text->toUtf8().size()>64)issue(where,"TEXT_EXCEEDS_64_BYTES");
                    }else issue(where,"NUMERIC_CONVERSION_REQUIRES_EXPLICIT_SCALE_DRAFT");
                    continue;
                }
                if((kind==PublicationSlotKind::Number && index>=16)||(kind==PublicationSlotKind::Flag && index>=32)){issue(where,"FIXED_SLOT_CAPACITY");continue;}
                PublicationSource source{PublicationSourceKind::Variable,v.id,{},0};
                assignments.rows.append({kind,index,source});definitions.append({source,type,v.unit,{}});
                mappings.append(QJsonObject{{"recipe",recipe.id},{"group",g.id},{"field",f.name},{"source",v.id},{"legacyDestination",profile.fieldMapping.value(f.name)},{"kind",kind==PublicationSlotKind::Number?"Number":"Flag"},{"index",index},{"required",g.required}});
                const auto& slot=kind==PublicationSlotKind::Number?in.publication.numbers[index]:in.publication.flags[index];
                if(!slot.required)issue(where,"DESTINATION_REQUIRED_POLICY_MISMATCH");
                if(kind==PublicationSlotKind::Number && (slot.scaleNumerator!=1 || slot.scaleDenominator!=1 || slot.offsetNumerator!=0 || slot.minimum!=std::numeric_limits<qint32>::min() || slot.maximum!=std::numeric_limits<qint32>::max()))issue(where,"NUMERIC_CONVERSION_REQUIRES_EXPLICIT_SCALE_DRAFT");
            }
        }
        if(poses>1)issue(recipe.id,"MULTIPLE_WHOLE_POSES");
        if(!groups.isEmpty()){
            try{assignments.validate(in.publication,definitions);}catch(const std::exception& e){issue(recipe.id,QString::fromUtf8(e.what()));}
            recipe.definition.remove("publication");
            // Preserve graph IDs/dependencies: old publisher is an inert node.
            for(auto& t:tools)if(t.kind=="publish_data"){t.enabled=false;t.parameters=encodePublishConfig({});}
            recipe.definition=withEditorTools(recipe.definition,tools);
            recipe.definition=withRecipePublicationAssignments(recipe.definition,assignments);
            need(recipe.recipeVersion<999999999,"Recipe version exhaustion");++recipe.recipeVersion;recipe.status=RecipeStatus::Draft;
        }
        const auto rev=derived(json(recipe.definition)+QByteArray::number(recipe.recipeVersion));
        auto e=bind(catalogue,{CatalogueCategory::RecipeRevision,QUuid(recipe.id),rev,{}},recipe.name);
        mappings.append(QJsonObject{{"recipe",recipe.id},{"revision",uid(rev)},{"key",double(e.key)}});
    }
    catalogue.revision=derived(registryBytes+json(in.publication.toJson())+ProjectRepository::encode(project)+uid(in.station).toUtf8());
    validateIdentityTransition(oldCatalogue,catalogue);
    const auto before=inventory(in.archiveDirectory);auto unresolved=pending(in,oldCatalogue);
    need(inventory(in.archiveDirectory)==before,"Archive changed during read-only analysis");
    CommunicationMigrationPlan plan;
    plan.files={{"legacy-project.json",projectBytes},{"legacy-profiles.json",profileBytes},{"legacy-registry.json",registryBytes},{"legacy-binary.bin",binaryBytes},
                {"project.json",ProjectRepository::encode(project)},{"registry.json",JsonStationIdentityStore::encode(catalogue)},
                {"template.json",json(in.publication.toJson())}};
    // This schema is the real server profile serializer's schema; package
    // validation loads it with CommunicationServerProfiles before activation.
    const auto& s=in.server;
    plan.files["server.json"]=json({{"schema","visionflow.communication-server.v4/1"},{"enabled",false},{"address",s.address},{"port",s.port},{"peerPolicy",s.peerPolicy},{"parserDeadlineMs",s.parserDeadlineMs},{"ackTimeoutMs",s.ackTimeoutMs},{"retries",s.retries},{"maxStations",s.maxStations},{"maxQueuedResponses",s.maxQueuedResponses},{"writeHighWaterBytes",s.writeHighWaterBytes},{"maxReadBatch",s.maxReadBatch}});
    QJsonObject hashes;for(auto i=plan.files.cbegin();i!=plan.files.cend();++i)hashes[i.key()]=QString::fromLatin1(hash(i.value()));
    plan.report={{"schema","visionflow.communication-migration/1"},{"sources",QJsonObject{{"project",description(in.projectPath,projectBytes)},{"profiles",description(in.profilesPath,profileBytes)},{"registry",description(in.registryPath,registryBytes)},{"binary",description(in.binaryPath,binaryBytes)}}},
        {"archive",path(in.archiveDirectory,true)},{"ledger",before},{"pending",unresolved},{"unsupported",unsupported},{"mapping",mappings},{"files",hashes},
        {"installation",uid(catalogue.installation)},{"station",uid(in.station)},{"stationKey",double(station.key)},{"templateKey",double(templ.key)}};
    need(read(in.projectPath)==projectBytes && read(in.profilesPath)==profileBytes && read(in.registryPath)==registryBytes && read(in.binaryPath,256*1024*1024)==binaryBytes,"Migration source changed during analysis");
    return plan;
}
struct CommunicationMigrationTransaction::Lease {
    QLockFile workspace,archive;
    Lease(const QString& w,const QString& a):workspace(w+"/migration.lock"),archive(a+"/archive.lock"){
        workspace.setStaleLockTime(0);archive.setStaleLockTime(0);
        need(workspace.tryLock(0) && archive.tryLock(0),"Migration requires exclusive workspace/archive writer");
    }
};
CommunicationMigrationTransaction::CommunicationMigrationTransaction(CommunicationMigrationInput in,QString workspace,Fault fault)
    :input_(std::move(in)),workspace_(path(workspace,true)),fault_(std::move(fault)){
    CommunicationListenerOwnership::requireMigrationAllowed();
    lease_=std::make_unique<Lease>(workspace_,path(input_.archiveDirectory,true));
}
CommunicationMigrationTransaction::~CommunicationMigrationTransaction()=default;
void CommunicationMigrationTransaction::boundary(const QString& p) const{if(fault_)fault_(p);}
QByteArray CommunicationMigrationTransaction::pointerDigest() const {
    auto p=workspace_+"/active.json";return QFileInfo::exists(p)?hash(read(p)):QByteArray{};
}
QByteArray CommunicationMigrationTransaction::ledgerDigest() const{return hash(json(inventory(input_.archiveDirectory)));}
QJsonObject CommunicationMigrationTransaction::resolveSelection(const QString& workspace){
    const auto w=path(workspace,true);const auto pointer=object(read(w+"/active.json"));
    CommunicationMigrationInput in;in.archiveDirectory=pointer["archive"].toString();
    CommunicationMigrationTransaction transaction(in,w);const auto selected=transaction.active();
    const auto package=selected["package"].toString();const auto m=transaction.manifest(package,selected["manifest"].toString().toLatin1());
    need(m["unsupported"].toArray().isEmpty() && m["pending"].toArray().isEmpty(),"Unsupported migration selection");
    QJsonObject result=selected;result["projectDirectory"]=QFileInfo(m["sources"].toObject()["project"].toObject()["path"].toString()).absolutePath();
    result["sourceRegistry"]=m["sources"].toObject()["registry"].toObject()["path"];
    result["station"]=m["station"];result["mapping"]=m["mapping"];
    const auto dir=w+"/"+package;
    for(const auto& name:{"project","registry","template","server"}){result[name]=dir+"/"+name+".json";result[QString(name)+"Digest"]=m["files"].toObject()[QString(name)+".json"];}
    result["baselineRegistry"]=dir+"/legacy-registry.json";
    result["baselineRegistryDigest"]=m["files"].toObject()["legacy-registry.json"];
    return result;
}
QByteArray CommunicationMigrationTransaction::prepare(const QString& package,const QByteArray& reviewed){
    safeName(package);const auto plan=CommunicationMigrationPlanner(input_).analyze();need(plan.digest()==reviewed,"Stale migration analysis");
    const auto dir=workspace_+"/"+package;need(!QFileInfo::exists(dir) && QDir().mkdir(dir),"Migration package already exists/unavailable");boundary("package-created");
    for(auto i=plan.files.cbegin();i!=plan.files.cend();++i){write(dir+"/"+i.key(),i.value(),[&](const QString& p){boundary(i.key()+":"+p);});boundary(i.key());}
    auto m=plan.report;m["planDigest"]=QString::fromLatin1(plan.digest());m["package"]=package;
    write(dir+"/manifest.json",json(m),[&](const QString& p){boundary("manifest:"+p);});boundary("manifest-committed");return hash(json(m));
}
QJsonObject CommunicationMigrationTransaction::manifest(const QString& package,const QByteArray& digest) const {
    safeName(package);const auto dir=path(workspace_+"/"+package,true);const auto b=read(dir+"/manifest.json");
    need(hash(b)==digest,"Migration manifest digest mismatch");auto m=object(b);
    need(m["schema"]=="visionflow.communication-migration/1" && m["package"]==package && m["archive"]==path(input_.archiveDirectory,true),"Migration manifest identity mismatch");
    const auto files=m["files"].toObject();need(files.size()==8,"Migration package file set invalid");
    for(auto i=files.begin();i!=files.end();++i){need(!i.key().contains('/') && !i.key().contains('\\') && i.key()!="..","Unsafe migration file name");need(hash(read(dir+"/"+i.key(),256*1024*1024))==i.value().toString().toLatin1(),"Migration staged/backup hash mismatch");}
    ProjectRepository{}.load(dir+"/project.json");CommunicationProfiles(dir+"/legacy-profiles.json").load();
    JsonStationIdentityStore::decode(read(dir+"/registry.json"));PublicationTemplate::fromJson(object(read(dir+"/template.json")));
    auto s=CommunicationServerProfiles(dir+"/server.json").load();need(s && !s->enabled,"Migration server must remain disabled");return m;
}
QJsonObject CommunicationMigrationTransaction::active() const {
    if(pointerDigest().isEmpty())return {{"mode","legacy"},{"project",path(input_.projectPath)},{"profiles",path(input_.profilesPath)}};
    auto o=object(read(workspace_+"/active.json"));
    need(o.size()==6 && o["schema"]=="visionflow.communication-selection/1" && !QUuid(o["revision"].toString()).isNull() && (o["mode"]=="v4" || o["mode"]=="legacy"),"Invalid active migration pointer");
    manifest(o["package"].toString(),o["manifest"].toString().toLatin1());need(o["archive"]==path(input_.archiveDirectory,true),"Active archive mismatch");return o;
}
void CommunicationMigrationTransaction::swap(const QJsonObject& next,const QByteArray& expected){
    need(pointerDigest()==expected,"Stale activation revision");boundary("before-pointer");
    write(workspace_+"/active.json",json(next),[&](const QString& p){boundary("pointer:"+p);});boundary("after-pointer");
}
void CommunicationMigrationTransaction::activate(const QString& package,const QByteArray& reviewed,const QByteArray& expected){
    const auto m=manifest(package,reviewed);need(m["unsupported"].toArray().isEmpty() && m["pending"].toArray().isEmpty(),"Unsupported/pending migration blocked");
    const auto fresh=CommunicationMigrationPlanner(input_).analyze();
    need(fresh.supported() && fresh.digest()==m["planDigest"].toString().toLatin1(),"Migration source/ledger changed; analyze again");
    need(active()["mode"]=="legacy","Already migrated; rollback/review required");boundary("validated");
    swap({{"schema","visionflow.communication-selection/1"},{"revision",uid(QUuid::createUuid())},{"mode","v4"},{"package",package},{"manifest",QString::fromLatin1(reviewed)},{"archive",path(input_.archiveDirectory,true)}},expected);
}
void CommunicationMigrationTransaction::rollback(const QByteArray& expected,const QByteArray& reviewedLedger){
    auto o=active();need(o["mode"]=="v4" && pointerDigest()==expected,"Rollback requires current v4 revision");
    auto m=manifest(o["package"].toString(),o["manifest"].toString().toLatin1());
    need(ledgerDigest()==reviewedLedger,"Rollback ledger revision changed");
    const auto c=JsonStationIdentityStore::decode(read(input_.registryPath));
    need(uid(c.installation)==m["installation"] && pending(input_,c).isEmpty(),"Rollback pending work requires drain/reconcile");
    // Binary/config identity is checked against the immutable backup, while the
    // archive stays live. Never replace the ledger with a backup snapshot.
    for(const auto pair:{std::pair{"project",input_.projectPath},std::pair{"profiles",input_.profilesPath},std::pair{"binary",input_.binaryPath}})
        need(hash(read(pair.second,256*1024*1024))==m["sources"].toObject()[pair.first].toObject()["sha256"].toString().toLatin1(),"Rollback binary/config differs from backup");
    const auto original=JsonStationIdentityStore::decode(read(workspace_+"/"+o["package"].toString()+"/legacy-registry.json"));
    const auto staged=JsonStationIdentityStore::decode(read(workspace_+"/"+o["package"].toString()+"/registry.json"));
    if(c!=original && c!=staged)validateIdentityTransition(staged,c);
    boundary("rollback-validated");o["mode"]="legacy";o["revision"]=uid(QUuid::createUuid());swap(o,expected);
}
}
