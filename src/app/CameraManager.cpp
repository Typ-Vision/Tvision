#include "CameraManager.hpp"
#include "application/ExperimentalOecsAcquisition.hpp"
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
#include <QPixmap>
namespace visionflow {
namespace {
QLabel* text(QString value,QString name={}){auto* l=new QLabel(value);l->setObjectName(name);l->setWordWrap(true);l->setTextFormat(Qt::PlainText);l->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);return l;}
QPushButton* button(QLayout* l,QString value,QString name){auto* b=new QPushButton(value);b->setObjectName(name);l->addWidget(b);return b;}
bool matches(const MeasurementCamera& c,const CameraDiscoveryEntry& d){return c.physical ? d.industrial && *c.physical==d.industrial->identity : d.webcam && c.device.adapter==d.webcam->adapter && c.device.providerId==d.webcam->providerId;}
}
CameraManager::CameraManager(std::function<MeasurementWorkspace*()> context,std::function<bool()> gate,CameraSession& jobs,
    QString settings,QWidget* parent,Scanner scanner,CameraProviderFactory factory)
    :QWidget(parent),context_(std::move(context)),gate_(std::move(gate)),jobs_(jobs),local_(std::move(settings)),scanner_(std::move(scanner)),factory_(std::move(factory)) {
    setObjectName("cameraManager");if(!scanner_)scanner_=[](const auto& files,const auto& cancel){return CameraService{}.inventory(files,1000,cancel);};
    auto* outer=new QVBoxLayout(this);auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);outer->addWidget(scroll);
    auto* body=new QWidget;scroll->setWidget(body);auto* layout=new QVBoxLayout(body);
    layout->addWidget(text("Kamera yönetimi","cameraTitle"));
    layout->addWidget(text("Kamera kaydı ve keşif edinim izni vermez. Önizleme ölçüm veya çevrim sonucu değildir."));
    auto* actions=new QHBoxLayout;layout->addLayout(actions);scan_=button(actions,"Cihazları ara","cameraScan");stop_=button(actions,"Durdur","cameraStop");
    status_=text("Henüz arama yapılmadı. Kayıtlar için kaydedilmiş proje açın.","cameraStatus");layout->addWidget(status_);
    auto* advanced=new QGroupBox("Bağlantı ayrıntıları / yerel producer");advanced->setCheckable(true);advanced->setChecked(false);auto* av=new QVBoxLayout(advanced);
    producer_=new QLineEdit;producer_->setObjectName("cameraProducer");producer_->setPlaceholderText("Boşsa hatırlanan producer'lar ve kurulu GenTL ortamı kullanılır");av->addWidget(producer_);
    auto* choose=button(av,"Producer seç…","cameraChoose");layout->addWidget(advanced);
    producer_->hide();choose->hide();connect(advanced,&QGroupBox::toggled,this,[this,choose](bool visible){producer_->setVisible(visible);choose->setVisible(visible);});
    connect(choose,&QPushButton::clicked,this,[this]{attempt([this]{if(jobs_.busy() || !gate_())return;auto f=QFileDialog::getOpenFileName(this,"Windows x64 GenTL producer",{},"GenTL producer (*.cti)");if(!f.isEmpty()){local_.setProducers({f});producer_->setText(f);}});});
    table_=new QTableWidget(0,5);table_->setObjectName("cameraDevices");table_->setHorizontalHeaderLabels({"Kamera","Model","Seri / kimlik","Transport","Durum"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);table_->setSelectionMode(QAbstractItemView::SingleSelection);table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table_->horizontalHeader()->setStretchLastSection(true);table_->setMinimumHeight(190);layout->addWidget(table_);
    detail_=text({},"cameraDetail");layout->addWidget(detail_);
    auto* form=new QFormLayout;form->setRowWrapPolicy(QFormLayout::WrapLongRows);layout->addLayout(form);
    name_=new QLineEdit;name_->setObjectName("cameraName");name_->setMaxLength(128);form->addRow("Kayıt adı",name_);
    register_=button(layout,"Seçili kamerayı projeye kaydet","cameraRegister");
    auto* rename=button(layout,"Kayıt adını güncelle","cameraRename");
    connect(rename,&QPushButton::clicked,this,[this]{attempt([this]{
        if(jobs_.busy() || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
        const int i=table_->currentRow();auto* ws=context_();if(!ws || i<0 || !rows_[i].camera)throw std::runtime_error("Kayıtlı kamera seçin.");
        auto e=ws->environment();for(auto& c:e.current.cameras)if(c.id==rows_[i].camera->id){auto next=c;next.name=name_->text().trimmed();c=c.rebound(next);}
        ws->replace(e.current,e.revision);refresh();if(onChanged)onChanged();
    });});
    routes_=new QComboBox;routes_->setObjectName("cameraRoutes");routes_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);routes_->setMinimumContentsLength(14);form->addRow("Yerel rota",routes_);
    profiles_=new QComboBox;profiles_->setObjectName("cameraProfiles");form->addRow("Kurulum / acquisition profili",profiles_);
    auto* draft=button(layout,"Kurulum / profil taslağını düzenle","cameraProfileEdit");
    connect(draft,&QPushButton::clicked,this,[this]{attempt([this]{editProfile();});});
    auto* remember=button(layout,"Seçili yerel rotayı hatırla","cameraRememberRoute");
    connect(remember,&QPushButton::clicked,this,[this]{attempt([this]{
        if(jobs_.busy() || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
        const auto i=table_->currentRow();if(i<0 || !rows_[i].discovery || !rows_[i].discovery->industrial || routes_->currentIndex()<0)throw std::runtime_error("Exact kamera ve rota seçin.");
        const auto& group=*rows_[i].discovery->industrial;
        if(inventory_.cancelled || !inventory_.errors.isEmpty() || group.ambiguous)throw std::runtime_error("Tam keşif gerekli.");
        resolveRoutes(group.identity,inventory_.industrial.devices);local_.setRoute(group.identity,group.routes[routes_->currentIndex()].route);status_->setText("Yerel rota hatırlandı; edinim izni değişmedi.");
    });});
    exposure_=new QLineEdit;exposure_->setObjectName("cameraExposure");gain_=new QLineEdit;gain_->setObjectName("cameraGain");
    form->addRow("Exposure (µs)",exposure_);form->addRow("Gain (dB)",gain_);
    timeout_=new QSpinBox;timeout_->setObjectName("cameraTimeout");timeout_->setRange(1,30000);timeout_->setValue(1000);timeout_->setSuffix(" ms");form->addRow("İşlem bütçesi",timeout_);
    experimental_=new QCheckBox("Deneysel OECS edinimini kullan");experimental_->setObjectName("cameraExperimental");layout->addWidget(experimental_);
    experimentalNotice_=text(ExperimentalOecsAcquisition::notice(),"cameraExperimentalNotice");outer->insertWidget(0,experimentalNotice_);experimentalNotice_->hide();
    runtime_=new QLineEdit(ExperimentalOecsAcquisition::installedRuntime());runtime_->setObjectName("cameraExperimentalRuntime");form->addRow("Deneysel OECS runtime DLL",runtime_);runtime_->hide();form->labelForField(runtime_)->hide();
    connect(experimental_,&QCheckBox::toggled,this,[this,form](bool on){runtime_->setVisible(on);form->labelForField(runtime_)->setVisible(on);experimentalNotice_->setVisible(on);lastFrame_.reset();image_->clear();image_->setText("Önizleme yok.");controls();});
    connect(runtime_,&QLineEdit::textChanged,this,[this]{controls();});
    auto* commands=new QHBoxLayout;layout->addLayout(commands);apply_=button(commands,"Ayarları uygula","cameraApply");preview_=button(commands,"Önizleme","cameraPreview");
    check_=button(layout,"Seçili GenTL rotasını salt okunur aç / kapat","cameraCheck");
    readback_=text("Gerçek readback yok.","cameraReadback");layout->addWidget(readback_);
    save_=button(layout,"Görünen kareyi kanıtıyla kaydet…","cameraSaveFrame");connect(save_,&QPushButton::clicked,this,[this]{attempt([this]{saveFrame();});});
    image_=text("Önizleme yok.","cameraPreviewImage");image_->setAlignment(Qt::AlignCenter);image_->setMinimumHeight(100);layout->addWidget(image_);
    connect(scan_,&QPushButton::clicked,this,&CameraManager::scan);connect(stop_,&QPushButton::clicked,this,&CameraManager::stop);
    connect(register_,&QPushButton::clicked,this,[this]{attempt([this]{registerSelected();});});
    connect(apply_,&QPushButton::clicked,this,[this]{attempt([this]{start(false);});});
    connect(preview_,&QPushButton::clicked,this,[this]{attempt([this]{start(true);});});
    connect(check_,&QPushButton::clicked,this,&CameraManager::checkConnection);
    connect(table_,&QTableWidget::itemSelectionChanged,this,&CameraManager::select);
    connect(routes_,&QComboBox::currentIndexChanged,this,[this]{controls();});
    connect(profiles_,&QComboBox::currentIndexChanged,this,[this]{
        attempt([this]{auto* ws=context_();if(!ws)return;for(const auto& p:ws->environment().current.profiles)if(p.id.value==profiles_->currentData().toString()){exposure_->setText(QString::number(p.requested.exposureTimeUs,'g',12));gain_->setText(QString::number(p.requested.gain,'g',12));}controls();});
    });
    timer_=new QTimer(this);timer_->setInterval(20);connect(timer_,&QTimer::timeout,this,[this]{attempt([this]{tick();});});timer_->start();refresh();
}
CameraManager::~CameraManager(){if(worker_)worker_->requestShutdown();worker_.reset();if(job_)jobs_.finish(job_);}
void CameraManager::attempt(std::function<void()> f){try{f();}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));if(worker_){worker_->requestShutdown();closing_=true;}controls();}}
void CameraManager::refresh(){attempt([this]{
    const auto selected=table_->currentRow();QSignalBlocker blocker(table_);rows_.clear();table_->setRowCount(0);
    auto* ws=context_();if(ws){auto e=ws->environment();for(const auto& c:e.current.cameras)if(!e.archivedCameras.contains(c.id.value)){
        Row row{c,{}};for(const auto& d:inventory_.devices)if(matches(c,d))row.discovery=d;rows_.append(row);
    }}
    for(const auto& d:inventory_.devices){bool registered=false;for(const auto& row:rows_)if(row.camera && matches(*row.camera,d))registered=true;if(!registered)rows_.append({{},d});}
    for(int i=0;i<rows_.size();++i){const auto& r=rows_[i];QStringList cells;
        if(r.camera){const auto& c=*r.camera;cells={c.name,c.physical?c.physical->model:"Webcam",c.physical?c.physical->serial:c.device.providerId,c.physical?"Industrial":"Qt",ws->availability(c)};}
        else if(r.discovery->industrial){const auto& g=*r.discovery->industrial;cells={g.identity.vendor,g.identity.model,g.identity.serial,"Industrial",g.ambiguous?"Belirsiz":"Keşfedildi • kayıtlı değil"};}
        else cells={"Webcam","Qt kamera",r.discovery->webcam->providerId,"Qt","Keşfedildi • kayıtlı değil"};
        table_->insertRow(i);for(int col=0;col<cells.size();++col){auto* item=new QTableWidgetItem(cells[col]);item->setToolTip(cells[col]);table_->setItem(i,col,item);}
    }
    if(!rows_.isEmpty())table_->selectRow(std::clamp(selected,0,int(rows_.size()-1)));select();
});}
void CameraManager::select(){attempt([this]{
    lastFrame_.reset();
    QSignalBlocker rb(routes_),pb(profiles_);routes_->clear();profiles_->clear();name_->clear();exposure_->clear();gain_->clear();readback_->setText("Gerçek readback yok.");image_->clear();image_->setText("Önizleme yok.");
    const int index=table_->currentRow();if(index<0 || index>=rows_.size()){detail_->setText("Kamera seçin.");controls();return;}
    const auto& row=rows_[index];name_->setText(row.camera?row.camera->name:row.discovery->industrial?row.discovery->industrial->identity.model:"Webcam");
    if(row.discovery && row.discovery->industrial){const auto& g=*row.discovery->industrial;QString details=g.identity.vendor+" / "+g.identity.model+"\nSeri: "+g.identity.serial;
        for(const auto& d:g.routes){routes_->addItem(d.route.backend+" • "+d.route.interfaceId+" • "+d.route.deviceId);routes_->setItemData(routes_->count()-1,d.route.producer,Qt::ToolTipRole);}
        if(g.routes.size()>1)routes_->setCurrentIndex(-1);
        if(g.identity.usable())if(auto route=local_.route(g.identity))for(int i=0;i<g.routes.size();++i)if(g.routes[i].route==*route)routes_->setCurrentIndex(i);
        detail_->setText(details+"\n"+factory_.capabilities(g.routes.front()).unavailableReason);
    }else detail_->setText(row.camera && row.camera->physical?"Kayıtlı kamera çevrimdışı. Yeniden keşif ve açık bağlantı gerekir.":"Webcam • ortak kamera kaydı. Edinim mevcut kalibrasyon/Editor yolundadır.");
    if(row.camera)if(auto* ws=context_()){const auto e=ws->environment();for(const auto& p:e.current.profiles)if(p.camera==row.camera->id)profiles_->addItem(p.name,p.id.value);
        if(profiles_->count())for(const auto& p:e.current.profiles)if(p.id.value==profiles_->currentData().toString()){exposure_->setText(QString::number(p.requested.exposureTimeUs,'g',12));gain_->setText(QString::number(p.requested.gain,'g',12));}
    }controls();
});}
void CameraManager::controls(){
    const bool idle=!jobs_.busy() && !worker_ && gate_();const int row=table_->currentRow();const bool selected=row>=0 && row<rows_.size();
    scan_->setEnabled(idle);table_->setEnabled(idle);producer_->setEnabled(idle);name_->setEnabled(idle);
    findChild<QPushButton*>("cameraChoose")->setEnabled(idle);stop_->setEnabled(jobs_.busy() || bool(worker_));
    register_->setEnabled(idle && selected && !rows_[row].camera && context_() && !inventory_.cancelled && inventory_.errors.isEmpty());
    findChild<QPushButton*>("cameraRename")->setEnabled(idle && selected && rows_[row].camera.has_value());
    findChild<QPushButton*>("cameraProfileEdit")->setEnabled(idle && selected && rows_[row].camera && rows_[row].camera->physical.has_value());
    routes_->setEnabled(idle);profiles_->setEnabled(idle);timeout_->setEnabled(idle);
    IndustrialCameraCapabilities caps;bool route=false;
    if(selected && rows_[row].discovery && rows_[row].discovery->industrial){const auto& g=*rows_[row].discovery->industrial;
        route=routes_->currentIndex()>=0 && routes_->currentIndex()<g.routes.size() && !g.ambiguous && !inventory_.cancelled && inventory_.errors.isEmpty();
        if(route)caps=factory_.capabilities(g.routes[routes_->currentIndex()]);
    }
    const bool configured=selected && rows_[row].camera && profiles_->count() && route;
    const bool exp=experimental_->isChecked();
    bool expAvailable=false;
    if(route)expAvailable=ExperimentalOecsAcquisition::available(rows_[row].discovery->industrial->routes[routes_->currentIndex()],runtime_->text().trimmed());
    experimental_->setEnabled(idle);runtime_->setEnabled(idle);
    save_->setEnabled(lastFrame_.has_value() && experimentalSession_ && !closing_);
    exposure_->setEnabled(idle && configured && (exp?expAvailable:caps.exposure));gain_->setEnabled(idle && configured && (exp?expAvailable:caps.gain));
    apply_->setEnabled(idle && configured && !exp && caps.configure);preview_->setEnabled(idle && configured && (exp?expAvailable:caps.preview));
    check_->setEnabled(idle && route && rows_[row].discovery->industrial->routes[routes_->currentIndex()].route.backend=="gentl");
    findChild<QPushButton*>("cameraRememberRoute")->setEnabled(idle && route);
}
void CameraManager::editProfile(){
    if(jobs_.busy() || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
    auto* ws=context_();const int i=table_->currentRow();if(!ws || i<0 || !rows_[i].camera || !rows_[i].camera->physical)throw std::runtime_error("Kayıtlı endüstriyel kamera gerekli.");
    const auto camera=*rows_[i].camera;auto e=ws->environment();std::optional<AcquisitionProfile> old;std::optional<MeasurementSetupDescriptor> setup;
    for(const auto& p:e.current.profiles)if(p.id.value==profiles_->currentData().toString())old=p;
    if(old)for(const auto& s:e.current.setups)if(s.id==old->setup)setup=s;
    QDialog dialog(this);dialog.setObjectName("cameraProfileDialog");dialog.setWindowTitle("İstenen kamera ayarları • çevrimdışı taslak");auto* layout=new QVBoxLayout(&dialog);
    layout->addWidget(text("Tam sensör Mono8, offset 0, binning/decimation 1 ve reverse kapalı istenir. Kaydet gerçek readback veya kalibrasyon etkinleştirmesi değildir."));
    auto* form=new QFormLayout;form->setRowWrapPolicy(QFormLayout::WrapLongRows);layout->addLayout(form);
    auto line=[&](QString label,QString name,QString value){auto* field=new QLineEdit(value);field->setObjectName(name);field->setMaxLength(128);form->addRow(label,field);return field;};
    auto* profileName=line("Profil adı","draftProfileName",old?old->name:QString{});
    auto* lens=line("Optik kurulum / lens","draftLens",setup?setup->opticalProfile:QString{});
    auto* plane=line("Çalışma düzlemi","draftPlane",setup?setup->workingPlane:QString{});
    auto size=[&](QString label,QString name,int value){auto* field=new QSpinBox;field->setObjectName(name);field->setRange(0,1000000);field->setSpecialValueText("Gerekli");field->setValue(value);form->addRow(label,field);return field;};
    auto* width=size("İstenen sensör genişliği","draftWidth",setup?setup->geometry.resolution.width():0);
    auto* height=size("İstenen sensör yüksekliği","draftHeight",setup?setup->geometry.resolution.height():0);
    auto* exposure=line("Exposure (µs)","draftExposure",old?QString::number(old->requested.exposureTimeUs):QString{});
    auto* gain=line("Gain (dB)","draftGain",old?QString::number(old->requested.gain):QString{});
    auto* error=text({});layout->addWidget(error);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{
        if(jobs_.busy() || !gate_() || context_()!=ws)throw std::runtime_error("Çalışma bağlamı değişti.");
        bool a=false,b=false;auto us=exposure->text().toDouble(&a),db=gain->text().toDouble(&b);if(!a || !b)throw std::runtime_error("Sayısal exposure/gain gerekli.");
        GeometryDescriptor g;g.resolution={width->value(),height->value()};g.sensorRoi=QRect(QPoint(0,0),g.resolution);g.binning=QSize(1,1);g.decimation=QSize(1,1);g.reverse=GeometryMirror{};g.complete=true;
        auto next=setup.value_or(MeasurementSetupDescriptor{});if(!setup){next.id={QUuid::createUuid().toString(QUuid::WithoutBraces)};next.camera=camera.id;}
        next.name=profileName->text();next.opticalProfile=lens->text();next.workingPlane=plane->text();next.geometry=g;next.deviceRevision=camera.device.revision;
        if(setup)next=setup->revised(next);else next.validate();
        auto profile=old.value_or(AcquisitionProfile{});if(!old){profile.id={QUuid::createUuid().toString(QUuid::WithoutBraces)};profile.camera=camera.id;profile.setup=next.id;}
        profile.name=profileName->text();profile.requested={"Mono8",old?old->requested.trigger:TriggerSettings{true,"FrameStart","Software"},AutoMode::Off,us,AutoMode::Off,db,old?old->requested.illumination:QString{}};
        if(old)profile=old->revised(profile);else profile.validate();
        auto candidate=e.current;
        if(setup){for(auto& s:candidate.setups)if(s.id==next.id)s=next;}else candidate.setups.append(next);
        if(old){for(auto& p:candidate.profiles)if(p.id==profile.id)p=profile;}else candidate.profiles.append(profile);
        ws->replace(candidate,e.revision);dialog.accept();
    }catch(const std::exception& ex){error->setText(QString::fromUtf8(ex.what()));}});
    dialog.resize(560,560);if(dialog.exec()==QDialog::Accepted){refresh();status_->setText("İstenen profil taslağı kaydedildi. Gerçek readback yok; edinim yeteneği ayrıca denetlenir.");if(onChanged)onChanged();}
}
void CameraManager::scan(){attempt([this]{
    if(jobs_.busy() || worker_ || !gate_())return;
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
    auto entry=*rows_[i].discovery;ws->observeInventory(inventory_);const auto id=ws->registerCamera(entry,name_->text());
    if(entry.industrial && routes_->currentIndex()>=0)local_.setRoute(entry.industrial->identity,entry.industrial->routes[routes_->currentIndex()].route);
    refresh();status_->setText("Kamera kaydedildi • CameraId: "+id.value+". Kalibrasyon sistem özetinde aynı kayıttır.");if(onChanged)onChanged();
}
IndustrialCameraRequest CameraManager::request() const {
    auto* ws=context_();const int i=table_->currentRow();if(!ws || i<0 || !rows_[i].camera)throw std::runtime_error("Kayıtlı kamera gerekli.");
    IndustrialCameraRequest r;r.camera=*rows_[i].camera;r.sourceId="camera-manager:"+r.camera.id.value;
    const auto e=ws->environment();bool found=false;for(const auto& p:e.current.profiles)if(p.id.value==profiles_->currentData().toString()){r.profile=p;found=true;}
    if(!found)throw std::runtime_error("Ölçüm kurulumu ve acquisition profili gerekli.");
    found=false;for(const auto& s:e.current.setups)if(s.id==r.profile.setup && !e.archivedSetups.contains(s.id.value)){r.setup=s;found=true;}
    if(!found)throw std::runtime_error("Profilin güncel kurulumu gerekli.");
    if(!rows_[i].discovery || !rows_[i].discovery->industrial || routes_->currentIndex()<0)throw std::runtime_error("Yerel rota seçin.");
    r.device=CameraService::resolveIndustrial(r.camera,inventory_.industrial,rows_[i].discovery->industrial->routes[routes_->currentIndex()].route);r.validate();return r;
}
void CameraManager::start(bool preview){
    if(jobs_.busy() || worker_ || !gate_())throw std::runtime_error("Kamera/Runtime meşgul.");
    auto r=request();bool a=false,b=false;const double exposure=exposure_->text().toDouble(&a),gain=gain_->text().toDouble(&b);
    if(!a || !b || !std::isfinite(exposure) || !std::isfinite(gain))throw std::runtime_error("Sayısal exposure/gain gerekli.");
    auto desired=r.profile;desired.requested.exposureTimeUs=exposure;desired.requested.gain=gain;desired=r.profile.revised(desired);
    auto* ws=context_();auto e=ws->environment();for(auto& p:e.current.profiles)if(p.id==desired.id)p=desired;ws->replace(e.current,e.revision);r.profile=desired;
    experimentalSession_=experimental_->isChecked();
    if(experimentalSession_){
        if(!preview || !ExperimentalOecsAcquisition::available(r.device,runtime_->text().trimmed()))throw std::runtime_error("Deneysel OECS runtime veya desteklenen cihaz gerekli.");
        r.profile.requested.trigger={true,"FrameBurstStart","Software"};
    }
    local_.setRoute(r.device.identity,r.device.route);if(preview && !experimentalSession_)r.profile.requested.trigger={false,{},{}};
    auto opened=experimentalSession_?ExperimentalOecsAcquisition::open(r,runtime_->text().trimmed()):preview?factory_.resolveOpen(r,inventory_.industrial,r.device.route,CameraLeaseUse::Editor,FrameQueueMode::PreviewLatest):
                        factory_.configure(r,CameraLeaseUse::Editor);
    job_=jobs_.begin(SessionJob::Connection);worker_=std::move(opened);previewing_=preview;closing_=false;
    lastFrame_.reset();image_->clear();image_->setText("Önizleme yok.");readback_->setText("Gerçek readback bekleniyor…");worker_->submit(CameraOperation::Connect,timeout_->value());controls();
}
void CameraManager::tick(){
    if(!worker_)return;
    if(auto token=jobs_.token(SessionJob::Connection);token && token->load())stop();
    if(closing_){if(worker_->shutdownState()==ShutdownState::Ready){const auto error=worker_->lifecycle().error();worker_.reset();jobs_.finish(job_);job_=0;closing_=false;
        if(!error.contains("shut down"))status_->setText(error);controls();}return;}
    const bool accepted=worker_->poll();const auto state=worker_->lifecycle().state();
    if(state==CameraState::Fault){status_->setText(worker_->lifecycle().error());stop();return;}
    if(!accepted)return;
    if(state==CameraState::Connected)worker_->submit(CameraOperation::Configure,timeout_->value());
    else if(state==CameraState::Configured){
        if(auto actual=worker_->readback())readback_->setText(QString("Gerçek readback: %1×%2 Mono8 • exposure %3 µs • gain %4 dB").arg(actual->geometry.resolution.width()).arg(actual->geometry.resolution.height()).arg(actual->settings.exposureTimeUs).arg(actual->settings.gain));
        if(previewing_)worker_->submit(CameraOperation::Start,timeout_->value());else {status_->setText("Ayarlar gerçek readback ile doğrulandı.");stop();}
    }else if(state==CameraState::Streaming){
        if(auto frame=worker_->take()){image_->setPixmap(QPixmap::fromImage(frame->image.preview()).scaled(360,160,Qt::KeepAspectRatio,Qt::FastTransformation));lastFrame_=*frame;status_->setText(experimentalSession_?ExperimentalOecsAcquisition::notice():QString("Önizleme • ölçüm veya çevrim sonucu değildir."));controls();}
        worker_->submit(CameraOperation::Grab,timeout_->value());
    }
}
void CameraManager::stop(){
    if(jobs_.busy(SessionJob::Discovery))jobs_.stop(SessionJob::Discovery);
    if(!worker_ && jobs_.busy(SessionJob::Connection))jobs_.stop(SessionJob::Connection);
    if(worker_){worker_->requestStop(timeout_->value());worker_->requestShutdown(timeout_->value());closing_=true;}
    lastFrame_.reset();image_->clear();image_->setText("Önizleme durduruldu • görüntü temizlendi.");controls();
}
void CameraManager::saveFrame(){
    if(!lastFrame_ || !experimentalSession_ || closing_)throw std::runtime_error("Kaydedilebilir deneysel kare yok.");
    const auto frame=*lastFrame_;
    auto path=QFileDialog::getSaveFileName(this,"Deneysel kare ve doğrulanmamış eşleşme kaydı",{},"Mono8 raster (*.pgm)");
    if(path.isEmpty())return;if(!path.endsWith(".pgm",Qt::CaseInsensitive))path+=".pgm";
    ExperimentalOecsAcquisition::save(frame,path);status_->setText(ExperimentalOecsAcquisition::notice()+"\nKaydedildi: "+path+" + .json");
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
