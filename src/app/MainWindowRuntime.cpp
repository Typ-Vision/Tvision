#include "application/EditorObservation.hpp"
#include "application/MeasurementDiagnostic.hpp"
#include "application/CircleToolService.hpp"
#include "application/CalibrationWorkflow.hpp"
#include "MainWindow.hpp"
#include "WorkspacePanel.hpp"
#include "ImageViewer.hpp"
#include "EdgeProfileView.hpp"
#include "application/ToolResultCodec.hpp"
#include "application/ProductionResultCodec.hpp"
#include "application/SourceStatusPresenter.hpp"
#include "application/IndustrialRuntime.hpp"
#include "application/ExperimentalRuntime.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include "application/ExperimentalOecsAcquisition.hpp"
#include <QCheckBox>
#include "application/ReferenceService.hpp"
#include <QFileDialog>
#include <QSaveFile>
#include <QComboBox>
#include <QFileInfo>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QTableWidget>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QMenuBar>
#include <QListWidget>
#include <QStackedWidget>
namespace visionflow {
namespace {
QLabel* plain(const QString& value){auto* label=new QLabel(value);label->setWordWrap(true);label->setTextFormat(Qt::PlainText);return label;}
}
QWidget* MainWindow::makeRuntime() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);
    layout->addWidget(plain("Load Recipe → Run → Tek çevrim. Kamera, ayar veya model değiştiğinde Stop → yeniden Load Recipe → Run. Fiziksel kabul ayrıdır."));
    runtimeStatus_=plain("Aktif sürüm yok; çevrim çalıştırılmadı.");runtimeStatus_->setObjectName("runtimeStatus");layout->addWidget(runtimeStatus_);
    auto* actions=new QHBoxLayout;
    const QVector<QPair<QString,QString>> commands{{"run","Run"},{"stop","Stop"},{"cycle","Tek çevrim"},{"retry","Kaydı tekrar yaz"}};
    for(const auto& command:commands){auto* b=new QPushButton(command.second);b->setObjectName("runtime_"+command.first);actions->addWidget(b);connect(b,&QPushButton::clicked,this,[this,action=command.first]{runtimeAction(action);});}
    layout->addLayout(actions);
    runtimeSources_=new QComboBox;runtimeSources_->setObjectName("runtimeSourceSelector");runtimeSources_->setAccessibleName("Runtime görüntü kaynağı");layout->addWidget(runtimeSources_);
    runtimePreparation_=new QTableWidget;runtimePreparation_->setObjectName("runtimePreparation");runtimePreparation_->setColumnCount(4);runtimePreparation_->setHorizontalHeaderLabels({"Kaynak / kamera","Kalibrasyon / birim","Hazırlık","Gerçek kare"});runtimePreparation_->setEditTriggers(QAbstractItemView::NoEditTriggers);runtimePreparation_->setSelectionBehavior(QAbstractItemView::SelectRows);runtimePreparation_->setSelectionMode(QAbstractItemView::SingleSelection);runtimePreparation_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);runtimePreparation_->setMinimumHeight(130);runtimePreparation_->setMaximumHeight(170);layout->addWidget(runtimePreparation_);
    auto* history=new QPushButton("Logs / CSV");history->setObjectName("runtimeHistory");connect(history,&QPushButton::clicked,this,[this]{selectPage("logs");refreshLogs();});actions->addWidget(history);
    connect(runtimeSources_,&QComboBox::currentIndexChanged,this,[this]{showRuntimeResult();});
    connect(runtimePreparation_,&QTableWidget::currentCellChanged,this,[this](int row,int,int,int){if(row>=0 && row<runtimeSources_->count())runtimeSources_->setCurrentIndex(row);});
    auto* split=new QSplitter;runtimeViewer_=new ImageViewer;split->addWidget(runtimeViewer_);
    auto* panel=new QWidget;auto* results=new QVBoxLayout(panel);runtimeTools_=new QComboBox;runtimeTools_->setObjectName("runtimeTools");results->addWidget(runtimeTools_);
    runtimeSummary_=plain("Yeni çevrim sonucu yok.");runtimeSummary_->setObjectName("runtimeSummary");runtimeSummary_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);results->addWidget(runtimeSummary_);runtimeEdgeProfile_=new EdgeProfileView;runtimeEdgeProfile_->setObjectName("runtimeEdgeProfile");results->addWidget(runtimeEdgeProfile_);results->addStretch();
    runtimeTools_->setMinimumContentsLength(12);runtimeTools_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setWidget(panel);scroll->setMinimumWidth(250);split->addWidget(scroll);split->setStretchFactor(0,3);split->setStretchFactor(1,2);split->setSizes({680,420});layout->addWidget(split,1);
    connect(runtimeTools_,&QComboBox::currentIndexChanged,this,[this]{showRuntimeResult();});return page;
}
QWidget* MainWindow::makeLogs() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);logStatus_=plain("Kalıcı çevrim kaydı yok.");layout->addWidget(logStatus_);
    auto* back=new QPushButton("Runtime'a dön");back->setObjectName("returnToRuntime");layout->addWidget(back);connect(back,&QPushButton::clicked,this,[this]{selectPage("runtime");});
    auto* refresh=new QPushButton("Kayıtları yenile");refresh->setObjectName("refreshLogs");layout->addWidget(refresh);connect(refresh,&QPushButton::clicked,this,[this]{refreshLogs();});
    auto* exportCsv=new QPushButton("Seçili kayıt türünü dışa aktar (CSV)");exportCsv->setObjectName("exportLogsCsv");layout->addWidget(exportCsv);
    connect(exportCsv,&QPushButton::clicked,this,[this]{
        if(!service_.hasProject() || !logTable_->item(logTable_->currentRow(),0)){logStatus_->setText("Önce bir kayıt seçin.");return;}
        const auto path=QFileDialog::getSaveFileName(this,"Çevrim kanıtı",{},"CSV (*.csv)");if(path.isEmpty())return;
        try {exportLogCsv(path);logStatus_->setText("Çevrim kanıtı CSV kaydedildi.");}
        catch(const std::exception& e){logStatus_->setText(QString::fromUtf8(e.what()));}
    });
    logTable_=new QTableWidget;logTable_->setColumnCount(5);logTable_->setHorizontalHeaderLabels({"UTC / Cycle","Reçete","Execution","Sonuç / birim","İletişim"});logTable_->horizontalHeaderItem(1)->setToolTip("Reçete / sürüm");logTable_->horizontalHeaderItem(3)->setToolTip("Kaynak / sonuç / birim");logTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);logTable_->setSelectionBehavior(QAbstractItemView::SelectRows);logTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);layout->addWidget(logTable_,1);
    auto* detail=new QTextEdit;detail->setReadOnly(true);detail->setObjectName("logDetail");layout->addWidget(detail,1);
    connect(logTable_,&QTableWidget::currentCellChanged,this,[this,detail](int row,int,int,int){const auto* item=logTable_->item(row,0);detail->setPlainText(item ? item->data(Qt::UserRole).toString() : QString{});});return page;
}
void MainWindow::exportLogCsv(const QString& path) {
    const auto* selected=logTable_->item(logTable_->currentRow(),0);
    if(!service_.hasProject() || !selected)throw std::runtime_error("Önce bir kayıt seçin.");
    const ProductionStore store(service_.path()+".production");
    const auto data=selected->data(Qt::UserRole+1).toBool() ? ExperimentalRuntime::csv(store.recent("experimental_cycles")):ProductionResultCodec::csv(store.recent("cycles"));
    QSaveFile file(path);file.setDirectWriteFallback(false);
    if(!file.open(QIODevice::WriteOnly) || file.write(data)!=data.size() || !file.commit())throw std::runtime_error("CSV atomik kayıt başarısız.");
}
IndustrialExecutionInputs MainWindow::runtimeCameraInputs() const{
    if(!measurementWorkspace_)throw std::runtime_error("Measurement workspace unavailable");
    auto inputs=runtimeCameraComposition_.value_or(IndustrialExecutionInputs{});inputs.contract=measurementWorkspace_->environment().current;
    for(const auto& c:inputs.contract.cameras)if(c.physical)if(const auto route=CameraLocalConfiguration(settingsFile_).route(*c.physical))inputs.routes[c.id.value]=*route;
    return inputs;
}
void MainWindow::ensureRuntime() {
    if(!service_.hasProject() || service_.path().isEmpty())throw std::runtime_error("Önce kaydedilmiş bir proje açın.");
    if(runtime_ && runtimePath_==service_.path())return;
    if(plc_ && plc_->blocking())throw std::runtime_error("Önce PLC kaydı/ACK tamamlanmalı.");
    if(runtimeJobs_.busy() || (runtime_ && runtime_->busy()))throw std::runtime_error("Önce Runtime çevrimini/kaydını tamamlayın.");
    if(plc_)plc_->bind(nullptr,nullptr);runtimeCoordinator_.reset();runtimePath_=service_.path();runtime_=std::make_unique<RuntimeService>(runtimePath_+".production",runtimePath_+".calibrations.json",RuntimeIdentity::localEngineer(),[this]{return systemCalibrationSnapshot();},[this](const Recipe& r){return systemCalibrationSnapshot(&r);},[this](const Recipe& r){if(!measurementWorkspace_)throw std::runtime_error("Measurement workspace unavailable");return measurementWorkspace_->acquisition(r);},[this](const Recipe& r){return measurementWorkspace_->productionSources(r,runtimeCameraInputs());});
    runtime_->setAutomaticAcquisition([this](const Recipe& r){
        if(!measurementWorkspace_)throw std::runtime_error("Measurement workspace unavailable");
        const auto inputs=runtimeCameraInputs();
        return measurementWorkspace_->productionSources(r,inputs,true);
    });
    runtimeCoordinator_=std::make_unique<RuntimeCoordinator>(*runtime_);
    if(plc_)plc_->bind(runtime_.get(),runtimeCoordinator_.get(),runtimePath_+".production",runtimePath_+".calibrations.json");
    runtimeCycle_={};runtimePending_.reset();runtimeResult_.reset();showRuntimeResult();
}
void MainWindow::updateOperatorMode(bool active) {
    if(operatorMode_==active)return;
    operatorMode_=active;
    if(active) {
        operatorGeometry_=saveGeometry();operatorWindowState_=windowState();
        pages_->setCurrentIndex(pageIds_.indexOf("runtime"));pageTitle_->setText("Runtime");
        navigation_->setEnabled(false);workspacePanel_->setAvailable(false);
        menuBar()->setEnabled(false);menuBar()->hide();showFullScreen();
    } else {
        menuBar()->setEnabled(true);menuBar()->show();
        navigation_->setEnabled(true);workspacePanel_->setAvailable(true);
        restoreGeometry(operatorGeometry_);setWindowState(operatorWindowState_);
        selectPage("runtime");
    }
}
void MainWindow::refreshRuntime(bool notification) {
    if(!runtimeStatus_)return;
    if(runtime_ && runtimePath_==service_.path() && service_.activeRecipe())runtime_->observeInstallation(*service_.activeRecipe());
    if(runtime_ && runtimePath_!=service_.path() && !runtimeJobs_.busy() && !runtime_->busy() && !(plc_ && plc_->blocking())) {
        if(plc_)plc_->bind(nullptr,nullptr);runtimeCoordinator_.reset();runtime_.reset();runtimePath_.clear();runtimeResult_.reset();showRuntimeResult();
    }
    // Installation/snapshot invalidation and project retirement are never deferred.
    // Only a workspace-originated visual rebuild waits for this page to be shown.
    if(notification)runtimeRefresh_.invalidate("runtime notification / page entry");
    if(!notification && pages_->currentIndex()!=pageIds_.indexOf("runtime"))return;
    editor_observation::Scope observation(editor_observation::Runtime);
    runtimeRefresh_.rendered();
    const auto state=runtimeCoordinator_ ? runtimeCoordinator_->state() : RuntimeState::Unloaded;
    const bool network=plc_ && plc_->listening(),working=runtimeJobs_.busy() || (plc_ && plc_->working()),pending=runtime_ && runtime_->busy();
    updateOperatorMode(state==RuntimeState::Starting || state==RuntimeState::Running || state==RuntimeState::Stopping || working);
    QString status;
    switch(state){
    case RuntimeState::Unloaded:status="Yüklü reçete yok — Recipe Manager → Load Recipe";break;
    case RuntimeState::LoadedStopped:status="Durduruldu — Run bekleniyor";break;
    case RuntimeState::Starting:status="Hazırlanıyor";break;
    case RuntimeState::Running:status=working ? "Çalışıyor — çevrim sürüyor" : (runtimeCoordinator_->mode()==RuntimeTrigger::Manual ? "Çalışıyor — manuel çevrim bekleniyor" : "Çalışıyor — yazılım tetiği bekleniyor");break;
    case RuntimeState::Stopping:status="Durduruluyor — kamera/kayıt bekleniyor";break;
    case RuntimeState::Fault:status="FAULT — "+runtimeCoordinator_->error();if(runtimeCoordinator_->loaded())for(const auto& t:editorTools(runtimeCoordinator_->loaded()->definition))if(t.enabled && t.kind=="industrial_camera"){status="FAULT — "+acquisitionStatus("ERROR",runtimeCoordinator_->error());break;}break;
    }
    if(runtimeCoordinator_ && runtimeCoordinator_->loaded()){
        const auto& r=*runtimeCoordinator_->loaded();status+="\nYüklü: "+r.name+QString(" • v%1 • %2").arg(r.recipeVersion).arg(typeLabel(r.applicationType));
    }
    if(runtimeCoordinator_ && runtimeCoordinator_->recordingPending() && !working)status+="\nKAYIT BEKLİYOR — yeni çevrim engelli";
    if(runtimeCoordinator_ && !runtimeCoordinator_->error().isEmpty()){if(state!=RuntimeState::Fault)status+="\n"+runtimeCoordinator_->error();status+="\n"+runtimeCoordinator_->recovery();}
    if(network)status+="\nPLC: "+plc_->state();
    runtimeStatus_->setText(status);
    refreshRuntimeSources();
    const bool available=!working && !pending && !(plc_ && plc_->blocking()) && !editorSession_.busy() && !cameraSession_.busy();
    findChild<QPushButton*>("runtime_run")->setEnabled(available && (state==RuntimeState::LoadedStopped || state==RuntimeState::Fault));
    findChild<QPushButton*>("runtime_cycle")->setEnabled(available && state==RuntimeState::Running && runtimeCoordinator_->mode()==RuntimeTrigger::Manual);
    findChild<QPushButton*>("runtime_stop")->setEnabled(state==RuntimeState::Starting || state==RuntimeState::Running || working);
    findChild<QPushButton*>("runtime_retry")->setVisible(runtimePending_.has_value() || (plc_ && plc_->recordingPending()));
    findChild<QPushButton*>("runtime_retry")->setEnabled(!working && ((pending && runtimePending_.has_value()) || (plc_ && plc_->recordingPending())));
}
void MainWindow::runtimeAction(const QString& action) {
    const auto timing=AcquisitionTiming::request("runtime");const auto commandedAt=AcquisitionTiming::Clock::now();AcquisitionTimingBinding binding(timing);AcquisitionTimingSpan commandTiming("runtime.user-"+action);
    try {
        if(action=="stop"){
            if(plc_ && plc_->working())plc_->cancel();
            if(runtimeCoordinator_)runtimeCoordinator_->stop();
            runtimeJobs_.stop(SessionJob::Execution);runtimeResult_.reset();showRuntimeResult();refreshRuntime();return;
        }
        if(action=="retry" && plc_ && plc_->recordingPending()){plc_->retryRecording();refreshRuntime();return;}
        if(plc_ && plc_->blocking())throw std::runtime_error("PLC kayıt/teslimi bekleniyor.");
        if(runtimeJobs_.busy())return;
        ensureRuntime();
        if(action=="retry") {
            if(!runtimePending_)return;
            const bool stopped=runtimeCoordinator_->state()==RuntimeState::Stopping || runtimeCoordinator_->state()==RuntimeState::LoadedStopped;
            runtimeCoordinator_->retryRecording();if(measurementWorkspace_ && runtime_->active())measurementWorkspace_->observeAcquisition(runtime_->active()->recipe(),*runtimePending_);if(stopped)runtimeResult_.reset();else runtimeResult_=std::move(runtimePending_);runtimePending_.reset();runtimeCycle_={};showRuntimeResult();refreshRuntime();return;
        }
        if(editorSession_.busy() || cameraSession_.busy())throw std::runtime_error("Önce diğer kamera/editor işini tamamlayın.");
        runtimeResult_.reset();showRuntimeResult();
        if(action=="run") {
            runtimeCoordinator_->start();
        } else if(action=="cycle") {
            runtimeCycle_=runtimeCoordinator_->begin(RuntimeTrigger::Manual);const auto cycle=runtimeCycle_;const auto catalog=runtimePath_+".calibrations.json";
            auto* watcher=runtimeJobs_.watch<CycleResult>(this,SessionJob::Execution);const auto cancel=runtimeCoordinator_->cancellation();
            connect(watcher,&QFutureWatcher<CycleResult>::finished,this,[this,watcher,cancel,timing,commandedAt]{
                AcquisitionTimingBinding binding(timing);AcquisitionTimingSpan delivery("runtime.command-to-delivery",commandedAt);
                auto result=watcher->result();watcher->deleteLater();
                delivery.finish(cancel->load()?"cancelled":result.state==ExecutionState::Done?"success":"error");
                completeRuntimeResult(std::move(result),cancel);
            });
            runtimeJobs_.launch(watcher,[cycle,catalog,cancel,timing,commandedAt]{AcquisitionTimingBinding binding(timing);AcquisitionTiming::record(timing,"runtime.worker-queue",commandedAt);return RuntimeService::execute(cycle,catalog,cancel);});
        }
        refreshRuntime();
    }catch(const ProductionException& e){runtimeSummary_->setText((e.code()==ProductionError::Busy ? "Meşgul — ikinci çevrim başlatılmadı: " : runtimeCoordinator_ ? runtimeCoordinator_->recovery()+"\n":"")+QString::fromUtf8(e.what()));refreshRuntime();}
    catch(const std::exception& e){const auto diagnostic=QString::fromUtf8(e.what());runtimeSummary_->setText(diagnostic.contains("native",Qt::CaseInsensitive) ? "Endüstriyel kamera kullanılamıyor. Bu backend için edinim doğrulaması tamamlanmadı.":diagnostic);runtimeSummary_->setToolTip(diagnostic);refreshRuntime();}
}
void MainWindow::completeRuntimeResult(CycleResult result,const std::shared_ptr<std::atomic_bool>& cancel){
    try {
        if(cancel && cancel->load())RuntimeService::suppressCancelled(runtimeCycle_,result);
        runtimeCoordinator_->finish(runtimeCycle_.ticket,result);
        if(measurementWorkspace_ && runtime_->active())measurementWorkspace_->observeAcquisition(runtime_->active()->recipe(),result);
        runtimeResult_=result;runtimePending_.reset();runtimeCycle_={};showRuntimeResult();
    }catch(const ProductionException& e){
        if(e.code()==ProductionError::PersistenceFailure)runtimePending_=result;else runtimePending_.reset();
        runtimeSummary_->setText((e.code()==ProductionError::PersistenceFailure ? "Kayıt bekliyor — Kaydı tekrar yaz aynı completion'ı yazar.\n":runtimeCoordinator_->recovery()+"\n")+QString::fromUtf8(e.what()));runtimeViewer_->clearImage("Sonuç yayımlanmadı.");
    }catch(const std::exception& e){runtimePending_.reset();runtimeSummary_->setText("Completion işlenemedi — tanıyı inceleyin: "+QString::fromUtf8(e.what()));runtimeViewer_->clearImage("Sonuç yayımlanmadı.");}
    refreshRuntime();
}
void MainWindow::refreshRuntimeSources(){
    if(!runtimeSources_)return;
    const auto loaded=runtimeCoordinator_ ? runtimeCoordinator_->loaded():std::optional<Recipe>{};
    const auto snapshot=runtime_ ? runtime_->active():nullptr;
    const Recipe* recipe=runtimeResult_ && snapshot ? &snapshot->recipe():loaded ? &*loaded:nullptr;
    const auto previous=runtimeSources_->currentData().toString();QSignalBlocker blocker(runtimeSources_);QSignalBlocker tableBlocker(runtimePreparation_);runtimeSources_->clear();runtimePreparation_->setRowCount(0);
    if(!recipe)return;const auto graph=validateSourceGraph(editorTools(recipe->definition),editorConnections(recipe->definition));
    for(const auto& tool:editorTools(recipe->definition))if(graph.requiredSources.contains(tool.id)){
        runtimeSources_->addItem(tool.name,tool.id);const int row=runtimePreparation_->rowCount();runtimePreparation_->insertRow(row);
        QJsonObject deps;if(snapshot && snapshot->recipe().id==recipe->id)deps=snapshot->dependencies().contains("sourceSet") ? snapshot->dependencies()["sourceSet"].toObject()[tool.id].toObject():snapshot->dependencies();
        QString model="Raw / px • metrik model kullanılmıyor";if(deps.contains("systemCalibration")){const auto evidence=SystemCalibration::decodeEvidence(deps["systemCalibration"].toObject());model=QString("Geçerli • mm • %1 v%2").arg(evidence.record.name()).arg(evidence.selection.version);}
        else if(deps.contains("experimentalCalibration")){const auto record=CalibrationModelRecord::decode(deps["experimentalCalibration"].toObject()["record"].toObject());model=QString("mm • %1 v%2").arg(record.name()).arg(deps["experimentalCalibration"].toObject()["selection"].toObject()["version"].toInt());}
        else if(SystemCalibration::requestsCalibration(SystemCalibration::branch(*recipe,tool.id),false))model="mm • Exact model gerekli / Run'da doğrulanacak";
        QString actual="Kare yok";if(runtimeResult_)for(const auto& f:runtimeResult_->frames)if(f.sourceId==tool.id)actual=f.frameId;
        QString readiness=deps.isEmpty() ? "Beyan / Run bekleniyor":"Hazırlık snapshot'ı";
        if(runtimeResult_)for(const auto& result:runtimeResult_->tools)if(result.toolId==tool.id)readiness=executionStateName(result.state)+(result.errorCode.isEmpty() ? "":" / "+result.errorCode);
        if(runtimeResult_ && runtimeResult_->state==ExecutionState::Skipped){readiness="Durduruldu / iptal — sonuç bastırıldı";actual="Geçerli çevrim karesi yok";}
        QString sourceLabel=tool.name;const auto binding=deps["measurementSource"].toObject()["binding"].toObject();
        const auto target=SystemCalibration::setupReference(SystemCalibration::branch(*recipe,tool.id));
        const auto cameraId=target ? target->camera.value:tool.parameters["sourceReference"].toObject()["cameraId"].toString();bool namedCamera=false;
        if(!cameraId.isEmpty() && measurementWorkspace_){const auto environment=measurementWorkspace_->environment();for(const auto& camera:environment.current.cameras)if(camera.id.value==cameraId){const auto shortId=environment.registrations.value(camera.id.value).shortId;sourceLabel+="\n"+(shortId.isEmpty()?camera.id.value:shortId)+" / "+camera.name;namedCamera=true;break;}}
        if(!binding.isEmpty() && !target){const auto bound=SetupReference::fromJson(binding["target"].toObject());sourceLabel+=" / "+bound.camera.value;}
        QString industrialDetail;
        if(tool.kind=="industrial_camera"){
            const auto admission=deps["industrialSource"].toObject();QJsonObject acquisition;
            if(runtimeResult_ && runtimeResult_->industrialEvidence.contains(tool.id))acquisition=industrialCompletionEvidence(*runtimeResult_->industrialEvidence[tool.id]);
            const auto presentation=presentIndustrialSource(tool.parameters["sourceReference"].toObject(),admission,acquisition);
            if(!namedCamera)sourceLabel+="\n"+presentation.device;industrialDetail=presentation.detail;
            if(!deps.contains("systemCalibration") && !deps.contains("experimentalCalibration"))model+="\nÖlçüm doğrulanmadı";
            if(runtimeResult_){for(const auto& t:runtimeResult_->tools)if(t.toolId==tool.id){
                if(runtimeResult_->state==ExecutionState::Skipped)readiness=acquisitionStatus("SKIPPED");
                else if(t.state==ExecutionState::Skipped)readiness="Çalıştırılmadı • kare yok";
                else readiness=acquisitionStatus(executionStateName(t.state),t.errorCode+" "+t.message);
                if(t.state==ExecutionState::Done && actual=="Kare yok" && runtimeResult_->state!=ExecutionState::Skipped)readiness="Kare yok";
                industrialDetail+="\n"+t.message;
            }}
            else if(runtimeCoordinator_ && runtimeCoordinator_->state()==RuntimeState::Fault){readiness=acquisitionStatus("ERROR",runtimeCoordinator_->error());industrialDetail+="\n"+runtimeCoordinator_->error();}
        }
        if(snapshot && snapshot->validation().scope=="experimental_runtime/2"){
            industrialDetail+="\n"+ExperimentalRuntime::sourceSummary(deps,tool.id);
            const auto acquisition=deps["experimentalAcquisition"].toObject();
            const auto physical=deps["camera"].toObject()["physical"].toObject();
            const auto profile=deps["profile"].toObject();
            industrialDetail+="\nSeri: "+physical["serial"].toString(acquisition["serial"].toString())+
                " • Profil: "+profile["id"].toString(acquisition["profileId"].toString());
            const auto image=deps["imageInfo"].toObject();
            industrialDetail+="\nKöken: "+deps["measurementSource"].toObject()["origin"].toString()+
                " • İçerik: "+deps["contentId"].toString();
            if(!image.isEmpty())industrialDetail+=QString(" • %1×%2").arg(image["width"].toInt()).arg(image["height"].toInt());
        }else industrialDetail+="\n"+QString::fromUtf8(QJsonDocument(deps).toJson(QJsonDocument::Indented));
        const QStringList values{sourceLabel,model,readiness,actual};for(int col=0;col<values.size();++col){auto* item=new QTableWidgetItem(values[col]);item->setToolTip(values[col]+"\n"+tool.id+"\n"+industrialDetail);runtimePreparation_->setItem(row,col,item);}
        runtimeSources_->setItemText(row,sourceLabel.replace('\n'," • "));
    }
    runtimePreparation_->resizeRowsToContents();
    runtimeSources_->setCurrentIndex(std::max(0,runtimeSources_->findData(previous)));runtimePreparation_->selectRow(runtimeSources_->currentIndex());runtimeSources_->setVisible(runtimeSources_->count()>1);runtimePreparation_->setVisible(runtimeSources_->count()>0);
}
void MainWindow::showRuntimeResult() {
    if(!runtimeViewer_)return;
    runtimeSummary_->setToolTip({});refreshRuntimeSources();
    runtimeEdgeProfile_->setDiagnostics({});runtimeViewer_->onRoiChanged={};runtimeViewer_->setRoi({},false);runtimeViewer_->setRoiRecipeBound(false);
    runtimeViewer_->setToolOverlay({},{});
    if(!runtimeResult_){runtimeViewer_->clearImage("Yeni çevrim sonucu yok.");runtimeSummary_->setText("Yeni çevrim sonucu yok.");QSignalBlocker blocker(runtimeTools_);runtimeTools_->clear();return;}
    const auto& c=*runtimeResult_;const auto snapshot=runtime_ ? runtime_->active() : nullptr;QStringList lines;
    if(snapshot && snapshot->dependencies().contains("systemCalibration")){const auto s=SystemCalibration::decodeEvidence(snapshot->dependencies()["systemCalibration"].toObject());lines<<QString("Sistem kalibrasyonu: %1 v%2 • etkinleştirme %3").arg(s.record.name()).arg(s.selection.version).arg(s.selection.revision)<<(s.record.residualsMillimetres().isEmpty() ? "Bağımsız kontrol yapılmadı":"Bağımsız kontrol kanıtı mevcut");if(!s.scope.isEmpty()){const auto target=SetupReference::fromJson(s.scope["target"].toObject());lines<<QString("Kamera %1 • kurulum %2 r%3").arg(target.camera.value,target.setup.value).arg(target.revision);}}
    lines<<executionStateName(c.state)+QString(" • %1 ms").arg(c.milliseconds,0,'f',2)<<"Cycle: "+c.cycleId<<"Frame: "+c.frameId<<"Config: "+QString::fromLatin1(c.configId.left(12))<<(runtime_ && runtime_->experimental() ? "Kalıcı kayıt tamamlandı • Parça: Değerlendirilmedi • PLC / robot: Kullanılamıyor":plc_ && plc_->listening() ? "Kalıcı kayıt tamamlandı • PLC durumu Communication ekranında" : "Kalıcı kayıt tamamlandı • İletişim: NOT_REQUESTED");
    if(snapshot && snapshot->dependencies().contains("sourceSet")){const auto evidence=snapshot->dependencies()["sourceSet"].toObject()[runtimeSources_->currentData().toString()].toObject()["systemCalibration"].toObject();if(!evidence.isEmpty()){const auto model=SystemCalibration::decodeEvidence(evidence);lines<<QString("Seçili kaynak modeli: %1 v%2").arg(model.record.name()).arg(model.selection.version);}}
    if(snapshot){const auto source=runtimeSources_->currentData().toString();const auto admission=snapshot->dependencies()["sourceSet"].toObject()[source].toObject()["industrialSource"].toObject();if(!admission.isEmpty()){QJsonObject acquisition;if(c.industrialEvidence.contains(source))acquisition=industrialCompletionEvidence(*c.industrialEvidence[source]);lines<<presentIndustrialSource({},admission,acquisition).detail;}}
    if(!c.error.isEmpty())lines<<c.error;
    runtimeViewer_->setRoiMapping({"viewer/raw"});std::optional<ImageFrame> rawFrame;
    const auto selectedSource=runtimeSources_->currentData().toString();
    const auto graph=snapshot ? validateSourceGraph(editorTools(snapshot->recipe().definition),editorConnections(snapshot->recipe().definition)):GraphValidation{};
    bool image=false;
    for(const auto& t:c.tools)for(const auto& v:t.outputs)if(v.valid && v.origin.sourceId==selectedSource && std::holds_alternative<ImageFrame>(v.data) && std::get<ImageFrame>(v.data).processing().isEmpty()){rawFrame=std::get<ImageFrame>(v.data);runtimeViewer_->setImage(rawFrame->preview());image=true;}
    if(!image)runtimeViewer_->clearImage("Bu kaynak için geçerli çevrim görüntüsü yok.");
    if(rawFrame)lines<<"Kaynak: "+selectedSource<<"Seçili frame: "+rawFrame->metadata().frameId;
    const auto selected=runtimeTools_->currentData().toString();
    {QSignalBlocker blocker(runtimeTools_);runtimeTools_->clear();if(snapshot)for(const auto& t:editorTools(snapshot->recipe().definition))if(graph.toolSources.value(t.id)==selectedSource)runtimeTools_->addItem(t.name,t.id);runtimeTools_->setCurrentIndex(std::max(0,runtimeTools_->findData(selected)));}
    for(const auto& t:c.tools){if(graph.toolSources.value(t.toolId)!=selectedSource)continue;QString name=t.toolId;if(snapshot)for(const auto& tool:editorTools(snapshot->recipe().definition))if(tool.id==t.toolId)name=tool.name;
        lines<<name+" • "+executionStateName(t.state)+" • "+(t.fitDiagnostic ? presentMeasurementDiagnostic(*t.fitDiagnostic):t.errorCode+" "+t.message);
        for(auto it=t.outputs.cbegin();it!=t.outputs.cend();++it)if(it->port.type!=PortType::Image) {
            const auto value=ToolResultCodec::display(*it);
            lines<<"  "+it.key()+(it->diagnostic ? " [diagnostic]: ":": ")+value+" "+it->port.unit;
        }
        if(t.decision!=PartDecision::NotEvaluated)lines<<QString("  Ölçüm: %1 mm • %2").arg(std::get<double>(t.outputs.value("value").data),0,'f',6).arg(t.decision==PartDecision::Pass ? "PASS" : "FAIL");
        if(t.toolId==runtimeTools_->currentData().toString()){if(t.state==ExecutionState::Done){runtimeEdgeProfile_->setDiagnostics(t.edgeDiagnostics);if((t.edgeDiagnostics || t.outputs.contains("valuePx") || t.outputs.contains("widthPx") || (t.outputs.contains("linePx") || t.outputs.contains("circlePx"))) && snapshot){for(const auto& tool:editorTools(snapshot->recipe().definition))if(tool.id==t.toolId){try{if(tool.parameters.contains("referenceBinding"))throw std::runtime_error("Reference ROI uses current resolver");auto roi=ToolRoi::fromJson(tool.parameters["scanRoi"].toObject());if(roi.frame.unit==LengthUnit::Millimetre && rawFrame){const auto frame=*rawFrame;const auto ctx=RuntimeService::calibration(*snapshot,frame,runtimePath_+".calibrations.json");runtimeViewer_->setRoiMapping(roi.frame,[frame,ctx](QPointF p){return CircleToolService::mapPoint(frame,ctx,p,true);});}else runtimeViewer_->setRoiMapping(roi.frame);runtimeViewer_->setRoi(roi,false);runtimeViewer_->setRoiRecipeBound(true);runtimeViewer_->setRoiEditable(false);}catch(const std::exception&){}break;}}}const auto processed=t.outputs.value("image");if(processed.valid)if(const auto* frame=std::get_if<ImageFrame>(&processed.data))if(!frame->processing().isEmpty()){runtimeViewer_->setPreviewPixels(frame->preview());lines<<"  Görüntü: işlenmiş Gray8 (aynı raw geometri)";}QString annotation;if(t.outputs.contains("valuePx") || t.outputs.contains("widthPx") || (t.outputs.contains("linePx") || t.outputs.contains("circlePx")))annotation=t.message;annotation.replace(" • ","\n");if(t.decision!=PartDecision::NotEvaluated)annotation=QString("%1 mm • %2").arg(std::get<double>(t.outputs.value("value").data),0,'f',3).arg(t.decision==PartDecision::Pass ? "PASS" : "FAIL");runtimeViewer_->setToolOverlay(t.regions,t.points,t.lines,annotation,t.mask.pixels,t.mask.size,t.mask.origin,t.rejectedPoints,(t.outputs.contains("linePx") || t.outputs.contains("circlePx")));const auto codes=t.outputs.value("codes");if(codes.valid)if(const auto* list=std::get_if<CodeResults>(&codes.data))runtimeViewer_->setCodeOverlay(*list);}
    }
    // Resolve only against this recorded cycle and immutable snapshot. A failed
    // current reference leaves the viewer empty of ROI, never nominal geometry.
    if(snapshot && rawFrame)for(const auto& tool:editorTools(snapshot->recipe().definition))
        if(tool.id==runtimeTools_->currentData().toString() && tool.parameters.contains("referenceBinding"))try {
            const auto binding=toolReferenceBinding(tool);ToolDefinition producer;ToolResult source,current;
            for(const auto& node:editorTools(snapshot->recipe().definition))if(node.id==binding.sourceTool)producer=node;
            for(const auto& r:c.tools){if(r.toolId==binding.sourceTool)source=r;if(r.toolId==tool.id)current=r;}
            if(current.state!=ExecutionState::Done)break;
            if(binding.world) {
                const auto ctx=RuntimeService::calibration(*snapshot,*rawFrame,runtimePath_+".calibrations.json");
                const auto mapped=ReferenceService::resolveWorld(binding,producer,source,*rawFrame,ctx);
                if(mapped.map){runtimeViewer_->setRoiMapping(binding.nominal.frame,mapped.map->nominalToRaw);runtimeViewer_->setRoi(binding.nominal,false);}
            }else {
                const auto mapped=ReferenceService::resolve(binding,producer,source,*rawFrame);
                if(mapped.roi){runtimeViewer_->setRoiMapping(mapped.roi->frame);runtimeViewer_->setRoi(mapped.roi,false);}
            }
            runtimeViewer_->setRoiRecipeBound(true);runtimeViewer_->setRoiEditable(false);
        }catch(const std::exception&){}
    QStringList diagnoses;
    for(const auto& t:c.tools)if(t.fitDiagnostic && graph.toolSources.value(t.toolId)==selectedSource){QString name=t.toolId;if(snapshot)for(const auto& configured:editorTools(snapshot->recipe().definition))if(configured.id==t.toolId)name=configured.name;diagnoses<<name+" • "+executionStateName(t.state)<<presentMeasurementDiagnostic(*t.fitDiagnostic);}
    if(snapshot && snapshot->validation().scope.startsWith("experimental_runtime/")){
        QStringList headline{"Kaynak: "+runtimeSources_->currentText()};
        if(c.state==ExecutionState::Done)for(const auto& t:c.tools)if(t.state==ExecutionState::Done && graph.toolSources.value(t.toolId)==selectedSource)for(auto i=t.outputs.begin();i!=t.outputs.end();++i)if(i->valid && std::holds_alternative<double>(i->data) && (i->port.unit=="mm" || i->port.unit.startsWith("px")))headline<<i.key()+": "+ToolResultCodec::display(*i)+" "+i->port.unit;
        headline<<"Parça: Değerlendirilmedi • PLC / robot: Kullanılamıyor";
        lines.prepend(headline.join('\n'));lines<<ExperimentalRuntime::sourceSummary(snapshot->dependencies()["sourceSet"].toObject()[selectedSource].toObject(),selectedSource);
    }
    runtimeSummary_->setText((diagnoses+lines).join("\n"));
}
void MainWindow::refreshLogs() {
    logTable_->setRowCount(0);findChild<QTextEdit*>("logDetail")->clear();
    try {
        if(!service_.hasProject()){logStatus_->setText("Önce proje açın.");return;}
        auto records=ProductionStore(service_.path()+".production").recent("cycles");records+=ProductionStore(service_.path()+".production").recent("experimental_cycles");
        const auto events=ProductionStore(service_.path()+".production").recent("events",256);
        QMap<QString,QString> delivery;
        for(const auto& e:events){const auto action=e["action"].toString();if(action=="PLC_ACK")delivery[e["cycleId"].toString()]="ACKED";else if(action=="PLC_RESULT"){const auto id=e["response"].toObject()["cycleId"].toString();if(!delivery.contains(id))delivery[id]="RESULT_RECORDED / ACK kanıtı bu listede yok";}}
        logStatus_->setText(QString("Son %1 kalıcı çevrim • restart sonrasında salt okunur • PLC: son 256 event ile ilişkilendirilir").arg(records.size()));
        for(const auto& record:records){const bool experimental=record["schema"]=="visionflow.experimental-runtime1" || record["schema"]=="visionflow.experimental-runtime2";const auto summary=experimental ? ExperimentalRuntime::summary(record):ProductionResultCodec::summary(record);const int row=logTable_->rowCount();logTable_->insertRow(row);QStringList decisions;
            QMap<QString,QString> sourceNames;if(experimental)for(const auto& tool:editorTools(record["configuration"].toObject()["definition"].toObject()))sourceNames[tool.id]=tool.name;
            for(const auto& v:record["tools"].toArray()){const auto t=v.toObject();if(t["decision"]!="NOT_EVALUATED")decisions<<t["name"].toString(t["toolId"].toString().left(8))+": "+t["decision"].toString();}
            if(experimental && record["state"]=="DONE"){
                QStringList metric,pixels;const auto sources=record["dependencies"].toObject()["sourceSet"].toObject();
                for(const auto& v:record["tools"].toArray()){const auto t=v.toObject();if(t["state"]!="DONE")continue;const auto outputs=t["outputs"].toObject();for(auto i=outputs.begin();i!=outputs.end();++i){const auto o=i->toObject();if(!o.contains("typedPort"))continue;const auto port=ToolResultCodec::decode(o["typedPort"].toObject());if(!port.valid || !std::holds_alternative<double>(port.data))continue;
                    const auto text=sourceNames.value(port.origin.sourceId,port.origin.sourceId)+" / "+i.key()+": "+ToolResultCodec::display(port,3)+" "+port.port.unit;
                    if(port.port.unit=="mm")metric<<text;else if(!sources[port.origin.sourceId].toObject().contains("experimentalCalibration") && port.port.unit.startsWith("px"))pixels<<text;
                }}decisions=metric+pixels;
            }
            const QStringList values{record["completedAtUtc"].toString()+"\n"+record["cycleId"].toString(),record["recipeName"].toString(record["recipeId"].toString().left(8))+QString(" v%1").arg(record["recipeVersion"].toInt()),record["state"].toString(),decisions.isEmpty() ? "NOT_EVALUATED" : decisions.join(experimental ? "\n":"; "),delivery.value(record["cycleId"].toString(),record["communicationState"].toString())};
            for(int col=0;col<values.size();++col)logTable_->setItem(row,col,new QTableWidgetItem(values[col]));if(experimental){logTable_->item(row,2)->setText(record["state"].toString());logTable_->item(row,4)->setText("Kullanılamıyor");}for(int col=0;col<values.size();++col)logTable_->item(row,col)->setToolTip(summary);logTable_->item(row,0)->setData(Qt::UserRole,experimental ? summary:summary+"\n\n"+QString::fromUtf8(QJsonDocument(record).toJson(QJsonDocument::Indented)));logTable_->item(row,0)->setData(Qt::UserRole+1,experimental);logTable_->item(row,0)->setData(Qt::UserRole+2,record["schema"].toString());}
        logTable_->resizeRowsToContents();for(int row=0;row<logTable_->rowCount();++row)logTable_->setRowHeight(row,std::min(110,logTable_->rowHeight(row)));
    }catch(const std::exception& e){logStatus_->setText("Kayıt okunamadı: "+QString::fromUtf8(e.what()));}
}
}
