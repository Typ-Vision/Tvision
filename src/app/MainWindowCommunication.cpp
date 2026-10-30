#include "MainWindow.hpp"
#include <QFormLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QSettings>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTimer>
#include <QFileInfo>
#include <QTabWidget>
#include <QApplication>
#include <QDir>
#include "CommunicationPanel.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include "DeliveryDiagnosticsWidget.hpp"

namespace visionflow {
MultiSourceDependencies MainWindow::withPublication(const Recipe& r,MultiSourceDependencies d) const {
    d.publication=PublicationService::capture(r.definition,CommunicationProfiles(QFileInfo(settingsFile_).absolutePath()+"/communication-profiles.json"));return d;
}
void MainWindow::resetRuntimeTcp(){
    if(tcpEndpoint_ && (runtime_->busy() || tcpEndpoint_->recordingPending() || runtimeCoordinator_->deliveryPending()))throw std::runtime_error("TCP v3 kayıt/ACK bekleniyor.");
    tcpPublicationTargets_.clear();tcpEndpoint_.reset();tcpRouter_.reset();tcpLedger_.reset();tcpInputs_.reset();
}
void MainWindow::startRuntimeTcp(){
    if(!QCoreApplication::arguments().contains("--tcp-acceptance-smoke") && !QCoreApplication::arguments().contains("--publication-smoke"))return;
    if(!findChild<QCheckBox*>("runtimeTcpV3")->isChecked())return;
    if(plc_ && plc_->listening())throw std::runtime_error("TCP v3 için önce v2 dinlemesini durdurun.");
    if(runtime_->experimental() || runtimeCoordinator_->mode()!=RuntimeTrigger::SoftwareTrigger)throw std::runtime_error("TCP v3: kayıtlı SoftwareTrigger ve normal Runtime gerekir.");
    const auto snapshot=runtime_->active();if(!snapshot || !snapshot->publication())throw std::runtime_error("TCP v3: yüklü Publish Data profili yok.");
    QVector<OutputGroup> groups;for(const auto& t:editorTools(snapshot->recipe().definition))if(t.enabled && t.kind=="publish_data")for(const auto& id:decodePublishConfig(t.parameters).groups)for(const auto& g:recipeOutputGroups(snapshot->recipe().definition))if(g.id==id)groups.append(g);
    if(groups.isEmpty())throw std::runtime_error("TCP v3: etkin yayın grubu gerekir.");
    const auto group=groups.first();const auto profile=snapshot->publication()->profiles.value(group.profileId);
    if(!profile.enabled || profile.protocol!="tcp" || profile.role!="server")throw std::runtime_error("TCP v3: enabled TCP server profili gerekir.");
    CommunicationTcpOptions options;options.address=QHostAddress(profile.address);options.port=profile.port;options.ackTimeoutMs=profile.timeoutMs;options.retryCount=profile.retries;
    options.workcell="local";options.project=runtimeProjectId_;options.endpoint=profile.id;options.peer=findChild<QLineEdit*>("runtimeTcpPeer")->text().trimmed();options.outputGroupId=group.id;options.required=group.required;
    tcpInputs_=std::make_unique<InputSnapshotStore>();tcpLedger_=std::make_unique<DeliveryStore>(runtimePath_+".production");
    tcpRouter_=std::make_unique<TriggerRouter>(*runtimeCoordinator_,*runtime_,*tcpInputs_,*tcpLedger_,[this]{return editorSession_.busy() || cameraSession_.busy() || runtimeJobs_.busy()?QString("NOT_READY"):QString{};});
    tcpEndpoint_=std::make_unique<CommunicationTcpEndpoint>(*runtime_,*runtimeCoordinator_,*tcpRouter_,*tcpInputs_,*tcpLedger_,runtimePath_+".production",runtimePath_+".calibrations.json");
    tcpEndpoint_->started=[this]{runtimeResult_.reset();showRuntimeResult();refreshRuntime();};
    tcpEndpoint_->completed=[this](const CycleResult& r){if(measurementWorkspace_ && runtime_->active())measurementWorkspace_->observeAcquisition(runtime_->active()->recipe(),r);if(runtimeCoordinator_->state()==RuntimeState::Running)runtimeResult_=r;else runtimeResult_.reset();showRuntimeResult();refreshRuntime();};
    try{
        tcpEndpoint_->listen(options);
        QVector<CommunicationTcpEndpoint*> targets{tcpEndpoint_.get()};QSet<QString> ids{group.profileId};
        for(int index=1;index<groups.size();++index){
            const auto& g=groups[index];const auto p=snapshot->publication()->profiles.value(g.profileId);
            if(ids.contains(p.id) || !p.enabled || p.protocol!="tcp" || p.role!="server")throw std::runtime_error("TCP v3: her hedef için ayrı enabled TCP server profili gerekir.");
            ids.insert(p.id);auto o=options;o.address=QHostAddress(p.address);o.port=p.port;o.endpoint=p.id;o.outputGroupId=g.id;o.required=g.required;o.ackTimeoutMs=p.timeoutMs;o.retryCount=p.retries;o.inputs.clear();
            auto target=std::make_unique<CommunicationTcpEndpoint>(*runtime_,*runtimeCoordinator_,*tcpRouter_,*tcpInputs_,*tcpLedger_,runtimePath_+".production",runtimePath_+".calibrations.json");
            target->listen(o);targets.append(target.get());tcpPublicationTargets_.push_back(std::move(target));
        }
        if(targets.size()>1)tcpEndpoint_->setPublicationTargets(targets);
    }catch(...){tcpPublicationTargets_.clear();tcpEndpoint_.reset();throw;}
    if(deliveryDiagnostics_)deliveryDiagnostics_->bindEndpoint(tcpEndpoint_.get());
}

QWidget* MainWindow::makeCommunication() {
    communicationPanel_=new CommunicationPanel;
    communicationPanel_->legacyHistory=[this]{
        if(!service_.hasProject())return QString("Proje seçilmedi.");
        QJsonArray rows;
        for(const auto& r:DeliveryStore(service_.path()+".production").recovery())rows.append(QJsonObject{{"endpoint",r.request.endpoint},{"request",r.request.requestId},{"state",r.status.state},{"cycle",r.status.cycleId}});
        return QString::fromUtf8(QJsonDocument(rows).toJson());
    };
    return communicationPanel_;
}
void MainWindow::bindCommunication(CommunicationWorkspace* workspace){
    if(workspace)workspace->setMaintenanceBarrier([this]{
        if(editorSession_.busy() || cameraSession_.busy() || runtimeJobs_.busy() || editorCameras_.hasTargets()
           || (runtime_ && (runtime_->busy() || runtime_->camerasPending() || runtime_->camerasDraining()))
           || (runtimeCoordinator_ && runtimeCoordinator_->state()!=RuntimeState::Unloaded && runtimeCoordinator_->state()!=RuntimeState::LoadedStopped))return QString("Önce Editor / Calibration / Runtime işlerini Stop/drain yapın.");
        return QString{};
    });
    communicationPanel_->bind(workspace);
}
bool MainWindow::verifyEndpointProfiles(const QString& artifacts) {
    selectPage("communication");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();if(!grab().save(QDir(artifacts).filePath(QString("communication-%1x%2.png").arg(size.width()).arg(size.height()))))return false;}
    return communicationPanel_ && !findChild<QComboBox*>("endpointList") && !findChild<QPushButton*>("plc_listen") && !runtime_;
}
void MainWindow::refreshCommunication() {if(communicationPanel_)communicationPanel_->refresh();}
void MainWindow::initializeCommunication() {
    // Legacy settings are retained as history. They cannot start a hidden parser.
    plc_=std::make_unique<PlcRuntimeSocket>();
    refreshCommunication();
}
void MainWindow::communicationAction(const QString&) {displayError("Legacy listener UI kaldırıldı. Migration analizini kullanın.");}
}
