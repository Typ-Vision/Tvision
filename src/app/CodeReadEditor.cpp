#include "CodeReadEditor.hpp"
#include "domain/CodeReadConfig.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
namespace visionflow {
CodeReadEditor::CodeReadEditor(const QJsonObject& p,QWidget* parent):QWidget(parent),original_(p) {
    setObjectName("codeReadSettings");
    auto* form=new QFormLayout(this);form->setContentsMargins(0,0,0,0);form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    formats_=new QListWidget;formats_->setObjectName("codeFormats");formats_->setMaximumHeight(110);formats_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    for(const auto& f:codeFormats()){auto* item=new QListWidgetItem(f,formats_);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(p["formats"].toArray().contains(f) ? Qt::Checked:Qt::Unchecked);}
    form->addRow("Formatlar (en az bir)",formats_);
    auto spin=[&](const char* key,int low,int high,const QString& label){auto* s=new QSpinBox;s->setObjectName(QString("code_")+key);s->setRange(low,high);s->setValue(p[key].toInt());form->addRow(label,s);connect(s,&QSpinBox::editingFinished,this,[this]{changed();});return s;};
    minimumExpected_=spin("minCount",0,128,"Beklenen en az adet");maximumExpected_=spin("maxExpected",0,128,"Beklenen en çok adet");
    rule_=new QComboBox;rule_->setObjectName("codeRule");
    const QStringList rules{"none","exact","prefix","length","regex"},labels{"Yalnız adet","Tam metin","Metin başlangıcı","Uzunluk (UTF-16)","Regex (tam metin)"};
    for(int i=0;i<rules.size();++i)rule_->addItem(labels[i],rules[i]);rule_->setCurrentIndex(rule_->findData(p["rule"].toString()));form->addRow("İçerik kuralı",rule_);
    expected_=new QLineEdit(p["expected"].toString());expected_->setObjectName("codeExpected");expected_->setMaxLength(256);expected_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);form->addRow("Beklenen metin / regex",expected_);
    minimumLength_=spin("minLength",0,4096,"En az UTF-16 birimi");maximumLength_=spin("maxLength",0,4096,"En çok UTF-16 birimi");
    selection_=new QComboBox;selection_->setObjectName("codeSelection");selection_->addItem("Tüm kodlar","all");selection_->addItem("En az bir kod","any");selection_->addItem("Seçili sıra","selected");selection_->setCurrentIndex(selection_->findData(p["selection"].toString()));form->addRow("İçerik uygulaması",selection_);
    index_=spin("selectedIndex",0,127,"Kod sırası (0'dan)");maximum_=spin("maxCount",1,128,"Okuma üst sınırı");budget_=spin("budgetMs",1,60000,"Süre bütçesi (ms)");
    auto* note=new QLabel("Adet/içerik kabulü parça kararı değildir. Okunamadı: sembol yokluğu kanıtlanmaz. Karışık açık/koyu 1D kodlarda tam liste kabulü yoktur; adet kontrolünde tek polarite kullanın. Uzunluk UTF-16: emoji 2 birimdir; byte sayısı değildir. Regex: grup/alternatif/backreference yok, en çok bir tekrar. Stop/süre bütçesi native çağrıyı zorla kesmez.");note->setWordWrap(true);note->setTextFormat(Qt::PlainText);form->addRow(note);
    connect(formats_,&QListWidget::itemChanged,this,[this]{changed();});connect(rule_,&QComboBox::currentIndexChanged,this,[this]{changed();});connect(selection_,&QComboBox::currentIndexChanged,this,[this]{changed();});connect(expected_,&QLineEdit::editingFinished,this,[this]{changed();});
    for(auto* label:findChildren<QLabel*>())label->setWordWrap(true);
    for(auto* combo:{rule_,selection_}){combo->setMinimumContentsLength(8);combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);}
    changed();
}
QJsonObject CodeReadEditor::parameters() const {
    auto p=original_;QJsonArray formats;for(int i=0;i<formats_->count();++i)if(formats_->item(i)->checkState()==Qt::Checked)formats.append(formats_->item(i)->text());p["formats"]=formats;
    p["rule"]=rule_->currentData().toString();p["expected"]=expected_->text();p["selection"]=selection_->currentData().toString();
    p["minCount"]=minimumExpected_->value();p["maxExpected"]=maximumExpected_->value();p["minLength"]=minimumLength_->value();p["maxLength"]=maximumLength_->value();p["selectedIndex"]=index_->value();p["maxCount"]=maximum_->value();p["budgetMs"]=budget_->value();return p;
}
void CodeReadEditor::changed(){const auto rule=rule_->currentData().toString();expected_->setEnabled(rule=="exact" || rule=="prefix" || rule=="regex");minimumLength_->setEnabled(rule=="length");maximumLength_->setEnabled(rule=="length");index_->setEnabled(selection_->currentData()=="selected");if(onChanged)onChanged();}
}
