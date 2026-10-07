#include "MainWindow.hpp"
#include "ImageViewer.hpp"
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
namespace visionflow {
namespace {
QLabel* plain(const QString& value){auto* label=new QLabel(value);label->setWordWrap(true);label->setTextFormat(Qt::PlainText);return label;}
}
QWidget* MainWindow::makeRuntime() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);
    layout->addWidget(plain("Üç uygulama türü aynı Runtime'ı kullanır. Doğrulama yürütme yapılandırmasını kabul eder; fiziksel ölçüm kabulü değildir. Tek kare / host isteği; PLC gönderimi yok."));
    runtimeStatus_=plain("Aktif sürüm yok; çevrim çalıştırılmadı.");runtimeStatus_->setObjectName("runtimeStatus");layout->addWidget(runtimeStatus_);
    auto* actions=new QHBoxLayout;
    const QVector<QPair<QString,QString>> commands{{"validate","Taslağı doğrula"},{"activate","Etkinleştir"},{"run","Tek çevrim"},{"stop","Stop"},{"retry","Kaydı tekrar yaz"}};
    for(const auto& command:commands){auto* b=new QPushButton(command.second);b->setObjectName("runtime_"+command.first);actions->addWidget(b);connect(b,&QPushButton::clicked,this,[this,action=command.first]{runtimeAction(action);});}
    layout->addLayout(actions);
    auto* versions=new QHBoxLayout;runtimeHistory_=new QComboBox;runtimeHistory_->setObjectName("runtimeHistory");versions->addWidget(runtimeHistory_,1);
    auto* rollback=new QPushButton("Önceki etkin sürüme dön");rollback->setObjectName("runtime_rollback");versions->addWidget(rollback);connect(rollback,&QPushButton::clicked,this,[this]{runtimeAction("rollback");});layout->addLayout(versions);
    auto* split=new QSplitter;runtimeViewer_=new ImageViewer;split->addWidget(runtimeViewer_);
    auto* panel=new QWidget;auto* results=new QVBoxLayout(panel);runtimeTools_=new QComboBox;runtimeTools_->setObjectName("runtimeTools");results->addWidget(runtimeTools_);
    runtimeSummary_=plain("READY — sonuç yok.");runtimeSummary_->setObjectName("runtimeSummary");runtimeSummary_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);results->addWidget(runtimeSummary_);results->addStretch();
    runtimeTools_->setMinimumContentsLength(12);runtimeTools_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setWidget(panel);scroll->setMinimumWidth(250);split->addWidget(scroll);split->setStretchFactor(0,3);split->setStretchFactor(1,2);split->setSizes({680,420});layout->addWidget(split,1);
    connect(runtimeTools_,&QComboBox::currentIndexChanged,this,[this]{showRuntimeResult();});return page;
}
QWidget* MainWindow::makeLogs() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);logStatus_=plain("Kalıcı çevrim kaydı yok.");layout->addWidget(logStatus_);
    auto* refresh=new QPushButton("Kayıtları yenile");refresh->setObjectName("refreshLogs");layout->addWidget(refresh);connect(refresh,&QPushButton::clicked,this,[this]{refreshLogs();});
    logTable_=new QTableWidget;logTable_->setColumnCount(5);logTable_->setHorizontalHeaderLabels({"UTC / Cycle","Reçete / sürüm","Execution","Ölçüm kararları","İletişim"});logTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);logTable_->setSelectionBehavior(QAbstractItemView::SelectRows);logTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);layout->addWidget(logTable_,1);
    auto* detail=new QTextEdit;detail->setReadOnly(true);detail->setObjectName("logDetail");layout->addWidget(detail,1);
    connect(logTable_,&QTableWidget::currentCellChanged,this,[this,detail](int row,int,int,int){const auto* item=logTable_->item(row,0);detail->setPlainText(item ? item->data(Qt::UserRole).toString() : QString{});});return page;
}
void MainWindow::ensureRuntime() {
    if(!service_.hasProject() || service_.path().isEmpty())throw std::runtime_error("Önce kaydedilmiş bir proje açın.");
    if(runtime_ && runtimePath_==service_.path())return;
    if(runtimeJobs_.busy() || (runtime_ && runtime_->busy()))throw std::runtime_error("Önce Runtime çevrimini/kaydını tamamlayın.");
    runtimePath_=service_.path();runtime_=std::make_unique<RuntimeService>(runtimePath_+".production",runtimePath_+".calibrations.json",RuntimeIdentity::localEngineer());
    runtimeCycle_={};runtimePending_.reset();runtimeResult_.reset();runtimeHistory_->clear();showRuntimeResult();
}
void MainWindow::refreshRuntime() {
    if(!runtimeStatus_)return;
    if(runtime_ && runtimePath_==service_.path() && service_.activeRecipe())runtime_->observeInstallation(*service_.activeRecipe());
    if(runtime_ && runtimePath_!=service_.path() && !runtimeJobs_.busy() && !runtime_->busy()) {runtime_.reset();runtimePath_.clear();runtimeResult_.reset();runtimeHistory_->clear();showRuntimeResult();}
    const bool working=runtimeJobs_.busy(),pending=runtime_ && runtime_->busy();const auto active=runtime_ ? runtime_->active() : nullptr;
    QString state=working ? "BUSY" : pending ? "KAYIT BEKLİYOR — yeni çevrim engelli" : active ? "READY" : "Aktif sürüm yok";
    if(runtime_)state+=" | Yerel Engineer: "+runtime_->principal();
    if(active)state+="\nAktif: "+active->recipe().name+QString(" • v%1 • %2").arg(active->recipe().recipeVersion).arg(typeLabel(active->recipe().applicationType))+"\nValidation: "+active->validation().id+" | Config: "+QString::fromLatin1(active->validation().contentId.left(12));
    if(runtime_ && runtime_->validation())state+="\nSon taslak doğrulaması: "+runtime_->validation()->id;
    runtimeStatus_->setText(state);
    const bool ready=!working && !pending;
    findChild<QPushButton*>("runtime_validate")->setEnabled(ready && service_.activeRecipe() && !editorSession_.busy() && !cameraSession_.busy());
    findChild<QPushButton*>("runtime_activate")->setEnabled(ready && service_.activeRecipe() && runtime_ && runtime_->validation().has_value());
    findChild<QPushButton*>("runtime_run")->setEnabled(ready && active && runtimePath_==service_.path() && !editorSession_.busy() && !cameraSession_.busy());
    findChild<QPushButton*>("runtime_stop")->setEnabled(working);
    findChild<QPushButton*>("runtime_retry")->setEnabled(!working && pending && runtimePending_.has_value());
    findChild<QPushButton*>("runtime_rollback")->setEnabled(ready && runtimeHistory_->count()>0);
}
void MainWindow::runtimeAction(const QString& action) {
    try {
        if(action=="stop"){runtimeJobs_.stop(SessionJob::Execution);runtimeJobs_.stop(SessionJob::Image);refreshRuntime();return;}
        if(runtimeJobs_.busy())return;
        ensureRuntime();
        if(action=="retry") {
            if(!runtimePending_)return;
            runtime_->finish(runtimeCycle_.ticket,*runtimePending_);runtimeResult_=std::move(runtimePending_);runtimePending_.reset();runtimeCycle_={};showRuntimeResult();refreshRuntime();return;
        }
        if(runtime_->busy())throw std::runtime_error("Çevrim kaydı tamamlanmadan yeni işlem yapılamaz.");
        if(action=="activate" || action=="rollback") {
            if(action=="activate"){if(!service_.activeRecipe())return;runtime_->activate(*service_.activeRecipe(),QFileInfo(runtimePath_).absolutePath());}
            else runtime_->rollback(runtimeHistory_->currentData().toString());
            const auto s=runtime_->active();if(runtimeHistory_->findData(s->validation().id)<0)runtimeHistory_->addItem(s->recipe().name+QString(" v%1 • ").arg(s->recipe().recipeVersion)+s->validation().id.left(8),s->validation().id);
            runtimeHistory_->setCurrentIndex(runtimeHistory_->findData(s->validation().id));runtimeResult_.reset();showRuntimeResult();refreshRuntime();return;
        }
        if(editorSession_.busy() || cameraSession_.busy())throw std::runtime_error("Önce diğer kamera/editor işini tamamlayın.");
        runtimeResult_.reset();showRuntimeResult();
        if(action=="validate") {
            if(!applyToolProperties() || !service_.activeRecipe())return;
            const auto recipe=*service_.activeRecipe();const auto signature=flowSignature();const auto path=runtimePath_;const auto directory=QFileInfo(path).absolutePath();
            auto* watcher=runtimeJobs_.watch<ImageLoad>(this,SessionJob::Image);const auto cancel=runtimeJobs_.token(SessionJob::Image);
            connect(watcher,&QFutureWatcher<ImageLoad>::finished,this,[this,watcher,recipe,signature,path,directory,cancel]{
                const auto result=watcher->result();watcher->deleteLater();
                try {
                    if(cancel->load())throw std::runtime_error("Doğrulama iptal edildi.");
                    if(path!=service_.path() || signature!=flowSignature())throw std::runtime_error("Taslak değişti; doğrulamayı yeniden başlatın.");
                    if(!result.frame)throw std::runtime_error(result.error.toUtf8().constData());
                    runtime_->validate(recipe,directory,*result.frame);runtimeViewer_->setImage(result.frame->preview());runtimeViewer_->fitImage();runtimeSummary_->setText("Yapılandırma doğrulandı. Etkinleştirme bekleniyor; parça sonucu yok.");
                }catch(const std::exception& e){runtimeSummary_->setText(QString::fromUtf8(e.what()));}
                refreshRuntime();
            });
            runtimeJobs_.launch(watcher,[recipe,directory,cancel]{ImageLoad result;try{result.frame=RuntimeService::probe(recipe,directory,cancel);}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;});
        } else if(action=="run") {
            runtimeCycle_=runtime_->begin();const auto cycle=runtimeCycle_;const auto catalog=runtimePath_+".calibrations.json";
            auto* watcher=runtimeJobs_.watch<CycleResult>(this,SessionJob::Execution);const auto cancel=runtimeJobs_.token(SessionJob::Execution);
            connect(watcher,&QFutureWatcher<CycleResult>::finished,this,[this,watcher,cancel]{
                auto result=watcher->result();watcher->deleteLater();if(cancel->load())EditorSession::suppressCancelled(result);
                runtimePending_=result;
                try {runtime_->finish(runtimeCycle_.ticket,result);runtimeResult_=std::move(runtimePending_);runtimePending_.reset();runtimeCycle_={};showRuntimeResult();}
                catch(const std::exception& e){runtimeSummary_->setText("Kayıt bekliyor: "+QString::fromUtf8(e.what()));runtimeViewer_->clearImage("Kayıt bekliyor; sonuç yayımlanmadı.");}
                refreshRuntime();
            });
            runtimeJobs_.launch(watcher,[cycle,catalog,cancel]{return RuntimeService::execute(cycle,catalog,cancel);});
        }
        refreshRuntime();
    }catch(const std::exception& e){runtimeSummary_->setText(QString::fromUtf8(e.what()));refreshRuntime();}
}
void MainWindow::showRuntimeResult() {
    if(!runtimeViewer_)return;
    runtimeViewer_->setToolOverlay({},{});
    if(!runtimeResult_){runtimeViewer_->clearImage("Yeni çevrim sonucu yok.");runtimeSummary_->setText("READY — sonuç yok.");QSignalBlocker blocker(runtimeTools_);runtimeTools_->clear();return;}
    const auto& c=*runtimeResult_;const auto snapshot=runtime_ ? runtime_->active() : nullptr;QStringList lines;
    lines<<executionStateName(c.state)+QString(" • %1 ms").arg(c.milliseconds,0,'f',2)<<"Cycle: "+c.cycleId<<"Frame: "+c.frameId<<"Config: "+QString::fromLatin1(c.configId.left(12))<<"Kalıcı kayıt tamamlandı • İletişim: NOT_REQUESTED";
    if(!c.error.isEmpty())lines<<c.error;
    bool image=false;
    for(const auto& t:c.tools)for(const auto& v:t.outputs)if(v.valid && std::holds_alternative<ImageFrame>(v.data)){runtimeViewer_->setImage(std::get<ImageFrame>(v.data).preview());image=true;}
    if(!image)runtimeViewer_->clearImage("Bu çevrim için geçerli görüntü yok.");
    const auto selected=runtimeTools_->currentData().toString();
    {QSignalBlocker blocker(runtimeTools_);runtimeTools_->clear();if(snapshot)for(const auto& t:editorTools(snapshot->recipe().definition))runtimeTools_->addItem(t.name,t.id);runtimeTools_->setCurrentIndex(std::max(0,runtimeTools_->findData(selected)));}
    for(const auto& t:c.tools){QString name=t.toolId;if(snapshot)for(const auto& tool:editorTools(snapshot->recipe().definition))if(tool.id==t.toolId)name=tool.name;
        lines<<name+" • "+executionStateName(t.state)+" • "+t.errorCode+" "+t.message;
        for(auto it=t.outputs.cbegin();it!=t.outputs.cend();++it)if(it->valid && !std::holds_alternative<ImageFrame>(it->data) && !t.calibration) {
            QString value;if(const auto p=std::get_if<double>(&it->data))value=QString::number(*p,'f',6);
            else if(const auto p=std::get_if<bool>(&it->data))value=*p ? "true" : "false";
            else if(const auto p=std::get_if<QPointF>(&it->data))value=QString("(%1, %2)").arg(p->x()).arg(p->y());
            else if(const auto p=std::get_if<QRect>(&it->data))value=QString("(%1, %2) %3×%4").arg(p->x()).arg(p->y()).arg(p->width()).arg(p->height());
            lines<<"  "+it.key()+": "+value+" "+it->port.unit;
        }
        if(t.decision!=PartDecision::NotEvaluated)lines<<QString("  Ölçüm: %1 mm • %2").arg(std::get<double>(t.outputs.value("value").data),0,'f',6).arg(t.decision==PartDecision::Pass ? "PASS" : "FAIL");
        if(t.toolId==runtimeTools_->currentData().toString()){QString annotation;if(t.decision!=PartDecision::NotEvaluated)annotation=QString("%1 mm • %2").arg(std::get<double>(t.outputs.value("value").data),0,'f',3).arg(t.decision==PartDecision::Pass ? "PASS" : "FAIL");runtimeViewer_->setToolOverlay(t.regions,t.points,t.lines,annotation);}
    }runtimeSummary_->setText(lines.join("\n"));
}
void MainWindow::refreshLogs() {
    logTable_->setRowCount(0);findChild<QTextEdit*>("logDetail")->clear();
    try {
        if(!service_.hasProject()){logStatus_->setText("Önce proje açın.");return;}
        const auto records=ProductionStore(service_.path()+".production").recent("cycles");
        logStatus_->setText(QString("Son %1 kalıcı çevrim • restart sonrasında salt okunur • NOT_REQUESTED: PLC teslimi yapılmadı").arg(records.size()));
        for(const auto& record:records){const int row=logTable_->rowCount();logTable_->insertRow(row);QStringList decisions;
            for(const auto& v:record["tools"].toArray()){const auto t=v.toObject();if(t["decision"]!="NOT_EVALUATED")decisions<<t["name"].toString(t["toolId"].toString().left(8))+": "+t["decision"].toString();}
            const QStringList values{record["completedAtUtc"].toString()+"\n"+record["cycleId"].toString(),record["recipeName"].toString(record["recipeId"].toString().left(8))+QString(" v%1").arg(record["recipeVersion"].toInt()),record["state"].toString(),decisions.isEmpty() ? "NOT_EVALUATED" : decisions.join("; "),record["communicationState"].toString()};
            for(int col=0;col<values.size();++col)logTable_->setItem(row,col,new QTableWidgetItem(values[col]));logTable_->item(row,0)->setData(Qt::UserRole,QString::fromUtf8(QJsonDocument(record).toJson(QJsonDocument::Indented)));}
    }catch(const std::exception& e){logStatus_->setText("Kayıt okunamadı: "+QString::fromUtf8(e.what()));}
}
}
