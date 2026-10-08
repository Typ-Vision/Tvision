#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include "application/CalibrationStudyStore.hpp"
#include "application/CalibrationReportService.hpp"
#include <QLabel>
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
#include <QFutureWatcher>
#include <QJsonArray>
#include <QLocale>
#include <stdexcept>
#include <cmath>
namespace visionflow {
namespace {
QLabel* note(const QString& text){auto* label=new QLabel(text);label->setWordWrap(true);label->setTextFormat(Qt::PlainText);label->setObjectName("muted");return label;}
struct CalibrationWork {std::optional<PlanarCalibration> value;QString error;};
}
QWidget* MainWindow::makeCalibration() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);layout->setContentsMargins(0,0,0,0);
    layout->addWidget(note("Pixel Calibration • Chessboard homography / Legacy uniform scale\nSabit çalışma düzleminde raw pixel → mm. Lens düzeltmesi ve robot dönüşümü yok. Lens ve düzlem kimlikleri kullanıcı beyanıdır; fiziksel cihaz kabulü değildir."));
    auto* row=new QHBoxLayout;auto* acquire=new QPushButton("Kare al");acquire->setObjectName("calibrationAcquire");row->addWidget(acquire);
    connect(acquire,&QPushButton::clicked,this,[this]{if(!service_.activeRecipe())return;for(const auto& t:service_.tools())if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){invalidateChessboard(true);runExecution(t.id);refreshCalibration();return;}});
    auto* import=new QPushButton("Test görüntüsü içe aktar (geçici)");import->setObjectName("calibrationTestImport");row->addWidget(import);
    connect(import,&QPushButton::clicked,this,[this]{if(editorSession_.busy())return;invalidateChessboard(true);refreshCalibration();const auto path=QFileDialog::getOpenFileName(this,"Calibration test görüntüsü",{},"Görüntüler (*.png *.jpg *.jpeg *.bmp)");if(path.isEmpty()){chessboardStatus_->setText("İçe aktarma iptal edildi; önceki test sonucu temizlendi.");return;}importCalibrationTestImage(path);});
    auto* stop=new QPushButton("Stop");stop->setObjectName("calibrationStop");row->addWidget(stop);connect(stop,&QPushButton::clicked,this,[this]{editorSession_.stop(SessionJob::Teaching);editorSession_.stop(SessionJob::Image);editorSession_.stop(SessionJob::Execution);});
    auto* editor=new QPushButton("Recipe Editor'a dön");row->addWidget(editor);connect(editor,&QPushButton::clicked,this,[this]{selectPage("editor");});row->addStretch();layout->addLayout(row);
    auto* body=new QHBoxLayout;auto* imagePanel=new QWidget;auto* imageLayout=new QVBoxLayout(imagePanel);imageLayout->setContentsMargins(0,0,0,0);
    calibrationFrameInfo_=note("Görüntü yok.");calibrationFrameInfo_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);imageLayout->addWidget(calibrationFrameInfo_);calibrationViewer_=new ImageViewer;calibrationViewer_->setObjectName("calibrationViewer");imageLayout->addWidget(calibrationViewer_,1);calibrationPixelInfo_=note("Piksel: —");imageLayout->addWidget(calibrationPixelInfo_);body->addWidget(imagePanel,1);
    calibrationViewer_->onPixel=[this](QPoint p){calibrationPixelInfo_->setText(p.x()>=0 ? QString("X:%1 Y:%2 px").arg(p.x()).arg(p.y()) : "Piksel: —");};
    auto* scroll=new QScrollArea;scroll->setObjectName("calibrationScroll");scroll->setWidgetResizable(true);scroll->setMinimumWidth(300);calibrationControls_=new QWidget;auto* controls=new QVBoxLayout(calibrationControls_);auto* mode=new QComboBox;mode->setObjectName("calibrationModel");mode->addItem("Legacy uniform scale","uniform_planar_scale");mode->addItem("Chessboard raw homography","raw_planar_homography");controls->addWidget(mode);connect(mode,&QComboBox::currentIndexChanged,this,[this]{invalidateCalibrationDraft();});auto* board=new QFormLayout;
    auto* targetId=new QLineEdit;targetId->setObjectName("chessboardTargetId");targetId->setMaxLength(128);board->addRow("Chessboard target ID",targetId);
    for(const auto& entry:QVector<QPair<QString,QString>>{{"chessboardColumns","İç köşe sütun"},{"chessboardRows","İç köşe satır"}}){auto* count=new QSpinBox;count->setObjectName(entry.first);count->setRange(3,32);count->setValue(entry.first=="chessboardColumns" ? 9 : 6);board->addRow(entry.second,count);connect(count,&QSpinBox::valueChanged,this,[this]{invalidateChessboard();refreshCalibration();});}
    auto* spacing=new QDoubleSpinBox;spacing->setObjectName("chessboardSpacing");spacing->setDecimals(6);spacing->setRange(0,1000000);spacing->setSuffix(" mm");board->addRow("Gerçek kare kenarı",spacing);
    connect(spacing,&QDoubleSpinBox::valueChanged,this,[this]{invalidateChessboard();refreshCalibration();});connect(targetId,&QLineEdit::textChanged,this,[this]{invalidateChessboard();refreshCalibration();});
    auto* detect=new QPushButton("Köşeleri bul");detect->setObjectName("calibrationDetectChessboard");board->addRow(detect);connect(detect,&QPushButton::clicked,this,&MainWindow::detectCalibrationChessboard);
    controls->addLayout(board);chessboardStatus_=note("Chessboard: hedef bilgilerini girin. Kare sayısı yerine iç köşe sayısını kullanın. Orijin P1; +X sütun, +Y satır yönü. Simetrik plakanın fiziksel yönü otomatik belirlenmez. Köşeleri bul → H hesapla → iki nokta incele. Fit hatası bağımsız doğruluk kabulü değildir.");chessboardStatus_->setObjectName("chessboardStatus");controls->addWidget(chessboardStatus_);
    auto* hButton=new QPushButton("H hesapla");hButton->setObjectName("calibrationFitHomography");controls->addWidget(hButton);connect(hButton,&QPushButton::clicked,this,&MainWindow::calculateCalibrationHomography);
    calibrationMappingStatus_=note("H yok. Lens bozulması ihmal edilebilir varsayılır; kalibre bölge iç köşelerin convex hull alanıdır.");calibrationMappingStatus_->setObjectName("calibrationMappingStatus");controls->addWidget(calibrationMappingStatus_);
    auto* trialForm=new QFormLayout;
    for(const auto& name:QStringList{"trialX1","trialY1","trialX2","trialY2"}){auto* value=new QDoubleSpinBox;value->setObjectName(name);value->setDecimals(6);value->setRange(0,16000000);value->setSuffix(" px");trialForm->addRow(name,value);connect(value,&QDoubleSpinBox::valueChanged,this,[this]{calibrationTrialStatus_->setText("Noktalar değişti — mesafeyi yeniden inceleyin.");});}
    controls->addLayout(trialForm);auto* trialButton=new QPushButton("İki nokta mesafesini incele");trialButton->setObjectName("calibrationTrialDistance");controls->addWidget(trialButton);connect(trialButton,&QPushButton::clicked,this,&MainWindow::inspectCalibrationDistance);
    calibrationTrialStatus_=note("H sonrası görüntüde iki noktaya tıklayın veya orijinal px girin. Deneme bağımsız validation değildir.");calibrationTrialStatus_->setObjectName("calibrationTrialStatus");controls->addWidget(calibrationTrialStatus_);
    calibrationViewer_->onPointClicked=[this](QPointF point){if(!calibrationMapping_ || editorSession_.busy())return;if(calibrationTrialPoints_.size()==2)calibrationTrialPoints_.clear();calibrationTrialPoints_.append(point);const auto i=calibrationTrialPoints_.size();findChild<QDoubleSpinBox*>(i==1 ? "trialX1" : "trialX2")->setValue(point.x());findChild<QDoubleSpinBox*>(i==1 ? "trialY1" : "trialY2")->setValue(point.y());if(i==2)inspectCalibrationDistance();else calibrationTrialStatus_->setText("İkinci noktayı seçin.");};
    auto* form=new QFormLayout;
    calibrationId_=new QLineEdit;calibrationId_->setObjectName("calibrationId");calibrationId_->setMaxLength(128);form->addRow("Calibration ID",calibrationId_);
    auto* version=new QSpinBox;version->setObjectName("calibrationVersion");version->setRange(1,1000000000);form->addRow("Yeni sürüm",version);
    calibrationLens_=new QLineEdit;calibrationLens_->setObjectName("calibrationLens");calibrationLens_->setMaxLength(256);form->addRow("Lens / kurulum kimliği",calibrationLens_);
    calibrationPlane_=new QLineEdit;calibrationPlane_->setObjectName("calibrationPlane");calibrationPlane_->setMaxLength(256);form->addRow("Çalışma düzlemi",calibrationPlane_);
    auto* setup=new QPushButton("Kurulumu uygula");setup->setObjectName("calibrationSetup");form->addRow(setup);
    connect(setup,&QPushButton::clicked,this,[this]{try {
        if(editorSession_.busy() || !service_.activeRecipe())return;
        const auto before=service_.activeRecipe()->definition;service_.setCalibrationSetup(calibrationLens_->text(),calibrationPlane_->text());
        if(before!=service_.activeRecipe()->definition){invalidateCalibrationDraft();editorSession_.frame.reset();invalidateMatch();calibrationFrameId_.clear();refreshWorkspace();}
        calibrationStatus_->setText("Kurulum kaydedilmek üzere DRAFT. Yeni kare alıp örnekleri girin.");
    }catch(const std::exception& e){calibrationStatus_->setText(QString::fromUtf8(e.what()));}});
    auto* error=new QDoubleSpinBox;error->setObjectName("calibrationMaxError");error->setRange(0,1000000);error->setDecimals(6);error->setValue(0.01);error->setSuffix(" mm");form->addRow("Holdout hata sınırı",error);controls->addLayout(form);
    auto* sampleNote=note("İlk satır öğretim; sonraki en az iki farklı segment bağımsız holdout. Koordinatlar orijinal piksel merkezleridir. Bilinen mm uzunluklarını gerçek referans ölçümünden girin.");sampleNote->setObjectName("calibrationSampleNote");controls->addWidget(sampleNote);
    calibrationSamples_=new QTableWidget(0,8);calibrationSamples_->setObjectName("calibrationSamples");calibrationSamples_->setHorizontalHeaderLabels({"ID","X1 px","Y1 px","X2 px","Y2 px","Known mm","Measured mm","Residual mm"});calibrationSamples_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);calibrationSamples_->setMinimumHeight(170);controls->addWidget(calibrationSamples_,1);
    auto* samples=new QHBoxLayout;auto* add=new QPushButton("Örnek ekle");add->setObjectName("calibrationAddSample");samples->addWidget(add);connect(add,&QPushButton::clicked,this,[this]{if(calibrationSamples_->rowCount()<257){const auto r=calibrationSamples_->rowCount();calibrationSamples_->insertRow(r);for(int c=0;c<6;++c)calibrationSamples_->setItem(r,c,new QTableWidgetItem(c==0 ? QString("sample-%1").arg(r+1) : QString{}));invalidateCalibrationDraft();}});
    auto* remove=new QPushButton("Seçili örneği sil");samples->addWidget(remove);connect(remove,&QPushButton::clicked,this,[this]{if(calibrationSamples_->currentRow()>=0){calibrationSamples_->removeRow(calibrationSamples_->currentRow());invalidateCalibrationDraft();}});controls->addLayout(samples);
    auto* commands=new QGridLayout;int index=0;
    for(const auto& entry:QVector<QPair<QString,QString>>{{"calculate","Hesapla"},{"validate","Doğrula"},{"save","Kaydet ve bağla"},{"load","Kayıtlı kanıtı aç"}}){auto* b=new QPushButton(entry.second);b->setObjectName("calibration_"+entry.first);commands->addWidget(b,index/2,index%2);++index;connect(b,&QPushButton::clicked,this,[this,action=entry.first]{calibrationAction(action);});}controls->addLayout(commands);
    auto* studyButtons=new QHBoxLayout;auto* saveStudy=new QPushButton("Test kanıtını kaydet");saveStudy->setObjectName("calibrationSaveStudy");studyButtons->addWidget(saveStudy);connect(saveStudy,&QPushButton::clicked,this,[this]{if(editorSession_.busy())return;const auto path=QFileDialog::getSaveFileName(this,"Test kanıtı kaydet",{},"Calibration study (*.vfcalstudy)");if(!path.isEmpty())calibrationStudyAction("save",path);});
    auto* openStudy=new QPushButton("Test kanıtını aç");openStudy->setObjectName("calibrationOpenStudy");studyButtons->addWidget(openStudy);connect(openStudy,&QPushButton::clicked,this,[this]{if(editorSession_.busy())return;invalidateChessboard(true);refreshCalibration();const auto path=QFileDialog::getOpenFileName(this,"Test kanıtı aç",{},"Calibration study (*.vfcalstudy)");if(!path.isEmpty())calibrationStudyAction("load",path);});controls->addLayout(studyButtons);
    auto* report=new QPushButton("Kanıt raporunu dışa aktar (CSV)");report->setObjectName("calibrationExportReport");controls->addWidget(report);connect(report,&QPushButton::clicked,this,[this]{if(editorSession_.busy())return;const auto path=QFileDialog::getSaveFileName(this,"Kalibrasyon kanıt raporu",{},"CSV (*.csv)");if(!path.isEmpty())exportCalibrationReport(path);});
    calibrationStatus_=note("Kalibrasyon yok. Önce reçete açın.");calibrationStatus_->setObjectName("calibrationStatus");controls->addWidget(calibrationStatus_);controls->addStretch();scroll->setWidget(calibrationControls_);body->addWidget(scroll,1);layout->addLayout(body,1);
    for(auto* edit:{calibrationId_,calibrationLens_,calibrationPlane_})connect(edit,&QLineEdit::textEdited,this,[this,edit]{if(edit!=calibrationId_)invalidateChessboard();invalidateCalibrationDraft();});
    connect(version,&QSpinBox::valueChanged,this,[this]{invalidateCalibrationDraft();});connect(error,&QDoubleSpinBox::valueChanged,this,[this]{invalidateCalibrationDraft();});
    connect(calibrationSamples_,&QTableWidget::cellChanged,this,[this]{invalidateCalibrationDraft();});
    connect(calibrationSamples_,&QTableWidget::currentCellChanged,this,[this]{refreshCalibration();});return page;
}
void MainWindow::invalidateCalibrationDraft(){++calibrationGeneration_;calibrationCandidate_.reset();calibrationReport_.reset();if(calibrationSamples_){QSignalBlocker blocker(calibrationSamples_);for(int r=0;r<calibrationSamples_->rowCount();++r)for(int c=6;c<8;++c)if(auto* item=calibrationSamples_->item(r,c))item->setText("");}if(calibrationStatus_)calibrationStatus_->setText("Örnek/ayar değişti — yeniden hesaplama ve doğrulama gerekli.");refreshCalibration();}
void MainWindow::refreshCalibration() {
    if(!calibrationControls_)return;const auto* r=service_.activeRecipe();const auto identity=r ? service_.project().id+"/"+r->id : QString{};
    const auto context=flowSignature();if(context!=chessboardContext_){chessboardContext_=context;invalidateChessboard(true);}
    if(identity!=calibrationRecipeId_) {
        invalidateChessboard(true);calibrationRecipeId_=identity;++calibrationGeneration_;calibrationCandidate_.reset();calibrationFrameId_.clear();
        const QSignalBlocker table(calibrationSamples_),id(calibrationId_),lens(calibrationLens_),plane(calibrationPlane_);auto* version=findChild<QSpinBox*>("calibrationVersion");const QSignalBlocker vb(version);
        calibrationSamples_->setRowCount(0);{auto* mode=findChild<QComboBox*>("calibrationModel");const QSignalBlocker blocker(mode);mode->setCurrentIndex(r && r->calibrationBinding && r->calibrationBinding->modelType=="raw_planar_homography" ? 1 : 0);}calibrationId_->setText(r && r->calibrationBinding ? r->calibrationBinding->calibrationId : "");version->setValue(r && r->calibrationBinding ? std::min(r->calibrationBinding->version+1,1000000000) : 1);
        const auto setup=r ? r->definition.value("planarSetup").toObject() : QJsonObject{};calibrationLens_->setText(setup["lensId"].toString());calibrationPlane_->setText(setup["workingPlaneId"].toString());
        calibrationStatus_->setText(r && r->calibrationBinding ? QString("Bağlı kayıt: %1 v%2. Kare al / Kayıtlı kanıtı aç. Current kontrolü her Run Flow'da yapılır.").arg(r->calibrationBinding->calibrationId).arg(r->calibrationBinding->version) : "Kalibrasyon bağlantısı yok.");
    }
    const auto frameId=editorSession_.frame ? editorSession_.frame->metadata().frameId : QString{};
    if(frameId!=calibrationFrameId_){invalidateChessboard(true);calibrationFrameId_=frameId;++calibrationGeneration_;calibrationCandidate_.reset();}
    const bool hasFrame=r && editorSession_.frame && !frameId.isEmpty();
    const auto displayFrame=calibrationTestImage_ ? std::optional<ImageFrame>(calibrationTestImage_->frame) : editorSession_.frame;
    if(displayFrame) {
        if(calibrationViewer_->property("frameId").toString()!=displayFrame->metadata().frameId){calibrationViewer_->setImage(displayFrame->preview());calibrationViewer_->setProperty("frameId",displayFrame->metadata().frameId);}
        calibrationViewer_->setToolOverlay({},{});calibrationFrameInfo_->setText(QString("%1×%2 px • edinilmiş kare • seçili örnek: %3").arg(displayFrame->size().width()).arg(displayFrame->size().height()).arg(calibrationSamples_->currentRow()<0 ? "yok" : calibrationSamples_->currentRow()==0 ? "öğretim" : "holdout"));
        if(calibrationTestImage_)calibrationFrameInfo_->setText(QString("%1×%2 px • Dosyadan test • %3\nSHA256: %4").arg(displayFrame->size().width()).arg(displayFrame->size().height()).arg(QFileInfo(displayFrame->path()).fileName()).arg(calibrationTestImage_->contentSha256.left(32)+" "+calibrationTestImage_->contentSha256.mid(32)));
        const auto row=calibrationTestImage_ ? -1 : calibrationSamples_->currentRow();QVector<QPointF> points;
        if(row>=0)for(int c:{1,3}) {
            const auto* x=calibrationSamples_->item(row,c);const auto* y=calibrationSamples_->item(row,c+1);bool xok=false,yok=false;
            double px=x ? x->text().toDouble(&xok) : 0,py=y ? y->text().toDouble(&yok) : 0;
            if(x && !xok)px=QLocale().toDouble(x->text(),&xok);if(y && !yok)py=QLocale().toDouble(y->text(),&yok);
            if(xok && yok && std::isfinite(px) && std::isfinite(py) && px>=0 && py>=0 && px<editorSession_.frame->size().width() && py<editorSession_.frame->size().height())points.append({px,py});
        }
        if(points.size()==2)calibrationViewer_->setToolOverlay({},points);
        if(calibrationChessboard_){const auto& p=calibrationChessboard_->pixels;calibrationViewer_->setChessboardOverlay(p,calibrationChessboard_->target.columns);}
        if(calibrationTrialPoints_.size()==2)calibrationViewer_->setToolOverlay({},calibrationTrialPoints_,{},calibrationTrialStatus_->text().section('\n',0,0));
    }else {calibrationViewer_->setProperty("frameId",QString{});calibrationViewer_->clearImage("Kare al ile reçetenin kaynağından görüntü edinin.");calibrationFrameInfo_->setText("Edinilmiş kare yok.");}
    const bool ready=r && !editorSession_.busy();calibrationControls_->setEnabled(!editorSession_.busy());findChild<QPushButton*>("calibrationTestImport")->setEnabled(!editorSession_.busy());findChild<QPushButton*>("calibrationDetectChessboard")->setEnabled(!editorSession_.busy() && displayFrame.has_value());findChild<QPushButton*>("calibrationFitHomography")->setEnabled(!editorSession_.busy() && calibrationChessboard_.has_value());findChild<QPushButton*>("calibrationTrialDistance")->setEnabled(!editorSession_.busy() && calibrationMapping_.has_value());
    const bool scaleReady=ready && !calibrationTestImage_;findChild<QPushButton*>("calibrationAcquire")->setEnabled(ready);findChild<QPushButton*>("calibrationStop")->setEnabled(editorSession_.busy());
    findChild<QPushButton*>("calibration_calculate")->setEnabled(scaleReady && hasFrame && calibrationSamples_->rowCount()>=1);
    findChild<QPushButton*>("calibration_validate")->setEnabled(scaleReady && hasFrame && calibrationCandidate_.has_value() && !calibrationCandidate_->validated() && calibrationSamples_->rowCount()>=3);
    findChild<QPushButton*>("calibration_save")->setEnabled(scaleReady && hasFrame && calibrationCandidate_.has_value() && calibrationCandidate_->validated());
    findChild<QPushButton*>("calibration_load")->setEnabled(ready && r->calibrationBinding.has_value());
    findChild<QPushButton*>("calibrationExportReport")->setEnabled(!editorSession_.busy() && calibrationReport_.has_value());findChild<QPushButton*>("calibrationSaveStudy")->setEnabled(!editorSession_.busy() && calibrationTestImage_.has_value() && calibrationMapping_.has_value());findChild<QPushButton*>("calibrationOpenStudy")->setEnabled(!editorSession_.busy());
    const bool homography=findChild<QComboBox*>("calibrationModel")->currentData().toString()=="raw_planar_homography";
    findChild<QLabel*>("calibrationSampleNote")->setText(homography ? "Tüm satırlar bağımsız holdout: en az iki farklı segment. Fit köşelerini yeniden validation olarak kullanmayın. Gerçek bağımsız referans mm girin; fiziksel kabul ayrıca gerekir." : "İlk satır öğretim; sonraki en az iki farklı segment bağımsız holdout. Orijinal piksel ve gerçek referans mm girin.");
    if(homography){findChild<QPushButton*>("calibration_calculate")->setEnabled(!editorSession_.busy() && calibrationChessboard_.has_value());findChild<QPushButton*>("calibration_validate")->setEnabled(!editorSession_.busy() && calibrationMapping_.has_value() && calibrationSamples_->rowCount()>=2);findChild<QPushButton*>("calibration_save")->setEnabled(ready && !calibrationTestImage_ && calibrationCandidate_ && calibrationCandidate_->mapping() && calibrationCandidate_->validated());}

}
void MainWindow::calibrationAction(const QString& action) {
    const auto* recipe=service_.activeRecipe();const bool homography=action=="load" ? recipe && recipe->calibrationBinding && recipe->calibrationBinding->modelType=="raw_planar_homography" : findChild<QComboBox*>("calibrationModel")->currentData().toString()=="raw_planar_homography";
    if(homography){homographyCalibrationAction(action);return;}
    if(action=="load" && !editorSession_.busy())invalidateChessboard(true);
    if(editorSession_.busy() || !service_.activeRecipe() || calibrationTestImage_)return;
    try {
        const auto recipe=*service_.activeRecipe();const auto path=service_.path()+".calibrations.json";
        const auto frame=editorSession_.frame;const auto candidate=calibrationCandidate_;const auto id=calibrationId_->text().trimmed();const int version=findChild<QSpinBox*>("calibrationVersion")->value();
        const double maxError=findChild<QDoubleSpinBox*>("calibrationMaxError")->value();QVector<CalibrationSample> samples;
        if(action!="load") {
            if(!frame)throw std::runtime_error("Önce kare alın.");const auto installation=CalibrationWorkflow::installation(recipe,*frame);
            if(recipe.definition["planarSetup"].toObject()["lensId"].toString()!=calibrationLens_->text().trimmed() || recipe.definition["planarSetup"].toObject()["workingPlaneId"].toString()!=calibrationPlane_->text().trimmed())throw std::runtime_error("Önce değişen kurulumu uygulayın ve yeni kare alın.");
            for(int r=0;r<calibrationSamples_->rowCount();++r) {
                auto text=[&](int c){const auto* item=calibrationSamples_->item(r,c);return item ? item->text() : QString{};};
                auto number=[&](int c){bool ok=false;auto v=text(c).toDouble(&ok);if(!ok)v=QLocale().toDouble(text(c),&ok);if(!ok)throw std::runtime_error("Örnek koordinatı/bilinen mm sayısal olmalı.");return v;};
                samples.append({text(0),{{number(1),number(2)},installation.source},{{number(3),number(4)},installation.source},number(5)});
            }
            if(samples.isEmpty())throw std::runtime_error("Öğretim örneği gerekli.");
        }
        const auto signature=flowSignature();const auto generation=calibrationGeneration_;auto* watcher=editorSession_.watch<CalibrationWork>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);calibrationStatus_->setText("BUSY — "+action);refreshCalibration();refreshFlow();
        connect(watcher,&QFutureWatcher<CalibrationWork>::finished,this,[this,watcher,signature,generation,action,frame,path,cancel]{
            const auto work=watcher->result();watcher->deleteLater();
            if(cancel->load() || signature!=flowSignature() || generation!=calibrationGeneration_){calibrationCandidate_.reset();calibrationStatus_->setText("İptal/değişen reçete/kare/ayar nedeniyle sonuç uygulanmadı.");refreshCalibration();refreshFlow();return;}
            if(!work.error.isEmpty()){calibrationCandidate_.reset();calibrationStatus_->setText(work.error);refreshCalibration();refreshFlow();return;}
            if(action=="save")try {CalibrationWorkflow::bind(service_,*frame,path,work.value->dependency().calibrationId);calibrationCandidate_.reset();auto* version=findChild<QSpinBox*>("calibrationVersion");{const QSignalBlocker blocker(version);version->setValue(std::min(work.value->dependency().version+1,1000000000));}invalidateMatch();refreshWorkspace();calibrationStatus_->setText("Kalibrasyon kaydedildi ve reçeteye bağlandı. Reçete DRAFT; Save ile reçete bağlantısını kaydedin.");}catch(const std::exception& e){calibrationStatus_->setText("Katalog kaydedildi; reçete bağlanamadı: "+QString::fromUtf8(e.what()));}
            else if(action=="load") {
                {const QSignalBlocker mode(findChild<QComboBox*>("calibrationModel"));findChild<QComboBox*>("calibrationModel")->setCurrentIndex(0);}
                const auto json=MeasurementService{}.encode(*work.value);QJsonArray samples;samples.append(json["teaching"]);for(const auto& s:json["holdout"].toArray())samples.append(s);
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
            else if(action=="validate") {if(!candidate)throw std::runtime_error("Önce hesaplayın.");result.value=MeasurementService{}.validate(*candidate,samples.mid(1),maxError);}
            else if(action=="save") {if(!candidate || !candidate->validated())throw std::runtime_error("Önce doğrulayın.");CalibrationService(path).save(*candidate);result.value=candidate;}
            else result.value=CalibrationService(path).load(recipe.calibrationBinding->calibrationId,recipe.calibrationBinding->version);
        }catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;});
    }catch(const std::exception& e){calibrationStatus_->setText(QString::fromUtf8(e.what()));refreshCalibration();}
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
    findChild<QComboBox*>("calibrationModel")->setCurrentIndex(1);
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
    if(!c.validated())return;calibrationReport_=c;const int offset=c.mapping() ? 0 : 1;const QSignalBlocker table(calibrationSamples_);
    for(int i=0;i<c.measuredHoldoutMillimetres().size() && i+offset<calibrationSamples_->rowCount();++i){for(int col=6;col<8;++col){const double value=col==6 ? c.measuredHoldoutMillimetres()[i] : c.residualHoldoutMillimetres()[i];auto* item=new QTableWidgetItem(QString::number(value,'g',10));item->setFlags(item->flags() & ~Qt::ItemIsEditable);calibrationSamples_->setItem(i+offset,col,item);}}
    auto* bound=findChild<QDoubleSpinBox*>("calibrationMaxError");const QSignalBlocker b(bound);bound->setValue(MeasurementService{}.encode(c)["maxErrorMm"].toDouble());
}
void MainWindow::exportCalibrationReport(const QString& path) {
    if(editorSession_.busy() || !calibrationReport_)return;
    const auto calibration=*calibrationReport_;auto* watcher=editorSession_.watch<QString>(this,SessionJob::Teaching);calibrationStatus_->setText("BUSY — kanıt raporu kaydediliyor");refreshCalibration();refreshFlow();
    connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher]{const auto error=watcher->result();watcher->deleteLater();calibrationStatus_->setText(error.isEmpty() ? "Kanıt CSV raporu kaydedildi. Matematiksel holdout raporudur; fiziksel doğruluk/üretim izni değildir." : error);refreshCalibration();refreshFlow();});
    editorSession_.launch(watcher,[path,calibration]{try{CalibrationReportService::saveCsv(path,calibration);return QString{};}catch(const std::exception& e){return QString::fromUtf8(e.what());}});
}

}
