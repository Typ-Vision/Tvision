#include "ResultEditor.hpp"
#include "domain/ResultDecision.hpp"
#include "domain/ToolRegistry.hpp"
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QRegularExpression>
namespace visionflow {
ResultEditor::ResultEditor(const ToolDefinition& tool,const QJsonObject& definition) {
    setObjectName("resultEditor");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);
    auto* help=new QLabel("Genel karar • salt okunur\nKaynakları seçip uygula; ardından Enabled aç.\nAçık Boolean çıkışlarını seçin. Eksik / atlanan girdi: NOT_EVALUATED. Match found/score otomatik kabul değildir.");help->setWordWrap(true);layout->addWidget(help);
    operation_=new QComboBox;operation_->setObjectName("resultOperator");operation_->addItems({"ALL","ANY"});operation_->setCurrentText(tool.parameters["operator"].toString());layout->addWidget(operation_);
    sources_=new QListWidget;sources_->setObjectName("resultSources");sources_->setMinimumHeight(130);sources_->setMaximumHeight(260);sources_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);layout->addWidget(sources_);
    const auto selected=tool.parameters["required"].toArray();QJsonArray seen;
    auto add=[&](const QString& text,const QJsonObject& binding,bool checked) {
        auto* item=new QListWidgetItem(text,sources_);item->setData(Qt::UserRole,binding);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(checked?Qt::Checked:Qt::Unchecked);item->setToolTip(text);seen.append(binding);
    };
    for(const auto& producer:editorTools(definition))if(producer.id!=tool.id && producer.kind!="result")for(const auto& port:outputPorts(producer.kind))if(port.type==PortType::Boolean && port.unit.isEmpty()) {
        QJsonObject binding{{"toolId",producer.id},{"port",port.id},{"revision",ToolRegistry::get(producer.kind).version}};
        add(producer.name+" / "+port.id+(producer.enabled?"":" [disabled]"),binding,selected.contains(binding));
    }
    for(const auto& b:selected)if(!seen.contains(b))add("Eksik / revision değişmiş: "+b.toObject()["toolId"].toString()+" / "+b.toObject()["port"].toString(),b.toObject(),true);
    error_=new QLabel;error_->setObjectName("resultError");error_->setWordWrap(true);error_->setMinimumWidth(0);error_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);error_->hide();layout->addWidget(error_);
}
QJsonObject ResultEditor::parameters() const {
    QJsonArray rows;for(int i=0;i<sources_->count();++i)if(sources_->item(i)->checkState()==Qt::Checked)rows.append(sources_->item(i)->data(Qt::UserRole).toJsonObject());
    return {{"operator",operation_->currentText()},{"required",rows}};
}
void ResultEditor::validateDraft() const {decodeResultConfig(parameters());}
void ResultEditor::error(const QString& text){auto wrapped=text;wrapped.replace(QRegularExpression("(\\S{24})(?=\\S)"),QString("\\1")+QChar(0x200b));error_->setText(wrapped);error_->show();}
}
