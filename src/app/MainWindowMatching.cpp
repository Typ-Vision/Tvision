#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QSet>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QScrollArea>
namespace visionflow {
QByteArray MainWindow::referenceSignature(const QString& tool) const {return editorSession_.referenceSignature(editorContext(),tool);}
std::optional<QPointF> MainWindow::referencePoint(const QString& source) const {return editorSession_.referencePoint(editorContext(),source);}
QByteArray MainWindow::matchSignature() const {
    if (!service_.activeRecipe() || !editorSession_.frame) return {};
    return service_.project().id.toUtf8()+service_.activeRecipe()->id.toUtf8()+selectedToolId_.toUtf8()+QByteArray::number(editorSession_.generation)+QJsonDocument(service_.activeRecipe()->definition).toJson(QJsonDocument::Compact);
}
void MainWindow::invalidateMatch() {editorSession_.invalidateResults(); if(imageViewer_)imageViewer_->setToolOverlay({},{});if (matchInfo_) matchInfo_->setText("Eşleştirme sonucu yok.");}
void MainWindow::refreshMatchOverlay() {
    if (editorSession_.matchOutcome && editorSession_.resultSignature!=matchSignature()) invalidateMatch();
    QRect model,search,result; bool enabled=false;
    if (service_.activeRecipe()) for (const auto& t : service_.tools()) if (t.id==selectedToolId_ && (t.kind=="template_match" || t.kind=="blob" || t.kind=="edge")) {
        model=MatchService::rectangle(t.parameters["modelRoi"].toObject()); search=MatchService::rectangle(t.parameters["searchRoi"].toObject()); enabled=t.enabled && bool(editorSession_.frame);
        if(t.kind=="edge" && !search.isEmpty())for(const auto& c:editorConnections(service_.activeRecipe()->definition))if(c.targetTool==t.id && c.targetPort=="reference") {
            const auto point=referencePoint(c.sourceTool);const auto origin=t.parameters["referenceOrigin"].toObject();
            for(const auto& source:service_.tools())if(source.id==c.sourceTool && referenceNeedsTeaching(t,source))search={};
            if(point && !origin.isEmpty() && !search.isEmpty())search.translate(qRound(point->x()-origin["x"].toDouble()),qRound(point->y()-origin["y"].toDouble()));
        }
        if(editorSession_.cycle && editorSession_.cycle->configId==flowSignature())for(const auto& r:editorSession_.cycle->tools)if(r.toolId==t.id && !r.searchRegion.isEmpty())search=r.searchRegion;
    }
    if (editorSession_.matchOutcome) result=editorSession_.matchOutcome->rectangle;
    imageViewer_->setRegions(model,search,result,!editorSession_.matchOutcome || editorSession_.matchOutcome->accepted);
    for (const auto name : {"modelRoi","searchRoi","clearSearch","runMatch"}) if (auto* b=findChild<QPushButton*>(name)) b->setEnabled(enabled && !editorSession_.busy(SessionJob::Teaching) && !editorSession_.busy(SessionJob::Execution) && !editorSession_.busy(SessionJob::Image));
}
void MainWindow::beginRoi(bool model) {
    if (editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Image) || editorSession_.busy(SessionJob::Teaching) || !applyToolProperties() || !editorSession_.frame) return;
    modelDrawing_=model; drawingTool_=selectedToolId_; imageViewer_->drawRegion(true);
    matchInfo_->setText(model ? "Model ROI: görüntü üzerinde sürükleyin (sarı)." : "Search ROI: görüntü üzerinde sürükleyin (mavi).");
}
void MainWindow::commitRoi(const QRect& roi) {
    if (editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Image) || editorSession_.busy(SessionJob::Teaching) || drawingTool_!=selectedToolId_ || !editorSession_.frame) return;
    try {
        for (auto tool : service_.tools()) if (tool.id==selectedToolId_ && (tool.kind=="template_match" || (!modelDrawing_ && (tool.kind=="blob" || tool.kind=="edge"))) && tool.enabled) {
            if (modelDrawing_) {
                const auto signature=matchSignature(); const auto frame=*editorSession_.frame;
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
                tool.parameters=editorSession_.taughtSearch(editorContext(),tool,roi);
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
