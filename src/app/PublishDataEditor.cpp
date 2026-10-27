#include "PublishDataEditor.hpp"
#include "SetVariablesEditor.hpp"
#include "application/PublicationService.hpp"
#include "domain/ToolGraph.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QJsonArray>
#include <QUuid>
#include <stdexcept>
#include <algorithm>
namespace visionflow {
namespace { QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);} }
PublishDataEditor::PublishDataEditor(const ToolDefinition& tool,const QJsonObject& definition,QString path)
    :tool_(tool),definition_(definition),profiles_(std::move(path)),groups_(recipeOutputGroups(definition)),selected_(decodePublishConfig(tool.parameters).groups) {
    setObjectName("publishDataEditor");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);
    auto* help=new QLabel("Publish Data • mantıksal paket\nSet Variables bundle alanlarını seçin. Önizleme ve Run Tool gönderim yapmaz. Fiziksel JSON eşlemesi Setup → Communication profilindedir.");help->setWordWrap(true);help->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);layout->addWidget(help);
    auto* form=new QFormLayout;group_=new QComboBox;group_->setObjectName("publishGroup");form->addRow("Output group UUID",group_);
    producer_=new QComboBox;producer_->setObjectName("publishProducer");producer_->addItem("Bundle seçin",QString{});
    for(const auto& t:editorTools(definition))if(t.kind=="set_variables")producer_->addItem(t.name+" / r"+QString::number(decodeAssignmentConfig(t.parameters,t.id).revision)+" / "+t.id,t.id);
    form->addRow("Kaynak bundle",producer_);endpoint_=new QComboBox;endpoint_->setObjectName("publishEndpoint");endpoint_->addItem("Endpoint seçin",QString{});
    try{for(const auto& p:profiles_.load()){endpoint_->addItem(p.id+" / contract r"+QString::number(p.contractRevision)+(p.enabled?"":" [disabled]"),p.id);endpoint_->setItemData(endpoint_->count()-1,p.contractRevision,Qt::UserRole+1);}}catch(...){}
    for(auto* combo:{group_,producer_,endpoint_}){combo->setFixedHeight(30);combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setMinimumContentsLength(8);combo->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);connect(combo,&QComboBox::currentTextChanged,this,[combo]{combo->setToolTip(combo->currentText());});}
    form->setVerticalSpacing(8);form->setRowWrapPolicy(QFormLayout::WrapAllRows);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow("Endpoint UUID",endpoint_);required_=new QCheckBox("Required (ACK beklenir)");required_->setObjectName("publishRequired");form->addRow(required_);layout->addLayout(form);
    fields_=new QTableWidget(0,2);fields_->setObjectName("publishFields");fields_->setHorizontalHeaderLabels({"Logical field","Bundle field"});fields_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);fields_->setMinimumHeight(125);fields_->setMaximumHeight(210);layout->addWidget(fields_);
    fieldInfo_=new QLabel;fieldInfo_->setObjectName("publishFieldInfo");fieldInfo_->setWordWrap(true);fieldInfo_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);layout->addWidget(fieldInfo_);
    connect(fields_,&QTableWidget::cellClicked,this,[this](int row,int){if(auto* picker=qobject_cast<QComboBox*>(fields_->cellWidget(row,1)))fieldInfo_->setText(picker->currentText());});
    auto* buttons=new QGridLayout;int buttonIndex=0;
    auto button=[&](const QString& name,const QString& text,auto fn){auto* b=new QPushButton(text);b->setObjectName(name);buttons->addWidget(b,buttonIndex/2,buttonIndex%2);++buttonIndex;connect(b,&QPushButton::clicked,this,fn);};
    button("publishAddGroup","+ Grup",[this]{store();OutputGroup g;g.id=uuid();groups_.append(g);selected_.append(g.id);refresh(g.id);});
    button("publishRemoveGroup","− Grup",[this]{const auto id=current_;groups_.erase(std::remove_if(groups_.begin(),groups_.end(),[&](const auto& g){return g.id==id;}),groups_.end());selected_.removeAll(id);current_.clear();refresh({});});
    button("publishAddField","+ Alan",[this]{addField();});button("publishRemoveField","− Alan",[this]{if(fields_->currentRow()>=0)fields_->removeRow(fields_->currentRow());});layout->addLayout(buttons);
    auto* previewButton=new QPushButton("Eşlemeyi çöz / önizle");previewButton->setObjectName("publishPreview");layout->addWidget(previewButton);connect(previewButton,&QPushButton::clicked,this,[this]{preview();});
    auto* rebind=new QPushButton("Revision bağını yenile");rebind->setObjectName("publishRebind");layout->addWidget(rebind);connect(rebind,&QPushButton::clicked,this,[this]{for(auto& g:groups_)if(g.id==current_){g.producerRevision=0;g.contractRevision=0;}store();preview();});
    status_=new QLabel;status_->setObjectName("publishStatus");status_->setWordWrap(true);status_->setTextFormat(Qt::PlainText);status_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);layout->addWidget(status_);
    connect(group_,&QComboBox::currentIndexChanged,this,[this]{store();load();});
    connect(producer_,&QComboBox::activated,this,[this]{fields_->setRowCount(0);});refresh({});
}
void PublishDataEditor::refresh(const QString& id){QSignalBlocker block(group_);group_->clear();for(const auto& key:selected_)group_->addItem(key,key);group_->setCurrentIndex(id.isEmpty()?0:group_->findData(id));load();}
void PublishDataEditor::load(){
    current_=group_->currentData().toString();OutputGroup g;for(const auto& item:groups_)if(item.id==current_)g=item;
    auto select=[](QComboBox* box,const QString& id){if(!id.isEmpty() && box->findData(id)<0)box->addItem("Eksik: "+id,id);box->setCurrentIndex(qMax(0,box->findData(id)));};
    select(producer_,g.producerId);select(endpoint_,g.profileId);required_->setChecked(g.required);fields_->setRowCount(0);for(const auto& f:g.fields)addField(f);
}
void PublishDataEditor::addField(const PublicationField& field){
    const int row=fields_->rowCount();if(row>=maxPublicationFields)return;fields_->insertRow(row);fields_->setItem(row,0,new QTableWidgetItem(field.name));auto* picker=new QComboBox;picker->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);picker->setMinimumContentsLength(8);picker->addItem("Alan seçin",QString{});
    const QStringList types{"Boolean","Int32","UInt32","Int64","Float64","String","Enum","SpatialTransform"},units{"unitless","mm","m","rad","degree"};
    for(const auto& t:editorTools(definition_))if(t.id==producer_->currentData().toString())for(const auto& r:decodeAssignmentConfig(t.parameters,t.id).rows)for(const auto& v:recipeVariables(definition_))if(v.id==r.targetId){auto text=v.name+" • "+types.at(int(v.type))+" / "+units.at(int(v.unit));if(v.frameConstraint)text+=" / "+v.frameConstraint->source.frameUuid+" → "+v.frameConstraint->target.frameUuid;picker->addItem(text,v.id);picker->setItemData(picker->count()-1,text,Qt::ToolTipRole);}
    if(!field.fieldId.isEmpty() && picker->findData(field.fieldId)<0)picker->addItem("Eksik: "+field.fieldId,field.fieldId);picker->setCurrentIndex(qMax(0,picker->findData(field.fieldId)));fields_->setCellWidget(row,1,picker);
    connect(picker,&QComboBox::currentTextChanged,this,[this,picker]{fieldInfo_->setText(picker->currentText());});fieldInfo_->setText(picker->currentText());
}
void PublishDataEditor::store(){for(auto& g:groups_)if(g.id==current_){
    const auto producer=producer_->currentData().toString(),profile=endpoint_->currentData().toString();
    if(g.producerId!=producer || g.producerRevision==0)for(const auto& t:editorTools(definition_))if(t.id==producer)g.producerRevision=decodeAssignmentConfig(t.parameters,t.id).revision;
    if((g.profileId!=profile || g.contractRevision==0) && endpoint_->currentData(Qt::UserRole+1).isValid())g.contractRevision=endpoint_->currentData(Qt::UserRole+1).toInt();
    g.producerId=producer;g.profileId=profile;g.required=required_->isChecked();g.fields.clear();
    for(int row=0;row<fields_->rowCount();++row)g.fields.append({fields_->item(row,0)->text().trimmed(),static_cast<QComboBox*>(fields_->cellWidget(row,1))->currentData().toString()});
}}
QJsonObject PublishDataEditor::parameters() const{return encodePublishConfig({selected_});}
QVector<OutputGroup> PublishDataEditor::groups() const{auto* self=const_cast<PublishDataEditor*>(this);self->store();return groups_;}
void PublishDataEditor::error(const QString& text){auto wrapped=text;for(int i=24;i<wrapped.size();i+=25)wrapped.insert(i,QChar(0x200b));status_->setText(wrapped);}
void PublishDataEditor::validateDraft(const ToolDefinition& tool) const{
    auto definition=definition_;QJsonArray a;for(const auto& g:groups())a.append(encodeOutputGroup(g));definition["publication"]=QJsonObject{{"schemaVersion",1},{"groups",a}};
    auto tools=editorTools(definition);for(auto& t:tools)if(t.id==tool.id)t=tool;definition=withEditorTools(definition,tools);
    if(tool.enabled || !selected_.isEmpty()){for(auto& t:tools)if(t.id==tool.id)t.enabled=true;definition=withEditorTools(definition,tools);PublicationService::capture(definition,profiles_);}
    for(const auto& g:groups())if(selected_.contains(g.id))for(const auto& producer:tools)if(producer.id==g.producerId){
        if(!producer.enabled)throw std::runtime_error("Publication bundle producer disabled");
        SetVariablesEditor::validateConfiguration(producer,recipeVariables(definition),tools);
    }
    const auto links=executionConnections(definition);const auto graph=definition["editor"].toObject()["schemaVersion"]==9?validateSourceGraph(tools,links):validateGraph(tools,links);if(!graph.valid())throw std::runtime_error(graph.errors.join("; ").toStdString());
}
void PublishDataEditor::preview(){try{auto tool=tool_;tool.parameters=parameters();validateDraft(tool);QStringList lines{"RESOLVED • tanım önizlemesi; canlı değer yok, gönderim yok."};const auto profiles=profiles_.load();for(const auto& g:groups())if(selected_.contains(g.id))for(const auto& f:g.fields)lines<<f.name+" ← "+f.fieldId+" → "+profiles[g.profileId].fieldMapping.value(f.name);status_->setText(lines.join('\n'));}catch(const std::exception& e){error(QString::fromUtf8(e.what()));}}
}
