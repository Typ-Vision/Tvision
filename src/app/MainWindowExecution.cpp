#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTabWidget>
#include <QScrollArea>
#include <QEventLoop>
#include <QStatusBar>
#include <algorithm>
namespace visionflow {
EditorContext MainWindow::editorContext() const {
    if(!service_.activeRecipe())return {};
    return {service_.project().id,service_.path(),service_.activeRecipe()->id,service_.activeRecipe()->definition};
}
QByteArray MainWindow::flowSignature() const {return editorSession_.signature(editorContext());}
void MainWindow::refreshFlow() {
    if(editorSession_.cycle && editorSession_.cycle->configId!=flowSignature()) {editorSession_.cycle.reset();matchInfo_->setText("READY — ayar/görüntü değişti, çevrim sonucu yok.");}
    QString text="Önce reçete açın.";
    if(service_.activeRecipe()) {
        const auto graph=validateGraph(service_.tools(),editorConnections(service_.activeRecipe()->definition));
        text=graph.valid() ? "DAG geçerli • "+QString(editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Image) ? "BUSY" : "READY") : graph.errors.join(" | ");
    }
    if(service_.activeRecipe())for(const auto& c:editorConnections(service_.activeRecipe()->definition)) {
        QString source=c.sourceTool,target=c.targetTool;
        for(const auto& t:service_.tools()){if(t.id==source)source=t.name;if(t.id==target)target=t.name;}
        text+="\n"+source+"."+c.sourcePort+" → "+target+"."+c.targetPort;
    }
    graphInfo_->setText(text);
    findChild<QPushButton*>("runFlow")->setEnabled(service_.activeRecipe() && !editorSession_.busy(SessionJob::Execution) && !editorSession_.busy(SessionJob::Teaching) && !editorSession_.busy(SessionJob::Image));
    findChild<QPushButton*>("stopFlow")->setEnabled(editorSession_.busy(SessionJob::Execution));
    for(int i=0;i<flowTools_->count();++i) {
        auto* item=flowTools_->item(i);QString state=editorSession_.busy(SessionJob::Execution) ? "BUSY" : "READY";
        if(editorSession_.cycle)for(const auto& r:editorSession_.cycle->tools)if(r.toolId==item->data(Qt::UserRole).toString())state=executionStateName(r.state)+QString(" %1 ms").arg(r.milliseconds,0,'f',2);
        item->setToolTip(state);
        for(const auto& t:service_.tools())if(t.id==item->data(Qt::UserRole).toString())item->setText(t.name+" • "+toolLabel(t.kind)+" / "+state);
    }
    showExecutionResult();
}
void MainWindow::showExecutionResult() {
    imageViewer_->setToolOverlay({},{});
    if(!editorSession_.cycle)return;
    QStringList lines;lines<<QString("%1 | Cycle %2 | Frame %3 | %4 ms").arg(executionStateName(editorSession_.cycle->state),editorSession_.cycle->cycleId,editorSession_.cycle->frameId).arg(editorSession_.cycle->milliseconds,0,'f',2);
    if(!editorSession_.cycle->error.isEmpty())lines<<editorSession_.cycle->error;
    auto ordered=editorSession_.cycle->tools;
    std::stable_sort(ordered.begin(),ordered.end(),[this](const auto& a,const auto& b){return a.toolId==selectedToolId_ && b.toolId!=selectedToolId_;});
    for(const auto& r:ordered) {
        QString name=r.toolId;for(const auto& t:service_.tools())if(t.id==r.toolId)name=t.name;
        lines<<name+" • "+executionStateName(r.state)+QString(" • %1 ms ").arg(r.milliseconds,0,'f',2)+r.message;
        for(auto it=r.outputs.begin();it!=r.outputs.end();++it) {
            const auto& v=it.value();if(!v.valid) {lines<<"  "+it.key()+": geçersiz";continue;}
            QString value;
            if(auto* n=std::get_if<double>(&v.data))value=QString::number(*n,'f',4);
            else if(auto* b=std::get_if<bool>(&v.data))value=*b ? "true" : "false";
            else if(auto* p=std::get_if<QPointF>(&v.data))value=QString("(%1, %2)").arg(p->x()).arg(p->y());
            else if(auto* p=std::get_if<QRect>(&v.data))value=QString("(%1, %2) %3×%4").arg(p->x()).arg(p->y()).arg(p->width()).arg(p->height());
            else if(auto* f=std::get_if<ImageFrame>(&v.data))value=QString("%1×%2").arg(f->size().width()).arg(f->size().height());
            lines<<"  "+it.key()+": "+value+" "+v.port.unit;
        }
        if(r.toolId==selectedToolId_) {
            imageViewer_->setToolOverlay(r.regions,r.points,r.lines);
        }
    }
    matchInfo_->setText(lines.join("\n"));
    statusBar()->showMessage("Editor çevrimi: "+executionStateName(editorSession_.cycle->state)+" • "+editorSession_.cycle->cycleId);
}
void MainWindow::runFlow() {
    runExecution({});
}
void MainWindow::runExecution(const QString& targetTool) {
    if(editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Teaching) || editorSession_.busy(SessionJob::Image) || !applyToolProperties() || !service_.activeRecipe())return;
    const auto definition=service_.activeRecipe()->definition;
    editorSession_.references.clear();invalidateMatch();editorSession_.cycle.reset();editorSession_.frame.reset();imageViewer_->clearImage("BUSY — görüntü ediniliyor…");imageInfo_->setText("Yeni çevrim görüntüsü bekleniyor.");auto* watcher=editorSession_.watch<CycleResult>(this,SessionJob::Execution);
    refreshFlow();matchInfo_->setText("BUSY — acquisition ve DAG çalışıyor…");
    findChild<QTabWidget*>("editorTabs")->setCurrentWidget(findChild<QScrollArea*>("resultsScroll"));
    const auto signature=flowSignature();const auto id=service_.activeRecipe()->id;const auto directory=QFileInfo(service_.path()).absolutePath();const auto cancel=editorSession_.token(SessionJob::Execution);
    connect(watcher,&QFutureWatcher<CycleResult>::finished,this,[this,watcher,signature,targetTool]{
        auto result=watcher->result();watcher->deleteLater();
        if(editorSession_.acceptCycle(std::move(result),signature,editorContext())) {
            if(editorSession_.frame) {const auto& frame=*editorSession_.frame;imageViewer_->setImage(frame.preview());imageInfo_->setText(QString("%1×%2 px | Frame %3").arg(frame.size().width()).arg(frame.size().height()).arg(editorSession_.cycle->frameId));}
            if(!targetTool.isEmpty() && editorSession_.cycle->state!=ExecutionState::Skipped)for(const auto& r:editorSession_.cycle->tools)if(r.toolId==targetTool && r.state==ExecutionState::Done) {
                const auto region=r.outputs.value("region"),score=r.outputs.value("score"),found=r.outputs.value("found");
                if(std::holds_alternative<QRect>(region.data) && score.valid && found.valid) {
                    editorSession_.matchOutcome=MatchOutcome{std::get<QRect>(region.data),std::get<double>(score.data),r.milliseconds,std::get<bool>(found.data)};
                    editorSession_.resultSignature=matchSignature();
                }
            }
        } else {editorSession_.cycle.reset();matchInfo_->setText("READY — değişen ayar/görüntü nedeniyle çevrim sonucu atıldı.");}
        refreshMatchOverlay();refreshFlow();
    });
    editorSession_.launch(watcher,[definition,directory,id,signature,cancel,targetTool]{return ExecutionService{}.run(definition,directory,id,signature,cancel,targetTool);});
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
            if(commit)selectedToolId_=service_.setImageFile(result.frame->path());
            editorSession_.frame=result.frame;invalidateMatch();editorSession_.cycle.reset();
            imageBinding_=editorRecipeId_+"|"+service_.imageFilePath();imageViewer_->setImage(editorSession_.frame->preview());
            imageInfo_->setText(QString("%1 × %2 px | %3 | %4").arg(editorSession_.frame->size().width()).arg(editorSession_.frame->size().height()).arg(editorSession_.frame->pixelFormat(),QFileInfo(editorSession_.frame->path()).fileName()));refreshWorkspace();
        }catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
        else displayError(result.error);
        refreshFlow();refreshMatchOverlay();
    });
    editorSession_.launch(watcher,[path]{return EditorSession::load(path);});
    // Smoke assertions run synchronously, but decoding still takes place on the worker.
    if(smokeMode_) {QEventLoop loop;connect(watcher,&QFutureWatcher<ImageLoad>::finished,&loop,&QEventLoop::quit);loop.exec();}
}
}

