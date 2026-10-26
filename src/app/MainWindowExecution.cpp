#include "application/EditorObservation.hpp"
#include "application/MeasurementDiagnostic.hpp"
#include "MainWindow.hpp"
#include "application/IndustrialExecution.hpp"
#include "application/AcquisitionPolicy.hpp"
#include "application/ExperimentalOecsAcquisition.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include "application/SystemCalibration.hpp"
#include "ImageViewer.hpp"
#include "EdgeProfileView.hpp"
#include "application/ToolResultCodec.hpp"
#include "application/CalibrationWorkflow.hpp"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QListWidget>
#include <QTabWidget>
#include <QScrollArea>
#include <QEventLoop>
#include <QStatusBar>
#include <algorithm>
#include <cmath>
#include <numbers>
#include "domain/ToolRegistry.hpp"
#include "domain/PoseTrackingConfig.hpp"
namespace visionflow {
std::optional<ActiveCalibrationSnapshot> MainWindow::systemCalibrationSnapshot(const Recipe* recipe) const {const auto* r=recipe ? recipe:service_.activeRecipe();if(r && (r->definition.contains("sourceSetup") || r->definition.contains("calibrationPolicy"))){if(!measurementWorkspace_ || measurementWorkspaceKey_!=service_.path()+"\n"+service_.project().id)throw std::runtime_error("Measurement workspace unavailable");return measurementWorkspace_->forRecipe(*r);}if(!measurementWorkspace_ || !measurementWorkspace_->legacyCalibrations().systemPolicy())return {};return measurementWorkspace_->legacyCalibrations().snapshot();}
QJsonObject MainWindow::systemCalibrationIdentity() const {
    editor_observation::Scope observation(editor_observation::CalibrationIdentity);
    if(service_.activeRecipe() && service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]!=9 && measurementWorkspace_)try{const auto ref=SystemCalibration::setupReference(*service_.activeRecipe());if(ref && measurementWorkspace_->calibrations().status(*ref).experimental){const auto typed=measurementWorkspace_->calibrations().experimentalSnapshots({*ref}).first();return {{"policy","experimental-editor-model/1"},{"contentId",typed.snapshot.calibration.selection.contentId},{"revision",typed.snapshot.calibration.selection.revision}};}}catch(const std::exception& e){return {{"experimental",true},{"unavailable",QString::fromUtf8(e.what())}};}
    if(service_.activeRecipe() && service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9){QJsonObject identities;for(const auto& t:service_.tools())if(ToolRegistry::get(t.kind).acquisitionSource){try{const auto branch=SystemCalibration::branch(*service_.activeRecipe(),t.id);const auto ref=SystemCalibration::setupReference(branch);if(ref && measurementWorkspace_ && measurementWorkspace_->calibrations().status(*ref).experimental){const auto typed=measurementWorkspace_->calibrations().experimentalSnapshots({*ref}).first();const auto& model=typed.snapshot.calibration;identities[t.id]=QJsonObject{{"experimental",true},{"contentId",model.selection.contentId},{"revision",model.selection.revision}};continue;}const auto snapshot=measurementWorkspace_ ? measurementWorkspace_->forRecipe(branch):std::nullopt;if(snapshot)identities[t.id]=QJsonObject{{"contentId",snapshot->selection.contentId},{"revision",snapshot->selection.revision}};else identities[t.id]=QJsonObject{{"unavailable","No bound model"}};}catch(const std::exception& e){identities[t.id]=QJsonObject{{"unavailable",QString::fromUtf8(e.what())}};}}return identities;}

    try {const auto s=systemCalibrationSnapshot();if(!s)return {};return {{"policy","system-model-ready/1"},{"contentId",s->selection.contentId},{"revision",s->selection.revision}};}catch(const std::exception& e){return {{"policy","system-model-ready/1"},{"unavailable",QString::fromUtf8(e.what())}};}
}
CalibrationResolver MainWindow::calibrationResolver(const Recipe& recipe) const {
    const auto directory=QFileInfo(service_.path()).absolutePath(),path=service_.path()+".calibrations.json";
    if(recipe.definition["editor"].toObject()["schemaVersion"]==9){
        QMap<QString,CalibrationResolver> resolvers;
        for(const auto& t:editorTools(recipe.definition))if(t.enabled && ToolRegistry::get(t.kind).acquisitionSource)try{
            const auto branch=SystemCalibration::branch(recipe,t.id);if(!measurementWorkspace_)throw std::runtime_error("Measurement workspace unavailable");const auto ref=SystemCalibration::setupReference(branch);if(ref && measurementWorkspace_->calibrations().status(*ref).experimental){const auto typed=measurementWorkspace_->calibrations().experimentalSnapshots({*ref}).first();resolvers[t.id]=[typed,branch](const ImageFrame& f){return SystemCalibration::resolveExperimental(typed,branch,f);};continue;}const auto snapshot=measurementWorkspace_->forRecipe(branch);if(!snapshot)throw std::runtime_error("Source requires explicit calibration binding");resolvers[t.id]=[snapshot,branch,directory](const ImageFrame& f){return SystemCalibration::resolve(*snapshot,branch,directory,f);};
        }catch(const std::exception& e){const auto error=QString::fromUtf8(e.what());resolvers[t.id]=[error](const ImageFrame& f){MeasurementExecutionContext c;c.rawSize=f.size();c.error=error;return c;};}
        return [resolvers](const ImageFrame& f){const auto it=resolvers.constFind(f.metadata().sourceId);if(it!=resolvers.cend())return (*it)(f);MeasurementExecutionContext c;c.error="Unknown source calibration";return c;};
    }
    try{const auto ref=SystemCalibration::setupReference(recipe);if(ref && measurementWorkspace_ && measurementWorkspace_->calibrations().status(*ref).experimental){const auto typed=measurementWorkspace_->calibrations().experimentalSnapshots({*ref}).first();return [typed,recipe](const ImageFrame& f){return SystemCalibration::resolveExperimental(typed,recipe,f);};}if(const auto s=systemCalibrationSnapshot(&recipe))return [s,recipe,directory](const ImageFrame& f){return SystemCalibration::resolve(*s,recipe,directory,f);};}
    catch(const std::exception& e){const auto error=QString::fromUtf8(e.what());return [error](const ImageFrame& f){MeasurementExecutionContext c;c.rawSize=f.size();c.error=error;return c;};}
    return [recipe,path](const ImageFrame& f){return CalibrationWorkflow::resolve(recipe,f,path);};
}
QString MainWindow::calibrationSummary(const EditorContext* context) const {try{if(service_.activeRecipe() && service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9){const auto source=editorSession_.sourceForTool(context ? *context:editorContext(),selectedToolId_);if(source.isEmpty() || !measurementWorkspace_)return "Kaynak seçin";const auto branch=SystemCalibration::branch(*service_.activeRecipe(),source);const auto ref=SystemCalibration::setupReference(branch);if(ref){const auto status=measurementWorkspace_->calibrations().status(*ref);const auto prefix=QString{};if(status.state==CalibrationAvailability::None)return prefix+"Kalibrasyon yok — raw/px kullanılabilir";if(status.state==CalibrationAvailability::RecalibrationRequired)return prefix+"Yeniden kalibrasyon gerekli";if(!status.sessionPermitted)return prefix+"Kayıtlı / bağlantı ve oturum denetimi bekleniyor";if(status.experimental)return prefix+"Etkin model";}if(const auto s=measurementWorkspace_->forRecipe(branch))return s->record.name()+QString(" • v%1 • bu kaynak").arg(s->selection.version);return "Bu kaynakta kalibrasyon bağı yok";}if(service_.activeRecipe() && measurementWorkspace_){const auto ref=SystemCalibration::setupReference(*service_.activeRecipe());if(ref){const auto status=measurementWorkspace_->calibrations().status(*ref);const auto prefix=QString{};if(status.state==CalibrationAvailability::None)return prefix+"Kalibrasyon yok — raw/px kullanılabilir";if(status.state==CalibrationAvailability::RecalibrationRequired)return prefix+"Yeniden kalibrasyon gerekli";if(!status.sessionPermitted)return prefix+"Kayıtlı / bağlantı ve oturum denetimi bekleniyor";if(status.experimental)return prefix+"Etkin model";}}if(const auto s=systemCalibrationSnapshot())return s->record.name()+QString(" • v%1 • etkinleştirme %2").arg(s->selection.version).arg(s->selection.revision);if(service_.activeRecipe() && service_.activeRecipe()->calibrationBinding)return "Kayıtlı reçete kalibrasyonu — uyum her çalıştırmada denetlenir";return "Aktif kalibrasyon yok — Kalibrasyon sayfasında kamera seçin";}catch(const std::exception& e){return QString::fromUtf8(e.what());}}
std::optional<MeasurementEnvironment> MainWindow::editorDisplayEnvironment() const {
    editor_observation::Scope observation(editor_observation::DisplayEnvironment);
    return measurementWorkspace_ ? std::optional{measurementWorkspace_->environment()}:std::nullopt;
}
EditorContext MainWindow::editorContext() const {
    editor_observation::Scope observation(editor_observation::Context);
    if(!service_.activeRecipe())return {};
    const auto& r=*service_.activeRecipe();return {service_.project().id,service_.path(),r.id,r.definition,r.calibrationBinding ? r.calibrationBinding->toJson() : QJsonObject{},systemCalibrationIdentity()};
}
QByteArray MainWindow::flowSignature() const {return editorSession_.signature(editorContext());}
void MainWindow::refreshFlow() {
    const auto environment=editorDisplayEnvironment();
    refreshFlowView(editorContext(),environment);
}
void MainWindow::refreshFlowView(const EditorContext& context,const std::optional<MeasurementEnvironment>& environment) {
    if(auto* status=propertiesBody_->findChild<QLabel*>("industrialSourceStatus")){
        if(!status->property("bindingStatus").isValid())status->setProperty("bindingStatus",status->text());
        status->setText(status->property("bindingStatus").toString()+"\n"+editorCameraStatus(selectedToolId_));
    }
    if(editorSession_.busy()){imageViewer_->cancelRoi();imageViewer_->drawRegion(false);imageViewer_->setSearchRegionEditing({}, {},false);}
    const auto identity=context.systemCalibration;if(identity!=editorCalibrationIdentity_ && !editorSession_.busy()){if(service_.activeRecipe() && service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9){for(auto i=identity.begin();i!=identity.end();++i)if(editorCalibrationIdentity_[i.key()]!=i.value())editorSession_.calibrationChanged(context,i.key());}else editorSession_.calibrationChanged(context);editorCalibrationIdentity_=identity;}
    if(!editorSession_.busy() && environment && service_.activeRecipe()){
        const auto tools=service_.tools();
        if(std::any_of(tools.cbegin(),tools.cend(),[](const auto& tool){return tool.kind=="industrial_camera";})){
        const auto& e=*environment;
        for(const auto& tool:tools)if(tool.kind=="industrial_camera")try{
            const auto ref=IndustrialSourceReference::fromJson(tool.parameters["sourceReference"].toObject());ref.validateAgainst(e.current);
            if(e.archivedCameras.contains(ref.camera.value) || e.archivedSetups.contains(ref.setup.value))throw std::runtime_error("Archived industrial source");
        }catch(const std::exception&){editorSession_.invalidateTool(context,tool.id);editorSession_.sourceFrames.remove(tool.id);if(editorSession_.frame && editorSession_.frame->metadata().sourceId==tool.id)editorSession_.frame.reset();}
    }
    }
    editorSession_.reconcile(context);
    refreshVariableCatalog(context);
    QString text="Önce reçete açın.";
    if(service_.activeRecipe()) {
        const auto graph=service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9 ? validateSourceGraph(service_.tools(),editorConnections(service_.activeRecipe()->definition)):validateGraph(service_.tools(),editorConnections(service_.activeRecipe()->definition));
        text=graph.valid() ? "DAG geçerli • "+QString(editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Image) ? "BUSY" : "READY") : graph.errors.join(" | ");
    }
    if(service_.activeRecipe())for(const auto& c:editorConnections(service_.activeRecipe()->definition)) {
        QString source=c.sourceTool,target=c.targetTool;
        for(const auto& t:service_.tools()){if(t.id==source)source=t.name;if(t.id==target)target=t.name;}
        text+="\n"+source+"."+c.sourcePort+" → "+target+"."+c.targetPort;
    }
    graphInfo_->setText(text);
    findChild<QPushButton*>("runFlow")->setEnabled(service_.activeRecipe() && !editorSession_.busy(SessionJob::Execution) && !editorSession_.busy(SessionJob::Teaching) && !editorSession_.busy(SessionJob::Image));
    if(auto* run=findChild<QPushButton*>("runTool")) {
        bool source=false,unavailable=false;if(service_.activeRecipe())for(const auto& t:service_.tools())if(t.id==selectedToolId_){source=ToolRegistry::get(t.kind).acquisitionSource;unavailable=t.kind=="industrial_camera" && !t.parameters["sourceReference"].isObject();}
        run->setText(source ? "Görüntü al" : "Run Tool");run->setEnabled(!selectedToolId_.isEmpty() && !editorSession_.busy() && !unavailable);
    }
    findChild<QPushButton*>("stopFlow")->setEnabled(editorSession_.busy() || !pendingDrawTool_.isEmpty() || imageViewer_->drawingActive() || editorCameras_.hasTargets() || editorCameras_.draining());
    for(const auto name:{"teachPoseReference","clearPoseReference","runSelectedTool"})if(auto* button=findChild<QPushButton*>(name)) {
        bool enabled=!editorSession_.busy();
        if(QString(name)=="runSelectedTool" && service_.activeRecipe())for(const auto& t:service_.tools())if(t.id==selectedToolId_ && t.kind=="industrial_camera" && !t.parameters["sourceReference"].isObject())enabled=false;
        if(QString(name)=="teachPoseReference")enabled=enabled && editorSession_.frame.has_value();
        if(QString(name)=="clearPoseReference"){enabled=false;for(const auto& t:service_.tools())if(t.id==selectedToolId_)enabled=!editorSession_.busy() && (t.parameters.contains("poseTracking") || t.parameters.contains("referenceBinding"));}
        button->setEnabled(enabled);
    }
    bool tracking=false;if(service_.activeRecipe())for(const auto& t:service_.tools())if(t.id==selectedToolId_)tracking=(t.parameters.contains("poseTracking") || t.parameters.contains("referenceBinding"));
    if(auto* picker=propertiesBody_->findChild<QComboBox*>("poseReferencePicker")){
        picker->setEnabled(!editorSession_.busy());const auto key=picker->currentData().toString();
        for(const auto name:{"referenceOffsetX","referenceOffsetY","referenceOffsetAngle"})if(auto* n=propertiesBody_->findChild<QDoubleSpinBox*>(name))n->setEnabled(!editorSession_.busy() && !key.isEmpty() && (QString(name)!="referenceOffsetAngle" || key.endsWith("/pose")));
    }
    if(tracking)for(const auto name:{"edgeDraw","edgeApplyScan","edgeReverse","metrologyDraw","metrologyApplyRoi","searchRoi","clearSearch"})if(auto* button=findChild<QPushButton*>(name))button->setEnabled(false);
    for(int i=0;i<flowTools_->count();++i) {
        auto* item=flowTools_->item(i);QString state=editorSession_.busy(SessionJob::Execution) ? "BUSY" : "READY";
        if(editorSession_.cycle)for(const auto& r:editorSession_.cycle->tools)if(r.toolId==item->data(Qt::UserRole).toString())state=executionStateName(r.state)+QString(" %1 ms").arg(r.milliseconds,0,'f',2);
        item->setToolTip(state);
        for(const auto& t:service_.tools())if(t.id==item->data(Qt::UserRole).toString())item->setText(t.name+" • "+toolLabel(t.kind)+" / "+state);
    }
    findChild<QPushButton*>("addTool")->setEnabled(service_.activeRecipe() && !editorSession_.busy());
    showExecutionResultView(context);
    if(!pendingDrawTool_.isEmpty() && !drawPreparationQueued_){drawPreparationQueued_=true;QTimer::singleShot(0,this,[this]{drawPreparationQueued_=false;prepareNewToolDrawing();});}
}
void MainWindow::showExecutionResult() {showExecutionResultView(editorContext());}
void MainWindow::showExecutionResultView(const EditorContext& context) {
    if(drawingImage_ && imageViewer_->drawingActive()) {
        try {const auto current=editorSession_.toolImage(context,selectedToolId_);if(current.contentId()!=drawingImage_->contentId() || current.processingId()!=drawingImage_->processingId())throw std::runtime_error("Drawing image changed");}
        catch(const std::exception&){imageViewer_->cancelRoi();imageViewer_->drawRegion(false);drawingImage_.reset();}
    }else drawingImage_.reset();
    edgeProfile_->setDiagnostics({});
    imageViewer_->setToolOverlay({},{});
    QImage displayPreview;
    if(editorSession_.frame){displayPreview=drawingImage_ ? drawingImage_->preview():editorSession_.frame->preview();imageInfo_->setText(QString("Ham g\u00f6r\u00fcnt\u00fc - %1 x %2 px - Frame %3").arg(editorSession_.frame->size().width()).arg(editorSession_.frame->size().height()).arg(editorSession_.frame->metadata().frameId));}
    if(drawingImage_)imageInfo_->setText(drawingImage_->processing().isEmpty() ? "Ham görüntü · bağlı kaynak":"İşlenmiş görüntü · bağlı Preprocess");
    if(!drawingImage_ && service_.activeRecipe())for(const auto& tool:service_.tools())if(tool.id==selectedToolId_ && tool.kind=="blob")try{
        const auto image=editorSession_.toolImage(context,tool.id);displayPreview=image.preview();
        QString source=image.metadata().sourceId;for(const auto& candidate:service_.tools())if(candidate.id==source){source=candidate.name;break;}
        imageInfo_->setText(QString("%1 · bağlı kaynak: %2 · %3 × %4 px · Frame %5").arg(image.processing().isEmpty() ? "Ham görüntü":"İşlenmiş görüntü",source).arg(image.size().width()).arg(image.size().height()).arg(image.metadata().frameId));
    }catch(const std::exception&){}
    if(!editorSession_.frame && !drawingImage_){imageViewer_->clearImage("Bu kaynak için güncel kare yok.",editorSession_.busy());imageInfo_->setText("Görüntü yok.");}
    // Resolve the final raster before uploading: do not bounce processed -> raw -> processed.
    if(editorSession_.cycle)for(const auto& r:editorSession_.cycle->tools)
        if(r.toolId==selectedToolId_ && r.state==ExecutionState::Done &&
           (editorSession_.sourceFrames.isEmpty() || (editorSession_.frame && editorSession_.sourceForTool(context,r.toolId)==editorSession_.frame->metadata().sourceId))){
            const auto processed=r.outputs.value("image");
            if(processed.valid)if(const auto* image=std::get_if<ImageFrame>(&processed.data))
                if(!image->processing().isEmpty())displayPreview=image->preview();
        }
    if(!displayPreview.isNull())imageViewer_->setPreviewPixels(displayPreview,editorRecipeId_+"|"+(editorSession_.frame ? editorSession_.frame->metadata().sourceId:QString{}));
    if(!editorSession_.cycle){matchInfo_->clear();return;}
    if(editorSession_.frame){const auto evidence=editorSession_.cycle->industrialEvidence.value(editorSession_.frame->metadata().sourceId);
        if(evidence)imageInfo_->setText(imageInfo_->text()+QString("\n%1 • %2 | Profil %3 r%4 | actual exposure %5 µs / gain %6 | offset %7,%8")
            .arg(evidence->request.device.identity.model,evidence->request.device.identity.serial,evidence->request.profile.name).arg(evidence->request.profile.revision)
            .arg(evidence->actual.settings.exposureTimeUs).arg(evidence->actual.settings.gain).arg(evidence->actual.geometry.offset.x()).arg(evidence->actual.geometry.offset.y()));
    }
    QStringList lines;if(editorSession_.cycle->partial)lines<<"Seçili araç değerlendirmesi";lines<<QString("%1 | %2 ms").arg(executionStateName(editorSession_.cycle->state)).arg(editorSession_.cycle->milliseconds,0,'f',2);
    if(!editorSession_.cycle->error.isEmpty())lines<<editorSession_.cycle->error;
    auto ordered=editorSession_.cycle->tools;
    std::stable_sort(ordered.begin(),ordered.end(),[this](const auto& a,const auto& b){return a.toolId==selectedToolId_ && b.toolId!=selectedToolId_;});
    for(const auto& r:ordered) {
        QString name=r.toolId;for(const auto& t:service_.tools())if(t.id==r.toolId)name=t.name;
        lines<<name+" • "+executionStateName(r.state)+QString(" • %1 ms ").arg(r.milliseconds,0,'f',2)+(r.fitDiagnostic ? presentMeasurementDiagnostic(*r.fitDiagnostic):r.errorCode+" "+r.message);
        for(const auto& tool:service_.tools())if(tool.id==r.toolId) {
            const auto preference=outputPreferences_.load({service_.project().id,service_.activeRecipe()->id,tool.id});
            if(!preference.error.isEmpty())lines<<"  Görünüm tercihi: "+preference.error;
            QMap<QString,OutputAvailabilityInfo> availability;
            for(const auto& port:outputPorts(tool.kind)) {
                const auto it=r.outputs.constFind(port.id);
                availability[port.id]=it==r.outputs.cend() ? OutputAvailabilityInfo{OutputAvailability::Unavailable,"OUTPUT_NOT_PRODUCED"} : OutputAvailabilityInfo{it->valid ? OutputAvailability::Available:OutputAvailability::Unavailable,it->invalidReason};
            }
            const auto visibility=OutputVisibilityPolicy::resolve(tool,preference.selection,availability);
            const auto visible=visibility.visiblePortIds();
            for(const auto& entry:visibility.ports)if(entry.visible) {
                if(entry.availability.state==OutputAvailability::Unavailable)lines<<"  "+entry.port.id+": kullanılamıyor · "+entry.availability.reason;
                else {const auto& v=r.outputs[entry.port.id];lines<<"  "+entry.port.id+(v.diagnostic ? " [diagnostic]: ":": ")+ToolResultCodec::display(v,4)+" "+v.port.unit;}
            }
            for(auto it=visibility.invalidOutputReasons.cbegin();it!=visibility.invalidOutputReasons.cend();++it)if(!visible.contains(it.key()))lines<<"  Uyarı · "+it.key()+": kullanılamıyor · "+it.value();
            if(visible.isEmpty())lines<<"  Sonuç çıkışları gizli (görüntü/overlay viewer'da korunur).";
        }
        if(r.toolId==selectedToolId_ && (editorSession_.sourceFrames.isEmpty() || (editorSession_.frame && editorSession_.sourceForTool(context,r.toolId)==editorSession_.frame->metadata().sourceId))) {
            if(r.state==ExecutionState::Done)edgeProfile_->setDiagnostics(r.edgeDiagnostics);
            const auto processed=r.outputs.value("image");if(r.state==ExecutionState::Done && processed.valid)if(const auto* image=std::get_if<ImageFrame>(&processed.data))if(!image->processing().isEmpty()){imageInfo_->setText(QString("İşlenmiş Gray8 • %1×%2 px • geometri korunur • %3").arg(image->size().width()).arg(image->size().height()).arg(QString::fromLatin1(image->processingId().toHex().left(12))));}
            QString annotation;if(r.outputs.contains("valuePx") || r.outputs.contains("widthPx") || (r.outputs.contains("linePx") || r.outputs.contains("circlePx")))annotation=r.message;annotation.replace(" • ","\n");
            // The world tracking status is already in Results. Avoid placing it
            // over the ROI frame/unit label at the image origin.
            for(const auto& t:service_.tools())if(t.id==r.toolId && isWorldPoseTracking(t))annotation.clear();
            if(r.calibration && r.state==ExecutionState::Done && r.outputs.value("value").valid)annotation=QString("%1 mm • %2").arg(std::get<double>(r.outputs.value("value").data),0,'f',3).arg(r.decision==PartDecision::Pass ? "PASS" : "FAIL");
            auto displayLines=r.lines;const auto pose=r.outputs.value("pose");
            if(r.state==ExecutionState::Done && pose.valid)if(const auto* actual=std::get_if<Pose2D>(&pose.data)) {
                displayLines.append(QLineF(actual->position,actual->position+QPointF(std::cos(actual->angleRadians),std::sin(actual->angleRadians))*20));
                annotation=QString("X %1 / Y %2 raw px\n+X angle %3 deg (Y-down)").arg(actual->position.x(),0,'f',2).arg(actual->position.y(),0,'f',2).arg(actual->angleRadians*180/std::numbers::pi,0,'f',2);
            }
            imageViewer_->setToolOverlay(r.regions,r.points,displayLines,annotation,r.mask.pixels,r.mask.size,r.mask.origin,r.rejectedPoints,(r.outputs.contains("linePx") || r.outputs.contains("circlePx")));const auto codes=r.outputs.value("codes");if(codes.valid)if(const auto* list=std::get_if<CodeResults>(&codes.data))imageViewer_->setCodeOverlay(*list);
        }
    }
    matchInfo_->setText(lines.join("\n"));
    // Preprocess/drawing may replace imageInfo above; keep provenance visible.
    statusBar()->showMessage((editorSession_.cycle->partial ? "Editor araç değerlendirmesi: " : "Editor çevrimi: ")+executionStateName(editorSession_.cycle->state)+" • "+editorSession_.cycle->cycleId);
}
void MainWindow::runFlow() {
    runExecution({});
}
void MainWindow::runExecution(const QString& targetTool) {
    const bool resumed=std::exchange(editorCameraResume_,false);
    if(runtimeJobs_.busy() || (runtime_ && runtime_->busy()) || editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Teaching) || editorSession_.busy(SessionJob::Image) || !applyToolProperties() || !service_.activeRecipe())return;
    const auto timing=AcquisitionTiming::request("editor");const auto commandedAt=AcquisitionTiming::Clock::now();AcquisitionTimingBinding timingBinding(timing);AcquisitionTimingSpan commandTiming("editor.user-command");
    if(targetTool.isEmpty() && measurementWorkspace_)try{const auto before=service_.activeRecipe()->definition;service_.refreshEditorCameraSources(measurementWorkspace_->environment());if(before!=service_.activeRecipe()->definition){editorSession_.reconcile(editorContext());refreshWorkspace();}}catch(const std::exception& e){matchInfo_->setText(QString::fromUtf8(e.what()));return;}
    bool needsAcquisition=targetTool.isEmpty();for(const auto& tool:service_.tools())if(tool.id==targetTool)needsAcquisition=ToolRegistry::get(tool.kind).acquisitionSource;
    if(needsAcquisition){
        syncEditorCameras(resumed?QString{}:targetTool.isEmpty()?QString("*"):targetTool,true);
        if(editorCameras_.pending(targetTool)){editorCameraPendingExecution_=targetTool;matchInfo_->setText("Kamera bağlantısı hazırlanıyor…");refreshFlow();return;}
    }
    findChild<QTabWidget*>("editorTabs")->setCurrentWidget(findChild<QScrollArea*>("resultsScroll"));
    const auto recipe=*service_.activeRecipe();const auto definition=recipe.definition;const auto catalogPath=service_.path()+".calibrations.json";const auto resolver=calibrationResolver(recipe);QJsonObject acquisitionPlan;try{if(recipe.definition["editor"].toObject()["schemaVersion"]==9){QJsonObject sources;for(const auto& t:editorTools(definition))if(t.enabled && ToolRegistry::get(t.kind).acquisitionSource && (targetTool.isEmpty() || t.id==targetTool)){const auto branch=SystemCalibration::branch(recipe,t.id);if(branch.definition.contains("sourceSetup")){if(!measurementWorkspace_)throw std::runtime_error("Measurement workspace unavailable");sources[t.id]=measurementWorkspace_->acquisition(branch);}}acquisitionPlan["sources"]=sources;}else if(recipe.definition.contains("sourceSetup")){if(!measurementWorkspace_)throw std::runtime_error("Measurement workspace unavailable");acquisitionPlan=measurementWorkspace_->acquisition(recipe);}}catch(const std::exception& e){matchInfo_->setText(QString::fromUtf8(e.what()));return;}
    bool acquisition=false;for(const auto& t:editorTools(definition))if(t.id==targetTool)acquisition=ToolRegistry::get(t.kind).acquisitionSource;
    if(!targetTool.isEmpty() && !acquisition) {
        try {
            const auto evaluation=editorSession_.evaluation(editorContext(),targetTool);
            const auto signature=flowSignature();const auto directory=QFileInfo(service_.path()).absolutePath();
            auto* watcher=editorSession_.watch<CycleResult>(this,SessionJob::Execution);const auto cancel=editorSession_.token(SessionJob::Execution);
            editorSession_.invalidateTool(editorContext(),targetTool);editorSession_.matchOutcome.reset();editorSession_.resultSignature.clear();refreshFlow();matchInfo_->setText("BUSY — yalnız seçili araç, mevcut ham görüntü…");
            connect(watcher,&QFutureWatcher<CycleResult>::finished,this,[this,watcher,signature,targetTool,timing,commandedAt]{
                AcquisitionTimingBinding binding(timing);AcquisitionTimingSpan delivery("editor.command-to-delivery",commandedAt);
                auto result=watcher->result();watcher->deleteLater();
                const bool accepted=editorSession_.acceptTool(std::move(result),signature,editorContext());delivery.finish(!accepted?"superseded":editorSession_.cycle->state==ExecutionState::Skipped?"cancelled":editorSession_.cycle->state==ExecutionState::Done?"success":"error");
                if(accepted) {
                    if(editorSession_.cycle->state!=ExecutionState::Skipped)for(const auto& r:editorSession_.cycle->tools)if(r.toolId==targetTool && r.state==ExecutionState::Done) {
                        const auto region=r.outputs.value("region"),score=r.outputs.value("score"),found=r.outputs.value("found");
                        if(std::holds_alternative<QRect>(region.data) && score.valid && found.valid) {
                            editorSession_.matchOutcome=MatchOutcome{std::get<QRect>(region.data),std::get<double>(score.data),r.milliseconds,std::get<bool>(found.data)};
                            editorSession_.resultSignature=matchSignature();
                            const auto point=r.outputs.value("point");if(point.valid && std::holds_alternative<QPointF>(point.data))editorSession_.references[targetTool]={referenceSignature(targetTool),std::get<QPointF>(point.data)};
                        }
                    }
                }else {editorSession_.matchOutcome.reset();matchInfo_->setText("Değişen girdi/ayar nedeniyle araç sonucu atıldı.");}
                refreshMatchOverlay();refreshFlow();refreshCalibration();
            });
            editorSession_.launch(watcher,[definition,directory,recipe,signature,cancel,targetTool,evaluation,resolver,timing]{AcquisitionTimingBinding binding(timing);return ExecutionService{}.runTool(definition,directory,recipe.id,signature,cancel,targetTool,evaluation,resolver);});
        }catch(const std::exception& e){matchInfo_->setText(QString::fromUtf8(e.what()));}
        return;
    }
    IndustrialExecutionInputs industrial;
    if(measurementWorkspace_){const auto e=measurementWorkspace_->environment();industrial.contract=e.current;
        for(const auto& t:editorTools(definition))if(t.enabled && t.kind=="industrial_camera"){const auto ref=t.parameters["sourceReference"].toObject();if(e.archivedCameras.contains(ref["cameraId"].toString()) || e.archivedSetups.contains(ref["setupId"].toString())){matchInfo_->setText("Arşivlenmiş kamera/kurulum edinimi reddedildi.");return;}}
        for(const auto& c:industrial.contract.cameras)if(c.physical)if(const auto route=CameraLocalConfiguration(settingsFile_).route(*c.physical))industrial.routes[c.id.value]=*route;
    }
    industrial=editorCameras_.executionInputs();
    const bool multi=definition["editor"].toObject()["schemaVersion"]==9;
    if(!multi || targetTool.isEmpty())++editorSession_.generation;

    if(multi && !targetTool.isEmpty())editorSession_.invalidateTool(editorContext(),targetTool);
    editorSession_.references.clear();invalidateMatch();editorSession_.cycle.reset();editorSession_.frame.reset();imageViewer_->clearImage("BUSY — görüntü ediniliyor…",true);imageInfo_->setText("Yeni çevrim görüntüsü bekleniyor.");auto* watcher=editorSession_.watch<CycleResult>(this,SessionJob::Execution);
    refreshFlow();matchInfo_->setText("BUSY — acquisition ve DAG çalışıyor…");
    findChild<QTabWidget*>("editorTabs")->setCurrentWidget(findChild<QScrollArea*>("resultsScroll"));
    const auto signature=flowSignature();const auto id=service_.activeRecipe()->id;const auto directory=QFileInfo(service_.path()).absolutePath();const auto cancel=editorSession_.token(SessionJob::Execution);
    connect(watcher,&QFutureWatcher<CycleResult>::finished,this,[this,watcher,signature,targetTool,timing,commandedAt]{
                AcquisitionTimingBinding binding(timing);AcquisitionTimingSpan delivery("editor.command-to-delivery",commandedAt);
        auto result=watcher->result();watcher->deleteLater();
        const bool accepted=editorSession_.acceptCycle(std::move(result),signature,editorContext());delivery.finish(!accepted?"superseded":editorSession_.cycle->state==ExecutionState::Skipped?"cancelled":editorSession_.cycle->state==ExecutionState::Done?"success":"error");
        if(accepted) {
            if(measurementWorkspace_ && service_.activeRecipe()){
                if(service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9){for(const auto& t:service_.tools())if(t.enabled && ToolRegistry::get(t.kind).acquisitionSource)measurementWorkspace_->observeAcquisition(SystemCalibration::branch(*service_.activeRecipe(),t.id),*editorSession_.cycle);}else measurementWorkspace_->observeAcquisition(*service_.activeRecipe(),*editorSession_.cycle);
            }
            if(service_.activeRecipe() && service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9 && !selectedToolId_.isEmpty())editorSession_.selectSource(editorSession_.sourceForTool(editorContext(),selectedToolId_));
            if(editorSession_.frame) {const auto& frame=*editorSession_.frame;imageViewer_->setImage(frame.preview(),editorRecipeId_+"|"+frame.metadata().sourceId);imageInfo_->setText(QString("%1×%2 px | Frame %3").arg(frame.size().width()).arg(frame.size().height()).arg(editorSession_.cycle->frameId));}
            if(!targetTool.isEmpty() && editorSession_.cycle->state!=ExecutionState::Skipped)for(const auto& r:editorSession_.cycle->tools)if(r.toolId==targetTool && r.state==ExecutionState::Done) {
                const auto region=r.outputs.value("region"),score=r.outputs.value("score"),found=r.outputs.value("found");
                if(std::holds_alternative<QRect>(region.data) && score.valid && found.valid) {
                    editorSession_.matchOutcome=MatchOutcome{std::get<QRect>(region.data),std::get<double>(score.data),r.milliseconds,std::get<bool>(found.data)};
                    editorSession_.resultSignature=matchSignature();
                }
            }
        } else {editorSession_.cycle.reset();matchInfo_->setText("READY — değişen ayar/görüntü nedeniyle çevrim sonucu atıldı.");}
        refreshMatchOverlay();refreshFlow();refreshCalibration();
    });
    editorSession_.launch(watcher,[definition,directory,id,signature,cancel,targetTool,resolver,acquisitionPlan,industrial,timing,commandedAt]{AcquisitionTimingBinding binding(timing);AcquisitionTiming::record(timing,"editor.worker-queue",commandedAt);return ExecutionService(industrial).run(definition,directory,id,signature,cancel,targetTool,{},resolver,nullptr,acquisitionPlan);});
}
void MainWindow::loadImageAsync(const QString& path,bool commit) {
    editorSession_.references.clear();
    const auto generation=++editorSession_.generation;const auto recipe=editorRecipeId_;const auto signature=flowSignature();
    auto* watcher=editorSession_.watch<ImageLoad>(this,SessionJob::Image);
    connect(watcher,&QFutureWatcher<ImageLoad>::finished,this,[this,watcher,generation,recipe,signature,commit]{
        const auto result=watcher->result();watcher->deleteLater();
        if(generation!=editorSession_.generation || recipe!=editorRecipeId_ || signature!=flowSignature()){
            if(generation==editorSession_.generation && recipe==editorRecipeId_){imageBinding_.clear();refreshWorkspace();}else refreshFlow();return;
        }
        if(result.frame)try {
            if(commit){const bool multi=service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9;const auto source=multi ? editorSession_.sourceForTool(editorContext(),selectedToolId_):QString{};if(multi && source.isEmpty())throw std::runtime_error("Önce dosya kaynağını seçin.");selectedToolId_=service_.setImageFile(result.frame->path(),source);}
            auto loaded=*result.frame;if(service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9){auto m=loaded.metadata();m.sourceId=selectedToolId_;loaded=loaded.withMetadata(m);}
            editorSession_.frame=loaded;invalidateMatch();editorSession_.cycle.reset();editorSession_.adoptFrame(editorContext());
            imageBinding_=editorRecipeId_+"|"+service_.imageFilePath();imageViewer_->setImage(editorSession_.frame->preview(),editorRecipeId_+"|"+editorSession_.frame->metadata().sourceId);
            imageInfo_->setText(QString("%1 × %2 px | %3 | %4").arg(editorSession_.frame->size().width()).arg(editorSession_.frame->size().height()).arg(editorSession_.frame->pixelFormat(),QFileInfo(editorSession_.frame->path()).fileName()));refreshWorkspace();
        }catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
        else displayError(result.error);
        refreshFlow();refreshMatchOverlay();
    });
    editorSession_.launch(watcher,[path]{try{return EditorSession::load(path,AcquisitionPolicy::replay(path));}catch(const std::exception& e){return ImageLoad{{},QString::fromUtf8(e.what())};}});
    // Smoke assertions run synchronously, but decoding still takes place on the worker.
    if(smokeMode_) {QEventLoop loop;connect(watcher,&QFutureWatcher<ImageLoad>::finished,&loop,&QEventLoop::quit);loop.exec();}
}
}

