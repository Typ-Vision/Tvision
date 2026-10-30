#include "CommunicationStationComposition.hpp"
#include "FixedSlotPublication.hpp"
#include "persistence/ProjectRepository.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "domain/ToolRegistry.hpp"
#include "domain/IndustrialSourceReference.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QSaveFile>
#include <algorithm>

namespace visionflow {
namespace {
using C=CommunicationV4Codec;
void need(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
QByteArray read(const QString& path){QFile f(path);need(!QFileInfo(path).isSymLink() && f.open(QIODevice::ReadOnly) && f.size()<=20*1024*1024,"Machine manifest unavailable");return f.readAll();}
QByteArray selectedBytes(const QJsonObject& selection,const QString& name){const auto bytes=read(selection[name].toString());need(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()==selection[name+"Digest"].toString().toLatin1(),"Selected immutable manifest changed after validation");return bytes;}
quint32 key(const IdentityCatalogue& c,CatalogueCategory category,const QUuid& entity,const QUuid& revision={}){for(const auto& e:c.entries)if(!e.retired && e.identity==CatalogueIdentity{category,entity,revision,{}})return e.key;throw std::runtime_error("Immutable manifest catalogue key missing/retired");}
QUuid cameraRevision(const StationSnapshot& s){QByteArray bytes("visionflow.camera-set/1\0",23);for(const auto& c:s.selection.cameras){bytes+=c.sourceId.toUtf8();bytes+=c.camera.toRfc4122();QByteArray n(4,0);C::putUInt32(n,0,c.cameraKey);bytes+=n;}return QUuid::fromRfc4122(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).first(16));}
ScalarUnit unit(const QString& u){if(u.isEmpty() || u=="ratio" || u=="count")return ScalarUnit::Unitless;if(u=="mm")return ScalarUnit::Millimetre;if(u=="m")return ScalarUnit::Metre;if(u=="rad")return ScalarUnit::Radian;if(u=="deg" || u=="degree")return ScalarUnit::Degree;throw std::runtime_error("Publication unit requires explicit converter");}
// Scalar production bridge. Whole pose needs the explicit frame/adapter
// catalogue integration from COMM4-19; planar outputs never become Pose.
QVector<PublicationBinding> bindings(const Recipe& recipe){
    const auto a=recipePublicationAssignments(recipe.definition);need(bool(a),"Publication assignments missing");
    const auto tools=editorTools(recipe.definition);const auto graph=validateGraph(tools,executionConnections(recipe.definition));need(graph.valid(),"Invalid publication graph");
    QVector<PublicationBinding> out;QSet<QString> visiting;
    std::function<void(PublicationSource)> add=[&](PublicationSource source){
        for(const auto& b:out)if(b.definition.source==source)return;
        const auto token=QString::number(int(source.kind))+source.id+":"+source.port;need(!visiting.contains(token),"Publication dependency cycle");visiting.insert(token);
        PublicationBinding b;b.definition.source=source;
        if(source.kind==PublicationSourceKind::Variable){
            const auto variables=recipeVariables(recipe.definition);auto found=std::find_if(variables.begin(),variables.end(),[&](const auto& v){return v.id==source.id;});need(found!=variables.end(),"Unknown publication variable");
            need(found->type!=VariableType::SpatialTransform,"Whole Pose adapter unavailable");b.definition.type=PublicationValueType(int(found->type));b.definition.unit=found->unit;
            int writers=0;
            for(const auto& t:tools)if(t.enabled && t.kind=="set_variables"){
                const auto config=decodeAssignmentConfig(t.parameters,t.id);
                for(const auto& row:config.rows)if(row.targetId==source.id){++writers;b.producerId=t.id;b.port=source.id;b.revision=config.revision;
                    if(const auto* rhs=std::get_if<ToolOutputAssignmentSource>(&row.source))b.dependencies.append({PublicationSourceKind::ToolOutput,rhs->toolId,rhs->port,rhs->revision});
                    if(const auto* rhs=std::get_if<BundleAssignmentSource>(&row.source)){
                        PublicationSource dep{PublicationSourceKind::Variable,rhs->fieldId,{},0};add(dep);
                        const auto it=std::find_if(out.begin(),out.end(),[&](const auto& parent){return parent.definition.source==dep;});need(it!=out.end() && it->producerId==rhs->producerId && it->revision==rhs->revision,"Bundle producer requires explicit publication binding");
                        b.dependencies=it->dependencies;b.dependencies.append(dep);
                    }
                }
            }
            need(writers==1,"Publication variable must have one immutable writer");
        }else{
            auto t=std::find_if(tools.begin(),tools.end(),[&](const auto& tool){return tool.enabled && tool.id==source.id;});need(t!=tools.end(),"Missing direct publication producer");
            const auto descriptor=ToolRegistry::get(t->kind);need(source.revision==descriptor.version,"Stale direct publication version");
            auto p=std::find_if(descriptor.outputs.begin(),descriptor.outputs.end(),[&](const auto& port){return port.id==source.port;});need(p!=descriptor.outputs.end(),"Missing direct publication port");
            need(p->type==PortType::Number || p->type==PortType::Boolean || p->type==PortType::Text,"Whole/scalar publication converter unavailable");
            b.producerId=source.id;b.port=source.port;b.revision=source.revision;b.definition.unit=unit(p->unit);b.definition.type=p->type==PortType::Number?PublicationValueType::Float64:p->type==PortType::Boolean?PublicationValueType::Boolean:PublicationValueType::String;
        }
        for(const auto& dep:b.dependencies)add(dep);out.append(b);visiting.remove(token);
    };
    for(const auto& row:a->rows)add(row.source);return out;
}
}
QJsonObject CommunicationStationComposition::restoreSelection(const QString& machine){
    const QDir root(machine);const auto work=root.filePath("migration");if(!QFile::exists(work+"/active.json"))return {};
    const auto selected=CommunicationMigrationTransaction::resolveSelection(work);
    JsonStationIdentityStore live(root.filePath("communication-identities-v4.json"));
    need(QFileInfo(selected["sourceRegistry"].toString()).canonicalFilePath()==QFileInfo(root.filePath("communication-identities-v4.json")).canonicalFilePath(),"Selection catalogue authority mismatch");
    const auto current=live.load();const auto baseline=JsonStationIdentityStore::decode(selectedBytes(selected,"baselineRegistry"));const auto seed=JsonStationIdentityStore::decode(selectedBytes(selected,"registry"));
    if(selected["mode"]=="v4" && current==baseline)live.compareExchange(current.revision,seed);
    else if(current!=seed && !(selected["mode"]=="legacy" && current==baseline))validateIdentityTransition(seed,current);
    return selected;
}
CommunicationStationComposition::CommunicationStationComposition(QString machine,QJsonObject selected,StationIdentityStore& ids,CommunicationPeerSessionService& sessions)
    :directory_(std::move(machine)),selection_(std::move(selected)),identities_(ids),sessions_(sessions),selectionStore_(directory_.filePath("communication-stations-v4.json")),activations_(selectionStore_,identities_,boundary_){
    try{
        const auto catalogue=identities_.load();validateIdentityCatalogue(catalogue);
        ledger_=std::make_unique<StationDeliveryLedger>(selection_.value("archive").toString().isEmpty()?directory_.filePath("production"):selection_["archive"].toString(),catalogue.installation);
        durableAdmission_=std::make_unique<StationDeliveryLedger::Admission>(*ledger_);durableControl_=std::make_unique<StationDeliveryLedger::Control>(*ledger_);
        controls_=std::make_unique<StationControlService>(activations_,identities_,durableControl_.get(),[this](quint32 k){return recipe(k);});
        // Creating an empty selection catalogue after explicit installation
        // enrollment does not select a recipe or grant Run permission.
        if(!QFile::exists(directory_.filePath("communication-stations-v4.json")))activations_.initialize();
        auto addTemplate=[&](const QString& path){const auto publication=PublicationTemplate::fromJson(QJsonDocument::fromJson(read(path)).object());templates_.insert(key(catalogue,CatalogueCategory::TemplateRevision,publication.id,publication.revision),publication);};
        if(selection_.value("mode")=="v4"){selectedBytes(selection_,"template");addTemplate(selection_["template"].toString());selectedBytes(selection_,"template");}
        for(const auto& file:QDir(directory_.filePath("publication-templates-v4")).entryList({"*.json"},QDir::Files))addTemplate(directory_.filePath("publication-templates-v4/"+file));
        auto save=[&](const StationRecipeSelection& choice){
            const auto a=recipePublicationAssignments(choice.recipe.definition);if(!a)return;
            // Historical selections stay in the archive, but retired recipe,
            // template or camera keys cannot become a live manifest again.
            try{need(key(catalogue,CatalogueCategory::RecipeRevision,QUuid(choice.recipe.id),choice.revision)==choice.recipeKey,"Recipe manifest key mismatch");for(const auto& c:choice.cameras)need(key(catalogue,CatalogueCategory::Camera,c.camera)==c.cameraKey,"Camera manifest key mismatch");key(catalogue,CatalogueCategory::TemplateRevision,a->templateId,a->templateRevision);}catch(...){return;}
            const auto tk=key(catalogue,CatalogueCategory::TemplateRevision,a->templateId,a->templateRevision);need(templates_.contains(tk),"Publication template manifest missing");
            FixedSlotPublication::capture(templates_[tk],*a,bindings(choice.recipe),"configuration-only");
            if(recipes_.contains(choice.recipeKey))need(stationRecipeBytes(recipes_[choice.recipeKey].selection)==stationRecipeBytes(choice),"Immutable saved recipe conflict");
            recipes_[choice.recipeKey]={choice,tk};
        };
        for(const auto& s:selectionStore_.load().history)save(s.selection);
        if(selection_.value("mode")=="v4"){
            const auto expected=selectedBytes(selection_,"project");const auto project=ProjectRepository{}.load(selection_["project"].toString());need(ProjectRepository::encode(project)==expected,"Selected project changed during load");
            for(const auto& r:project.recipes){
                StationRecipeSelection choice;choice.recipe=r;choice.projectDirectory=selection_["projectDirectory"].toString();
                for(const auto& v:selection_["mapping"].toArray()){const auto o=v.toObject();if(o["recipe"]==r.id && o.contains("revision")){choice.revision=QUuid(o["revision"].toString());choice.recipeKey=quint32(o["key"].toDouble());}}
                for(const auto& t:editorTools(r.definition))if(t.enabled && ToolRegistry::get(t.kind).acquisitionSource){const auto id=t.kind=="industrial_camera"?QUuid(IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject()).camera.value):QUuid(t.id);choice.cameras.append({t.id,id,key(catalogue,CatalogueCategory::Camera,id)});}
                save(choice);
            }
        }
        for(const auto& entry:catalogue.entries)if(!entry.retired && entry.identity.category==CatalogueCategory::Station){
            auto station=std::make_unique<Station>();station->definition={entry.identity.entity,entry.key,entry.name};
            station->runtime=std::make_unique<RuntimeService>(selection_.value("archive").toString().isEmpty()?directory_.filePath("production"):selection_["archive"].toString(),directory_.filePath("calibrations.json"),RuntimeIdentity::localEngineer());
            station->coordinator=std::make_unique<RuntimeCoordinator>(*station->runtime);boundary_.bind(entry.identity.entity,*station->runtime,*station->coordinator);station->run=std::make_unique<StationRunController>(boundary_,entry.identity.entity);controls_->bind(station->definition,*station->run);
            execution_.bind(entry.identity.entity,*station->coordinator,directory_.filePath("calibrations.json"),[this](const auto& i,const auto& c,const auto& r){complete(i,c,r);});stations_.push_back(std::move(station));
        }
        admissionBoundary_=std::make_unique<StationRuntimeAdmissionBoundary>(boundary_,[this](const QUuid& id){return manifest(id);},[this](const auto& s){
            if(ledger_->pending(s.station->station.id) || ledger_->pending(s.station->station.id,true))return StationAdmissionReservation{2,{}};
            // Only sources supported by the ordinary providers may execute.
            // Native/webcam physical identity integration is never synthesized.
            for(const auto& t:editorTools(s.station->selection.recipe.definition))if(t.enabled && ToolRegistry::get(t.kind).acquisitionSource && t.kind!="image_file")return StationAdmissionReservation{17,{}};
            return execution_.reserve(s);
        },[this](const auto& i,const auto& c,const auto& lease){execution_.dispatch(i,c,lease);});
        for(const auto& s:stations_)admissionBoundary_->bind(*s->run);
        admission_=std::make_unique<StationAdmissionService>(*durableAdmission_,*admissionBoundary_);
        sessions_.setDeliveryLedger(ledger_.get());sessions_.setControlHandler([this](const auto& k,const auto& f){return controls_->handle(k,f);});sessions_.setTriggerHandler([this](const auto& k,const auto& f){return admission_->trigger(k,f);});
    }catch(const std::exception& e){error_=QString::fromUtf8(e.what());}
}
CommunicationStationComposition::~CommunicationStationComposition(){sessions_.setMetadata({});sessions_.setStatusHandler({});sessions_.setTriggerHandler({});sessions_.setControlHandler({});sessions_.setDeliveryLedger(nullptr);}
std::optional<StationSavedRecipe> CommunicationStationComposition::recipe(quint32 k) const {
    if(!recipes_.contains(k))return {};
    try{const auto& saved=recipes_[k];const auto catalogue=identities_.load();const auto a=recipePublicationAssignments(saved.selection.recipe.definition);if(!a)return {};
        need(key(catalogue,CatalogueCategory::RecipeRevision,QUuid(saved.selection.recipe.id),saved.selection.revision)==k,"Retired recipe");need(key(catalogue,CatalogueCategory::TemplateRevision,a->templateId,a->templateRevision)==saved.templateKey,"Retired template");for(const auto& c:saved.selection.cameras)need(key(catalogue,CatalogueCategory::Camera,c.camera)==c.cameraKey,"Retired camera");return saved;
    }catch(...){return {};}
}
std::optional<StationAdmissionSnapshot> CommunicationStationComposition::manifest(const QUuid& id) const {
    const auto s=activations_.snapshot(id);if(!s || s->selection.cameras.size()>32)return {};const auto r=recipe(s->selection.recipeKey);if(!r || stationRecipeBytes(r->selection)!=stationRecipeBytes(s->selection))return {};
    for(const auto& t:editorTools(s->selection.recipe.definition))if(t.enabled && ToolRegistry::get(t.kind).acquisitionSource && t.kind!="image_file")return {};
    return StationAdmissionSnapshot{s,r->templateKey,cameraRevision(*s),{},{},{}};
}
void CommunicationStationComposition::attach(CommunicationServerLifecycle& server,CommunicationWorkspace& workspace){
    workspace.setMachinePaths(directory_.filePath("migration"),directory_.filePath("communication-identities-v4.json"));
    if(ledger_)workspace.setLedger(ledger_.get());if(!error_.isEmpty()){workspace.setNotice("Station composition unavailable: "+error_);return;}
    need(QDir().mkpath(directory_.filePath("migration")),"Machine migration workspace unavailable");
    for(const auto& s:stations_){const auto id=s->definition.id;server.addStation(*s->run);workspace.bind({s->definition,s->run.get(),[this,id]{return activations_.snapshot(id);},[this,id](bool automatic){const auto m=manifest(id);need(bool(m),"Immutable publication manifest unavailable");controls_->setAuto(id,m->station->activation,automatic);},"Immutable saved template",{},[this,id]{return manifest(id)?QString{}:QString("Immutable publication / source provider unavailable; Run/Auto kapalı.");}});}
    workspace.setRecipeControl([this]{QVector<StationSavedRecipe> values;for(auto k:recipes_.keys())if(const auto saved=recipe(k))values.append(*saved);return values;},[this](const QUuid& id,const QUuid& expected,quint32 r,quint32 t){controls_->select(id,expected,r,t);});
    workspace.setPoll([this]{poll();});
    workspace.setRemoteControl([this]{return remoteEnabled_;},[this](bool enabled){controls_->setRemoteEnabled(enabled);remoteEnabled_=enabled;});
    // Remote Enable is process-local, defaults false on every startup. It is an
    // explicit operator permission, never recovered from persisted Run state.
    sessions_.setStatusHandler([this](const CommunicationV4Frame& f){
        CommunicationV4Frame response{f.header,QByteArray(896,0)};response.header.kind=102;C::putUInt32(response.body,824,2);
        const auto cat=identities_.load();const CatalogueEntry* station=nullptr;for(const auto& e:cat.entries)if(e.key==f.header.stationKey && e.identity.category==CatalogueCategory::Station && !e.retired)station=&e;
        if(!station){response.header.kind=190;C::putUInt32(response.body,76,10);return response;}
        const auto s=activations_.snapshot(station->identity.entity);const auto m=manifest(station->identity.entity);
        if(s && !m){response.header.kind=190;C::putUInt32(response.body,76,20);return response;}
        response.header.recipeRevisionKey=s?s->selection.recipeKey:0;response.header.templateRevisionKey=m?m->templateKey:0;response.header.activation=s?s->activation:QUuid{};
        bool ready=false,busy=false;for(const auto& b:stations_)if(b->definition.id==station->identity.entity){const auto state=b->run->state().mode;ready=m && (state==StationMode::ArmedManual || state==StationMode::ArmedAuto);busy=state==StationMode::Running || state==StationMode::Preparing || state==StationMode::Draining;}
        const auto pending=int(ledger_->pending(station->identity.entity))+int(ledger_->pending(station->identity.entity,true));C::putUInt32(response.body,756,ready && !pending);C::putUInt32(response.body,760,busy);C::putUInt32(response.body,764,pending);return response;
    });
    sessions_.setMetadata([this](const IdentityCatalogue& cat,const CatalogueEntry& e){
        QByteArray content,dependencies;QByteArray metadata(256,0);C::putUInt32(metadata,0,1);C::putUInt32(metadata,4,256);
        // Exact immutable, versioned byte manifests. Names are excluded.
        if(e.identity.category==CatalogueCategory::Station){
            const auto s=activations_.snapshot(e.identity.entity);const auto m=manifest(e.identity.entity);content="visionflow.station-manifest/1"+e.identity.entity.toRfc4122();
            quint32 mode=0;for(const auto& station:stations_)if(station->definition.id==e.identity.entity)mode=quint32(station->run->state().mode);C::putUInt32(metadata,72,mode);
            if(s && m){C::putUInt32(metadata,76,s->selection.recipeKey);C::putUInt32(metadata,80,m->templateKey);C::putUInt32(metadata,84,s->selection.cameras.size());metadata.replace(88,16,s->activation.toRfc4122());metadata.replace(104,16,m->cameraSetRevision.toRfc4122());dependencies=s->selection.revision.toRfc4122()+m->cameraSetRevision.toRfc4122();content+=s->activation.toRfc4122();}C::putUInt32(metadata,120,remoteEnabled_);
        }else if(e.identity.category==CatalogueCategory::RecipeRevision){
            const auto r=recipe(e.key);if(!r)return QByteArray{};
            content="visionflow.recipe-manifest/1"+QJsonDocument(r->selection.recipe.definition).toJson(QJsonDocument::Compact)+r->selection.revision.toRfc4122();dependencies=QByteArray::number(r->templateKey);for(const auto& c:r->selection.cameras)dependencies+=c.camera.toRfc4122();
            C::putUInt32(metadata,72,r->selection.recipe.applicationType==ApplicationType::Measurement?1:r->selection.recipe.applicationType==ApplicationType::DefectInspection?2:3);C::putUInt32(metadata,76,r->selection.cameras.size());C::putUInt32(metadata,80,r->templateKey);
        }else if(e.identity.category==CatalogueCategory::TemplateRevision){
            if(!templates_.contains(e.key))return QByteArray{};content=QJsonDocument(templates_[e.key].toJson()).toJson(QJsonDocument::Compact);dependencies=templates_[e.key].revision.toRfc4122();for(auto [o,v]:{std::pair{72,16},{76,32},{80,4},{84,1},{88,64},{92,1},{96,1},{100,1}})C::putUInt32(metadata,o,v);
        }else if(e.identity.category==CatalogueCategory::Camera){
            bool found=false;for(const auto& r:recipes_)for(const auto& c:r.selection.cameras)if(c.cameraKey==e.key){content="visionflow.camera-manifest/1"+c.camera.toRfc4122();dependencies=r.selection.revision.toRfc4122();for(const auto& t:editorTools(r.selection.recipe.definition))if(t.id==c.sourceId){C::putUInt32(metadata,72,t.kind=="image_file"?1:t.kind=="webcam"?2:3);C::putUInt32(metadata,88,t.kind=="image_file"?3:0);found=true;}}
            if(!found)return QByteArray{};
        }else return QByteArray{};
        metadata.replace(8,32,QCryptographicHash::hash(content,QCryptographicHash::Sha256));metadata.replace(40,32,QCryptographicHash::hash(dependencies,QCryptographicHash::Sha256));return metadata;
    });
}
void CommunicationStationComposition::poll(){if(!error_.isEmpty())return;controls_->poll();execution_.poll();for(auto it=completed_.begin();it!=completed_.end();)if(admission_->release(*it))it=completed_.erase(it);else ++it;}
void CommunicationStationComposition::complete(const StationAdmissionIntent& intent,const ProductionCycle&,const CycleResult& result){
    // Retry the same sealed terminal, never regenerate result UUIDs after a
    // partial terminal/pending commit.
    if(const auto old=ledger_->inspect(intent.identity);old && old->value.replay.header.kind==111){ledger_->seal(intent.identity,old->value.replay);completed_.append(intent.identity);return;}
    const auto& r=intent.snapshot.station->selection.recipe;const auto a=recipePublicationAssignments(r.definition);need(bool(a) && templates_.contains(intent.snapshot.templateKey),"Immutable publication missing at terminal");
    const auto bs=bindings(r);PublicationCycleView view;view.inputs.cycleId=result.cycleId;view.inputs.configId=QString::fromLatin1(result.configId.toHex());view.inputs.variables=recipeVariables(r.definition);
    view.execution=result.state==ExecutionState::Done?PublicationExecution::Done:result.state==ExecutionState::Skipped?PublicationExecution::Canceled:PublicationExecution::Error;view.decision=result.decision()==ResultDecision::Ok?PublicationDecision::Ok:result.decision()==ResultDecision::Nok?PublicationDecision::Nok:PublicationDecision::NotEvaluated;
    for(const auto& tool:result.tools)if(tool.state==ExecutionState::Done){for(const auto& port:tool.outputs)if(port.valid)if(const auto* bundle=std::get_if<AssignmentBundle>(&port.data))view.inputs.bundles[tool.toolId]=*bundle;}
    for(const auto& b:bs){
        if(b.definition.source.kind==PublicationSourceKind::Variable){const auto bundle=view.inputs.bundles.value(b.producerId);if(bundle.fields.contains(b.port))std::visit([&](const auto& value){view.producerFrames[b.producerId]=value.frameId;},bundle.fields[b.port]);continue;}
        for(const auto& tool:result.tools)if(tool.toolId==b.producerId && tool.state==ExecutionState::Done){const auto port=tool.outputs.value(b.port);if(!port.valid)continue;ValueEnvelope e;e.validity=ValueValidity::Valid;e.unit=b.definition.unit;e.cycleId=port.cycleId;e.configId=view.inputs.configId;e.frameId=port.frameId;e.producerId=b.producerId;e.producerPort=b.port;
            if(const auto* v=std::get_if<double>(&port.data)){e.type=ScalarType::Float64;e.value=*v;}else if(const auto* v=std::get_if<bool>(&port.data)){e.type=ScalarType::Boolean;e.value=*v;}else if(const auto* v=std::get_if<QString>(&port.data)){e.type=ScalarType::String;e.value=*v;}else continue;
            view.inputs.toolOutputs[b.producerId][b.port]={b.revision,e};view.producerFrames[b.producerId]=port.frameId;
        }
    }
    const auto fixed=FixedSlotPublication::capture(templates_[intent.snapshot.templateKey],*a,bs,view.inputs.configId)->evaluate(view);
    CommunicationV4Frame terminal{intent.accepted.header,QByteArray(896,0)};terminal.header.kind=111;terminal.body.replace(0,16,QUuid::createUuid().toRfc4122());terminal.body.replace(16,16,QUuid(result.cycleId).toRfc4122());
    C::putUInt32(terminal.body,64,quint32(fixed.execution));C::putUInt32(terminal.body,68,quint32(fixed.decision));C::putUInt32(terminal.body,72,fixed.resultValid);C::putUInt32(terminal.body,76,fixed.resultValid?0:22);
    C::putUInt32(terminal.body,80,fixed.numberValid);C::putUInt32(terminal.body,84,fixed.flagValid);C::putUInt32(terminal.body,88,fixed.flagValue);C::putUInt32(terminal.body,92,fixed.textValid);C::putUInt32(terminal.body,100,3);
    if(result.frames.size()==1)terminal.body.replace(136,16,QUuid(result.frames.first().frameId).toRfc4122());
    for(int n=0;n<16;++n)C::putInt32(terminal.body,152+4*n,fixed.numbers[n]);
    for(int n=0;n<4;++n){C::putUInt32(terminal.body,216+68*n,fixed.texts[n].size());terminal.body.replace(220+68*n,fixed.texts[n].size(),fixed.texts[n]);}
    C::putUInt32(terminal.body,764,1);if(intent.snapshot.station->selection.cameras.size()==1)C::putUInt32(terminal.body,820,intent.snapshot.station->selection.cameras.first().cameraKey);C::putUInt32(terminal.body,824,10);
    // Digest includes the final body; codec validates the sealed form.
    auto raw=C::encode(intent.accepted,V4Direction::Response);C::putUInt32(raw,8,111);raw.replace(128,896,terminal.body);terminal.body.replace(32,32,C::terminalDigest(raw));C::encode(terminal,V4Direction::Response);ledger_->seal(intent.identity,terminal);completed_.append(intent.identity);
}
}
