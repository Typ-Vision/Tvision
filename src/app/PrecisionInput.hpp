#pragma once
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QKeyEvent>
#include <algorithm>

namespace visionflow {
// UI precision is independent of model precision. Only explicit edits replace
// the original value, including typing the already displayed rounded value.
class PrecisionInputState final : public QObject {
    QDoubleSpinBox* spin_;
    bool eventFilter(QObject*, QEvent* event) override {
        if(event->type()==QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key()==Qt::Key_Escape) {
            spin_->setValue(spin_->property("precisionDisplay").toDouble());
            spin_->setProperty("precisionEdited",false);
        }
        return false;
    }
public:
    explicit PrecisionInputState(QDoubleSpinBox* spin):QObject(spin),spin_(spin) {
        spin->installEventFilter(this);
        auto* text=spin->findChild<QLineEdit*>();text->installEventFilter(this);
        connect(text,&QLineEdit::textEdited,spin,[spin]{spin->setProperty("precisionEdited",true);});
        connect(spin,&QDoubleSpinBox::valueChanged,spin,[spin]{
            if(spin->property("precisionOriginal").isValid())spin->setProperty("precisionEdited",true);
        });
        connect(spin,&QDoubleSpinBox::editingFinished,spin,[spin]{
            if((spin->property("precisionEdited").toBool() || spin->value()!=spin->property("precisionDisplay").toDouble()) && spin->property("precisionPositive").toBool())
                spin->setValue(std::max(.001,spin->value()));
        });
    }
};
inline void loadPrecisionInput(QDoubleSpinBox* spin,double original,bool length=false,bool positive=false) {
    if(!spin->property("precisionOriginal").isValid())new PrecisionInputState(spin);
    if(length){spin->setDecimals(3);spin->setSingleStep(.001);if(positive)spin->setMinimum(original>0 && original<.001 ? 0:std::max(.001,spin->minimum()));}
    spin->setValue(original);
    spin->setProperty("precisionOriginal",original);spin->setProperty("precisionDisplay",spin->value());
    spin->setProperty("precisionEdited",false);spin->setProperty("precisionPositive",length && positive);
    spin->setToolTip(spin->toolTip()+QString("\nÖzgün değer: %1").arg(original,0,'g',17));
}
inline bool precisionInputEdited(const QDoubleSpinBox* spin) {return spin->property("precisionEdited").toBool() || spin->value()!=spin->property("precisionDisplay").toDouble();}
inline double precisionValue(const QDoubleSpinBox* spin) {
    if(!spin->property("precisionOriginal").isValid())return spin->value();
    const bool edited=spin->property("precisionEdited").toBool() || spin->value()!=spin->property("precisionDisplay").toDouble();
    return edited ? (spin->property("precisionPositive").toBool() ? std::max(.001,spin->value()):spin->value()):spin->property("precisionOriginal").toDouble();
}
inline bool coordinateLength(const QString& key){return key!="angle" && key!="start" && key!="sweep";}
inline bool positiveExtent(const QString& key){return key=="width" || key=="height" || key=="outer" || key=="inner";}
}
