#include "PrecisionInput.hpp"
#include "EdgePointEditor.hpp"
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
namespace visionflow {
EdgePointEditor::EdgePointEditor(const QJsonObject& p,QWidget* parent):QWidget(parent),original_(p){
    setObjectName("edgePointSettings");auto* f=new QFormLayout(this);f->setContentsMargins(0,0,0,0);f->setRowWrapPolicy(QFormLayout::WrapAllRows);
    auto spin=[&](const char* key,double low,double high,const QString& label){auto* s=new QDoubleSpinBox;s->setObjectName(QString("ep_")+key);s->setDecimals(9);s->setRange(low,high);loadPrecisionInput(s,p[key].toDouble(),QString(key)!="minContrast",low>0);f->addRow(label,s);connect(s,&QDoubleSpinBox::editingFinished,this,[this]{if(onChanged)onChanged();});};
    spin("stepPx",.05,16,"Tarama adımı (px)");spin("widthStepPx",.25,16,"Enine adım (px)");spin("sigmaPx",0,16,"Gaussian sigma (px)");spin("minContrast",.000000001,5100,"Min kontrast (gray8/px, dahil)");
    polarity_=new QComboBox;polarity_->setObjectName("ep_polarity");polarity_->addItem("Açığa geçiş (A → B)","rising");polarity_->addItem("Koyuya geçiş (A → B)","falling");polarity_->addItem("Her iki yön","any");polarity_->setCurrentIndex(polarity_->findData(p["polarity"].toString()));f->addRow("Kenar yönü",polarity_);
    selection_=new QComboBox;selection_->setObjectName("ep_selection");selection_->addItem("İlk","first");selection_->addItem("Son","last");selection_->addItem("En güçlü","strongest");selection_->setCurrentIndex(selection_->findData(p["selection"].toString()));f->addRow("Seçim",selection_);
    subpixel_=new QCheckBox("Subpixel inceltme");subpixel_->setObjectName("ep_subpixel");subpixel_->setChecked(p["subpixel"].toBool());f->addRow(subpixel_);
    for(auto* c:{polarity_,selection_}){c->setMinimumContentsLength(8);c->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);connect(c,&QComboBox::currentIndexChanged,this,[this]{if(onChanged)onChanged();});}
    connect(subpixel_,&QCheckBox::toggled,this,[this]{if(onChanged)onChanged();});
    auto* help=new QLabel("Ok A → B tarama yönüdür. Ok ters çevrilirse aynı fiziksel kenar için polarity de ters çevrilir. Tam şerit görüntü içinde olmalıdır. Bulunamadı: geçerli nokta ve ölçüm yok. Alt piksel inceltme fiziksel hassasiyet garantisi değildir.");help->setWordWrap(true);f->addRow(help);
}
QJsonObject EdgePointEditor::parameters() const {
    auto p=original_;for(const auto key:{"stepPx","widthStepPx","sigmaPx","minContrast"}){auto* s=findChild<QDoubleSpinBox*>(QString("ep_")+key);p[key]=precisionValue(s);}
    p["polarity"]=polarity_->currentData().toString();p["selection"]=selection_->currentData().toString();p["subpixel"]=subpixel_->isChecked();return p;
}
}
