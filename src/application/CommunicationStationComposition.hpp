#pragma once
#include "CommunicationWorkspace.hpp"
#include "StationExecutionRegistry.hpp"
#include "StationRuntimeAdmissionBoundary.hpp"
#include "persistence/StationActivationStore.hpp"
#include <QDir>

namespace visionflow {
// The normal entry point and software integration tests use this same owner.
// It never restores Run/Auto. Hardware lacking normal production providers
// remains unavailable; a file-source manifest is not physical acceptance.
class CommunicationStationComposition final {
public:
    CommunicationStationComposition(QString machineDirectory,QJsonObject selection,
        StationIdentityStore&,CommunicationPeerSessionService&);
    ~CommunicationStationComposition();
    void attach(CommunicationServerLifecycle&,CommunicationWorkspace&);
    void poll();
    static QJsonObject restoreSelection(const QString& machineDirectory);
private:
    struct Station {
        StationDefinition definition;
        std::unique_ptr<RuntimeService> runtime;
        std::unique_ptr<RuntimeCoordinator> coordinator;
        std::unique_ptr<StationRunController> run;
    };
    std::optional<StationSavedRecipe> recipe(quint32) const;
    std::optional<StationAdmissionSnapshot> manifest(const QUuid&) const;
    void complete(const StationAdmissionIntent&,const ProductionCycle&,const CycleResult&);
    QDir directory_;QJsonObject selection_;
    StationIdentityStore& identities_;CommunicationPeerSessionService& sessions_;
    StationRuntimeBoundary boundary_;
    FileStationActivationStore selectionStore_;
    StationActivationService activations_;
    std::unique_ptr<StationDeliveryLedger> ledger_;
    std::unique_ptr<StationDeliveryLedger::Admission> durableAdmission_;
    std::unique_ptr<StationDeliveryLedger::Control> durableControl_;
    std::unique_ptr<StationControlService> controls_;
    std::vector<std::unique_ptr<Station>> stations_;
    QMap<quint32,StationSavedRecipe> recipes_;
    QMap<quint32,PublicationTemplate> templates_;
    StationExecutionRegistry execution_;
    std::unique_ptr<StationRuntimeAdmissionBoundary> admissionBoundary_;
    std::unique_ptr<StationAdmissionService> admission_;
    QVector<V4RequestIdentity> completed_;
    QString error_;
    bool remoteEnabled_=false;
};
}
