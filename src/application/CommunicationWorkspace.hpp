#pragma once
#include "CommunicationServerLifecycle.hpp"
#include "StationDeliveryLedger.hpp"
#include "CameraLeaseArbiter.hpp"
#include "CommunicationMigration.hpp"

namespace visionflow {
struct CommunicationStationView {
    StationDefinition station;
    StationRuntimeState runtime;
    std::shared_ptr<const StationSnapshot> snapshot;
    QString cameraOwners, publication, unavailable;
    bool automatic=false, canRun=false, canAuto=false;
};
// Owner-thread application port. Selection is deliberately absent: selecting a
// UI row cannot mutate a listener, activation, Runtime or delivery record.
class CommunicationWorkspace final {
public:
    struct Binding {
        StationDefinition station;
        StationRunController* run=nullptr;
        std::function<std::shared_ptr<const StationSnapshot>()> snapshot;
        std::function<void(bool)> automatic;
        QString publication, unavailable;
        std::function<QString()> unavailableReason;
    };
    CommunicationWorkspace(CommunicationServerLifecycle&,const CommunicationServerProfiles&,StationIdentityStore&);
    void bind(Binding);
    void setLedger(StationDeliveryLedger* ledger){ledger_=ledger;}
    void setMaintenanceBarrier(std::function<QString()> barrier){maintenanceBarrier_=std::move(barrier);}
    void setMachinePaths(QString workspace,QString registry){machineWorkspace_=std::move(workspace);machineRegistry_=std::move(registry);}
    using RecipeList=std::function<QVector<StationSavedRecipe>()>;
    void setRecipeControl(RecipeList recipes,std::function<void(const QUuid&,const QUuid&,quint32,quint32)> select){recipes_=std::move(recipes);select_=std::move(select);}
    QVector<StationSavedRecipe> recipes() const {owner();return recipes_?recipes_():QVector<StationSavedRecipe>{};}
    void selectRecipe(const QUuid&,const QUuid&,quint32 recipeKey,quint32 templateKey);
    void setPoll(std::function<void()> poll){poll_=std::move(poll);}
    void setNotice(QString notice){notice_=std::move(notice);}
    void setRemoteControl(std::function<bool()> read,std::function<void(bool)> write){remoteRead_=std::move(read);remoteWrite_=std::move(write);}
    bool remoteEnabled() const {return remoteRead_ && remoteRead_();}
    void setRemoteEnabled(bool enabled){owner();if(restartRequired_ || !remoteWrite_)throw std::runtime_error("Remote control unavailable / restart required");remoteWrite_(enabled);}
    QVector<CommunicationStationView> stations() const;
    QString listener() const;
    QString history() const;
    CommunicationServerProfile profile() const;
    void saveProfile(const CommunicationServerProfile&);
    void command(const QUuid&,const QUuid& expectedActivation,const QString&);
    void poll();
    bool drained() const;
    static QString modeName(StationMode);
    // Read-only analysis is always allowed. Transactional mutations require all
    // registered production owners drained; the transaction also fences sockets.
    CommunicationMigrationPlan analyze(const CommunicationMigrationInput&) const;
    QByteArray prepare(const CommunicationMigrationInput&,const QString& workspace,const QString& package,const QByteArray& reviewed);
    void activate(const CommunicationMigrationInput&,const QString& workspace,const QString& package,const QByteArray& reviewed,const QByteArray& pointer);
    void rollback(const CommunicationMigrationInput&,const QString& workspace,const QByteArray& pointer,const QByteArray& ledger);
    QJsonObject reviewRollback(const CommunicationMigrationInput&,const QString& workspace) const;
private:
    void owner() const;
    void maintenance() const;
    void machineInput(const CommunicationMigrationInput&,const QString&) const;
    CommunicationServerLifecycle& server_;
    const CommunicationServerProfiles& profiles_;
    StationIdentityStore& identities_;
    StationDeliveryLedger* ledger_=nullptr;
    QMap<QUuid,Binding> bindings_;
    std::function<QString()> maintenanceBarrier_;
    QString machineWorkspace_,machineRegistry_;
    RecipeList recipes_;
    std::function<void(const QUuid&,const QUuid&,quint32,quint32)> select_;
    std::function<void()> poll_;
    QString notice_;
    bool restartRequired_=false;
    std::function<bool()> remoteRead_;
    std::function<void(bool)> remoteWrite_;
    QThread* owner_=QThread::currentThread();
};
}
