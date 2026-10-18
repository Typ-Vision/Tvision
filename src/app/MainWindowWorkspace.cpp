#include "MainWindow.hpp"
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <exception>

namespace visionflow {
namespace {
QLabel* text(const QString& value) {
    auto* result=new QLabel(value); result->setWordWrap(true); result->setTextFormat(Qt::PlainText); return result;
}
QLineEdit* field(QFormLayout* form, const QString& title, const char* id, const QString& value={}, int max=64) {
    auto* edit=new QLineEdit(value); edit->setObjectName(id); edit->setMaxLength(max);
    form->addRow(title,edit); return edit;
}
QString selectedRecipeId(QTableWidget* table) {
    auto* item=table->item(table->currentRow(),0); return item ? item->data(Qt::UserRole).toString() : QString{};
}
}
QWidget* MainWindow::makeProjectManager() {
    auto* page=new QWidget; auto* layout=new QVBoxLayout(page);
    auto* actions=new QHBoxLayout;
    auto* create=new QPushButton("New Project"); create->setObjectName("newProject");
    auto* open=new QPushButton("Open Project");
    auto* edit=new QPushButton("Proje ayarlarını düzenle"); edit->setObjectName("editProject");
    auto* save=new QPushButton("Save Project"); save->setObjectName("saveProject");
    for (auto* button : {create,open,edit,save}) actions->addWidget(button);
    actions->addStretch(); layout->addLayout(actions);
    connect(create,&QPushButton::clicked,this,[this]{showProjectDialog();});
    connect(open,&QPushButton::clicked,this,&MainWindow::openProjectDialog);
    connect(edit,&QPushButton::clicked,this,[this]{showProjectDialog(true);});
    connect(save,&QPushButton::clicked,this,&MainWindow::saveProject);
    projectSummary_=text("Proje yok."); layout->addWidget(projectSummary_);
    projectTree_=new QTreeWidget; projectTree_->setHeaderLabels({"Proje içeriği","Durum / Tür"});
    projectTree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(projectTree_,1);
    connect(projectTree_,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem* item){
        const auto id=item->data(0,Qt::UserRole).toString(); if (id.isEmpty()) return;
        try { service_.selectRecipe(id); refreshWorkspace(); selectPage("editor"); }
        catch (const std::exception& e) { displayError(QString::fromUtf8(e.what())); }
    });
    return page;
}
QWidget* MainWindow::makeRecipeManager() {
    auto* page=new QWidget; auto* layout=new QVBoxLayout(page);
    layout->addWidget(text("Robot Guidance, Defect Inspection ve Measurement aynı Recipe Editor'ü kullanır."));
    auto* actions=new QHBoxLayout;
    auto* create=new QPushButton("New Recipe"); create->setObjectName("newRecipe");
    auto* open=new QPushButton("Edit Recipe"); open->setObjectName("openRecipe");
    auto* load=new QPushButton("Load Recipe"); load->setObjectName("loadRecipe");
    auto* edit=new QPushButton("Bilgileri düzenle"); edit->setObjectName("editRecipe");
    auto* save=new QPushButton("Save Project / Recipes"); save->setObjectName("saveRecipes");
    actions->addWidget(load);actions->addWidget(open);actions->addStretch();layout->addLayout(actions);
    auto* utilities=new QHBoxLayout;for(auto* button:{create,edit,save})utilities->addWidget(button);
    utilities->addStretch();layout->addLayout(utilities);
    connect(create,&QPushButton::clicked,this,[this]{showRecipeDialog();});
    connect(open,&QPushButton::clicked,this,&MainWindow::openSelectedRecipe);
    connect(load,&QPushButton::clicked,this,&MainWindow::loadSelectedRecipe);
    connect(edit,&QPushButton::clicked,this,[this]{
        if (!service_.hasProject()) return;
        const auto id=selectedRecipeId(recipeTable_); if (id.isEmpty()) return;
        service_.selectRecipe(id); refreshWorkspace(); showRecipeDialog(true);
    });
    connect(save,&QPushButton::clicked,this,&MainWindow::saveProject);
    auto* filters=new QHBoxLayout;
    recipeSearch_=new QLineEdit; recipeSearch_->setObjectName("recipeSearch"); recipeSearch_->setPlaceholderText("Reçete adı veya açıklama ara…");
    typeFilter_=new QComboBox; typeFilter_->setObjectName("typeFilter"); typeFilter_->addItem("All applications","");
    for (auto type : {ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement})
        typeFilter_->addItem(typeLabel(type),typeKey(type));
    filters->addWidget(recipeSearch_,1); filters->addWidget(typeFilter_); layout->addLayout(filters);
    recipeTable_=new QTableWidget(0,7); recipeTable_->setObjectName("recipeTable");
    recipeTable_->setHorizontalHeaderLabels({"Recipe","Application Type","Description","Camera","Version","Status","Modified (UTC)"});
    recipeTable_->setSelectionBehavior(QAbstractItemView::SelectRows); recipeTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    recipeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers); recipeTable_->verticalHeader()->hide();
    recipeTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    recipeTable_->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
    layout->addWidget(recipeTable_,1);
    recipeCount_=text(""); layout->addWidget(recipeCount_);
    connect(recipeTable_,&QTableWidget::itemSelectionChanged,this,[open,edit,load,this]{
        const bool selected=!selectedRecipeId(recipeTable_).isEmpty(); open->setEnabled(selected); edit->setEnabled(selected); load->setEnabled(selected);
    });
    open->setEnabled(false); edit->setEnabled(false); load->setEnabled(false);
    connect(recipeSearch_,&QLineEdit::textChanged,this,[this]{refreshRecipes();});
    connect(typeFilter_,&QComboBox::currentIndexChanged,this,[this]{refreshRecipes();});
    connect(recipeTable_,&QTableWidget::cellDoubleClicked,this,[this]{openSelectedRecipe();});
    return page;
}
void MainWindow::showProjectDialog(bool edit) {
    if (edit && !service_.hasProject()) return;
    QDialog dialog(this); dialog.setObjectName("projectDialog");
    dialog.setWindowTitle(edit ? "Project Settings" : "New Project"); dialog.setMinimumWidth(640);
    auto* layout=new QVBoxLayout(&dialog); auto* form=new QFormLayout;
    const Project p=edit ? service_.project() : Project{};
    auto* name=field(form,"Proje adı","projectName",p.name);
    auto* description=field(form,"Açıklama","projectDescription",p.description,2048);
    auto* location=field(form,"Proje dosyası","projectPath",edit ? service_.path() : "",1024);
    location->setReadOnly(edit); location->setPlaceholderText("Mevcut klasör içinde yeni bir .vfproject dosyası seçin");
    auto* browse=new QPushButton("Dosya konumunu seç…"); browse->setEnabled(!edit); form->addRow("",browse);
    connect(browse,&QPushButton::clicked,&dialog,[&]{
        const auto path=QFileDialog::getSaveFileName(&dialog,"Yeni proje dosyası",QDir::currentPath(),"VisionFlow Project (*.vfproject)",nullptr,QFileDialog::DontConfirmOverwrite);
        if (!path.isEmpty()) location->setText(path.endsWith(".vfproject",Qt::CaseInsensitive) ? path : path+".vfproject");
    });
    auto* camera=field(form,"Varsayılan görüntü kaynağı","projectCamera",edit ? p.cameraReference : "ImageFile");
    auto* robot=field(form,"Robot türü (opsiyonel)","projectRobot",p.robotType);
    auto* protocol=field(form,"Protokol (opsiyonel)","projectProtocol",p.communicationProtocol);
    auto* calibration=field(form,"Kalibrasyon referansı (opsiyonel)","projectCalibration",p.calibrationReference);
    layout->addLayout(form);
    layout->addWidget(text("Kaynak ve donanım alanları yalnız metadata'dır; bağlantı veya kalibrasyon geçerliliği henüz doğrulanmaz."));
    auto* error=text(""); error->setObjectName("error"); layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(edit ? "Apply" : "Create Project");
    buttons->button(QDialogButtonBox::Ok)->setObjectName("acceptProject"); layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        try {
            if (edit) service_.updateProject(name->text(),description->text(),camera->text(),robot->text(),protocol->text(),calibration->text());
            else {
                if (!location->text().endsWith(".vfproject",Qt::CaseInsensitive)) throw std::runtime_error("Proje dosyası .vfproject uzantılı olmalı.");
                // Validate the replacement before asking about current unsaved work.
                ProjectService candidate;
                candidate.createProject(name->text(),description->text(),location->text(),camera->text(),robot->text(),protocol->text(),calibration->text());
                if (!resolveUnsavedChanges()) return;
                service_=std::move(candidate);
            }
            refreshWorkspace(); selectPage("projects"); dialog.accept();
        } catch (const std::exception& e) { error->setText(QString::fromUtf8(e.what())); }
    });
    dialog.exec();
}
void MainWindow::openProjectDialog() {
    const auto path=QFileDialog::getOpenFileName(this,"Open Project",QDir::currentPath(),"VisionFlow Project (*.vfproject)");
    if (path.isEmpty()) return;
    try {
        ProjectService candidate; candidate.openProject(path);
        if (!resolveUnsavedChanges()) return;
        // If Save was chosen for the same path, load the newly saved content.
        candidate.openProject(path); service_=std::move(candidate);
        refreshWorkspace(); selectPage("projects");
    } catch (const std::exception& e) { displayError(QString::fromUtf8(e.what())); }
}
void MainWindow::showRecipeDialog(bool edit) {
    if (!applyToolProperties()) return;
    if (!service_.hasProject()) { displayError("Önce bir proje oluşturun veya açın."); return; }
    const Recipe* active=service_.activeRecipe(); if (edit && !active) return;
    const Recipe original=edit ? *active : Recipe{};
    QDialog dialog(this); dialog.setObjectName("recipeDialog"); dialog.setWindowTitle(edit ? "Edit Recipe" : "New Recipe"); dialog.setMinimumWidth(820);
    auto* layout=new QVBoxLayout(&dialog); auto* cards=new QHBoxLayout; auto* group=new QButtonGroup(&dialog);
    const QStringList summaries={"Parçayı bul → Pick Reference → Robot Transform → Output",
        "Reference → Inspection ROI → Defect → Result","Features → Calculate → Limits → Result"};
    int i=0;
    for (auto type : {ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement}) {
        auto* box=new QGroupBox; auto* content=new QVBoxLayout(box);
        auto* radio=new QRadioButton(typeLabel(type)); radio->setObjectName(typeKey(type)); group->addButton(radio,i);
        radio->setChecked(edit ? original.applicationType==type : i==0); radio->setEnabled(!edit);
        box->setProperty("selected",radio->isChecked());
        connect(radio,&QRadioButton::toggled,box,[box](bool checked){
            box->setProperty("selected",checked); box->style()->unpolish(box); box->style()->polish(box); box->update();
        });
        content->addWidget(radio); content->addWidget(text(summaries[i])); cards->addWidget(box,1); ++i;
    }
    layout->addLayout(cards); auto* form=new QFormLayout;
    auto* name=field(form,"Reçete adı","recipeName",original.name);
    auto* description=field(form,"Açıklama","recipeDescription",original.description,2048);
    auto* camera=field(form,"Görüntü kaynağı referansı","recipeCamera",edit ? original.cameraReference : service_.project().cameraReference);
    auto* calibration=field(form,"Kalibrasyon referansı","recipeCalibration",edit ? original.calibrationReference : service_.project().calibrationReference);
    layout->addLayout(form);
    layout->addWidget(text(edit ? "Tür değişikliği flow uyumluluğu doğrulamasıyla daha sonra eklenecek. Değişiklikler reçeteyi DRAFT yapar." :
        "Reçete DRAFT oluşturulur. Başlangıç akışı burada açıklamadır; tool'lar sonraki adımlarda uygulanacak."));
    auto* error=text(""); error->setObjectName("error"); layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(edit ? "Apply && Open Editor" : "Create && Open Editor");
    buttons->button(QDialogButtonBox::Ok)->setObjectName("acceptRecipe"); layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        try {
            if (edit) service_.editRecipe(original.id,name->text(),description->text(),camera->text(),calibration->text());
            else service_.createRecipe(name->text(),description->text(),static_cast<ApplicationType>(group->checkedId()),camera->text(),calibration->text());
            refreshWorkspace(); selectPage("editor"); dialog.accept();
        } catch (const std::exception& e) { error->setText(QString::fromUtf8(e.what())); }
    });
    dialog.exec();
}
void MainWindow::openSelectedRecipe() {
    const auto id=selectedRecipeId(recipeTable_); if (id.isEmpty()) return;
    try { service_.selectRecipe(id); refreshWorkspace(); selectPage("editor"); }
    catch (const std::exception& e) { displayError(QString::fromUtf8(e.what())); }
}
void MainWindow::loadSelectedRecipe() {
    const auto id=selectedRecipeId(recipeTable_); if(id.isEmpty())return;
    try {
        if(!applyToolProperties())return;
        ensureRuntime();
        if((plc_ && plc_->blocking()) || runtimeJobs_.busy() || runtime_->busy() ||
           runtimeCoordinator_->state()==RuntimeState::Running || runtimeCoordinator_->state()==RuntimeState::Starting || runtimeCoordinator_->state()==RuntimeState::Stopping)
            throw std::runtime_error("Önce Stop ve bekleyen kayıt/teslimi tamamlayın; açık PLC oturumunu durdurun.");
        if(service_.dirty()) {
            QMessageBox choice(QMessageBox::Question,"Load Recipe","Kaydedilmemiş değişiklikler var. Runtime hangi sürümü yüklesin?",QMessageBox::NoButton,this);
            choice.setObjectName("loadRecipeChoice");
            auto* save=choice.addButton("Kaydet ve yükle",QMessageBox::AcceptRole);save->setObjectName("saveAndLoad");
            auto* saved=choice.addButton("Kaydedilmiş sürümü yükle",QMessageBox::ActionRole);saved->setObjectName("loadSaved");
            auto* cancel=choice.addButton("İptal",QMessageBox::RejectRole);cancel->setObjectName("cancelLoad");
            choice.setDefaultButton(save);choice.setEscapeButton(cancel);choice.exec();
            if(choice.clickedButton()==save){service_.save();refreshWorkspace();}
            else if(choice.clickedButton()!=saved)return;
        }
        ProjectService saved;saved.openProject(service_.path());saved.selectRecipe(id);
        const auto options=saved.activeRecipe()->definition["runtime"].toObject();
        runtimeCoordinator_->load(*saved.activeRecipe(),QFileInfo(service_.path()).absolutePath(),options["trigger"]=="SoftwareTrigger" ? RuntimeTrigger::SoftwareTrigger : RuntimeTrigger::Manual);
        if(plc_)plc_->setMapping(options["measurementToolId"].toString());
        runtimeResult_.reset();showRuntimeResult();refreshRecipes();refreshRuntime();selectPage("runtime");
    } catch(const std::exception& e) {
        const auto previous=runtimeCoordinator_ && runtimeCoordinator_->loaded() ? runtimeCoordinator_->loaded()->name+QString(" v%1").arg(runtimeCoordinator_->loaded()->recipeVersion) : QString("yok");
        const auto message="Load başarısız: "+QString::fromUtf8(e.what())+"\nKorunan yüklü reçete: "+previous;
        displayError(message);runtimeSummary_->setText(message);refreshRuntime();
    }
}
void MainWindow::refreshRecipes() {
    QString previous=selectedRecipeId(recipeTable_);
    if (previous.isEmpty() && service_.activeRecipe()) previous=service_.activeRecipe()->id;
    recipeTable_->setRowCount(0);
    if (!service_.hasProject()) { recipeCount_->setText("Önce Project Manager'dan bir proje oluşturun veya açın."); return; }
    const auto query=recipeSearch_->text().trimmed(); const auto filter=typeFilter_->currentData().toString();
    for (const auto& r : service_.project().recipes) {
        if (!filter.isEmpty() && typeKey(r.applicationType)!=filter) continue;
        if (!query.isEmpty() && !r.name.contains(query,Qt::CaseInsensitive) && !r.description.contains(query,Qt::CaseInsensitive)) continue;
        const int row=recipeTable_->rowCount(); recipeTable_->insertRow(row);
        const bool dirty=service_.recipeDirty(r.id);
        const QStringList columns={r.name,typeLabel(r.applicationType),r.description,r.cameraReference,
            "v"+QString::number(r.recipeVersion)+(dirty ? " *" : ""),statusKey(r.status),r.modifiedAt.toString(Qt::ISODateWithMs)};
        for (int column=0;column<columns.size();++column) {
            auto* item=new QTableWidgetItem(columns[column]);
            if (column==0) item->setData(Qt::UserRole,r.id);
            item->setToolTip(dirty ? "Kaydedilmemiş değişiklikler var. Başarılı kayıtta sürüm güncellenir." : columns[column]);
            recipeTable_->setItem(row,column,item);
        }
        if (r.id==previous) recipeTable_->selectRow(row);
    }
    recipeCount_->setText(QString("%1 / %2 reçete%3").arg(recipeTable_->rowCount()).arg(service_.project().recipes.size())
        .arg(service_.project().recipes.isEmpty() ? " — New Recipe ile başlayın." : ""));
    const auto loaded=runtimePath_==service_.path() && runtimeCoordinator_ && runtimeCoordinator_->loaded() ? runtimeCoordinator_->loaded()->name+QString(" v%1").arg(runtimeCoordinator_->loaded()->recipeVersion) : QString("yok");
    recipeCount_->setText(recipeCount_->text()+"\nEditörde: "+(service_.activeRecipe() ? service_.activeRecipe()->name : QString("yok"))+" | Runtime yüklü: "+loaded);
}
void MainWindow::refreshWorkspace() {
    workspaceError_->hide(); saveAction_->setEnabled(service_.hasProject() && service_.dirty());
    statusBar()->showMessage("Hazır — Tool yürütmesi: Run Flow / Results.");
    const bool has=service_.hasProject(); const auto* active=service_.activeRecipe();
    const QString dirty=service_.dirty() ? " • Kaydedilmedi" : "";
    context_->setText(QString("Proje: %1%2   |   Reçete: %3   |   Sistem: yapılandırılmadı")
        .arg(has ? service_.project().name : "seçilmedi",dirty,active ? active->name+" / "+typeLabel(active->applicationType) : "seçilmedi"));
    setWindowTitle("VisionFlow"+(has ? " — "+service_.project().name : " Industrial Vision Suite")+(service_.dirty() ? " *" : ""));
    projectTree_->clear();
    if (has) {
        const auto& p=service_.project();
        projectSummary_->setText(p.name+dirty+"\n"+p.description+"\n"+service_.path());
        auto* cameras=new QTreeWidgetItem(projectTree_,{"Cameras","Metadata; bağlantı yok"});
        new QTreeWidgetItem(cameras,{p.cameraReference,"Doğrulanmadı"});
        auto* recipes=new QTreeWidgetItem(projectTree_,{"Recipes",QString::number(p.recipes.size())});
        for (const auto& r : p.recipes) {
            auto* item=new QTreeWidgetItem(recipes,{r.name,typeLabel(r.applicationType)+" / "+statusKey(r.status)});
            item->setData(0,Qt::UserRole,r.id);
        }
        new QTreeWidgetItem(projectTree_,{"Calibration",p.calibrationReference.isEmpty() ? "Seçilmedi" : p.calibrationReference+" / Doğrulanmadı"});
        new QTreeWidgetItem(projectTree_,{"Communication",p.communicationProtocol.isEmpty() ? "Yapılandırılmadı" : p.communicationProtocol+" / Bağlı değil"});
        new QTreeWidgetItem(projectTree_,{"Variables","Henüz yok"}); new QTreeWidgetItem(projectTree_,{"Logs","Henüz yok"});
        projectTree_->expandAll();
    } else projectSummary_->setText("Proje yok. New Project veya Open Project ile başlayın.");
    if(auto* name=findChild<QLabel*>("editorRecipeName"))name->setText(active ? active->name:"Recipe Editor");
    editorContext_->setText(active ? QString("%1%2 · %3 · v%4")
        .arg(statusKey(active->status),service_.recipeDirty(active->id) ? " • Kaydedilmedi":"",typeLabel(active->applicationType),QString::number(active->recipeVersion)) : "Aktif reçete yok");
    editorContext_->setToolTip(context_->text()+(active ? "\nKaynak: "+active->cameraReference:QString{}));
    dashboardRecipe_->setText(active ? active->name+" / "+typeLabel(active->applicationType)+" / "+statusKey(active->status) : "Seçilmedi");
    for (const auto& id : {"newRecipe","saveProject","saveRecipes","editProject"})
        if (auto* button=findChild<QPushButton*>(id)) button->setEnabled(has);
    if (auto* button=findChild<QPushButton*>("saveWorkspace")) button->setEnabled(has && service_.dirty());
    if (auto* button=findChild<QPushButton*>("editActiveRecipe")) button->setEnabled(active!=nullptr);
    refreshRecipes();
    refreshEditor();
    refreshCalibration();
    refreshRuntime();
}
void MainWindow::saveProject() {
    if (!applyToolProperties()) return;
    try {
        service_.save(); refreshWorkspace(); statusBar()->showMessage("Proje ve reçeteler atomik olarak kaydedildi.",5000);
    } catch (const std::exception& e) { displayError(QString::fromUtf8(e.what())); }
}
void MainWindow::displayError(const QString& message) {
    workspaceError_->setText(message); workspaceError_->show(); statusBar()->showMessage(message);
}
bool MainWindow::resolveUnsavedChanges() {
    if(runtimeCoordinator_ && runtimeCoordinator_->state()==RuntimeState::Running){displayError("Proje değiştirmeden/kapatmadan önce Runtime Stop seçin.");return false;}
    if(plc_ && plc_->blocking()){displayError("Önce PLC kayıt/ACK tamamlanmalı.");return false;}
    if (!applyToolProperties()) return false;
    if (!service_.dirty()) return true;
    if (smokeMode_) return false; // Test failures must not block on a modal prompt.
    const auto choice=QMessageBox::warning(this,"Kaydedilmemiş değişiklikler",
        "Proje veya reçete değişikliklerini kaydetmek ister misiniz?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
    if (choice==QMessageBox::Cancel) return false;
    if (choice==QMessageBox::Discard) { service_.discardChanges(); refreshWorkspace(); return true; }
    saveProject(); return !service_.dirty();
}
bool MainWindow::verifyWorkspace(const QString& artifacts) {
    // Drive the actual dialogs and controls, using isolated files under build/.
    const QString path=QDir(artifacts).filePath("ui-workspace.vfproject");
    if (QFileInfo::exists(path)) QFile::remove(path);
    bool dialogCompleted=false;
    QTimer::singleShot(0,this,[&]{
        auto* dialog=findChild<QDialog*>("projectDialog"); if (!dialog) return;
        dialog->findChild<QLineEdit*>("projectName")->setText("Test Projesi");
        dialog->findChild<QLineEdit*>("projectPath")->setText(path);
        dialog->grab().save(QDir(artifacts).filePath("step2-new-project.png"));
        dialog->findChild<QPushButton*>("acceptProject")->click();
        dialogCompleted=service_.hasProject(); if (!dialogCompleted) dialog->reject();
    });
    findChild<QPushButton*>("newProject")->click();
    if (!dialogCompleted || !service_.dirty()) return false;
    findChild<QPushButton*>("saveProject")->click(); if (service_.dirty()) return false;
    QStringList ids;
    for (int type=0;type<3;++type) {
        dialogCompleted=false;
        QTimer::singleShot(0,this,[&,type]{
            auto* dialog=findChild<QDialog*>("recipeDialog"); if (!dialog) return;
            dialog->findChild<QLineEdit*>("recipeName")->setText("Part_"+QString::number(type+1));
            dialog->findChild<QRadioButton*>(typeKey(static_cast<ApplicationType>(type)))->click();
            if (type==2) dialog->grab().save(QDir(artifacts).filePath("step2-new-recipe.png"));
            dialog->findChild<QPushButton*>("acceptRecipe")->click();
            dialogCompleted=service_.activeRecipe() && service_.activeRecipe()->applicationType==static_cast<ApplicationType>(type);
            if (!dialogCompleted) dialog->reject();
        });
        findChild<QPushButton*>("newRecipe")->click();
        if (!dialogCompleted || !editorContext_->text().contains("Kaydedilmedi")) return false;
        ids.append(service_.activeRecipe()->id);
    }
    findChild<QPushButton*>("saveWorkspace")->click(); if (service_.dirty()) return false;
    for (int index=1;index<=3;++index) {
        typeFilter_->setCurrentIndex(index); if (recipeTable_->rowCount()!=1) return false;
    }
    typeFilter_->setCurrentIndex(0); recipeSearch_->setText("part_2");
    if (recipeTable_->rowCount()!=1) return false;
    recipeSearch_->clear(); selectPage("recipes"); QApplication::processEvents();
    grab().save(QDir(artifacts).filePath("step2-recipes.png"));
    resize(850,600);QApplication::processEvents();if(width()!=850 || height()!=600)return false;grab().save(QDir(artifacts).filePath("p2-recipes-small.png"));resize(1400,900);
    service_.openProject(path); refreshWorkspace(); if (service_.project().recipes.size()!=3) return false;
    recipeTable_->selectRow(1); findChild<QPushButton*>("openRecipe")->click();
    if (!service_.activeRecipe() || service_.activeRecipe()->id!=ids[1]) return false;
    dialogCompleted=false;
    QTimer::singleShot(0,this,[&]{
        auto* dialog=findChild<QDialog*>("recipeDialog"); if (!dialog) return;
        dialog->findChild<QLineEdit*>("recipeDescription")->setText("Güncellenmiş açıklama");
        dialog->findChild<QPushButton*>("acceptRecipe")->click();
        dialogCompleted=service_.dirty(); if (!dialogCompleted) dialog->reject();
    });
    findChild<QPushButton*>("editActiveRecipe")->click(); if (!dialogCompleted) return false;
    findChild<QPushButton*>("saveWorkspace")->click(); if (service_.dirty()) return false;
    service_.openProject(path); service_.selectRecipe(ids[1]); refreshWorkspace(); selectPage("editor"); QApplication::processEvents();
    const auto* recipe=service_.activeRecipe();
    grab().save(QDir(artifacts).filePath("step2-editor.png"));
    return recipe && recipe->recipeVersion==2 && recipe->status==RecipeStatus::Draft && recipe->description=="Güncellenmiş açıklama";
}
}
