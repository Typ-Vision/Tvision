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
#include "EndpointProfileEditor.hpp"

namespace visionflow {
QWidget* MainWindow::makeCommunication() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);
    auto* description=new QLabel("PLC • TCP/IP Socket • VisionFlow sunucu / PLC istemci\nSürümlü JSON satırları: TRIGGER → BUSY → DONE → ACK. Robot iletişimi henüz uygulanmadı.");
    description->setWordWrap(true);layout->addWidget(description);
    auto* form=new QFormLayout;
    QSettings settings(settingsFile_,QSettings::IniFormat);
    for(const auto key:{"enabled","autoStart"}) {auto* box=new QCheckBox(QString(key)=="enabled" ? "TCP iletişimini etkinleştir" : "Uygulama açılışında dinlemeyi başlat");box->setObjectName("plc_"+QString(key));box->setChecked(settings.value("socket/"+QString(key),false).toBool());form->addRow(box);}
plcAddress_=new QLineEdit("127.0.0.1");plcAddress_->setObjectName("plcAddress");form->addRow("Yerel IP (dinleme)",plcAddress_);
    auto* port=new QSpinBox;port->setRange(0,65535);port->setValue(5000);port->setObjectName("plcPort");form->addRow("TCP port (0: otomatik)",port);
    auto* ack=new QSpinBox;ack->setRange(100,60000);ack->setValue(2000);ack->setSuffix(" ms");ack->setObjectName("plcAckTimeout");form->addRow("ACK bekleme süresi",ack);
    plcMapping_=new QComboBox;plcMapping_->setObjectName("plcMapping");plcMapping_->addItem("Karar eşlemesi yok",QString{});form->addRow("Reçete sonuç kaynağı (Save gerekir)",plcMapping_);
    auto* mode=new QComboBox;mode->setObjectName("recipeTrigger");mode->addItems({"Manual","SoftwareTrigger"});form->addRow("Reçete tetik modu (Save gerekir)",mode);
    auto* retries=new QSpinBox;retries->setRange(0,10);retries->setValue(settings.value("socket/retries",3).toInt());retries->setObjectName("plcRetries");form->addRow("ACK tekrar sayısı",retries);
    plcAddress_->setText(settings.value("socket/address","127.0.0.1").toString());port->setValue(settings.value("socket/port",5000).toInt());ack->setValue(settings.value("socket/ackTimeout",2000).toInt());
    layout->addLayout(form);
    auto* hint=new QLabel;hint->setObjectName("plcHint");hint->setWordWrap(true);layout->addWidget(hint);
    auto updateRecipe=[this,mode]{if(!service_.activeRecipe())return;try{service_.setRuntimeOptions(mode->currentText(),plcMapping_->currentData().toString());refreshWorkspace();}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}};
    connect(mode,&QComboBox::currentIndexChanged,this,updateRecipe);connect(plcMapping_,&QComboBox::currentIndexChanged,this,updateRecipe);
    auto* actions=new QGridLayout;int actionIndex=0;
    for(const auto& command:QVector<QPair<QString,QString>>{{"save","Ayarları kaydet"},{"listen","Dinlemeyi başlat"},{"stop","Dinlemeyi durdur"},{"cancel","Çevrimi iptal et"},{"retry","Kaydı tekrar yaz"}}) {
        auto* button=new QPushButton(command.second);button->setObjectName("plc_"+command.first);actions->addWidget(button,actionIndex/3,actionIndex%3);++actionIndex;
        connect(button,&QPushButton::clicked,this,[this,action=command.first]{communicationAction(action);});
    }layout->addLayout(actions);
    plcStatus_=new QLabel("STOPPED — TCP bağlantısı yok.");plcStatus_->setTextFormat(Qt::PlainText);plcStatus_->setWordWrap(true);plcStatus_->setObjectName("plcStatus");layout->addWidget(plcStatus_);
    auto* trace=new QPlainTextEdit;trace->setReadOnly(true);trace->setObjectName("plcTrace");trace->setMaximumBlockCount(128);layout->addWidget(trace,1);
    auto* note=new QLabel("Socket reçeteden bağımsızdır. AutoStart otomatik Run yapmaz. Tetik modu ve sonuç kaynağı reçeteye Save ile kaydedilir. Fiziksel PLC ve ölçüm kabulü açık.");note->setWordWrap(true);layout->addWidget(note);
    connect(page->findChild<QCheckBox*>("plc_enabled"),&QCheckBox::toggled,this,[this]{refreshCommunication();});
    auto* timer=new QTimer(page);timer->setInterval(250);connect(timer,&QTimer::timeout,this,[this]{refreshCommunication();});timer->start();
    for(auto* field:page->findChildren<QLineEdit*>())field->setMinimumHeight(26);
    for(auto* field:page->findChildren<QSpinBox*>())field->setMinimumHeight(26);
    for(auto* field:page->findChildren<QComboBox*>())field->setMinimumHeight(26);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setWidget(page);
    auto* tabs=new QTabWidget;
    auto* v3scroll=new QScrollArea;v3scroll->setObjectName("endpointScroll");v3scroll->setWidgetResizable(true);
    v3scroll->setWidget(new EndpointProfileEditor(QFileInfo(settingsFile_).absolutePath()+"/communication-profiles.json",[this]{
        return runtimeCoordinator_ && runtimeCoordinator_->state()!=RuntimeState::Unloaded && runtimeCoordinator_->state()!=RuntimeState::LoadedStopped;
    }));
    tabs->addTab(v3scroll,"TCP v3 profilleri");tabs->addTab(scroll,"TCP v2 (mevcut)");return tabs;
}
bool MainWindow::verifyEndpointProfiles(const QString& artifacts) {
    selectPage("communication");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();if(!grab().save(QDir(artifacts).filePath(QString("communication-%1x%2.png").arg(size.width()).arg(size.height()))))return false;}
    findChild<QLineEdit*>("endpointAddress")->setText("invalid address");
    findChild<QPushButton*>("endpoint_save")->click();
    auto* status=findChild<QLabel*>("endpointStatus");
    findChild<QScrollArea*>("endpointScroll")->ensureWidgetVisible(status);
    QApplication::processEvents();
    if(!grab().save(QDir(artifacts).filePath("communication-850x600-error-scroll.png")))return false;
    return status->text().contains("IPv4") && !plc_->listening() && !runtime_ && findChild<QComboBox*>("endpointList");
}
void MainWindow::refreshCommunication() {
    if(!plcStatus_)return;const bool listening=plc_ && plc_->listening();
    const auto recipe=service_.activeRecipe();
    {QSignalBlocker blocker(plcMapping_);auto* mode=findChild<QComboBox*>("recipeTrigger");QSignalBlocker modeBlocker(mode);
        plcMapping_->clear();plcMapping_->addItem("Karar eşlemesi yok",QString{});
        if(recipe)for(const auto& t:editorTools(recipe->definition))if(t.kind=="measurement" && t.enabled)plcMapping_->addItem(t.name+" • "+t.id.left(8),t.id);
        const auto options=recipe ? recipe->definition["runtime"].toObject() : QJsonObject{};
        plcMapping_->setCurrentIndex(std::max(0,plcMapping_->findData(options["measurementToolId"].toString())));
        mode->setCurrentText(options["trigger"].toString("Manual"));mode->setEnabled(recipe!=nullptr);
    }
    if(hardwareStatus_)hardwareStatus_->setText("Camera: bağlı değil\nRobot: bağlı değil\nPLC: "+QString(plc_ && plc_->connected() ? "TCP bağlı" : listening ? "TCP dinleniyor" : "bağlı değil"));
    plcAddress_->setEnabled(!listening);plcMapping_->setEnabled(recipe!=nullptr);
    findChild<QSpinBox*>("plcPort")->setEnabled(!listening);findChild<QSpinBox*>("plcAckTimeout")->setEnabled(!listening);
    findChild<QSpinBox*>("plcRetries")->setEnabled(!listening);
    findChild<QPushButton*>("plc_listen")->setEnabled(!listening && findChild<QCheckBox*>("plc_enabled")->isChecked());
    const bool enabled=findChild<QCheckBox*>("plc_enabled")->isChecked();
    findChild<QLabel*>("plcHint")->setText(listening ? "Dinleme açık. Endpoint ayarlarını değiştirmek için önce dinlemeyi durdurun." : enabled ? "Ayarları kaydet → Dinlemeyi başlat. Dinlemek için reçete veya Run gerekmez." : "Dinleme kapalı: TCP iletişimini etkinleştir kutusunu işaretleyin → Ayarları kaydet → Dinlemeyi başlat.");
    findChild<QPushButton*>("plc_listen")->setToolTip(enabled ? "TCP sunucusunu başlatır; Run başlatmaz." : "Önce TCP iletişimini etkinleştir kutusunu işaretleyin.");
    findChild<QPushButton*>("plc_stop")->setEnabled(listening && !plc_->blocking());
    findChild<QPushButton*>("plc_cancel")->setEnabled(listening && plc_->working());
    findChild<QPushButton*>("plc_retry")->setEnabled(listening && plc_->recordingPending());
    const auto lastError=plcStatus_->property("lastError").toString();
    if(plc_){plcStatus_->setText(plc_->state()+" • "+plcAddress_->text()+":"+QString::number(plc_->port())+"\nSession: "+plc_->sessionId()+(lastError.isEmpty() ? QString{} : "\n"+lastError));
        auto* trace=findChild<QPlainTextEdit*>("plcTrace");const auto text=plc_->diagnostics().join('\n');if(trace->toPlainText()!=text){trace->setPlainText(text);trace->moveCursor(QTextCursor::End);}}
}
void MainWindow::initializeCommunication() {
    plc_=std::make_unique<PlcRuntimeSocket>();
    plc_->setUnavailable([this]{return editorSession_.busy() || cameraSession_.busy() || runtimeJobs_.busy() ? QString("NOT_READY") : QString{};});
    plc_->changed=[this]{refreshCommunication();refreshRuntime();};
    plc_->started=[this]{runtimeResult_.reset();showRuntimeResult();runtimeSummary_->setText("BUSY — PLC isteği çalışıyor; yeni sonuç yok.");};
    plc_->completed=[this](const CycleResult& r){if(measurementWorkspace_ && runtime_ && runtime_->active())measurementWorkspace_->observeAcquisition(runtime_->active()->recipe(),r);if(runtimeCoordinator_ && runtimeCoordinator_->state()==RuntimeState::Running)runtimeResult_=r;else runtimeResult_.reset();showRuntimeResult();};
    if(findChild<QCheckBox*>("plc_enabled")->isChecked() && findChild<QCheckBox*>("plc_autoStart")->isChecked())communicationAction("listen");
    refreshCommunication();
}
void MainWindow::communicationAction(const QString& action) {
    plcStatus_->setProperty("lastError",QString{});
    try {
        if(action=="save" || action=="listen") {
            QHostAddress address;if(!address.setAddress(plcAddress_->text()))throw std::runtime_error("Geçerli yerel IPv4/IPv6 adresi girin.");
            if(action=="save") {
                if(!findChild<QCheckBox*>("plc_enabled")->isChecked() && plc_->listening())plc_->stop();
                QSettings settings(settingsFile_,QSettings::IniFormat);
                for(const auto key:{"enabled","autoStart"})settings.setValue("socket/"+QString(key),findChild<QCheckBox*>("plc_"+QString(key))->isChecked());
                settings.setValue("socket/address",plcAddress_->text());settings.setValue("socket/port",findChild<QSpinBox*>("plcPort")->value());settings.setValue("socket/ackTimeout",findChild<QSpinBox*>("plcAckTimeout")->value());settings.setValue("socket/retries",findChild<QSpinBox*>("plcRetries")->value());settings.sync();
                if(settings.status()!=QSettings::NoError)throw std::runtime_error("Socket settings could not be saved.");
            }else {
                if(!findChild<QCheckBox*>("plc_enabled")->isChecked())throw std::runtime_error("TCP iletişimini etkinleştir kutusunu işaretleyin.");
                PlcSocketOptions options;options.address=address;options.port=quint16(findChild<QSpinBox*>("plcPort")->value());options.ackTimeoutMs=findChild<QSpinBox*>("plcAckTimeout")->value();options.retryCount=findChild<QSpinBox*>("plcRetries")->value();
                if(runtimeCoordinator_ && runtimeCoordinator_->loaded())options.measurementToolId=runtimeCoordinator_->loaded()->definition["runtime"].toObject()["measurementToolId"].toString();
                plc_->listen(options);
            }
        }else if(plc_) {
            if(action=="stop")plc_->stop();else if(action=="cancel")plc_->cancel();else if(action=="retry")plc_->retryRecording();
        }
        refreshCommunication();refreshRuntime();
    }catch(const std::exception& e){const auto message=QString::fromUtf8(e.what());plcStatus_->setProperty("lastError",message);plcStatus_->setText(message);}
}
}
