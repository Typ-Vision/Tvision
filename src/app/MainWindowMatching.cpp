#include "application/RoiService.hpp"
#include "application/CircleToolService.hpp"
#include "application/MetrologyService.hpp"
#include "application/CalibrationWorkflow.hpp"
#include "MainWindow.hpp"
#include "PoseMatchEditor.hpp"
#include "application/PoseRoiService.hpp"
#include "application/ReferenceService.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "ImageViewer.hpp"
#include "EdgeProfileView.hpp"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QSet>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QScrollArea>
#include <QTimer>
#include <QDialog>
namespace visionflow {
QByteArray MainWindow::referenceSignature(const QString& tool) const {return editorSession_.referenceSignature(editorContext(),tool);}
std::optional<QPointF> MainWindow::referencePoint(const QString& source) const {return editorSession_.referencePoint(editorContext(),source);}
QByteArray MainWindow::matchSignature() const {
    if (!service_.activeRecipe() || !editorSession_.frame) return {};
    return service_.project().id.toUtf8()+service_.activeRecipe()->id.toUtf8()+selectedToolId_.toUtf8()+QByteArray::number(editorSession_.generation)+QJsonDocument(service_.activeRecipe()->definition).toJson(QJsonDocument::Compact);
}
void MainWindow::invalidateMatch() {if(edgeProfile_)edgeProfile_->setDiagnostics({});editorSession_.invalidateResults(); if(imageViewer_)imageViewer_->setToolOverlay({},{});if (matchInfo_) matchInfo_->setText("Eşleştirme sonucu yok.");}
void MainWindow::refreshMatchOverlay() {refreshMatchOverlayView(editorContext());}
void MainWindow::refreshMatchOverlayView(const EditorContext& context) {
    if(drawingImage_ && imageViewer_->drawingActive()){
        try{const auto current=editorSession_.toolImage(context,selectedToolId_);const auto& a=current.metadata();const auto& b=drawingImage_->metadata();
            if(current.contentId()==drawingImage_->contentId() && current.processingId()==drawingImage_->processingId() && current.geometry()==drawingImage_->geometry() && a.sourceId==b.sourceId && a.frameId==b.frameId && a.cycleId==b.cycleId)return;
        }catch(const std::exception&){}
        imageViewer_->cancelRoi();imageViewer_->drawRegion(false);drawingImage_.reset();
    }
    if (editorSession_.matchOutcome && editorSession_.resultSignature!=matchSignature()) invalidateMatch();
    imageViewer_->onRoiEdited={};
    imageViewer_->setScanGuides({});
    const auto* geometryDialog=findChild<QDialog*>("roiGeometryDialog");
    const bool geometryWorkspace=geometryDialog && geometryDialog->isVisible();
    std::optional<ImageFrame> inputImage;
    if(service_.activeRecipe())try{inputImage=editorSession_.toolImage(context,selectedToolId_);}catch(const std::exception&){}
    const auto localImage=inputImage ? inputImage:editorSession_.frame;
    bool cropOutput=false;
    if(!drawingImage_ && editorSession_.cycle && editorSession_.cycle->configId==editorSession_.signature(context))for(const auto& t:service_.tools())if(t.id==selectedToolId_ && t.kind=="roi" && t.parameters["cropImage"].toBool())for(const auto& r:editorSession_.cycle->tools)if(r.toolId==t.id && r.state==ExecutionState::Done && r.outputs.value("image").valid)cropOutput=true;
    bool bound=false;
    if(!geometryWorkspace && service_.activeRecipe() && editorSession_.frame)for(const auto& t:service_.tools())if(t.id==selectedToolId_ && (t.parameters.contains("referenceBinding") || t.parameters.contains("poseTracking") || t.kind=="edge_point" || t.kind=="edge_pair" || t.kind=="line_fit" || t.kind=="circle_fit")){
        bound=true;const auto id=t.id;
        imageViewer_->setScanGuides([kind=t.kind,p=t.parameters](const ToolRoi& roi){return MetrologyService::scanGuides(kind,p,roi);});
        if(t.parameters.contains("referenceBinding")){
            imageViewer_->setRoi({},false);imageViewer_->setRoiEditable(false);
            try{
                const auto b=toolReferenceBinding(t);const auto e=editorSession_.evaluation(context);const auto image=editorSession_.toolImage(context,id);
                if(!e.cache.contains(b.sourceTool))throw std::runtime_error("Current reference unavailable");
                for(const auto& source:service_.tools())if(source.id==b.sourceTool){
                    if(b.world){const auto ctx=calibrationResolver(*service_.activeRecipe())(image);const auto r=ReferenceService::resolveWorld(b,source,e.cache[b.sourceTool].result,image,ctx);
                        if(!r.map)throw std::runtime_error(r.reason.toUtf8().constData());imageViewer_->setRoiMapping(r.map->world,r.map->nominalToImage,{});imageViewer_->setRoi(b.nominal,false);
                    }else {const auto r=ReferenceService::resolve(b,source,e.cache[b.sourceTool].result,image);if(!r.roi)throw std::runtime_error(r.reason.toUtf8().constData());imageViewer_->setRoiMapping({image.metadata().coordinateSystemId});imageViewer_->setRoi(r.roi,false);}
                }
            }catch(const std::exception&){imageViewer_->setRoi({},false);}
            imageViewer_->setRoiEditable(false);
            continue;
        }
        if(t.parameters.contains("poseTracking")) {
            try {
                const auto evaluation=editorSession_.evaluation(context);const auto image=editorSession_.toolImage(context,id);const auto sourceId=trackingSourceTool(t);
                if(!evaluation.cache.contains(sourceId))throw std::runtime_error("Current pose unavailable");
                for(const auto& source:service_.tools())if(source.id==sourceId){
                    if(isWorldPoseTracking(t)) {
                        const auto ctx=calibrationResolver(*service_.activeRecipe())(image);
                        const auto resolved=PoseRoiService::resolveWorld(worldPoseTracking(t),source,evaluation.cache[sourceId].result,image,ctx);
                        if(!resolved.map)throw std::runtime_error(resolved.reason.toUtf8().constData());
                        imageViewer_->setRoiMapping(resolved.map->world,resolved.map->nominalToImage,{});imageViewer_->setRoi(ToolRoi::fromJson(t.parameters["scanRoi"].toObject()),false);
                    }else {
                        const auto resolved=PoseRoiService::resolve(poseTrackingBinding(t),source,evaluation.cache[sourceId].result,image);
                        if(!resolved.roi)throw std::runtime_error(resolved.reason.toUtf8().constData());
                        imageViewer_->setRoiMapping({image.metadata().coordinateSystemId});imageViewer_->setRoi(resolved.roi,false);
                    }
                }
            }catch(const std::exception&){imageViewer_->setRoi({},false);}
            imageViewer_->setRoiEditable(false);continue;
        }
        if(t.kind!="circle_fit" || t.parameters["mode"]!="calibrated")imageViewer_->setRoiMapping({localImage->metadata().coordinateSystemId});
        else try{const auto frame=*localImage;const auto ctx=calibrationResolver(*service_.activeRecipe())(frame);if(!ctx.calibration || !ctx.current)throw std::runtime_error("Current Circle calibration unavailable");imageViewer_->setRoiMapping(ctx.calibration->dependency().target,[frame,ctx](QPointF p){return CircleToolService::mapPoint(frame,ctx,p,true);},[frame,ctx](QPointF p){return CircleToolService::mapPoint(frame,ctx,p,false);});}catch(const std::exception&){imageViewer_->setRoiMapping({localImage->metadata().coordinateSystemId});}
        const auto json=t.parameters["scanRoi"].toObject();
        try{imageViewer_->setRoi(json.isEmpty() ? std::optional<ToolRoi>{}:std::optional<ToolRoi>{ToolRoi::fromJson(json)},false);}catch(const std::exception&){imageViewer_->setRoi({},false);}
        imageViewer_->onRoiEdited=[this,id,line=t.kind=="line_fit",circle=t.kind=="circle_fit"]{
            if(selectedToolId_!=id || editorSession_.busy())return;
            const auto roi=imageViewer_->roi();if(roi && (circle ? !(std::holds_alternative<Annulus>(roi->geometry) || std::holds_alternative<ArcRoi>(roi->geometry)):line ? !std::holds_alternative<RotatedRectangle>(roi->geometry):!std::holds_alternative<ScanSegment>(roi->geometry)))return;
            QTimer::singleShot(0,this,[this,id,roi]{try{if(selectedToolId_!=id || editorSession_.busy() || !editorSession_.frame || (roi && roi->frame.unit==LengthUnit::Pixel && roi->frame.id!=editorSession_.toolImage(editorContext(),id).metadata().coordinateSystemId) || !applyToolProperties())return;for(auto tool:service_.tools())if(tool.id==id){tool.parameters["scanRoi"]=roi ? roi->toJson():QJsonObject{};if(tool.kind=="circle_fit"){tool.parameters.remove("worldCalibration");if(roi && roi->frame.unit==LengthUnit::Millimetre){const auto ctx=calibrationResolver(*service_.activeRecipe())(editorSession_.toolImage(editorContext(),id));if(ctx.calibration && ctx.current && ctx.calibration->systemBound())tool.parameters["worldCalibration"]=ctx.current->toJson();}}service_.updateTool(tool);refreshWorkspace();break;}}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}});
        };
    }
    if(!bound && !geometryWorkspace){imageViewer_->setRoiMapping({editorSession_.frame ? localImage->metadata().coordinateSystemId:QString("viewer/raw")});imageViewer_->setRoi({},false);}imageViewer_->setRoiRecipeBound(bound);
    QRect model,search,result; bool enabled=false;QString searchIdentity;bool searchEditable=false;
    imageViewer_->onSearchRegionEdited={};
    if (service_.activeRecipe()) for (const auto& t : service_.tools()) if (t.id==selectedToolId_ && (t.kind=="roi" || t.kind=="template_match" || t.kind=="pattern_pose" || t.kind=="shape_pose" || t.kind=="blob" || t.kind=="edge" || t.kind=="code_read")) {
        model=MatchService::rectangle(t.parameters["modelRoi"].toObject()); search=MatchService::rectangle(t.parameters["searchRoi"].toObject()); enabled=t.enabled && bool(inputImage);
        if(t.parameters.contains("poseTracking") || t.parameters.contains("referenceBinding")){search={};enabled=false;}
        if(t.kind=="edge" && !t.parameters.contains("referenceBinding") && !search.isEmpty())for(const auto& c:editorConnections(service_.activeRecipe()->definition))if(c.targetTool==t.id && c.targetPort=="reference") {
            const auto point=editorSession_.referencePoint(context,c.sourceTool);const auto origin=t.parameters["referenceOrigin"].toObject();
            for(const auto& source:service_.tools())if(source.id==c.sourceTool && referenceNeedsTeaching(t,source))search={};
            if(point && !origin.isEmpty() && !search.isEmpty())search.translate(qRound(point->x()-origin["x"].toDouble()),qRound(point->y()-origin["y"].toDouble()));
        }
        if(!t.parameters.contains("poseTracking") && !t.parameters.contains("referenceBinding") && editorSession_.cycle && editorSession_.cycle->configId==editorSession_.signature(context))for(const auto& r:editorSession_.cycle->tools)if(r.toolId==t.id && !r.searchRegion.isEmpty())search=r.searchRegion;
        if(t.kind=="blob" || t.kind=="roi"){
            bool externallyBound=t.parameters.contains("poseTracking") || t.parameters.contains("referenceBinding");
            for(const auto& c:editorConnections(service_.activeRecipe()->definition))if(c.targetTool==t.id && (c.targetPort=="region" || c.targetPort=="reference" || c.targetPort=="poseReference"))externallyBound=true;
            if(externallyBound)enabled=false;
            try{
                const auto image=editorSession_.toolImage(context,t.id);
                searchIdentity=service_.project().id+"/"+service_.activeRecipe()->id+"/"+t.id+"/"+image.contentId().toHex()+"/"+image.processingId().toHex()+"/"+image.metadata().sourceId+"/"+image.metadata().frameId+"/"+image.metadata().coordinateSystemId;
                searchEditable=enabled && !cropOutput && !externallyBound && !editorSession_.busy() && !geometryWorkspace;
                if(searchEditable){
                    // A full-frame execution searchRegion is not a saved searchRoi.
                    search=MatchService::rectangle(t.parameters["searchRoi"].toObject());
                    imageViewer_->onSearchRegionEdited=[this,id=t.id,identity=searchIdentity](QRect roi){
                        if(editorSession_.busy() || selectedToolId_!=id || !service_.activeRecipe())return;
                        try{
                            const auto current=editorSession_.toolImage(editorContext(),id);
                            if(identity!=service_.project().id+"/"+service_.activeRecipe()->id+"/"+id+"/"+current.contentId().toHex()+"/"+current.processingId().toHex()+"/"+current.metadata().sourceId+"/"+current.metadata().frameId+"/"+current.metadata().coordinateSystemId)return;
                            for(auto tool:service_.tools())if(tool.id==id){
                                tool.parameters=roi.isEmpty() ? tool.parameters:tool.kind=="roi" ? RoiService::teach(current,roi,tool.parameters["cropImage"].toBool()):editorSession_.taughtSearch(editorContext(),tool,roi);
                                if(roi.isEmpty())tool.parameters["searchRoi"]=QJsonObject{};
                                service_.updateTool(tool);invalidateMatch();refreshWorkspace();break;
                            }
                        }catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));refreshMatchOverlay();}
                    };
                }
            }catch(const std::exception&){searchEditable=false;}
        }
    }
    if(cropOutput){model={};search={};searchEditable=false;}
    if (editorSession_.matchOutcome) result=editorSession_.matchOutcome->rectangle;
    imageViewer_->setSearchRegionEditing(searchIdentity,search,searchEditable);
    imageViewer_->setRegions(model,search,result,!editorSession_.matchOutcome || editorSession_.matchOutcome->accepted);
    for (const auto name : {"modelRoi","searchRoi","clearSearch","runMatch"}) if (auto* b=findChild<QPushButton*>(name)) b->setEnabled(enabled && !editorSession_.busy(SessionJob::Teaching) && !editorSession_.busy(SessionJob::Execution) && !editorSession_.busy(SessionJob::Image));
}
void MainWindow::beginRoi(bool model) {
    if (editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Image) || editorSession_.busy(SessionJob::Teaching) || !applyToolProperties() || !editorSession_.frame) return;
    for(const auto& t:service_.tools())if(t.id==selectedToolId_ && (t.parameters.contains("poseTracking") || t.parameters.contains("referenceBinding")))return;
    for(const auto& t:service_.tools())if(t.id==selectedToolId_ && t.kind=="blob")for(const auto& c:editorConnections(service_.activeRecipe()->definition))if(c.targetTool==t.id && (c.targetPort=="region" || c.targetPort=="reference" || c.targetPort=="poseReference"))return;
    if(!setDrawingImage())return;
    modelDrawing_=model; drawingTool_=selectedToolId_; imageViewer_->drawRegion(true);
    matchInfo_->setText(model ? "Model ROI: görüntü üzerinde sürükleyin (sarı)." : "Search ROI: görüntü üzerinde sürükleyin (mavi).");
}
void MainWindow::commitRoi(const QRect& roi) {
    if (editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Image) || editorSession_.busy(SessionJob::Teaching) || drawingTool_!=selectedToolId_ || !editorSession_.frame) return;
    try {
        for(auto tool:service_.tools())if(tool.id==selectedToolId_ && tool.kind=="roi" && tool.enabled){
            const auto current=editorSession_.toolImage(editorContext(),tool.id);
            if(!drawingImage_ || current.contentId()!=drawingImage_->contentId() || current.processingId()!=drawingImage_->processingId() || RoiService::imageBinding(current)!=RoiService::imageBinding(*drawingImage_) || current.metadata().frameId!=drawingImage_->metadata().frameId || current.metadata().cycleId!=drawingImage_->metadata().cycleId)throw std::runtime_error("Çizim görüntüsü değişti. ROI'yi yeniden çizin.");
            tool.parameters=RoiService::teach(current,roi,tool.parameters["cropImage"].toBool());service_.updateTool(tool);invalidateMatch();refreshWorkspace();return;
        }
        for(auto tool:service_.tools())if(tool.id==selectedToolId_ && (tool.kind=="pattern_pose" || tool.kind=="shape_pose") && tool.enabled) {
            if(!modelDrawing_){tool.parameters["searchRoi"]=MatchService::rectangle(roi);service_.updateTool(tool);refreshWorkspace();return;}
            const auto image=editorSession_.toolImage(editorContext(),tool.id);
            auto* settings=dynamic_cast<PoseMatchEditor*>(propertiesBody_->findChild<QWidget*>("poseMatchSettings"));
            if(!settings)throw std::runtime_error("Pose teaching settings unavailable");
            const auto anchor=settings->teachingAnchor();const auto signature=flowSignature();
            auto* watcher=editorSession_.watch<PoseTeaching>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);
            refreshMatchOverlay();refreshFlow();matchInfo_->setText("Pose modeli öğretiliyor…");
            connect(watcher,&QFutureWatcher<PoseTeaching>::finished,this,[this,watcher,signature,tool,cancel,image]() mutable {
                const auto work=watcher->result();watcher->deleteLater();
                if(!cancel->load() && signature==flowSignature()) {
                    if(!work.error.isEmpty())displayError(work.error);
                    else try{
                        const auto current=editorSession_.toolImage(editorContext(),tool.id);
                        if(current.contentId()==image.contentId() && current.processingId()==image.processingId()){tool.parameters=work.parameters;service_.updateTool(tool);refreshWorkspace();}
                    }catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
                }
                refreshMatchOverlay();refreshFlow();
            });
            editorSession_.launch(watcher,[tool,image,roi,anchor]{return EditorSession::teachPose(tool,image,roi,anchor);});return;
        }
        for (auto tool : service_.tools()) if (tool.id==selectedToolId_ && (tool.kind=="template_match" || (!modelDrawing_ && (tool.kind=="blob" || tool.kind=="edge" || tool.kind=="code_read"))) && tool.enabled) {
            if (modelDrawing_) {
                const auto signature=matchSignature(); const auto frame=editorSession_.toolImage(editorContext(),tool.id);
                auto* watcher=editorSession_.watch<ModelTeaching>(this,SessionJob::Teaching); invalidateMatch(); refreshMatchOverlay(); matchInfo_->setText("Model öğretiliyor…");
                connect(watcher,&QFutureWatcher<ModelTeaching>::finished,this,[this,watcher,signature,tool,roi]() mutable {
                    const auto work=watcher->result(); watcher->deleteLater();
                    if (signature==matchSignature()) {
                        if (!work.error.isEmpty()) displayError(work.error);
                        else try {
                            tool.parameters["modelRoi"]=MatchService::rectangle(roi); tool.parameters["modelPng"]=work.model;
                            service_.updateTool(tool); refreshWorkspace();
                        } catch (const std::exception& error) {displayError(QString::fromUtf8(error.what()));}
                    }
                    invalidateMatch(); refreshMatchOverlay();
                });
                editorSession_.launch(watcher,[frame,roi]{return EditorSession::teach(frame,roi);}); return;
            } else {
                if(tool.kind=="code_read")tool.parameters["searchRoi"]=MatchService::rectangle(roi);
                else tool.parameters=editorSession_.taughtSearch(editorContext(),tool,roi);
            }
            service_.updateTool(tool); invalidateMatch(); refreshWorkspace(); return;
        }
    } catch (const std::exception& error) {displayError(QString::fromUtf8(error.what())); refreshMatchOverlay();}
}
void MainWindow::runMatch() {
    if(!service_.activeRecipe())return;
    for(const auto& tool:service_.tools())if(tool.id==selectedToolId_ && tool.kind=="template_match") {
        runExecution(tool.id);return;
    }
}
}
