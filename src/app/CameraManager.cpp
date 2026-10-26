#include "CameraManager.hpp"
#include "ImageViewer.hpp"
#include <QSplitter>
#include "application/ExperimentalOecsAcquisition.hpp"
#include "application/AcquisitionPolicy.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleValidator>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>
#include <QScopedValueRollback>
namespace visionflow {
namespace {
QLabel* text(QString value,QString name={}){auto* l=new QLabel(value);l->setObjectName(name);l->setWordWrap(true);l->setTextFormat(Qt::PlainText);l->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);return l;}
QPushButton* button(QLayout* l,QString value,QString name){auto* b=new QPushButton(value);b->setObjectName(name);l->addWidget(b);return b;}
bool matches(const MeasurementCamera& c,const CameraDiscoveryEntry& d){return c.physical ? d.industrial && *c.physical==d.industrial->identity : d.webcam && c.device.adapter==d.webcam->adapter && c.device.providerId==d.webcam->providerId;}
}
CameraManager::CameraManager(std::function<MeasurementWorkspace*()> context,std::function<bool()> gate,CameraSession& jobs,
    QString settings,QWidget* parent,Scanner scanner,CameraProviderFactory factory,ExperimentalOecsAcquisition::ApiFactory experimentalApi,std::function<QString(QWidget*)> framePathChooser)
    :QWidget(parent),context_(std::move(context)),gate_(std::move(gate)),jobs_(jobs),local_(std::move(settings)),scanner_(std::move(scanner)),factory_(std::move(factory)),experimentalApi_(std::move(experimentalApi)),framePathChooser_(std::move(framePathChooser)) {
    if(!framePathChooser_)framePathChooser_=[](QWidget* parent){return QFileDialog::getSaveFileName(parent,"Kareyi kaydet",{},"Mono8 raster (*.pgm)");};
    setObjectName("cameraManager");if(!scanner_)scanner_=[](const auto& files,const auto& cancel){return CameraService{}.inventory(files,1000,cancel);};
    auto* outer=new QVBoxLayout(this);auto* split=new QSplitter(Qt::Horizontal);split->setObjectName("cameraSplitter");split->setChildrenCollapsible(false);outer->addWidget(split);
    auto* scroll=new QScrollArea;scroll->setObjectName("cameraSettingsScroll");scroll->setWidgetResizable(true);scroll->setMinimumWidth(330);split->addWidget(scroll);
    auto* previewBody=new QWidget;auto* pv=new QVBoxLayout(previewBody);pv->setContentsMargins(0,0,0,0);split->addWidget(previewBody);split->setStretchFactor(0,0);split->setStretchFactor(1,1);split->setSizes({460,900});
    auto* body=new QWidget;scroll->setWidget(body);auto* layout=new QVBoxLayout(body);
    layout->addWidget(text("Kamera yönetimi","cameraTitle"));
    layout->addWidget(text("Kamera kaydı ve keşif edinim izni vermez. Önizleme ölçüm veya çevrim sonucu değildir."));
    auto* actions=new QHBoxLayout;layout->addLayout(actions);scan_=button(actions,"Cihazları ara","cameraScan");
    status_=text("Henüz arama yapılmadı. Kayıtlar için kaydedilmiş proje açın.","cameraStatus");layout->addWidget(status_);
    auto* project=button(layout,"Proje oluştur / aç","cameraProject");connect(project,&QPushButton::clicked,this,[this]{if(onProject)onProject();});
    auto* advanced=new QGroupBox("Ayrıntılar");advanced->setObjectName("cameraAdvanced");advanced->setCheckable(true);advanced->setChecked(false);auto* al=new QVBoxLayout(advanced);advancedBody_=new QWidget;al->addWidget(advancedBody_);auto* av=new QVBoxLayout(advancedBody_);advancedBody_->hide();connect(advanced,&QGroupBox::toggled,advancedBody_,&QWidget::setVisible);
    producer_=new QLineEdit;producer_->setObjectName("cameraProducer");producer_->setPlaceholderText("Boşsa hatırlanan producer'lar ve kurulu GenTL ortamı kullanılır");av->addWidget(producer_);
    auto* choose=button(av,"Producer seç…","cameraChoose");

    connect(choose,&QPushButton::clicked,this,[this]{attempt([this]{if(jobs_.busy() || !gate_())return;auto f=QFileDialog::getOpenFileName(this,"Windows x64 GenTL producer",{},"GenTL producer (*.cti)");if(!f.isEmpty()){local_.setProducers({f});producer_->setText(f);}});});
    table_=new QTableWidget(0,5);table_->setObjectName("cameraDevices");table_->setHorizontalHeaderLabels({"Kamera","Model","Seri / kimlik","Transport","Durum"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);table_->setSelectionMode(QAbstractItemView::SingleSelection);table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table_->horizontalHeader()->setStretchLastSection(true);table_->setMinimumHeight(150);table_->setMaximumHeight(210);layout->addWidget(table_);
    detail_=text({},"cameraDetail");layout->addWidget(detail_);
    auto* form=new QFormLayout;form->setRowWrapPolicy(QFormLayout::WrapLongRows);layout->addLayout(form);
    shortId_=new QLineEdit;shortId_->setObjectName("cameraShortId");shortId_->setMaxLength(32);shortId_->setPlaceholderText("Boşsa CAM_01 otomatik atanır");form->addRow("Kamera ID",shortId_);
    name_=new QLineEdit;name_->setObjectName("cameraName");name_->setMaxLength(128);form->addRow("Açıklama / ad",name_);
    register_=button(layout,"Ekle","cameraRegister");
    auto* rename=button(layout,"ID / adı kaydet","cameraRename");
    connect(rename,&QPushButton::clicked,this,[this]{attempt([this]{
        if(jobs_.busy() || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
        const int i=table_->currentRow();auto* ws=context_();if(!ws || i<0 || !rows_[i].camera)throw std::runtime_error("Kayıtlı kamera seçin.");
        auto e=ws->environment();ws->renameCamera(rows_[i].camera->id,shortId_->text(),name_->text().trimmed().isEmpty()?shortId_->text():name_->text(),e.revision);refresh();if(onChanged)onChanged();
    });});
    auto* setupCard=new QGroupBox("Optik / Geometri");setupCard->setObjectName("cameraSetupCard");auto* sv=new QVBoxLayout(setupCard);layout->addWidget(setupCard);
    setupStatus_=text("Kayıtlı kamera seçin.","cameraSetupStatus");sv->addWidget(setupStatus_);
    auto* pf=new QFormLayout;pf->setRowWrapPolicy(QFormLayout::WrapLongRows);sv->addLayout(pf);
    auto* af=new QFormLayout;af->setRowWrapPolicy(QFormLayout::WrapLongRows);av->addLayout(af);
    routes_=new QComboBox;routes_->setObjectName("cameraRoutes");routes_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);routes_->setMinimumContentsLength(14);af->addRow("Yerel rota",routes_);
    profiles_=new QComboBox;profiles_->setObjectName("cameraProfiles");pf->addRow("Kurulum / profil",profiles_);pf->labelForField(profiles_)->setObjectName("cameraProfilesLabel");
    auto* defaults=button(sv,"Seçili kurulumu / profili kullan","cameraDefaultsSave");connect(defaults,&QPushButton::clicked,this,[this]{attempt([this]{if(jobs_.busy() || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");auto* ws=context_();const int i=table_->currentRow();if(!ws || i<0 || !rows_[i].camera)throw std::runtime_error("Kayıtlı kamera seçin.");auto e=ws->environment();for(const auto& p:e.current.profiles)if(p.id.value==profiles_->currentData().toString()){ws->selectCameraDefaults(rows_[i].camera->id,shortId_->text(),p.setup,p.id,e.revision);refresh();if(onChanged)onChanged();return;}throw std::runtime_error("Profil seçin.");});});
    auto* draft=button(sv,"Optik / geometriyi düzenle…","cameraProfileEdit");
    connect(draft,&QPushButton::clicked,this,[this]{attempt([this]{editProfile();});});
    connect(button(sv,"Yeni kurulum / profil…","cameraSetupAdd"),&QPushButton::clicked,this,[this]{attempt([this]{editProfile(true);});});
    for(const auto& action:QStringList{"Operational","ArchiveSetup","Archive","OpticalChange"}){
        const auto title=action=="Operational"?"Operasyonel seç":action=="ArchiveSetup"?"Kurulumu arşivle":action=="Archive"?"Kamerayı arşivle":"Optik / kamera konumu değişti";
        connect(button(sv,title,"camera"+action),&QPushButton::clicked,this,[this,action]{attempt([this,action]{
            if(jobs_.busy() || previewActive_ || settings_ || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
            auto* ws=context_();const int i=table_->currentRow();if(!ws || i<0 || !rows_[i].camera)throw std::runtime_error("Kayıtlı kamera seçin.");
            auto e=ws->environment();const auto camera=rows_[i].camera->id;std::optional<SetupId> setup;
            for(const auto& p:e.current.profiles)if(p.id.value==profiles_->currentData().toString())setup=p.setup;
            if(action=="Archive")ws->archive(camera);
            else if(action=="OpticalChange")ws->notifyOpticalChange(camera,e.revision);
            else {if(!setup)throw std::runtime_error("Kurulum / profil seçin.");if(action=="Operational")ws->calibrations().setOperational(camera,*setup);else ws->archiveSetup(*setup);}
            refresh();if(onChanged)onChanged();
            status_->setText(action=="OpticalChange"?"Optik değişikliği bildirildi; ilgili kurulumların kullanım izni kesildi.":"Kamera / kurulum yönetimi kaydedildi.");
        });});
    }
    auto* remember=button(av,"Seçili yerel rotayı hatırla","cameraRememberRoute");
    connect(remember,&QPushButton::clicked,this,[this]{attempt([this]{
        if(jobs_.busy() || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
        const auto i=table_->currentRow();if(i<0 || !rows_[i].discovery || !rows_[i].discovery->industrial || routes_->currentIndex()<0)throw std::runtime_error("Exact kamera ve rota seçin.");
        const auto& group=*rows_[i].discovery->industrial;
        if(inventory_.cancelled || !inventory_.errors.isEmpty() || group.ambiguous)throw std::runtime_error("Tam keşif gerekli.");
        resolveRoutes(group.identity,inventory_.industrial.devices);local_.setRoute(group.identity,group.routes[routes_->currentIndex()].route);status_->setText("Yerel rota hatırlandı; edinim izni değişmedi.");
    });});
    exposure_=new QLineEdit;exposure_->setObjectName("cameraExposure");gain_=new QLineEdit;gain_->setObjectName("cameraGain");
    form->addRow("Exposure (µs)",exposure_);form->addRow("Gain (dB)",gain_);
    timeout_=new QSpinBox;timeout_->setObjectName("cameraTimeout");timeout_->setRange(1,30000);timeout_->setValue(5000);timeout_->setSuffix(" ms");af->addRow("İşlem bütçesi",timeout_);
    runtime_=new QLineEdit(ExperimentalOecsAcquisition::installedRuntime());runtime_->setObjectName("cameraExperimentalRuntime");af->addRow("OECS sürücü DLL",runtime_);
    connect(runtime_,&QLineEdit::textChanged,this,[this]{settingsKnown_=false;exposure_->clear();gain_->clear();controls();});
    auto* commands=new QHBoxLayout;layout->addLayout(commands);settingsRead_=button(commands,"Ayarları oku","cameraSettingsRead");apply_=button(commands,"Uygula ve kaydet","cameraApply");
    auto* capture=new QHBoxLayout;pv->addLayout(capture);preview_=button(capture,"Tek görüntü","cameraPreview");
    auto* previewCommands=new QHBoxLayout;pv->addLayout(previewCommands);live_=button(previewCommands,"Canlı görüntü","cameraLive");stop_=button(previewCommands,"Durdur","cameraStop");
    pv->addWidget(text("Canlı görüntü test önizlemesidir; video dosyası kaydedilmez."));
    check_=button(av,"Seçili GenTL rotasını salt okunur aç / kapat","cameraCheck");
    readback_=text("Gerçek readback yok.","cameraReadback");layout->addWidget(readback_);layout->addWidget(advanced);
    previewStatus_=text("Önizleme yok.","cameraPreviewStatus");pv->addWidget(previewStatus_);
    save_=button(pv,"Görünen kareyi kanıtıyla kaydet…","cameraSaveFrame");connect(save_,&QPushButton::clicked,this,[this]{attempt([this]{saveFrame();});});
    auto* zoom=new QHBoxLayout;pv->addLayout(zoom);
    image_=new ImageViewer;image_->setObjectName("cameraPreviewImage");image_->clearImage("Önizleme yok.");pv->addWidget(image_,1);
    connect(button(zoom,"Sığdır","cameraFit"),&QPushButton::clicked,image_,&ImageViewer::fitImage);
    connect(button(zoom,"1:1","cameraActualSize"),&QPushButton::clicked,image_,&ImageViewer::actualSize);
    connect(button(zoom,"−","cameraZoomOut"),&QPushButton::clicked,this,[this]{image_->zoomBy(1./1.2);});
    connect(button(zoom,"+","cameraZoomIn"),&QPushButton::clicked,this,[this]{image_->zoomBy(1.2);});
    layout->addStretch();
    connect(scan_,&QPushButton::clicked,this,&CameraManager::scan);connect(stop_,&QPushButton::clicked,this,&CameraManager::stop);
    connect(register_,&QPushButton::clicked,this,[this]{attempt([this]{registerSelected();});});
    connect(settingsRead_,&QPushButton::clicked,this,[this]{attempt([this]{this->settings(false);});});
    connect(apply_,&QPushButton::clicked,this,[this]{attempt([this]{this->settings(true);});});
    connect(preview_,&QPushButton::clicked,this,[this]{attempt([this]{start(false);});});
    connect(live_,&QPushButton::clicked,this,[this]{attempt([this]{start(true);});});
    connect(check_,&QPushButton::clicked,this,&CameraManager::checkConnection);
    connect(table_,&QTableWidget::itemSelectionChanged,this,&CameraManager::select);
    connect(routes_,&QComboBox::currentIndexChanged,this,[this]{if(previewActive_)stop();clearFrame();settingsKnown_=false;exposure_->clear();gain_->clear();controls();});
    connect(profiles_,&QComboBox::currentIndexChanged,this,[this]{
        if(previewActive_)stop();clearFrame();settingsKnown_=false;exposure_->clear();gain_->clear();controls();
    });
    timer_=new QTimer(this);timer_->setInterval(20);connect(timer_,&QTimer::timeout,this,[this]{attempt([this]{tick();});});timer_->start();refresh();
}
CameraManager::~CameraManager(){previewSession_.requestShutdown();if(settings_)settings_->stop();settings_.reset();if(job_)jobs_.finish(job_);}
void CameraManager::attempt(std::function<void()> f){try{f();}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));if(previewActive_){previewSession_.requestShutdown();closing_=true;clearFrame();}if(settings_){settings_->stop();settingsClosing_=true;settingsKnown_=false;}controls();}}
void CameraManager::refresh(){attempt([this]{
    const auto selected=table_->currentRow();const auto key=selectedKey_;QSignalBlocker blocker(table_);rows_.clear();table_->setRowCount(0);
    auto* ws=context_();if(ws){auto e=ws->environment();for(const auto& c:e.current.cameras)if(!e.archivedCameras.contains(c.id.value)){
        Row row{c,{}};for(const auto& d:inventory_.devices)if(matches(c,d))row.discovery=d;rows_.append(row);
    }}
    for(const auto& d:inventory_.devices){bool registered=false;for(const auto& row:rows_)if(row.camera && matches(*row.camera,d))registered=true;if(!registered)rows_.append({{},d});}
    for(int i=0;i<rows_.size();++i){const auto& r=rows_[i];QStringList cells;
        if(r.camera){const auto& c=*r.camera;cells={ws->environment().registrations.value(c.id.value).shortId+" • "+c.name,c.physical?c.physical->model:"Webcam",c.physical?c.physical->serial:c.device.providerId,c.physical?"Industrial":"Qt",ws->availability(c)};}
        else if(r.discovery->industrial){const auto& g=*r.discovery->industrial;cells={g.identity.vendor,g.identity.model,g.identity.serial,"Industrial",g.ambiguous?"Belirsiz":"Keşfedildi • kayıtlı değil"};}
        else cells={"Webcam","Qt kamera",r.discovery->webcam->providerId,"Qt","Keşfedildi • kayıtlı değil"};
        table_->insertRow(i);for(int col=0;col<cells.size();++col){auto* item=new QTableWidgetItem(cells[col]);item->setToolTip(cells[col]);table_->setItem(i,col,item);}
    }
    int next=std::clamp(selected,0,std::max(0,int(rows_.size()-1)));for(int i=0;i<rows_.size();++i){const auto& r=rows_[i];const auto k=r.camera?r.camera->id.value:r.discovery->industrial?(r.discovery->industrial->identity.vendor+"/"+r.discovery->industrial->identity.model+"/"+r.discovery->industrial->identity.serial):r.discovery->webcam->providerId;if(k==key)next=i;}
    if(!rows_.isEmpty())table_->selectRow(next);select();
});}
void CameraManager::select(){attempt([this]{
    if(previewActive_)stop();
    clearFrame();settingsKnown_=false;shortId_->clear();
    QSignalBlocker rb(routes_),pb(profiles_);routes_->clear();profiles_->clear();name_->clear();profiles_->hide();findChild<QWidget*>("cameraProfilesLabel")->hide();findChild<QPushButton*>("cameraDefaultsSave")->hide();exposure_->clear();gain_->clear();readback_->setText("Gerçek readback yok.");image_->clearImage("Önizleme yok.");
    const int index=table_->currentRow();if(index<0 || index>=rows_.size()){setupStatus_->setText("Kayıtlı kamera seçin.");detail_->setText("Kamera seçin.");controls();return;}
    const auto& row=rows_[index];selectedKey_=row.camera?row.camera->id.value:row.discovery->industrial?(row.discovery->industrial->identity.vendor+"/"+row.discovery->industrial->identity.model+"/"+row.discovery->industrial->identity.serial):row.discovery->webcam->providerId;name_->setText(row.camera?row.camera->name:row.discovery->industrial?row.discovery->industrial->identity.model:"Webcam");
    if(row.discovery && row.discovery->industrial){const auto& g=*row.discovery->industrial;QString details=g.identity.vendor+" / "+g.identity.model+"\nSeri: "+g.identity.serial;
        for(const auto& d:g.routes){routes_->addItem(d.route.backend+" • "+d.route.interfaceId+" • "+d.route.deviceId);routes_->setItemData(routes_->count()-1,d.route.producer,Qt::ToolTipRole);}
        if(g.routes.size()>1)routes_->setCurrentIndex(-1);
        if(g.identity.usable())if(auto route=local_.route(g.identity))for(int i=0;i<g.routes.size();++i)if(g.routes[i].route==*route)routes_->setCurrentIndex(i);
        detail_->setText(details);
        if(g.routes.size()>1 && routes_->currentIndex()<0){static_cast<QGroupBox*>(advancedBody_->parentWidget())->setChecked(true);detail_->setText(detail_->text()+"\nBirden fazla bağlantı rotası var; Ayrıntılar’dan yerel rota seçin.");}
    }else detail_->setText(row.camera && row.camera->physical?"Kayıtlı kamera çevrimdışı. Yeniden keşif ve açık bağlantı gerekir.":"Webcam • ortak kamera kaydı. Edinim mevcut kalibrasyon/Editor yolundadır.");
    if(row.camera)if(auto* ws=context_()){const auto e=ws->environment();shortId_->setText(e.registrations.value(row.camera->id.value).shortId);const auto defaults=e.defaults(row.camera->id);for(const auto& p:e.current.profiles)if(p.camera==row.camera->id && !e.archivedSetups.contains(p.setup.value))profiles_->addItem(p.name,p.id.value);
        profiles_->setCurrentIndex(-1);if(defaults.profile)for(int i=0;i<profiles_->count();++i)if(profiles_->itemData(i).toString()==defaults.profile->value)profiles_->setCurrentIndex(i);
        const bool ambiguous=defaults.status==CameraDefaultsStatus::Ambiguous;const bool multiple=profiles_->count()>1 || ambiguous || (profiles_->count()>0 && profiles_->currentIndex()<0);profiles_->setVisible(multiple);findChild<QWidget*>("cameraProfilesLabel")->setVisible(multiple);findChild<QPushButton*>("cameraDefaultsSave")->setVisible(multiple);
        if(ambiguous){detail_->setText(detail_->text()+"\nEski kayıt: Optik / Geometri kartından profil seçip seçimi kaydedin.");}
        if(defaults.status==CameraDefaultsStatus::Pending)detail_->setText(detail_->text()+"\nKurulum bilgisi bekleniyor; önizleme için Optik / Geometri kartındaki kurulumu tamamlayın. Ölçüm etkin değil.");
        exposure_->clear();gain_->clear();exposure_->setPlaceholderText("Önce Ayarları oku");gain_->setPlaceholderText("Önce Ayarları oku");
    }controls();
});}
void CameraManager::controls(){
    const bool idle=!afterStop_ && !jobs_.busy() && !previewActive_ && !settings_ && gate_();const int row=table_->currentRow();const bool selected=row>=0 && row<rows_.size();
    scan_->setEnabled(idle);table_->setEnabled(idle);producer_->setEnabled(idle);name_->setEnabled(idle);shortId_->setEnabled(idle);findChild<QPushButton*>("cameraProject")->setVisible(!context_());
    findChild<QPushButton*>("cameraChoose")->setEnabled(idle);stop_->setEnabled(jobs_.busy() || previewActive_ || bool(settings_) || lastFrame_.has_value());
    register_->setEnabled(idle && selected && !rows_[row].camera && context_() && !inventory_.cancelled && inventory_.errors.isEmpty());
    register_->setText(selected && rows_[row].camera?"Kamera kayıtlı":"Ekle");
    register_->setToolTip(!context_()?"Önce bir proje oluşturup kaydedin veya kayıtlı proje açın.":selected && rows_[row].camera?"Bu kamera zaten projede. Kamera ID veya açıklamasını değiştirmek için ID / adı kaydet kullanın.":"Seçili kamerayı projeye ekle; ayar okuma veya edinim onayı gerekmez.");
    findChild<QPushButton*>("cameraRename")->setEnabled(idle && selected && rows_[row].camera.has_value());
    findChild<QPushButton*>("cameraProfileEdit")->setEnabled(idle && selected && rows_[row].camera && rows_[row].camera->physical.has_value());
    for(const auto& name:QStringList{"cameraSetupAdd","cameraOperational","cameraArchiveSetup","cameraArchive","cameraOpticalChange"})
        findChild<QPushButton*>(name)->setEnabled(idle && selected && rows_[row].camera && (name=="cameraArchive" || name=="cameraOpticalChange" || rows_[row].camera->physical.has_value()));
    findChild<QPushButton*>("cameraDefaultsSave")->setEnabled(idle);routes_->setEnabled(idle);profiles_->setEnabled(idle);timeout_->setEnabled(idle);
    IndustrialCameraCapabilities caps;bool route=false;
    if(selected && rows_[row].discovery && rows_[row].discovery->industrial){const auto& g=*rows_[row].discovery->industrial;
        route=routes_->currentIndex()>=0 && routes_->currentIndex()<g.routes.size() && !g.ambiguous && !inventory_.cancelled && inventory_.errors.isEmpty();
        if(route)caps=factory_.capabilities(g.routes[routes_->currentIndex()]);
    }
    if(selected && rows_[row].camera && context_()){const auto e=context_()->environment();
        setupStatus_->setText("Kurulum / profil seçimi gerekli.");
        bool pending=false;
        for(const auto& p:e.current.profiles)if(p.id.value==profiles_->currentData().toString())for(const auto& s:e.current.setups)if(s.id==p.setup){pending=s.pending || p.pending;
            setupStatus_->setText(QString("%1 • %2\nOptik: %3\nDüzlem: %4\n%5×%6 • %7")
                .arg(s.name).arg(s.pending || p.pending?"Eksik / taslak":"Tamamlandı • ölçüm izni ayrıca denetlenir").arg(s.opticalProfile,s.workingPlane)
                .arg(s.geometry.resolution.width()).arg(s.geometry.resolution.height()).arg(e.operational.value(rows_[row].camera->id.value)==s.id.value?"Operasyonel":"Operasyonel değil"));
        }
        findChild<QPushButton*>("cameraProfileEdit")->setText(pending?"Optik / geometri taslağını tamamla…":"Optik / geometriyi düzenle…");
    }else setupStatus_->setText("Kayıtlı kamera seçin.");
    const bool configured=selected && rows_[row].camera && profiles_->currentIndex()>=0 && route;
    bool acquisitionReady=false;if(configured)try{request();acquisitionReady=true;}catch(const std::exception&){}
    const bool exp=selected && rows_[row].camera && AcquisitionPolicy::camera(*rows_[row].camera);
    bool expAvailable=false,settingsAvailable=false;
    if(route){const auto& device=rows_[row].discovery->industrial->routes[routes_->currentIndex()];
        expAvailable=ExperimentalOecsAcquisition::available(device,runtime_->text().trimmed());
        settingsAvailable=CameraSettingsService::available(device,factory_,runtime_->text().trimmed());}
    runtime_->setEnabled(idle);
    save_->setEnabled(lastFrame_.has_value() && experimentalSession_ && !closing_ && previewSession_.state()!=PreviewState::Stopping);
    settingsRead_->setEnabled(idle && configured && settingsAvailable);
    exposure_->setEnabled(idle && configured && settingsKnown_ && settingsAvailable && exposureWritable_);gain_->setEnabled(idle && configured && settingsKnown_ && settingsAvailable && gainWritable_);
    apply_->setEnabled(idle && configured && settingsKnown_ && settingsAvailable);preview_->setEnabled(idle && configured && acquisitionReady && (exp?expAvailable:caps.preview));
    QString guidance;
    if(!context_())guidance="Kamera eklemek için önce proje oluşturup kaydedin veya kayıtlı proje açın.";
    else if(!selected)guidance="Cihazları ara, ardından listeden bir kamera seçin.";
    else if(!rows_[row].camera)guidance="Önce Ekle ile kamerayı projeye kaydedin; ardından desteklenen ayarları okuyabilirsiniz.";
    else if(!rows_[row].camera->physical)guidance="Kamera kayıtlı. Webcam exposure/gain ve bu sayfada önizleme desteklenmiyor; görüntü için Editor / Kalibrasyon kullanın.";
    else if(!route)guidance="Kamera kayıtlı. Ayarlar için yeniden tarayın ve gerekirse Ayrıntılar’dan tek bir bağlantı rotası seçin.";
    else if(!configured)guidance="Kamera kayıtlı. Optik / Geometri kartından kurulum/profil seçimini tamamlayın.";
    else if(!settingsAvailable)guidance="Kamera kayıtlı; bu cihaz için ayar backend’i veya OECS runtime kullanılamıyor.";
    else guidance="Kamera kayıtlı. Ayarları oku → exposure/gain → Uygula ve kaydet. Görüntü almak için Tek görüntü veya Canlı görüntü kullanın.";
    settingsRead_->setToolTip(guidance);
    if(!settingsKnown_ && !settings_){readback_->setText("Gerçek readback yok. "+guidance);const auto hint=configured && settingsAvailable?"Önce Ayarları oku":"Ayar okuma kullanılamıyor";exposure_->setPlaceholderText(hint);gain_->setPlaceholderText(hint);}
    preview_->setToolTip(!exp && !caps.preview?"Bu kamera için görüntü edinimi kullanılamıyor.":"Kayıtlı kurulum/profil ile görüntü testi; ölçüm kabulü değildir.");
    live_->setToolTip(preview_->toolTip());
    live_->setEnabled(preview_->isEnabled());
    check_->setEnabled(idle && route && rows_[row].discovery->industrial->routes[routes_->currentIndex()].route.backend=="gentl");
    findChild<QPushButton*>("cameraRememberRoute")->setEnabled(idle && route);
}
void CameraManager::editProfile(bool add){
    if(jobs_.busy() || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
    auto* ws=context_();const int i=table_->currentRow();if(!ws || i<0 || !rows_[i].camera || !rows_[i].camera->physical)throw std::runtime_error("Kayıtlı endüstriyel kamera gerekli.");
    const auto camera=*rows_[i].camera;auto e=ws->environment();std::optional<AcquisitionProfile> old;std::optional<MeasurementSetupDescriptor> setup;
    for(const auto& p:e.current.profiles)if(!add && p.id.value==profiles_->currentData().toString())old=p;
    if(old)for(const auto& s:e.current.setups)if(s.id==old->setup)setup=s;
    QDialog dialog(this);dialog.setObjectName("cameraProfileDialog");dialog.setWindowTitle(add?"Yeni kurulum / profil":"Optik / geometri düzenle");auto* layout=new QVBoxLayout(&dialog);
    layout->addWidget(text("Tam sensör Mono8, offset 0, binning/decimation 1 ve reverse kapalı istenir. Kaydet gerçek readback veya kalibrasyon etkinleştirmesi değildir."));
    auto* form=new QFormLayout;form->setRowWrapPolicy(QFormLayout::WrapLongRows);layout->addLayout(form);
    auto line=[&](QString label,QString name,QString value){auto* field=new QLineEdit(value);field->setObjectName(name);field->setMaxLength(128);form->addRow(label,field);return field;};
    auto* profileName=line("Profil adı","draftProfileName",old?old->name:QString{});
    auto* lens=line("Optik kurulum / lens","draftLens",setup?setup->opticalProfile:QString{});
    auto* plane=line("Çalışma düzlemi","draftPlane",setup?setup->workingPlane:QString{});
    auto size=[&](QString label,QString name,int value){auto* field=new QSpinBox;field->setObjectName(name);field->setRange(0,1000000);field->setSpecialValueText("Gerekli");field->setValue(value);form->addRow(label,field);return field;};
    const auto geometry=setup && !setup->pending?setup->geometry.resolution:settingsKnown_?settingsGeometry_:QSize{};
    auto* width=size("İstenen sensör genişliği","draftWidth",geometry.width());
    auto* height=size("İstenen sensör yüksekliği","draftHeight",geometry.height());
    auto* exposure=line("Exposure (µs)","draftExposure",old && !old->pending?QString::number(old->requested.exposureTimeUs):settingsKnown_?exposure_->text():QString{});
    auto* gain=line("Gain (dB)","draftGain",old && !old->pending?QString::number(old->requested.gain):settingsKnown_?gain_->text():QString{});
    auto* trigger=new QComboBox;trigger->setObjectName("draftTriggerSelector");trigger->addItems({"Off","FrameStart","FrameBurstStart"});if(old && old->requested.trigger.enabled && trigger->findText(old->requested.trigger.selector)<0)trigger->addItem(old->requested.trigger.selector);trigger->setCurrentText(old?(old->requested.trigger.enabled?old->requested.trigger.selector:"Off"):"FrameStart");form->addRow("Software trigger selector",trigger);
    auto* error=text({});layout->addWidget(error);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{
        if(jobs_.busy() || !gate_() || context_()!=ws)throw std::runtime_error("Çalışma bağlamı değişti.");
        bool a=false,b=false;auto us=exposure->text().toDouble(&a),db=gain->text().toDouble(&b);if(!a || !b)throw std::runtime_error("Sayısal exposure/gain gerekli.");
        GeometryDescriptor g;g.resolution={width->value(),height->value()};g.sensorRoi=QRect(QPoint(0,0),g.resolution);g.binning=QSize(1,1);g.decimation=QSize(1,1);g.reverse=GeometryMirror{};g.complete=true;
        auto next=setup.value_or(MeasurementSetupDescriptor{});if(!setup){next.id={QUuid::createUuid().toString(QUuid::WithoutBraces)};next.camera=camera.id;}
        next.pending=false;next.name=profileName->text();next.opticalProfile=lens->text();next.workingPlane=plane->text();next.geometry=g;next.deviceRevision=camera.device.revision;
        if(setup)next=setup->revised(next);else next.validate();
        auto profile=old.value_or(AcquisitionProfile{});if(!old){profile.id={QUuid::createUuid().toString(QUuid::WithoutBraces)};profile.camera=camera.id;profile.setup=next.id;}
        profile.pending=false;profile.name=profileName->text();profile.requested={"Mono8",(trigger->currentText()=="Off"?TriggerSettings{}:old && old->requested.trigger.enabled && old->requested.trigger.selector==trigger->currentText()?old->requested.trigger:TriggerSettings{true,trigger->currentText(),"Software"}),AutoMode::Off,us,AutoMode::Off,db,old?old->requested.illumination:QString{}};
        if(old)profile=old->revised(profile);else profile.validate();
        auto candidate=e.current;
        if(setup){for(auto& s:candidate.setups)if(s.id==next.id)s=next;}else candidate.setups.append(next);
        if(old){for(auto& p:candidate.profiles)if(p.id==profile.id)p=profile;}else candidate.profiles.append(profile);
        ws->replace(candidate,e.revision);dialog.accept();
    }catch(const std::exception& ex){error->setText(QString::fromUtf8(ex.what()));}});
    dialog.resize(560,560);if(dialog.exec()==QDialog::Accepted){refresh();status_->setText("Kurulum / profil kaydedildi. Cihaza ayar uygulanmadı; edinim yeteneği ayrıca denetlenir.");if(onChanged)onChanged();}
}
void CameraManager::scan(){attempt([this]{
    if(jobs_.busy() || previewActive_ || settings_ || !gate_())return;
    clearFrame();
    auto* ws=context_();const auto captured=ws?ws->environment().projectId:QString{};
    const auto typed=producer_->text().trimmed();auto files=typed.isEmpty()?local_.producers():QStringList{typed};
    auto* watcher=jobs_.watch<CameraInventory>(this,SessionJob::Discovery);auto cancel=jobs_.token(SessionJob::Discovery);status_->setText("Kameralar aranıyor…");controls();
    connect(watcher,&QFutureWatcher<CameraInventory>::finished,this,[this,watcher,captured,files,cancel]{
        auto result=watcher->result();watcher->deleteLater();if(cancel->load()){result.cancelled=true;result.devices.clear();result.industrial.devices.clear();result.industrial.cancelled=true;}
        attempt([&]{inventory_=result;
            auto* current=context_();if((current?current->environment().projectId:QString{})!=captured){inventory_={};status_->setText("Proje değişti; eski keşif atıldı.");refresh();return;}
            if(current)current->observeInventory(inventory_);
            if(!files.isEmpty() && !result.cancelled && result.errors.isEmpty())local_.setProducers(files);
            status_->setText(result.cancelled?"Arama iptal edildi; keşif temizlendi.":QString("%1 kamera grubu. Keşif edinim kabulü değildir.").arg(result.devices.size()));
            if(!result.errors.isEmpty())status_->setText(status_->text()+"\n"+result.errors.join('\n'));refresh();
        });
    });auto scanner=scanner_;jobs_.launch(watcher,[scanner,files,cancel]{try{return scanner(files,cancel);}catch(const std::exception& e){CameraInventory r;r.errors.append(QString::fromUtf8(e.what()));return r;}});
});}
void CameraManager::registerSelected(){
    if(jobs_.busy() || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
    auto* ws=context_();const int i=table_->currentRow();if(!ws || i<0 || !rows_[i].discovery || rows_[i].camera)throw std::runtime_error("Keşfedilmiş, kayıtlı olmayan kamera seçin.");
    auto entry=*rows_[i].discovery;ws->observeInventory(inventory_);const auto result=ws->registerCamera(entry,shortId_->text(),name_->text(),ws->environment().revision);const auto id=result.camera;selectedKey_=id.value;
    if(entry.industrial && routes_->currentIndex()>=0)local_.setRoute(entry.industrial->identity,entry.industrial->routes[routes_->currentIndex()].route);
    refresh();status_->setText((result.existing?"Kamera zaten kayıtlı • CameraId: ":"Kamera kaydedildi • CameraId: ")+id.value+". Kalibrasyon sistem özetinde aynı kayıttır.");if(onChanged)onChanged();
}
IndustrialCameraRequest CameraManager::request() const {
    auto* ws=context_();const int i=table_->currentRow();if(!ws || i<0 || !rows_[i].camera)throw std::runtime_error("Kayıtlı kamera gerekli.");
    IndustrialCameraRequest r;r.camera=*rows_[i].camera;r.sourceId="camera-manager:"+r.camera.id.value;
    const auto e=ws->environment();bool found=false;for(const auto& p:e.current.profiles)if(p.id.value==profiles_->currentData().toString()){r.profile=p;found=true;}
    if(!found)throw std::runtime_error("Ölçüm kurulumu ve acquisition profili gerekli.");
    found=false;for(const auto& s:e.current.setups)if(s.id==r.profile.setup && !e.archivedSetups.contains(s.id.value)){r.setup=s;found=true;}
    if(!found)throw std::runtime_error("Profilin güncel kurulumu gerekli.");
    if(!rows_[i].discovery || !rows_[i].discovery->industrial || routes_->currentIndex()<0)throw std::runtime_error("Yerel rota seçin.");
    if(r.setup.pending || r.profile.pending)throw std::runtime_error("Önce Ayarları oku → Uygula ve kaydet; ardından Optik / Geometri kartındaki kurulumu tamamlayın.");
    r.device=CameraService::resolveIndustrial(r.camera,inventory_.industrial,rows_[i].discovery->industrial->routes[routes_->currentIndex()].route);r.validate();return r;
}
void CameraManager::clearFrame(){
    ++frameRevision_;lastFrame_.reset();image_->clearImage("Önizleme yok.");previewStatus_->setText("Önizleme yok.");
}
void CameraManager::start(bool live){
    if(jobs_.busy() || previewActive_ || settings_ || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
    ++frameRevision_;lastFrame_.reset();image_->clearImage("Önizleme yok.",true);
    auto r=request();experimentalSession_=AcquisitionPolicy::camera(r.camera);
    if(experimentalSession_){
        if(!ExperimentalOecsAcquisition::available(r.device,runtime_->text().trimmed()))throw std::runtime_error("OECS sürücü DLL veya desteklenen cihaz gerekli.");
        AcquisitionPolicy::profile(r.profile);
    }else r.profile.requested.trigger={false,{},{}};
    local_.setRoute(r.device.identity,r.device.route);
    const auto runtime=runtime_->text().trimmed();const auto inventory=inventory_.industrial;const auto factory=factory_;const bool exp=experimentalSession_;const auto api=experimentalApi_;
    job_=jobs_.begin(SessionJob::Connection);previewActive_=true;liveMode_=live;closing_=false;
    previewSession_.start([r,runtime,inventory,factory,exp,api]{return exp?ExperimentalOecsAcquisition::open(r,runtime,api):
        factory.resolveOpen(r,inventory,r.device.route,CameraLeaseUse::Preview,FrameQueueMode::PreviewLatest,1);},live,timeout_->value());
    status_->setText("Önizleme başlatılıyor…");previewStatus_->setText(status_->text());controls();
}
void CameraManager::settings(bool apply){
    if(jobs_.busy() || previewActive_ || settings_ || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
    auto* ws=context_();const int i=table_->currentRow();if(!ws || i<0 || !rows_[i].camera)throw std::runtime_error("Kayıtlı kamera seçin.");
    clearFrame();
    desired_={};if(apply){bool a=false,b=false;double us=exposure_->text().toDouble(&a),db=gain_->text().toDouble(&b);if(!a || !b || !std::isfinite(us) || !std::isfinite(db))throw std::runtime_error("Sayısal exposure/gain gerekli.");desired_={us,db};}
    if(rows_[i].discovery && rows_[i].discovery->industrial && routes_->currentIndex()>=0)local_.setRoute(rows_[i].discovery->industrial->identity,rows_[i].discovery->industrial->routes[routes_->currentIndex()].route);
    settings_=ws->cameraSettings(rows_[i].camera->id,ws->environment().revision,local_,factory_,gate_,runtime_->text().trimmed(),experimentalApi_);
    settingsApply_=apply;settingsClosing_=false;settings_->connect(timeout_->value());job_=jobs_.begin(SessionJob::Connection);status_->setText("Gerçek kamera ayarları okunuyor…");controls();
}
void CameraManager::settingsReadback(){
    const auto s=settings_->snapshot();settingsKnown_=true;
    settingsGeometry_=s.actual.geometry.resolution;
    exposureWritable_=s.exposure.access==FeatureAccess::ReadWrite;gainWritable_=s.gain.access==FeatureAccess::ReadWrite;
    const auto show=[](QLineEdit* field,const CameraFeature& f){
        if(!f.readable() || (f.type!=FeatureType::Float && f.type!=FeatureType::Integer)){field->clear();field->setToolTip("Desteklenmiyor");return;}
        const double value=f.type==FeatureType::Float?std::get<double>(f.value):double(std::get<qint64>(f.value));
        field->setText(QString::number(value,'g',12));field->setToolTip(QString("%1 – %2 %3").arg(f.minimum).arg(f.maximum).arg(f.unit));
    };
    show(exposure_,s.exposure);show(gain_,s.gain);
    readback_->setText(QString("Gerçek readback: %1×%2 • exposure %3 %4 [%5–%6] • gain %7 %8 [%9–%10]")
        .arg(s.actual.geometry.resolution.width()).arg(s.actual.geometry.resolution.height()).arg(exposure_->text()).arg(s.exposure.unit).arg(s.exposure.minimum).arg(s.exposure.maximum).arg(gain_->text()).arg(s.gain.unit).arg(s.gain.minimum).arg(s.gain.maximum));
}
void CameraManager::leave(std::function<void()> done){
    afterStop_=std::move(done);stop();
}
void CameraManager::tick(){
    pollJobs();
    if(afterStop_ && !jobs_.busy() && !previewActive_ && !settings_){
        auto done=std::move(afterStop_);afterStop_={};controls();done();
    }
}
void CameraManager::pollJobs(){
    if(settings_){
        if(auto token=jobs_.token(SessionJob::Connection);token && token->load() && !settingsClosing_)stop();
        if(settingsClosing_){if(settings_->shutdownState()==ShutdownState::Ready){settings_.reset();jobs_.finish(job_);job_=0;settingsClosing_=false;controls();}return;}
        settings_->poll();const auto state=settings_->state();
        if(state==CameraState::Fault){status_->setText(settings_->error());settingsKnown_=false;settings_->stop();settingsClosing_=true;return;}
        if(state==CameraState::Connected){if(settingsApply_){settingsApply_=false;settings_->apply(desired_,timeout_->value());}else{settingsReadback();status_->setText("Gerçek ayarlar okundu; değişiklik kaydedilmedi.");settings_->stop();settingsClosing_=true;}}
        else if(state==CameraState::Configured){settingsReadback();status_->setText("Ayarlar gerçek readback ile doğrulandı ve kaydedildi.");if(onChanged)onChanged();settings_->stop();settingsClosing_=true;}
        return;
    }
    if(!previewActive_)return;
    if(auto token=jobs_.token(SessionJob::Connection);token && token->load() && !closing_)stop();
    previewSession_.poll();
    if(previewSession_.state()==PreviewState::Error){
        clearFrame();status_->setText("Görüntü alınamadı: "+previewSession_.error());
    }else if(auto frame=previewSession_.takeLatest();frame && frame->generation==previewSession_.generation() && !closing_ && !frameDialogOpen_){
        ++frameRevision_;lastFrame_=std::move(frame->frame);
        image_->setPreviewPixels(lastFrame_->image.preview(),selectedKey_+"/"+profiles_->currentData().toString());
        const auto stats=previewSession_.stats();
        status_->setText(QString("Önizleme • %1 • %2 • %3 kare • %4 FPS • ölçüm veya çevrim sonucu değildir.")
            .arg(liveMode_?"Canlı":"Tek görüntü")
            .arg(frame->kind==PreviewAcquisitionKind::NativeContinuous?"Continuous":"Sıralı tek kare")
            .arg(stats.completed).arg(stats.fps,0,'f',1));
    }
    if(previewSession_.drained()){
        if(closing_ && previewSession_.state()!=PreviewState::Error)status_->setText("Önizleme durduruldu • cihaz serbest.");
        previewActive_=false;jobs_.finish(job_);job_=0;closing_=false;
    }
    previewStatus_->setText(status_->text().section('\n',0,0));
    controls();
}
void CameraManager::stop(){
    if(jobs_.busy(SessionJob::Discovery))jobs_.stop(SessionJob::Discovery);
    if(!previewActive_ && jobs_.busy(SessionJob::Connection))jobs_.stop(SessionJob::Connection);
    if(settings_){settings_->stop();settingsClosing_=true;settingsKnown_=false;}
    previewSession_.stop(timeout_->value());closing_=previewActive_;
    clearFrame();status_->setText(previewActive_?"Durduruluyor • cihazın kapanması bekleniyor…":"Önizleme durduruldu • görüntü temizlendi.");previewStatus_->setText(status_->text());controls();
}
void CameraManager::saveFrame(){
    if(!lastFrame_ || !experimentalSession_ || closing_ || previewSession_.state()==PreviewState::Stopping)throw std::runtime_error("Kaydedilebilir kare yok.");
    // Hold the visible raster while the chooser runs its nested event loop.
    // Stop/error/selection still retire it, and the revision is rechecked below.
    QScopedValueRollback<bool> dialogGuard(frameDialogOpen_,true);
    const auto revision=frameRevision_;
    auto path=framePathChooser_(this);
    if(path.isEmpty())return;if(!path.endsWith(".pgm",Qt::CaseInsensitive))path+=".pgm";
    if(!lastFrame_ || !experimentalSession_ || closing_ || revision!=frameRevision_)throw std::runtime_error("Görünen kare değişti veya durduruldu; kayıt iptal edildi.");
    ExperimentalOecsAcquisition::save(*lastFrame_,path);status_->setText("Kaydedildi: "+path+" + .json");
}
void CameraManager::checkConnection(){attempt([this]{
    if(jobs_.busy() || !gate_())return;const auto i=table_->currentRow();if(i<0 || !rows_[i].discovery || !rows_[i].discovery->industrial || routes_->currentIndex()<0)return;
    const auto d=rows_[i].discovery->industrial->routes[routes_->currentIndex()];
    CameraDevice raw{d.route.producer,d.route.interfaceId,d.route.deviceId,d.identity.vendor,d.identity.model,d.identity.serial,{}};
    auto* w=jobs_.watch<QString>(this,SessionJob::Connection);const auto cancel=jobs_.token(SessionJob::Connection);status_->setText("Salt okunur bağlantı kontrolü…");controls();
    connect(w,&QFutureWatcher<QString>::finished,this,[this,w,cancel]{const auto error=w->result();w->deleteLater();status_->setText(cancel->load()?"Kontrol iptal edildi; bekleyen çağrı tamamlandı. Görüntü alınmadı.":error.isEmpty()?"Salt okunur aç/kapat tamamlandı. Cihaz kontrol sonunda kapatıldı; görüntü alınmadı.":error);controls();});
    jobs_.launch(w,[raw]{try{CameraService{}.checkConnection(raw);return QString{};}catch(const std::exception& e){return QString::fromUtf8(e.what());}});
});}
}
