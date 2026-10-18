#include "MainWindow.hpp"
#include "CalibrationHome.hpp"
#include "CalibrationOverview.hpp"
#include "ImageViewer.hpp"
#include "application/CalibrationStudyStore.hpp"
#include "application/CalibrationReportService.hpp"
#include <QLabel>
#include <QGroupBox>
#include <QRegularExpression>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QFormLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QDir>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QLocale>
#include <stdexcept>
#include <cmath>
namespace visionflow {
namespace {
QLabel* note(const QString& text){auto* label=new QLabel(text);label->setWordWrap(true);label->setTextFormat(Qt::PlainText);label->setObjectName("muted");return label;}
struct CalibrationWork {std::optional<PlanarCalibration> value;QString error;};
// UI contract: fixed decimal input, Turkish comma, no exponent/group separator/clamp.
constexpr double manualMinimum=0.000001,manualMaximum=1000000;
QLocale manualLocale() {QLocale locale(QLocale::Turkish,QLocale::Turkey);locale.setNumberOptions(QLocale::OmitGroupSeparator|QLocale::RejectGroupSeparator);return locale;}
QString manualText(double value) {
    auto text=manualLocale().toString(value,'f',6);bool ok=false;
    if(manualLocale().toDouble(text,&ok)!=value || !ok)return manualLocale().toString(value,'g',17);
    while(text.endsWith('0'))text.chop(1);if(text.endsWith(','))text.chop(1);return text;
}
double manualInput(const QString& text) {
    if(!QRegularExpression("^[0-9]+(?:,[0-9]{1,6})?$").match(text).hasMatch())throw std::runtime_error("px/mm: ondalık virgülle pozitif sayı girin; en çok 6 ondalık, binlik ayırıcı yok.");
    bool ok=false;const double value=QLocale(QLocale::Turkish,QLocale::Turkey).toDouble(text,&ok);
    if(!ok || !std::isfinite(value) || value<manualMinimum || value>manualMaximum)throw std::runtime_error("px/mm aralığı 0,000001–1000000 olmalı; değer değiştirilmedi.");
    return value;
}
}
QWidget* MainWindow::makeCalibration() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);layout->setContentsMargins(0,0,0,0);
    auto* step=note("1. Yöntem seçin → 2. Görüntü ve kurulum → 3. Ölçek → 4. Bağımsız doğrulama → 5. Kaydet ve bağla");step->setObjectName("calibrationStep");layout->addWidget(step);
    auto* body=new QHBoxLayout;auto* imagePanel=new QWidget;auto* imageLayout=new QVBoxLayout(imagePanel);imageLayout->setContentsMargins(0,0,0,0);
    calibrationFrameInfo_=note("Görüntü yok.");calibrationFrameInfo_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);imageLayout->addWidget(calibrationFrameInfo_);
    calibrationViewer_=new ImageViewer;calibrationViewer_->setObjectName("calibrationViewer");imageLayout->addWidget(calibrationViewer_,1);
    calibrationPixelInfo_=note("Piksel: —");imageLayout->addWidget(calibrationPixelInfo_);body->addWidget(imagePanel,1);
    calibrationViewer_->onPixel=[this](QPoint p){calibrationPixelInfo_->setText(p.x()>=0 ? QString("X:%1 Y:%2 px").arg(p.x()).arg(p.y()) : "Piksel: —");};
    auto* scroll=new QScrollArea;scroll->setObjectName("calibrationScroll");scroll->setWidgetResizable(true);scroll->setMinimumWidth(300);
    calibrationControls_=new QWidget;auto* controls=new QVBoxLayout(calibrationControls_);
    auto section=[&](const QString& title){auto* g=new QGroupBox(title);auto* v=new QVBoxLayout(g);controls->addWidget(g);return v;};
    auto button=[&](QLayout* parent,const QString& text,const QString& name){auto* b=new QPushButton(text);b->setObjectName(name);parent->addWidget(b);return b;};
    auto* method=section("1. Yöntem seç");auto* mode=new QComboBox;mode->setObjectName("calibrationModel");mode->addItem("Bilinen uzunluktan ölçek","uniform_planar_scale");mode->addItem("Kalibrasyon plakası (perspektif)","raw_planar_homography");mode->addItem("Manuel ölçek (px/mm)","manual_planar_scale");mode->setCurrentIndex(2);method->addWidget(mode);
    auto* description=note("");description->setObjectName("calibrationMethodNote");method->addWidget(description);
    connect(mode,&QComboBox::currentIndexChanged,this,[this]{invalidateChessboard();invalidateCalibrationDraft();});
    auto* acquisition=section("2. Görüntü ve kurulum");auto* form=new QFormLayout;
    calibrationLens_=new QLineEdit;calibrationLens_->setObjectName("calibrationLens");calibrationLens_->setMaxLength(256);form->addRow("Lens / kurulum",calibrationLens_);
    calibrationPlane_=new QLineEdit;calibrationPlane_->setObjectName("calibrationPlane");calibrationPlane_->setMaxLength(256);form->addRow("Çalışma düzlemi",calibrationPlane_);acquisition->addLayout(form);
    auto* setup=button(acquisition,"Kurulumu uygula","calibrationSetup");connect(setup,&QPushButton::clicked,this,[this]{try{if(editorSession_.busy() || !service_.activeRecipe())return;const auto before=service_.activeRecipe()->definition;service_.setCalibrationSetup(calibrationLens_->text(),calibrationPlane_->text());if(before!=service_.activeRecipe()->definition){invalidateCalibrationDraft();editorSession_.frame.reset();invalidateMatch();calibrationFrameId_.clear();refreshWorkspace();}calibrationStatus_->setText("Kurulum taslakta. Yeni kare alın.");}catch(const std::exception& e){calibrationStatus_->setText(QString::fromUtf8(e.what()));}});
    auto* acquire=button(acquisition,"Kare al — reçete kaynağı","calibrationAcquire");connect(acquire,&QPushButton::clicked,this,[this]{if(!service_.activeRecipe())return;for(const auto& t:service_.tools())if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){invalidateChessboard(true);runExecution(t.id);refreshCalibration();return;}});
    auto* import=button(acquisition,"Test görüntüsü içe aktar (geçici)","calibrationTestImport");connect(import,&QPushButton::clicked,this,[this]{if(editorSession_.busy())return;invalidateChessboard(true);refreshCalibration();const auto path=QFileDialog::getOpenFileName(this,"Calibration test görüntüsü",{},"Görüntüler (*.png *.jpg *.jpeg *.bmp)");if(!path.isEmpty())importCalibrationTestImage(path);});
    auto* scale=section("3. Ölçeği belirle");auto* manual=new QWidget;manual->setObjectName("calibrationManualFields");auto* mf=new QFormLayout(manual);mf->setContentsMargins(0,0,0,0);
    auto* factor=new QLineEdit;factor->setObjectName("calibrationPxPerMm");factor->setMaxLength(32);factor->setPlaceholderText("Pozitif px/mm — ör. 20");factor->setLocale(QLocale(QLocale::Turkish,QLocale::Turkey));mf->addRow("px/mm",factor);
    auto* inverse=note("");inverse->setObjectName("calibrationInverse");mf->addRow("mm/px (1 ÷ px/mm)",inverse);mf->addRow(note("Aralık: 0,000001–1000000 px/mm; en çok 6 ondalık. Türkçe ondalık virgül; binlik ayırıcı kullanılmaz."));scale->addWidget(manual);
    connect(factor,&QLineEdit::textChanged,this,[this]{invalidateCalibrationDraft();});
    auto* teaching=new QWidget;teaching->setObjectName("calibrationTeachingFields");auto* tl=new QVBoxLayout(teaching);tl->setContentsMargins(0,0,0,0);tl->addWidget(note("Ayrı öğretim segmenti — doğrulama örneği olarak tekrar kullanılmaz."));
    auto* teach=new QTableWidget(1,6);teach->setObjectName("calibrationTeaching");teach->setHorizontalHeaderLabels({"ID","X1 px","Y1 px","X2 px","Y2 px","Bilinen mm"});teach->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);teach->setFixedHeight(90);for(int c=0;c<6;++c)teach->setItem(0,c,new QTableWidgetItem);tl->addWidget(teach);scale->addWidget(teaching);connect(teach,&QTableWidget::cellChanged,this,[this]{invalidateCalibrationDraft();});
    auto* boardFields=new QWidget;boardFields->setObjectName("calibrationBoardFields");auto* bl=new QVBoxLayout(boardFields);bl->setContentsMargins(0,0,0,0);auto* board=new QFormLayout;
    auto* targetId=new QLineEdit;targetId->setObjectName("chessboardTargetId");targetId->setMaxLength(128);board->addRow("Plaka kimliği",targetId);
    for(const auto& entry:QVector<QPair<QString,QString>>{{"chessboardColumns","İç köşe sütun"},{"chessboardRows","İç köşe satır"}}){auto* count=new QSpinBox;count->setObjectName(entry.first);count->setRange(3,32);count->setValue(entry.first=="chessboardColumns" ? 9 : 6);board->addRow(entry.second,count);connect(count,&QSpinBox::valueChanged,this,[this]{invalidateChessboard();refreshCalibration();});}
    auto* spacing=new QDoubleSpinBox;spacing->setObjectName("chessboardSpacing");spacing->setDecimals(6);spacing->setRange(0,1000000);spacing->setSuffix(" mm");board->addRow("Gerçek kare kenarı",spacing);bl->addLayout(board);
    connect(spacing,&QDoubleSpinBox::valueChanged,this,[this]{invalidateChessboard();refreshCalibration();});connect(targetId,&QLineEdit::textChanged,this,[this]{invalidateChessboard();refreshCalibration();});
    connect(button(bl,"Köşeleri bul","calibrationDetectChessboard"),&QPushButton::clicked,this,&MainWindow::detectCalibrationChessboard);
    chessboardStatus_=note("İç köşe sayısı ve gerçek kare mm girin. Orijin ilk köşe; +X sütun / +Y satır. Simetrik plakanın fiziksel yönü otomatik belirlenmez.");chessboardStatus_->setObjectName("chessboardStatus");bl->addWidget(chessboardStatus_);
    connect(button(bl,"Perspektif dönüşümünü hesapla","calibrationFitHomography"),&QPushButton::clicked,this,&MainWindow::calculateCalibrationHomography);
    auto* technical=new QGroupBox("Teknik ayrıntılar");technical->setObjectName("calibrationTechnical");technical->setCheckable(true);technical->setChecked(false);auto* details=new QVBoxLayout(technical);
    calibrationMappingStatus_=note("Dönüşüm yok. Raw görüntü; lens bozulması düzeltilmez. Bölge iç köşelerle sınırlıdır.");calibrationMappingStatus_->setObjectName("calibrationMappingStatus");details->addWidget(calibrationMappingStatus_);calibrationMappingStatus_->hide();connect(technical,&QGroupBox::toggled,calibrationMappingStatus_,&QWidget::setVisible);bl->addWidget(technical);scale->addWidget(boardFields);
    connect(button(scale,"Ölçeği uygula","calibration_calculate"),&QPushButton::clicked,this,[this]{calibrationAction("calculate");});
    auto* pointTarget=new QComboBox;pointTarget->setObjectName("calibrationPointTarget");pointTarget->addItems({"Nokta hedefi seçin","Öğretim","Seçili doğrulama satırı","Deneme"});scale->addWidget(pointTarget);
    auto* pointRow=new QHBoxLayout;auto* start=button(pointRow,"Başlangıç seç","calibrationPickStart");auto* end=button(pointRow,"Bitiş seç","calibrationPickEnd");scale->addLayout(pointRow);
    auto* pickStatus=note("Hedef ve başlangıç/bitiş seçin; ardından görüntüye tıklayın.");pickStatus->setObjectName("calibrationPickStatus");scale->addWidget(pickStatus);
    for(auto* b:{start,end})connect(b,&QPushButton::clicked,this,[this,b,start]{calibrationViewer_->setProperty("pickEnd",b!=start);findChild<QLabel*>("calibrationPickStatus")->setText((b==start ? "Başlangıç" : "Bitiş")+QString(" seçilecek: ")+findChild<QComboBox*>("calibrationPointTarget")->currentText());});
    connect(pointTarget,&QComboBox::currentIndexChanged,this,[this]{calibrationViewer_->setProperty("pickEnd",QVariant{});findChild<QLabel*>("calibrationPickStatus")->setText("Başlangıç seç / Bitiş seç ile hedefi hazırlayın.");});
    calibrationViewer_->onPointClicked=[this](QPointF p){if(editorSession_.busy() || !calibrationViewer_->property("pickEnd").isValid())return;const auto target=findChild<QComboBox*>("calibrationPointTarget")->currentIndex();const bool end=calibrationViewer_->property("pickEnd").toBool();auto* table=target==1 ? findChild<QTableWidget*>("calibrationTeaching") : calibrationSamples_;const auto model=findChild<QComboBox*>("calibrationModel")->currentData().toString();if(target==3){findChild<QDoubleSpinBox*>(end ? "trialX2" : "trialX1")->setValue(p.x());findChild<QDoubleSpinBox*>(end ? "trialY2" : "trialY1")->setValue(p.y());}else if((target==1 && model=="uniform_planar_scale") || (target==2 && table->currentRow()>=0)){const int row=target==1 ? 0 : table->currentRow();for(int i=0;i<2;++i){const int col=(end ? 3 : 1)+i;table->setItem(row,col,new QTableWidgetItem(QString::number(i==0 ? p.x() : p.y(),'g',17)));}}else{findChild<QLabel*>("calibrationPickStatus")->setText("Geçerli hedef ve doğrulama satırı seçin.");return;}calibrationViewer_->setProperty("pickEnd",QVariant{});findChild<QLabel*>("calibrationPickStatus")->setText("Nokta orijinal px olarak yazıldı. Diğer uç için yeniden seçim yapın.");refreshCalibration();};
    auto* trials=new QWidget;trials->setObjectName("calibrationTrialFields");auto* trialLayout=new QVBoxLayout(trials);trialLayout->setContentsMargins(0,0,0,0);auto* trialForm=new QFormLayout;
    const QStringList labels{"Başlangıç X","Başlangıç Y","Bitiş X","Bitiş Y"};int li=0;for(const auto& name:QStringList{"trialX1","trialY1","trialX2","trialY2"}){auto* value=new QDoubleSpinBox;value->setObjectName(name);value->setDecimals(6);value->setRange(0,16000000);value->setSuffix(" px");trialForm->addRow(labels[li++],value);connect(value,&QDoubleSpinBox::valueChanged,this,[this]{++calibrationGeneration_;++chessboardGeneration_;calibrationTrialPoints_.clear();calibrationTrialStatus_->setText("Noktalar değişti — deneme mesafesini yeniden hesaplayın.");});}trialLayout->addLayout(trialForm);
    connect(button(trialLayout,"Deneme mesafesi","calibrationTrialDistance"),&QPushButton::clicked,this,&MainWindow::inspectCalibrationDistance);calibrationTrialStatus_=note("Doğrulanmadı — deneme. Üretim sonucu değildir.");calibrationTrialStatus_->setObjectName("calibrationTrialStatus");trialLayout->addWidget(calibrationTrialStatus_);scale->addWidget(trials);
    auto* validation=section("4. Bağımsız doğrulama");auto* sampleNote=note("Ölçeği belirlerken kullanmadığınız en az iki bilinen mesafeyi ekleyin.");sampleNote->setObjectName("calibrationSampleNote");validation->addWidget(sampleNote);
    calibrationSamples_=new QTableWidget(0,8);calibrationSamples_->setObjectName("calibrationSamples");calibrationSamples_->setHorizontalHeaderLabels({"ID","X1 px","Y1 px","X2 px","Y2 px","Bilinen mm","Ölçülen mm","Mutlak hata mm"});calibrationSamples_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);calibrationSamples_->setMinimumHeight(170);validation->addWidget(calibrationSamples_);
    auto* sampleButtons=new QHBoxLayout;connect(button(sampleButtons,"Örnek ekle","calibrationAddSample"),&QPushButton::clicked,this,[this]{if(calibrationSamples_->rowCount()<256){const auto r=calibrationSamples_->rowCount();calibrationSamples_->insertRow(r);for(int c=0;c<6;++c)calibrationSamples_->setItem(r,c,new QTableWidgetItem(c==0 ? QString("sample-%1").arg(r+1) : QString{}));invalidateCalibrationDraft();}});connect(button(sampleButtons,"Seçili örneği sil","calibrationRemoveSample"),&QPushButton::clicked,this,[this]{if(calibrationSamples_->currentRow()>=0){calibrationSamples_->removeRow(calibrationSamples_->currentRow());invalidateCalibrationDraft();}});validation->addLayout(sampleButtons);
    auto* error=new QDoubleSpinBox;error->setObjectName("calibrationMaxError");error->setRange(0,1000000);error->setDecimals(6);error->setValue(.01);error->setSuffix(" mm");auto* ef=new QFormLayout;ef->addRow("Maksimum mutlak hata",error);validation->addLayout(ef);connect(button(validation,"Doğrula","calibration_validate"),&QPushButton::clicked,this,[this]{calibrationAction("validate");});
    auto* persistence=section("5. Kaydet ve bağla");auto* pf=new QFormLayout;calibrationId_=new QLineEdit;calibrationId_->setObjectName("calibrationId");calibrationId_->setMaxLength(128);pf->addRow("Kalibrasyon ID",calibrationId_);auto* version=new QSpinBox;version->setObjectName("calibrationVersion");version->setRange(1,1000000000);pf->addRow("Yeni sürüm",version);persistence->addLayout(pf);
    connect(button(persistence,"Kaydet ve bağla","calibration_save"),&QPushButton::clicked,this,[this]{calibrationAction("save");});calibrationStatus_=note("Kalibrasyon yok.");calibrationStatus_->setObjectName("calibrationStatus");persistence->addWidget(calibrationStatus_);
    auto* records=new QGroupBox("Kayıtlar ve rapor");records->setObjectName("calibrationRecords");records->setCheckable(true);records->setChecked(false);auto* rl=new QVBoxLayout(records);auto* content=new QWidget;content->setObjectName("calibrationRecordsContent");auto* rc=new QVBoxLayout(content);rc->setContentsMargins(0,0,0,0);rl->addWidget(content);content->hide();connect(records,&QGroupBox::toggled,content,&QWidget::setVisible);
    connect(button(rc,"Kayıtlı kanıtı aç","calibration_load"),&QPushButton::clicked,this,[this]{calibrationAction("load");});connect(button(rc,"Test kanıtını kaydet","calibrationSaveStudy"),&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getSaveFileName(this,"Test kanıtı kaydet",{},"Calibration study (*.vfcalstudy)");if(!path.isEmpty())calibrationStudyAction("save",path);});connect(button(rc,"Test kanıtını aç","calibrationOpenStudy"),&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getOpenFileName(this,"Test kanıtı aç",{},"Calibration study (*.vfcalstudy)");if(!path.isEmpty())calibrationStudyAction("load",path);});connect(button(rc,"Kanıt raporunu dışa aktar (CSV)","calibrationExportReport"),&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getSaveFileName(this,"Kanıt raporu",{},"CSV (*.csv)");if(!path.isEmpty())exportCalibrationReport(path);});controls->addWidget(records);
    auto* stop=new QPushButton("Stop");stop->setObjectName("calibrationStop");layout->addWidget(stop);connect(stop,&QPushButton::clicked,this,[this]{editorSession_.stop(SessionJob::Teaching);editorSession_.stop(SessionJob::Image);editorSession_.stop(SessionJob::Execution);invalidateChessboard();refreshCalibration();});
    for(auto* edit:{calibrationId_,calibrationLens_,calibrationPlane_})connect(edit,&QLineEdit::textEdited,this,[this,edit]{if(edit!=calibrationId_)invalidateChessboard();invalidateCalibrationDraft();});connect(version,&QSpinBox::valueChanged,this,[this]{invalidateCalibrationDraft();});connect(error,&QDoubleSpinBox::valueChanged,this,[this]{invalidateCalibrationDraft();});connect(calibrationSamples_,&QTableWidget::cellChanged,this,[this]{invalidateCalibrationDraft();});connect(calibrationSamples_,&QTableWidget::currentCellChanged,this,[this]{refreshCalibration();});
    for(auto* f:calibrationControls_->findChildren<QFormLayout*>())f->setRowWrapPolicy(QFormLayout::WrapAllRows);calibrationSamples_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);teach->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);controls->addStretch();scroll->setWidget(calibrationControls_);body->addWidget(scroll,1);layout->addLayout(body,1);
    auto* wrapper=new QWidget;auto* outer=new QVBoxLayout(wrapper);auto* stack=new QStackedWidget;
    calibrationHome_=new CalibrationHome([this]{CalibrationHomeContext c;if(service_.hasProject()){c.path=service_.path();c.projectId=service_.project().id;c.recipes=service_.project().recipes;if(service_.activeRecipe())c.currentRecipe=*service_.activeRecipe();c.directory=QFileInfo(c.path).absolutePath();if(service_.activeRecipe())for(const auto& t:service_.tools())if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){if(c.source){c.source.reset();break;}c.source=t;}}return c;},[this]{return !editorSession_.busy() && !cameraSession_.busy() && !runtimeJobs_.busy() && !(runtime_ && runtime_->busy()) && !(runtimeCoordinator_ && (runtimeCoordinator_->state()==RuntimeState::Running || runtimeCoordinator_->state()==RuntimeState::Starting || runtimeCoordinator_->state()==RuntimeState::Stopping)) && !(plc_ && plc_->blocking());});
    calibrationHome_->onSelectionChanged=[this]{editorSession_.calibrationChanged(editorContext());runtimeResult_.reset();refreshMatchOverlay();refreshFlow();showRuntimeResult();refreshRuntime();};
    calibrationOverview_=new CalibrationOverview([this]{return measurementWorkspace();});
    calibrationOverview_->onCameraManager=[this]{selectPage("camera");};
    calibrationOverview_->setRecipes([this]{return service_.hasProject() ? service_.project().recipes:QVector<Recipe>{};});
    calibrationOverview_->onChanged=[this]{refreshFlow();refreshMatchOverlay();runtimeResult_.reset();showRuntimeResult();refreshRuntime();};
    stack->addWidget(calibrationOverview_);stack->addWidget(calibrationHome_);stack->addWidget(page);outer->addWidget(stack,1);
    auto* navigation=new QHBoxLayout;outer->addLayout(navigation);
    QStringList names{"Sistem özeti","Önceki kalibrasyon akışı","Eski kayıt / kanıt ekranı"};
    for(int i=0;i<names.size();++i){auto* b=new QPushButton(names[i]);b->setObjectName(i==2 ? "calibrationLegacyView":i==1 ? "calibrationPreviousView":"calibrationOverviewView");navigation->addWidget(b);connect(b,&QPushButton::clicked,this,[stack,i]{stack->setCurrentIndex(i);});}
    return wrapper;
}
MeasurementWorkspace* MainWindow::measurementWorkspace(){
        const auto key=service_.hasProject() ? service_.path()+"\n"+service_.project().id:QString{};
        if(key!=measurementWorkspaceKey_){measurementWorkspace_.reset();measurementWorkspaceKey_.clear();
            if(service_.hasProject() && !service_.path().isEmpty()){
                measurementWorkspace_=std::make_shared<MeasurementWorkspace>(service_.path(),service_.project().id,[this]{return !editorSession_.busy() && !cameraSession_.busy() && !runtimeJobs_.busy() && !(runtime_ && runtime_->busy()) && !(runtimeCoordinator_ && (runtimeCoordinator_->state()==RuntimeState::Running || runtimeCoordinator_->state()==RuntimeState::Starting || runtimeCoordinator_->state()==RuntimeState::Stopping)) && !(plc_ && plc_->blocking());});
                measurementWorkspaceKey_=key;
            }
        }return measurementWorkspace_.get();
}
void MainWindow::invalidateCalibrationDraft(){++calibrationGeneration_;calibrationCandidate_.reset();calibrationReport_.reset();calibrationTrialPoints_.clear();if(calibrationTrialStatus_)calibrationTrialStatus_->setText("Doğrulanmadı — deneme sonucu yok.");if(calibrationSamples_){QSignalBlocker blocker(calibrationSamples_);for(int r=0;r<calibrationSamples_->rowCount();++r)for(int c=6;c<8;++c)if(auto* item=calibrationSamples_->item(r,c))item->setText("");}if(calibrationStatus_)calibrationStatus_->setText("Örnek/ayar değişti — yeniden hesaplama ve doğrulama gerekli.");refreshCalibration();}
void MainWindow::refreshCalibration() {
    if(calibrationOverview_)calibrationOverview_->refresh();
    if(calibrationHome_)calibrationHome_->refresh();
    if(!calibrationControls_)return;const auto* r=service_.activeRecipe();const auto identity=r ? service_.project().id+"/"+r->id : QString{};
    const auto context=flowSignature();if(context!=chessboardContext_){chessboardContext_=context;invalidateChessboard(true);}
    if(identity!=calibrationRecipeId_) {
        invalidateChessboard(true);calibrationRecipeId_=identity;++calibrationGeneration_;calibrationCandidate_.reset();calibrationFrameId_.clear();
        const QSignalBlocker table(calibrationSamples_),id(calibrationId_),lens(calibrationLens_),plane(calibrationPlane_);auto* version=findChild<QSpinBox*>("calibrationVersion");const QSignalBlocker vb(version);
        calibrationSamples_->setRowCount(0);{auto* factor=findChild<QLineEdit*>("calibrationPxPerMm");QSignalBlocker fb(factor);factor->clear();auto* teaching=findChild<QTableWidget*>("calibrationTeaching");QSignalBlocker tb(teaching);for(int c=0;c<6;++c)teaching->item(0,c)->setText("");}{auto* mode=findChild<QComboBox*>("calibrationModel");const QSignalBlocker blocker(mode);mode->setCurrentIndex(mode->findData(r && r->calibrationBinding ? r->calibrationBinding->modelType : QString("manual_planar_scale")));}calibrationId_->setText(r && r->calibrationBinding ? r->calibrationBinding->calibrationId : "");version->setValue(r && r->calibrationBinding ? std::min(r->calibrationBinding->version+1,1000000000) : 1);
        const auto setup=r ? r->definition.value("planarSetup").toObject() : QJsonObject{};calibrationLens_->setText(setup["lensId"].toString());calibrationPlane_->setText(setup["workingPlaneId"].toString());
        calibrationStatus_->setText(r && r->calibrationBinding ? QString("Bağlı kayıt: %1 v%2. Kare al / Kayıtlı kanıtı aç. Current kontrolü her Run Flow'da yapılır.").arg(r->calibrationBinding->calibrationId).arg(r->calibrationBinding->version) : "Kalibrasyon bağlantısı yok.");
    }
    const auto frameId=editorSession_.frame ? editorSession_.frame->metadata().frameId : QString{};
    if(frameId!=calibrationFrameId_){invalidateChessboard(true);calibrationFrameId_=frameId;++calibrationGeneration_;calibrationCandidate_.reset();}
    const bool hasFrame=r && editorSession_.frame && !frameId.isEmpty();
    const auto displayFrame=calibrationTestImage_ ? std::optional<ImageFrame>(calibrationTestImage_->frame) : editorSession_.frame;
    if(displayFrame) {
        if(calibrationViewer_->property("frameId").toString()!=displayFrame->metadata().frameId){calibrationViewer_->setImage(displayFrame->preview());calibrationViewer_->setProperty("frameId",displayFrame->metadata().frameId);}
        calibrationViewer_->setToolOverlay({},{});calibrationFrameInfo_->setText(QString("%1×%2 px • edinilmiş kare • seçili örnek: %3").arg(displayFrame->size().width()).arg(displayFrame->size().height()).arg(calibrationSamples_->currentRow()<0 ? "yok" : "doğrulama"));
        if(calibrationTestImage_)calibrationFrameInfo_->setText(QString("%1×%2 px • Dosyadan test • %3\nSHA256: %4").arg(displayFrame->size().width()).arg(displayFrame->size().height()).arg(QFileInfo(displayFrame->path()).fileName()).arg(calibrationTestImage_->contentSha256.left(32)+" "+calibrationTestImage_->contentSha256.mid(32)));
        const auto row=calibrationSamples_->currentRow();QVector<QPointF> points;
        if(row>=0)for(int c:{1,3}) {
            const auto* x=calibrationSamples_->item(row,c);const auto* y=calibrationSamples_->item(row,c+1);bool xok=false,yok=false;
            double px=x ? x->text().toDouble(&xok) : 0,py=y ? y->text().toDouble(&yok) : 0;
            if(x && !xok)px=QLocale().toDouble(x->text(),&xok);if(y && !yok)py=QLocale().toDouble(y->text(),&yok);
            if(xok && yok && std::isfinite(px) && std::isfinite(py) && px>=0 && py>=0 && px<displayFrame->size().width() && py<displayFrame->size().height())points.append({px,py});
        }
        if(points.size()==2)calibrationViewer_->setToolOverlay({},points);
        if(calibrationChessboard_){const auto& p=calibrationChessboard_->pixels;calibrationViewer_->setChessboardOverlay(p,calibrationChessboard_->target.columns);}
        if(calibrationTrialPoints_.size()==2)calibrationViewer_->setToolOverlay({},calibrationTrialPoints_,{},calibrationTrialStatus_->text().section('\n',0,0).section(" •",0,0));
    }else {calibrationViewer_->setProperty("frameId",QString{});calibrationViewer_->clearImage("Kare al ile reçetenin kaynağından görüntü edinin.");calibrationFrameInfo_->setText("Edinilmiş kare yok.");}
    const bool idle=!editorSession_.busy(),ready=r && idle;
    const auto model=findChild<QComboBox*>("calibrationModel")->currentData().toString();const bool homography=model=="raw_planar_homography",manual=model=="manual_planar_scale";
    calibrationControls_->setEnabled(idle);findChild<QPushButton*>("calibrationStop")->setEnabled(!idle);
    findChild<QWidget*>("calibrationManualFields")->setVisible(manual);findChild<QWidget*>("calibrationTeachingFields")->setVisible(!manual && !homography);findChild<QWidget*>("calibrationBoardFields")->setVisible(homography);findChild<QWidget*>("calibrationTrialFields")->setVisible(manual || homography);
    findChild<QLabel*>("calibrationMethodNote")->setText(manual ? "Sabit ve her yönde aynı ölçek. px/mm girin; doğrulama katsayıyı değiştirmez." : homography ? "Perspektif için plaka. Lens bozulması düzeltilmez; fiziksel kabul ayrıca gerekir." : "İki ham nokta ve bilinen mm ile sabit ölçek. Öğretim ve doğrulama ayrıdır.");
    bool factorValid=false;try{const double value=manualInput(findChild<QLineEdit*>("calibrationPxPerMm")->text());factorValid=true;findChild<QLabel*>("calibrationInverse")->setText(manualLocale().toString(1/value,'g',12));}catch(const std::exception& e){findChild<QLabel*>("calibrationInverse")->setText(QString::fromUtf8(e.what()));}
    findChild<QPushButton*>("calibrationTestImport")->setEnabled(idle);findChild<QPushButton*>("calibrationAcquire")->setEnabled(ready);
    findChild<QPushButton*>("calibrationDetectChessboard")->setEnabled(idle && homography && displayFrame.has_value());findChild<QPushButton*>("calibrationFitHomography")->setEnabled(idle && homography && calibrationChessboard_.has_value());
    findChild<QPushButton*>("calibrationTrialDistance")->setEnabled(idle && (manual ? calibrationCandidate_.has_value() : homography && calibrationMapping_.has_value()));
    findChild<QPushButton*>("calibration_calculate")->setText(manual ? "Ölçeği uygula" : "Ölçeği hesapla");findChild<QPushButton*>("calibration_calculate")->setVisible(!homography);
    findChild<QPushButton*>("calibration_calculate")->setEnabled(idle && (manual ? displayFrame.has_value() && factorValid : ready && hasFrame && !calibrationTestImage_));
    findChild<QPushButton*>("calibration_validate")->setEnabled(idle && calibrationSamples_->rowCount()>=2 && (homography ? calibrationMapping_.has_value() : calibrationCandidate_.has_value()));
    findChild<QPushButton*>("calibration_save")->setEnabled(ready && hasFrame && !calibrationTestImage_ && calibrationCandidate_ && calibrationCandidate_->validated());
    findChild<QPushButton*>("calibration_load")->setEnabled(ready && r->calibrationBinding.has_value());findChild<QPushButton*>("calibrationExportReport")->setEnabled(idle && calibrationReport_.has_value());
    findChild<QPushButton*>("calibrationSaveStudy")->setVisible(homography);findChild<QPushButton*>("calibrationSaveStudy")->setEnabled(idle && homography && calibrationTestImage_ && calibrationMapping_);findChild<QPushButton*>("calibrationOpenStudy")->setEnabled(idle);
    findChild<QLabel*>("calibrationSampleNote")->setText(QString("Ölçeği belirlerken kullanmadığınız en az iki bilinen mesafeyi ekleyin. Tüm satırlar bağımsız doğrulamadır. ")+(homography ? "Fit köşelerini tekrar kullanmayın." : manual && calibrationTestImage_ ? "Dosyadan test bellektedir; manuel test kaydı desteklenmez, reçeteye bağlanamaz." : "Gerçek bağımsız referans mm girin."));
    QString next=!idle ? "İşlem sürüyor; Stop ile iptal edebilirsiniz." : !displayFrame ? "2. Reçete kaynağından kare alın veya test görüntüsü içe aktarın." : calibrationCandidate_ && calibrationCandidate_->validated() ? "5. Kaydet ve bağla → Save Project / Recipes. Fiziksel doğruluk kabulü değildir." : homography && !calibrationMapping_ ? "3. Plaka bilgileri → Köşeleri bul → Perspektif dönüşümünü hesapla." : !homography && !calibrationCandidate_ ? "3. Ölçeği belirleyin; kimlik ve yeni sürümü 5. bölümde girin." : "4. En az iki bağımsız segmenti girip Doğrula kullanın.";
    if(calibrationTestImage_)next+=" Dosyadan test reçeteye bağlanamaz.";findChild<QLabel*>("calibrationStep")->setText(next);
}
void MainWindow::calibrationAction(const QString& action) {
    if(action=="validate" && !editorSession_.busy()){calibrationReport_.reset();QSignalBlocker table(calibrationSamples_);for(int r=0;r<calibrationSamples_->rowCount();++r)for(int col=6;col<8;++col)if(auto* item=calibrationSamples_->item(r,col))item->setText("");}
    const auto* recipe=service_.activeRecipe();const bool homography=action=="load" ? recipe && recipe->calibrationBinding && recipe->calibrationBinding->modelType=="raw_planar_homography" : findChild<QComboBox*>("calibrationModel")->currentData().toString()=="raw_planar_homography";
    if(homography){homographyCalibrationAction(action);return;}
    const auto selected=action=="load" && recipe && recipe->calibrationBinding ? recipe->calibrationBinding->modelType : findChild<QComboBox*>("calibrationModel")->currentData().toString();if(selected=="manual_planar_scale"){manualCalibrationAction(action);return;}
    if(action=="load" && !editorSession_.busy())invalidateChessboard(true);
    if(editorSession_.busy() || !service_.activeRecipe() || calibrationTestImage_)return;
    try {
        const auto recipe=*service_.activeRecipe();const auto path=service_.path()+".calibrations.json";
        const auto frame=editorSession_.frame;const auto candidate=calibrationCandidate_;const auto id=calibrationId_->text().trimmed();const int version=findChild<QSpinBox*>("calibrationVersion")->value();
        const double maxError=findChild<QDoubleSpinBox*>("calibrationMaxError")->value();QVector<CalibrationSample> samples;
        if(action!="load") {
            if(!frame)throw std::runtime_error("Önce kare alın.");const auto installation=CalibrationWorkflow::installation(recipe,*frame,"uniform_planar_scale");
            if(recipe.definition["planarSetup"].toObject()["lensId"].toString()!=calibrationLens_->text().trimmed() || recipe.definition["planarSetup"].toObject()["workingPlaneId"].toString()!=calibrationPlane_->text().trimmed())throw std::runtime_error("Önce değişen kurulumu uygulayın ve yeni kare alın.");
            if(action=="validate")for(int r=0;r<calibrationSamples_->rowCount();++r) {
                auto text=[&](int c){const auto* item=calibrationSamples_->item(r,c);return item ? item->text() : QString{};};
                auto number=[&](int c){bool ok=false;auto v=text(c).toDouble(&ok);if(!ok)v=QLocale().toDouble(text(c),&ok);if(!ok)throw std::runtime_error("Örnek koordinatı/bilinen mm sayısal olmalı.");return v;};
                samples.append({text(0),{{number(1),number(2)},installation.source},{{number(3),number(4)},installation.source},number(5)});
            }
            if(action=="calculate") {
                auto* teaching=findChild<QTableWidget*>("calibrationTeaching");auto text=[&](int col){return teaching->item(0,col)->text();};auto number=[&](int col){bool ok=false;double v=text(col).toDouble(&ok);if(!ok)v=QLocale().toDouble(text(col),&ok);if(!ok || !std::isfinite(v))throw std::runtime_error("Öğretim koordinatı/bilinen mm sayısal olmalı.");return v;};
                samples.prepend({text(0),{{number(1),number(2)},installation.source},{{number(3),number(4)},installation.source},number(5)});
            }
        }
        const auto signature=flowSignature();const auto generation=calibrationGeneration_;auto* watcher=editorSession_.watch<CalibrationWork>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);calibrationStatus_->setText("BUSY — "+action);refreshCalibration();refreshFlow();
        connect(watcher,&QFutureWatcher<CalibrationWork>::finished,this,[this,watcher,signature,generation,action,frame,path,cancel]{
            const auto work=watcher->result();watcher->deleteLater();
            if(cancel->load() || signature!=flowSignature() || generation!=calibrationGeneration_){calibrationCandidate_.reset();calibrationStatus_->setText("İptal/değişen reçete/kare/ayar nedeniyle sonuç uygulanmadı.");refreshCalibration();refreshFlow();return;}
            if(!work.error.isEmpty()){calibrationCandidate_.reset();calibrationStatus_->setText(work.error);refreshCalibration();refreshFlow();return;}
            if(action=="save")try {CalibrationWorkflow::bind(service_,*frame,path,work.value->dependency().calibrationId);calibrationCandidate_.reset();auto* version=findChild<QSpinBox*>("calibrationVersion");{const QSignalBlocker blocker(version);version->setValue(std::min(work.value->dependency().version+1,1000000000));}invalidateMatch();refreshWorkspace();calibrationStatus_->setText("Kalibrasyon kaydedildi ve reçeteye bağlandı. Reçete DRAFT; Save ile reçete bağlantısını kaydedin.");}catch(const std::exception& e){calibrationStatus_->setText("Katalog kaydedildi; reçete bağlanamadı: "+QString::fromUtf8(e.what()));}
            else if(action=="load") {
                {const QSignalBlocker mode(findChild<QComboBox*>("calibrationModel"));findChild<QComboBox*>("calibrationModel")->setCurrentIndex(0);}
                const auto json=MeasurementService{}.encode(*work.value);QJsonArray samples;for(const auto& s:json["holdout"].toArray())samples.append(s);auto* teaching=findChild<QTableWidget*>("calibrationTeaching");const QSignalBlocker tb(teaching);const QStringList tk{"id","x1","y1","x2","y2","knownMm"};for(int c=0;c<6;++c){const auto v=json["teaching"].toObject().value(tk[c]);teaching->item(0,c)->setText(v.isString() ? v.toString() : QString::number(v.toDouble(),'g',17));}
                const QSignalBlocker blocker(calibrationSamples_);calibrationSamples_->setRowCount(samples.size());const QStringList keys{"id","x1","y1","x2","y2","knownMm"};
                for(int r=0;r<samples.size();++r)for(int c=0;c<keys.size();++c){const QJsonValue v=samples[r].toObject().value(keys[c]);calibrationSamples_->setItem(r,c,new QTableWidgetItem(v.isString() ? v.toString() : QString::number(v.toDouble(),'g',17)));}
                calibrationCandidate_.reset();showCalibrationEvidence(*work.value);calibrationStatus_->setText(QString("Kayıtlı v%1 kanıtı açıldı • %2 mm/px • RMS %3 mm. Yeni sürüm için yeniden hesapla/doğrula.").arg(work.value->dependency().version).arg(work.value->millimetresPerPixel(),0,'g',10).arg(work.value->validationRmsMillimetres(),0,'g',10));
            } else {
                calibrationCandidate_=work.value;if(work.value->validated())showCalibrationEvidence(*work.value);
                QString text=QString("%1 • %2 mm/px • %3×%4 px").arg(work.value->validated() ? "Holdout doğrulandı — üretim izni değildir" : "Candidate — holdout doğrulaması gerekli").arg(work.value->millimetresPerPixel(),0,'g',10).arg(work.value->dependency().resolution.width()).arg(work.value->dependency().resolution.height());
                if(work.value->validated())text+=QString(" • RMS %1 mm").arg(work.value->validationRmsMillimetres(),0,'g',10);
                calibrationStatus_->setText(text);
            }
            refreshCalibration();refreshFlow();
        });
        editorSession_.launch(watcher,[recipe,path,frame,candidate,id,version,maxError,samples,action]{CalibrationWork result;try {
            if(action=="calculate")result.value=CalibrationWorkflow::teach(recipe,*frame,id,version,samples.first());
            else if(action=="validate") {if(!candidate)throw std::runtime_error("Önce hesaplayın.");result.value=MeasurementService{}.validate(*candidate,samples,maxError);}
            else if(action=="save") {if(!candidate || !candidate->validated())throw std::runtime_error("Önce doğrulayın.");CalibrationService(path).save(*candidate);result.value=candidate;}
            else result.value=CalibrationService(path).load(recipe.calibrationBinding->calibrationId,recipe.calibrationBinding->version);
        }catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;});
    }catch(const std::exception& e){calibrationStatus_->setText(QString::fromUtf8(e.what()));refreshCalibration();}
}
void MainWindow::manualCalibrationAction(const QString& action) {
    if(editorSession_.busy())return;
    if(action=="load")invalidateChessboard(true);
    try {
        const auto* active=service_.activeRecipe();const auto recipe=active ? std::optional<Recipe>(*active) : std::optional<Recipe>{};
        const auto frame=calibrationTestImage_ ? std::optional<ImageFrame>(calibrationTestImage_->frame) : editorSession_.frame;
        const bool offline=calibrationTestImage_.has_value();const auto path=service_.path()+".calibrations.json";const auto candidate=calibrationCandidate_;
        const auto id=calibrationId_->text().trimmed();const int version=findChild<QSpinBox*>("calibrationVersion")->value();const double error=findChild<QDoubleSpinBox*>("calibrationMaxError")->value();
        double factor=0;CalibrationDependency setup;QVector<CalibrationSample> samples;
        if(action=="calculate" || action=="validate") {
            if(!frame)throw std::runtime_error("Önce kare alın veya test görüntüsü içe aktarın.");
            if(id.isEmpty())throw std::runtime_error("5. bölümde kalibrasyon kimliği ve yeni sürümü girin.");
            if(offline)setup={id,version,"pending",frame->metadata().sourceId,"test-only/manual",frame->size(),"test-only/manual",{frame->metadata().coordinateSystemId,LengthUnit::Pixel,AxisConvention::XRightYDown},{"test-only/manual/mm",LengthUnit::Millimetre,AxisConvention::XRightYUp}};
            else {if(!recipe)throw std::runtime_error("Önce reçete açın.");if(recipe->definition["planarSetup"].toObject()["lensId"].toString()!=calibrationLens_->text().trimmed() || recipe->definition["planarSetup"].toObject()["workingPlaneId"].toString()!=calibrationPlane_->text().trimmed())throw std::runtime_error("Değişen kurulumu uygulayın ve yeni kare alın.");const auto s=CalibrationWorkflow::installation(*recipe,*frame,"manual_planar_scale");setup={id,version,"pending",s.cameraId,s.lensId,s.resolution,s.workingPlaneId,s.source,s.target};}
            if(action=="calculate")factor=manualInput(findChild<QLineEdit*>("calibrationPxPerMm")->text());
            else for(int r=0;r<calibrationSamples_->rowCount();++r){auto text=[&](int c){auto* item=calibrationSamples_->item(r,c);return item ? item->text() : QString{};};auto number=[&](int c){bool ok=false;double v=text(c).toDouble(&ok);if(!ok)v=QLocale().toDouble(text(c),&ok);if(!ok || !std::isfinite(v))throw std::runtime_error("Doğrulama koordinatı/bilinen mm sayısal olmalı.");return v;};samples.append({text(0),{{number(1),number(2)},setup.source},{{number(3),number(4)},setup.source},number(5)});}
        }
        if(action=="save" && (offline || !recipe || !frame || !candidate || !candidate->manualProvenance() || !candidate->validated()))throw std::runtime_error("Edinilmiş kare ve doğrulanmış manuel kanıt gerekli; dosyadan test bağlanamaz.");
        if(action=="load" && (!recipe || !recipe->calibrationBinding))throw std::runtime_error("Kayıtlı bağlantı yok.");
        if(action=="validate"){calibrationReport_.reset();QSignalBlocker table(calibrationSamples_);for(int r=0;r<calibrationSamples_->rowCount();++r)for(int c=6;c<8;++c)if(auto* i=calibrationSamples_->item(r,c))i->setText("");}
        const auto generation=calibrationGeneration_,imageGeneration=chessboardGeneration_;const auto signature=flowSignature();auto* watcher=editorSession_.watch<CalibrationWork>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);calibrationStatus_->setText("İşlem sürüyor — "+action);refreshCalibration();refreshFlow();
        connect(watcher,&QFutureWatcher<CalibrationWork>::finished,this,[this,watcher,generation,imageGeneration,signature,cancel,action,frame,path]{const auto work=watcher->result();watcher->deleteLater();
            if(cancel->load() || generation!=calibrationGeneration_ || imageGeneration!=chessboardGeneration_ || signature!=flowSignature()){calibrationCandidate_.reset();calibrationReport_.reset();calibrationStatus_->setText("İptal/değişiklik nedeniyle sonuç uygulanmadı. Yazılmış katalog/CSV geri alınmaz.");}
            else if(!work.error.isEmpty()){calibrationCandidate_.reset();calibrationReport_.reset();calibrationStatus_->setText(work.error);}
            else if(action=="save") {try{CalibrationWorkflow::bind(service_,*frame,path,work.value->dependency().calibrationId,"manual_planar_scale");calibrationCandidate_.reset();auto* version=findChild<QSpinBox*>("calibrationVersion");{QSignalBlocker blocker(version);version->setValue(std::min(work.value->dependency().version+1,1000000000));}refreshWorkspace();showCalibrationEvidence(*work.value);calibrationStatus_->setText("Kalibrasyon kaydedildi, reçete bağlantısı taslak. Save Project / Recipes ile kaydedin.");}catch(const std::exception& e){calibrationStatus_->setText("Katalog kaydedildi; reçete bağlantısı başarısız: "+QString::fromUtf8(e.what()));}}
            else if(action=="load") {const auto& c=*work.value;if(!c.manualProvenance())throw std::runtime_error("Bağlı kayıt manuel değil.");const QSignalBlocker table(calibrationSamples_),mode(findChild<QComboBox*>("calibrationModel")),factor(findChild<QLineEdit*>("calibrationPxPerMm"));findChild<QComboBox*>("calibrationModel")->setCurrentIndex(2);
                // Preserve exact stored input even if a service-created record has more UI precision.
                findChild<QLineEdit*>("calibrationPxPerMm")->setText(manualText(c.pixelsPerMillimetre()));const auto evidence=MeasurementService{}.encode(c)["holdout"].toArray();const QStringList keys{"id","x1","y1","x2","y2","knownMm"};calibrationSamples_->setRowCount(evidence.size());for(int r=0;r<evidence.size();++r)for(int col=0;col<6;++col){const auto v=evidence[r].toObject().value(keys[col]);calibrationSamples_->setItem(r,col,new QTableWidgetItem(v.isString() ? v.toString() : QString::number(v.toDouble(),'g',17)));}calibrationCandidate_.reset();showCalibrationEvidence(c);calibrationStatus_->setText(QString("Kayıtlı manuel v%1 • gerçek px/mm %2 • RMS %3 mm / max %4 mm. Yeni sürüm için yeniden uygula/doğrula.").arg(c.dependency().version).arg(c.pixelsPerMillimetre(),0,'g',17).arg(c.validationRmsMillimetres()).arg(c.maxValidationErrorMillimetres()));}
            else {calibrationCandidate_=work.value;if(work.value->validated())showCalibrationEvidence(*work.value);calibrationStatus_->setText(work.value->validated() ? QString("Bağımsız doğrulama kabulü • RMS %1 mm / maksimum hata %2 mm. Fiziksel doğruluk/üretim izni değildir.").arg(work.value->validationRmsMillimetres(),0,'g',10).arg(work.value->maxValidationErrorMillimetres(),0,'g',10) : QString("Doğrulanmadı — deneme • %1 px/mm → %2 mm/px. Bağımsız doğrulama gerekli.").arg(work.value->pixelsPerMillimetre(),0,'g',17).arg(work.value->millimetresPerPixel(),0,'g',17));}
            refreshCalibration();refreshFlow();});
        editorSession_.launch(watcher,[action,recipe,frame,offline,path,candidate,setup,factor,id,version,samples,error]{CalibrationWork work;try{if(action=="calculate")work.value=offline ? MeasurementService{}.manualScale(setup,factor,{"file_test",frame->metadata().sourceId,frame->metadata().frameId,frame->size()}) : CalibrationWorkflow::manualScale(*recipe,*frame,id,version,factor);else if(action=="validate"){if(!candidate || !candidate->manualProvenance())throw std::runtime_error("Önce ölçeği uygulayın.");work.value=MeasurementService{}.validate(*candidate,samples,error);}else if(action=="save"){CalibrationService(path).save(*candidate);work.value=candidate;}else work.value=CalibrationService(path).load(recipe->calibrationBinding->calibrationId,recipe->calibrationBinding->version);}catch(const std::exception& e){work.error=QString::fromUtf8(e.what());}return work;});
    }catch(const std::exception& e){calibrationCandidate_.reset();calibrationReport_.reset();calibrationStatus_->setText(QString::fromUtf8(e.what()));refreshCalibration();}
}

void MainWindow::invalidateChessboard(bool clearImage) {
    ++chessboardGeneration_;calibrationReport_.reset();if(calibrationSamples_){QSignalBlocker blocker(calibrationSamples_);for(int r=0;r<calibrationSamples_->rowCount();++r)for(int c=6;c<8;++c)if(auto* item=calibrationSamples_->item(r,c))item->setText("");}if(calibrationStatus_)calibrationStatus_->setText("Kalibrasyon adayı/holdout sonucu yok — hedef/görüntü/kurulum değişti.");calibrationChessboard_.reset();calibrationMapping_.reset();calibrationTrialPoints_.clear();calibrationCandidate_.reset();if(calibrationMappingStatus_)calibrationMappingStatus_->setText("H yok — görüntü/hedef/kurulum değişti.");if(calibrationTrialStatus_)calibrationTrialStatus_->setText("Mesafe sonucu yok.");if(clearImage)calibrationTestImage_.reset();
    if(chessboardStatus_)chessboardStatus_->setText("Köşe sonucu yok — hedef/kurulum/görüntü değişti. Yeniden Köşeleri bul.");
}
void MainWindow::importCalibrationTestImage(const QString& path) {
    if(editorSession_.busy())return;
    invalidateChessboard(true);calibrationCandidate_.reset();calibrationStatus_->setText("Test importu: uniform scale hesap/bağlama sonucu temizlendi.");refreshCalibration();
    const auto generation=chessboardGeneration_;const auto signature=flowSignature();
    struct Work {std::optional<CalibrationTestImage> image;QString error;};
    auto* watcher=editorSession_.watch<Work>(this,SessionJob::Image);const auto cancel=editorSession_.token(SessionJob::Image);
    chessboardStatus_->setText("BUSY — test görüntüsü okunuyor");refreshCalibration();refreshFlow();
    connect(watcher,&QFutureWatcher<Work>::finished,this,[this,watcher,generation,signature,cancel]{const auto result=watcher->result();watcher->deleteLater();
        if(cancel->load() || generation!=chessboardGeneration_ || signature!=flowSignature()){chessboardStatus_->setText("İptal/değişiklik nedeniyle import sonucu uygulanmadı.");}
        else if(!result.error.isEmpty())chessboardStatus_->setText(result.error);
        else {calibrationTestImage_=result.image;chessboardStatus_->setText("Dosyadan test — target bilgilerini girip Köşeleri bul. Kamera binding'i değişmedi.");}
        refreshCalibration();refreshFlow();});
    editorSession_.launch(watcher,[path]{Work work;try{work.image=ChessboardService::importImage(path);}catch(const std::exception& e){work.error=QString::fromUtf8(e.what());}return work;});
}
void MainWindow::detectCalibrationChessboard() {
    if(editorSession_.busy())return;
    const auto frame=calibrationTestImage_ ? std::optional<ImageFrame>(calibrationTestImage_->frame) : editorSession_.frame;if(!frame)return;
    invalidateChessboard();refreshCalibration();
    const ChessboardTarget target{findChild<QLineEdit*>("chessboardTargetId")->text().trimmed(),findChild<QSpinBox*>("chessboardColumns")->value(),findChild<QSpinBox*>("chessboardRows")->value(),findChild<QDoubleSpinBox*>("chessboardSpacing")->value()};
    const auto generation=chessboardGeneration_;const auto signature=flowSignature();
    struct Work {std::optional<ChessboardObservation> observation;QString error;};
    auto* watcher=editorSession_.watch<Work>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);
    chessboardStatus_->setText("BUSY — chessboard köşeleri aranıyor");refreshCalibration();refreshFlow();
    connect(watcher,&QFutureWatcher<Work>::finished,this,[this,watcher,generation,signature,cancel]{const auto result=watcher->result();watcher->deleteLater();
        if(cancel->load() || generation!=chessboardGeneration_ || signature!=flowSignature())chessboardStatus_->setText("İptal/değişiklik nedeniyle köşe sonucu uygulanmadı.");
        else if(!result.error.isEmpty())chessboardStatus_->setText(result.error);
        else {calibrationChessboard_=result.observation;chessboardStatus_->setText(QString("%1 alt piksel köşe • P1=(0,0) mm; +X sütun / +Y satır. Sıra ve yönü inceleyin. Girilen %2 mm görselden doğrulanmaz. H/validation/üretim kabulü yok.").arg(result.observation->pixels.size()).arg(result.observation->target.squareMillimetres));}
        refreshCalibration();refreshFlow();});
    editorSession_.launch(watcher,[frame,target]{Work work;try{work.observation=ChessboardService::detect(*frame,target);}catch(const std::exception& e){work.error=QString::fromUtf8(e.what());}return work;});
}

void MainWindow::calculateCalibrationHomography() {
    if(editorSession_.busy() || !calibrationChessboard_)return;
    calibrationMapping_.reset();calibrationTrialPoints_.clear();invalidateCalibrationDraft();calibrationTrialStatus_->setText("Mesafe sonucu yok.");
    const auto observation=*calibrationChessboard_;const auto generation=chessboardGeneration_;const auto signature=flowSignature();
    struct Work {std::optional<PlaneMapping> mapping;QString error;};
    auto* watcher=editorSession_.watch<Work>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);calibrationMappingStatus_->setText("BUSY — H hesaplanıyor (fit sınırı 1 px)");refreshCalibration();refreshFlow();
    connect(watcher,&QFutureWatcher<Work>::finished,this,[this,watcher,generation,signature,cancel]{auto work=watcher->result();watcher->deleteLater();
        if(cancel->load() || generation!=chessboardGeneration_ || signature!=flowSignature())calibrationMappingStatus_->setText("H iptal/değişiklik nedeniyle uygulanmadı.");
        else if(!work.error.isEmpty())calibrationMappingStatus_->setText(work.error);
        else {calibrationMapping_=work.mapping;QString text=QString("H hesaplandı • Fit RMS %1 mm • inverse RMS %2 px / max %3 px\nBağımsız validation henüz yok. Raw pixel → mm; lens düzeltmesi uygulanmadı.\n").arg(work.mapping->fitRmsMillimetres(),0,'g',8).arg(work.mapping->inverseRmsPixels(),0,'g',8).arg(work.mapping->maxInversePixels(),0,'g',8);const auto h=work.mapping->matrix();for(int r=0;r<3;++r)text+=QString("[%1  %2  %3]\n").arg(h[r*3],0,'g',8).arg(h[r*3+1],0,'g',8).arg(h[r*3+2],0,'g',8);calibrationMappingStatus_->setText(text);}
        refreshCalibration();refreshFlow();});
    editorSession_.launch(watcher,[observation]{Work work;try{work.mapping=PlaneMappingService::fit(observation);}catch(const std::exception& e){work.error=QString::fromUtf8(e.what());}return work;});
}
void MainWindow::inspectCalibrationDistance() {
    if(!editorSession_.busy() && calibrationCandidate_ && calibrationCandidate_->manualProvenance()) {
        const auto c=*calibrationCandidate_;const Point2D a{{findChild<QDoubleSpinBox*>("trialX1")->value(),findChild<QDoubleSpinBox*>("trialY1")->value()},c.dependency().source},b{{findChild<QDoubleSpinBox*>("trialX2")->value(),findChild<QDoubleSpinBox*>("trialY2")->value()},c.dependency().source};calibrationTrialPoints_={a.position,b.position};
        struct Work{std::optional<double> mm;QString error;};const auto generation=calibrationGeneration_,imageGeneration=chessboardGeneration_;const auto signature=flowSignature();auto* watcher=editorSession_.watch<Work>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);calibrationTrialStatus_->setText("Deneme hesaplanıyor.");refreshCalibration();refreshFlow();
        connect(watcher,&QFutureWatcher<Work>::finished,this,[this,watcher,generation,imageGeneration,signature,cancel]{const auto work=watcher->result();watcher->deleteLater();if(cancel->load() || generation!=calibrationGeneration_ || imageGeneration!=chessboardGeneration_ || signature!=flowSignature())calibrationTrialStatus_->setText("Mesafe sonucu yok — iptal/değişiklik.");else if(!work.error.isEmpty())calibrationTrialStatus_->setText("Mesafe yok: "+work.error);else calibrationTrialStatus_->setText(QString("%1 mm • Doğrulanmadı — deneme; üretim sonucu değildir.").arg(*work.mm,0,'g',17));refreshCalibration();refreshFlow();});
        editorSession_.launch(watcher,[c,a,b]{Work work;try{work.mm=MeasurementService{}.trialDistance(c,a,b);}catch(const std::exception& e){work.error=QString::fromUtf8(e.what());}return work;});return;
    }
    if(editorSession_.busy() || !calibrationMapping_)return;
    const QPointF a{findChild<QDoubleSpinBox*>("trialX1")->value(),findChild<QDoubleSpinBox*>("trialY1")->value()},b{findChild<QDoubleSpinBox*>("trialX2")->value(),findChild<QDoubleSpinBox*>("trialY2")->value()};calibrationTrialPoints_={a,b};
    const auto mapping=*calibrationMapping_;const auto generation=chessboardGeneration_;const auto signature=flowSignature();
    struct Work {std::optional<PlaneDistance> distance;QString error;};
    auto* watcher=editorSession_.watch<Work>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);calibrationTrialStatus_->setText("BUSY — mesafe hesaplanıyor");refreshCalibration();refreshFlow();
    connect(watcher,&QFutureWatcher<Work>::finished,this,[this,watcher,generation,signature,cancel]{const auto work=watcher->result();watcher->deleteLater();
        if(cancel->load() || generation!=chessboardGeneration_ || signature!=flowSignature())calibrationTrialStatus_->setText("Mesafe iptal/değişiklik nedeniyle uygulanmadı.");
        else if(!work.error.isEmpty())calibrationTrialStatus_->setText("Mesafe yok: "+work.error);
        else {const auto& d=*work.distance;calibrationTrialStatus_->setText(QString("%1 px → %2 mm\nP1=(%3,%4) mm; P2=(%5,%6) mm. Deneme; validation/PASS değildir.").arg(d.pixels,0,'g',10).arg(d.millimetres,0,'g',10).arg(d.firstMillimetres.x(),0,'g',8).arg(d.firstMillimetres.y(),0,'g',8).arg(d.secondMillimetres.x(),0,'g',8).arg(d.secondMillimetres.y(),0,'g',8));}refreshCalibration();refreshFlow();});
    editorSession_.launch(watcher,[mapping,a,b]{Work work;try{work.distance=PlaneMappingService::distance(mapping,a,b);}catch(const std::exception& e){work.error=QString::fromUtf8(e.what());}return work;});
}

void MainWindow::homographyCalibrationAction(const QString& action) {
    if(editorSession_.busy())return;if(action=="calculate"){calculateCalibrationHomography();return;}if(action=="load")invalidateChessboard(true);
    try {
        const auto* active=service_.activeRecipe();const auto recipe=active ? std::optional<Recipe>(*active) : std::optional<Recipe>{};
        const auto frame=calibrationTestImage_ ? std::optional<ImageFrame>(calibrationTestImage_->frame) : editorSession_.frame;
        const auto mapping=calibrationMapping_;const auto candidate=calibrationCandidate_;const auto path=service_.path()+".calibrations.json";
        const auto id=calibrationId_->text().trimmed();const int version=findChild<QSpinBox*>("calibrationVersion")->value();const auto maxError=findChild<QDoubleSpinBox*>("calibrationMaxError")->value();
        QVector<CalibrationSample> samples;CalibrationDependency setup;
        if(action=="validate") {
            if(!frame || !mapping)throw std::runtime_error("Önce köşeleri bulup H hesaplayın.");
            if(id.isEmpty() || id.size()>128)throw std::runtime_error("Calibration ID gerekli.");
            if(calibrationTestImage_)setup={id,version,"pending",frame->metadata().sourceId,"test-only/raw",frame->size(),"test-only/"+mapping->observation().target.id,{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"test-plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
            else {if(!recipe)throw std::runtime_error("Önce reçete açın.");if(recipe->definition["planarSetup"].toObject()["lensId"].toString()!=calibrationLens_->text().trimmed() || recipe->definition["planarSetup"].toObject()["workingPlaneId"].toString()!=calibrationPlane_->text().trimmed())throw std::runtime_error("Değişen kurulumu uygulayın ve yeni kare alın.");const auto s=CalibrationWorkflow::installation(*recipe,*frame,"raw_planar_homography");setup={id,version,"pending",s.cameraId,s.lensId,s.resolution,s.workingPlaneId,s.source,s.target};}
            for(int r=0;r<calibrationSamples_->rowCount();++r){auto text=[&](int c){const auto* item=calibrationSamples_->item(r,c);return item ? item->text() : QString{};};auto number=[&](int c){bool ok=false;double v=text(c).toDouble(&ok);if(!ok)v=QLocale().toDouble(text(c),&ok);if(!ok || !std::isfinite(v))throw std::runtime_error("Holdout koordinatı/bilinen mm sayısal olmalı.");return v;};samples.append({text(0),{{number(1),number(2)},setup.source},{{number(3),number(4)},setup.source},number(5)});}
        }
        if(action=="save" && (!recipe || !frame || calibrationTestImage_ || !candidate || !candidate->mapping() || !candidate->validated()))throw std::runtime_error("Test görüntüsü üretim kaydına bağlanamaz; edinilmiş kare ve bağımsız validation gerekli.");
        if(action=="load" && (!recipe || !recipe->calibrationBinding))throw std::runtime_error("Kayıtlı binding yok.");
        const auto generation=chessboardGeneration_,validationGeneration=calibrationGeneration_;const auto signature=flowSignature();auto* watcher=editorSession_.watch<CalibrationWork>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);calibrationStatus_->setText("BUSY — Homography "+action);refreshCalibration();refreshFlow();
        connect(watcher,&QFutureWatcher<CalibrationWork>::finished,this,[this,watcher,generation,validationGeneration,signature,cancel,action,frame,path]{const auto work=watcher->result();watcher->deleteLater();
            if(cancel->load() || generation!=chessboardGeneration_ || validationGeneration!=calibrationGeneration_ || signature!=flowSignature()){calibrationCandidate_.reset();calibrationStatus_->setText("Değişiklik/iptal nedeniyle homography kabulü uygulanmadı.");}
            else if(!work.error.isEmpty()){calibrationCandidate_.reset();calibrationStatus_->setText(work.error);}
            else if(action=="save"){try{CalibrationWorkflow::bind(service_,*frame,path,work.value->dependency().calibrationId,"raw_planar_homography");calibrationCandidate_.reset();auto* version=findChild<QSpinBox*>("calibrationVersion");{QSignalBlocker blocker(version);version->setValue(std::min(work.value->dependency().version+1,1000000000));}refreshWorkspace();showCalibrationEvidence(*work.value);calibrationStatus_->setText("Homography kaydedildi ve reçeteye bağlandı. DRAFT; normal Save gerekli. Fiziksel doğruluk kabulü değildir.");}catch(const std::exception& e){calibrationStatus_->setText("Katalog kaydedildi; binding başarısız: "+QString::fromUtf8(e.what()));}}
            else if(action=="load"){const auto& c=*work.value;if(!c.mapping()){calibrationStatus_->setText("Bağlı kayıt homography değil.");}else{const QSignalBlocker table(calibrationSamples_),mode(findChild<QComboBox*>("calibrationModel")),id(calibrationId_);findChild<QComboBox*>("calibrationModel")->setCurrentIndex(1);calibrationId_->setText(c.dependency().calibrationId);calibrationMapping_=c.mapping();calibrationChessboard_=c.mapping()->observation();calibrationCandidate_.reset();const auto json=MeasurementService{}.encode(c);const auto evidence=json["holdout"].toArray();calibrationSamples_->setRowCount(evidence.size());const QStringList keys{"id","x1","y1","x2","y2","knownMm"};for(int r=0;r<evidence.size();++r)for(int col=0;col<6;++col){const auto v=evidence[r].toObject().value(keys[col]);calibrationSamples_->setItem(r,col,new QTableWidgetItem(v.isString() ? v.toString() : QString::number(v.toDouble(),'g',17)));}calibrationStatus_->setText(QString("Kayıtlı homography v%1 • holdout RMS %2 mm / max %3 mm. Fresh kare/kurulum her yürütmede denetlenir.").arg(c.dependency().version).arg(c.validationRmsMillimetres(),0,'g',10).arg(c.maxValidationErrorMillimetres(),0,'g',10));showCalibrationEvidence(c);calibrationMappingStatus_->setText(QString("Kayıtlı H • Fit RMS %1 mm; inverse RMS %2 px. Yeni sürüm için fresh köşeler ve yeniden validation.").arg(c.mapping()->fitRmsMillimetres()).arg(c.mapping()->inverseRmsPixels()));}}
            else {calibrationCandidate_=work.value;showCalibrationEvidence(*work.value);calibrationStatus_->setText(QString("Bağımsız holdout kabulü • RMS %1 mm / max %2 mm. %3 • Fiziksel doğruluk/üretim izni değildir.").arg(work.value->validationRmsMillimetres(),0,'g',10).arg(work.value->maxValidationErrorMillimetres(),0,'g',10).arg(work.value->dependency().cameraId.startsWith("calibration-test:") ? "Dosyadan test; kamera binding'i yok" : "Edinilmiş kaynak"));}
            refreshCalibration();refreshFlow();});
        editorSession_.launch(watcher,[action,mapping,candidate,setup,samples,maxError,path,recipe]{CalibrationWork work;try{if(action=="validate")work.value=MeasurementService{}.validate(MeasurementService{}.teachHomography(setup,*mapping),samples,maxError);else if(action=="save"){CalibrationService(path).save(*candidate);work.value=candidate;}else work.value=CalibrationService(path).load(recipe->calibrationBinding->calibrationId,recipe->calibrationBinding->version);}catch(const std::exception& e){work.error=QString::fromUtf8(e.what());}return work;});
    }catch(const std::exception& e){calibrationCandidate_.reset();calibrationStatus_->setText(QString::fromUtf8(e.what()));refreshCalibration();}
}

void MainWindow::calibrationStudyAction(const QString& action,const QString& path) {
    if(editorSession_.busy())return;
    try {
        const auto image=calibrationTestImage_;const auto mapping=calibrationMapping_;const auto candidate=calibrationCandidate_;
        if(action=="save" && (!image || !mapping))throw std::runtime_error("Önce test görüntüsünde H hesaplayın.");
        if(action=="load")invalidateChessboard(true);
        const auto id=calibrationId_->text().trimmed();const int version=findChild<QSpinBox*>("calibrationVersion")->value();const auto generation=chessboardGeneration_;const auto signature=flowSignature();
        struct Work {std::optional<CalibrationStudy> study;QString error;};auto* watcher=editorSession_.watch<Work>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);calibrationStatus_->setText("BUSY — Test kanıtı "+action);refreshCalibration();refreshFlow();
        connect(watcher,&QFutureWatcher<Work>::finished,this,[this,watcher,action,generation,signature,cancel]{const auto work=watcher->result();watcher->deleteLater();
            if(cancel->load() || generation!=chessboardGeneration_ || signature!=flowSignature())calibrationStatus_->setText("İptal/değişiklik nedeniyle test kanıtı uygulanmadı.");else if(!work.error.isEmpty())calibrationStatus_->setText(work.error);
            else if(action=="load"){calibrationTestImage_=work.study->image;calibrationMapping_=work.study->calibration.mapping();calibrationChessboard_=calibrationMapping_->observation();const auto& c=work.study->calibration;const QSignalBlocker table(calibrationSamples_),mode(findChild<QComboBox*>("calibrationModel")),id(calibrationId_),version(findChild<QSpinBox*>("calibrationVersion"));findChild<QComboBox*>("calibrationModel")->setCurrentIndex(1);calibrationId_->setText(c.dependency().calibrationId);findChild<QSpinBox*>("calibrationVersion")->setValue(c.dependency().version);const auto evidence=MeasurementService{}.encode(c)["holdout"].toArray();calibrationSamples_->setRowCount(evidence.size());const QStringList keys{"id","x1","y1","x2","y2","knownMm"};for(int r=0;r<evidence.size();++r)for(int col=0;col<6;++col){const auto v=evidence[r].toObject().value(keys[col]);calibrationSamples_->setItem(r,col,new QTableWidgetItem(v.isString() ? v.toString() : QString::number(v.toDouble(),'g',17)));}calibrationCandidate_=c;showCalibrationEvidence(c);calibrationMappingStatus_->setText(QString("Test kanıtından H • fit RMS %1 mm; inverse RMS %2 px. Dosya hash'i ve matematiksel evidence replay denetlendi.").arg(calibrationMapping_->fitRmsMillimetres()).arg(calibrationMapping_->inverseRmsPixels()));calibrationStatus_->setText(c.validated() ? QString("Dosyadan test • holdout RMS %1 mm / max %2 mm. Fiziksel kamera binding'i/üretim izni yok.").arg(c.validationRmsMillimetres()).arg(c.maxValidationErrorMillimetres()) : "Dosyadan test candidate • bağımsız holdout henüz yok; üretim izni yok.");}
            else calibrationStatus_->setText("Test kanıtı kaydedildi. Kaynak görüntü aynı yolda/hash ile korunmalı; fiziksel kamera binding'i değildir.");refreshCalibration();refreshFlow();});
        editorSession_.launch(watcher,[action,path,image,mapping,candidate,id,version]{Work work;try{if(action=="load")work.study=CalibrationStudyStore::load(path);else{PlanarCalibration c=candidate && candidate->mapping() ? *candidate : MeasurementService{}.teachHomography({id.isEmpty() ? "test-study" : id,version,"pending",image->frame.metadata().sourceId,"test-only/raw",image->frame.size(),"test-only/"+mapping->observation().target.id,{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"test-plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}},*mapping);CalibrationStudyStore::save(path,*image,c);work.study=CalibrationStudy{*image,c};}}catch(const std::exception& e){work.error=QString::fromUtf8(e.what());}return work;});
    }catch(const std::exception& e){calibrationStatus_->setText(QString::fromUtf8(e.what()));refreshCalibration();}
}

void MainWindow::showCalibrationEvidence(const PlanarCalibration& c) {
    if(c.mapping()){const auto& t=c.mapping()->observation().target;auto* id=findChild<QLineEdit*>("chessboardTargetId");auto* columns=findChild<QSpinBox*>("chessboardColumns");auto* rows=findChild<QSpinBox*>("chessboardRows");auto* spacing=findChild<QDoubleSpinBox*>("chessboardSpacing");const QSignalBlocker a(id),b(columns),d(rows),e(spacing);id->setText(t.id);columns->setValue(t.columns);rows->setValue(t.rows);spacing->setValue(t.squareMillimetres);}
    if(!c.validated())return;calibrationReport_=c;const int offset=0;const QSignalBlocker table(calibrationSamples_);
    for(int i=0;i<c.measuredHoldoutMillimetres().size() && i+offset<calibrationSamples_->rowCount();++i){for(int col=6;col<8;++col){const double value=col==6 ? c.measuredHoldoutMillimetres()[i] : c.residualHoldoutMillimetres()[i];auto* item=new QTableWidgetItem(QString::number(value,'g',10));item->setFlags(item->flags() & ~Qt::ItemIsEditable);calibrationSamples_->setItem(i+offset,col,item);}}
    auto* bound=findChild<QDoubleSpinBox*>("calibrationMaxError");const QSignalBlocker b(bound);bound->setValue(MeasurementService{}.encode(c)["maxErrorMm"].toDouble());
}
void MainWindow::exportCalibrationReport(const QString& path) {
    if(editorSession_.busy() || !calibrationReport_)return;
    const auto generation=calibrationGeneration_,imageGeneration=chessboardGeneration_;const auto signature=flowSignature();
    const auto calibration=*calibrationReport_;auto* watcher=editorSession_.watch<QString>(this,SessionJob::Teaching);calibrationStatus_->setText("BUSY — kanıt raporu kaydediliyor");refreshCalibration();refreshFlow();
    const auto cancel=editorSession_.token(SessionJob::Teaching);
    connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,generation,imageGeneration,signature,cancel]{const auto error=watcher->result();watcher->deleteLater();calibrationStatus_->setText(cancel->load() || generation!=calibrationGeneration_ || imageGeneration!=chessboardGeneration_ || signature!=flowSignature() ? "Rapor işlemi sırasında oturum değişti/iptal edildi; yazılmış CSV geri alınmaz." : error.isEmpty() ? "Kanıt CSV raporu kaydedildi. Matematiksel holdout raporudur; fiziksel doğruluk/üretim izni değildir." : error);refreshCalibration();refreshFlow();});
    editorSession_.launch(watcher,[path,calibration]{try{CalibrationReportService::saveCsv(path,calibration);return QString{};}catch(const std::exception& e){return QString::fromUtf8(e.what());}});
}

}
