#include "PublishDataEditor.hpp"
#include "application/PublicationAssignmentCatalog.hpp"
#include "application/PublicationService.hpp"
#include "persistence/PublicationTemplateStore.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QDir>
#include "domain/ToolGraph.hpp"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
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
namespace {
QString shortId(const QUuid& id){return id.toString(QUuid::WithoutBraces).left(8);}
}
PublishDataEditor::PublishDataEditor(const ToolDefinition& tool,const QJsonObject& definition,QString path,
                                     QString templateDirectory,const CycleResult* cycle)
    :tool_(tool),definition_(definition),templateDirectory_(std::move(templateDirectory)),cycle_(cycle) {
    Q_UNUSED(path);
    setObjectName("publishDataEditor");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSizeConstraint(QLayout::SetMinimumSize);
    auto* help=new QLabel("Genel karar ve sonuç geçerliliği terminale otomatik eklenir. Slotlar ek veriler içindir. Önizleme veri göndermez.");help->setWordWrap(true);help->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);layout->addWidget(help);
    // ---- v4 fixed-slot assignment (machine template is read-only) ----
    auto* slotBox=new QGroupBox("Slot ataması (v4)");slotBox->setObjectName("publishSlotBox");
    auto* slotLayout=new QVBoxLayout(slotBox);
    auto* slotNote=new QLabel("Şablon ve slot sözleşmesi makineye aittir (Communication). Burada yalnız reçete kaynağı bağlanır. Tip/birim/frame uyuşmazlığı uygulamayı engeller; ağ yayını yapılmaz.");
    slotNote->setWordWrap(true);slotNote->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);slotLayout->addWidget(slotNote);
    templateSelector_=new QComboBox;templateSelector_->setObjectName("publishTemplateSelector");templateSelector_->setMinimumHeight(40);
    templateSelector_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);templateSelector_->setMinimumContentsLength(8);
    templateSelector_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);slotLayout->addWidget(templateSelector_);
    templateInfo_=new QLabel;templateInfo_->setObjectName("publishTemplateInfo");templateInfo_->setWordWrap(true);templateInfo_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);slotLayout->addWidget(templateInfo_);
    connect(templateSelector_,&QComboBox::currentIndexChanged,this,[this]{if(loadingSlots_)return;startAssignment(templateSelector_->currentIndex());});
    slotFilter_=new QComboBox;slotFilter_->setObjectName("publishSlotFilter");slotFilter_->setMinimumHeight(40);
    for(const auto& label:QStringList{"Tüm slotlar","Yalnız Number","Yalnız Flag","Yalnız Text","Yalnız Pose","Atananlar","Sorunlular"})slotFilter_->addItem(label);
    slotFilter_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);slotFilter_->setMinimumContentsLength(8);
    slotFilter_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);slotLayout->addWidget(slotFilter_);
    connect(slotFilter_,&QComboBox::currentIndexChanged,this,[this]{refreshSlots();});
    slotTable_=new QTableWidget(0,2);slotTable_->setObjectName("publishSlots");slotTable_->setHorizontalHeaderLabels({"Slot / durum","Kaynak"});
    slotTable_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Interactive);
    slotTable_->setColumnWidth(0,96);
    slotTable_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    slotTable_->horizontalHeader()->setMinimumSectionSize(48);
    slotTable_->setWordWrap(false);slotTable_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    slotTable_->setTextElideMode(Qt::ElideRight);slotTable_->setMinimumWidth(0);slotTable_->setMinimumHeight(170);slotTable_->setMaximumHeight(300);
    slotTable_->setSelectionBehavior(QAbstractItemView::SelectRows);slotTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    // Ignore the table's content size hint so the narrow properties panel can
    // never be widened by long contract strings.
    slotTable_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
    slotTable_->verticalHeader()->setVisible(false);slotTable_->setAccessibleName("Sabit yayın slotları");
    slotLayout->addWidget(slotTable_);
    connect(slotTable_,&QTableWidget::currentCellChanged,this,[this]{slotSelectionChanged();});
    slotInfo_=new QLabel;slotInfo_->setObjectName("publishSlotInfo");slotInfo_->setWordWrap(true);slotInfo_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);slotLayout->addWidget(slotInfo_);
    auto* slotButtons=new QVBoxLayout;
    auto slotButton=[&](const QString& name,const QString& text,auto fn){auto* b=new QPushButton(text);b->setObjectName(name);slotButtons->addWidget(b);connect(b,&QPushButton::clicked,this,fn);};
    slotButton("publishSlotPreview","Slot önizle",[this]{slotPreview();});
    slotButton("publishSlotClear","Atamayı temizle",[this]{clearAssignment();});
    slotLayout->addLayout(slotButtons);
    slotStatus_=new QLabel;slotStatus_->setObjectName("publishSlotStatus");slotStatus_->setWordWrap(true);slotStatus_->setTextFormat(Qt::PlainText);
    slotStatus_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);slotLayout->addWidget(slotStatus_);
    layout->addWidget(slotBox);loadTemplates();
}
QJsonObject PublishDataEditor::parameters() const{return tool_.parameters;}
std::optional<PublicationAssignments> PublishDataEditor::assignments() const{return assignment_;}
void PublishDataEditor::error(const QString& text){slotStatus_->setText(text);}
void PublishDataEditor::validateDraft(const ToolDefinition& tool) const{
    if(assignmentsEdited_ && tool.enabled && !decodePublishConfig(tool.parameters).groups.isEmpty())
        throw std::runtime_error("Etkin legacy yayın bağı korunuyor; V4 slot atamasından önce açık legacy geçişi gerekir.");
    const auto tools=editorTools(definition_);const auto links=executionConnections(definition_);
    const auto graph=definition_["editor"].toObject()["schemaVersion"]==9?validateSourceGraph(tools,links):validateGraph(tools,links);
    if(!graph.valid())throw std::runtime_error(graph.errors.join("; ").toStdString());
}
// ---- v4 slot assignment ----
void PublishDataEditor::loadTemplates(){
    QSignalBlocker block(templateSelector_);templateSelector_->clear();templates_.clear();templateError_.clear();
    templateSelector_->addItem("Şablon seçin",-1);
    if(templateDirectory_.isEmpty()){templateError_="Makine şablon dizini verilmedi; slot ataması bu oturumda kullanılamaz.";templateInfo_->setText(templateError_);slotTable_->setRowCount(0);return;}
    QStringList errors;PublicationTemplateStore store(templateDirectory_);
    for(const auto& manifest:store.load(&errors)){
        templates_.append(manifest.value);
        QString label=sharedTemplateLabel(manifest.value);
        try{const auto c=JsonStationIdentityStore(QDir(templateDirectory_).filePath("../communication-identities-v4.json")).load();for(const auto& e:c.entries)if(!e.retired && e.identity==CatalogueIdentity{CatalogueCategory::TemplateRevision,manifest.value.id,manifest.value.revision,{}}){label=e.name+" • "+label;break;}}catch(const std::exception&){}
        templateSelector_->addItem(label,templates_.size()-1);
        templateSelector_->setItemData(templateSelector_->count()-1,manifest.path,Qt::ToolTipRole);
    }
    if(!errors.isEmpty())templateError_="Bazı şablon manifest'leri okunamadı: "+errors.join(" • ");
    const auto stored=recipePublicationAssignments(definition_);
    if(stored){assignment_=stored;for(int i=0;i<templates_.size();++i)if(templates_[i].id==stored->templateId && templates_[i].revision==stored->templateRevision){templateSelector_->setCurrentIndex(i+1);startAssignment(i+1);break;}}
    if(templateSelector_->currentIndex()<=0){
        selectedTemplate_=PublicationTemplate{};hasTemplate_=false;
        templateInfo_->setText(templates_.isEmpty()?"Bu makinede hazırlanmış publication template yok. Communication → Makine kurulumu → Kurulumu başlat ile hazırlanır."
                                                    :"Slot ataması için bir makine şablon revision'ı seçin. Seçim reçeteye yalnız kaynak bağı ekler.");
        slotTable_->setRowCount(0);slotStatus_->clear();slotInfo_->clear();
    }
    if(!templateError_.isEmpty())templateInfo_->setToolTip(templateError_);
}
QString PublishDataEditor::sharedTemplateLabel(const PublicationTemplate& t){
    return shortId(t.id)+"… • revision "+shortId(t.revision)+"…";
}
void PublishDataEditor::startAssignment(int selectorIndex){
    if(selectorIndex<=0 || selectorIndex>templates_.size()){hasTemplate_=false;selectedTemplate_=PublicationTemplate{};slotTable_->setRowCount(0);slotStatus_->clear();slotInfo_->clear();templateInfo_->setText("Şablon seçilmedi; mevcut atama korunur.");return;}
    selectedTemplate_=templates_[selectorIndex-1];hasTemplate_=true;
    if(!assignment_ || assignment_->templateId!=selectedTemplate_.id || assignment_->templateRevision!=selectedTemplate_.revision){
        assignment_=PublicationAssignments{selectedTemplate_.id,selectedTemplate_.revision,{}};assignmentsEdited_=true;
    }
    templateInfo_->setText(QString("Şablon %1… • revision %2… • 16 Number / 32 Flag / 4 Text / 1 Pose. Slot adı ve sözleşme salt okunur.")
        .arg(shortId(selectedTemplate_.id),shortId(selectedTemplate_.revision)));
    templateInfo_->setToolTip(QString("Şablon %1 • revision %2%3")
        .arg(selectedTemplate_.id.toString(QUuid::WithoutBraces),selectedTemplate_.revision.toString(QUuid::WithoutBraces),
             templateError_.isEmpty()?QString{}:QString("\n")+templateError_));
    slotStatus_->setText("Şablon bağlandı. Kaynak seçimi ve Apply gerekir.");
    refreshSlots();
}
void PublishDataEditor::refreshSlots(){
    if(!hasTemplate_){slotTable_->setRowCount(0);return;}
    loadingSlots_=true;
    const auto selectedRow=slotTable_->currentRow();
    const auto options=publicationSourceOptions(definition_);
    const auto candidate=candidateDefinition();
    const auto draft=assignment_?*assignment_:PublicationAssignments{selectedTemplate_.id,selectedTemplate_.revision,{}};
    const auto preview=previewNow();
    const int filter=slotFilter_?slotFilter_->currentIndex():0;
    auto visible=[&](const PublicationSlotRow& row){switch(filter){
        case 1:return row.kind==PublicationSlotKind::Number;case 2:return row.kind==PublicationSlotKind::Flag;
        case 3:return row.kind==PublicationSlotKind::Text;case 4:return row.kind==PublicationSlotKind::Pose;
        case 5:return row.assigned;case 6:return !row.valid;default:return true;}};
    QVector<PublicationSlotRow> rows;for(const auto& row:preview.rows)if(visible(row))rows.append(row);
    slotTable_->setRowCount(rows.size());
    for(int n=0;n<rows.size();++n){
        const auto& row=rows[n];
        if(!slotTable_->item(n,0))slotTable_->setItem(n,0,new QTableWidgetItem);
        auto* nameItem=slotTable_->item(n,0);
        // Two columns only: the compact status badge rides on the slot name so
        // the narrow properties panel never needs a horizontal scrollbar.
        nameItem->setText(publicationSlotDisplayName(row.kind,row.index,slotFor(selectedTemplate_,row.kind,row.index))
                          +(row.badge.isEmpty()?QString{}:"  "+row.badge));
        nameItem->setData(Qt::UserRole,int(row.kind));nameItem->setData(Qt::UserRole+1,row.index);
        nameItem->setToolTip(nameItem->text()+"\n"+row.contract+"\n"+row.status);
        nameItem->setForeground(row.valid?(row.required?QColor("#1b5e20"):QColor("#37474f")):QColor("#b3261e"));
        auto* picker=qobject_cast<QComboBox*>(slotTable_->cellWidget(n,1));
        if(!picker){picker=new QComboBox;picker->setObjectName("publishSlotSource");picker->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);picker->setMinimumContentsLength(2);picker->setMinimumHeight(32);picker->setMinimumWidth(56);
            connect(picker,&QComboBox::currentTextChanged,this,[this,picker]{int tableRow=-1;for(int r=0;r<slotTable_->rowCount();++r)if(slotTable_->cellWidget(r,1)==picker){tableRow=r;break;}if(tableRow<0||loadingSlots_)return;slotChanged(tableRow,1);});slotTable_->setCellWidget(n,1,picker);}
        const QSignalBlocker guard(picker);picker->clear();picker->addItem("Atanmadı",QString{});
        for(const auto& option:options){
            const auto reason=option.blockReason.isEmpty()?QString{}:QString(" — ")+option.blockReason;
            picker->addItem(option.label+reason,option.detail);
            picker->setItemData(picker->count()-1,QString::number(int(option.source.kind))+":"+option.source.id+":"+option.source.port,Qt::UserRole);
            picker->setItemData(picker->count()-1,option.source.revision,Qt::UserRole+1);
        }
        int index=0;
        if(row.assigned){
            QString key;
            for(const auto& assigned:draft.rows)if(assigned.kind==row.kind && assigned.index==row.index)
                key=QString::number(int(assigned.source.kind))+":"+assigned.source.id+":"+assigned.source.port;
            index=picker->findData(key,Qt::UserRole);
            if(index<0){picker->addItem("Eksik: "+row.source,QString{});picker->setItemData(picker->count()-1,key,Qt::UserRole);index=picker->count()-1;}
        }
        picker->setCurrentIndex(index);
    }
    if(selectedRow>=0 && selectedRow<slotTable_->rowCount())slotTable_->selectRow(selectedRow);
    loadingSlots_=false;slotSelectionChanged();
}
PublicationSlot PublishDataEditor::slotFor(const PublicationTemplate& t,PublicationSlotKind kind,int index){
    switch(kind){case PublicationSlotKind::Number:return t.numbers[index];case PublicationSlotKind::Flag:return t.flags[index];
    case PublicationSlotKind::Text:return t.texts[index];case PublicationSlotKind::Pose:return t.poses[index];}
    return {};
}
PublicationSlotKind PublishDataEditor::rowKind(int row) const{const auto* item=slotTable_->item(row,0);return item?PublicationSlotKind(item->data(Qt::UserRole).toInt()):PublicationSlotKind::Number;}
int PublishDataEditor::rowIndex(int row) const{const auto* item=slotTable_->item(row,0);return item?item->data(Qt::UserRole+1).toInt():0;}
QJsonObject PublishDataEditor::candidateDefinition() const{
    auto definition=definition_;
    if(assignment_)definition=withRecipePublicationAssignments(definition,*assignment_);else definition.remove("publicationV4");
    return definition;
}
PublicationPreview PublishDataEditor::previewNow() const{
    const auto draft=assignment_?*assignment_:PublicationAssignments{selectedTemplate_.id,selectedTemplate_.revision,{}};
    auto preview=previewPublication(selectedTemplate_,draft,candidateDefinition(),cycle_);
    // A cycle that no longer matches this definition explains nothing about the
    // slot values; fall back to the contract-only view instead of a fake failure.
    const bool stale=std::any_of(preview.rows.begin(),preview.rows.end(),[&](const auto& row){
        return row.valueChecked && !row.valid && (row.status.contains("provenance") || row.status.contains("snapshot") || row.status.contains("bundle"));
    });
    if(stale){
        auto metadata=previewPublication(selectedTemplate_,draft,candidateDefinition(),nullptr);
        metadata.errors.prepend("Bu tanımla eşleşen güncel kabul edilmiş çevrim yok; değer önizlemesi yok.");
        return metadata;
    }
    return preview;
}
void PublishDataEditor::slotChanged(int row,int){
    auto* picker=qobject_cast<QComboBox*>(slotTable_->cellWidget(row,1));if(!picker||!hasTemplate_)return;
    const auto kind=rowKind(row);const int index=rowIndex(row);
    if(!assignment_)assignment_=PublicationAssignments{selectedTemplate_.id,selectedTemplate_.revision,{}};
    assignment_->rows.erase(std::remove_if(assignment_->rows.begin(),assignment_->rows.end(),[&](const auto& r){return r.kind==kind && r.index==index;}),assignment_->rows.end());
    const auto key=picker->currentData(Qt::UserRole).toString();
    if(!key.isEmpty()){
        const auto parts=key.split(':');if(parts.size()==3){
            SlotAssignment assignment;assignment.kind=kind;assignment.index=index;
            assignment.source.kind=PublicationSourceKind(parts[0].toInt());assignment.source.id=parts[1];assignment.source.port=parts[2];
            assignment.source.revision=picker->currentData(Qt::UserRole+1).toInt();
            assignment_->rows.append(assignment);
        }
    }
    assignmentsEdited_=true;refreshSlots();updateSlotStatus();
}
void PublishDataEditor::updateSlotStatus(){
    if(!hasTemplate_){slotStatus_->setText("Şablon seçilmedi.");return;}
    const auto preview=previewNow();
    int assigned=0,invalid=0,requiredMissing=0;
    QStringList problems;
    for(const auto& row:preview.rows){if(row.assigned)++assigned;if(!row.valid){++invalid;if(row.required && !row.assigned)++requiredMissing;
        if(problems.size()<6)problems<<publicationSlotDisplayName(row.kind,row.index,slotFor(selectedTemplate_,row.kind,row.index))+": "+row.status;}}
    QStringList lines;
    lines<<QString("%1 slot atandı • %2 sorunlu%3").arg(assigned).arg(invalid).arg(requiredMissing?QString(" • %1 zorunlu slot eksik").arg(requiredMissing):QString{});
    if(invalid>problems.size())problems<<QString("… +%1 sorunlu alan").arg(invalid-problems.size());
    lines<<problems;
    if(!preview.errors.isEmpty())lines<<preview.errors.join(" • ");
    if(!cycle_)lines<<"Değer önizlemesi için kabul edilmiş çevrim yok; overflow/UTF-8 sınırı yalnız Runtime değerlendirmesinde kesinleşir.";
    slotStatus_->setText(lines.join('\n'));
}
void PublishDataEditor::slotSelectionChanged(){
    const int row=slotTable_->currentRow();if(row<0||!hasTemplate_){slotInfo_->clear();return;}
    const auto name=slotTable_->item(row,0)?slotTable_->item(row,0)->text():QString{};
    const auto status=slotTable_->item(row,0)?slotTable_->item(row,0)->toolTip():QString{};
    const auto kind=rowKind(row);
    slotInfo_->setText(name+"\n"+publicationSlotContractText(kind,slotFor(selectedTemplate_,kind,rowIndex(row)))+"\n"+status);
}
void PublishDataEditor::slotPreview(){
    try{
        if(!hasTemplate_)throw std::runtime_error("Önce bir makine şablon revision'ı seçin");
        if(!assignment_)throw std::runtime_error("Slot ataması yok");
        const auto preview=previewNow();
        QStringList lines{"SLOT ÖNİZLEMESİ • tanım; gönderim ve ağ yok."};
        for(const auto& row:preview.rows)if(row.assigned || !row.valid)
            lines<<publicationSlotDisplayName(row.kind,row.index,slotFor(selectedTemplate_,row.kind,row.index))+": "+row.status;
        for(const auto& error:preview.errors)lines<<"HATA: "+error;
        if(!cycle_)lines<<"Kabul edilmiş çevrim olmadığı için değer/wire önizlemesi yok.";
        slotStatus_->setText(lines.join('\n'));
    }catch(const std::exception& e){slotStatus_->setText(QString::fromUtf8(e.what()));}
}
void PublishDataEditor::clearAssignment(){
    if(!hasTemplate_){slotStatus_->setText("Şablon seçilmedi.");return;}
    assignment_=PublicationAssignments{selectedTemplate_.id,selectedTemplate_.revision,{}};assignmentsEdited_=true;
    refreshSlots();updateSlotStatus();slotStatus_->setText("Slot ataması temizlendi; zorunlu slotlar uygulamayı engeller.");
}
void PublishDataEditor::validateAssignments() const{
    if(!assignmentsEdited_)return;
    if(!hasTemplate_)throw std::runtime_error("Slot ataması için makine şablonu seçin");
    if(!assignment_)throw std::runtime_error("Slot ataması eksik");
    const auto preview=previewPublication(selectedTemplate_,*assignment_,candidateDefinition(),nullptr);
    QStringList problems=preview.errors;
    for(const auto& row:preview.rows)if(row.assigned || row.required)if(!row.valid)
        problems<<publicationSlotDisplayName(row.kind,row.index,slotFor(selectedTemplate_,row.kind,row.index))+": "+row.status;
    if(!problems.isEmpty())throw std::runtime_error(problems.join("; ").toStdString());
}
}
