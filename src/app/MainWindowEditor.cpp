#include "application/CalibrationWorkflow.hpp"
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
#include "domain/GeometryMeasurementConfig.hpp"
#include "EdgeProfileView.hpp"
#include "application/BackendCapabilities.hpp"
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
#include <QTimer>
#include <QJsonDocument>
#include <exception>
namespace visionflow {
namespace {
QLabel* note(const QString& value) { auto* result=new QLabel(value); result->setWordWrap(true); result->setTextFormat(Qt::PlainText); result->setObjectName("muted"); return result; }
}
QWidget* MainWindow::makeEditor() {
    auto* page=new QWidget; auto* layout=new QVBoxLayout(page); layout->setContentsMargins(0,0,0,0);
    auto* commands=new QHBoxLayout;
    auto* back=new QPushButton("← Recipes"); commands->addWidget(back); connect(back,&QPushButton::clicked,this,[this]{selectPage("recipes");});
    auto* edit=new QPushButton("Reçete bilgileri"); edit->setObjectName("editActiveRecipe"); commands->addWidget(edit);
    connect(edit,&QPushButton::clicked,this,[this]{showRecipeDialog(true);});
    auto* save=new QPushButton("Save"); save->setObjectName("saveWorkspace"); commands->addWidget(save); connect(save,&QPushButton::clicked,this,&MainWindow::saveProject);
    auto* runFlowButton=new QPushButton("Run Flow"); runFlowButton->setObjectName("runFlow"); commands->addWidget(runFlowButton); connect(runFlowButton,&QPushButton::clicked,this,&MainWindow::runFlow);
    runFlowButton->setToolTip("Yeni görüntü edin ve tam akışı çalıştır.");
    auto* runTool=new QPushButton("Run Tool");runTool->setObjectName("runTool");commands->addWidget(runTool);
    connect(runTool,&QPushButton::clicked,this,[this]{for(const auto& t:service_.tools())if(t.id==selectedToolId_){runExecution(t.id);return;}});
    auto* stop=new QPushButton("Stop");stop->setObjectName("stopFlow");commands->addWidget(stop);connect(stop,&QPushButton::clicked,this,[this]{pendingDrawTool_.clear();imageViewer_->cancelRoi();imageViewer_->drawRegion(false);editorSession_.stop(SessionJob::Execution);editorSession_.stop(SessionJob::Teaching);});
    commands->addStretch(); layout->addLayout(commands); editorContext_=note(""); layout->addWidget(editorContext_);
    horizontal_=new QSplitter(Qt::Horizontal); horizontal_->setObjectName("editorHorizontal"); horizontal_->setChildrenCollapsible(false);
    auto* toolsPanel=new QWidget; toolsPanel->setMinimumWidth(150); auto* toolsLayout=new QVBoxLayout(toolsPanel);
    toolsLayout->addWidget(note("TOOLBOX")); auto* search=new QLineEdit; search->setPlaceholderText("Tool ara…"); toolsLayout->addWidget(search);
    toolbox_=new QListWidget; toolbox_->setObjectName("toolbox"); toolbox_->setAccessibleName("Toolbox");
    for (const auto& descriptor : ToolRegistry::entries()) {
        const auto kind=descriptor.kind;
        auto* item=new QListWidgetItem(toolLabel(kind),toolbox_); item->setData(Qt::UserRole,QString(kind));
        if(kind=="code_read" && !BackendCapabilities::get("zxing-reader").available){item->setFlags(item->flags() & ~Qt::ItemIsEnabled);item->setToolTip("Kod okuyucu backend bu derlemede yok; araç eklenemez.");continue;}
        item->setToolTip(QString(kind)=="image_file" ? "Dosyadan görüntü edinme" : QString(kind)=="template_match" ? "ROI/model öğretme ve sabit açı/ölçek eşleştirme" : "Gerçek tool yürütmesi; girişleri Properties üzerinden bağlayın.");
    }
    toolbox_->setCurrentRow(0); toolsLayout->addWidget(toolbox_,1);
    auto* add=new QPushButton("Add Tool"); add->setObjectName("addTool"); toolsLayout->addWidget(add);
    connect(add,&QPushButton::clicked,this,&MainWindow::addSelectedTool);
    connect(toolbox_,&QListWidget::itemDoubleClicked,this,[this]{addSelectedTool();});
    connect(search,&QLineEdit::textChanged,this,[this](const QString& query){for (int i=0;i<toolbox_->count();++i) toolbox_->item(i)->setHidden(!toolbox_->item(i)->text().contains(query,Qt::CaseInsensitive));});
    horizontal_->addWidget(toolsPanel);
    auto* imagePanel=new QWidget; auto* imageLayout=new QVBoxLayout(imagePanel); imageLayout->setContentsMargins(0,0,0,0);
    auto* toolbar=new QHBoxLayout;
    auto* open=new QPushButton("Open Image"); open->setObjectName("openImage"); toolbar->addWidget(open); connect(open,&QPushButton::clicked,this,&MainWindow::openImageDialog);
    auto* reload=new QPushButton("Reload"); reload->setObjectName("reloadImage"); toolbar->addWidget(reload);
    connect(reload,&QPushButton::clicked,this,[this]{imageBinding_.clear(); refreshWorkspace();});
    imageViewer_=new ImageViewer; imageViewer_->setObjectName("imageViewer");imageViewer_->setMinimumHeight(80);
    auto* fit=new QPushButton("Fit"); fit->setObjectName("fitImage"); toolbar->addWidget(fit); connect(fit,&QPushButton::clicked,this,[this]{imageViewer_->fitImage();});
    auto* minus=new QPushButton("−"); minus->setObjectName("zoomOut"); toolbar->addWidget(minus); connect(minus,&QPushButton::clicked,this,[this]{imageViewer_->zoomBy(1.0/1.25);});
    auto* plus=new QPushButton("+"); plus->setObjectName("zoomIn"); toolbar->addWidget(plus); connect(plus,&QPushButton::clicked,this,[this]{imageViewer_->zoomBy(1.25);});
    auto* actual=new QPushButton("1:1"); toolbar->addWidget(actual); connect(actual,&QPushButton::clicked,this,[this]{imageViewer_->actualSize();});
    auto* geometry=new QPushButton("ROI geometrisi…");geometry->setObjectName("roiGeometry");geometry->setToolTip("Geçici subpixel geometri; reçeteye bağlı değildir.");toolbar->addWidget(geometry);
    connect(geometry,&QPushButton::clicked,this,[this]{if(editorSession_.frame){showRoiEditor(imageViewer_,this,editorSession_.frame->metadata().coordinateSystemId.isEmpty() ? "viewer/raw":editorSession_.frame->metadata().coordinateSystemId);if(auto* dialog=findChild<QDialog*>("roiGeometryDialog"))if(!dialog->property("restoreRecipeRoi").toBool()){dialog->setProperty("restoreRecipeRoi",true);connect(dialog,&QDialog::finished,this,[this]{refreshMatchOverlay();});}}});
    toolbar->addStretch();auto* toolbarBody=new QWidget;toolbarBody->setLayout(toolbar);toolbar->setSizeConstraint(QLayout::SetMinimumSize);
    auto* toolbarScroll=new QScrollArea;toolbarScroll->setWidgetResizable(true);toolbarScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);toolbarScroll->setFixedHeight(60);toolbarScroll->setWidget(toolbarBody);imageLayout->addWidget(toolbarScroll);
    imageInfo_=note("Görüntü yok."); imageLayout->addWidget(imageInfo_); imageLayout->addWidget(imageViewer_,1);
    auto* footer=new QHBoxLayout; pixelInfo_=note("Piksel: —"); zoomInfo_=note("Zoom: —"); footer->addWidget(pixelInfo_,1); footer->addWidget(zoomInfo_); imageLayout->addLayout(footer);
    imageViewer_->onZoom=[this](double zoom){zoomInfo_->setText(QString("Zoom: %1%").arg(zoom*100,0,'f',1)); pixelInfo_->setText("Piksel: —");};
    imageViewer_->onPixel=[this](QPoint pixel){const ImageFrame* displayed=drawingImage_ ? &*drawingImage_:editorSession_.frame ? &*editorSession_.frame:nullptr;if(editorSession_.cycle)for(const auto& r:editorSession_.cycle->tools)if(r.toolId==selectedToolId_){auto it=r.outputs.constFind("image");if(it!=r.outputs.cend() && it->valid)if(const auto* image=std::get_if<ImageFrame>(&it->data))displayed=image;}pixelInfo_->setText(displayed && pixel.x()>=0 ? QString("X:%1 Y:%2 px  |  %3").arg(pixel.x()).arg(pixel.y()).arg(displayed->pixelText(pixel.x(),pixel.y())) : "Piksel: —");};
    imageViewer_->onImageClicked={};
    horizontal_->addWidget(imagePanel);
    auto* scroll=new QScrollArea; scroll->setObjectName("propertiesScroll"); scroll->setWidgetResizable(true); scroll->setMinimumWidth(210);
    propertiesBody_=new QWidget; new QVBoxLayout(propertiesBody_); scroll->setWidget(propertiesBody_); horizontal_->addWidget(scroll);
    horizontal_->setStretchFactor(1,1);
    auto* tabs=new QTabWidget; tabs->setObjectName("editorTabs"); tabs->setMinimumHeight(120);
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
    for (const auto name : {"Log","Variables","Communication","Diagnostics"}) tabs->addTab(note(QString(name)+" — henüz veri yok."),name);
    vertical_=new QSplitter(Qt::Vertical); vertical_->setObjectName("editorVertical"); vertical_->setChildrenCollapsible(false);
    vertical_->addWidget(horizontal_); vertical_->addWidget(tabs); vertical_->setStretchFactor(0,1); layout->addWidget(vertical_,1);
    horizontal_->setSizes({180,680,300}); vertical_->setSizes({540,220});
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
    const auto* recipe=service_.activeRecipe(); const QString recipeId=recipe ? service_.project().id+"/"+recipe->id : "";
    if (recipeId!=editorRecipeId_) {pendingDrawTool_.clear();imageViewer_->cancelRoi();selectedToolId_.clear(); editorRecipeId_=recipeId; imageBinding_.clear();}
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
    if (binding!=imageBinding_) {
        ++editorSession_.generation; invalidateMatch();
        imageBinding_=binding; editorSession_.frame.reset();
        imageViewer_->clearImage(webcamSource ? "Run Flow ile webcam'den tek kare alın." : recipe ? "Open Image ile görüntü yükleyin." : "Önce bir reçete açın."); imageInfo_->setText("Görüntü yok.");
        if (!path.isEmpty()) {
            loadImageAsync(path,false);
        }
    }
    for (const auto name : {"openImage","reloadImage","addTool"}) if (auto* button=findChild<QPushButton*>(name)) button->setEnabled(recipe!=nullptr);
    if (!propertiesEditing_) showToolProperties();
    bool sourceSelected=false; for (const auto& t : tools) if (t.id==selectedToolId_ && t.kind=="image_file") sourceSelected=true;
    imageViewer_->setSourceSelected(sourceSelected);
    refreshMatchOverlay();
    refreshFlow();
}
void MainWindow::selectTool(const QString& id) {
    if (id!=selectedToolId_ && !applyToolProperties()) {
        const QSignalBlocker blocker(flowTools_);
        for (int i=0;i<flowTools_->count();++i) if (flowTools_->item(i)->data(Qt::UserRole).toString()==selectedToolId_) flowTools_->setCurrentRow(i);
        return;
    }
    if(id!=selectedToolId_){pendingDrawTool_.clear();imageViewer_->cancelRoi();imageViewer_->resetRoiHistory();}
    selectedToolId_=id;
    imageViewer_->drawRegion(false);
    const QSignalBlocker blocker(flowTools_); flowTools_->clearSelection(); flowTools_->setCurrentItem(nullptr);
    for (int i=0;i<flowTools_->count();++i) if (flowTools_->item(i)->data(Qt::UserRole).toString()==id) flowTools_->setCurrentRow(i);
    showToolProperties();
    bool source=false; if (service_.activeRecipe()) for (const auto& t : service_.tools()) if (t.id==id && t.kind=="image_file") source=true;
    imageViewer_->setSourceSelected(source);
    refreshMatchOverlay();
    refreshFlow();
}
void MainWindow::showToolProperties() {
    const QScopedValueRollback<bool> guard(propertiesEditing_,true);
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
    if (tool.kind=="image_file") {
        auto* path=new QLineEdit(tool.parameters["path"].toString()); path->setReadOnly(true); path->setCursorPosition(0); path->setPlaceholderText("Seçilmedi"); path->setToolTip(path->text()); form->addRow("File",path);
        auto* choose=new QPushButton("Open Image…"); form->addRow(choose); connect(choose,&QPushButton::clicked,this,&MainWindow::openImageDialog);
    } else if(tool.kind=="webcam") {
        auto* index=new QSpinBox;index->setObjectName("webcamIndex");index->setRange(0,63);index->setValue(tool.parameters["deviceIndex"].toInt());form->addRow("Device index",index);
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
        settings->onRoi=[this,kind=tool.kind](const ToolRoi& value){try{auto roi=value;if(editorSession_.busy() || !applyToolProperties())return;if(kind=="circle_fit")for(const auto& t:service_.tools())if(t.id==selectedToolId_ && t.parameters["mode"]=="calibrated"){auto ctx=calibrationResolver(*service_.activeRecipe())(*editorSession_.frame);if(!ctx.calibration || !ctx.current)throw std::runtime_error("Circle teaching requires current calibration");roi.frame=ctx.calibration->dependency().target;}if(!editorSession_.frame || (roi.frame.unit==LengthUnit::Pixel && !roi.inside(editorSession_.frame->size())))throw std::runtime_error("ROI görüntü sınırı dışında");imageViewer_->setRoi(roi);}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}};
        form->addRow("Kalibrasyon",note(calibrationSummary()));
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
    } else if(tool.kind=="preprocess") {
        auto* settings=new PreprocessEditor(tool.parameters);form->addRow(settings);settings->onChanged=[this]{applyToolProperties();};
    } else if (tool.kind=="blob") {
        auto* value=new QSpinBox; value->setObjectName("threshold"); value->setRange(0,255); value->setValue(tool.parameters["threshold"].toInt()); form->addRow("Threshold (8-bit)",value);
        connect(value,&QSpinBox::valueChanged,this,[this]{applyToolProperties();});
        auto* polarity=new QComboBox;polarity->setObjectName("blobPolarity");polarity->addItem("Açık (> eşik)","bright");polarity->addItem("Koyu (≤ eşik)","dark");polarity->setCurrentIndex(polarity->findData(tool.parameters["polarity"].toString()));form->addRow("Nesne",polarity);
        auto* connectivity=new QComboBox;connectivity->setObjectName("blobConnectivity");connectivity->addItem("4",4);connectivity->addItem("8",8);connectivity->setCurrentIndex(connectivity->findData(tool.parameters["connectivity"].toInt()));form->addRow("Komşuluk",connectivity);
        for(const auto key:{"minArea","maxArea"}){auto* area=new QSpinBox;area->setObjectName(key);area->setRange(1,2147483647);area->setSuffix(" px²");area->setValue(tool.parameters[key].isNull() ? tool.parameters["minArea"].toInt() : tool.parameters[key].toInt());area->setEnabled(QString(key)!="maxArea" || !tool.parameters["maxArea"].isNull());form->addRow(QString(key)=="minArea" ? "Min Area (dahil)" : "Max Area (dahil)",area);connect(area,&QSpinBox::editingFinished,this,[this]{applyToolProperties();});}
        auto* unlimited=new QCheckBox("Max Area: Sınırsız");unlimited->setObjectName("blobUnlimited");unlimited->setChecked(tool.parameters["maxArea"].isNull());form->addRow(unlimited);
        connect(unlimited,&QCheckBox::toggled,this,[this]{applyToolProperties();});
        connect(polarity,&QComboBox::currentIndexChanged,this,[this]{applyToolProperties();});connect(connectivity,&QComboBox::currentIndexChanged,this,[this]{applyToolProperties();});
        form->addRow(note("Alan filtresi adayları seçer; PASS/FAIL üretmez. Kırmızı maske gerçek kabul edilen piksellerdir. Delikler korunur. Bbox: en çok 2048; ROI: en çok 16 Mi piksel."));
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
        form->addRow("Kalibrasyon",note(calibrationSummary()));
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
    for(const auto& output:outputPorts(tool.kind))outputForm->addRow(output.id,note(portTypeName(output.type)+" · "+output.unit+" · "+output.frame));
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
        auto* settings=tool.kind=="edge_point" ? panel->findChild<QWidget*>("edgePointSettings"):editor;
        auto* settingsRows=static_cast<QFormLayout*>(settings->layout());
        QList<QWidget*> advanced;
        const auto defaults=defaultParameters(tool.kind);
        const QStringList basic{"mode","direction","selection","polarity","minContrast","scanCount","scanWidthPx","scanWidth","minWidthPx","maxWidthPx","lower","upper","angleLower","angleUpper","diameterLower","diameterUpper","deviationLower","deviationUpper"};
        for(auto it=defaults.begin();it!=defaults.end();++it)if(!basic.contains(it.key()))if(auto* field=settings->findChild<QWidget*>((tool.kind=="edge_point" ? "ep_":"mt_")+it.key()))advanced.append(field);
        // Technical help follows the same view preference.
        for(auto* label:settings->findChildren<QLabel*>(QString{},Qt::FindDirectChildrenOnly))if(settingsRows->labelForField(label)==nullptr && label->wordWrap())advanced.append(label);
        propertyDetailMode(settingsRows,tool.kind,settingsFile_,advanced,[settings,defaults,basic]{
            const auto p=dynamic_cast<EdgePointEditor*>(settings) ? static_cast<EdgePointEditor*>(settings)->parameters():static_cast<MetrologyEditor*>(settings)->parameters();
            for(auto it=defaults.begin();it!=defaults.end();++it)if(!basic.contains(it.key()) && it.key()!="scanRoi" && p.value(it.key())!=it.value())return true;
            return false;
        },presentation);
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
}
bool MainWindow::applyToolProperties() {
    if (propertiesEditing_ || !service_.activeRecipe()) return true;
    auto* name=propertiesBody_->findChild<QLineEdit*>("toolName"); if (!name) return true;
    try {
        for (auto tool : service_.tools()) if (tool.id==selectedToolId_) {
            tool.name=name->text().trimmed(); tool.description=propertiesBody_->findChild<QLineEdit*>("toolDescription")->text();
            tool.enabled=propertiesBody_->findChild<QCheckBox*>("toolEnabled")->isChecked();
            if (auto* score=propertiesBody_->findChild<QDoubleSpinBox*>("minScore")) tool.parameters["minScore"]=score->value()/100;
            if(tool.kind=="pattern_pose" || tool.kind=="shape_pose")if(auto* settings=dynamic_cast<PoseMatchEditor*>(propertiesBody_->findChild<QWidget*>("poseMatchSettings")))tool.parameters=settings->parameters();
            if (auto* threshold=propertiesBody_->findChild<QSpinBox*>("threshold")) tool.parameters["threshold"]=threshold->value();
            if(tool.kind=="edge_pair" || tool.kind=="line_fit" || tool.kind=="circle_fit")if(auto* settings=dynamic_cast<MetrologyEditor*>(propertiesBody_->findChild<QWidget*>("metrologySettings")))tool.parameters=settings->parameters();
            if(tool.kind=="edge_point")if(auto* settings=dynamic_cast<EdgePointEditor*>(propertiesBody_->findChild<QWidget*>("edgePointSettings")))tool.parameters=settings->parameters();
            if(tool.kind=="code_read")if(auto* settings=dynamic_cast<CodeReadEditor*>(propertiesBody_->findChild<QWidget*>("codeReadSettings")))tool.parameters=settings->parameters();
            if(tool.kind=="preprocess")if(auto* settings=dynamic_cast<PreprocessEditor*>(propertiesBody_->findChild<QWidget*>("preprocessSettings")))tool.parameters=settings->parameters();
            if(tool.kind=="blob") {
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
                validateGeometryMeasurement(p);
                const bool changed=p["operation"]!=tool.parameters["operation"] || p["mode"]!=tool.parameters["mode"];
                tool.parameters=p;
                if(changed){
                    pendingDrawTool_.clear();
                    auto list=editorConnections(service_.activeRecipe()->definition);
                    list.erase(std::remove_if(list.begin(),list.end(),[&](const auto& c){return c.targetTool==tool.id && c.targetPort!="image";}),list.end());
                    service_.updateTool(tool);service_.setConnections(list);
                }
            }
            service_.updateTool(tool); propertiesEditing_=true; refreshWorkspace(); propertiesEditing_=false; return true;
        }
    } catch (const std::exception& e) {propertiesEditing_=false;if(auto* mode=propertiesBody_->findChild<QComboBox*>("propertyDetailMode"))mode->setCurrentIndex(1); displayError(QString::fromUtf8(e.what())); return false;}
    return true;
}
void MainWindow::addSelectedTool() {
    if (!applyToolProperties()) return;
    if (!toolbox_->currentItem() || toolbox_->currentItem()->isHidden()) return;
    if(editorSession_.busy())return;
    try {const auto previous=service_.tools();selectedToolId_=service_.addTool(toolbox_->currentItem()->data(Qt::UserRole).toString());
        if(std::none_of(previous.begin(),previous.end(),[this](const auto& t){return t.id==selectedToolId_;}))for(auto tool:service_.tools())if(tool.id==selectedToolId_ && QStringList{"edge_pair","line_fit","circle_fit","geometry_measurement"}.contains(tool.kind)){tool.parameters["mode"]="calibrated";if(tool.kind=="circle_fit"){tool.parameters["unitsPerSample"]=.1;tool.parameters["scanWidth"]=.1;tool.parameters["inlierDistance"]=.1;tool.parameters["maxRms"]=.035;tool.parameters["maxResidual"]=.1;}service_.updateTool(tool);break;}
        imageViewer_->cancelRoi();pendingDrawTool_.clear();
        if(std::none_of(previous.begin(),previous.end(),[this](const auto& t){return t.id==selectedToolId_;})){pendingDrawTool_=selectedToolId_;pendingDrawRecipe_=service_.project().id+"/"+service_.activeRecipe()->id;}
        refreshWorkspace();}
    catch (const std::exception& e) {displayError(QString::fromUtf8(e.what()));}
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
        else if(tool.kind=="blob" || tool.kind=="edge" || tool.kind=="code_read")action="searchRoi";
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
