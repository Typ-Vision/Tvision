#include "CommunicationPanel.hpp"
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QSignalBlocker>

namespace visionflow {
namespace {
QLabel* plain(QString text={}){auto* l=new QLabel(text);l->setTextFormat(Qt::PlainText);l->setWordWrap(true);l->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);return l;}
}
CommunicationPanel::CommunicationPanel(QWidget* parent):QWidget(parent){
    setObjectName("communicationWorkspace");auto* root=new QVBoxLayout(this);
    auto* scroll=new QScrollArea;scroll->setObjectName("communicationScroll");scroll->setWidgetResizable(true);root->addWidget(scroll);
    auto* content=new QWidget;auto* layout=new QVBoxLayout(content);scroll->setWidget(content);
    listener_=plain("V4 listener bağlanmadı.");listener_->setObjectName("communicationListener");layout->addWidget(listener_);
    auto* profile=new QGroupBox("Tek makine listener profili");auto* form=new QFormLayout(profile);
    enabled_=new QCheckBox("Açılışta dinle • Run / Auto başlatmaz");enabled_->setObjectName("communicationEnabled");form->addRow(enabled_);
    address_=new QLineEdit;address_->setObjectName("communicationAddress");form->addRow("&Yerel adres",address_);
    port_=new QSpinBox;port_->setRange(1,65535);port_->setObjectName("communicationPort");form->addRow("&Port",port_);
    auto* save=new QPushButton("Profili &kaydet (yeniden başlatmada kullanılır)");save->setObjectName("communicationSave");form->addRow(save);
    connect(save,&QPushButton::clicked,this,[this]{try{if(!workspace_)throw std::runtime_error("Workspace bağlanmadı");auto p=workspace_->profile();p.enabled=enabled_->isChecked();p.address=address_->text().trimmed();p.port=quint16(port_->value());workspace_->saveProfile(p);message_->setText("Profil kaydedildi. Etkin listener değişmedi; yeniden başlatın.");}catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}});
    layout->addWidget(profile);
    remote_=new QCheckBox("Remote Enable — PLC reçete seçimi / Auto / Stop izni (açılışta kapalı)");remote_->setObjectName("communicationRemoteEnable");layout->addWidget(remote_);
    connect(remote_,&QCheckBox::toggled,this,[this](bool enabled){try{if(!workspace_)throw std::runtime_error("Workspace bağlanmadı");workspace_->setRemoteEnabled(enabled);}catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));QSignalBlocker block(remote_);remote_->setChecked(workspace_ && workspace_->remoteEnabled());}});
    layout->addWidget(plain("İstasyonlar • Bağlantı, üretim durumu ve teslim birbirinden bağımsızdır."));
    stations_=new QTableWidget(0,7);stations_->setObjectName("communicationStations");stations_->setAccessibleName("İstasyon durumları");stations_->setHorizontalHeaderLabels({"Station","Run durumu","Auto","Aktif reçete / revision","Kamera sahibi","Teslim / ACK","Health"});
    stations_->setSelectionBehavior(QAbstractItemView::SelectRows);stations_->setSelectionMode(QAbstractItemView::SingleSelection);stations_->setEditTriggers(QAbstractItemView::NoEditTriggers);stations_->setMinimumHeight(180);stations_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);stations_->horizontalHeader()->setStretchLastSection(true);layout->addWidget(stations_);
    const int widths[]{140,140,70,320,260,150,280};for(int column=0;column<7;++column)stations_->setColumnWidth(column,widths[column]);
    connect(stations_,&QTableWidget::itemSelectionChanged,this,[this]{selection();});
    auto* recipeRow=new QHBoxLayout;recipes_=new QComboBox;recipes_->setObjectName("communicationSavedRecipe");recipes_->setAccessibleName("Kayıtlı immutable reçete revision");recipes_->setMinimumContentsLength(16);recipes_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);recipeRow->addWidget(recipes_,1);
    auto* choose=new QPushButton("Seçili station'a reçeteyi yükle (Run açmaz)");choose->setObjectName("stationSelectRecipe");recipeRow->addWidget(choose);layout->addLayout(recipeRow);
    connect(choose,&QPushButton::clicked,this,[this]{const int n=stations_->currentRow();if(!workspace_ || n<0 || n>=rows_.size() || recipes_->currentIndex()<0)return;try{const auto& row=rows_[n];workspace_->selectRecipe(row.station.id,row.snapshot?row.snapshot->activation:QUuid{},recipes_->currentData().toUInt(),recipes_->currentData(Qt::UserRole+1).toUInt());message_->setText("Station reçete seçimi kaydedildi; Run / Auto kapalı.");}catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}refresh();});
    auto* commands=new QHBoxLayout;auto add=[&](QString title,QString name,QString cmd){auto* b=new QPushButton(title);b->setObjectName(name);commands->addWidget(b);connect(b,&QPushButton::clicked,this,[this,cmd]{command(cmd);});return b;};
    run_=add("&Run","stationRun","run");automatic_=add("&Auto","stationAuto","auto");manual_=add("Auto kapat","stationManual","manual");stop_=add("&Stop / drain","stationStop","stop");layout->addLayout(commands);
    detail_=plain();detail_->setObjectName("communicationStationDetail");layout->addWidget(detail_);
    auto* inspect=new QPushButton("Teslim ve salt okunur &geçmişi yenile");inspect->setObjectName("communicationHistoryRefresh");layout->addWidget(inspect);
    history_=new QPlainTextEdit;history_->setObjectName("communicationHistory");history_->setReadOnly(true);history_->setMinimumHeight(140);history_->setMaximumBlockCount(2048);layout->addWidget(history_);
    connect(inspect,&QPushButton::clicked,this,[this]{try{history_->setPlainText((workspace_?workspace_->history():"V4 ledger bağlanmadı.")+"\nLegacy geçmiş (salt okunur)\n"+(legacyHistory?legacyHistory():"Proje arşivi seçilmedi."));}catch(const std::exception& e){history_->setPlainText("RECOVERY_REQUIRED: "+QString::fromUtf8(e.what()));}});
    auto* migrationBox=new QGroupBox("Legacy analiz / migration / rollback");auto* ml=new QVBoxLayout(migrationBox);
    ml->addWidget(plain("Önce read-only analiz yapın. Unsupported ve pending ACK geçişi engeller. Eski dosyalar ve ledger korunur. Pending kayıtları eski kurulu sürümde drain edin. Bakım için listener ve bütün Runtime'lar kapalı olmalıdır."));
    migrationRequest_=new QLineEdit;migrationRequest_->setObjectName("communicationMigrationRequest");migrationRequest_->setPlaceholderText("Gözden geçirilmiş migration istek JSON dosyasının mutlak yolu");migrationRequest_->setAccessibleName("Migration istek dosyası");ml->addWidget(migrationRequest_);
    auto* ma=new QGridLayout;int actionIndex=0;for(const auto& pair:QVector<QPair<QString,QString>>{{"analyze","Read-only analiz"},{"prepare","Backup + manifest"},{"activate","V4 seçimini uygula"},{"reviewRollback","Rollback incele"},{"rollback","Rollback uygula"}}){auto* b=new QPushButton(pair.second);b->setObjectName("migration_"+pair.first);ma->addWidget(b,actionIndex/2,actionIndex%2);++actionIndex;connect(b,&QPushButton::clicked,this,[this,cmd=pair.first]{migration(cmd);});}ml->addLayout(ma);
    migrationReport_=new QPlainTextEdit;migrationReport_->setObjectName("communicationMigrationReport");migrationReport_->setReadOnly(true);migrationReport_->setMinimumHeight(160);ml->addWidget(migrationReport_);layout->addWidget(migrationBox);
    layout->addWidget(plain("Robot kalibrasyon workspace'i COMM4-24 kapsamındadır. Fiziksel kamera / PLC / robot kabulü açık."));
    message_=plain();message_->setObjectName("communicationMessage");layout->addWidget(message_);layout->addStretch();
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    auto* timer=new QTimer(this);timer->setInterval(500);connect(timer,&QTimer::timeout,this,[this]{if(isVisible())refresh();});timer->start();selection();
}
void CommunicationPanel::bind(CommunicationWorkspace* w){workspace_=w;if(w)try{const auto p=w->profile();enabled_->setChecked(p.enabled);address_->setText(p.address);port_->setValue(p.port);QSignalBlocker block(remote_);remote_->setChecked(w->remoteEnabled());recipes_->clear();for(const auto& recipe:w->recipes()){recipes_->addItem(recipe.selection.recipe.name+" / "+recipe.selection.revision.toString(QUuid::WithoutBraces),recipe.selection.recipeKey);recipes_->setItemData(recipes_->count()-1,recipe.templateKey,Qt::UserRole+1);}}catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}refresh();}
void CommunicationPanel::refresh(){
    if(!workspace_)return;
    try{
        workspace_->poll();listener_->setText(workspace_->listener());
        QUuid selected;const auto previous=stations_->currentRow();if(previous>=0 && previous<rows_.size())selected=rows_[previous].station.id;
        auto next=workspace_->stations();QSignalBlocker block(stations_);rows_=std::move(next);stations_->setRowCount(rows_.size());int restore=-1;
        for(int n=0;n<rows_.size();++n){const auto& v=rows_[n];const QStringList values{v.station.name+QString(" (#%1)").arg(v.station.key),CommunicationWorkspace::modeName(v.runtime.mode),v.automatic?"ON":"OFF",v.snapshot?v.snapshot->selection.recipe.name+" / "+v.snapshot->selection.revision.toString(QUuid::WithoutBraces):"Seçilmedi",v.cameraOwners,v.runtime.deliveryPending?"PENDING ACK":"Pending yok",v.runtime.health.isEmpty()?v.unavailable:v.runtime.health};
            for(int c=0;c<values.size();++c){auto* item=stations_->item(n,c);if(!item){item=new QTableWidgetItem;stations_->setItem(n,c,item);}item->setText(values[c]);item->setToolTip(values[c]);}if(v.station.id==selected)restore=n;
        }
        if(restore>=0)stations_->selectRow(restore);else if(!rows_.isEmpty())stations_->selectRow(0);
        selection();
    }catch(const std::exception& e){rows_.clear();stations_->setRowCount(0);selection();message_->setText("RECOVERY_REQUIRED: "+QString::fromUtf8(e.what()));}
}
void CommunicationPanel::selection(){
    const int n=stations_->currentRow();const bool valid=n>=0 && n<rows_.size();run_->setEnabled(valid && rows_[n].canRun);automatic_->setEnabled(valid && rows_[n].canAuto && !rows_[n].automatic);manual_->setEnabled(valid && rows_[n].canAuto && rows_[n].automatic);stop_->setEnabled(valid);
    if(!valid){detail_->setText("Kayıtlı station seçilmedi. Enrollment / manifest eksikliği üretim izni oluşturmaz.");return;}
    const auto& v=rows_[n];detail_->setText(QString("Station UUID: %1\nActivation: %2\nPublication: %3\nKamera: %4\n%5%6").arg(v.station.id.toString(QUuid::WithoutBraces),v.snapshot?v.snapshot->activation.toString(QUuid::WithoutBraces):"yok",v.publication.isEmpty()?"Bağlanmadı":v.publication,v.cameraOwners,v.unavailable,v.runtime.deliveryPending?"\nPENDING ACK: Run/Auto kapalı; Stop ve exact ACK kullanılabilir.":""));
}
void CommunicationPanel::command(const QString& cmd){const int n=stations_->currentRow();if(!workspace_ || n<0 || n>=rows_.size())return;const auto row=rows_[n];try{workspace_->command(row.station.id,row.snapshot?row.snapshot->activation:QUuid{},cmd);message_->setText("Station komutu: "+cmd);}catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}refresh();}
void CommunicationPanel::migration(const QString& cmd){
    try{
        if(!workspace_)throw std::runtime_error("Workspace bağlanmadı");
        if(cmd=="analyze"){
            reviewedInput_.reset();planDigest_.clear();manifestDigest_.clear();pointerDigest_.clear();ledgerDigest_.clear();
            QFile f(migrationRequest_->text());if(!f.open(QIODevice::ReadOnly) || f.size()>1024*1024)throw std::runtime_error("Migration istek dosyası okunamadı (1 MiB sınırı)");
            QJsonParseError error;const auto doc=QJsonDocument::fromJson(f.readAll(),&error);if(error.error!=QJsonParseError::NoError || !doc.isObject())throw std::runtime_error("Geçersiz migration JSON");const auto o=doc.object();
            if(o["schema"]!="visionflow.communication-migration-request/1")throw std::runtime_error("Migration istek şeması desteklenmiyor");
            CommunicationMigrationInput in;in.projectPath=o["project"].toString();in.profilesPath=o["profiles"].toString();in.binaryPath=o["binary"].toString();in.registryPath=o["registry"].toString();in.archiveDirectory=o["archive"].toString();in.station=QUuid(o["station"].toString());in.publication=PublicationTemplate::fromJson(o["publication"].toObject());
            const auto server=CommunicationServerProfiles(o["serverProfile"].toString()).load();if(!server)throw std::runtime_error("Explicit disabled server profili gerekir");in.server=*server;
            const auto plan=workspace_->analyze(in);migrationReport_->setPlainText(QString::fromUtf8(QJsonDocument(plan.report).toJson()));
            reviewedInput_=in;reviewedRequest_=migrationRequest_->text();workspacePath_=o["workspace"].toString();packagePath_=o["package"].toString();planDigest_=plan.digest();message_->setText(plan.supported()?"Analiz supported. Backup + manifest için açık eylem gerekir.":"UNSUPPORTED / PENDING — rapordaki nedenleri çözün. Geçiş kapalı.");return;
        }
        if(!reviewedInput_ || reviewedRequest_!=migrationRequest_->text())throw std::runtime_error("Önce güncel istek için read-only analiz yapın");
        if(cmd=="prepare"){manifestDigest_=workspace_->prepare(*reviewedInput_,workspacePath_,packagePath_,planDigest_);const auto state=workspace_->reviewRollback(*reviewedInput_,workspacePath_);pointerDigest_=state["pointerDigest"].toString().toLatin1();message_->setText("Backup + manifest hazır. SHA256: "+QString::fromLatin1(manifestDigest_)+"\nExpected pointer: "+QString::fromLatin1(pointerDigest_));}
        else if(cmd=="activate"){
            if(manifestDigest_.isEmpty())throw std::runtime_error("Önce backup + manifest hazırlayın");
            workspace_->activate(*reviewedInput_,workspacePath_,packagePath_,manifestDigest_,pointerDigest_);manifestDigest_.clear();message_->setText("V4 seçim pointer'ı kaydedildi. Üretim izni açılmadı; yeni seçimi yeniden başlatmada doğrulayın.");
        }else if(cmd=="reviewRollback"){
            const auto active=workspace_->reviewRollback(*reviewedInput_,workspacePath_);pointerDigest_=active["pointerDigest"].toString().toLatin1();ledgerDigest_=active["ledgerDigest"].toString().toLatin1();migrationReport_->setPlainText(QString::fromUtf8(QJsonDocument(active).toJson()));message_->setText("Rollback review hazır; pending ve revision uygulamada tekrar denetlenir.");
        }else if(cmd=="rollback"){
            if(pointerDigest_.isEmpty() || ledgerDigest_.isEmpty())throw std::runtime_error("Önce rollback incelemesi yapın");workspace_->rollback(*reviewedInput_,workspacePath_,pointerDigest_,ledgerDigest_);pointerDigest_.clear();ledgerDigest_.clear();message_->setText("Rollback selection tamam. Live ledger korundu. Eski kurulu sürümle yeniden başlatın.");
        }
    }catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}
}
}
