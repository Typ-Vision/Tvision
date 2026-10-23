#include "CalibrationWizard.hpp"
#include "ImageViewer.hpp"
#include "domain/EdgePointConfig.hpp"
#include "application/CalibrationTargetLibrary.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QScrollArea>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QScreen>
#include <QSignalBlocker>
#include <QDir>
#include <QUuid>
#include <QTimer>
#include <stdexcept>
namespace visionflow {
namespace {
QLabel* label(QString text){auto* w=new QLabel(text);w->setWordWrap(true);w->setTextFormat(Qt::PlainText);return w;}
QPushButton* button(QLayout* l,QString text,QString id){auto* b=new QPushButton(text);b->setObjectName(id);l->addWidget(b);return b;}
QDoubleSpinBox* number(QString id){auto* n=new QDoubleSpinBox;n->setObjectName(id);n->setDecimals(6);n->setRange(0,1000000);n->setSuffix(" mm");return n;}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
}
CalibrationWizard::CalibrationWizard(bool plate,std::optional<ToolDefinition> source,QString directory,std::function<void(const CalibrationModelRecord&)> save,QWidget* parent)
 :QDialog(parent),plateMode_(plate),directory_(directory),source_(source),save_(save){
    setObjectName("calibrationWizard");setWindowTitle(plate ? "Kalibrasyon plakası ile kalibrasyon":"Bilinen ölçü ile kalibrasyon");
    setSizeGripEnabled(true);resize(1160,780);if(screen())resize(size().boundedTo(screen()->availableGeometry().size()-QSize(30,50)));
    auto* root=new QVBoxLayout(this);heading_=label("");heading_->setObjectName("wizardHeading");root->addWidget(heading_);
    auto* body=new QHBoxLayout;viewer_=new ImageViewer;viewer_->setObjectName("wizardViewer");viewer_->setMinimumSize(180,180);body->addWidget(viewer_,3);
    auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setMinimumWidth(290);scroll->setMaximumWidth(370);pages_=new QStackedWidget;scroll->setWidget(pages_);body->addWidget(scroll,2);root->addLayout(body,1);
    auto page=[&]{auto* w=new QWidget;auto* v=new QVBoxLayout(w);pages_->addWidget(w);return v;};
    auto* image=page();auto* intro=label("Referans aynı çalışma düzleminde olmalı. Image File veya Webcam kaynağı kullanılır.");intro->setObjectName("wizardSourceNote");image->addWidget(intro);
    auto* acquire=button(image,"Reçete kaynağından al","wizardAcquire");acquire->setEnabled(source_.has_value());
    connect(acquire,&QPushButton::clicked,this,[this]{if(independentTarget_){attempt([this]{captureIndependent();});return;}attempt([this]{session_.clear();pendingFrame_.reset();setup_.reset();controls_={};captureCancel_=std::make_shared<std::atomic_bool>(false);
        const auto source=*source_;const auto directory=directory_;const auto cancel=captureCancel_;auto* watcher=jobs_.watch<ImageLoad>(this,SessionJob::Image);
        connect(watcher,&QFutureWatcher<ImageLoad>::finished,this,[this,watcher]{auto r=watcher->result();watcher->deleteLater();if(captureCancel_ && !captureCancel_->load()){if(r.frame)presentFrame(*r.frame,false);else status_->setText(r.error);}refresh();});
        jobs_.launch(watcher,[source,directory,cancel]{ImageLoad r;try{r.frame=CalibrationCapture::acquire(source,directory,cancel);}catch(const std::exception& e){r.error=QString::fromUtf8(e.what());}return r;});refresh();});});
    connect(button(image,"Dosyadan aç","wizardOpenFile"),&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getOpenFileName(this,"Kalibrasyon görüntüsü",{},"Görüntüler (*.pgm *.png *.jpg *.jpeg *.bmp)");if(!path.isEmpty())openFile(path);});
    imageInfo_=label("Görüntü yok.");imageInfo_->setObjectName("wizardImageInfo");image->addWidget(imageInfo_);
    auto* sourceDetails=new QGroupBox("Ayrıntılar • dosya denemesi / kurulum");sourceDetails->setObjectName("wizardSourceDetails");sourceDetails->setCheckable(true);sourceDetails->setChecked(false);auto* detailLayout=new QVBoxLayout(sourceDetails);auto* detailFields=new QWidget;detailLayout->addWidget(detailFields);detailFields->hide();connect(sourceDetails,&QGroupBox::toggled,detailFields,&QWidget::setVisible);
    auto* form=new QFormLayout(detailFields);usage_=new QComboBox;usage_->setObjectName("wizardUsage");usage_->addItems({"Sadece deneme","Bu kurulumdan kaydedilmiş görüntü"});form->addRow("Dosya kullanımı",usage_);
    installation_=new QLineEdit;installation_->setObjectName("wizardInstallation");installation_->setPlaceholderText("Örn. Hat 1 ölçüm istasyonu");form->addRow("Ortak kurulum adı",installation_);
    lens_=new QLineEdit;lens_->setObjectName("wizardLens");form->addRow("Lens / kamera kurulumu",lens_);
    plane_=new QLineEdit;plane_->setObjectName("wizardPlane");form->addRow("Çalışma düzlemi",plane_);form->addRow(findChild<QPushButton*>("wizardOpenFile"));image->addWidget(sourceDetails);
    auto* identityNote=label("Aynı kurulum beyanı fiziksel cihazı otomatik doğrulamaz. Deneme kayıtları etkinleştirilemez. Webcam index kalıcı cihaz kimliği değildir.");identityNote->setObjectName("wizardIdentityNote");image->addWidget(identityNote);image->addStretch();
    auto* detectPage=page();method_=new QComboBox;method_->setObjectName("wizardMethod");method_->addItems({"İki kenar bul (Kaliper)","Tek kenar bul"});detectPage->addWidget(method_);
    connect(button(detectPage,"Tarama oku çiz →","wizardDraw"),&QPushButton::clicked,this,[this]{viewer_->beginRoi(0);status_->setText("Kenarları kesecek oku görüntüde sürükleyin. Ok uçları ölçüm noktası değildir.");});
    polarity_=new QComboBox;polarity_->setObjectName("wizardPolarity");polarity_->addItems({"Her iki yön","Koyudan açığa","Açıktan koyuya"});detectPage->addWidget(polarity_);
    contrast_=number("wizardContrast");contrast_->setSuffix("");contrast_->setValue(10);detectPage->addWidget(label("En az kontrast"));detectPage->addWidget(contrast_);
    auto* detail=new QGroupBox("Ayrıntılar");detail->setObjectName("wizardAdvanced");detail->setCheckable(true);detail->setChecked(false);auto* dv=new QVBoxLayout(detail);auto* fields=new QWidget;auto* df=new QFormLayout(fields);
    sampling_=number("wizardSampling");sampling_->setSuffix(" px");sampling_->setRange(.05,16);sampling_->setValue(.5);df->addRow("Örnekleme adımı",sampling_);
    sigma_=number("wizardSigma");sigma_->setSuffix(" px");sigma_->setRange(0,16);sigma_->setValue(1);df->addRow("Gaussian sigma",sigma_);
    stripWidth_=number("wizardStripWidth");stripWidth_->setSuffix(" px");stripWidth_->setRange(1,1024);stripWidth_->setValue(1);df->addRow("Tarama şeridi",stripWidth_);
    selection_=new QComboBox;selection_->addItems({"En güçlü","İlk","Son"});df->addRow("Aday seçimi",selection_);subpixel_=new QCheckBox("Subpixel inceltme");subpixel_->setChecked(true);df->addRow(subpixel_);dv->addWidget(fields);fields->hide();connect(detail,&QGroupBox::toggled,fields,&QWidget::setVisible);detectPage->addWidget(detail);detail->setVisible(!plate);
    auto* reverse=button(detectPage,"Tarama yönünü ters çevir","wizardReverse");reverse->setVisible(!plate);connect(reverse,&QPushButton::clicked,this,[this]{attempt([this]{if(!viewer_->roi())throw std::runtime_error("Önce tarama oku çizin.");auto roi=*viewer_->roi();auto& segment=std::get<ScanSegment>(roi.geometry);std::swap(segment.start,segment.end);viewer_->setRoi(roi);lastScan_.clear();});});
    pattern_=new QComboBox;pattern_->setObjectName("wizardPattern");pattern_->addItems({"Chessboard","Simetrik daireli plaka"});detectPage->addWidget(pattern_);
    auto* grid=new QFormLayout;columns_=new QSpinBox;columns_->setObjectName("wizardColumns");columns_->setRange(3,32);columns_->setValue(9);grid->addRow("Yatay adet",columns_);rows_=new QSpinBox;rows_->setObjectName("wizardRows");rows_->setRange(3,32);rows_->setValue(6);grid->addRow("Dikey adet",rows_);spacing_=number("wizardSpacing");grid->addRow("Gerçek aralık",spacing_);detectPage->addLayout(grid);
    auto* explanation=label("Chessboard: iç köşe sayısını girin (kare sayısı = iç köşe + 1). ▦\nDaireli plaka: daire sayısı ve merkezler arası mesafe; ●──● aralık çap değildir. Eşit X/Y aralığı gerekir.");detectPage->addWidget(explanation);
    auto* targets=new QComboBox;targets->setObjectName("wizardTargets");auto* targetName=new QLineEdit;targetName->setObjectName("wizardTargetName");targetName->setMaxLength(128);targetName->setPlaceholderText("Plaka tanımı adı");detectPage->addWidget(targets);detectPage->addWidget(targetName);
    const auto targetPath=QDir(directory_).absoluteFilePath("calibration-targets.json");
    connect(targets,&QComboBox::activated,this,[this,targets,targetPath,targetName]{attempt([&]{const auto name=targets->currentText();const auto t=CalibrationTargetLibrary(targetPath).read().value(name);pattern_->setCurrentIndex(t.pattern==CalibrationGridPattern::Chessboard ? 0:1);columns_->setValue(t.columns);rows_->setValue(t.rows);spacing_->setValue(t.squareMillimetres);targetName->setText(name);});});
    auto* saveTarget=button(detectPage,"Plaka tanımını kaydet","wizardSaveTarget");connect(saveTarget,&QPushButton::clicked,this,[this,targetPath,targetName,targets]{attempt([&]{CalibrationGridTarget t{targetName->text().trimmed(),columns_->value(),rows_->value(),spacing_->value(),pattern_->currentIndex()==0 ? CalibrationGridPattern::Chessboard:CalibrationGridPattern::SymmetricCircles};CalibrationTargetLibrary(targetPath).save(targetName->text().trimmed(),t);targets->clear();targets->addItems(CalibrationTargetLibrary(targetPath).read().keys());status_->setText("Plaka tanımı kaydedildi.");});});
    targets->setVisible(plate);targetName->setVisible(plate);saveTarget->setVisible(plate);try{targets->addItems(CalibrationTargetLibrary(targetPath).read().keys());}catch(const std::exception& e){targets->setToolTip(QString::fromUtf8(e.what()));}
    auto invalidatePlate=[this]{session_.invalidatePlate();resultRecord_.reset();controls_={};viewer_->setChessboardOverlay({},0);viewer_->setValidDomain({});distance_->clear();};
    connect(pattern_,&QComboBox::currentIndexChanged,this,invalidatePlate);connect(columns_,&QSpinBox::valueChanged,this,invalidatePlate);connect(rows_,&QSpinBox::valueChanged,this,invalidatePlate);connect(spacing_,&QDoubleSpinBox::valueChanged,this,invalidatePlate);
    method_->setVisible(!plate);polarity_->setVisible(!plate);contrast_->setVisible(!plate);pattern_->setVisible(plate);columns_->setVisible(plate);rows_->setVisible(plate);spacing_->setVisible(plate);explanation->setVisible(plate);
    // Keep form labels hidden along with plate fields in the distance wizard.
    for(int i=0;i<grid->rowCount();++i)if(auto* item=grid->itemAt(i,QFormLayout::LabelRole))item->widget()->setVisible(plate);
    auto* draw=findChild<QPushButton*>("wizardDraw");draw->setVisible(!plate);
    connect(button(detectPage,plate ? "Plakayı bul":"Kenarları bul","wizardDetect"),&QPushButton::clicked,this,[this]{attempt([this]{detect();});});
    candidates_=new QComboBox;candidates_->setObjectName("wizardCandidates");detectPage->addWidget(candidates_);
    connect(candidates_,&QComboBox::activated,this,[this](int i){attempt([this,i]{session_.chooseCandidate(lastScan_,i);displayPoints();});});
    points_=new QListWidget;points_->setObjectName("wizardPoints");points_->setSelectionMode(QAbstractItemView::ExtendedSelection);detectPage->addWidget(points_);
    connect(points_,&QListWidget::itemSelectionChanged,this,[this]{displayPoints();});
    auto* pointActions=new QHBoxLayout;detectPage->addLayout(pointActions);
    connect(button(pointActions,"Sil","wizardDelete"),&QPushButton::clicked,this,[this]{attempt([this]{QStringList ids;for(auto* item:points_->selectedItems())ids.append(item->data(Qt::UserRole+1).toString());for(const auto& s:ids)session_.removeScan(s);displayPoints();});});
    connect(button(pointActions,"Yeniden bul","wizardRefind"),&QPushButton::clicked,this,[this]{attempt([this]{if(points_->selectedItems().isEmpty())throw std::runtime_error("Bir nokta seçin.");lastScan_=points_->selectedItems().first()->data(Qt::UserRole+1).toString();for(const auto& p:session_.points())if(p.scanId==lastScan_){viewer_->setRoi(p.roi);break;}detect();});});
    connect(points_,&QListWidget::itemChanged,this,[this](QListWidgetItem* item){attempt([this,item]{session_.renamePoint(item->data(Qt::UserRole).toString(),item->text());});});
    if(plate){points_->hide();candidates_->hide();for(int i=0;i<pointActions->count();++i)pointActions->itemAt(i)->widget()->hide();}
    detectPage->addStretch();
    auto* measure=page();measure->addWidget(label(plate ? "Numaralı noktaları ve ölçüm bölgesini inceleyin. Yanlış fiziksel aralık fotoğraftan doğrulanamaz.":"İki gerçek algılanmış noktayı önceki adımda seçin. Yalnız seçilen çift hesaba girer."));
    distance_=label("");distance_->setObjectName("wizardDistance");measure->addWidget(distance_);known_=number("wizardKnownMm");auto* knownLabel=label("Bu iki noktanın gerçek mesafesi [mm]");knownLabel->setVisible(!plate);measure->addWidget(knownLabel);measure->addWidget(known_);known_->setVisible(!plate);
    measure->addWidget(label("Plaka dönüşümü lens bozulmasını düzeltmez. Simetrik desenin orijini fiziksel robot referansı değildir."));measure->addStretch();
    auto* finish=page();result_=label("");result_->setObjectName("wizardResult");finish->addWidget(result_);name_=new QLineEdit;name_->setObjectName("wizardName");name_->setMaxLength(128);name_->setText((plate ? "Plaka ":"Bilinen ölçü ")+QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm"));finish->addWidget(label("Kayıt adı"));finish->addWidget(name_);
    finish->addWidget(label("İsteğe bağlı bağımsız kontrol: Geri ile yeni kenarlar bulun, iki farklı nokta seçin; kontrol referansını girin. Öğretim çifti/plaka fit noktaları kontrol sayılamaz."));
    connect(button(finish,"Yeni kontrolü iptal et","wizardCancelControl"),&QPushButton::clicked,this,[this]{pendingControl_=false;checking_=false;status_->setText("Yeni kontrol iptal edildi; son hesaplanmış model/kanıt korunur.");refresh();});
    connect(button(finish,"Başka bir mesafeyle kontrol et","wizardBeginControl"),&QPushButton::clicked,this,[this]{attempt([this]{if(!resultRecord_)throw std::runtime_error("Önce modeli hesaplayın.");checking_=true;pendingControl_=true;step_=1;method_->show();polarity_->show();contrast_->show();findChild<QPushButton*>("wizardDraw")->show();points_->show();candidates_->show();pattern_->hide();columns_->hide();rows_->hide();spacing_->hide();lastScan_.clear();refresh();});});
    auto* controlLabel=label("Kontrol çiftinin gerçek mesafesi [mm]");controlLabel->setObjectName("wizardControlLabel");finish->addWidget(controlLabel);controlKnown_=number("wizardControlMm");finish->addWidget(controlKnown_);limited_=new QCheckBox("Kontrol için hata sınırı uygula");limited_->setObjectName("wizardControlUseLimit");finish->addWidget(limited_);limit_=number("wizardControlLimit");finish->addWidget(limit_);
    connect(button(finish,"Seçili çiftle kontrol et","wizardControl"),&QPushButton::clicked,this,[this]{attempt([this]{if(!resultRecord_ || points_->selectedItems().size()!=2)throw std::runtime_error("Hesaplanmış model ve iki farklı algılanmış nokta seçin.");
        const auto selected=points_->selectedItems();QPointF a,b;for(const auto& p:session_.points()){if(p.id==selected[0]->data(Qt::UserRole).toString())a=p.pixel;if(p.id==selected[1]->data(Qt::UserRole).toString())b=p.pixel;}
        auto controls=controls_;bool replaced=false;for(auto& sample:controls.samples)if((sample.first.position==a && sample.second.position==b) || (sample.first.position==b && sample.second.position==a)){sample.knownMillimetres=controlKnown_->value();replaced=true;break;}if(!replaced)controls.samples.append({uuid(),{a,setup_->source},{b,setup_->source},controlKnown_->value()});controls.limitMillimetres=limited_->isChecked() ? std::optional(limit_->value()):std::nullopt;
        auto model=CalibrationModelRecord::create(resultRecord_->model(),*setup_,resultRecord_->image(),controls);controls_=controls;resultRecord_=model;pendingControl_=false;result_->setText(QString("Bağımsız kontrol: %1 mm fark. %2").arg(model.residualsMillimetres().last(),0,'g',10).arg(model.controlAccepted() ? (*model.controlAccepted() ? "Sınır içinde":"Sınır dışında — etkinleştirilemez"):"Kabul sınırı tanımlanmadı"));});});
    finish->addStretch();
    auto pending=[this]{if(checking_){pendingControl_=true;status_->setText("Kontrol girdisi değişti; yeniden kontrol edin.");}};
    connect(controlKnown_,&QDoubleSpinBox::valueChanged,this,pending);connect(limit_,&QDoubleSpinBox::valueChanged,this,pending);connect(limited_,&QCheckBox::toggled,this,pending);
    status_=label("");status_->setObjectName("wizardStatus");root->addWidget(status_);
    auto* actions=new QHBoxLayout;back_=button(actions,"Geri","wizardBack");connect(back_,&QPushButton::clicked,this,[this]{if(checking_ && step_==3)step_=1;else if(step_>0)--step_;refresh();});
    next_=button(actions,"Bu görüntüyü kullan","wizardNext");connect(next_,&QPushButton::clicked,this,[this]{attempt([this]{advance();});});
    connect(button(actions,"İşlemi durdur","wizardStop"),&QPushButton::clicked,this,[this]{cancelWork();status_->setText("İşlem iptal edildi; geç sonuç kullanılmaz.");refresh();});
    connect(button(actions,"İptal","wizardCancel"),&QPushButton::clicked,this,&CalibrationWizard::reject);root->addLayout(actions);
    viewer_->onRoiEdited=[this]{if(jobs_.busy())cancelWork();lastScan_.clear();};
    viewer_->onPointClicked=[this](QPointF p){if(!lastResult_ || step_!=1)return;attempt([this,p]{double best=15/viewer_->zoomFactor();int selected=-1;const auto& r=*lastResult_;for(int i=0;i<r.candidates().size();++i){const double d=QLineF(p,r.candidates()[i].first).length();if(d<best){best=d;selected=i;}}if(selected<0)return;int candidate=selected;if(!r.pairs().isEmpty()){candidate=-1;for(int i=0;i<r.pairs().size();++i)if(r.pairs()[i].first==selected || r.pairs()[i].second==selected){candidate=i;break;}}if(candidate>=0){session_.chooseCandidate(lastScan_,candidate);candidates_->setCurrentIndex(candidate);displayPoints();}});};
    for(auto* f:pages_->findChildren<QFormLayout*>())f->setRowWrapPolicy(QFormLayout::WrapAllRows);
    for(auto* combo:pages_->findChildren<QComboBox*>()){combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setMinimumContentsLength(0);combo->setMinimumWidth(0);}
    for(auto* text:pages_->findChildren<QLabel*>())text->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    pages_->setMinimumWidth(0);pages_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* timer=new QTimer(this);connect(timer,&QTimer::timeout,this,[this]{pollCamera();});timer->start(20);
    refresh();
}
CalibrationWizard::CalibrationWizard(bool plate,IndependentCaptureRequest target,QString directory,std::function<void(const CalibrationModelRecord&)> save,QWidget* parent,IndependentCaptureBackend backend)
 :CalibrationWizard(plate,std::optional<ToolDefinition>{},directory,std::move(save),parent){
    independentTarget_=std::move(target);backend_=std::move(backend);
    auto* acquire=findChild<QPushButton*>("wizardAcquire");acquire->setText("Kameradan resim çek");acquire->setEnabled(independentTarget_->camera.has_value());
    installation_->setText(independentTarget_->setup ? independentTarget_->setup->name:"Dosyadan deneme");
    lens_->setText(independentTarget_->setup ? independentTarget_->setup->opticalProfile:"trial-lens");plane_->setText(independentTarget_->setup ? independentTarget_->setup->workingPlane:"trial-plane");
    for(auto* field:{installation_,lens_,plane_})field->setReadOnly(true);
    usage_->setCurrentIndex(independentTarget_->setup ? 1:0);
    findChild<QLabel*>("wizardIdentityNote")->setText("Fiziksel mesafe veya plaka aralığını bağımsız referanstan girin. Hesaplanan model tek başına fiziksel doğruluk kabulü değildir.");
    findChild<QLabel*>("wizardSourceNote")->setText(independentTarget_->camera ? "Seçili kamera: "+independentTarget_->camera->name+"\nReferans aynı çalışma düzleminde olmalı. Hedef bu sihirbaz boyunca sabittir." : "Dosyadan bağımsız deneme. Kamera edinimi ve etkinleştirme oluşturmaz.");
    usage_->setToolTip("Kullanım seçimini dosyayı açmadan önce yapın. Yeni dosya/edinim önceki hesabı temizler.");
    if(independentTarget_->origin==SourceOrigin::CameraAcquired)attempt([this]{independent_.prepareCamera(*independentTarget_,backend_);});
}
void CalibrationWizard::setSaveAndUse(SaveUse command){saveUse_=std::move(command);refresh();}
void CalibrationWizard::showSaveResult(const CalibrationSaveUseResult& outcome){
    status_->clear();
    const auto prefix="";
    const auto control=outcome.independentControlAccepted ? (*outcome.independentControlAccepted ? "Bağımsız kontrol sınır içinde.":"Bağımsız kontrol reddedildi.") : "Bağımsız fiziksel kontrol yapılmadı.";
    if(outcome.state==CalibrationSaveUseState::Activated){savedAndUsed_=true;independent_.closeCamera();activationProbe_.closeCamera();result_->setText(QString(prefix)+"Kaydedildi ve bu oturumda kullanıma hazır.\n"+control);back_->setEnabled(false);}
    else result_->setText(QString(prefix)+(outcome.state==CalibrationSaveUseState::CandidateSaved ? "Aday kaydedildi; kullanıma alınamadı. Önceki seçim korunuyor. Yeniden deneyebilirsiniz.\n":"Kaydet ve kullan tamamlanamadı. Önceki seçim korunuyor.\n")+outcome.error+"\n"+control);
    refresh();if(savedAndUsed_)back_->setEnabled(false);
}
void CalibrationWizard::saveAndUse(const CalibrationModelRecord& record){
    if(!independentTarget_ || !independentTarget_->camera)throw std::runtime_error("Kaydet ve kullan için kayıtlı kamera gerekir.");
    const auto proof=independent_.evidence();
    if(proof && proof->origin==SourceOrigin::CameraAcquired){showSaveResult(saveUse_(record,independent_));return;}
    // A file retains its provenance. A separate live probe verifies activation.
    auto request=*independentTarget_;request.origin=SourceOrigin::CameraAcquired;request.filePath.clear();
    activationProbe_.prepareCamera(request,backend_);
    const auto job=activationProbe_.begin(request);const auto backend=backend_;
    auto* watcher=jobs_.watch<IndependentCaptureResult>(this,SessionJob::Image);
    connect(watcher,&QFutureWatcher<IndependentCaptureResult>::finished,this,[this,watcher,record]{const auto r=watcher->result();watcher->deleteLater();
        if(activationProbe_.accept(r))showSaveResult(saveUse_(record,activationProbe_));
        else if(!r.error().isEmpty())result_->setText("Canlı uyum denetimi tamamlanamadı; önceki seçim korunuyor.\n"+r.error());refresh();});
    jobs_.launch(watcher,[job,backend]{return IndependentCalibrationSession::run(job,backend);});refresh();
}
void CalibrationWizard::captureIndependent(QString path){
    const auto timing=AcquisitionTiming::request("calibration");AcquisitionTimingBinding binding(timing);AcquisitionTimingSpan commandTiming("calibration.user-command");
    if(jobs_.busy())throw std::runtime_error("Önce devam eden işi durdurun.");
    cancelWork(false);pendingFrame_.reset();setup_.reset();independentSetup_.reset();resultRecord_.reset();controls_={};lastResult_.reset();viewer_->clearImage();imageInfo_->setText("Görüntü yok.");step_=0;
    auto request=*independentTarget_;request.filePath=path;
    request.origin=path.isEmpty() ? SourceOrigin::CameraAcquired:usage_->currentIndex()==1 ? SourceOrigin::SetupFile:SourceOrigin::TrialFile;
    if(request.origin==SourceOrigin::TrialFile){request.camera.reset();request.setup.reset();request.profile.reset();request.localRoute.reset();request.policy=CalibrationCapturePolicy::Trusted;}
    if(path.isEmpty())independent_.prepareCamera(request,backend_);
    else independent_.closeCamera();
    const auto job=independent_.begin(request);const auto backend=backend_;
    auto* watcher=jobs_.watch<IndependentCaptureResult>(this,SessionJob::Image);
    connect(watcher,&QFutureWatcher<IndependentCaptureResult>::finished,this,[this,watcher,timing]{AcquisitionTimingBinding binding(timing);const auto r=watcher->result();watcher->deleteLater();
        if(independent_.accept(r)){const auto frame=*independent_.math().frame();const auto setup=*independent_.acceptedSetup();const auto evidence=*independent_.evidence();
            presentFrame(frame,evidence.origin!=SourceOrigin::CameraAcquired);independentSetup_=setup;independentOrigin_=evidence.origin==SourceOrigin::CameraAcquired ? "acquired":sourceOriginName(evidence.origin);
            imageInfo_->setText(QString("%1 • %2×%3 px\nHedef: %4").arg(independentOrigin_).arg(frame.size().width()).arg(frame.size().height()).arg(evidence.target ? evidence.target->setup.value:"Yalnız deneme"));
        }else if(!r.error().isEmpty())status_->setText(r.error());refresh();});
    jobs_.launch(watcher,[job,backend]{return IndependentCalibrationSession::run(job,backend);});refresh();
}
CalibrationWizard::~CalibrationWizard(){cancelWork();}
void CalibrationWizard::cancelWork(bool closeCamera){if(closeCamera){independent_.closeCamera();activationProbe_.closeCamera();}activationProbe_.cancel();savedAndUsed_=false;namedRecord_.reset();independent_.cancel(closeCamera);session_.cancel();if(independentTarget_){pendingFrame_.reset();setup_.reset();independentSetup_.reset();resultRecord_.reset();controls_={};step_=0;if(viewer_)viewer_->clearImage();if(imageInfo_)imageInfo_->setText("Görüntü yok.");}if(captureCancel_)captureCancel_->store(true);}
void CalibrationWizard::reject(){done(QDialog::Rejected);}
void CalibrationWizard::done(int result){
    if(pendingDone_)return;
    cancelWork();pendingDone_=result;pollCamera();
}
void CalibrationWizard::pollCamera(){
    const auto a=independent_.cameraStatus(),b=activationProbe_.cameraStatus();
    const bool draining=(a.closing && !a.drained) || (b.closing && !b.drained);
    if(pendingDone_){
        pages_->setEnabled(false);next_->setEnabled(false);back_->setEnabled(false);
        if(draining || jobs_.busy()){status_->setText("Kamera ve işlem kapanışı bekleniyor…");return;}
        const int result=*pendingDone_;pendingDone_.reset();QDialog::done(result);return;
    }
    if(draining){pages_->setEnabled(false);next_->setEnabled(false);status_->setText("Kamera kapanışı bekleniyor…");return;}
    refresh();
    if(a.preparing)status_->setText("Kamera hazırlanıyor…");
    else if(a.state==AcquisitionSessionState::Fault){
        QString error;for(const auto& source:a.sources)if(!source.error.isEmpty())error=source.error;
        status_->setText(error.isEmpty()?"Kamera bağlantısı tamamlanamadı.":error);
    }else if(status_->text()=="Kamera hazırlanıyor…" || status_->text()=="Kamera kapanışı bekleniyor…")status_->clear();
}
void CalibrationWizard::attempt(std::function<void()> f){try{status_->clear();f();}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));}refresh();}
void CalibrationWizard::openFile(const QString& path){if(independentTarget_){attempt([this,path]{captureIndependent(path);});return;}attempt([this,path]{if(jobs_.busy())throw std::runtime_error("Önce devam eden işi durdurun.");cancelWork();session_.clear();pendingFrame_.reset();setup_.reset();controls_={};resultRecord_.reset();viewer_->clearImage();captureCancel_=std::make_shared<std::atomic_bool>(false);const auto cancel=captureCancel_;auto* w=jobs_.watch<ImageLoad>(this,SessionJob::Image);connect(w,&QFutureWatcher<ImageLoad>::finished,this,[this,w,cancel]{const auto r=w->result();w->deleteLater();if(!cancel->load()){if(r.frame)presentFrame(*r.frame,true);else status_->setText(r.error);}refresh();});jobs_.launch(w,[path]{ImageLoad r;try{r.frame=ChessboardService::importImage(path).frame;}catch(const std::exception& e){r.error=QString::fromUtf8(e.what());}return r;});});}
void CalibrationWizard::presentFrame(ImageFrame f,bool imported){pendingFrame_=f;imported_=imported;checking_=false;pendingControl_=false;session_.clear();setup_.reset();controls_={};resultRecord_.reset();lastResult_.reset();step_=0;viewer_->setImage(f.preview());imageInfo_->setText(QString("%1 • %2×%3 px\n%4").arg(imported ? "Dosya kökenli":"Reçete kaynağından edinilmiş").arg(f.size().width()).arg(f.size().height()).arg(QFileInfo(f.path()).fileName()));displayPoints();refresh();}
void CalibrationWizard::refresh(){pages_->setCurrentIndex(step_);heading_->setText(QString("%1 / 4 — %2").arg(step_+1).arg(QStringList{"Görüntü","Noktaları bul",plateMode_ ? "Noktaları onayla":"Çifti ve mesafeyi onayla","Sonuç ve kayıt"}[step_]));
    const bool plate=plateMode_ && !checking_;for(auto* w:QList<QWidget*>{pattern_,columns_,rows_,spacing_,findChild<QComboBox*>("wizardTargets"),findChild<QLineEdit*>("wizardTargetName"),findChild<QPushButton*>("wizardSaveTarget")})w->setVisible(plate);
    for(auto* w:QList<QWidget*>{method_,polarity_,contrast_,points_,candidates_,findChild<QPushButton*>("wizardDraw"),findChild<QPushButton*>("wizardReverse"),findChild<QGroupBox*>("wizardAdvanced"),findChild<QPushButton*>("wizardDelete"),findChild<QPushButton*>("wizardRefind")})w->setVisible(!plate);
    for(auto* f:pages_->findChildren<QFormLayout*>())for(auto* field:QList<QWidget*>{columns_,rows_,spacing_})if(auto* text=f->labelForField(field))text->setVisible(plate);
    for(auto* field:QList<QWidget*>{controlKnown_,limited_,limit_,findChild<QLabel*>("wizardControlLabel"),findChild<QPushButton*>("wizardControl")})field->setVisible(checking_);
    findChild<QPushButton*>("wizardCancelControl")->setVisible(checking_);findChild<QPushButton*>("wizardCancelControl")->setText(pendingControl_ ? "Yeni kontrolü iptal et":"Kontrol düzenlemesini bitir");
    const auto camera=independent_.cameraStatus(),probe=activationProbe_.cameraStatus();
    const bool closing=pendingDone_.has_value() || (camera.closing && !camera.drained) || (probe.closing && !probe.drained);
    back_->setEnabled(step_>0 && !jobs_.busy() && !savedAndUsed_ && !closing);next_->setEnabled(!jobs_.busy() && !closing);next_->setText(QStringList{"Bu görüntüyü kullan",plateMode_ ? "Noktaları incele":"Seçili çifti kullan",plateMode_ ? "Onayla ve hesapla":"Hesapla",saveUse_ ? (savedAndUsed_ ? "Bitir":resultRecord_ && resultRecord_->image().origin=="trial_file" ? "Denemeyi kaydet":"Kaydet ve kullan"):"Kaydet ve bitir"}[step_]);
    pages_->setEnabled(!jobs_.busy() && !savedAndUsed_ && !closing);usage_->setEnabled(independentTarget_ ? !pendingFrame_ && independentTarget_->setup.has_value():imported_);if(jobs_.busy())status_->setText("İşlem sürüyor… Stop veya İptal kullanılabilir.");else if(status_->text()=="İşlem sürüyor… Stop veya İptal kullanılabilir.")status_->clear();}
void CalibrationWizard::displayPoints(){
    QStringList selected;for(auto* i:points_->selectedItems())selected.append(i->data(Qt::UserRole).toString());
    QSignalBlocker block(points_);points_->clear();QVector<QPointF> pixels;QVector<QLineF> lines;
    for(const auto& p:session_.points()){auto* i=new QListWidgetItem(p.name,points_);i->setFlags(i->flags()|Qt::ItemIsEditable);i->setData(Qt::UserRole,p.id);i->setData(Qt::UserRole+1,p.scanId);i->setToolTip(QString("%1 • %2 • %3, %4 px").arg(p.pairId.isEmpty() ? "Kenar":"Kaliper",p.refined ? "Subpixel inceltildi":"İnceltilmedi").arg(p.pixel.x(),0,'g',9).arg(p.pixel.y(),0,'g',9));i->setSelected(selected.contains(p.id));pixels.append(p.pixel);}
    if(selected.size()==2){QPointF a,b;for(const auto& p:session_.points()){if(p.id==selected[0])a=p.pixel;if(p.id==selected[1])b=p.pixel;}lines.append({a,b});}
    QVector<QPointF> alternatives;if(lastResult_){QSet<int> allowed;for(const auto& pair:lastResult_->pairs()){allowed.insert(pair.first);allowed.insert(pair.second);}for(int i=0;i<lastResult_->candidates().size();++i)if((lastResult_->pairs().isEmpty() || allowed.contains(i)) && !pixels.contains(lastResult_->candidates()[i].first))alternatives.append(lastResult_->candidates()[i].first);}
    if(!plateMode_ || checking_)viewer_->setToolOverlay({},pixels,lines,alternatives.isEmpty() ? "Gerçek algılanmış noktalar":"Diğer gerçek adaylar kırmızı; seçili noktalar listede",{}, {},{},alternatives);
}
void CalibrationWizard::selectPair(){const auto s=points_->selectedItems();if(s.size()==2)session_.selectPair(s[0]->data(Qt::UserRole).toString(),s[1]->data(Qt::UserRole).toString());else if(!session_.distancePixels())throw std::runtime_error("İki farklı algılanmış nokta seçin.");}
void CalibrationWizard::detect(){if(jobs_.busy())return;
    if(!checking_){controls_={};resultRecord_.reset();}status_->clear();if(plateMode_ && !checking_){CalibrationGridTarget target{uuid(),columns_->value(),rows_->value(),spacing_->value(),pattern_->currentIndex()==0 ? CalibrationGridPattern::Chessboard:CalibrationGridPattern::SymmetricCircles};auto job=session_.scanPlate(target);viewer_->setChessboardOverlay({},0);viewer_->setValidDomain({});auto* w=jobs_.watch<WizardPlateResult>(this,SessionJob::Teaching);
        connect(w,&QFutureWatcher<WizardPlateResult>::finished,this,[this,w]{const auto r=w->result();w->deleteLater();if(session_.accept(r)){status_->setText(r.error());if(session_.plate()){const auto& p=*session_.plate();viewer_->setChessboardOverlay(p.pixels,p.target.columns);distance_->setText(QString("%1 / %2 nokta bulundu. Gerçek aralığı kontrol edin.").arg(p.pixels.size()).arg(p.target.columns*p.target.rows));}}refresh();});jobs_.launch(w,[job]{return CalibrationWizardSession::run(job);});
    }else{if(!viewer_->roi())throw std::runtime_error("Önce bir tarama oku çizin.");auto roi=*viewer_->roi();std::get<ScanSegment>(roi.geometry).width=stripWidth_->value();if(lastScan_.isEmpty())lastScan_=uuid();auto parameters=edgePointDefaults();parameters["polarity"]=QStringList{"any","rising","falling"}[polarity_->currentIndex()];parameters["minContrast"]=contrast_->value();parameters["stepPx"]=sampling_->value();parameters["sigmaPx"]=sigma_->value();parameters["selection"]=QStringList{"strongest","first","last"}[selection_->currentIndex()];parameters["subpixel"]=subpixel_->isChecked();
        EdgePairOptions options;options.profile.stepPx=sampling_->value();options.profile.sigmaPx=sigma_->value();options.subpixel=subpixel_->isChecked();options.selection=selection_->currentIndex()==0 ? EdgeSelection::Strongest:selection_->currentIndex()==1 ? EdgeSelection::First:EdgeSelection::Last;options.minContrast=contrast_->value();options.polarity=polarity_->currentIndex()==0 ? StripePolarity::Any:polarity_->currentIndex()==1 ? StripePolarity::Bright:StripePolarity::Dark;
        auto job=method_->currentIndex()==0 ? session_.scanPair(lastScan_,roi,options):session_.scanEdge(lastScan_,roi,parameters);displayPoints();auto* w=jobs_.watch<WizardScanResult>(this,SessionJob::Teaching);
        connect(w,&QFutureWatcher<WizardScanResult>::finished,this,[this,w]{const auto r=w->result();w->deleteLater();if(session_.accept(r)){lastResult_=r;status_->setText(!r.error().isEmpty() ? r.error():!r.selected() ? "Kenar bulunamadı. Tarama ve kontrastı değiştirin.":"Gerçek kenarlar bulundu. Çifti inceleyin.");QSignalBlocker b(candidates_);candidates_->clear();const int count=r.pairs().isEmpty() ? r.candidates().size():r.pairs().size();for(int i=0;i<count;++i)candidates_->addItem(QString("Aday %1").arg(i+1));if(r.selected())candidates_->setCurrentIndex(*r.selected());displayPoints();if(session_.distancePixels() && points_->count()>=2){QSignalBlocker p(points_);points_->clearSelection();points_->item(points_->count()-1)->setSelected(true);points_->item(points_->count()-2)->setSelected(true);}}refresh();});jobs_.launch(w,[job]{return CalibrationWizardSession::run(job);});
    }refresh();
}
void CalibrationWizard::advance(){
    if(step_==0 && independentTarget_){if(!pendingFrame_ || !independentSetup_)throw std::runtime_error("Önce görüntü alın veya dosya açın.");
        const auto s=*independentSetup_;if(!setup_ || *setup_!=s || !session_.frame()){session_.useFrame(*pendingFrame_,s,independentOrigin_,independent_.evidence()->experimental);setup_=s;controls_={};resultRecord_.reset();checking_=false;}viewer_->setRoiFrame(s.source.id);viewer_->setRoiEditable(true);++step_;
    }else if(step_==0){if(!pendingFrame_)throw std::runtime_error("Önce görüntü alın veya dosya açın.");ToolDefinition source;
        if(source_)source=*source_;else {source.kind="image_file";source.parameters={{"path",pendingFrame_->path()}};}
        if(imported_ && usage_->currentIndex()==1 && !source_)throw std::runtime_error("Kurulum ilişkilendirmesi için hedef reçete kaynağını açın.");
        const auto s=CalibrationCapture::setup(source,directory_,*pendingFrame_,installation_->text(),lens_->text(),plane_->text());
        const auto origin=imported_ ? (usage_->currentIndex()==1 ? "setup_file":"trial_file"):"acquired";
        if(!setup_ || *setup_!=s || !session_.frame() || origin_!=origin){session_.useFrame(*pendingFrame_,s,origin);setup_=s;origin_=origin;controls_={};resultRecord_.reset();checking_=false;}
        viewer_->setRoiFrame(s.source.id);viewer_->setRoiEditable(true);++step_;
    }else if(step_==1){if(checking_){selectPair();pendingControl_=true;step_=3;}else {if(plateMode_){if(!session_.plate())throw std::runtime_error("Önce tam ve geçerli plaka bulun.");}else {selectPair();distance_->setText(QString("Seçili çift: %1 px").arg(*session_.distancePixels(),0,'g',10));}++step_;}
    }else if(step_==2){CalibrationModelRecord record=[&]{if(plateMode_){session_.confirmPlate();return session_.calculatePlate();}session_.setKnownMillimetres(known_->value());return session_.calculate();}();
        controls_={};pendingControl_=false;
        result_->setText(record.model().mapping() ? QString("Hesaplandı — plaka uyumu RMS %1 mm; ters dönüşüm RMS %2 px, en fazla %3 px. Bunlar bağımsız fiziksel hata değildir. Lens düzeltmesi uygulanmadı. Bağımsız kontrol yapılmadı.").arg(record.model().mapping()->fitRmsMillimetres(),0,'g',8).arg(record.model().mapping()->inverseRmsPixels(),0,'g',8).arg(record.model().mapping()->maxInversePixels(),0,'g',8):QString("Hesaplandı — %1 mm/px. Bağımsız kontrol yapılmadı.").arg(record.model().millimetresPerPixel(),0,'g',12));
        resultRecord_=record;if(record.model().mapping())viewer_->setValidDomain(record.model().mapping()->regionPixels());++step_;
    }else {if(savedAndUsed_){accept();return;}if(pendingControl_)throw std::runtime_error("Yeni kontrolü hesaplayın veya Yeni kontrolü iptal et kullanın.");if(!resultRecord_)throw std::runtime_error("Girdi değişti. Yeniden hesaplayın.");const auto& r=*resultRecord_;auto record=CalibrationModelRecord::create(r.model(),r.setup(),r.image(),controls_).named(name_->text().trimmed());if(saveUse_ && record.image().origin!="trial_file"){if(!namedRecord_ || namedRecord_->name()!=record.name() || namedBase_!=r.contentId()){namedRecord_=record;namedBase_=r.contentId();}saveAndUse(*namedRecord_);return;}save_(record);accept();}
    status_->clear();refresh();
}
}
