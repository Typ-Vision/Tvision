#include "MainWindow.hpp"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

namespace visionflow {
namespace {
QLabel* label(const QString& text, const char* name = "muted") {
    auto* result = new QLabel(text); result->setObjectName(name); result->setWordWrap(true);
    result->setTextFormat(Qt::PlainText); return result;
}
QFrame* card(const QString& title, const QString& text) {
    auto* frame = new QFrame; frame->setObjectName("card");
    auto* layout = new QVBoxLayout(frame); layout->setContentsMargins(18,18,18,18);
    layout->addWidget(label(title,"heading")); layout->addWidget(label(text)); layout->addStretch(); return frame;
}
}
MainWindow::MainWindow(const QString& settingsFile) : settingsFile_(settingsFile) {
    smokeMode_=QCoreApplication::arguments().contains("--smoke-test");
    setWindowTitle("VisionFlow Industrial Vision Suite"); resize(1400,900); setMinimumSize(850,600);
    auto* file = menuBar()->addMenu("File");
    auto* create=file->addAction("New Project"); create->setShortcut(QKeySequence::New);
    connect(create,&QAction::triggered,this,[this]{showProjectDialog();});
    auto* open=file->addAction("Open Project"); open->setShortcut(QKeySequence::Open);
    connect(open,&QAction::triggered,this,&MainWindow::openProjectDialog);
    saveAction_=file->addAction("Save Project / Recipes"); saveAction_->setShortcut(QKeySequence::Save);
    connect(saveAction_,&QAction::triggered,this,&MainWindow::saveProject);
    file->addSeparator(); auto* exit = file->addAction("Exit");
    exit->setShortcut(QKeySequence::Quit); connect(exit,&QAction::triggered,this,&QWidget::close);
    auto* view = menuBar()->addMenu("View");
    auto* editor = view->addAction("Recipe Editor");
    connect(editor,&QAction::triggered,this,[this]{selectPage("editor");});
    auto* reset = view->addAction("Panel düzenini sıfırla");
    connect(reset,&QAction::triggered,this,[this]{horizontal_->setSizes({200,700,260}); vertical_->setSizes({550,210});});
    auto* setup = menuBar()->addMenu("Setup");
    for (const auto& id : {QString("communication"), QString("calibration")}) {
        auto* action = setup->addAction(id=="communication" ? "Communication" : "Calibration");
        action->setObjectName("setup_"+id);
        connect(action,&QAction::triggered,this,[this,id]{selectPage(id);});
    }
    auto* root = new QWidget; auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0,0,0,0); rootLayout->setSpacing(0);
    auto* sidebar = new QFrame; sidebar->setObjectName("sidebar"); sidebar->setFixedWidth(215);
    auto* side = new QVBoxLayout(sidebar);
    side->addWidget(label("VF  VisionFlow","brand")); side->addWidget(label("INDUSTRIAL VISION SUITE"));
    navigation_ = new QListWidget; navigation_->setAccessibleName("Ana navigasyon"); side->addWidget(navigation_,1);
    hardwareStatus_=label("Camera: bağlı değil\nRobot / PLC: bağlı değil\nGeliştirme aşaması 6");side->addWidget(hardwareStatus_); rootLayout->addWidget(sidebar);
    auto* content = new QWidget; auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20,16,20,16); pageTitle_ = label("Dashboard","heading");
    contentLayout->addWidget(pageTitle_);
    context_=label(""); contentLayout->addWidget(context_);
    workspaceError_=label(""); workspaceError_->setObjectName("error"); workspaceError_->hide();
    contentLayout->addWidget(workspaceError_);
    pages_ = new QStackedWidget; contentLayout->addWidget(pages_,1); rootLayout->addWidget(content,1); setCentralWidget(root);
    struct Page { const char* id; const char* title; const char* description; };
    const Page specs[] = {
        {"dashboard","Dashboard",""},
        {"projects","Project Manager",""},
        {"recipes","Recipe Manager",""},
        {"camera","Camera Manager","Kamera ve dosya görüntü kaynakları henüz yapılandırılmadı."},
        {"calibration","Calibration","Camera / Pixel / Robot kalibrasyonları sonraki aşamada eklenecek."},
        {"communication","Communication","Robot / PLC protokol servisleri sonraki aşamada eklenecek."},
        {"runtime","Runtime","Doğrulanmış reçete yok. Henüz çevrim çalıştırılmadı."},
        {"diagnostics","Diagnostics","Donanım ve tool tanılama servisleri henüz uygulanmadı."},
        {"logs","Logs","Üretim ve audit kayıtları sonraki aşamada eklenecek."},
        {"users","Users","Oturum ve rol servisi henüz uygulanmadı."},
        {"settings","Settings","Panel düzeni kapanışta kaydedilir. View menüsünden sıfırlanabilir."}
    };
    for (const auto& spec : specs) {
        if (QString(spec.id)=="dashboard" || QString(spec.id)=="camera") {
            auto* group = new QListWidgetItem(QString(spec.id)=="dashboard" ? "WORKSPACE" : "SYSTEM",navigation_);
            group->setFlags(Qt::NoItemFlags); group->setForeground(QColor("#8fa0b5"));
        }
        pageIds_.append(spec.id);
        const QString id(spec.id);
        pages_->addWidget(id=="dashboard" ? makeDashboard() : id=="projects" ? makeProjectManager() :
                          id=="recipes" ? makeRecipeManager() : id=="camera" ? makeCameraManager() : id=="calibration" ? makeCalibration() : id=="communication" ? makeCommunication() : id=="runtime" ? makeRuntime() : id=="logs" ? makeLogs() : makePlaceholder(spec.title,spec.description));
        if(id!="calibration" && id!="communication") {
            auto* item = new QListWidgetItem(spec.title,navigation_); item->setData(Qt::UserRole,id);
        }
    }
    pageIds_.append("editor"); pages_->addWidget(makeEditor());
    connect(navigation_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item){
        if (item && item->data(Qt::UserRole).isValid()) selectPage(item->data(Qt::UserRole).toString());
    });
    connect(navigation_,&QListWidget::itemClicked,this,[this](QListWidgetItem* item){
        if(item && item->data(Qt::UserRole).isValid())selectPage(item->data(Qt::UserRole).toString());
    });
    statusBar()->showMessage("Hazır — UI çatısı. Vision çevrimi çalıştırılmadı.");
    QSettings settings(settingsFile_,QSettings::IniFormat);
    restoreGeometry(settings.value("window/geometry").toByteArray());
    restoreLayout(); refreshWorkspace(); selectPage("dashboard");
    if (!smokeMode_) restoreSession();
    initializeCommunication();
}
QWidget* MainWindow::makePlaceholder(const QString& title, const QString& text) {
    auto* page = new QWidget; auto* layout = new QVBoxLayout(page);
    layout->addWidget(card(title,text)); layout->addStretch();
    return page;
}
QWidget* MainWindow::makeDashboard() {
    auto* page = new QWidget; auto* layout = new QVBoxLayout(page);
    layout->addWidget(card("Çalışma alanına hoş geldiniz","Önce bir proje ve reçete oluşturulacak. Donanım bağlantısı ve üretim sonucu henüz yok."));
    auto* row = new QHBoxLayout; row->addWidget(card("Kamera","Bağlı değil"));
    auto* recipeCard=card("Reçete","Seçilmedi"); dashboardRecipe_=recipeCard->findChild<QLabel*>("muted");
    row->addWidget(recipeCard); row->addWidget(card("Son çevrim","Çalıştırılmadı")); layout->addLayout(row);
    auto* projects = new QPushButton("Project Manager'a git"); projects->setObjectName("primary"); layout->addWidget(projects);
    connect(projects,&QPushButton::clicked,this,[this]{selectPage("projects");}); layout->addStretch(); return page;
}
void MainWindow::selectPage(const QString& id) {
    if(operatorMode_ && id!="runtime")return;
    const int index = pageIds_.indexOf(id); if (index<0) return;
    if (id!="editor" && pages_->currentIndex()==pageIds_.indexOf("editor") && !applyToolProperties()) {
        const QSignalBlocker blocker(navigation_);
        for (int i=0;i<navigation_->count();++i) if (navigation_->item(i)->data(Qt::UserRole).toString()=="recipes") navigation_->setCurrentRow(i);
        return;
    }
    const QString navId = id=="editor" ? "recipes" : id;
    if(id=="calibration" || id=="communication") {
        const QSignalBlocker blocker(navigation_);navigation_->clearSelection();
        pageTitle_->setText(id=="calibration" ? "Calibration" : "Communication");
    }
    for (int i=0;i<navigation_->count();++i) {
        auto* item = navigation_->item(i);
        if (item->data(Qt::UserRole).toString()==navId) {
            if (navigation_->currentItem()!=item) navigation_->setCurrentItem(item);
            pageTitle_->setText(id=="editor" ? "Recipe Editor" : item->text()); break;
        }
    }
    pages_->setCurrentIndex(index);
    if(id=="calibration")refreshCalibration();
    if(id=="runtime")refreshRuntime();
    if(id=="communication")refreshCommunication();
    if(id=="logs")refreshLogs();
}
void MainWindow::saveLayout() {
    QDir().mkpath(QFileInfo(settingsFile_).absolutePath()); QSettings settings(settingsFile_,QSettings::IniFormat);
    settings.setValue("window/geometry",saveGeometry()); settings.setValue("editor/horizontal",horizontal_->saveState());
    settings.setValue("editor/vertical",vertical_->saveState()); settings.sync();
    if (service_.hasProject() && QFileInfo::exists(service_.path())) {
        settings.setValue("session/project",QFileInfo(service_.path()).absoluteFilePath());
        settings.setValue("session/recipe",service_.activeRecipe() ? service_.activeRecipe()->id : QString());
        settings.setValue("session/page",pageIds_.value(pages_->currentIndex(),"dashboard"));
    }
    settings.sync();
}
void MainWindow::restoreSession() {
    QSettings settings(settingsFile_,QSettings::IniFormat);
    const QString path=settings.value("session/project").toString();
    if (path.isEmpty()) return;
    try {
        service_.openProject(path);
        const QString recipe=settings.value("session/recipe").toString();
        for (const auto& item : service_.project().recipes)
            if (item.id==recipe) { service_.selectRecipe(recipe); break; }
        refreshWorkspace();
        const QString page=settings.value("session/page","projects").toString();
        selectPage(page=="editor" && !service_.activeRecipe() ? "recipes" : page);
    } catch (const std::exception& error) {
        displayError("Son proje açılamadı: "+QString::fromUtf8(error.what())+" — Open Project ile başka bir dosya seçebilirsiniz.");
    }
}
bool MainWindow::verifySession() {
    if (!service_.hasProject() || service_.dirty() || !service_.activeRecipe()) return false;
    const QString project=service_.project().id, recipe=service_.activeRecipe()->id;
    selectPage("editor"); saveLayout();
    MainWindow restored(settingsFile_); restored.restoreSession();
    if (!restored.service_.hasProject() || restored.service_.project().id!=project ||
        !restored.service_.activeRecipe() || restored.service_.activeRecipe()->id!=recipe ||
        restored.service_.dirty() || restored.pages_->currentIndex()!=restored.pageIds_.indexOf("editor")) return false;
    QSettings settings(settingsFile_,QSettings::IniFormat);
    const auto path=settings.value("session/project");
    settings.setValue("session/project",settingsFile_+".missing"); settings.sync();
    MainWindow missing(settingsFile_); missing.restoreSession();
    const bool valid=!missing.service_.hasProject() && !missing.workspaceError_->text().isEmpty();
    settings.setValue("session/project",path); settings.sync();
    return valid;
}
void MainWindow::restoreLayout() {
    QSettings settings(settingsFile_,QSettings::IniFormat);
    horizontal_->restoreState(settings.value("editor/horizontal").toByteArray()); vertical_->restoreState(settings.value("editor/vertical").toByteArray());
}
void MainWindow::closeEvent(QCloseEvent* event) {
    if(plc_ && plc_->blocking()){displayError("PLC çevrim kaydı ve ACK tamamlanmadan pencere kapatılamaz.");event->ignore();return;}
    if (editorSession_.busy(SessionJob::Teaching) || editorSession_.busy(SessionJob::Execution) || editorSession_.busy(SessionJob::Image) || cameraSession_.busy() || runtimeJobs_.busy() || (runtime_ && runtime_->busy())) {displayError("Arka plan işi tamamlandıktan sonra pencereyi kapatın."); event->ignore(); return;}
    if (!resolveUnsavedChanges()) { event->ignore(); return; }
    saveLayout(); QMainWindow::closeEvent(event);
}
bool MainWindow::verifyShell() {
    for (const auto& id : pageIds_) { selectPage(id); if (pages_->currentIndex()!=pageIds_.indexOf(id)) return false; }
    selectPage("editor"); horizontal_->setSizes({180,600,240}); vertical_->setSizes({460,180});
    const auto horizontal = horizontal_->sizes(); const auto vertical = vertical_->sizes(); saveLayout();
    horizontal_->setSizes({250,400,300}); vertical_->setSizes({300,300}); restoreLayout();
    QSettings settings(settingsFile_,QSettings::IniFormat);
    return horizontal_->sizes()==horizontal && vertical_->sizes()==vertical && settings.status()==QSettings::NoError;
}
}
