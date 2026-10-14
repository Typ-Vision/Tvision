#include "PreprocessEditor.hpp"
#include <QComboBox>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QJsonArray>
namespace visionflow {
PreprocessEditor::PreprocessEditor(const QJsonObject& p,QWidget* parent):QWidget(parent) {
    setObjectName("preprocessSettings");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);
    channel_=new QComboBox;channel_->setObjectName("preprocessChannel");
    channel_->addItem("Gri (Gray8)","gray");channel_->addItem("Kırmızı","red");channel_->addItem("Yeşil","green");channel_->addItem("Mavi","blue");channel_->addItem("Alfa","alpha");channel_->setCurrentIndex(channel_->findData(p["channel"].toString()));layout->addWidget(channel_);
    steps_=new QListWidget;steps_->setObjectName("preprocessSteps");steps_->setMaximumHeight(90);layout->addWidget(steps_);
    for(const auto& v:p["steps"].toArray()){auto* item=new QListWidgetItem(v.toObject()["op"].toString(),steps_);item->setData(Qt::UserRole,v.toObject());}
    auto* form=new QFormLayout;form->setRowWrapPolicy(QFormLayout::WrapAllRows);operation_=new QComboBox;operation_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);operation_->setMinimumContentsLength(8);operation_->setObjectName("preprocessOperation");
    const QStringList names{"Gaussian","Median","Eşik (> değer)","Adaptif ortalama","Adaptif Gaussian","Erode","Dilate","Open","Close"};
    const QStringList ops{"gaussian","median","threshold","adaptive_mean","adaptive_gaussian","erode","dilate","open","close"};
    for(int i=0;i<ops.size();++i)operation_->addItem(names[i],ops[i]);form->addRow("Filtre",operation_);
    size_=new QComboBox;for(int k=3;k<=31;k+=2)size_->addItem(QString::number(k),k);form->addRow("Filtre boyutu (px)",size_);
    value_=new QDoubleSpinBox;value_->setDecimals(2);value_->setRange(-255,255);valueLabel_=new QLabel("Değer");form->addRow(valueLabel_,value_);layout->addLayout(form);
    auto* buttons=new QGridLayout;int buttonIndex=0;
    for(const auto& label:QStringList{"Ekle","Sil","↑","↓"}){auto* b=new QPushButton(label);b->setObjectName(label=="Ekle" ? "preprocessAdd":label=="Sil" ? "preprocessDelete":label=="↑" ? "preprocessUp":"preprocessDown");buttons->addWidget(b,buttonIndex/2,buttonIndex%2);++buttonIndex;connect(b,&QPushButton::clicked,this,[this,label]{
        int row=steps_->currentRow();if(label=="Ekle"){if(steps_->count()>=16)return;auto* item=new QListWidgetItem("gaussian",steps_);item->setData(Qt::UserRole,QJsonObject{{"op","gaussian"},{"size",3},{"value",0}});steps_->setCurrentItem(item);}
        else if(row>=0){if(label=="Sil")delete steps_->takeItem(row);else {const int next=row+(label=="↑" ? -1:1);if(next<0 || next>=steps_->count())return;auto* item=steps_->takeItem(row);steps_->insertItem(next,item);steps_->setCurrentItem(item);}}
        loadStep();changed();});}layout->addLayout(buttons);
    auto* note=new QLabel("Sıra yukarıdan aşağıya. Tam kare, Gray8 çıktı; Gray16 → 0–255. Kenarlar tekrar edilir; maske/crop/resize yok. Alfa/renk kanalı kaynakta bulunmalıdır.");note->setWordWrap(true);layout->addWidget(note);
    connect(channel_,&QComboBox::currentIndexChanged,this,[this]{changed();});connect(steps_,&QListWidget::currentRowChanged,this,[this]{loadStep();});
    connect(operation_,&QComboBox::currentIndexChanged,this,[this]{if(loading_)return;loading_=true;configureValue();value_->setValue(operation_->currentData()=="threshold" ? 128:0);loading_=false;saveStep();});
    connect(size_,&QComboBox::currentIndexChanged,this,[this]{saveStep();});connect(value_,&QDoubleSpinBox::editingFinished,this,[this]{saveStep();});
    if(steps_->count())steps_->setCurrentRow(0);loadStep();
}
QJsonObject PreprocessEditor::parameters() const {QJsonArray array;for(int i=0;i<steps_->count();++i)array.append(steps_->item(i)->data(Qt::UserRole).toJsonObject());return {{"channel",channel_->currentData().toString()},{"steps",array}};}
void PreprocessEditor::changed(){if(!loading_ && onChanged)onChanged();}
void PreprocessEditor::loadStep(){loading_=true;auto* item=steps_->currentItem();operation_->setEnabled(item);size_->setEnabled(item);value_->setEnabled(item);if(item){auto s=item->data(Qt::UserRole).toJsonObject();operation_->setCurrentIndex(operation_->findData(s["op"].toString()));size_->setCurrentIndex(size_->findData(s["size"].toInt()));configureValue();value_->setValue(s["value"].toDouble());}loading_=false;}
void PreprocessEditor::configureValue(){
    const auto op=operation_->currentData().toString();
    value_->setEnabled(steps_->currentItem() && (op=="gaussian" || op=="threshold" || op.startsWith("adaptive")));
    size_->setEnabled(steps_->currentItem() && op!="threshold");
    value_->setRange(op.startsWith("adaptive") ? -255:0,op=="gaussian" ? 100:255);
    valueLabel_->setText(op=="gaussian" ? "Sigma (0: otomatik)":op=="threshold" ? "Eşik (0–255)":op.startsWith("adaptive") ? "C (çıkarılan sabit)":"Değer kullanılmaz");
}
void PreprocessEditor::saveStep(){if(loading_ || !steps_->currentItem())return;const auto op=operation_->currentData().toString();const bool parameter=op=="gaussian" || op=="threshold" || op.startsWith("adaptive");double v=parameter ? value_->value():0;loading_=true;value_->setValue(v);loading_=false;steps_->currentItem()->setData(Qt::UserRole,QJsonObject{{"op",op},{"size",size_->currentData().toInt()},{"value",v}});steps_->currentItem()->setText(operation_->currentText());changed();}
}
