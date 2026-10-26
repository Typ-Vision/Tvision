#include "MainWindow.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <limits>
#include <stdexcept>

namespace visionflow {
namespace {
const QStringList types={"Boolean","Int32","UInt32","Int64","Float64","String","Enum","SpatialTransform"};
const QStringList units={"unitless","mm","m","rad","degree"};
const QStringList validity={"Valid","Missing","Stale","Skipped","Invalid","Error"};
QString cellValue(const AssignmentValue& value) {
    const auto object=encodeAssignmentValue(value);
    const auto data=object["kind"]=="scalar" ? object["envelope"].toObject()["value"]:object["transform"];
    if(data.isString())return data.toString();
    if(data.isBool())return data.toBool() ? "true":"false";
    if(data.isDouble())return QString::number(data.toDouble(),'g',17);
    return QString::fromUtf8(QJsonDocument(data.toObject()).toJson(QJsonDocument::Compact));
}
}
QWidget* MainWindow::makeVariableCatalog() {
    auto* page=new QWidget;page->setObjectName("variableCatalog");
    auto* layout=new QVBoxLayout(page);layout->setContentsMargins(4,4,4,4);
    auto* commands=new QHBoxLayout;
    auto* add=new QPushButton("Ekle");add->setObjectName("variableAdd");
    auto* edit=new QPushButton("Düzenle");edit->setObjectName("variableEdit");
    auto* remove=new QPushButton("Sil");remove->setObjectName("variableRemove");
    for(auto* b:{add,edit,remove}){b->setFocusPolicy(Qt::TabFocus);commands->addWidget(b);}
    commands->addStretch();layout->addLayout(commands);
    variableTable_=new QTableWidget;variableTable_->setObjectName("variableTable");
    variableTable_->setColumnCount(10);
    variableTable_->setMinimumHeight(150);
    variableTable_->setHorizontalHeaderLabels({"Ad","Tip","Kapsam","Birim","Değer","Geçerlilik / neden","Kaynak","Erişim","Cycle ID","Açıklama"});
    variableTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    variableTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    variableTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    variableTable_->setTextElideMode(Qt::ElideNone);
    variableTable_->setWordWrap(false);
    variableTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    variableTable_->setColumnWidth(0,260);variableTable_->setColumnWidth(1,145);
    variableTable_->setColumnWidth(4,240);variableTable_->setColumnWidth(5,260);
    variableTable_->setColumnWidth(6,260);variableTable_->setColumnWidth(8,260);variableTable_->setColumnWidth(9,320);
    layout->addWidget(variableTable_,1);
    auto* error=new QLabel;error->setObjectName("variableError");error->setTextFormat(Qt::PlainText);error->setWordWrap(true);error->hide();layout->addWidget(error);
    connect(add,&QPushButton::clicked,this,[this]{editVariable();});
    connect(edit,&QPushButton::clicked,this,[this]{if(auto* item=variableTable_->item(variableTable_->currentRow(),0))if(!item->data(Qt::UserRole).toString().isEmpty())editVariable(item->data(Qt::UserRole).toString());});
    connect(remove,&QPushButton::clicked,this,&MainWindow::removeSelectedVariable);
    connect(variableTable_,&QTableWidget::cellDoubleClicked,this,[edit](int,int){if(edit->isEnabled())edit->click();});
    connect(variableTable_,&QTableWidget::itemSelectionChanged,this,[this,edit,remove]{
        const auto* item=variableTable_->item(variableTable_->currentRow(),0);
        const bool writable=service_.activeRecipe() && !editorSession_.busy() && item && !item->data(Qt::UserRole).toString().isEmpty();
        edit->setEnabled(writable);remove->setEnabled(writable);
    });
    return page;
}
void MainWindow::refreshVariableCatalog(const EditorContext& context) {
    if(!variableTable_)return;
    const auto* selected=variableTable_->item(variableTable_->currentRow(),0);
    const auto selection=selected ? selected->data(Qt::UserRole).toString():QString{};
    const int scroll=variableTable_->verticalScrollBar()->value();
    variableTable_->setRowCount(0);
    const bool available=service_.activeRecipe();
    auto* message=findChild<QLabel*>("variableError");
    if(message->property("recipeId").toString()!=context.recipeId){message->hide();message->setProperty("recipeId",context.recipeId);}
    findChild<QPushButton*>("variableAdd")->setEnabled(available && !editorSession_.busy());
    findChild<QPushButton*>("variableEdit")->setEnabled(false);findChild<QPushButton*>("variableRemove")->setEnabled(false);
    if(!available)return;
    const auto* cycle=editorSession_.cycle && editorSession_.accepts(editorSession_.cycle->configId,context) && !editorSession_.busy() ? &*editorSession_.cycle:nullptr;
    auto append=[&](const QStringList& cells,const QString& id){
        const int row=variableTable_->rowCount();variableTable_->insertRow(row);
        for(int col=0;col<cells.size();++col){auto* item=new QTableWidgetItem(cells[col]);item->setToolTip(cells[col]);variableTable_->setItem(row,col,item);}
        variableTable_->item(row,0)->setData(Qt::UserRole,id);
        if(!id.isEmpty() && selection==id)variableTable_->selectRow(row);
    };
    const auto order=validateGraph(editorTools(context.definition),editorConnections(context.definition)).order;
    for(const auto& v:service_.variables()) {
        QString value="—",state="Missing — bu çevrimde atanmadı",source="—",cycleId="—";
        // Only an accepted Editor presentation may supply a value. Never read a
        // persisted recipe/default or resurrect a prior cycle after invalidation.
        if(cycle)for(const auto& producer:order)for(const auto& result:cycle->tools)if(result.toolId==producer)for(const auto& port:result.outputs)if(port.valid)
            if(const auto* bundle=std::get_if<AssignmentBundle>(&port.data);bundle && bundle->committed && bundle->cycleId==cycle->cycleId && bundle->fields.contains(v.id)) {
                const auto& field=bundle->fields[v.id];
                std::visit([&](const auto& envelope){
                    state=validity.at(int(envelope.validity))+(envelope.reason.isEmpty()?QString{}:" — "+envelope.reason);
                    value=envelope.value ? cellValue(field):"—";
                    source=bundle->producerId+".bundle";cycleId=bundle->cycleId;
                },field);
            }
        append({v.name,types.at(int(v.type)),"Recipe-cycle",units.at(int(v.unit)),value,state,source,"Read/Write",cycleId,v.comment},v.id);
    }
    append({"System.RecipeID","String","System","unitless",context.recipeId,"Valid","Editor","Read-only","—","Aktif reçete UUID"},{});
    append({"System.CycleID","String","System","unitless",cycle ? cycle->cycleId:"—",cycle ? "Valid":"Missing","Editor","Read-only",cycle ? cycle->cycleId:"—","Yalnız kabul edilmiş Editor çevrimi"},{});
    append({"System.Decision","Enum","System","unitless","—","Missing — genel karar henüz değerlendirilmedi","—","Read-only","—","Set Variables genel kararı değiştiremez"},{});
    variableTable_->resizeColumnToContents(0);
    variableTable_->setColumnWidth(0,qMax(260,variableTable_->columnWidth(0)));
    variableTable_->verticalScrollBar()->setValue(scroll);
}
void MainWindow::editVariable(const QString& id) {
    if(!service_.activeRecipe() || editorSession_.busy() || !applyToolProperties())return;
    VariableDefinition original;
    if(!id.isEmpty()){
        bool found=false;for(const auto& v:service_.variables())if(v.id==id){original=v;found=true;break;}
        if(!found)return;
    }
    const auto recipeId=service_.activeRecipe()->id;
    QDialog dialog(this);dialog.setObjectName("variableDialog");dialog.setWindowTitle(id.isEmpty()?"Değişken ekle":"Değişken düzenle");dialog.resize(580,490);
    auto* layout=new QVBoxLayout(&dialog);auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);auto* body=new QWidget;auto* form=new QFormLayout(body);scroll->setWidget(body);layout->addWidget(scroll,1);
    auto* uuid=new QLineEdit(id.isEmpty()?"Kaydet sırasında oluşturulur":id);uuid->setReadOnly(true);uuid->setObjectName("variableUuid");form->addRow("UUID",uuid);
    auto* name=new QLineEdit(original.name);name->setObjectName("variableName");form->addRow("Ad",name);
    auto* type=new QComboBox;type->setObjectName("variableType");type->addItems(types);type->setCurrentIndex(int(original.type));form->addRow("Tip",type);
    auto* unit=new QComboBox;unit->setObjectName("variableUnit");unit->addItems(units);unit->setCurrentIndex(int(original.unit));form->addRow("Birim",unit);
    auto* comment=new QPlainTextEdit(original.comment);comment->setObjectName("variableComment");comment->setMaximumHeight(90);form->addRow("Açıklama",comment);
    auto* enumId=new QLineEdit(original.enumDefinitionId);enumId->setObjectName("variableEnumId");form->addRow("Enum tanım ID",enumId);
    QStringList codes;for(auto code:original.enumCodes)codes<<QString::number(code);
    auto* enumCodes=new QLineEdit(codes.join(","));enumCodes->setObjectName("variableEnumCodes");enumCodes->setToolTip("Virgülle ayrılmış Int32 kodları; en fazla 256");form->addRow("Enum kodları",enumCodes);
    auto* frames=new QPlainTextEdit;frames->setObjectName("variableFrames");frames->setMaximumHeight(90);frames->setPlaceholderText("İsteğe bağlı: source ve target FrameRef JSON");
    if(original.frameConstraint)frames->setPlainText(QString::fromUtf8(QJsonDocument(QJsonObject{{"source",original.frameConstraint->source.toJson()},{"target",original.frameConstraint->target.toJson()}}).toJson(QJsonDocument::Compact)));
    form->addRow("Frame kısıtı",frames);
    auto sync=[=]{const bool enumeration=type->currentIndex()==int(VariableType::Enum);enumId->setEnabled(enumeration);enumCodes->setEnabled(enumeration);frames->setEnabled(type->currentIndex()==int(VariableType::SpatialTransform));};
    connect(type,&QComboBox::currentIndexChanged,&dialog,sync);sync();
    auto* error=new QLabel;error->setObjectName("variableDialogError");error->setTextFormat(Qt::PlainText);error->setWordWrap(true);layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);buttons->setObjectName("variableDialogButtons");layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{
        if(!service_.activeRecipe() || service_.activeRecipe()->id!=recipeId || editorSession_.busy())throw std::runtime_error("Reçete değişti veya Editor meşgul.");
        auto v=original;v.name=name->text();v.type=VariableType(type->currentIndex());v.unit=ScalarUnit(unit->currentIndex());v.comment=comment->toPlainText();
        v.enumDefinitionId.clear();v.enumCodes.clear();v.frameConstraint.reset();
        if(v.type==VariableType::Enum){v.enumDefinitionId=enumId->text();for(const auto& token:enumCodes->text().split(',')){
            bool ok=false;const auto code=token.trimmed().toLongLong(&ok);
            if(!ok || code<std::numeric_limits<qint32>::min() || code>std::numeric_limits<qint32>::max() || v.enumCodes.contains(qint32(code)))throw std::runtime_error("Enum kodları benzersiz Int32 olmalı.");v.enumCodes.insert(qint32(code));}}
        if(v.type==VariableType::SpatialTransform && !frames->toPlainText().trimmed().isEmpty()){
            QJsonParseError parse;const auto json=QJsonDocument::fromJson(frames->toPlainText().toUtf8(),&parse);
            if(parse.error!=QJsonParseError::NoError || !json.isObject() || json.object().size()!=2 || !json.object()["source"].isObject() || !json.object()["target"].isObject())throw std::runtime_error("Frame kısıtı source/target JSON nesneleri ister.");
            v.frameConstraint=VariableFrameConstraint{FrameRef::fromJson(json.object()["source"].toObject()),FrameRef::fromJson(json.object()["target"].toObject())};}
        const auto before=service_.activeRecipe()->definition;
        if(id.isEmpty())service_.addVariable(v);else service_.updateVariable(v);
        if(service_.activeRecipe()->definition!=before){editorSession_.invalidateResults();refreshWorkspace();}
        findChild<QLabel*>("variableError")->hide();dialog.accept();
    }catch(const std::exception& e){error->setText(QString::fromUtf8(e.what()));}});
    name->setFocus();dialog.exec();
}
void MainWindow::removeSelectedVariable() {
    if(!service_.activeRecipe() || editorSession_.busy())return;
    const auto* item=variableTable_->item(variableTable_->currentRow(),0);
    const QString id=item ? item->data(Qt::UserRole).toString():QString{};
    if(id.isEmpty() || !applyToolProperties())return;
    try{service_.removeVariable(id);editorSession_.invalidateResults();findChild<QLabel*>("variableError")->hide();refreshWorkspace();}
    catch(const std::exception& e){auto* error=findChild<QLabel*>("variableError");error->setText(QString::fromUtf8(e.what()));error->show();}
}
}
