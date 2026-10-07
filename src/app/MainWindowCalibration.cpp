#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include <QLabel>
#include <QLineEdit>
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
struct CalibrationWork {std::optional<PlanarScaleCalibration> value;QString error;};
}
QWidget* MainWindow::makeCalibration() {
    auto* page=new QWidget;auto* layout=new QVBoxLayout(page);layout->setContentsMargins(0,0,0,0);
    layout->addWidget(note("Pixel Calibration • Uniform planar scale\nParalel çalışma düzlemi varsayımı. Lens/perspective/robot dönüşümü yok. Lens ve düzlem kimlikleri kullanıcı beyanıdır; fiziksel cihaz kabulü değildir."));
    auto* row=new QHBoxLayout;auto* acquire=new QPushButton("Kare al");acquire->setObjectName("calibrationAcquire");row->addWidget(acquire);
    connect(acquire,&QPushButton::clicked,this,[this]{if(!service_.activeRecipe())return;for(const auto& t:service_.tools())if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")){runExecution(t.id);refreshCalibration();return;}});
    auto* editor=new QPushButton("Recipe Editor'a dön");row->addWidget(editor);connect(editor,&QPushButton::clicked,this,[this]{selectPage("editor");});row->addStretch();layout->addLayout(row);
    auto* body=new QHBoxLayout;auto* imagePanel=new QWidget;auto* imageLayout=new QVBoxLayout(imagePanel);imageLayout->setContentsMargins(0,0,0,0);
    calibrationFrameInfo_=note("Görüntü yok.");imageLayout->addWidget(calibrationFrameInfo_);calibrationViewer_=new ImageViewer;calibrationViewer_->setObjectName("calibrationViewer");imageLayout->addWidget(calibrationViewer_,1);calibrationPixelInfo_=note("Piksel: —");imageLayout->addWidget(calibrationPixelInfo_);body->addWidget(imagePanel,1);
    calibrationViewer_->onPixel=[this](QPoint p){calibrationPixelInfo_->setText(p.x()>=0 ? QString("X:%1 Y:%2 px").arg(p.x()).arg(p.y()) : "Piksel: —");};
    auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setMinimumWidth(420);calibrationControls_=new QWidget;auto* controls=new QVBoxLayout(calibrationControls_);auto* form=new QFormLayout;
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
    controls->addWidget(note("İlk satır öğretim; sonraki en az iki farklı segment bağımsız holdout. Koordinatlar orijinal piksel merkezleridir. Bilinen mm uzunluklarını gerçek referans ölçümünden girin."));
    calibrationSamples_=new QTableWidget(0,6);calibrationSamples_->setObjectName("calibrationSamples");calibrationSamples_->setHorizontalHeaderLabels({"ID","X1 px","Y1 px","X2 px","Y2 px","Known mm"});calibrationSamples_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);calibrationSamples_->setMinimumHeight(170);controls->addWidget(calibrationSamples_,1);
    auto* samples=new QHBoxLayout;auto* add=new QPushButton("Örnek ekle");add->setObjectName("calibrationAddSample");samples->addWidget(add);connect(add,&QPushButton::clicked,this,[this]{if(calibrationSamples_->rowCount()<257){const auto r=calibrationSamples_->rowCount();calibrationSamples_->insertRow(r);for(int c=0;c<6;++c)calibrationSamples_->setItem(r,c,new QTableWidgetItem(c==0 ? QString("sample-%1").arg(r+1) : QString{}));invalidateCalibrationDraft();}});
    auto* remove=new QPushButton("Seçili örneği sil");samples->addWidget(remove);connect(remove,&QPushButton::clicked,this,[this]{if(calibrationSamples_->currentRow()>=0){calibrationSamples_->removeRow(calibrationSamples_->currentRow());invalidateCalibrationDraft();}});controls->addLayout(samples);
    auto* commands=new QGridLayout;int index=0;
    for(const auto& entry:QVector<QPair<QString,QString>>{{"calculate","Hesapla"},{"validate","Doğrula"},{"save","Kaydet ve bağla"},{"load","Kayıtlı kanıtı aç"}}){auto* b=new QPushButton(entry.second);b->setObjectName("calibration_"+entry.first);commands->addWidget(b,index/2,index%2);++index;connect(b,&QPushButton::clicked,this,[this,action=entry.first]{calibrationAction(action);});}controls->addLayout(commands);
    calibrationStatus_=note("Kalibrasyon yok. Önce reçete açın.");calibrationStatus_->setObjectName("calibrationStatus");controls->addWidget(calibrationStatus_);controls->addStretch();scroll->setWidget(calibrationControls_);body->addWidget(scroll,1);layout->addLayout(body,1);
    for(auto* edit:{calibrationId_,calibrationLens_,calibrationPlane_})connect(edit,&QLineEdit::textEdited,this,[this]{invalidateCalibrationDraft();});
    connect(version,&QSpinBox::valueChanged,this,[this]{invalidateCalibrationDraft();});connect(error,&QDoubleSpinBox::valueChanged,this,[this]{invalidateCalibrationDraft();});
    connect(calibrationSamples_,&QTableWidget::cellChanged,this,[this]{invalidateCalibrationDraft();});
    connect(calibrationSamples_,&QTableWidget::currentCellChanged,this,[this]{refreshCalibration();});return page;
}
void MainWindow::invalidateCalibrationDraft(){++calibrationGeneration_;calibrationCandidate_.reset();if(calibrationStatus_)calibrationStatus_->setText("Örnek/ayar değişti — yeniden hesaplama ve doğrulama gerekli.");refreshCalibration();}
void MainWindow::refreshCalibration() {
    if(!calibrationControls_)return;const auto* r=service_.activeRecipe();const auto identity=r ? service_.project().id+"/"+r->id : QString{};
    if(identity!=calibrationRecipeId_) {
        calibrationRecipeId_=identity;++calibrationGeneration_;calibrationCandidate_.reset();calibrationFrameId_.clear();
        const QSignalBlocker table(calibrationSamples_),id(calibrationId_),lens(calibrationLens_),plane(calibrationPlane_);auto* version=findChild<QSpinBox*>("calibrationVersion");const QSignalBlocker vb(version);
        calibrationSamples_->setRowCount(0);calibrationId_->setText(r && r->calibrationBinding ? r->calibrationBinding->calibrationId : "");version->setValue(r && r->calibrationBinding ? std::min(r->calibrationBinding->version+1,1000000000) : 1);
        const auto setup=r ? r->definition.value("planarSetup").toObject() : QJsonObject{};calibrationLens_->setText(setup["lensId"].toString());calibrationPlane_->setText(setup["workingPlaneId"].toString());
        calibrationStatus_->setText(r && r->calibrationBinding ? QString("Bağlı kayıt: %1 v%2. Kare al / Kayıtlı kanıtı aç. Current kontrolü her Run Flow'da yapılır.").arg(r->calibrationBinding->calibrationId).arg(r->calibrationBinding->version) : "Kalibrasyon bağlantısı yok.");
    }
    const auto frameId=editorSession_.frame ? editorSession_.frame->metadata().frameId : QString{};
    if(frameId!=calibrationFrameId_){calibrationFrameId_=frameId;++calibrationGeneration_;calibrationCandidate_.reset();}
    const bool hasFrame=r && editorSession_.frame && !frameId.isEmpty();
    if(hasFrame) {
        calibrationViewer_->setImage(editorSession_.frame->preview());calibrationFrameInfo_->setText(QString("%1×%2 px • edinilmiş kare • seçili örnek: %3").arg(editorSession_.frame->size().width()).arg(editorSession_.frame->size().height()).arg(calibrationSamples_->currentRow()<0 ? "yok" : calibrationSamples_->currentRow()==0 ? "öğretim" : "holdout"));
        const auto row=calibrationSamples_->currentRow();QVector<QPointF> points;
        if(row>=0)for(int c:{1,3}) {
            const auto* x=calibrationSamples_->item(row,c);const auto* y=calibrationSamples_->item(row,c+1);bool xok=false,yok=false;
            double px=x ? x->text().toDouble(&xok) : 0,py=y ? y->text().toDouble(&yok) : 0;
            if(x && !xok)px=QLocale().toDouble(x->text(),&xok);if(y && !yok)py=QLocale().toDouble(y->text(),&yok);
            if(xok && yok && std::isfinite(px) && std::isfinite(py) && px>=0 && py>=0 && px<editorSession_.frame->size().width() && py<editorSession_.frame->size().height())points.append({px,py});
        }
        if(points.size()==2)calibrationViewer_->setToolOverlay({},points);
    }else {calibrationViewer_->clearImage("Kare al ile reçetenin kaynağından görüntü edinin.");calibrationFrameInfo_->setText("Edinilmiş kare yok.");}
    const bool ready=r && !editorSession_.busy();calibrationControls_->setEnabled(ready);findChild<QPushButton*>("calibrationAcquire")->setEnabled(ready);
    findChild<QPushButton*>("calibration_calculate")->setEnabled(ready && hasFrame && calibrationSamples_->rowCount()>=1);
    findChild<QPushButton*>("calibration_validate")->setEnabled(ready && hasFrame && calibrationCandidate_.has_value() && !calibrationCandidate_->validated() && calibrationSamples_->rowCount()>=3);
    findChild<QPushButton*>("calibration_save")->setEnabled(ready && hasFrame && calibrationCandidate_.has_value() && calibrationCandidate_->validated());
    findChild<QPushButton*>("calibration_load")->setEnabled(ready && r->calibrationBinding.has_value());
}
void MainWindow::calibrationAction(const QString& action) {
    if(editorSession_.busy() || !service_.activeRecipe())return;
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
        const auto signature=flowSignature();const auto generation=calibrationGeneration_;auto* watcher=editorSession_.watch<CalibrationWork>(this,SessionJob::Teaching);calibrationStatus_->setText("BUSY — "+action);refreshCalibration();refreshFlow();
        connect(watcher,&QFutureWatcher<CalibrationWork>::finished,this,[this,watcher,signature,generation,action,frame,path]{
            const auto work=watcher->result();watcher->deleteLater();
            if(signature!=flowSignature() || generation!=calibrationGeneration_){calibrationStatus_->setText("Değişen reçete/kare/ayar nedeniyle sonuç uygulanmadı.");refreshCalibration();refreshFlow();return;}
            if(!work.error.isEmpty()){calibrationCandidate_.reset();calibrationStatus_->setText(work.error);refreshCalibration();refreshFlow();return;}
            if(action=="save")try {CalibrationWorkflow::bind(service_,*frame,path,work.value->dependency().calibrationId);calibrationCandidate_.reset();auto* version=findChild<QSpinBox*>("calibrationVersion");{const QSignalBlocker blocker(version);version->setValue(std::min(work.value->dependency().version+1,1000000000));}invalidateMatch();refreshWorkspace();calibrationStatus_->setText("Kalibrasyon kaydedildi ve reçeteye bağlandı. Reçete DRAFT; Save ile reçete bağlantısını kaydedin.");}catch(const std::exception& e){calibrationStatus_->setText("Katalog kaydedildi; reçete bağlanamadı: "+QString::fromUtf8(e.what()));}
            else if(action=="load") {
                const auto json=MeasurementService{}.encode(*work.value);QJsonArray samples;samples.append(json["teaching"]);for(const auto& s:json["holdout"].toArray())samples.append(s);
                const QSignalBlocker blocker(calibrationSamples_);calibrationSamples_->setRowCount(samples.size());const QStringList keys{"id","x1","y1","x2","y2","knownMm"};
                for(int r=0;r<samples.size();++r)for(int c=0;c<keys.size();++c){const QJsonValue v=samples[r].toObject().value(keys[c]);calibrationSamples_->setItem(r,c,new QTableWidgetItem(v.isString() ? v.toString() : QString::number(v.toDouble(),'g',17)));}
                calibrationCandidate_.reset();calibrationStatus_->setText(QString("Kayıtlı v%1 kanıtı açıldı • %2 mm/px • RMS %3 mm. Yeni sürüm için yeniden hesapla/doğrula.").arg(work.value->dependency().version).arg(work.value->millimetresPerPixel(),0,'g',10).arg(work.value->validationRmsMillimetres(),0,'g',10));
            } else {
                calibrationCandidate_=work.value;
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
}
