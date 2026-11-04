#include "CommunicationWorkspace.hpp"
#include "CommunicationClientProfiles.hpp"
#include "StationIdentityRegistry.hpp"
#include "MachineSetup.hpp"
#include "StationSourceSetup.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <algorithm>

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
            if(v.snapshot && recipes_){bool saved=false;for(const auto& candidate:recipes_())saved|=stationManifestMatches(candidate.selection,v.snapshot->selection);if(!saved){v.canRun=false;v.canAuto=false;v.unavailable="Immutable recipe/template manifest unavailable or retired";}}
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
        v.cameraOwners=owners.isEmpty()?"Kamera ataması yok":owners.join('\n');
        for(const auto& source:catalogue.stationSources)if(source.station==v.station.id){
            v.source=source;QString cameraName;
            for(const auto& e:catalogue.entries)if(e.identity.category==CatalogueCategory::Camera && e.identity.entity==source.camera)cameraName=e.name;
            v.cameraOwners=cameraName+" / "+source.camera.toString(QUuid::WithoutBraces)+"\n"+source.path;
            if(!QFileInfo(source.path).isFile() || !QFileInfo(source.path).isReadable()){v.unavailable="Kaynak okunamıyor. Kaynak ayarları ile dosyayı seçin.";v.canRun=false;v.canAuto=false;}
        }
        if(!v.source){v.unavailable+="\nKalıcı kamera eşleşmesi yok; Kaynak ayarları ile açıkça eşleyin.";}
        if(restartRequired_)v.unavailable+="\nKaynak kaydedildi; Runtime için uygulamayı yeniden başlatın.";
        rows.append(v);
    }
    return rows;
}
QString CommunicationWorkspace::machineDirectory() const {
    if(machineWorkspace_.isEmpty())return {};
    return QFileInfo(machineWorkspace_).absolutePath();
}
QString CommunicationWorkspace::setupStatus() const {owner();return MachineSetup(machineDirectory()).status();}
QVector<CommunicationClientView> CommunicationWorkspace::clients() const {
    owner();const auto cat=identities_.load();validateIdentityCatalogue(cat);
    QVector<CommunicationClientView> result;
    for(const auto& e:cat.epochs)result.append({e.peer,e.epoch,!e.retired});
    return result;
}
CommunicationClientView CommunicationWorkspace::enrollClient(){
    owner();const auto cat=identities_.load();validateIdentityCatalogue(cat);
    if(!drained())throw std::runtime_error("Önce bütün station Runtime'larını Stop/drain yapın");
    if(remoteEnabled())throw std::runtime_error("Remote Enable kapatılmadan istemci kaydı değiştirilemez");
    if(ledger_)for(const auto& r:ledger_->recovery())if(!r.value.acknowledged)
        throw std::runtime_error("PENDING ACK: bekleyen teslim varken istemci kimliği değiştirilemez; exact ACK / reconcile gerekir");
    if(std::count_if(cat.epochs.cbegin(),cat.epochs.cend(),[](const auto& e){return !e.retired;})!=0)
        throw std::runtime_error("Etkin istemci var. İkinci etkin kimlik kaydedilemez; önce 'Epoch yenile' ile mevcut istemciyi emekliye ayırın.");
    const QUuid peer=QUuid::createUuid(),epoch=QUuid::createUuid();
    StationIdentityRegistry(identities_).enrollControllingEpoch(cat.revision,peer,epoch);
    return {peer,epoch,true};
}
CommunicationClientView CommunicationWorkspace::rolloverClient(){
    owner();const auto cat=identities_.load();validateIdentityCatalogue(cat);
    if(!drained())throw std::runtime_error("Önce bütün station Runtime'larını Stop/drain yapın");
    if(remoteEnabled())throw std::runtime_error("Remote Enable kapatılmadan epoch değiştirilemez");
    if(ledger_)for(const auto& r:ledger_->recovery())if(!r.value.acknowledged)
        throw std::runtime_error("PENDING ACK: bekleyen teslim varken epoch değiştirilemez; exact ACK / reconcile gerekir");
    const PeerEpochIdentity* active=nullptr;
    for(const auto& e:cat.epochs){if(!e.retired){if(active)throw std::runtime_error("Birden fazla etkin istemci; önce katalog onarımı gerekir");active=&e;}}
    if(!active)throw std::runtime_error("Etkin istemci yok; önce 'Yetkili istemci kaydet' kullanın");
    const auto peer=active->peer;const QUuid epoch=QUuid::createUuid();
    StationIdentityRegistry(identities_).replaceControllingEpoch(cat.revision,peer,epoch);
    return {peer,epoch,true};
}
QByteArray CommunicationWorkspace::exportClientProfile(const QString& label) const {
    owner();const auto cat=identities_.load();validateIdentityCatalogue(cat);
    const auto listener=profiles_.load();
    if(!listener)throw std::runtime_error("Önce listener profilini kaydedin; test profili bağlantı bilgisi taşır");
    const PeerEpochIdentity* active=nullptr;int count=0;
    for(const auto& e:cat.epochs)if(!e.retired){++count;active=&e;}
    if(count!=1 || !active)throw std::runtime_error("Test profili için tek etkin istemci gerekir; önce istemci kaydedin");
    CommunicationClientProfile profile{label,cat.installation,active->peer,active->epoch,
        CommunicationPeerSessionService::contractRevision(),listener->address,listener->port};
    return exportCommunicationClientProfile(cat,profile);
}
void CommunicationWorkspace::checkClientProfile(const QByteArray& bytes) const {
    owner();const auto cat=identities_.load();validateIdentityCatalogue(cat);
    validateCommunicationClientProfile(readCommunicationClientProfile(bytes),cat);
}
void CommunicationWorkspace::startSetup(){owner();if(restartRequired_)throw std::runtime_error("Migration sonrası yeniden başlatın.");MachineSetup(machineDirectory()).start();if(setupCompleted_)setupCompleted_();}
void CommunicationWorkspace::saveStationSource(const QUuid& expected,const QUuid& station,const QString& name,const QUuid& camera,const QString& cameraName,const QString& path){
    owner();if(!drained() || remoteEnabled())throw std::runtime_error("Önce Stop/drain yapın ve Remote Enable kapatın.");
    if(ledger_)for(const auto& r:ledger_->recovery())if(!r.value.acknowledged && r.key.station==station)throw std::runtime_error("PENDING ACK: kaynak değiştirilemez.");
    if(maintenanceBarrier_){const auto why=maintenanceBarrier_();if(!why.isEmpty())throw std::runtime_error(why.toStdString());}
    StationSourceSetup(identities_).save(expected,station,name,camera,cameraName,path);
    restartRequired_=true;notice_="İstasyon / kaynak kaydedildi. Runtime için uygulamayı yeniden başlatın; Run / Auto kapalı.";
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
    if(command=="stop"){if(batchActive_)cancelAutomaticProduction("Operatör Stop verdi; toplu geçiş iptal edildi.");b->run->stop();return;}
    if(batchActive_ || stopping_)throw std::runtime_error("Otomatik üretim geçişi sürüyor; önce Stop verin");
    if(restartRequired_)throw std::runtime_error("Migration selection changed: restart required");
    if(b->unavailableReason){const auto reason=b->unavailableReason();if(!reason.isEmpty())throw std::runtime_error(reason.toStdString());}
    if(!ledger_)throw std::runtime_error("Durable ledger bağlanmadı");
    const auto snapshot=b->snapshot?b->snapshot():nullptr;
    if(!snapshot || snapshot->activation!=expected)throw std::runtime_error("STALE_ACTIVATION");
    if(recipes_){bool saved=false;for(const auto& candidate:recipes_())saved|=stationManifestMatches(candidate.selection,snapshot->selection);if(!saved)throw std::runtime_error("Immutable recipe/template manifest unavailable or retired");}
    if(ledger_ && (ledger_->pending(id) || ledger_->pending(id,true)))throw std::runtime_error("PENDING_ACK — önce exact ACK / reconcile gerekir");
    if(command=="run")b->run->run(snapshot);
    else if(command=="auto" && b->automatic)b->automatic(true);
    else if(command=="manual" && b->automatic)b->automatic(false);
    else throw std::runtime_error("Üretim publication/dispatch portu bağlanmadı");
}
void CommunicationWorkspace::startAutomaticProduction(){
    owner();if(stopping_)throw std::runtime_error("Önce durdurmanın tamamlanmasını bekleyin");if(batchActive_)throw std::runtime_error("Otomatik üretim geçişi zaten sürüyor");
    const auto rows=stations();QStringList problems;
    if(rows.isEmpty())problems<<"İstasyon yok → Communication / İstasyon ekle";
    for(const auto& row:rows){
        const auto mode=row.runtime.mode;
        QString reason=row.unavailable;
        if(!row.source)reason+=" Kalıcı kamera eşleşmesi yok → Kaynak ayarları";
        if(!row.snapshot)reason+=" Reçete yükleyin → Dashboard / Reçete yükle";
        if(row.runtime.deliveryPending)reason+=" PENDING ACK → Communication / exact ACK veya reconcile";
        if(!row.canAuto && reason.isEmpty())reason="Run/Auto portu hazır değil → Communication";
        if(mode!=StationMode::Stopped && mode!=StationMode::ArmedManual && mode!=StationMode::ArmedAuto)
            reason+=" Çalışma/hazırlık tamamlanmalı → Dashboard / Stop";
        if(!reason.trimmed().isEmpty())problems<<row.station.name+": "+reason;
    }
    if(!problems.isEmpty()){batchStatus_="Başlatılamadı:\n"+problems.join('\n');throw std::runtime_error(batchStatus_.toStdString());}
    batch_.clear();for(const auto& row:rows)batch_.append({row.station.id,row.snapshot->activation,false,false,row.automatic,bindings_[row.station.id].run->controlGeneration()});
    batchIndex_=0;batchActive_=true;batchRollback_=false;batchEnabling_=false;batchTimer_.start();
    batchStatus_="İstasyonlar sırayla hazırlanıyor; Remote Enable bağımsızdır.";
    advanceAutomaticProduction();
}
void CommunicationWorkspace::cancelAutomaticProduction(const QString& reason){
    if(!batchActive_ || batchRollback_)return;
    batchRollback_=true;batchStatus_=reason+"\nBu işlemle başlatılan istasyonlar durduruluyor; pending sonuç korunur.";
    for(const auto& target:batch_){auto& b=bindings_[target.station];
        if(target.started)b.run->stop();
        else if(target.changed && !target.wasAutomatic){if(b.automatic)b.automatic(false);}
    }
}
void CommunicationWorkspace::advanceAutomaticProduction(){
    if(!batchActive_)return;
    if(batchRollback_){for(const auto& target:batch_)if(target.started && !bindings_[target.station].run->drained())return;
        batchActive_=false;batchStatus_+="\nDurdurma tamamlandı. Nedeni düzeltip yeniden başlatın.";return;}
    try{
        const auto rows=stations();
        for(const auto& target:batch_){
            auto it=std::find_if(rows.cbegin(),rows.cend(),[&](const auto& row){return row.station.id==target.station;});
            if(it==rows.cend() || !it->snapshot || it->snapshot->activation!=target.activation || !it->source || !it->canAuto || !it->unavailable.isEmpty())
                throw std::runtime_error((bindings_[target.station].station.name+": Hazırlık değişti; kaynak / reçete / pending teslimi Communication ekranında denetleyin.").toStdString());
        }
        for(int n=0;n<batch_.size();++n)if((n<batchIndex_ || batch_[n].wasAutomatic) && !bindings_[batch_[n].station].run->automatic())
            throw std::runtime_error("Önceden hazırlanan istasyonun Auto izni değişti → Dashboard / Stop");
        if(batchTimer_.elapsed()>30000)throw std::runtime_error("Hazırlık zaman aşımı → Dashboard / Stop ve kaynak ayarları");
        if(batchIndex_==batch_.size()){batchActive_=false;batchStatus_="Otomatik üretim açık. Tüm istasyonlarda Run / Auto doğrulandı. Remote Enable bağımsızdır.";return;}
        auto& target=batch_[batchIndex_];auto& b=bindings_[target.station];
        if(target.wasAutomatic){++batchIndex_;return;}
        auto mode=b.run->state().mode;
        if(!target.changed){
            target.changed=true;
            if(mode==StationMode::Stopped){target.started=true;b.run->run(b.snapshot());}
            target.generation=b.run->controlGeneration();
            batchStatus_=b.station.name+": hazırlanıyor";return;
        }
        if(b.run->controlGeneration()!=target.generation && !(batchEnabling_ && b.run->automatic() && b.run->controlGeneration()==target.generation+1))
            throw std::runtime_error((b.station.name+": Çalışma izni değişti → Dashboard / Stop").toStdString());
        mode=b.run->state().mode;
        if(mode==StationMode::Preparing)return;
        if(mode!=StationMode::ArmedManual && mode!=StationMode::ArmedAuto)throw std::runtime_error((b.station.name+": Hazırlık başarısız: "+b.run->state().health+" → Kaynak ayarları / Reçete yükle").toStdString());
        if(!batchEnabling_){b.automatic(true);batchEnabling_=true;return;}
        if(!b.run->automatic())return;
        ++batchIndex_;batchEnabling_=false;
    }catch(const std::exception& e){cancelAutomaticProduction(QString::fromUtf8(e.what()));}
}
void CommunicationWorkspace::stopAll(){
    owner();batchActive_=false;batch_.clear();batchRollback_=false;batchEnabling_=false;
    stopping_=true;stopErrors_.clear();batchStatus_="Durduruluyor";
    for(auto& b:bindings_)try{b.run->stop();}catch(const std::exception& e){stopErrors_<<b.station.name+": "+QString::fromUtf8(e.what());}
    try{for(const auto& row:stations())if(!bindings_.contains(row.station.id))stopErrors_<<row.station.name+": Runtime bağlanmadı; yeniden başlatın.";}catch(const std::exception& e){stopErrors_<<QString::fromUtf8(e.what());}
    finishStop();
}
void CommunicationWorkspace::finishStop(){
    if(!stopping_)return;
    if(!stopErrors_.isEmpty()){stopping_=false;batchStatus_="Durdurma tamamlanamadı: "+stopErrors_.join("; ");return;}
    for(const auto& b:bindings_)if(!b.run->drained())return;
    stopping_=false;batchStatus_="Durduruldu";
    if(!ledger_)batchStatus_+=" · teslim durumu bilinmiyor";
    else for(const auto& b:bindings_)if(ledger_->pending(b.station.id) || ledger_->pending(b.station.id,true)){batchStatus_+=" · ACK bekliyor";break;}
}
void CommunicationWorkspace::poll(){
    owner();QStringList errors;
    for(auto& b:bindings_)try{b.run->poll();}catch(const std::exception& e){errors<<b.station.name+": "+QString::fromUtf8(e.what());}
    try{if(poll_)poll_();}catch(const std::exception& e){errors<<QString::fromUtf8(e.what());}
    if(!errors.isEmpty()){
        if(stopping_)stopErrors_.append(errors);
        else if(batchActive_)cancelAutomaticProduction(errors.join("; "));
        else throw std::runtime_error(errors.join("; ").toStdString());
    }
    advanceAutomaticProduction();finishStop();
}
bool CommunicationWorkspace::canSelectRecipe(const QUuid& id) const{
    owner();const auto b=bindings_.constFind(id);
    return !batchActive_ && !stopping_ && !restartRequired_ && b!=bindings_.cend() && b->run->drained() && ledger_ && !ledger_->pending(id) && !ledger_->pending(id,true);
}
QString CommunicationWorkspace::recipeReadiness(const QUuid& id) const{
    owner();const auto b=bindings_.constFind(id);
    if(b==bindings_.cend() || restartRequired_)return "Runtime bağlanmadı; programı yeniden açın";
    if(batchActive_ || stopping_ || !b->run->drained())return "Yüklemek için önce Dashboard → Stop; durdurmanın tamamlanmasını bekleyin";
    if(!ledger_ || ledger_->pending(id) || ledger_->pending(id,true))return "Bekleyen teslimi exact ACK / reconcile ile tamamlayın";
    return "Yüklemeye hazır";
}
void CommunicationWorkspace::selectRecipe(const QUuid& id,const QUuid& expected,quint32 recipe,quint32 publication){
    owner();if(!canSelectRecipe(id))throw std::runtime_error(recipeReadiness(id).toStdString());
    if(!select_)throw std::runtime_error("Saved recipe selection unavailable / restart required");
    select_(id,expected,recipe,publication);
}
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
