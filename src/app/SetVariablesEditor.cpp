#include "SetVariablesEditor.hpp"
#include "domain/ToolRegistry.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <stdexcept>

namespace visionflow {
namespace {
const QStringList types{"Boolean","Int32","UInt32","Int64","Float64","String","Enum","SpatialTransform"};
const QStringList units{"unitless","mm","m","rad","degree"};
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
using SourceChoice=QPair<QString,QJsonObject>;
QVector<SourceChoice> compatibleSources(const VariableDefinition& target,const QVector<VariableDefinition>& variables,
                                      const QVector<ToolDefinition>& tools,const QString& self) {
    QVector<SourceChoice> result;
    for(const auto& tool:tools)if(tool.id!=self && tool.enabled) {
        if(tool.kind=="set_variables") {
            const auto config=decodeAssignmentConfig(tool.parameters,tool.id);
            for(const auto& row:config.rows)for(const auto& field:variables)
                if(field.id==row.targetId && field.type==target.type && field.unit==target.unit &&
                   field.frameConstraint==target.frameConstraint && field.enumDefinitionId==target.enumDefinitionId)
                    result.append({tool.name+".bundle / "+field.name+" • "+types.at(int(field.type))+" / "+units.at(int(field.unit)),
                        {{"kind","bundle"},{"producerId",tool.id},{"fieldId",field.id},{"revision",config.revision}}});
        } else for(const auto& port:outputPorts(tool.kind)) {
            QString unit=port.unit;
            if(unit.isEmpty() || unit=="count" || unit=="ratio")unit="unitless";
            if(unit=="deg")unit="degree";
            const bool type=(port.type==PortType::Number && target.type==VariableType::Float64) ||
                (port.type==PortType::Boolean && target.type==VariableType::Boolean) ||
                (port.type==PortType::Text && target.type==VariableType::String);
            if(type && unit==units.at(int(target.unit)))
                result.append({tool.name+"."+port.id+" • "+types.at(int(target.type))+" / "+unit,
                    {{"kind","tool_output"},{"toolId",tool.id},{"port",port.id},{"revision",ToolRegistry::get(tool.kind).version}}});
        }
    }
    return result;
}
AssignmentValue typedConstant(const VariableDefinition& target,const QString& input) {
    QJsonParseError parse;
    const auto wrapper=QJsonDocument::fromJson(("["+input+"]").toUtf8(),&parse);
    require(parse.error==QJsonParseError::NoError && wrapper.isArray() && wrapper.array().size()==1,"Invalid typed constant JSON");
    auto value=wrapper.array().first();
    if(target.type==VariableType::SpatialTransform) {
        require(value.isObject(),"SpatialTransform object required");
        return SpatialAssignmentValue{SpatialTransform::fromJson(value.toObject()),ValueValidity::Valid};
    }
    ValueEnvelope envelope;envelope.type=ScalarType(int(target.type));envelope.unit=target.unit;envelope.validity=ValueValidity::Valid;
    // A valid seed supplies strict codec metadata; decoding the actual input
    // enforces integer bounds, finite Float64, Int64 strings and enum authority.
    EnumDefinitions enums;
    switch(envelope.type) {
    case ScalarType::Boolean:envelope.value=false;break;
    case ScalarType::Int32:envelope.value=qint32(0);break;
    case ScalarType::UInt32:envelope.value=quint32(0);break;
    case ScalarType::Int64:envelope.value=qint64(0);break;
    case ScalarType::Float64:envelope.value=0.0;break;
    case ScalarType::String:envelope.value=QString{};break;
    case ScalarType::Enum:
        enums[target.enumDefinitionId]=target.enumCodes;
        envelope.value=EnumValue{target.enumDefinitionId,*target.enumCodes.begin()};
        require(value.isDouble(),"Enum numeric code required");
        value=QJsonObject{{"definitionId",target.enumDefinitionId},{"code",value}};break;
    }
    auto encoded=CommunicationValues::encode(envelope,enums);encoded["value"]=value;
    return CommunicationValues::decode(encoded,enums);
}
QString constantInput(const QJsonObject& source) {
    if(source.contains("input"))return source["input"].toString();
    const auto value=source["value"].toObject();
    const auto envelope=value["envelope"].toObject();
    const auto payload=value["kind"]=="scalar" ? envelope["value"]:value["transform"];
    if(envelope["type"]=="Enum")return QString::number(payload.toObject()["code"].toInt());
    auto bytes=QJsonDocument(QJsonArray{payload}).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(bytes.mid(1,bytes.size()-2));
}
}

QString SetVariablesEditor::description(const VariableDefinition& v) const {
    return v.name+" • "+types.at(int(v.type))+" / "+units.at(int(v.unit));
}
SetVariablesEditor::SetVariablesEditor(const ToolDefinition& tool,const QJsonObject& definition,const CycleResult* cycle)
    :tool_(tool),tools_(editorTools(definition)),variables_(recipeVariables(definition)),rows_(tool.parameters["rows"].toArray()) {
    setObjectName("setVariablesEditor");
    auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);
    auto* hint=new QLabel("Tüm sağ taraflar aynı girişten okunur; grup atomik uygulanır. Editor dışarı veri yazmaz. Sabit: JSON değer; String/Int64 tırnaklı, Enum kodu, SpatialTransform nesnesi.");
    hint->setWordWrap(true);layout->addWidget(hint);
    list_=new QListWidget;list_->setObjectName("assignmentRows");list_->setMinimumHeight(100);list_->setMaximumHeight(170);layout->addWidget(list_);
    auto* buttons=new QHBoxLayout;layout->addLayout(buttons);
    auto button=[&](const QString& text,const QString& name,auto action) {
        auto* b=new QPushButton(text);b->setObjectName(name);buttons->addWidget(b);connect(b,&QPushButton::clicked,this,action);return b;
    };
    auto* add=button("+","assignmentAdd",[this] {
        if(rows_.size()>=AssignmentConfig::maxRows)return;
        rows_.append(QJsonObject{{"targetId",""},{"source",QJsonObject{{"kind","constant"},{"input","false"}}}});
        refreshRows(rows_.size()-1);
    });
    add->setToolTip("Satır ekle (en çok 64)");
    button("−","assignmentRemove",[this] {
        const int i=list_->currentRow();if(i>=0){rows_.removeAt(i);refreshRows(qMin(i,int(rows_.size())-1));}
    });
    for(int direction:{-1,1})button(direction<0 ? "↑":"↓",direction<0 ? "assignmentUp":"assignmentDown",[this,direction] {
        const int i=list_->currentRow(),j=i+direction;
        if(i>=0 && j>=0 && j<rows_.size()) {
            const QJsonValue value=rows_[i];rows_[i]=rows_[j];rows_[j]=value;refreshRows(j);
        }
    });
    auto* form=new QFormLayout;form->setRowWrapPolicy(QFormLayout::WrapAllRows);layout->addLayout(form);
    target_=new QComboBox;target_->setObjectName("assignmentTarget");target_->addItem("Hedef seçin",QString{});
    for(const auto& v:variables_)target_->addItem(description(v),v.id);
    form->addRow("Hedef • type / unit",target_);
    source_=new QComboBox;source_->setObjectName("assignmentSource");source_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);source_->setMinimumContentsLength(12);
    form->addRow("Uyumlu kaynak • type / unit",source_);
    constant_=new QLineEdit;constant_->setObjectName("assignmentConstant");constant_->setMaxLength(32768);form->addRow("Typed constant (JSON)",constant_);
    validity_=new QLabel;validity_->setObjectName("assignmentValidity");validity_->setWordWrap(true);layout->addWidget(validity_);
    QString result="Missing — güncel kabul edilmiş sonuç yok";
    if(cycle)for(const auto& r:cycle->tools)if(r.toolId==tool.id)
        result=r.state==ExecutionState::Done ? "Valid — atomik bundle committed (tüm satırlar)":QString("Uncommitted — %1").arg(r.message);
    validity_->setText(result);
    error_=new QLabel;error_->setObjectName("assignmentError");error_->setWordWrap(true);error_->setStyleSheet("color:#e88383");error_->hide();layout->addWidget(error_);
    connect(list_,&QListWidget::currentRowChanged,this,[this]{selectRow();});
    connect(target_,&QComboBox::currentIndexChanged,this,[this] {
        if(loading_)return;sources();constant_->setEnabled(true);storeRow();
    });
    connect(source_,&QComboBox::currentIndexChanged,this,[this] {
        if(loading_)return;constant_->setEnabled(source_->currentData().toJsonObject()["kind"]=="constant");storeRow();
    });
    connect(constant_,&QLineEdit::textEdited,this,[this]{storeRow();});
    refreshRows(rows_.isEmpty()?-1:0);
}
void SetVariablesEditor::error(const QString& message) {
    error_->setText(message);error_->setVisible(!message.isEmpty());
}
void SetVariablesEditor::refreshRows(int selected) {
    if(rows_!=tool_.parameters["rows"].toArray())validity_->setText("Missing — taslak henüz uygulanmadı / çalıştırılmadı");
    const QSignalBlocker block(list_);list_->clear();
    for(int i=0;i<rows_.size();++i) {
        const auto row=rows_[i].toObject();QString label="Hedef seçilmedi / invalid";
        for(const auto& v:variables_)if(v.id==row["targetId"])label=description(v);
        auto* item=new QListWidgetItem(QString::number(i+1)+". "+label,list_);item->setToolTip(item->text());
    }
    list_->setCurrentRow(selected);selectRow();
}
void SetVariablesEditor::sources(const QJsonObject& selected) {
    const QSignalBlocker block(source_);source_->clear();source_->addItem("Typed constant",QJsonObject{{"kind","constant"}});
    for(const auto& v:variables_)if(v.id==target_->currentData().toString())
        for(const auto& choice:compatibleSources(v,variables_,tools_,tool_.id))source_->addItem(choice.first,choice.second);
    if(!selected.isEmpty() && selected["kind"]!="constant") {
        int index=source_->findData(selected);
        if(index<0){source_->addItem("Invalid / eski kaynak — yeniden seçin",selected);index=source_->count()-1;}
        source_->setCurrentIndex(index);
    }
}
void SetVariablesEditor::selectRow() {
    loading_=true;const int i=list_->currentRow();const bool exists=i>=0;
    findChild<QPushButton*>("assignmentAdd")->setEnabled(rows_.size()<AssignmentConfig::maxRows);
    findChild<QPushButton*>("assignmentRemove")->setEnabled(exists);
    findChild<QPushButton*>("assignmentUp")->setEnabled(i>0);
    findChild<QPushButton*>("assignmentDown")->setEnabled(exists && i+1<rows_.size());
    target_->setEnabled(exists);source_->setEnabled(exists);constant_->setEnabled(exists);
    if(exists) {
        const auto row=rows_[i].toObject();int index=target_->findData(row["targetId"].toString());
        if(index<0){target_->addItem("Invalid target UUID",row["targetId"].toString());index=target_->count()-1;}
        target_->setCurrentIndex(index);const auto source=row["source"].toObject();sources(source);
        constant_->setText(source["kind"]=="constant" ? constantInput(source):QString{});
        constant_->setEnabled(source["kind"]=="constant");
    }
    loading_=false;
}
void SetVariablesEditor::storeRow() {
    if(loading_ || list_->currentRow()<0)return;
    auto source=source_->currentData().toJsonObject();if(source["kind"]=="constant")source["input"]=constant_->text();
    const int i=list_->currentRow();rows_[i]=QJsonObject{{"targetId",target_->currentData().toString()},{"source",source}};
    list_->item(i)->setText(QString::number(i+1)+". "+target_->currentText());
    list_->item(i)->setToolTip(target_->currentText()+" ← "+source_->currentText());
    validity_->setText("Missing — taslak henüz uygulanmadı / çalıştırılmadı");error({});
}
QJsonObject SetVariablesEditor::parameters() const {
    auto rows=rows_;
    for(int i=0;i<rows.size();++i) {
        auto row=rows[i].toObject(),source=row["source"].toObject();
        if(source.contains("input")) {
            const VariableDefinition* target=nullptr;
            for(const auto& v:variables_)if(v.id==row["targetId"].toString())target=&v;
            require(target,"Invalid target: recipe variable seçin");
            source=QJsonObject{{"kind","constant"},{"value",encodeAssignmentValue(typedConstant(*target,source["input"].toString()))}};
            row["source"]=source;rows[i]=row;
        }
    }
    QJsonObject result{{"revision",tool_.parameters["revision"]},{"rows",rows}};
    // Only a real edit changes revision; a no-op display preserves saved bytes.
    if(result!=tool_.parameters)result["revision"]=tool_.parameters["revision"].toInt()+1;
    return result;
}
void SetVariablesEditor::validateDraft() const {
    const auto config=decodeAssignmentConfig(parameters(),tool_.id);
    require(!config.rows.isEmpty(),"En az bir atama satırı gerekli");
    for(const auto& row:config.rows) {
        const VariableDefinition* target=nullptr;
        for(const auto& v:variables_)if(v.id==row.targetId)target=&v;
        require(target,"Invalid target UUID");
        if(const auto* constant=std::get_if<ConstantAssignmentSource>(&row.source)) {
            std::visit([&](const auto& envelope) {
                require(envelope.unit==target->unit,"Constant unit mismatch");
                using T=std::decay_t<decltype(envelope)>;
                if constexpr(std::is_same_v<T,ValueEnvelope>) {
                    require(int(envelope.type)==int(target->type),"Constant type mismatch");
                    EnumDefinitions enums;
                    if(target->type==VariableType::Enum) {
                        require(envelope.value && std::get<EnumValue>(*envelope.value).definitionId==target->enumDefinitionId,"Constant enum mismatch");
                        enums[target->enumDefinitionId]=target->enumCodes;
                    }
                    CommunicationValues::validate(envelope,enums);
                } else {
                    require(target->type==VariableType::SpatialTransform,"Constant spatial type mismatch");
                    if(target->frameConstraint)require(envelope.value->source==target->frameConstraint->source &&
                        envelope.value->target==target->frameConstraint->target,"Constant frame mismatch");
                }
            },constant->value);
        } else {
            const auto encoded=encodeAssignmentConfig({tool_.id,1,{row}})["rows"].toArray().first().toObject()["source"].toObject();
            bool compatible=false;
            for(const auto& choice:compatibleSources(*target,variables_,tools_,tool_.id))if(choice.second==encoded)compatible=true;
            require(compatible,"Kaynak type/unit/revision uyumsuz — yeniden seçin");
        }
    }
}
}
