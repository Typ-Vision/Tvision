#include "CalibrationOverview.hpp"
#include "CalibrationWizard.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QProgressBar>
#include <QCloseEvent>
#include "application/MeasurementProjectTransfer.hpp"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QUuid>
#include <QTimer>
namespace visionflow {
namespace {
class TransferProgressDialog final:public QDialog {public:using QDialog::QDialog;void reject() override{}protected:void closeEvent(QCloseEvent* e) override{e->ignore();}};
QLabel* label(QString text){auto* l=new QLabel(text);l->setWordWrap(true);l->setTextFormat(Qt::PlainText);l->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);return l;}
QPushButton* button(QLayout* layout,QString text,QString name){auto* b=new QPushButton(text);b->setObjectName(name);layout->addWidget(b);return b;}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
}
CalibrationOverview::CalibrationOverview(std::function<MeasurementWorkspace*()> context,QWidget* parent):QWidget(parent),context_(std::move(context)){
setObjectName("calibrationOverview");auto* outer=new QVBoxLayout(this);auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);outer->addWidget(scroll);auto* body=new QWidget;auto* layout=new QVBoxLayout(body);scroll->setWidget(body);
auto* title=label("Kalibrasyon • Kameralar");title->setStyleSheet("font-size:22px;font-weight:600");layout->addWidget(title);
summary_=label("");summary_->setObjectName("overviewSummary");layout->addWidget(summary_);
auto* actions=new QGridLayout;
auto command=[&](QString text,QString name,int row,int col,std::function<void()> f){auto* b=new QPushButton(text);b->setObjectName(name);actions->addWidget(b,row,col);connect(b,&QPushButton::clicked,this,[this,f]{attempt(f);});};
command("Kamera / kurulum ekle","overviewAdd",0,0,[this]{edit(true);});
command("Kurulumu düzenle","overviewEdit",0,1,[this]{edit(false);});
command("Yeni ölçüm kurulumu","overviewAddSetup",1,0,[this]{edit(false,true);});
command("Cihazları keşfet","overviewDiscover",1,1,[this]{if(onCameraManager){onCameraManager();return;}auto* c=context_();if(!c)throw std::runtime_error("Önce projeyi kaydedin.");c->discover();refresh();});
cards_=new QWidget;grid_=new QGridLayout(cards_);grid_->setContentsMargins(0,0,0,0);layout->addWidget(cards_);
auto* selected=new QGroupBox("Seçili kamera");auto* v=new QVBoxLayout(selected);layout->addWidget(selected);
detail_=label("");detail_->setObjectName("overviewDetail");detail_->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);v->addWidget(detail_);
connect(button(v,"Kalibre Et…","overviewCalibrate"),&QPushButton::clicked,this,[this]{attempt([this]{chooseMethod();});});
auto* experimental=new QCheckBox("Deneysel OECS kalibrasyonu • bu oturum için onayla");experimental->setObjectName("overviewExperimental");experimental->setToolTip("Tetik eşleşmesi ve fiziksel mm kabulü doğrulanmadı. PLC / robot çıkışı kapalıdır. Restart onayı sıfırlar.");v->addWidget(experimental);
connect(experimental,&QCheckBox::toggled,this,[this,experimental](bool enabled){attempt([&]{auto* c=context_();if(!c)throw std::runtime_error("Önce projeyi kaydedin.");c->calibrations().setExperimental(enabled);experimental_=enabled;refresh();});if(experimental_!=enabled){QSignalBlocker b(experimental);experimental->setChecked(experimental_);}});
auto* advanced=new QGroupBox("Ayrıntılar • geçmiş / dosya denemesi / kurulum");advanced->setObjectName("overviewDetails");advanced->setCheckable(true);advanced->setChecked(false);v->addWidget(advanced);auto* advancedLayout=new QVBoxLayout(advanced);auto* fields=new QWidget;advancedLayout->addWidget(fields);fields->hide();connect(advanced,&QGroupBox::toggled,fields,&QWidget::setVisible);auto* dv=new QVBoxLayout(fields);dv->addLayout(actions);
setups_=new QComboBox;setups_->setObjectName("overviewSetups");setups_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);setups_->setMinimumContentsLength(12);dv->addWidget(setups_);
profiles_=new QComboBox;profiles_->setObjectName("overviewProfiles");profiles_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);profiles_->setMinimumContentsLength(12);profiles_->setToolTip("Kalibrasyon edinimi ve etkinleştirme için kayıtlı acquisition profili");dv->addWidget(profiles_);
connect(setups_,&QComboBox::currentIndexChanged,this,[this]{if(!refreshing_)refresh();});
auto* commands=new QHBoxLayout;dv->addLayout(commands);
connect(button(commands,"Operasyonel seç","overviewOperational"),&QPushButton::clicked,this,[this]{attempt([this]{auto* c=context_();if(!c || selected_.isEmpty() || setups_->currentData().toString().isEmpty())throw std::runtime_error("Kamera ve kurulum seçin.");c->calibrations().setOperational({selected_},{setups_->currentData().toString()});if(onChanged)onChanged();refresh();});});
connect(button(commands,"Devre dışı bırak","overviewDeactivate"),&QPushButton::clicked,this,[this]{attempt([this]{auto* c=context_();if(!c || setups_->currentData().toString().isEmpty())throw std::runtime_error("Kurulum seçin.");c->calibrations().deactivate({setups_->currentData().toString()});if(onChanged)onChanged();refresh();});});
auto* methods=new QGridLayout;dv->addLayout(methods);int row=0;
for(const auto& text:QStringList{"Bilinen ölçü","Kalibrasyon plakası","Dosyadan deneme başlat","Etkinleştir","Robot kalibrasyonu"}){auto* b=new QPushButton(text);b->setObjectName("overviewMethod"+QString::number(row));methods->addWidget(b,row/2,row%2);
    if(row<3)connect(b,&QPushButton::clicked,this,[this,row]{attempt([this,row]{if(row==2)startTrial();else wizard(row==1,false);});});
    else if(row==3)connect(b,&QPushButton::clicked,this,[this]{attempt([this]{activate();});});
    else {b->setEnabled(false);b->setToolTip("Robot kalibrasyonu henüz uygulanmadı.");}++row;}
connect(button(dv,"Eski kayıtları kurulumla eşle","overviewMigrate"),&QPushButton::clicked,this,[this]{attempt([this]{migrate();});});
connect(button(dv,"Denemeyi bu kurulumla ilişkilendir","overviewAssociate"),&QPushButton::clicked,this,[this]{attempt([this]{associateTrial();});});
dv->addWidget(label("Geçmiş adaylar etkin seçimden ayrıdır. Normal akışta Kalibre Et → Kaydet ve kullan kullanılır. Bağımsız fiziksel kontrol ayrıca yapılmalıdır."));
records_=new QTableWidget(0,3);records_->setObjectName("overviewRecords");records_->setHorizontalHeaderLabels({"Kayıt / sürüm","Seçim","Bağımsız kontrol"});records_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);records_->setEditTriggers(QAbstractItemView::NoEditTriggers);records_->setSelectionBehavior(QAbstractItemView::SelectRows);records_->setMinimumHeight(140);dv->addWidget(records_);
connect(button(dv,"Kamerayı arşivle","overviewArchive"),&QPushButton::clicked,this,[this]{attempt([this]{auto* c=context_();if(!c || selected_.isEmpty())throw std::runtime_error("Kamera seçin.");c->archive({selected_});selected_.clear();refresh();});});
connect(button(dv,"Kurulumu arşivle","overviewArchiveSetup"),&QPushButton::clicked,this,[this]{attempt([this]{auto* c=context_();if(!c || setups_->currentData().toString().isEmpty())throw std::runtime_error("Kurulum seçin.");c->archiveSetup({setups_->currentData().toString()});refresh();});});
connect(button(dv,"Optik / kamera konumu değişti","overviewOpticalChange"),&QPushButton::clicked,this,[this]{attempt([this]{auto* c=context_();if(!c || selected_.isEmpty())throw std::runtime_error("Kamera seçin.");c->notifyOpticalChange({selected_},c->environment().revision);if(onChanged)onChanged();refresh();status_->setText("İlgili kurulumların kullanım izni kesildi. Optik/geometriyi tamamlayıp yeniden kalibre edin.");});});
auto* transfers=new QVBoxLayout;dv->addLayout(transfers);for(const auto& action:QStringList{"export","import","copy"}){const QString text=action=="export" ? "Kalibrasyon bağımlılıklarını dışa aktar":action=="import" ? "Boş projeye kalibrasyon bağımlılıklarını al":"Kaydedilmiş projeyi bağımlılıklarıyla farklı kaydet";connect(button(transfers,text,"overviewTransfer_"+action),&QPushButton::clicked,this,[this,action]{attempt([this,action]{transfer(action);});});}
status_=label("");status_->setObjectName("overviewStatus");layout->addWidget(status_);layout->addStretch();refresh();
}
void CalibrationOverview::attempt(std::function<void()> f){status_->clear();try{f();}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));}}
void CalibrationOverview::select(const QString& id){selected_=id;setups_->clear();refresh();}
void CalibrationOverview::resizeEvent(QResizeEvent* e){QWidget::resizeEvent(e);QTimer::singleShot(0,this,[this]{reflow();});}
void CalibrationOverview::reflow(){while(auto* item=grid_->takeAt(0))delete item;int columns=std::max(1,cards_->width()/260);for(int i=0;i<cardWidgets_.size();++i)grid_->addWidget(cardWidgets_[i],i/columns,i%columns);}
void CalibrationOverview::refresh(){
if(refreshing_)return;refreshing_=true;
try{
status_->clear();for(auto* w:cardWidgets_)delete w;cardWidgets_.clear();records_->setRowCount(0);auto* c=context_();
if(lastContext_!=c){lastContext_=c;experimental_=false;auto* box=findChild<QCheckBox*>("overviewExperimental");QSignalBlocker b(box);box->setChecked(false);}
if(!c){summary_->setText("Önce bir proje açın ve kaydedin. Reçete oluşturmanız gerekmez.");detail_->clear();setups_->clear();cards_->hide();refreshing_=false;return;}
auto e=c->environment();QVector<MeasurementCamera> cameras;for(const auto& camera:e.current.cameras)if(!e.archivedCameras.contains(camera.id.value))cameras.append(camera);
bool found=false;for(const auto& camera:cameras)if(camera.id.value==selected_)found=true;if(!found)selected_=cameras.isEmpty() ? QString{}:cameras.first().id.value;
summary_->setText(cameras.isEmpty() ? "Kamera veya ölçüm kurulumu ekleyin veya dosyadan deneme başlatın.":QString("%1 kamera • yalnız açık görüntü isteği cihazı açar. Kart seçimi görüntü almaz.").arg(cameras.size()));
cards_->setVisible(cameras.size()>1);
for(const auto& camera:cameras){
auto* card=new QPushButton;card->setObjectName("overviewCamera_"+camera.id.value);card->setCheckable(true);card->setChecked(camera.id.value==selected_);card->setMinimumHeight(108);card->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
const auto defaults=e.defaults(camera.id);QString setupName=defaults.status==CameraDefaultsStatus::Pending ? "Kurulum bilgisi bekleniyor":"Kurulum yok",choice="Kalibrasyon yok";
for(const auto& s:e.current.setups)if(defaults.setup && s.id==*defaults.setup){if(defaults.status!=CameraDefaultsStatus::Pending)setupName=s.name;const auto state=c->calibrations().status({s.camera,s.id,s.revision});choice=state.state==CalibrationAvailability::None ? "Kalibrasyon yok":state.state==CalibrationAvailability::Remembered ? "Kayıtlı / bağlantı bekleniyor":state.state==CalibrationAvailability::Valid ? "Geçerli":"Yeniden kalibrasyon gerekli";if(state.experimental)choice="Deneysel • "+choice;}
QString availability=c->availability(camera);
const auto full=camera.name+"\n"+setupName+"\n"+choice+"\n"+availability;card->setToolTip(full);card->setText((e.registrations.contains(camera.id.value) ? e.registrations[camera.id.value].shortId+" • ":"")+camera.name.left(32)+"\n"+setupName.left(32)+"\n"+choice+"\n"+availability.left(36));grid_->addWidget(card);cardWidgets_.append(card);connect(card,&QPushButton::clicked,this,[this,camera]{select(camera.id.value);});}
reflow();const auto previous=setups_->currentData().toString();setups_->clear();
for(const auto& s:e.current.setups)if(s.camera.value==selected_ && !e.archivedSetups.contains(s.id.value))setups_->addItem(s.name+QString(" • r%1").arg(s.revision),s.id.value);
int index=setups_->findData(previous);if(index<0 && !selected_.isEmpty()){const auto defaults=e.defaults({selected_});if(defaults.setup)index=setups_->findData(defaults.setup->value);}if(index>=0)setups_->setCurrentIndex(index);
const auto priorProfile=profiles_->currentData();profiles_->clear();bool industrial=false;
for(const auto& camera:cameras)if(camera.id.value==selected_)industrial=camera.physical.has_value();
findChild<QCheckBox*>("overviewExperimental")->setVisible(industrial);
for(const auto& p:e.current.profiles)if(p.camera.value==selected_ && p.setup.value==setups_->currentData().toString())profiles_->addItem(p.name+QString(" • r%1").arg(p.revision),p.id.value);
const auto defaults=selected_.isEmpty() ? CameraDefaults{}:e.defaults({selected_});profiles_->setCurrentIndex(profiles_->findData(defaults.profile ? QVariant(defaults.profile->value):priorProfile));if(profiles_->currentIndex()<0 && profiles_->count()==1)profiles_->setCurrentIndex(0);
profiles_->setPlaceholderText("Edinim profili seçin • Camera Manager'da düzenlenir");profiles_->setVisible(industrial);
detail_->clear();for(const auto& camera:cameras)if(camera.id.value==selected_){
    const auto identity=camera.physical ? camera.physical->vendor+" / "+camera.physical->model+" / Seri: "+camera.physical->serial : camera.device.providerId+" • "+camera.device.adapter;
    detail_->setText((e.registrations.contains(camera.id.value) ? e.registrations[camera.id.value].shortId+" • ":"")+camera.name+"\n"+c->availability(camera)+"\nCihaz: "+identity);
}
for(const auto& s:e.current.setups)if(s.id.value==setups_->currentData().toString()){
detail_->setText(detail_->text()+QString("\n%1 • %2 • %3×%4 px • %5").arg(s.opticalProfile,s.workingPlane).arg(s.geometry.resolution.width()).arg(s.geometry.resolution.height()).arg(s.pending ? "Kurulum bilgisi bekleniyor":"Kayıtlı kurulum"));
const auto state=c->calibrations().status({s.camera,s.id,s.revision});
QString text=state.state==CalibrationAvailability::None ? "Kalibrasyon yok":state.state==CalibrationAvailability::Remembered ? "Kayıtlı / bağlantı ve uyum denetimi bekleniyor":state.state==CalibrationAvailability::Valid ? "Geçerli":"Yeniden kalibrasyon gerekli";
if(state.experimental)text="Deneysel • "+text;
if(state.remembered && state.mathematicsCompatible)for(const auto& r:c->calibrations().records({s.camera,s.id,s.revision}))if(r.contentId()==state.remembered->contentId){text+=QString("\n")+(r.model().mapping() ? "Kalibrasyon plakası":"Bilinen ölçü")+" • "+r.recordedAt().toLocalTime().toString("dd.MM.yyyy HH:mm");if(state.sessionPermitted)text+=r.model().mapping() ? QString(" • plaka uyumu RMS %1 mm").arg(r.model().mapping()->fitRmsMillimetres(),0,'g',8):QString(" • %1 mm/px").arg(r.model().millimetresPerPixel(),0,'g',10);else text+=" • kayıtlı model, etkin değer değil";text+=r.controlAccepted() ? (*r.controlAccepted() ? "\nBağımsız kontrol sınır içinde":"\nBağımsız kontrol reddedildi"):"\nBağımsız fiziksel kontrol yapılmadı";}
detail_->setText(detail_->text()+"\n"+text);
try{for(const auto& r:c->calibrations().historyRecords(s.id)){int row=records_->rowCount();records_->insertRow(row);const bool old=r.setup().id!=s.id.value+"/"+QString::number(s.revision);QString choice=old ? "Eski kurulum revision • kullanılamaz":"Seçili değil";for(const auto& selection:e.selections)if(selection.contentId==r.contentId() && selection.target.setup==s.id)choice=old ? "Son seçim • eski kurulum revision":"Son seçim • uyum denetimi bekliyor";if(!old)try{const auto active=c->calibrations().snapshots({{s.camera,s.id,s.revision}});if(active.first().calibration.record.contentId()==r.contentId())choice="Seçili • kullanılabilir";}catch(const std::exception&){};
QString control=r.controlAccepted() ? (*r.controlAccepted() ? "Sınır içinde":"Reddedildi") : r.residualsMillimetres().isEmpty() ? "Yapılmadı":"Sonuç mevcut";
QStringList cells{r.name()+QString(" • v%1").arg(r.model().dependency().version),choice,control};for(int col=0;col<3;++col){auto* item=new QTableWidgetItem(cells[col]);item->setToolTip(cells[col]);if(col==0)item->setData(Qt::UserRole,r.contentId());records_->setItem(row,col,item);}}}catch(const std::exception& ex){status_->setText(QString::fromUtf8(ex.what()));}
}
}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));detail_->clear();records_->setRowCount(0);}
detail_->setToolTip(detail_->text());refreshing_=false;
}
void CalibrationOverview::configureCapture(IndependentCaptureRequest& request) const {
    if(!request.camera || !request.camera->physical)return;
    const auto e=context_()->environment();
    for(const auto& p:e.current.profiles)if(p.id.value==profiles_->currentData().toString() && p.camera==request.camera->id && request.setup && p.setup==request.setup->id)request.profile=p;
    if(!localSettings_.isEmpty())request.localRoute=CameraLocalConfiguration(localSettings_).route(*request.camera->physical);
}
void CalibrationOverview::activate(){
    auto* c=context_();if(!c || records_->currentRow()<0)throw std::runtime_error("Bir kayıt seçin.");const auto e=c->environment();std::optional<MeasurementCamera> camera;std::optional<MeasurementSetupDescriptor> setup;
    for(const auto& v:e.current.cameras)if(v.id.value==selected_)camera=v;for(const auto& v:e.current.setups)if(v.id.value==setups_->currentData().toString())setup=v;
    if(!camera || !setup)throw std::runtime_error("Hedef kamera ve kurulum seçin.");const SetupReference ref{camera->id,setup->id,setup->revision};const auto content=records_->item(records_->currentRow(),0)->data(Qt::UserRole).toString();const auto candidate=c->calibrations().candidate(ref,content);
    QString old="Seçim yok";for(const auto& selection:e.selections)if(selection.target.setup==ref.setup)old=selection.id+QString(" v%1").arg(selection.version);
    QStringList report{camera->name+" • "+setup->name+QString(" r%1").arg(setup->revision),"Mevcut seçim: "+old,"Yeni kayıt: "+candidate.record.name()+QString(" v%1").arg(candidate.selection.version),"Köken: "+candidate.record.image().origin,candidate.record.residualsMillimetres().isEmpty() ? "Bağımsız kontrol yapılmadı":"Bağımsız kontrol kanıtı mevcut"};
    QStringList blockers;if(!candidate.record.activatable())blockers<<"Deneme / reddedilmiş kontrol etkinleştirilemez.";if(e.operational.value(camera->id.value)!=setup->id.value)blockers<<"Önce hedef kurulumu operasyonel seçin.";if(!c->changeAllowed())blockers<<"Runtime/worker/pending kayıt/ACK meşgul.";
    IndependentCaptureRequest request;request.origin=SourceOrigin::CameraAcquired;request.camera=camera;request.setup=setup;configureCapture(request);try{request.validate();}catch(const std::exception& ex){blockers<<QString::fromUtf8(ex.what());}
    if(request.profile)report<<"Edinim profili: "+request.profile->name+QString(" r%1 • %2 µs • gain %3").arg(request.profile->revision).arg(request.profile->requested.exposureTimeUs).arg(request.profile->requested.gain);
    int affected=0;if(recipesProvider_)for(const auto& recipe:recipesProvider_())try{const auto target=SystemCalibration::setupReference(recipe);if(target && target->setup==ref.setup && target->camera==ref.camera){++affected;report<<SystemCalibration::impact(candidate,recipe,c->directory());}}catch(const std::exception& ex){report<<recipe.name+": "+QString::fromUtf8(ex.what());}
    if(!affected)report<<"Etkilenen reçete yok. Reçete olmadan kaynak uyumu denetlenebilir.";report<<"Kaydetme seçim değildir. Resim çek yalnız açık komutla başlar; yeni gerçek kare bu exact hedef için denetlenir.";report<<blockers;
    QDialog d(this);d.setObjectName("scopedActivationReview");d.setWindowTitle("Etkinleştirme incelemesi");auto* layout=new QVBoxLayout(&d);auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);auto* text=label(report.join('\n'));text->setObjectName("activationReviewText");text->setAlignment(Qt::AlignTop|Qt::AlignLeft);scroll->setWidget(text);layout->addWidget(scroll);
    auto* error=label("");error->setObjectName("activationStatus");layout->addWidget(error);auto* action=button(layout,"Resim çek, uyumu denetle ve etkinleştir","activationAcquire");action->setEnabled(blockers.isEmpty());auto* close=button(layout,"Kapat","activationClose");connect(close,&QPushButton::clicked,&d,&QDialog::reject);
    JobSession jobs;IndependentCalibrationSession probe;connect(&d,&QDialog::finished,&d,[&]{probe.cancel();});
    connect(action,&QPushButton::clicked,&d,[&]{try{if(jobs.busy())return;const auto job=probe.begin(request);const auto backend=backend_;auto* watcher=jobs.watch<IndependentCaptureResult>(&d,SessionJob::Image);action->setEnabled(false);error->setText("Gerçek kaynak karesi bekleniyor…");connect(watcher,&QFutureWatcher<IndependentCaptureResult>::finished,&d,[&,watcher]{const auto r=watcher->result();watcher->deleteLater();action->setEnabled(true);try{if(!probe.accept(r)){if(camera->physical){c->calibrations().disconnect(ref.camera);if(onChanged)onChanged();}throw std::runtime_error(r.error().isEmpty() ? "Edinim iptal edildi.":r.error().toUtf8().constData());}c->calibrations().activate(ref,content,probe);if(onChanged)onChanged();d.accept();}catch(const std::exception& ex){error->setText(QString::fromUtf8(ex.what()));}});jobs.launch(watcher,[job,backend]{return IndependentCalibrationSession::run(job,backend);});}catch(const std::exception& ex){error->setText(QString::fromUtf8(ex.what()));}});d.resize(680,540);d.exec();refresh();
}
void CalibrationOverview::migrate(){
    auto* c=context_();if(!c)throw std::runtime_error("Önce projeyi kaydedin.");const auto e=c->environment();std::optional<SetupReference> ref;for(const auto& s:e.current.setups)if(s.id.value==setups_->currentData().toString())ref=SetupReference{s.camera,s.id,s.revision};if(!ref)throw std::runtime_error("Hedef kurulum seçin.");const auto histories=c->legacyHistories();if(histories.isEmpty())throw std::runtime_error("Aktarılabilir eski kayıt yok.");
    QDialog d(this);d.setObjectName("measurementSetupMigration");d.setWindowTitle("Eski geçmişi açıkça kurulumla eşle");auto* layout=new QVBoxLayout(&d);layout->addWidget(label("Bütün sürümler özgün model/kanıtlarıyla korunur. Eşleme ayrı adaylar oluşturur; eski global seçim ve proje dosyası değişmez. Eksik raster kanıtı üretilmez. Reçete politikası kaynak properties’de açık bağlamayla değişir."));auto* history=new QComboBox;history->setObjectName("migrationHistory");for(auto it=histories.cbegin();it!=histories.cend();++it)history->addItem(it.key()+QString(" • %1 sürüm").arg(it.value().size()),it.key());layout->addWidget(history);auto* declaration=new QCheckBox("Seçili fiziksel kurulumla eşlemeyi beyan ediyorum");declaration->setObjectName("migrationDeclaration");layout->addWidget(declaration);auto* error=label("");error->setObjectName("migrationStatus");layout->addWidget(error);auto* action=button(layout,"Bütün sürümleri ayrı adaylar olarak aktar","migrationApply");action->setEnabled(false);connect(declaration,&QCheckBox::toggled,action,&QWidget::setEnabled);connect(action,&QPushButton::clicked,&d,[&]{try{if(!declaration->isChecked())throw std::runtime_error("Açık kurulum beyanı gerekli.");c->calibrations().migrateLegacy(*ref,histories.value(history->currentData().toString()));d.accept();}catch(const std::exception& ex){error->setText(QString::fromUtf8(ex.what()));}});connect(button(layout,"İptal","migrationCancel"),&QPushButton::clicked,&d,&QDialog::reject);d.resize(640,450);d.exec();refresh();
}
void CalibrationOverview::transfer(const QString& action){
    auto* c=context_();if(!c)throw std::runtime_error("Önce projeyi kaydedin.");if(!c->changeAllowed())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul.");QString path;
    if(action=="copy")path=QFileDialog::getSaveFileName(this,"Kaydedilmiş projeyi bağımlılıklarıyla farklı kaydet",c->directory(),"VisionFlow project (*.vfproject)");
    else if(action=="export")path=QFileDialog::getSaveFileName(this,"Kalibrasyon bağımlılıkları (proje/üretim/görüntüler ayrı)",{},"Measurement bundle (*.vfmeasurement)");
    else path=QFileDialog::getOpenFileName(this,"Boş projeye kalibrasyon bağımlılıklarını al",{},"Measurement bundle (*.vfmeasurement)");
    if(!path.isEmpty())transferFile(action,path);
}
void CalibrationOverview::transferFile(const QString& action,const QString& path){
    if(!QStringList{"copy","export","import"}.contains(action) || path.isEmpty())throw std::runtime_error("Geçersiz taşıma komutu / dosya.");
    auto* c=context_();if(!c)throw std::runtime_error("Önce projeyi kaydedin.");if(!c->changeAllowed())throw std::runtime_error("Runtime/worker/pending kayıt/ACK meşgul.");
    if(action=="copy"){const auto source=c->projectPath(),project=c->projectId();TransferProgressDialog progress(this);progress.setObjectName("measurementProjectTransferProgress");progress.setWindowTitle("Proje bağımlılıkları kopyalanıyor");progress.setWindowFlag(Qt::WindowCloseButtonHint,false);auto* layout=new QVBoxLayout(&progress);layout->addWidget(label("Son kaydedilmiş proje, kalibrasyon bağımlılıkları ve üretim geçmişi denetleniyor. İşlem bitene kadar bekleyin."));auto* bar=new QProgressBar;bar->setRange(0,0);layout->addWidget(bar);JobSession jobs;QString failure;auto* watcher=jobs.watch<QString>(&progress,SessionJob::Teaching);connect(watcher,&QFutureWatcher<QString>::finished,&progress,[&]{failure=watcher->result();watcher->deleteLater();progress.accept();});jobs.launch(watcher,[source,path,project]{try{MeasurementProjectTransfer::copy(source,path,project);return QString{};}catch(const std::exception& ex){return QString::fromUtf8(ex.what());}});progress.exec();if(!failure.isEmpty())throw std::runtime_error(failure.toUtf8().constData());status_->setText("Kaydedilmiş proje, kalibrasyon bağımlılıkları ve üretim geçmişi kopyalandı. Yeni projede kullanım izni yeniden verilmelidir. Görüntüler gömülmez; hedef varlıkları doğrulandı.");return;}

    if(action=="export"){const auto bytes=QJsonDocument(c->exportDependencies()).toJson();if(bytes.size()>12*1024*1024)throw std::runtime_error("Bağımlılık paketi boyut sınırı aşıldı.");QSaveFile file(path);file.setDirectWriteFallback(false);if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())throw std::runtime_error("Bağımlılık paketi yazılamadı.");status_->setText("Kalibrasyon registry/katalog paketi yazıldı. Proje, üretim geçmişi ve görüntüler bu pakete dahil değildir.");}
    else {QFile file(path);if(!file.open(QIODevice::ReadOnly) || file.size()>12*1024*1024)throw std::runtime_error("Bağımlılık paketi açılamadı / boyut sınırı.");QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);if(error.error!=QJsonParseError::NoError || !document.isObject())throw std::runtime_error("Geçersiz bağımlılık paketi.");c->importDependencies(document.object());refresh();status_->setText("Bağımlılıklar alındı; seçim hatırlanır, kullanım izni verilmez.");}
}
void CalibrationOverview::startTrial(){QDialog d(this);d.setObjectName("trialMethodChoice");d.setWindowTitle("Dosyadan deneme yöntemi");auto* layout=new QVBoxLayout(&d);layout->addWidget(label("Hedef kamera olmadan deneme kaydı oluşturulur. Daha sonra özgün dosyayla ayrı ilişkilendirme yapılabilir."));bool plate=false;connect(button(layout,"Bilinen ölçü","trialKnown"),&QPushButton::clicked,&d,&QDialog::accept);connect(button(layout,"Kalibrasyon plakası","trialPlate"),&QPushButton::clicked,&d,[&]{plate=true;d.accept();});connect(button(layout,"İptal","trialCancel"),&QPushButton::clicked,&d,&QDialog::reject);if(d.exec()==QDialog::Accepted)wizard(plate,true);}
void CalibrationOverview::chooseMethod(){
    QDialog choice(this);choice.setObjectName("calibrationMethodChoice");choice.setWindowTitle("Kalibre Et");auto* layout=new QVBoxLayout(&choice);layout->addWidget(label("Seçili kamera için yöntem seçin. Kamera, kurulum ve profil sihirbaz açılırken sabitlenir."));
    int method=-1;connect(button(layout,"Bilinen ölçü","calibrateKnown"),&QPushButton::clicked,&choice,[&]{method=0;choice.accept();});connect(button(layout,"Kalibrasyon plakası","calibratePlate"),&QPushButton::clicked,&choice,[&]{method=1;choice.accept();});auto* robot=button(layout,"Robot kalibrasyonu • Henüz desteklenmiyor","calibrateRobot");robot->setEnabled(false);robot->setToolTip("Hand-eye bu akışta desteklenmiyor.");connect(button(layout,"İptal","calibrateCancel"),&QPushButton::clicked,&choice,&QDialog::reject);
    if(choice.exec()==QDialog::Accepted && method>=0)wizard(method==1,false);
}
void CalibrationOverview::wizard(bool plate,bool trial){
    auto* c=context_();if(!c)throw std::runtime_error("Önce projeyi kaydedin.");const auto e=c->environment();IndependentCaptureRequest request;
    if(!trial && !selected_.isEmpty()){
        const auto defaults=e.defaults({selected_});if(defaults.status!=CameraDefaultsStatus::Ready)throw std::runtime_error("Kamera kurulum bilgisi eksik veya varsayılanı belirsiz. Ayrıntılar / Kameralar ekranında tamamlayın.");
        bool physical=false;for(const auto& camera:e.current.cameras)if(camera.id.value==selected_)physical=camera.physical.has_value();
        request=IndependentCaptureRequest::fromDefaults(e,{selected_},experimental_ && physical ? CalibrationCapturePolicy::ExperimentalOecs:CalibrationCapturePolicy::Trusted);
        if(request.camera->physical && !localSettings_.isEmpty())request.localRoute=CameraLocalConfiguration(localSettings_).route(*request.camera->physical);
    }
    const auto cameraId=selected_;const bool policy=experimental_;const auto target=request.setup ? std::optional(SetupReference{request.camera->id,request.setup->id,request.setup->revision}):std::nullopt;
    CalibrationWizard dialog(plate,request,c->directory(),[this,c,request](const auto& r){if(context_()!=c)throw std::runtime_error("Proje değişti.");if(r.image().origin=="trial_file")c->saveTrial(r);else c->calibrations().save({request.camera->id,request.setup->id,request.setup->revision},r);},this,backend_);
    const auto current=[this,c,cameraId,policy]{return context_()==c && selected_==cameraId && experimental_==policy;};
    if(target)dialog.setSaveAndUse([this,c,target,revision=e.revision,request,current](const auto& record,const auto& probe){
        if(!current()){CalibrationSaveUseResult result;result.error="Kamera / proje / oturum değişti.";return result;}
        const auto evidence=probe.evidence();auto result=c->saveAndUseCalibration({*target,revision,evidence ? evidence->generation:0,request.policy},record,probe,current);if(result.state==CalibrationSaveUseState::Activated && onChanged)onChanged();return result;
    });
    QTimer lifetime;connect(&lifetime,&QTimer::timeout,&dialog,[&]{if(!current())static_cast<QDialog&>(dialog).reject();});lifetime.start(50);dialog.exec();refresh();
}
void CalibrationOverview::associateTrial(){
    auto* c=context_();if(!c)throw std::runtime_error("Önce projeyi kaydedin.");auto e=c->environment();std::optional<SetupReference> target;
    for(const auto& s:e.current.setups)if(s.id.value==setups_->currentData().toString())target=SetupReference{s.camera,s.id,s.revision};
    if(!target)throw std::runtime_error("Hedef kurulum seçin.");const auto trials=c->trials();if(trials.isEmpty())throw std::runtime_error("Kaydedilmiş deneme yok.");
    QDialog d(this);d.setWindowTitle("Denemeyi ilişkilendir • yeni türetim");auto* layout=new QVBoxLayout(&d);layout->addWidget(label("Özgün dosya yeniden açılır ve piksel hash'i doğrulanır. Eski deneme değişmez; yeni kayıt etkinleştirilmez."));
    auto* choices=new QComboBox;for(const auto& r:trials)choices->addItem(r.name());layout->addWidget(choices);
    auto* action=button(layout,"Özgün dosyayı seç ve ilişkilendir","associateFile");auto* error=label("");layout->addWidget(error);JobSession jobs;struct AssociationResult{std::optional<CalibrationModelRecord> record;QString error;};
connect(action,&QPushButton::clicked,&d,[&]{if(jobs.busy())return;const auto path=QFileDialog::getOpenFileName(&d,"Özgün deneme görüntüsü",{},"Görüntüler (*.png *.jpg *.jpeg *.bmp)");if(path.isEmpty())return;try{const auto work=c->associationJob(trials.at(choices->currentIndex()),*target,path);auto* watcher=jobs.watch<AssociationResult>(&d,SessionJob::Teaching);action->setEnabled(false);choices->setEnabled(false);error->setText("Özgün raster yeniden denetleniyor…");connect(watcher,&QFutureWatcher<AssociationResult>::finished,&d,[&,watcher]{const auto r=watcher->result();watcher->deleteLater();action->setEnabled(true);choices->setEnabled(true);try{if(!r.record)throw std::runtime_error(r.error.toUtf8().constData());c->saveAssociation(*target,*r.record);d.accept();}catch(const std::exception& e){error->setText(QString::fromUtf8(e.what()));}});jobs.launch(watcher,[work]{try{return AssociationResult{work(),{}};}catch(const std::exception& e){return AssociationResult{{},QString::fromUtf8(e.what())};}});}catch(const std::exception& e){error->setText(QString::fromUtf8(e.what()));}});d.exec();refresh();
}
void CalibrationOverview::edit(bool addCamera,bool addSetup){
auto* c=context_();if(!c)throw std::runtime_error("Önce proje açın ve kaydedin.");auto e=c->environment();auto contract=e.current;
MeasurementCamera camera;MeasurementSetupDescriptor setup;bool cameraFound=false,setupFound=false;
for(const auto& v:contract.cameras)if(v.id.value==selected_){camera=v;cameraFound=true;}
for(const auto& v:contract.setups)if(v.id.value==setups_->currentData().toString()){setup=v;setupFound=true;}
if(!addCamera && !cameraFound)throw std::runtime_error("Önce kamera seçin.");if(!addCamera && !addSetup && !setupFound)throw std::runtime_error("Önce ölçüm kurulumu seçin.");
if(!addCamera && (camera.device.adapter!="webcam" || (setupFound && (setup.geometry.sensorRoi || setup.geometry.binning))))throw std::runtime_error("Bu adapter / sensör geometrisi için düzenleme henüz desteklenmiyor.");
QDialog dialog(this);dialog.setWindowTitle(addCamera ? "Kamera ve ölçüm kurulumu ekle":"Ölçüm kurulumu");auto* layout=new QVBoxLayout(&dialog);auto* form=new QFormLayout;form->setRowWrapPolicy(QFormLayout::WrapLongRows);layout->addLayout(form);
auto field=[&](QString title,QString value,QString name){auto* f=new QLineEdit(value);f->setMaxLength(128);f->setObjectName(name);form->addRow(title,f);return f;};
auto* name=field("Kamera adı",addCamera ? "":camera.name,"overviewCameraName");
auto* devices=new QComboBox;devices->setObjectName("overviewDevice");devices->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);devices->setMinimumContentsLength(15);
for(const auto& d:c->discovered())devices->addItem(d.adapter+" • "+d.providerId,d.providerId);form->addRow("Keşfedilmiş cihaz",devices);devices->setEnabled(addCamera);
if(!addCamera){devices->setPlaceholderText("Kayıtlı cihaz: "+camera.device.providerId);devices->setCurrentIndex(devices->findData(camera.device.providerId));}
auto* remap=new QCheckBox("Kamerayı seçili keşfedilmiş cihaza açıkça yeniden eşle");remap->setObjectName("overviewRemap");remap->setVisible(!addCamera);layout->addWidget(remap);connect(remap,&QCheckBox::toggled,devices,&QWidget::setEnabled);
if(addCamera)layout->addWidget(label("Önce Sistem özeti ekranından cihazları keşfedin. Yalnız keşfedilen provider kimliği kaydedilir; seri numarası üretilmez."));
auto* setupName=field("Kurulum adı",addCamera || addSetup ? "":setup.name,"overviewSetupName");auto* lens=field("Optik profil",addCamera || addSetup ? "":setup.opticalProfile,"overviewLens");auto* plane=field("Çalışma düzlemi",addCamera || addSetup ? "":setup.workingPlane,"overviewPlane");
auto* width=new QSpinBox;auto* height=new QSpinBox;for(auto* spin:{width,height})spin->setRange(1,32768);width->setObjectName("overviewWidth");height->setObjectName("overviewHeight");width->setValue(setupFound ? setup.geometry.resolution.width():640);height->setValue(setupFound ? setup.geometry.resolution.height():480);form->addRow("Görüntü genişliği px",width);form->addRow("Görüntü yüksekliği px",height);
width->setEnabled(addCamera ? true:camera.device.capabilities.resolution);height->setEnabled(width->isEnabled());
auto* known=new QCheckBox("Kritik görüntü geometrisi tam olarak beyan edildi");known->setObjectName("overviewGeometryKnown");known->setChecked(setupFound && setup.geometry.complete);layout->addWidget(known);
layout->addWidget(label("Sensör ROI/binning bu akışta desteklenmiyor. Optik/düzlem/geometri değişimi yeni revision üretir. Kaydet cihaz ayarı uygulamaz."));
auto* error=label("");layout->addWidget(error);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
const auto originalCamera=camera;
connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{auto camera=originalCamera;
if(addCamera){if(devices->currentIndex()<0)throw std::runtime_error("Keşfedilmiş bir cihaz seçin.");camera={{id()},name->text().trimmed(),c->discovered().at(devices->currentIndex())};if(!camera.device.capabilities.resolution)throw std::runtime_error("Adapter çözünürlük beyanını desteklemiyor.");for(const auto& other:contract.cameras)if(other.device.adapter==camera.device.adapter && other.device.providerId==camera.device.providerId)throw std::runtime_error("Cihaz zaten kayıtlı; mevcut kameraya kurulum ekleyin.");}
else camera.name=name->text().trimmed();
if(!addCamera && remap->isChecked()){if(devices->currentIndex()<0)throw std::runtime_error("Yeniden eşleme için keşfedilmiş cihaz seçin.");camera.device=camera.device.rebound(c->discovered().at(devices->currentIndex()));}
MeasurementSetupDescriptor next{{addCamera || addSetup ? id():setup.id.value},camera.id,setupName->text().trimmed(),lens->text().trimmed(),plane->text().trimmed(),1,camera.device.revision,{{width->value(),height->value()},{},{},known->isChecked()}};
if(!addCamera && !addSetup)next=setup.revised(next);
auto candidate=contract;
if(addCamera)candidate.cameras.append(camera);else for(auto& v:candidate.cameras)if(v.id==camera.id)v=camera;
if(addCamera || addSetup)candidate.setups.append(next);else for(auto& v:candidate.setups)if(v.id==next.id)v=next;
for(auto& v:candidate.setups)if(v.camera==camera.id && v.deviceRevision!=camera.device.revision){auto changed=v;changed.deviceRevision=camera.device.revision;v=v.revised(changed);}
c->replace(candidate,e.revision);selected_=camera.id.value;dialog.accept();refresh();
}catch(const std::exception& ex){error->setText(QString::fromUtf8(ex.what()));}});dialog.resize(480,580);dialog.exec();
}
}
