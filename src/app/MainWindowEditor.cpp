#include "application/EditorObservation.hpp"
#include "application/CalibrationWorkflow.hpp"
#include "application/ExperimentalOecsAcquisition.hpp"
#include "application/CircleToolService.hpp"
#include "MainWindow.hpp"
#include "PoseMatchEditor.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "ImageViewer.hpp"
#include "RoiEditor.hpp"
#include "PreprocessEditor.hpp"
#include "CodeReadEditor.hpp"
#include "EdgePointEditor.hpp"
#include "PropertySections.hpp"
#include "MetrologyEditor.hpp"
#include "Toolbox.hpp"
#include "ElidedLabel.hpp"
#include "domain/GeometryMeasurementConfig.hpp"
#include "EdgeProfileView.hpp"
#include "application/BackendCapabilities.hpp"
#include "domain/IndustrialSourceReference.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QScopedValueRollback>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QShortcut>
#include <QSettings>
#include <QTimer>
#include <QToolButton>
#include <QAction>
#include <QJsonDocument>
#include <exception>
namespace visionflow {
namespace {
QLabel* note(const QString& value) { auto* result=new QLabel(value); result->setWordWrap(true); result->setTextFormat(Qt::PlainText); result->setObjectName("muted"); return result; }
}
QWidget* MainWindow::makeEditor() {
    auto* page=new QWidget;page->setObjectName("editorPage");
    page->setStyleSheet(
        "QWidget#editorPage QPushButton {padding: 4px 6px; border-radius: 4px;}"
        "QWidget#editorPage QToolButton {background: #151d27; border: 1px solid #2b394a; padding: 4px 6px; border-radius: 4px;}"
        "QWidget#editorPage QLineEdit, QWidget#editorPage QComboBox {padding: 4px;}"
        "QWidget#editorPage QListWidget::item {padding: 5px 3px; margin: 1px; border-radius: 3px;}"
        "QWidget#editorPage QTabBar::tab {padding: 5px 9px;}"
        "QWidget#editorPage QCheckBox::indicator {width: 12px; height: 12px; border: 1px solid #62768f; border-radius: 2px; background: #111720;}"
        "QWidget#editorPage QCheckBox::indicator:checked {background: #66e3c4; border-color: #66e3c4;}");
    auto* layout=new QVBoxLayout(page); layout->setContentsMargins(0,0,0,0);layout->setSpacing(4);
    auto* heading=new QHBoxLayout;heading->setSpacing(8);
    auto* recipeName=new ElidedLabel;recipeName->setObjectName("editorRecipeName");
    recipeName->setStyleSheet("font-weight: 600; color: #edf4fb; font-size: 14px;");heading->addWidget(recipeName,1);
    editorContext_=note("");editorContext_->setWordWrap(false);heading->addWidget(editorContext_);layout->addLayout(heading);
    auto* commands=new QHBoxLayout;
    commands->setSpacing(4);
    auto* back=new QPushButton("Recipes");back->setToolTip("Recipe Manager'a dön");commands->addWidget(back); connect(back,&QPushButton::clicked,this,[this]{selectPage("recipes");});
    auto* details=new QWidget;details->setObjectName("editorDetails");auto* detailLayout=new QHBoxLayout(details);detailLayout->setContentsMargins(0,0,0,0);
    auto* edit=new QPushButton("Reçete bilgileri"); edit->setObjectName("editActiveRecipe"); detailLayout->addWidget(edit);
    connect(edit,&QPushButton::clicked,this,[this]{showRecipeDialog(true);});
    // Mouse commands retain the editing control: Save/Add commit it themselves,
    // avoiding an editingFinished refresh followed by the command refresh.
    auto* save=new QPushButton("Save"); save->setObjectName("saveWorkspace"); save->setFocusPolicy(Qt::TabFocus); commands->addWidget(save); connect(save,&QPushButton::clicked,this,&MainWindow::saveProject);
    auto* multi=new QPushButton("Çok kaynaklı Editor");multi->setObjectName("enableMultiSource");multi->setToolTip("Reçeteyi açıkça editor9 şemasına geçirir; mevcut bağlantılar korunur.");detailLayout->addWidget(multi);detailLayout->addStretch();
    connect(multi,&QPushButton::clicked,this,[this]{if(editorSession_.busy() || !applyToolProperties())return;try{service_.enableMultiSource();editorSession_.sourceFrames.clear();editorSession_.frame.reset();editorSession_.invalidateResults();refreshWorkspace();}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}});
    auto* runFlowButton=new QPushButton("Run Flow"); runFlowButton->setObjectName("runFlow"); commands->addWidget(runFlowButton); connect(runFlowButton,&QPushButton::clicked,this,&MainWindow::runFlow);
    runFlowButton->setToolTip("Yeni görüntü edin ve tam akışı çalıştır.");
    auto* runTool=new QPushButton("Run Tool");runTool->setObjectName("runTool");commands->addWidget(runTool);
    connect(runTool,&QPushButton::clicked,this,[this]{for(const auto& t:service_.tools())if(t.id==selectedToolId_){runExecution(t.id);return;}});
    auto* annotations=new QCheckBox("Açıklamaları göster");annotations->setObjectName("showResultAnnotations");
    annotations->setToolTip("Sonuç etiketlerini gösterir; geometri ve Results tablosu görünür kalır.");
    annotations->setChecked(QSettings(settingsFile_,QSettings::IniFormat).value("editor/showResultAnnotations",false).toBool());
    connect(annotations,&QCheckBox::toggled,this,[this](bool visible){imageViewer_->setResultAnnotationsVisible(visible);QSettings(settingsFile_,QSettings::IniFormat).setValue("editor/showResultAnnotations",visible);});
    auto* stop=new QPushButton("Stop");stop->setObjectName("stopFlow");commands->addWidget(stop);connect(stop,&QPushButton::clicked,this,[this]{pendingDrawTool_.clear();editorCameraPendingExecution_.reset();imageViewer_->cancelRoi();imageViewer_->drawRegion(false);editorSession_.stop(SessionJob::Execution);editorSession_.stop(SessionJob::Teaching);editorCameras_.stop();refreshFlow();});
    commands->addWidget(annotations);commands->addStretch();
    auto* more=new QPushButton("...");more->setObjectName("editorMore");more->setCheckable(true);more->setAccessibleName("Reçete ayrıntıları");more->setToolTip("Reçete bilgileri ve çok kaynaklı Editor");commands->addWidget(more);
    connect(more,&QPushButton::toggled,details,&QWidget::setVisible);layout->addLayout(commands);layout->addWidget(details);details->hide();
    for(auto* button:{back,save,runFlowButton,runTool,stop}){button->ensurePolished();button->setFixedWidth(button->fontMetrics().horizontalAdvance(button->text())+20);}
    runTool->setFixedWidth(qMax(runTool->width(),runTool->fontMetrics().horizontalAdvance("Görüntü al")+20));
    more->setFixedWidth(28);annotations->ensurePolished();annotations->setMinimumWidth(annotations->sizeHint().width());
    horizontal_=new QSplitter(Qt::Horizontal); horizontal_->setObjectName("editorHorizontal"); horizontal_->setChildrenCollapsible(false);
    auto* toolsPanel=new QWidget; toolsPanel->setMinimumWidth(150); auto* toolsLayout=new QVBoxLayout(toolsPanel);toolsLayout->setContentsMargins(4,4,4,4);toolsLayout->setSpacing(4);
    toolsLayout->addWidget(note("TOOLBOX")); auto* search=new QLineEdit; search->setPlaceholderText("Tool ara…"); toolsLayout->addWidget(search);
    auto* catalog=new Toolbox(settingsFile_);toolbox_=catalog;toolsLayout->addWidget(toolbox_,1);
    toolbox_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);toolbox_->setTextElideMode(Qt::ElideRight);
    for(int i=0;i<toolbox_->count();++i)if(!toolbox_->item(i)->data(Toolbox::HeaderRole).toBool())toolbox_->item(i)->setToolTip(toolbox_->item(i)->text()+"\n"+toolbox_->item(i)->toolTip());
    search->setObjectName("toolboxSearch");
    auto* add=new QPushButton("Add Tool");add->setObjectName("addTool");add->setFocusPolicy(Qt::TabFocus);toolsLayout->addWidget(add);
    auto updateAdd=[this,add]{const auto* item=toolbox_->currentItem();add->setEnabled(item && !item->isHidden() && (item->flags() & Qt::ItemIsEnabled) && ToolCatalog::canAdd(item->data(Qt::UserRole).toString()));};
    catalog->onAvailabilityChanged=updateAdd;catalog->onAddRequested=[this]{addSelectedTool();};
    connect(toolbox_,&QListWidget::currentItemChanged,this,[updateAdd]{updateAdd();});
    connect(add,&QPushButton::clicked,this,&MainWindow::addSelectedTool);
    connect(search,&QLineEdit::textChanged,this,[catalog](const QString& query){catalog->filter(query);});updateAdd();
    horizontal_->addWidget(toolsPanel);
    auto* imagePanel=new QWidget; auto* imageLayout=new QVBoxLayout(imagePanel); imageLayout->setContentsMargins(0,0,0,0);
    imageLayout->setSpacing(3);auto* toolbar=new QHBoxLayout;toolbar->setSpacing(3);
    auto* open=new QPushButton("Open");open->setToolTip("Open Image — görüntü dosyası seç"); open->setObjectName("openImage"); toolbar->addWidget(open); connect(open,&QPushButton::clicked,this,&MainWindow::openImageDialog);
    auto* sourcePicker=new QComboBox;sourcePicker->setObjectName("editorSourceSelector");sourcePicker->setAccessibleName("Görüntü kaynağı");sourcePicker->setMinimumWidth(70);sourcePicker->setMinimumContentsLength(4);sourcePicker->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);toolbar->addWidget(sourcePicker,1);
    connect(sourcePicker,&QComboBox::activated,this,[this,sourcePicker](int){selectTool(sourcePicker->currentData().toString());});
    auto* viewerMore=new QToolButton;viewerMore->setText("...");viewerMore->setObjectName("viewerMore");viewerMore->setToolTip("Görüntü: Reload, zoom ve geçici geometri");viewerMore->setAccessibleName("Görüntü komutları");viewerMore->setPopupMode(QToolButton::InstantPopup);
    auto* viewerMenu=new QMenu(viewerMore);viewerMore->setMenu(viewerMenu);
    auto* reload=viewerMenu->addAction("Reload"); reload->setObjectName("reloadImage");
    connect(viewerMenu,&QMenu::aboutToShow,this,[this,viewerMenu,reload]{QTimer::singleShot(0,this,[viewerMenu,reload]{viewerMenu->setActiveAction(reload);});});
    connect(reload,&QAction::triggered,this,[this]{if(service_.activeRecipe() && service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9){const auto source=editorSession_.sourceForTool(editorContext(),selectedToolId_);if(!source.isEmpty())runExecution(source);}else {imageBinding_.clear();refreshWorkspace();}});
    imageViewer_=new ImageViewer; imageViewer_->setObjectName("imageViewer");imageViewer_->setMinimumHeight(80);
    imageViewer_->setResultAnnotationsVisible(annotations->isChecked());
    auto* fit=new QPushButton("Fit"); fit->setObjectName("fitImage"); toolbar->addWidget(fit); connect(fit,&QPushButton::clicked,this,[this]{imageViewer_->fitImage();});
    auto* minus=viewerMenu->addAction("Zoom −"); minus->setObjectName("zoomOut");connect(minus,&QAction::triggered,this,[this]{imageViewer_->zoomBy(1.0/1.25);});
    auto* plus=viewerMenu->addAction("Zoom +"); plus->setObjectName("zoomIn");connect(plus,&QAction::triggered,this,[this]{imageViewer_->zoomBy(1.25);});
    auto* actual=viewerMenu->addAction("1:1");connect(actual,&QAction::triggered,this,[this]{imageViewer_->actualSize();});
    auto* geometry=viewerMenu->addAction("Geçici geometri denemesi…");geometry->setObjectName("roiGeometry");geometry->setToolTip("Reçeteye kaydedilmez. İşlem alanı için PreProcess → ROI ekleyin.");
    connect(geometry,&QAction::triggered,this,[this]{if(editorSession_.frame){showRoiEditor(imageViewer_,this,editorSession_.frame->metadata().coordinateSystemId.isEmpty() ? "viewer/raw":editorSession_.frame->metadata().coordinateSystemId);if(auto* dialog=findChild<QDialog*>("roiGeometryDialog"))if(!dialog->property("restoreRecipeRoi").toBool()){dialog->setProperty("restoreRecipeRoi",true);connect(dialog,&QDialog::finished,this,[this]{refreshMatchOverlay();});}}});
    toolbar->addWidget(viewerMore);imageLayout->addLayout(toolbar);
    imageInfo_=new ElidedLabel;imageInfo_->setObjectName("muted");imageInfo_->setText("Görüntü yok."); imageLayout->addWidget(imageInfo_); imageLayout->addWidget(imageViewer_,1);
    auto* footer=new QHBoxLayout; pixelInfo_=note("Piksel: —"); zoomInfo_=note("Zoom: —"); footer->addWidget(pixelInfo_,1); footer->addWidget(zoomInfo_); imageLayout->addLayout(footer);
    imageViewer_->onZoom=[this](double zoom){zoomInfo_->setText(QString("Zoom: %1%").arg(zoom*100,0,'f',1)); pixelInfo_->setText("Piksel: —");};
    imageViewer_->onPixel=[this](QPoint pixel){const ImageFrame* displayed=drawingImage_ ? &*drawingImage_:editorSession_.frame ? &*editorSession_.frame:nullptr;if(editorSession_.cycle)for(const auto& r:editorSession_.cycle->tools)if(r.toolId==selectedToolId_){auto it=r.outputs.constFind("image");if(it!=r.outputs.cend() && it->valid)if(const auto* image=std::get_if<ImageFrame>(&it->data))displayed=image;}pixelInfo_->setText(displayed && pixel.x()>=0 ? QString("X:%1 Y:%2 px  |  %3").arg(pixel.x()).arg(pixel.y()).arg(displayed->pixelText(pixel.x(),pixel.y())) : "Piksel: —");};
    imageViewer_->onImageClicked={};
    horizontal_->addWidget(imagePanel);
    auto* scroll=new QScrollArea; scroll->setObjectName("propertiesScroll"); scroll->setWidgetResizable(true); scroll->setMinimumWidth(210);
    propertiesBody_=new QWidget; new QVBoxLayout(propertiesBody_); scroll->setWidget(propertiesBody_); horizontal_->addWidget(scroll);
    horizontal_->setStretchFactor(1,1);
    auto* tabs=new QTabWidget; tabs->setObjectName("editorTabs"); tabs->setMinimumHeight(100);
    auto* flowPage=new QWidget; auto* flowLayout=new QHBoxLayout(flowPage); flowLayout->setContentsMargins(4,4,4,4);
    auto* graphPanel=new QWidget;auto* graphLayout=new QVBoxLayout(graphPanel);graphLayout->setContentsMargins(0,0,0,0);flowLayout->addWidget(graphPanel,1);
    graphInfo_=note("READY");auto* graphScroll=new QScrollArea;graphScroll->setWidgetResizable(true);graphScroll->setWidget(graphInfo_);graphLayout->addWidget(graphScroll,1);
    auto* validate=new QPushButton("Validate DAG");validate->setObjectName("validateGraph");graphLayout->addWidget(validate);connect(validate,&QPushButton::clicked,this,&MainWindow::refreshFlow);
    flowTools_=new QListWidget; flowTools_->setObjectName("flowTools"); flowTools_->setAccessibleName("Reçete tool listesi"); flowLayout->addWidget(flowTools_,1);
    flowTools_->setToolTip("Sağ tık: bir sıra yukarı/aşağı. Yürütme sırasını DAG bağlantıları belirler.");
    flowTools_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(flowTools_,&QListWidget::customContextMenuRequested,this,&MainWindow::showFlowMenu);
    connect(flowTools_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item){selectTool(item ? item->data(Qt::UserRole).toString() : "");});
    tabs->addTab(flowPage,"Flow");
    matchInfo_=note("Eşleştirme sonucu yok.");matchInfo_->setAlignment(Qt::AlignTop | Qt::AlignLeft);auto* results=new QScrollArea;results->setObjectName("resultsScroll");results->setWidgetResizable(true);results->setWidget(matchInfo_);tabs->addTab(results,"Results");
    edgeProfile_=new EdgeProfileView;edgeProfile_->setObjectName("edgeProfile");auto* profileScroll=new QScrollArea;profileScroll->setWidgetResizable(true);profileScroll->setWidget(edgeProfile_);tabs->addTab(profileScroll,"Kenar profili");
    imageViewer_->onRegion=[this](QRect roi){commitRoi(roi);};
    imageViewer_->onSearchRegionStarting=[this]{return !editorSession_.busy() && applyToolProperties();};
    imageViewer_->onSearchRegionActivityChanged=[this]{findChild<QPushButton*>("stopFlow")->setEnabled(editorSession_.busy() || !pendingDrawTool_.isEmpty() || imageViewer_->drawingActive());};
    for (const auto name : {"Log","Variables","Communication","Diagnostics"}) tabs->addTab(note(QString(name)+" — henüz veri yok."),name);
    vertical_=new QSplitter(Qt::Vertical); vertical_->setObjectName("editorVertical"); vertical_->setChildrenCollapsible(false);
    vertical_->addWidget(horizontal_); vertical_->addWidget(tabs); vertical_->setStretchFactor(0,1); layout->addWidget(vertical_,1);
    horizontal_->setSizes({170,780,280}); vertical_->setSizes({640,160});
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),page); escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,[this]{pendingDrawTool_.clear();if(imageViewer_->roiGestureActive()){imageViewer_->cancelRoi();return;}imageViewer_->drawRegion(false);}); return page;
}
void MainWindow::showFlowMenu(const QPoint& position) {
    auto* item=flowTools_->itemAt(position);
    if(!item || !service_.activeRecipe())return;
    const auto id=item->data(Qt::UserRole).toString(),recipe=editorRecipeId_;
    const auto row=flowTools_->row(item);
    auto* menu=new QMenu(flowTools_);menu->setObjectName("flowMoveMenu");menu->setAttribute(Qt::WA_DeleteOnClose);
    const bool idle=!editorSession_.busy();
    for(const int direction:{-1,1}) {
        auto* action=menu->addAction(direction<0 ? "Bir sıra yukarı":"Bir sıra aşağı");
        action->setObjectName(direction<0 ? "flowMoveUp":"flowMoveDown");
        action->setEnabled(idle && (direction<0 ? row>0:row+1<flowTools_->count()));
        connect(action,&QAction::triggered,this,[this,id,recipe,direction]{
            if(editorSession_.busy() || editorRecipeId_!=recipe)return;
            try {if(service_.moveTool(id,direction))refreshWorkspace();}
            catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
        });
    }
    menu->popup(flowTools_->viewport()->mapToGlobal(position));
}
void MainWindow::refreshEditor() {
    editor_observation::Scope observation(editor_observation::Editor);
    const auto epoch=++editorRefreshEpoch_;
    const auto environment=editorDisplayEnvironment();
    syncEditorCameras({},false,environment ? &*environment:nullptr);
    // A stack-local display view: never retained by callbacks or used as admission.
    // Build after camera-source reconciliation, which can change the recipe.
    const auto context=editorContext();
    const auto* recipe=service_.activeRecipe(); const QString recipeId=recipe ? service_.project().id+"/"+recipe->id : "";
    if (recipeId!=editorRecipeId_) {editorSession_.sourceFrames.clear();editorSession_.frame.reset();editorSession_.invalidateResults();matchInfo_->clear();imageInfo_->setText("Görüntü yok.");pendingDrawTool_.clear();imageViewer_->cancelRoi();selectedToolId_.clear(); editorRecipeId_=recipeId; imageBinding_.clear();}
    const auto tools=recipe ? service_.tools() : QVector<ToolDefinition>{};
    const QSignalBlocker blocker(flowTools_);
    if (!propertiesEditing_) flowTools_->clear();
    bool found=false;
    for (const auto& t : tools) {
        const QString caption=t.name+"  •  "+toolLabel(t.kind)+(t.enabled ? (t.kind=="image_file" ? "  / Kaynak yapılandırması" : t.kind=="template_match" ? "  / Match yapılandırması" : "  / Yürütülmedi") : "  / Disabled");
        QListWidgetItem* item=nullptr;
        if (propertiesEditing_) for (int i=0;i<flowTools_->count();++i) if (flowTools_->item(i)->data(Qt::UserRole).toString()==t.id) item=flowTools_->item(i);
        if (!item) item=new QListWidgetItem(flowTools_); item->setText(caption); item->setData(Qt::UserRole,t.id);
        if (t.id==selectedToolId_) { flowTools_->setCurrentItem(item); found=true; }
    }
    if (!found) selectedToolId_.clear();
    const QString path=service_.imageFilePath(); QString binding=recipeId+"|"+path;
    bool webcamSource=false;
    for(const auto& t:tools)if(t.kind=="webcam") {webcamSource=true;binding+="|"+t.id+"|"+QString::fromUtf8(QJsonDocument(t.parameters).toJson(QJsonDocument::Compact))+"|"+QString::number(t.enabled);}
    const bool multi=recipe && recipe->definition["editor"].toObject()["schemaVersion"]==9;
    if(auto* button=findChild<QPushButton*>("enableMultiSource")){button->setVisible(!multi);button->setEnabled(recipe && !editorSession_.busy());}
    if(auto* picker=findChild<QComboBox*>("editorSourceSelector")){const QSignalBlocker guard(picker);picker->clear();for(const auto& t:tools)if(ToolRegistry::get(t.kind).acquisitionSource)picker->addItem(t.name,t.id);picker->setVisible(multi);picker->setEnabled(!editorSession_.busy());const auto source=editorSession_.sourceForTool(context,selectedToolId_);picker->setCurrentIndex(picker->findData(source));}
    if(multi){editorSession_.reconcile(context);if(!selectedToolId_.isEmpty())editorSession_.selectSource(editorSession_.sourceForTool(context,selectedToolId_));imageBinding_=binding;}
    if (!multi && binding!=imageBinding_) {
        ++editorSession_.generation; invalidateMatch();
        imageBinding_=binding; editorSession_.frame.reset();
        imageViewer_->clearImage(webcamSource ? "Run Flow ile webcam'den tek kare alın." : recipe ? "Open Image ile görüntü yükleyin." : "Önce bir reçete açın."); imageInfo_->setText("Görüntü yok.");
        if (!path.isEmpty()) {
            loadImageAsync(path,false);
        }
    }
    for (const auto name : {"openImage","addTool"}) if (auto* button=findChild<QPushButton*>(name)) button->setEnabled(recipe!=nullptr);
    findChild<QAction*>("reloadImage")->setEnabled(recipe!=nullptr);
    const auto generation=editorSession_.generation;
    const auto current=[&]{
        const auto* active=service_.activeRecipe();
        return epoch==editorRefreshEpoch_ && generation==editorSession_.generation &&
            ((!active && context.recipeId.isEmpty()) || (active && active->id==context.recipeId &&
             active->definition==context.definition && service_.path()==context.path && service_.project().id==context.projectId));
    };
    if (!propertiesEditing_) showToolPropertiesView(context,environment);
    if(!current())return;
    bool sourceSelected=false; for (const auto& t : tools) if (t.id==selectedToolId_ && t.kind=="image_file") sourceSelected=true;
    imageViewer_->setSourceSelected(sourceSelected);
    refreshMatchOverlayView(context);
    if(!current())return;
    refreshFlowView(context,environment);
}
void MainWindow::selectTool(const QString& id) {
    if (id!=selectedToolId_ && !applyToolProperties()) {
        const QSignalBlocker blocker(flowTools_);
        for (int i=0;i<flowTools_->count();++i) if (flowTools_->item(i)->data(Qt::UserRole).toString()==selectedToolId_) flowTools_->setCurrentRow(i);
        return;
    }
    if(id!=selectedToolId_){pendingDrawTool_.clear();imageViewer_->cancelRoi();imageViewer_->resetRoiHistory();}
    selectedToolId_=id;
    if(service_.activeRecipe() && service_.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9){const auto source=editorSession_.sourceForTool(editorContext(),id);editorSession_.selectSource(source);if(auto* picker=findChild<QComboBox*>("editorSourceSelector")){const QSignalBlocker guard(picker);picker->setCurrentIndex(picker->findData(source));}}
    imageViewer_->drawRegion(false);
    const QSignalBlocker blocker(flowTools_); flowTools_->clearSelection(); flowTools_->setCurrentItem(nullptr);
    for (int i=0;i<flowTools_->count();++i) if (flowTools_->item(i)->data(Qt::UserRole).toString()==id) flowTools_->setCurrentRow(i);
    showToolProperties();
    bool source=false; if (service_.activeRecipe()) for (const auto& t : service_.tools()) if (t.id==id && t.kind=="image_file") source=true;
    imageViewer_->setSourceSelected(source);
    refreshMatchOverlay();
    refreshFlow();
}
void MainWindow::refreshOutputVisibilityControls() {
    if(!service_.activeRecipe())return;
    for(const auto& tool:service_.tools())if(tool.id==selectedToolId_) {
        const auto preference=outputPreferences_.load({service_.project().id,service_.activeRecipe()->id,tool.id});
        const auto selected=OutputVisibilityPolicy::resolve(tool,preference.selection).visiblePortIds();
        if(auto* outputs=propertiesBody_->findChild<QWidget*>(tool.kind+"/outputs"))for(auto* box:outputs->findChildren<QCheckBox*>()) {
            const QSignalBlocker block(box);box->setChecked(selected.contains(box->objectName().mid(QString("outputVisible_").size())));
        }
        if(!preference.error.isEmpty())if(auto* error=propertiesBody_->findChild<QLabel*>("outputPreferenceError")){error->setText(preference.error);error->show();}
        break;
    }
}
void MainWindow::showToolProperties() {
    const auto environment=editorDisplayEnvironment();
    showToolPropertiesView(editorContext(),environment);
}
void MainWindow::showToolPropertiesView(const EditorContext& context,const std::optional<MeasurementEnvironment>& displayEnvironment) {
    const QScopedValueRollback<bool> guard(propertiesEditing_,true);
    displayedProperties_.reset();
    auto* layout=static_cast<QVBoxLayout*>(propertiesBody_->layout());
    while (auto* item=layout->takeAt(0)) {delete item->widget(); delete item;}
    layout->addWidget(note("PROPERTIES"));
    std::optional<ToolDefinition> chosen;
    if (service_.activeRecipe()) for (const auto& t : service_.tools()) if (t.id==selectedToolId_) chosen=t;
    if (!chosen) {layout->addWidget(note("Tool seçilmedi. Toolbox'tan ekleyin veya Flow listesinden seçin.")); layout->addStretch(); return;}
    const auto tool=*chosen;
    findChild<QScrollArea*>("propertiesScroll")->setMinimumWidth(supportsPoseTracking(tool.kind) || tool.kind=="pattern_pose" || tool.kind=="shape_pose" ? 240:210);
    auto* panel=new QWidget; auto* form=new QFormLayout(panel);
    form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    auto* name=new QLineEdit(tool.name); name->setObjectName("toolName"); name->setMaxLength(64); form->addRow("Name",name);
    auto* description=new QLineEdit(tool.description); description->setObjectName("toolDescription"); description->setMaxLength(2048); form->addRow("Description",description);
    auto* enabled=new QCheckBox("Enabled"); enabled->setObjectName("toolEnabled"); enabled->setChecked(tool.enabled); form->addRow(enabled);
    auto* drawHint=note("");drawHint->setObjectName("newToolDrawHint");drawHint->hide();form->addRow(drawHint);
    if(tool.kind!="template_match") {
        auto* run=new QPushButton(ToolRegistry::get(tool.kind).acquisitionSource ? "Görüntü al" : "Run Tool");run->setObjectName("runSelectedTool");form->addRow(run);
        connect(run,&QPushButton::clicked,this,[this,tool]{runExecution(tool.id);});
    }
    if(ToolRegistry::get(tool.kind).acquisitionSource && tool.kind!="industrial_camera" && displayEnvironment){
        auto* cameras=new QComboBox;cameras->setObjectName("sourceCameraSelector");cameras->addItem("Kamera bağı yok / dosya raw px",QString{});
        const auto& environment=*displayEnvironment;const auto definition=service_.activeRecipe()->definition;const auto bindingForCamera=definition["sourceSetups"].toObject().contains(tool.id) ? definition["sourceSetups"].toObject()[tool.id].toObject():definition["sourceSetup"].toObject();std::optional<SetupReference> selected;if(bindingForCamera["sourceToolId"]==tool.id)selected=SetupReference::fromJson(bindingForCamera["target"].toObject());
        for(const auto& camera:environment.current.cameras)if(!environment.archivedCameras.contains(camera.id.value) && (tool.kind!="webcam" || !camera.physical)){const auto registration=environment.registrations.value(camera.id.value);cameras->addItem((registration.shortId.isEmpty()?camera.id.value:registration.shortId)+" • "+camera.name,camera.id.value);}
        if(selected)cameras->setCurrentIndex(qMax(0,cameras->findData(selected->camera.value)));form->addRow("Kamera ID / ad",cameras);
        connect(cameras,&QComboBox::activated,this,[this,tool,cameras]{try{if(!measurementWorkspace_->changeAllowed() || editorSession_.busy())throw std::runtime_error("Önce worker/Runtime işlemini bitirin.");const auto before=service_.activeRecipe()->definition;if(cameras->currentData().toString().isEmpty())service_.clearSourceSetup(tool.id);else service_.bindCameraSource(tool.id,measurementWorkspace_->environment(),CameraId{cameras->currentData().toString()});if(before!=service_.activeRecipe()->definition){editorSession_.invalidateTool(editorContext(),tool.id);editorSession_.sourceFrames.remove(tool.id);if(editorSession_.frame && editorSession_.frame->metadata().sourceId==tool.id)editorSession_.frame.reset();editorSession_.invalidateResults();imageBinding_.clear();}QTimer::singleShot(0,this,&MainWindow::refreshWorkspace);}catch(const std::exception& ex){displayError(QString::fromUtf8(ex.what()));}});
        form->addRow(note(calibrationSummary(&context)));
        auto* details=new QPushButton("Eski / açık kurulum bağı — ayrıntılar");details->setCheckable(true);details->setObjectName("sourceSetupDetails");form->addRow(details);auto* advanced=new QWidget;form->addRow(advanced);advanced->hide();connect(details,&QPushButton::toggled,advanced,&QWidget::setVisible);
        auto* form=new QFormLayout(advanced);

        auto* setups=new QComboBox;setups->setObjectName("sourceSetupSelector");setups->addItem("Kurulum bağı yok",QJsonObject{});
        const auto& e=*displayEnvironment;for(const auto& s:e.current.setups)if(!e.archivedSetups.contains(s.id.value) && !e.archivedCameras.contains(s.camera.value))setups->addItem(s.name+QString(" • r%1").arg(s.revision),SetupReference{s.camera,s.id,s.revision}.toJson());
        const auto d=service_.activeRecipe()->definition;const auto binding=d["sourceSetups"].toObject().contains(tool.id) ? d["sourceSetups"].toObject()[tool.id].toObject():d["sourceSetup"].toObject();const auto current=binding["sourceToolId"]==tool.id ? binding["target"].toObject():QJsonObject{};for(int i=0;i<setups->count();++i)if(setups->itemData(i).toJsonObject()==current)setups->setCurrentIndex(i);
        form->addRow("Ölçüm kurulumu",setups);form->addRow(note(calibrationSummary(&context)));
        auto* bind=new QPushButton("Kurulumu bağla");bind->setObjectName("sourceSetupApply");form->addRow(bind);
        connect(bind,&QPushButton::clicked,this,[this,setups,tool]{try{if(editorSession_.busy() || runtimeJobs_.busy() || (runtimeCoordinator_ && (runtimeCoordinator_->state()==RuntimeState::Running || runtimeCoordinator_->state()==RuntimeState::Starting || runtimeCoordinator_->state()==RuntimeState::Stopping)) || (plc_ && plc_->blocking()))throw std::runtime_error("Önce Runtime/worker/pending kayıt/ACK işlemini bitirin.");const auto before=service_.activeRecipe()->definition;const auto target=setups->currentData().toJsonObject();if(target.isEmpty())service_.clearSourceSetup(tool.id);else service_.bindSourceSetup(tool.id,target);if(before==service_.activeRecipe()->definition)return;editorSession_.invalidateTool(editorContext(),tool.id);editorSession_.sourceFrames.remove(tool.id);editorSession_.frame.reset();editorSession_.invalidateResults();imageBinding_.clear();matchInfo_->clear();refreshWorkspace();}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}});
        auto* manage=new QPushButton("Kurulumları aç");manage->setObjectName("sourceSetupManage");form->addRow(manage);connect(manage,&QPushButton::clicked,this,[this]{selectPage("calibration");});
    }
    if(tool.kind=="industrial_camera") {
        const auto reference=tool.parameters["sourceReference"].toObject();
        auto* status=note("Kamera seçin. Seçim varsayılan kurulum/profili birlikte bağlar; görüntü almaz.");status->setObjectName("industrialSourceStatus");form->addRow(status);
        auto* cameras=new QComboBox;cameras->setObjectName("sourceCameraSelector");cameras->addItem("Kamera seçin",QString{});
        if(displayEnvironment){const auto& e=*displayEnvironment;
            for(const auto& c:e.current.cameras)if(c.physical && !e.archivedCameras.contains(c.id.value)){
                const auto registration=e.registrations.value(c.id.value);cameras->addItem((registration.shortId.isEmpty()?c.id.value:registration.shortId)+" • "+c.name,c.id.value);
                cameras->setItemData(cameras->count()-1,c.physical->serial+" • "+measurementWorkspace_->availability(c),Qt::ToolTipRole);
            }
            if(!reference.isEmpty())try{IndustrialSourceReference::fromJson(reference).validateAgainst(e.current);status->setText("Kamera bağlı. "+calibrationSummary(&context));}catch(const std::exception& ex){status->setText("Bağ geçersiz: "+QString::fromUtf8(ex.what())+"\nUygun ayar snapshot’ı Run Flow hazırlığında yenilenir; geometri değişiminde kamerayı yeniden seçin.");}
        }
        cameras->setCurrentIndex(qMax(0,cameras->findData(reference["cameraId"].toString())));form->addRow("Kamera ID / ad",cameras);
        connect(cameras,&QComboBox::activated,this,[this,tool,cameras]{try{
            if(!measurementWorkspace_ || !measurementWorkspace_->changeAllowed() || editorSession_.busy())throw std::runtime_error("Önce Runtime/worker işlemini bitirin.");
            if(cameras->currentData().toString().isEmpty())return;
            const auto before=service_.activeRecipe()->definition;
            service_.bindCameraSource(tool.id,measurementWorkspace_->environment(),CameraId{cameras->currentData().toString()},tool.parameters["sourceReference"].toObject()["timeoutMs"].toInt(5000));
            if(before!=service_.activeRecipe()->definition){editorSession_.invalidateTool(editorContext(),tool.id);editorSession_.sourceFrames.remove(tool.id);if(editorSession_.frame && editorSession_.frame->metadata().sourceId==tool.id)editorSession_.frame.reset();editorSession_.invalidateResults();imageBinding_.clear();matchInfo_->clear();}
            // QComboBox still uses itself after activated returns (keyboard/popup input).
            // Rebuilding properties here would delete the sender inside its event handler.
            QTimer::singleShot(0,this,&MainWindow::refreshWorkspace);
        }catch(const std::exception& ex){displayError(QString::fromUtf8(ex.what()));}});
        auto* manage=new QPushButton(cameras->count()==1 ? "Kamera ekle — Setup":"Kameraları aç");manage->setObjectName("sourceCameraManage");form->addRow(manage);connect(manage,&QPushButton::clicked,this,[this]{selectPage("camera");});
        form->addRow(note("Raw/px kalibrasyonsuzdur. mm yalnız bu kameranın uygun etkin modeliyle hesaplanır. Model/optik değişimi world ROI ve referansı yeniden denetler."));
    } else if (tool.kind=="image_file") {
        auto* path=new QLineEdit(tool.parameters["path"].toString()); path->setReadOnly(true); path->setCursorPosition(0); path->setPlaceholderText("Seçilmedi"); path->setToolTip(path->text()); form->addRow("File",path);
        auto* choose=new QPushButton("Open Image…"); form->addRow(choose); connect(choose,&QPushButton::clicked,this,&MainWindow::openImageDialog);
    } else if(tool.kind=="webcam") {
        auto* index=new QSpinBox;index->setObjectName("webcamIndex");index->setRange(0,63);index->setValue(tool.parameters["deviceIndex"].toInt());form->addRow("Device index",index);index->setEnabled(!service_.activeRecipe()->definition.contains("sourceSetup") && !service_.activeRecipe()->definition["sourceSetups"].toObject().contains(tool.id));
        auto* timeout=new QSpinBox;timeout->setObjectName("webcamTimeout");timeout->setRange(1,60000);timeout->setSuffix(" ms");timeout->setValue(tool.parameters["timeoutMs"].toInt());form->addRow("Timeout",timeout);
        form->addRow(note("Run Flow: host isteğiyle tek kare. Donanım trigger/live/exposure/gain yok. Stop/deadline geç kareyi reddeder; sürücü çağrısını zorla kesmez. Index kalıcı cihaz kimliği değildir."));
        connect(index,&QSpinBox::valueChanged,this,[this]{applyToolProperties();});connect(timeout,&QSpinBox::valueChanged,this,[this]{applyToolProperties();});
    } else if (tool.kind=="template_match") {
        auto* value=new QDoubleSpinBox; value->setObjectName("minScore"); value->setRange(0,100); value->setDecimals(1); value->setSuffix(" %"); value->setValue(tool.parameters["minScore"].toDouble()*100); form->addRow("Minimum Score",value);
        connect(value,&QDoubleSpinBox::valueChanged,this,[this]{applyToolProperties();});
        auto* run=new QPushButton("Run Tool"); run->setObjectName("runMatch"); form->addRow(run); connect(run,&QPushButton::clicked,this,&MainWindow::runMatch);
        auto* model=new QPushButton("Model ROI çiz / öğret"); model->setObjectName("modelRoi"); form->addRow(model);
        connect(model,&QPushButton::clicked,this,[this]{beginRoi(true);});
        auto* search=new QPushButton("Search ROI çiz"); search->setObjectName("searchRoi"); form->addRow(search);
        connect(search,&QPushButton::clicked,this,[this]{beginRoi(false);});
        auto* clear=new QPushButton("Tam kare ara");clear->setObjectName("clearSearch"); clear->setObjectName("clearSearch"); form->addRow(clear);
        connect(clear,&QPushButton::clicked,this,[this]{
            if (!applyToolProperties()) return;
            try {for (auto t : service_.tools()) if (t.id==selectedToolId_) {t.parameters["searchRoi"]=QJsonObject{}; service_.updateTool(t); refreshWorkspace(); break;}}
            catch (const std::exception& e) {displayError(QString::fromUtf8(e.what()));}
        });
        form->addRow("Model",note(tool.parameters["modelPng"].toString().isEmpty() ? "Öğretilmedi" : "Öğretildi — proje içinde PNG"));
        for (const auto key : {"modelRoi","searchRoi"}) {
            const auto r=MatchService::rectangle(tool.parameters[key].toObject());
            form->addRow(QString(key),note(r.isEmpty() ? "Seçilmedi" : QString("%1, %2 / %3 × %4 px").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height())));
        }
    } else if(tool.kind=="pattern_pose" || tool.kind=="shape_pose") {
        auto* settings=new PoseMatchEditor(tool.kind,tool.parameters);form->addRow(settings);settings->onChanged=[this]{applyToolProperties();};
        auto* model=new QPushButton("Model ROI çiz / öğret");model->setObjectName("modelRoi");form->addRow(model);connect(model,&QPushButton::clicked,this,[this]{beginRoi(true);});
        auto* search=new QPushButton("Search ROI çiz");search->setObjectName("searchRoi");form->addRow(search);connect(search,&QPushButton::clicked,this,[this]{beginRoi(false);});
        auto* clear=new QPushButton("Tam kare ara");clear->setObjectName("clearSearch");clear->setObjectName("clearSearch");form->addRow(clear);connect(clear,&QPushButton::clicked,this,[this]{if(!applyToolProperties())return;try{for(auto t:service_.tools())if(t.id==selectedToolId_){t.parameters["searchRoi"]=QJsonObject{};service_.updateTool(t);refreshWorkspace();break;}}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}});
        form->addRow("Model",note(tool.parameters["modelPng"].toString().isEmpty() ? "Öğretilmedi":"Öğretildi — proje içinde PNG ve explicit anchor"));
    } else if(tool.kind=="geometry_measurement") {
        form->addRow(note("Raw px feature → pixel diagnostic veya kalibre mm refit. Line/Circle producer pixel olmalı; lineMm/circleMm girdi değildir. Çizgi sonsuz, segment sınırlı; signed +X→+Y, açı derece. accepted: yalnız tool toleransı."));
        const auto defaults=geometryMeasurementDefaults();
        for(const auto key:{"operation","mode","offset","angle"}){
            auto* c=new QComboBox;c->setObjectName(QString("gm_")+key);
            c->addItems(QString(key)=="operation" ? QStringList{"point_distance","x_offset","y_offset","point_line_distance","point_segment_distance","line_angle","line_intersection","parallel_distance","circle_centre_distance","circle_radius","circle_diameter"}:QString(key)=="mode" ? QStringList{"pixel","calibrated"}:QString(key)=="offset" ? QStringList{"absolute","signed"}:QStringList{"acute_undirected","unsigned_directed","signed_directed"});
            c->setCurrentText(tool.parameters[key].toString());form->addRow(key,c);
            connect(c,&QComboBox::currentIndexChanged,this,[this]{if(applyToolProperties())QTimer::singleShot(0,this,[this]{showToolProperties();});});
        }
        auto* tolerance=new QCheckBox("Toleransı değerlendir");tolerance->setObjectName("gm_toleranceEnabled");tolerance->setChecked(tool.parameters["toleranceEnabled"].toBool());tolerance->setEnabled(tool.parameters["operation"]!="line_intersection");form->addRow(tolerance);connect(tolerance,&QCheckBox::toggled,this,[this]{applyToolProperties();});
        auto* fitPanel=new QWidget;auto* fitForm=new QFormLayout(fitPanel);fitForm->setContentsMargins(0,0,0,0);fitForm->setRowWrapPolicy(QFormLayout::WrapAllRows);fitPanel->hide();
        auto* fitButton=new QPushButton("Refit ayarları…");form->addRow(fitButton);connect(fitButton,&QPushButton::clicked,fitPanel,[fitPanel]{fitPanel->setVisible(!fitPanel->isVisible());});
        for(auto i=defaults.begin();i!=defaults.end();++i)if(i->isDouble()){
            auto* n=new QDoubleSpinBox;n->setObjectName("gm_"+i.key());n->setDecimals(i.key()=="fitMinPoints" ? 0:9);n->setRange(-1e9,1e12);n->setValue(tool.parameters.value(i.key()).toDouble(i->toDouble()));
            const bool limit=i.key()=="lower" || i.key()=="upper" || i.key()=="nominal";
            (limit ? form:fitForm)->addRow(i.key()+(limit ? (tool.parameters["operation"]=="line_angle" ? " (deg)":tool.parameters["mode"]=="pixel" ? " (px)":" (mm)"):""),n);connect(n,&QDoubleSpinBox::editingFinished,this,[this]{applyToolProperties();});
        }
        auto* robust=new QCheckBox("Robust refit (RANSAC)");robust->setObjectName("gm_fitRansac");robust->setChecked(tool.parameters["fitRansac"].toBool());fitForm->addRow(robust);form->addRow(fitPanel);connect(robust,&QCheckBox::toggled,this,[this]{applyToolProperties();});
    } else if(tool.kind=="edge_pair" || tool.kind=="line_fit" || tool.kind=="circle_fit") {
        const auto frame=editorSession_.frame;auto* settings=new MetrologyEditor(tool.kind,tool.parameters,frame ? frame->metadata().coordinateSystemId:QString{},frame.has_value() && !editorSession_.busy());form->addRow(settings);settings->onChanged=[this]{applyToolProperties();};
        settings->onDraw=[this,kind=tool.kind]{if(auto* dialog=findChild<QDialog*>("roiGeometryDialog"))dialog->close();refreshMatchOverlay();if(!editorSession_.busy() && applyToolProperties() && setDrawingImage()){if(kind=="circle_fit")for(const auto& t:service_.tools())if(t.id==selectedToolId_ && t.parameters["mode"]=="calibrated"){const auto ctx=calibrationResolver(*service_.activeRecipe())(*editorSession_.frame);if(!ctx.calibration || !ctx.current){displayError("Circle teaching requires current calibration");return;}}imageViewer_->beginRoi(kind=="circle_fit" ? 2:kind=="edge_pair" ? 0:1);}};
        settings->onPreview=[this,id=tool.id](const ToolRoi& roi){
            if(editorSession_.busy() || selectedToolId_!=id)return;
            for(const auto& t:service_.tools())if(t.id==id && !t.parameters.contains("referenceBinding") && !t.parameters.contains("poseTracking")) {
                if(!editorSession_.frame || (roi.frame.unit==LengthUnit::Pixel && !roi.inside(editorSession_.frame->size())))return;
                invalidateMatch();imageViewer_->previewRoi(roi);
            }
        };
        settings->onRoi=[this,id=tool.id,kind=tool.kind](const ToolRoi& value){try{if(selectedToolId_!=id)return;auto roi=value;for(const auto& t:service_.tools())if(t.id==selectedToolId_ && (t.parameters.contains("referenceBinding") || t.parameters.contains("poseTracking")))return;if(editorSession_.busy() || !applyToolProperties())return;if(kind=="circle_fit")for(const auto& t:service_.tools())if(t.id==selectedToolId_ && t.parameters["mode"]=="calibrated"){auto ctx=calibrationResolver(*service_.activeRecipe())(*editorSession_.frame);if(!ctx.calibration || !ctx.current)throw std::runtime_error("Circle teaching requires current calibration");roi.frame=ctx.calibration->dependency().target;}if(!editorSession_.frame || (roi.frame.unit==LengthUnit::Pixel && !roi.inside(editorSession_.frame->size())))throw std::runtime_error("ROI görüntü sınırı dışında");imageViewer_->setRoi(roi);}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}};
        form->addRow("Kalibrasyon",note(calibrationSummary(&context)));
        auto* calibration=new QPushButton("Calibration aç");form->addRow(calibration);connect(calibration,&QPushButton::clicked,this,[this]{selectPage("calibration");});
    } else if(tool.kind=="edge_point") {
        auto* settings=new EdgePointEditor(tool.parameters);form->addRow(settings);settings->onChanged=[this]{applyToolProperties();};
        const auto frame=editorSession_.frame;const auto frameId=frame ? frame->metadata().coordinateSystemId:QString{};
        ScanSegment scan{{10,10},{50,10},4};if(frame){const auto size=frame->size();scan={{size.width()*.2,size.height()*.5},{size.width()*.8,size.height()*.5},4};}
        const auto saved=tool.parameters["scanRoi"].toObject();if(!saved.isEmpty())scan=std::get<ScanSegment>(ToolRoi::fromJson(saved).geometry);
        auto* draw=new QPushButton("Ok çiz (A → B)");draw->setObjectName("edgeDraw");draw->setEnabled(frame.has_value() && !editorSession_.busy());form->addRow(draw);
        connect(draw,&QPushButton::clicked,this,[this]{if(auto* dialog=findChild<QDialog*>("roiGeometryDialog"))dialog->close();refreshMatchOverlay();if(!editorSession_.busy() && applyToolProperties() && setDrawingImage())imageViewer_->beginRoi(0);});
        for(const auto& entry:QVector<QPair<QString,double>>{{"ax",scan.start.x()},{"ay",scan.start.y()},{"bx",scan.end.x()},{"by",scan.end.y()},{"width",scan.width}}){auto* spin=new QDoubleSpinBox;spin->setObjectName("scan_"+entry.first);spin->setDecimals(9);spin->setRange(entry.first=="width" ? .000000001:-1e8,entry.first=="width" ? 1024:1e8);spin->setValue(entry.second);form->addRow(entry.first+(isWorldPoseTracking(tool) ? " (nominal world mm)":" (raw px)"),spin);}
        auto* apply=new QPushButton("Koordinatları uygula");apply->setObjectName("edgeApplyScan");apply->setEnabled(frame.has_value() && !editorSession_.busy());form->addRow(apply);
        connect(apply,&QPushButton::clicked,this,[this,frameId]{if(auto* dialog=findChild<QDialog*>("roiGeometryDialog"))dialog->close();refreshMatchOverlay();if(editorSession_.busy() || !applyToolProperties())return;auto value=[this](const char* key){return propertiesBody_->findChild<QDoubleSpinBox*>(QString("scan_")+key)->value();};try{ToolRoi roi{{frameId},ScanSegment{{value("ax"),value("ay")},{value("bx"),value("by")},value("width")}};if(!editorSession_.frame || (roi.frame.unit==LengthUnit::Pixel && !roi.inside(editorSession_.frame->size())))throw std::runtime_error("Ok şeridi görüntü sınırları dışında.");imageViewer_->setRoi(roi);}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}});
        auto* reverse=new QPushButton("Yönü ters çevir");reverse->setObjectName("edgeReverse");reverse->setEnabled(!saved.isEmpty() && frame.has_value() && !editorSession_.busy());form->addRow(reverse);
        connect(reverse,&QPushButton::clicked,this,[this]{if(auto* dialog=findChild<QDialog*>("roiGeometryDialog"))dialog->close();refreshMatchOverlay();if(editorSession_.busy() || !applyToolProperties())return;try{for(auto t:service_.tools())if(t.id==selectedToolId_){auto roi=ToolRoi::fromJson(t.parameters["scanRoi"].toObject());auto& g=std::get<ScanSegment>(roi.geometry);std::swap(g.start,g.end);imageViewer_->setRoi(roi);break;}}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}});
        form->addRow(note(saved.isEmpty() ? "Ok öğretilmedi; çizim veya koordinatları uygula gerekir.":"Ok reçeteye bağlıdır; tutamaçları sürükleyin. Ctrl+Z geometriyi geri alır. Escape çizimi iptal eder."));
    } else if(tool.kind=="code_read") {
        auto* settings=new CodeReadEditor(tool.parameters);form->addRow(settings);settings->onChanged=[this]{applyToolProperties();};
        auto* roi=new QPushButton("Search ROI çiz");roi->setObjectName("searchRoi");form->addRow(roi);connect(roi,&QPushButton::clicked,this,[this]{beginRoi(false);});
        auto* clear=new QPushButton("Tam kare ara");clear->setObjectName("clearSearch");clear->setObjectName("clearSearch");form->addRow(clear);connect(clear,&QPushButton::clicked,this,[this]{if(!applyToolProperties())return;try{for(auto t:service_.tools())if(t.id==selectedToolId_){t.parameters["searchRoi"]=QJsonObject{};service_.updateTool(t);refreshWorkspace();break;}}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}});
        const auto roiValue=tool.parameters["searchRoi"].toObject();form->addRow("Arama alanı (raw px)",note(roiValue.isEmpty() ? "Tam kare":QString("X: %1 px · Y: %2 px\nGenişlik: %3 px · Yükseklik: %4 px").arg(roiValue["x"].toInt()).arg(roiValue["y"].toInt()).arg(roiValue["width"].toInt()).arg(roiValue["height"].toInt())));
    } else if(tool.kind=="roi") {
        auto* draw=new QPushButton("Dikdörtgen ROI çiz");draw->setObjectName("searchRoi");form->addRow(draw);
        connect(draw,&QPushButton::clicked,this,[this]{beginRoi(false);});
        const auto r=MatchService::rectangle(tool.parameters["searchRoi"].toObject());
        form->addRow("İşlem alanı (px)",note(r.isEmpty() ? "ROI çizilmedi":QString("X: %1 · Y: %2\nGenişlik: %3 · Yükseklik: %4").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height())));
        form->addRow(note("Image girdisini bağlayın, ROI çizin. Region çıktısını Blob veya Template Match aracının region girdisine bağlayın. Görüntü kırpılmaz. Yalnız dikdörtgen desteklenir; diğer araçlar kendi ROI alanını kullanır."));
    } else if(tool.kind=="preprocess") {
        auto* settings=new PreprocessEditor(tool.parameters);form->addRow(settings);settings->onChanged=[this]{applyToolProperties();};
    } else if (tool.kind=="blob") {
        auto* value=new QSpinBox; value->setObjectName("threshold"); value->setRange(0,255); value->setValue(tool.parameters["threshold"].toInt()); form->addRow("Threshold (8-bit)",value);
        connect(value,&QSpinBox::valueChanged,this,[this]{applyToolProperties();});
        auto* polarity=new QComboBox;polarity->setObjectName("blobPolarity");polarity->addItem("Açık (> eşik)","bright");polarity->addItem("Koyu (≤ eşik)","dark");polarity->setCurrentIndex(polarity->findData(tool.parameters["polarity"].toString()));form->addRow("Nesne",polarity);
        auto* connectivity=new QComboBox;connectivity->setObjectName("blobConnectivity");connectivity->addItem("4",4);connectivity->addItem("8",8);connectivity->setCurrentIndex(connectivity->findData(tool.parameters["connectivity"].toInt()));form->addRow("Komşuluk",connectivity);
        for(const auto key:{"minArea","maxArea"}){auto* area=new QSpinBox;area->setObjectName(key);area->setRange(1,2147483647);area->setSuffix(" px²");area->setValue(tool.parameters[key].isNull() ? tool.parameters["minArea"].toInt() : tool.parameters[key].toInt());area->setEnabled(QString(key)!="maxArea" || !tool.parameters["maxArea"].isNull());form->addRow(QString(key)=="minArea" ? "Min Area (dahil)" : "Max Area (dahil)",area);connect(area,&QSpinBox::editingFinished,this,[this]{applyToolProperties();});}
        auto* rejectBorder=new QCheckBox("ROI sınırına temas\nedenleri reddet");rejectBorder->setAccessibleName("ROI sınırına temas edenleri reddet");rejectBorder->setObjectName("rejectBorderTouching");rejectBorder->setChecked(tool.parameters["rejectBorderTouching"].toBool());rejectBorder->setToolTip("Arama alanının sınırına değen bağlı bölgeler sayım ve ölçümden çıkarılır.");form->addRow(rejectBorder);
        connect(rejectBorder,&QCheckBox::toggled,this,[this]{applyToolProperties();});
        auto* unlimited=new QCheckBox("Max Area: Sınırsız");unlimited->setObjectName("blobUnlimited");unlimited->setChecked(tool.parameters["maxArea"].isNull());form->addRow(unlimited);
        connect(unlimited,&QCheckBox::toggled,this,[this]{applyToolProperties();});
        connect(polarity,&QComboBox::currentIndexChanged,this,[this]{applyToolProperties();});connect(connectivity,&QComboBox::currentIndexChanged,this,[this]{applyToolProperties();});
        form->addRow(note("Alan filtresi adayları seçer; PASS/FAIL üretmez. Kırmızı maske gerçek kabul edilen piksellerdir. Delikler korunur. Bbox: en çok 2048; ROI: en çok 16 Mi piksel."));
        bool bound=tool.parameters.contains("poseTracking") || tool.parameters.contains("referenceBinding");
        for(const auto& c:editorConnections(service_.activeRecipe()->definition))if(c.targetTool==tool.id && (c.targetPort=="region" || c.targetPort=="reference" || c.targetPort=="poseReference"))bound=true;
        form->addRow(note(bound ? "Arama alanı region / referans / tracking girdisi tarafından belirlenir. Tutamaçla düzenleme kilitli; bağı kaldırın veya yeniden öğretin.":"Arama alanını gövdeden taşıyın veya sekiz tutamaçla boyutlandırın. Dışarıdan / orta tuşla pan. Escape iptal, Ctrl+Z son ROI değişimini geri alır."));
    } else if (tool.kind=="edge") {
        auto* value=new QComboBox; value->setObjectName("edgeDirection");
        value->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);value->setMinimumContentsLength(10);
        for (const auto key : {"left_to_right","right_to_left","top_to_bottom","bottom_to_top"}) value->addItem(QString(key).replace('_',' '),key);
        value->setCurrentIndex(value->findData(tool.parameters["direction"].toString())); form->addRow("Direction",value);
        connect(value,&QComboBox::currentIndexChanged,this,[this]{applyToolProperties();});
    } else if(tool.kind=="measurement") {
        form->addRow("Operation",note("Distance (mm) / inclusive tolerance"));
        for(const auto key:{"lowerMm","upperMm"}) {
            auto* value=new QDoubleSpinBox;value->setObjectName(key);value->setDecimals(6);value->setRange(0,1000000000);value->setSuffix(" mm");value->setValue(tool.parameters[key].toDouble());form->addRow(QString(key)=="lowerMm" ? "Alt tolerans" : "Üst tolerans",value);
            connect(value,&QDoubleSpinBox::editingFinished,this,[this]{applyToolProperties();});
        }
        form->addRow("Kalibrasyon",note(calibrationSummary(&context)));
        auto* calibration=new QPushButton("Calibration aç");form->addRow(calibration);connect(calibration,&QPushButton::clicked,this,[this]{selectPage("calibration");});
    } else form->addRow("Operation",note("Euclidean Distance (px)"));
    if(tool.kind=="edge") {
        auto* contrast=new QDoubleSpinBox;contrast->setObjectName("minContrast");contrast->setRange(1,255);contrast->setValue(tool.parameters["minContrast"].toDouble());form->addRow("Min Contrast",contrast);connect(contrast,&QDoubleSpinBox::valueChanged,this,[this]{applyToolProperties();});
        form->addRow(note("Referans bölümünde kaynak ve çıktı seçin. Güncel kaynak sonucu ile çizilmiş ROI arasındaki ilişki korunur."));
        const auto origin=tool.parameters["referenceOrigin"].toObject();const auto roi=MatchService::rectangle(tool.parameters["searchRoi"].toObject());
        if(!tool.parameters.contains("referenceBinding"))for(const auto& c:editorConnections(service_.activeRecipe()->definition))if(c.targetTool==tool.id && c.targetPort=="reference")
            for(const auto& source:service_.tools())if(source.id==c.sourceTool && referenceNeedsTeaching(tool,source))form->addRow(note("Referans modeli/geometrisi değişti. Search ROI çizerek yeniden öğretin."));
        if(!tool.parameters.contains("referenceBinding") && !origin.isEmpty() && !roi.isEmpty())form->addRow("Öğretilen ilişki",note(QString("ΔX: %1 / ΔY: %2 px • %3 × %4 px").arg(roi.x()-origin["x"].toDouble()).arg(roi.y()-origin["y"].toDouble()).arg(roi.width()).arg(roi.height())));
    }
    if(tool.kind=="blob" || tool.kind=="edge") {
        auto* roi=new QPushButton("Search ROI çiz");roi->setObjectName("searchRoi");form->addRow(roi);connect(roi,&QPushButton::clicked,this,[this]{beginRoi(false);});
        auto* clear=new QPushButton("Tam kare ara");clear->setObjectName("clearSearch");form->addRow(clear);connect(clear,&QPushButton::clicked,this,[this]{for(auto t:service_.tools())if(t.id==selectedToolId_){t.parameters["searchRoi"]=QJsonObject{};service_.updateTool(t);refreshWorkspace();break;}});
    }
    const auto connections=editorConnections(service_.activeRecipe()->definition);
    if(supportsReference(tool.kind))addTrackingProperties(form,tool);
    for(auto input:inputPorts(tool.kind)) {
        if(input.id=="poseReference" || (input.id=="reference" && supportsReference(tool.kind)))continue; // Teaching and reference commit together through the common picker.
        if(tool.kind=="geometry_measurement"){
            if(!geometryMeasurementRequiredInputs(tool.parameters).contains(input.id))continue;
            input.required=true;
        }
        auto* binding=new QComboBox;binding->setObjectName("input_"+input.id);binding->addItem(input.required ? "Bağlantı gerekli" : input.id=="reference" ? "Sabit görüntü koordinatları" : "Yerel ROI / tam kare",QString{});
        binding->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);binding->setMinimumContentsLength(10);
        QString current;
        for(const auto& c:connections)if(c.targetTool==tool.id && c.targetPort==input.id)current=c.sourceTool+"/"+c.sourcePort;
        for(const auto& upstream:service_.tools())if(upstream.id!=tool.id && !(tool.kind=="geometry_measurement" && (upstream.kind=="circle_fit" || upstream.kind=="line_fit") && upstream.parameters["mode"]=="calibrated" && input.id!="image") && !(tool.kind=="edge" && input.id=="region" && upstream.kind=="template_match"))for(const auto& output:outputPorts(upstream.kind))if(output.type==input.type && output.unit==input.unit && output.frame==input.frame)
            binding->addItem(upstream.name+"."+output.id,upstream.id+"/"+output.id);
        if(!current.isEmpty() && binding->findData(current)<0)binding->addItem("INVALID: "+current,current);
        binding->setCurrentIndex(current.isEmpty() ? 0 : binding->findData(current));
        if(tool.kind=="geometry_measurement"){
            binding->setToolTip("Yalnız aynı type/unit/frame portları listelenir. Eksik giriş sonuç üretmez. Feature ve samples aynı üreticiden gelmelidir.");
            if(input.id.startsWith("samples"))binding->setEnabled(false);
        }
        form->addRow(input.id=="image" ? QString("Görüntü kaynağı") : (input.id=="reference" ? QString("Referans merkezi") : input.id)+" · "+portTypeName(input.type)+" · "+input.unit+" · "+input.frame,binding);
        connect(binding,&QComboBox::currentIndexChanged,this,[this,binding,input,tool]{
            try {
                pendingDrawTool_.clear();
                auto list=editorConnections(service_.activeRecipe()->definition);
                list.erase(std::remove_if(list.begin(),list.end(),[&](const auto& c){return c.targetTool==tool.id && c.targetPort==input.id;}),list.end());
                const auto key=binding->currentData().toString();if(!key.isEmpty()){auto parts=key.split('/');list.append({parts[0],parts[1],tool.id,input.id});}
                auto updated=tool;for(const auto& current:service_.tools())if(current.id==tool.id)updated=current;
                if(input.id=="reference") {
                    updated.parameters["referenceOrigin"]=QJsonObject{};
                    if(!key.isEmpty()) {
                        const auto point=referencePoint(key.split('/')[0]);
                        if(!point)throw std::runtime_error("Referans merkezi bulunamadı. Önce modeli öğretin ve Run Flow çalıştırın.");
                        updated.parameters["referenceOrigin"]=QJsonObject{{"x",point->x()},{"y",point->y()}};
                        list.erase(std::remove_if(list.begin(),list.end(),[&](const auto& c){return c.targetTool==tool.id && c.targetPort=="region";}),list.end());
                    }
                }
                if(input.id=="region" && !key.isEmpty())list.erase(std::remove_if(list.begin(),list.end(),[&](const auto& c){return c.targetTool==tool.id && c.targetPort=="reference";}),list.end());
                if(tool.kind=="geometry_measurement" && (input.id.startsWith("line") || input.id.startsWith("circle"))){
                    const auto sample="samples"+input.id.right(1);
                    list.erase(std::remove_if(list.begin(),list.end(),[&](const auto& c){return c.targetTool==tool.id && c.targetPort==sample;}),list.end());
                    if(tool.parameters["mode"]=="calibrated" && !key.isEmpty())list.append({key.split('/')[0],"observationsPx",tool.id,sample});
                }
                service_.updateToolConnections(updated,list);propertiesEditing_=true;refreshWorkspace();propertiesEditing_=false;
                if(input.id=="reference" || input.id=="region" || tool.kind=="geometry_measurement")QTimer::singleShot(0,this,[this]{refreshWorkspace();});
            }catch(const std::exception& e){
                propertiesEditing_=false;const QSignalBlocker blocker(binding);QString saved;
                for(const auto& c:editorConnections(service_.activeRecipe()->definition))if(c.targetTool==tool.id && c.targetPort==input.id)saved=c.sourceTool+"/"+c.sourcePort;
                binding->setCurrentIndex(saved.isEmpty() ? 0 : binding->findData(saved));displayError(QString::fromUtf8(e.what()));
            }
        });
    }
    auto* outputs=propertySection(form,"Çıkışlar",tool.kind+"/outputs",settingsFile_);
    auto* outputForm=static_cast<QFormLayout*>(outputs->layout());
    const OutputVisibilityIdentity outputIdentity{service_.project().id,service_.activeRecipe()->id,tool.id};
    const auto preference=outputPreferences_.load(outputIdentity);
    auto* outputError=note(preference.error);outputError->setObjectName("outputPreferenceError");outputError->setVisible(!preference.error.isEmpty());outputForm->addRow(outputError);
    auto* outputHint=note("Yalnız Results listesini değiştirir. Görüntü ve overlay viewer'da kalır.");outputForm->addRow(outputHint);
    const auto visibility=OutputVisibilityPolicy::resolve(tool,preference.selection);
    for(const auto& entry:visibility.ports) {
        const auto& output=entry.port;
        auto* checkbox=new QCheckBox(output.id,outputs);checkbox->setObjectName("outputVisible_"+output.id);checkbox->setChecked(entry.visible);
        const QString description=output.type==PortType::Image ? "Güncel görüntü; viewer erişimi seçimden bağımsızdır." : output.type==PortType::Point ? "Hesaplanan nokta koordinatı." : output.type==PortType::Number ? "Hesaplanan sayısal sonuç." : output.type==PortType::Boolean ? "Gerçek sonuçtan gelen doğru/yanlış durumu." : "Aracın hesaplanan "+portTypeName(output.type)+" çıktısı.";
        checkbox->setAccessibleDescription(description);checkbox->setToolTip(output.id+" · "+portTypeName(output.type)+" · "+output.unit+" · "+output.frame+"\n"+description+"\nResults listesinde göster/gizle.");
        outputForm->addRow(checkbox,note(portTypeName(output.type)+" · "+output.unit));
        connect(checkbox,&QCheckBox::toggled,this,[this,outputs,outputIdentity,outputError](bool){
            QSet<QString> selected;for(auto* box:outputs->findChildren<QCheckBox*>())if(box->isChecked())selected.insert(box->objectName().mid(QString("outputVisible_").size()));
            const auto saved=outputPreferences_.save(outputIdentity,selected);
            outputError->setText(saved.error);outputError->setVisible(!saved.ok());
            refreshOutputVisibilityControls();
            showExecutionResult();
        });
    }
    for(const bool reset:{true,false}) {
        auto* button=new QPushButton(reset ? "Varsayılana dön":"Tümünü göster");button->setObjectName(reset ? "outputDefaults":"outputShowAll");outputForm->addRow(button);
        connect(button,&QPushButton::clicked,this,[this,outputs,outputIdentity,outputError,tool,reset]{
            const auto saved=reset ? outputPreferences_.reset(outputIdentity):outputPreferences_.showAll(outputIdentity,tool.kind);
            outputError->setText(saved.error);outputError->setVisible(!saved.ok());
            refreshOutputVisibilityControls();
            showExecutionResult();
        });
    }
    if(tool.kind=="edge_point" || tool.kind=="edge_pair" || tool.kind=="line_fit" || tool.kind=="circle_fit") {
        auto* editor=tool.kind=="edge_point" ? panel:panel->findChild<QWidget*>("metrologySettings");
        auto* rows=static_cast<QFormLayout*>(editor->layout());
        auto* manual=propertySection(rows,"Elle gir…",tool.kind+"/manual",settingsFile_);
        auto* manualRows=static_cast<QFormLayout*>(manual->layout());
        const auto prefix=tool.kind=="edge_point" ? QString("scan_"):QString("mt_roi_");
        const auto fields=editor->findChildren<QDoubleSpinBox*>();
        for(auto* field:fields)if(field->objectName().startsWith(prefix)) {
            const auto key=field->objectName().mid(prefix.size());
            if((tool.kind=="edge_point" || tool.kind=="edge_pair") && key=="width")continue;
            if(tool.kind=="circle_fit" && (key=="start" || key=="sweep"))continue;
            movePropertyRow(rows,field,manualRows);
        }
        auto* apply=editor->findChild<QPushButton*>(tool.kind=="edge_point" ? "edgeApplyScan":"metrologyApplyRoi");
        movePropertyRow(rows,apply,manualRows);
        // Move the drawing and manual section before algorithm settings.
        auto* draw=editor->findChild<QPushButton*>(tool.kind=="edge_point" ? "edgeDraw":"metrologyDraw");
        auto* toggle=editor->findChild<QPushButton*>(tool.kind+"/manualToggle");
        QFormLayout* presentation=rows;
        if(tool.kind=="edge_point") {
            auto* group=new QWidget;presentation=new QFormLayout(group);presentation->setContentsMargins(0,0,0,0);presentation->setRowWrapPolicy(QFormLayout::WrapAllRows);
            for(auto* widget:{static_cast<QWidget*>(draw),static_cast<QWidget*>(toggle),static_cast<QWidget*>(manual)})movePropertyRow(rows,widget,presentation);
            auto* settings=panel->findChild<QWidget*>("edgePointSettings");int index;QFormLayout::ItemRole role;rows->getWidgetPosition(settings,&index,&role);
            movePropertyRow(rows,settings,presentation);rows->insertRow(index,group);
        } else for(auto* widget:{static_cast<QWidget*>(manual),static_cast<QWidget*>(toggle),static_cast<QWidget*>(draw)}) {
            auto row=rows->takeRow(widget);delete row.labelItem;delete row.fieldItem;rows->insertRow(0,widget);
        }
        if(tool.kind=="circle_fit") {
            int index=3;
            for(const auto& key:{"start","sweep"}){auto* field=editor->findChild<QDoubleSpinBox*>(QString("mt_roi_")+key);auto row=rows->takeRow(field);
                rows->insertRow(index++,row.labelItem->widget(),field);delete row.labelItem;delete row.fieldItem;}
        }
        auto* settings=tool.kind=="edge_point" ? panel->findChild<QWidget*>("edgePointSettings"):editor;
        auto* settingsRows=static_cast<QFormLayout*>(settings->layout());
        QList<QWidget*> advanced;
        const auto defaults=defaultParameters(tool.kind);
        const bool fitTool=tool.kind=="line_fit" || tool.kind=="circle_fit";
        const QStringList basic=fitTool ? QStringList{"mode","direction","selection","polarity","minContrast","scanCount"}:QStringList{"mode","direction","selection","polarity","minContrast","scanCount","scanWidthPx","scanWidth","minWidthPx","maxWidthPx","lower","upper","angleLower","angleUpper","diameterLower","diameterUpper","deviationLower","deviationUpper"};
        if(fitTool) {
            int index=tool.kind=="circle_fit" ? 5:3;
            for(const auto& key:{"mode","direction","scanCount","polarity","selection","minContrast"})if(auto* field=settings->findChild<QWidget*>(QString("mt_")+key)) {
                auto row=settingsRows->takeRow(field);settingsRows->insertRow(index++,row.labelItem->widget(),field);delete row.labelItem;delete row.fieldItem;
            }
            if(auto* warning=settings->findChild<QLabel*>("metrologyPreflight")){auto row=settingsRows->takeRow(warning);settingsRows->insertRow(index,warning);delete row.labelItem;delete row.fieldItem;}
            const QVector<QPair<QString,QStringList>> groups{
                {"Gelişmiş tarama",{"scanWidthPx","scanWidth","sigmaPx"}},
                {"Fit / kontur kalitesi",{"inlierDistancePx","inlierDistanceMm","inlierDistance","maxRmsPx","maxRmsMm","maxRms","maxResidualPx","maxResidualMm","maxResidual","minPoints","minFoundPoints","minCoverage","minFitCoverage","minSpanCoverage","minSpanPx","minSpanMm","minAngularSpan","maxGapDegrees"}},
                {"Ürün sınırları",{"lower","upper","angleLower","angleUpper","diameterLower","diameterUpper","deviationLower","deviationUpper"}},
                {"Uzman örnekleme / solver",{"stepPx","widthStepPx","unitsPerSample","subpixel","ransac","maxCondition"}}
            };
            for(int i=0;i<groups.size();++i) {
                const QString key=tool.kind+"/measurementGroup"+QString::number(i);
                auto* groupHost=settingsRows;
                if(i==3){auto* expert=new QWidget;groupHost=new QFormLayout(expert);groupHost->setContentsMargins(0,0,0,0);settingsRows->addRow(expert);advanced.append(expert);}
                auto* body=propertySection(groupHost,groups[i].first,key,settingsFile_);auto* target=static_cast<QFormLayout*>(body->layout());
                for(const auto& name:groups[i].second)if(auto* field=settings->findChild<QWidget*>("mt_"+name))movePropertyRow(settingsRows,field,target);
                if(i==2)target->addRow(note("Bu sınırlar bulunan geometriyi değerlendirir; geometri bulmak için gevşetilmez. Line destek uzunluğu / yönlü açı; Circle çap / yüzey sapmasıdır."));
            }
        }
        for(auto it=defaults.begin();it!=defaults.end();++it)if(!basic.contains(it.key()))if(auto* field=settings->findChild<QWidget*>((tool.kind=="edge_point" ? "ep_":"mt_")+it.key()))if(settingsRows->indexOf(field)>=0)advanced.append(field);
        // Keep operational explanations and preflight visible in Basic.
        for(auto* label:settings->findChildren<QLabel*>(QString{},Qt::FindDirectChildrenOnly))if(settingsRows->labelForField(label)==nullptr && label->wordWrap() && !label->objectName().startsWith("metrology"))advanced.append(label);
        propertyDetailMode(settingsRows,tool.kind,settingsFile_,advanced,[settings,defaults,basic]{
            const auto p=dynamic_cast<EdgePointEditor*>(settings) ? static_cast<EdgePointEditor*>(settings)->parameters():static_cast<MetrologyEditor*>(settings)->parameters();
            for(auto it=defaults.begin();it!=defaults.end();++it)if(!basic.contains(it.key()) && it.key()!="scanRoi" && p.value(it.key())!=it.value())return true;
            return false;
        },presentation);
        if(fitTool)if(auto* customized=settings->findChild<QLabel*>("advancedCustomized")) {
            QStringList values;for(auto it=defaults.begin();it!=defaults.end();++it)if(!basic.contains(it.key()) && it.key()!="scanRoi" && tool.parameters.value(it.key())!=it.value()) {
                const auto value=tool.parameters.value(it.key());values<<it.key()+"="+(value.isDouble() ? QString::number(value.toDouble(),'g',12):value.isBool() ? (value.toBool() ? "true":"false"):value.toString());
            }
            customized->setText("Özel ayarlar korunuyor: "+values.join(", "));
        }
    }
    if(auto* image=panel->findChild<QComboBox*>("input_image")) {
        auto row=form->takeRow(image);form->insertRow(3,row.labelItem->widget(),image);delete row.labelItem;delete row.fieldItem;
        if(image->currentIndex()==0)image->setToolTip("Görüntü kaynağını seçin. Belirsiz veya eksik kaynak otomatik bağlanmaz.");
    }
    for(auto* label:panel->findChildren<QLabel*>())label->setWordWrap(true);
    for(auto* spin:panel->findChildren<QDoubleSpinBox*>())spin->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
    layout->addWidget(panel);
    layout->addWidget(note(tool.kind=="measurement" ? "İki gerçek DONE nokta ve güncel kalibrasyon gerekir. PASS/FAIL tek ölçüm kararıdır; reçete veya üretim izni değildir. Karşılaştırma ham mm değeriyle yapılır." : tool.kind=="image_file" ? "Görüntü piksel koordinatları kullanılır. Kamera bağlantısı yok." : tool.kind=="template_match" ? "Sabit açı/ölçek, tek sonuç. Skor olasılık değildir; sonuç Results sekmesinde." : "Run Flow ile gerçek sonucu hesaplayın. Parça kararı üretilmez."));
    connect(name,&QLineEdit::editingFinished,this,[this]{applyToolProperties();});
    connect(name,&QLineEdit::textEdited,this,[this]{applyToolProperties();});
    connect(description,&QLineEdit::editingFinished,this,[this]{applyToolProperties();});
    connect(description,&QLineEdit::textEdited,this,[this]{applyToolProperties();});
    connect(enabled,&QCheckBox::toggled,this,[this]{applyToolProperties();});
    auto* remove=new QPushButton("Remove Tool"); remove->setObjectName("removeTool"); layout->addWidget(remove);
    connect(remove,&QPushButton::clicked,this,[this]{try {service_.removeTool(selectedToolId_); selectedToolId_.clear(); refreshWorkspace();} catch (const std::exception& e) {displayError(QString::fromUtf8(e.what()));}});
    layout->addStretch();
    displayedProperties_=readToolProperties(tool);
}
ToolDefinition MainWindow::readToolProperties(ToolDefinition tool) const {
    tool.name=propertiesBody_->findChild<QLineEdit*>("toolName")->text().trimmed(); tool.description=propertiesBody_->findChild<QLineEdit*>("toolDescription")->text();
    tool.enabled=propertiesBody_->findChild<QCheckBox*>("toolEnabled")->isChecked();
    if (auto* score=propertiesBody_->findChild<QDoubleSpinBox*>("minScore")) tool.parameters["minScore"]=score->value()/100;
    if(tool.kind=="pattern_pose" || tool.kind=="shape_pose")if(auto* settings=dynamic_cast<PoseMatchEditor*>(propertiesBody_->findChild<QWidget*>("poseMatchSettings")))tool.parameters=settings->parameters();
    if (auto* threshold=propertiesBody_->findChild<QSpinBox*>("threshold")) tool.parameters["threshold"]=threshold->value();
    if(tool.kind=="edge_pair" || tool.kind=="line_fit" || tool.kind=="circle_fit")if(auto* settings=dynamic_cast<MetrologyEditor*>(propertiesBody_->findChild<QWidget*>("metrologySettings")))tool.parameters=settings->parameters();
    if(tool.kind=="edge_point")if(auto* settings=dynamic_cast<EdgePointEditor*>(propertiesBody_->findChild<QWidget*>("edgePointSettings")))tool.parameters=settings->parameters();
    if(tool.kind=="code_read")if(auto* settings=dynamic_cast<CodeReadEditor*>(propertiesBody_->findChild<QWidget*>("codeReadSettings")))tool.parameters=settings->parameters();
    if(tool.kind=="preprocess")if(auto* settings=dynamic_cast<PreprocessEditor*>(propertiesBody_->findChild<QWidget*>("preprocessSettings")))tool.parameters=settings->parameters();
    if(tool.kind=="blob") {
        tool.parameters["rejectBorderTouching"]=propertiesBody_->findChild<QCheckBox*>("rejectBorderTouching")->isChecked();
        tool.parameters["polarity"]=propertiesBody_->findChild<QComboBox*>("blobPolarity")->currentData().toString();
        tool.parameters["connectivity"]=propertiesBody_->findChild<QComboBox*>("blobConnectivity")->currentData().toInt();
        tool.parameters["minArea"]=propertiesBody_->findChild<QSpinBox*>("minArea")->value();
        tool.parameters["maxArea"]=propertiesBody_->findChild<QCheckBox*>("blobUnlimited")->isChecked() ? QJsonValue(QJsonValue::Null) : QJsonValue(propertiesBody_->findChild<QSpinBox*>("maxArea")->value());
    }
    if (auto* direction=propertiesBody_->findChild<QComboBox*>("edgeDirection")) tool.parameters["direction"]=direction->currentData().toString();
    if(auto* contrast=propertiesBody_->findChild<QDoubleSpinBox*>("minContrast"))tool.parameters["minContrast"]=contrast->value();
    if(auto* index=propertiesBody_->findChild<QSpinBox*>("webcamIndex"))tool.parameters["deviceIndex"]=index->value();
    if(auto* timeout=propertiesBody_->findChild<QSpinBox*>("webcamTimeout"))tool.parameters["timeoutMs"]=timeout->value();
    for(const auto key:{"lowerMm","upperMm"})if(auto* value=propertiesBody_->findChild<QDoubleSpinBox*>(key))tool.parameters[key]=value->value();
    if(tool.kind=="geometry_measurement"){
        auto p=tool.parameters;const auto defaults=geometryMeasurementDefaults();
        for(auto i=defaults.begin();i!=defaults.end();++i){const auto id="gm_"+i.key();
            if(auto* c=propertiesBody_->findChild<QComboBox*>(id))p[i.key()]=c->currentText();
            else if(auto* c=propertiesBody_->findChild<QCheckBox*>(id))p[i.key()]=c->isChecked();
            else if(auto* n=propertiesBody_->findChild<QDoubleSpinBox*>(id))p[i.key()]=n->value();
        }
        if(p["operation"]=="line_intersection")p["toleranceEnabled"]=false;
        if(!tool.parameters.contains("toleranceEnabled") && p["toleranceEnabled"]==defaults["toleranceEnabled"] && p["nominal"]==defaults["nominal"] && p["lower"]==defaults["lower"] && p["upper"]==defaults["upper"])
            for(auto key:{"toleranceEnabled","nominal","lower","upper"})p.remove(key);
        tool.parameters=p;
    }
    return tool;
}
MainWindow::PropertyCommit MainWindow::commitToolProperties() {
    if (propertiesEditing_ || !service_.activeRecipe() || !displayedProperties_ || displayedProperties_->id!=selectedToolId_) return PropertyCommit::Unchanged;
    try {
        for (const auto& original : service_.tools()) if (original.id==selectedToolId_) {
            const auto displayed=readToolProperties(original);
            auto tool=original;
            // Compare the current controls to their initial normalized display. Unedited
            // defaults, list ordering and rounded numbers retain their serialized form.
            if(displayed.name!=displayedProperties_->name)tool.name=displayed.name;
            if(displayed.description!=displayedProperties_->description)tool.description=displayed.description;
            if(displayed.enabled!=displayedProperties_->enabled)tool.enabled=displayed.enabled;
            for(auto i=displayed.parameters.begin();i!=displayed.parameters.end();++i)
                if(i.value()!=displayedProperties_->parameters.value(i.key()))tool.parameters[i.key()]=i.value();
            for(auto i=displayedProperties_->parameters.begin();i!=displayedProperties_->parameters.end();++i)
                if(!displayed.parameters.contains(i.key()))tool.parameters.remove(i.key());
            if(tool.name==original.name && tool.description==original.description && tool.enabled==original.enabled && tool.parameters==original.parameters) {
                displayedProperties_=displayed;
                return PropertyCommit::Unchanged;
            }
            const bool geometryChange=tool.kind=="geometry_measurement" &&
                (tool.parameters["operation"]!=original.parameters["operation"] || tool.parameters["mode"]!=original.parameters["mode"]);
            if(tool.kind=="geometry_measurement")validateGeometryMeasurement(tool.parameters);
            if(geometryChange) {
                auto links=editorConnections(service_.activeRecipe()->definition);
                links.erase(std::remove_if(links.begin(),links.end(),[&](const auto& c){return c.targetTool==tool.id && c.targetPort!="image";}),links.end());
                auto candidate=service_;
                candidate.updateTool(tool);
                candidate.setConnections(links);
                service_=std::move(candidate);
                pendingDrawTool_.clear();
            } else service_.updateTool(tool);
            displayedProperties_=displayed;
            return PropertyCommit::Changed;
        }
    } catch (const std::exception& e) {
        if(auto* mode=propertiesBody_->findChild<QComboBox*>("propertyDetailMode"))mode->setCurrentIndex(1);
        displayError(QString::fromUtf8(e.what()));
        return PropertyCommit::Failed;
    }
    return PropertyCommit::Unchanged;
}
bool MainWindow::applyToolProperties() {
    const auto result=commitToolProperties();
    if(result==PropertyCommit::Changed) {
        const QScopedValueRollback<bool> guard(propertiesEditing_,true);
        refreshWorkspace();
        refreshOutputVisibilityControls();
    }
    return result!=PropertyCommit::Failed;
}
void MainWindow::addSelectedTool() {
    if (!toolbox_->currentItem() || toolbox_->currentItem()->isHidden() || !(toolbox_->currentItem()->flags() & Qt::ItemIsEnabled) || !ToolCatalog::canAdd(toolbox_->currentItem()->data(Qt::UserRole).toString())) return;
    if(editorSession_.busy())return;
    const auto commit=commitToolProperties();
    if(commit==PropertyCommit::Failed)return;
    try {const auto previous=service_.tools();selectedToolId_=service_.addTool(toolbox_->currentItem()->data(Qt::UserRole).toString());
        if(std::none_of(previous.begin(),previous.end(),[this](const auto& t){return t.id==selectedToolId_;}))for(auto tool:service_.tools())if(tool.id==selectedToolId_ && QStringList{"edge_pair","line_fit","geometry_measurement"}.contains(tool.kind)){tool.parameters["mode"]="calibrated";if(tool.kind=="circle_fit"){tool.parameters["unitsPerSample"]=.1;tool.parameters["scanWidth"]=.1;tool.parameters["inlierDistance"]=.1;tool.parameters["maxRms"]=.035;tool.parameters["maxResidual"]=.1;}service_.updateTool(tool);break;}
        imageViewer_->cancelRoi();pendingDrawTool_.clear();
        if(std::none_of(previous.begin(),previous.end(),[this](const auto& t){return t.id==selectedToolId_;})){pendingDrawTool_=selectedToolId_;pendingDrawRecipe_=service_.project().id+"/"+service_.activeRecipe()->id;}
        refreshWorkspace();}
    catch (const std::exception& e) {if(commit==PropertyCommit::Changed){const QScopedValueRollback<bool> guard(propertiesEditing_,true);refreshWorkspace();}displayError(QString::fromUtf8(e.what()));}
}
void MainWindow::openImageDialog() {
    if (!service_.activeRecipe()) return;
    if (!applyToolProperties()) return;
    const auto path=QFileDialog::getOpenFileName(this,"Open Image",QFileInfo(service_.path()).absolutePath(),"Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All files (*)");
    if (!path.isEmpty()) loadEditorImage(path);
}
bool MainWindow::setDrawingImage() {
    try {
        const auto image=editorSession_.toolImage(editorContext(),selectedToolId_);
        drawingImage_=image;imageViewer_->setPreviewPixels(image.preview());
        findChild<QPushButton*>("stopFlow")->setEnabled(true);
        imageInfo_->setText(image.processing().isEmpty() ? "Ham görüntü · bağlı kaynak" : "İşlenmiş görüntü · bağlı Preprocess");
        return true;
    }catch(const std::exception& e){matchInfo_->setText(QString::fromUtf8(e.what()));return false;}
}
void MainWindow::prepareNewToolDrawing() {
    if(pendingDrawTool_.isEmpty())return;
    if(!service_.activeRecipe() || selectedToolId_!=pendingDrawTool_ || pendingDrawRecipe_!=service_.project().id+"/"+service_.activeRecipe()->id) {pendingDrawTool_.clear();return;}
    QString action;
    for(const auto& tool:service_.tools())if(tool.id==pendingDrawTool_ && tool.enabled && !tool.parameters.contains("poseTracking") && !tool.parameters.contains("referenceBinding")) {
        if(tool.kind=="edge_point")action="edgeDraw";
        else if(tool.kind=="edge_pair" || tool.kind=="line_fit" || tool.kind=="circle_fit")action="metrologyDraw";
        else if(tool.kind=="pattern_pose" || tool.kind=="shape_pose" || tool.kind=="template_match")action="modelRoi";
        else if(tool.kind=="roi" || tool.kind=="blob" || tool.kind=="edge" || tool.kind=="code_read")action="searchRoi";
    }
    if(action.isEmpty()){pendingDrawTool_.clear();return;}
    if(editorSession_.busy())return;
    if(!setDrawingImage()) {const QString text="Çizim için görüntü gerekli. Görüntü kaynağını bağlayıp kaynak/Preprocess çalıştırın.";matchInfo_->setText(text);if(auto* hint=propertiesBody_->findChild<QLabel*>("newToolDrawHint")){hint->setText(text);hint->show();}return;}
    if(auto* hint=propertiesBody_->findChild<QLabel*>("newToolDrawHint"))hint->hide();
    pendingDrawTool_.clear();
    if(auto* button=propertiesBody_->findChild<QPushButton*>(action)) {button->setEnabled(true);button->click();findChild<QPushButton*>("stopFlow")->setEnabled(true);}
}
bool MainWindow::loadEditorImage(const QString& path) {
    if(!applyToolProperties() || editorSession_.busy(SessionJob::Image) || !service_.activeRecipe())return false;
    editorRecipeId_=service_.project().id+"/"+service_.activeRecipe()->id;
    loadImageAsync(path,true);
    return !smokeMode_ || (editorSession_.frame && editorSession_.frame->path()==QFileInfo(path).absoluteFilePath());
}
}
