#include "CommunicationWorkspace.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>

namespace visionflow {
CommunicationWorkspace::CommunicationWorkspace(CommunicationServerLifecycle& s,const CommunicationServerProfiles& p,StationIdentityStore& i):server_(s),profiles_(p),identities_(i){}
void CommunicationWorkspace::owner() const {if(QThread::currentThread()!=owner_)throw std::runtime_error("Communication workspace requires owner thread");}
void CommunicationWorkspace::bind(Binding b){owner();if(!b.run || b.station.id!=b.run->station() || bindings_.contains(b.station.id))throw std::runtime_error("Invalid station workspace binding");bindings_.insert(b.station.id,std::move(b));}
QString CommunicationWorkspace::modeName(StationMode m){switch(m){case StationMode::Stopped:return "STOPPED";case StationMode::Preparing:return "PREPARING";case StationMode::ArmedManual:return "ARMED_MANUAL";case StationMode::ArmedAuto:return "ARMED_AUTO";case StationMode::Running:return "RUNNING";case StationMode::Draining:return "DRAINING";case StationMode::Blocked:return "BLOCKED";}return "UNKNOWN";}
QVector<CommunicationStationView> CommunicationWorkspace::stations() const {
    owner();const auto catalogue=identities_.load();validateIdentityCatalogue(catalogue);QVector<CommunicationStationView> rows;
    const auto leases=CameraLeaseArbiter::diagnostics();QSet<QUuid> pending;
    if(ledger_)for(const auto& r:ledger_->recovery())if(!r.value.acknowledged)pending.insert(r.key.station);
    for(const auto& e:catalogue.entries)if(!e.retired && e.identity.category==CatalogueCategory::Station){
        CommunicationStationView v;v.station={e.identity.entity,e.key,e.name};
        const auto b=bindings_.constFind(v.station.id);
        if(b!=bindings_.cend()){
            v.runtime=b->run->state();v.automatic=b->run->automatic();v.snapshot=b->snapshot?b->snapshot():nullptr;
            v.publication=b->publication;v.unavailable=b->unavailable;
            if(b->unavailableReason)v.unavailable=b->unavailableReason();
            if(v.snapshot && recipes_)for(const auto& saved:recipes_())if(saved.selection.recipeKey==v.snapshot->selection.recipeKey){const auto assignments=recipePublicationAssignments(saved.selection.recipe.definition);if(assignments)v.publication=QString("Template #%1 • %2 / %3").arg(saved.templateKey).arg(assignments->templateId.toString(QUuid::WithoutBraces),assignments->templateRevision.toString(QUuid::WithoutBraces));}
            v.canRun=v.snapshot && ledger_ && b->run->drained();v.canAuto=v.snapshot && ledger_ && bool(b->automatic);
            if(b->unavailableReason && !v.unavailable.isEmpty()){v.canRun=false;v.canAuto=false;}
            if(v.snapshot && recipes_){bool saved=false;for(const auto& candidate:recipes_())saved|=stationRecipeBytes(candidate.selection)==stationRecipeBytes(v.snapshot->selection);if(!saved){v.canRun=false;v.canAuto=false;v.unavailable="Immutable recipe/template manifest unavailable or retired";}}
        }else v.unavailable="Station Runtime / immutable manifest bağlanmadı.";
        v.runtime.deliveryPending=v.runtime.deliveryPending || pending.contains(v.station.id);
        if(!ledger_)v.unavailable+="\nLedger bağlanmadı; pending yokluğu doğrulanmadı.";
        if(v.runtime.deliveryPending || restartRequired_){v.canRun=false;v.canAuto=false;}
        QStringList owners;
        if(v.snapshot)for(const auto& camera:v.snapshot->selection.cameras){
            QString value=QString("%1: boş").arg(camera.camera.toString(QUuid::WithoutBraces));
            for(const auto& lease:leases)if(lease.cameraId==camera.camera.toString(QUuid::WithoutBraces) || lease.cameraId==camera.camera.toString())value=QString("%1: %2 (g%3%4)").arg(lease.cameraId,lease.owner).arg(lease.generation).arg(lease.quarantined?", QUARANTINED":"");
            owners.append(value);
        }
        v.cameraOwners=owners.isEmpty()?"Kamera ataması yok":owners.join('\n');rows.append(v);
    }
    return rows;
}
QString CommunicationWorkspace::listener() const {
    owner();QString state;
    switch(server_.state()){case CommunicationServerState::NotConfigured:state="NOT_CONFIGURED";break;case CommunicationServerState::Disabled:state="DISABLED";break;case CommunicationServerState::Listening:state="LISTENING";break;case CommunicationServerState::PeerConnected:state="CONNECTED";break;case CommunicationServerState::Fault:state="FAULT";break;case CommunicationServerState::Closing:state="CLOSING";break;case CommunicationServerState::Closed:state="CLOSED";break;}
    return QString("V4 • %1 • port %2\nListener generation: %3\n%4\nBağlantı durumu üretim Ready veya parça sonucu değildir.\n%5").arg(state).arg(server_.port()).arg(server_.listenerGeneration().toString(QUuid::WithoutBraces),server_.error(),notice_);
}
QString CommunicationWorkspace::history() const {
    owner();if(!ledger_)return "V4 ledger bağlanmadı. Geçmiş veya pending yokluğu doğrulanmadı.";
    QJsonArray a;for(const auto& r:ledger_->recovery())a.append(QJsonObject{{"station",r.key.station.toString(QUuid::WithoutBraces)},{"request",QString::number(r.key.request)},{"state",r.state},{"control",r.control},{"acknowledged",r.value.acknowledged},{"result",QUuid::fromRfc4122(r.value.replay.body.first(16)).toString(QUuid::WithoutBraces)}});
    return QString::fromUtf8(QJsonDocument(a).toJson());
}
CommunicationServerProfile CommunicationWorkspace::profile() const {owner();return profiles_.load().value_or(CommunicationServerProfile{});}
void CommunicationWorkspace::saveProfile(const CommunicationServerProfile& p){owner();profiles_.save(p);}
void CommunicationWorkspace::command(const QUuid& id,const QUuid& expected,const QString& command){
    owner();auto b=bindings_.find(id);if(b==bindings_.end())throw std::runtime_error("Station Runtime bağlanmadı");
    if(command=="stop"){b->run->stop();return;}
    if(restartRequired_)throw std::runtime_error("Migration selection changed: restart required");
    if(b->unavailableReason){const auto reason=b->unavailableReason();if(!reason.isEmpty())throw std::runtime_error(reason.toStdString());}
    if(!ledger_)throw std::runtime_error("Durable ledger bağlanmadı");
    const auto snapshot=b->snapshot?b->snapshot():nullptr;
    if(!snapshot || snapshot->activation!=expected)throw std::runtime_error("STALE_ACTIVATION");
    if(recipes_){bool saved=false;for(const auto& candidate:recipes_())saved|=stationRecipeBytes(candidate.selection)==stationRecipeBytes(snapshot->selection);if(!saved)throw std::runtime_error("Immutable recipe/template manifest unavailable or retired");}
    if(ledger_ && (ledger_->pending(id) || ledger_->pending(id,true)))throw std::runtime_error("PENDING_ACK — önce exact ACK / reconcile gerekir");
    if(command=="run")b->run->run(snapshot);
    else if(command=="auto" && b->automatic)b->automatic(true);
    else if(command=="manual" && b->automatic)b->automatic(false);
    else throw std::runtime_error("Üretim publication/dispatch portu bağlanmadı");
}
void CommunicationWorkspace::poll(){owner();for(auto& b:bindings_)b.run->poll();if(poll_)poll_();}
void CommunicationWorkspace::selectRecipe(const QUuid& id,const QUuid& expected,quint32 recipe,quint32 publication){owner();if(restartRequired_ || !select_ || !bindings_.contains(id))throw std::runtime_error("Saved recipe selection unavailable / restart required");select_(id,expected,recipe,publication);}
bool CommunicationWorkspace::drained() const {owner();for(const auto& b:bindings_)if(!b.run->drained())return false;return true;}
void CommunicationWorkspace::maintenance() const {owner();if(maintenanceBarrier_){const auto reason=maintenanceBarrier_();if(!reason.isEmpty())throw std::runtime_error(reason.toStdString());}if(!drained())throw std::runtime_error("Önce bütün station Runtime'larını Stop/drain yapın");if(!CameraLeaseArbiter::diagnostics().isEmpty())throw std::runtime_error("Kamera lease / drain tamamlanmadı");if(server_.state()==CommunicationServerState::Listening || server_.state()==CommunicationServerState::PeerConnected || server_.state()==CommunicationServerState::Closing)throw std::runtime_error("Listener açık: disabled profil ile yeniden başlatıp bakım yapın");}
CommunicationMigrationPlan CommunicationWorkspace::analyze(const CommunicationMigrationInput& in) const {owner();return CommunicationMigrationPlanner(in).analyze();}
void CommunicationWorkspace::machineInput(const CommunicationMigrationInput& in,const QString& work) const {
    if(!machineWorkspace_.isEmpty() && (QFileInfo(machineWorkspace_).canonicalFilePath().isEmpty() || QFileInfo(work).canonicalFilePath()!=QFileInfo(machineWorkspace_).canonicalFilePath()))throw std::runtime_error("Migration workspace must be the configured machine migration directory");
    if(!machineRegistry_.isEmpty() && QFileInfo(in.registryPath).canonicalFilePath()!=QFileInfo(machineRegistry_).canonicalFilePath())throw std::runtime_error("Migration must use the current machine catalogue authority");
}
QByteArray CommunicationWorkspace::prepare(const CommunicationMigrationInput& in,const QString& work,const QString& package,const QByteArray& review){maintenance();machineInput(in,work);CommunicationMigrationTransaction t(in,work);return t.prepare(package,review);}
void CommunicationWorkspace::activate(const CommunicationMigrationInput& in,const QString& work,const QString& package,const QByteArray& review,const QByteArray& pointer){maintenance();machineInput(in,work);CommunicationMigrationTransaction t(in,work);t.activate(package,review,pointer);restartRequired_=true;notice_="Migration seçim kaydı değişti. Run/Auto için yeniden başlatın.";}
void CommunicationWorkspace::rollback(const CommunicationMigrationInput& in,const QString& work,const QByteArray& pointer,const QByteArray& ledger){maintenance();machineInput(in,work);CommunicationMigrationTransaction t(in,work);t.rollback(pointer,ledger);restartRequired_=true;notice_="Rollback tamam. Live ledger korundu; eski kurulu sürümle yeniden başlatın.";}
QJsonObject CommunicationWorkspace::reviewRollback(const CommunicationMigrationInput& in,const QString& work) const {maintenance();CommunicationMigrationTransaction t(in,work);auto result=t.active();result["pointerDigest"]=QString::fromLatin1(t.pointerDigest());result["ledgerDigest"]=QString::fromLatin1(t.ledgerDigest());return result;}
}
