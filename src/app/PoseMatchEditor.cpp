#include "PoseMatchEditor.hpp"
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
namespace visionflow {
PoseMatchEditor::PoseMatchEditor(const QString& kind,const QJsonObject& p,QWidget* parent):QWidget(parent),original_(p) {
    setObjectName("poseMatchSettings");auto* form=new QFormLayout(this);form->setContentsMargins(0,0,0,0);form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    auto* help=new QLabel(kind=="shape_pose" ? "Shape: unsigned oriented chamfer. Skor yönteme özgüdür; olasılık değildir.":"Texture: masked NCC. Skor yönteme özgüdür; olasılık değildir.");help->setWordWrap(true);form->addRow(help);
    auto* hint=new QLabel("Model 16–256 px / Gray8. Anchor modelin sol üstüne göre px; yeni model için 0,0. Açı raw Y-down düzleminde saat yönünde derece. Tracking için scaleMin = scaleMax = 1 gerekir. Model veya anchor değişirse bağlı ROI yeniden öğretilmelidir.");hint->setWordWrap(true);form->addRow(hint);
    for(auto it=p.begin();it!=p.end();++it)if(it->isDouble()) {
        auto* n=new QDoubleSpinBox;n->setObjectName("pose_"+it.key());n->setDecimals(9);
        const auto key=it.key();double lo=0,hi=1;
        if(key=="angleMin" || key=="angleMax"){lo=-180;hi=180;}
        else if(key=="angleStep"){lo=1;hi=90;}
        else if(key=="scaleMin" || key=="scaleMax"){lo=.5;hi=2;}
        else if(key=="scaleStep"){lo=.02;hi=.5;}
        else if(key=="nmsDistance"){lo=2;hi=1024;}
        else if(key=="pyramidLevels" || key=="maxCandidates"){lo=key=="pyramidLevels" ? 1:2;hi=key=="pyramidLevels" ? 3:32;n->setDecimals(0);}
        n->setRange(lo,hi);n->setValue(it->toDouble());form->addRow(key,n);connect(n,&QDoubleSpinBox::editingFinished,this,[this]{if(onChanged)onChanged();});
    }
    auto* selection=new QComboBox;selection->setObjectName("pose_selection");selection->addItems({"reject_ambiguous","best"});selection->setCurrentText(p["selection"].toString());form->addRow("Selection",selection);connect(selection,&QComboBox::currentIndexChanged,this,[this]{if(onChanged)onChanged();});
    for(const auto key:{"x","y"}){auto* n=new QDoubleSpinBox;n->setObjectName(QString("pose_anchor_")+key);n->setDecimals(9);n->setRange(0,255);n->setValue(p["anchor"].toObject()[key].toDouble());form->addRow(QString("Anchor ")+key+" (model px)",n);connect(n,&QDoubleSpinBox::editingFinished,this,[this]{if(onChanged)onChanged();});}
}
QPointF PoseMatchEditor::teachingAnchor() const {return {findChild<QDoubleSpinBox*>("pose_anchor_x")->value(),findChild<QDoubleSpinBox*>("pose_anchor_y")->value()};}
QJsonObject PoseMatchEditor::parameters() const {
    auto p=original_;for(auto it=original_.begin();it!=original_.end();++it)if(it->isDouble())p[it.key()]=findChild<QDoubleSpinBox*>("pose_"+it.key())->value();
    p["selection"]=findChild<QComboBox*>("pose_selection")->currentText();
    if(!p["anchor"].toObject().isEmpty()){const auto a=teachingAnchor();p["anchor"]=QJsonObject{{"x",a.x()},{"y",a.y()}};}
    return p;
}
}
