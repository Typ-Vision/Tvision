#pragma once
#include <QWidget>
#include <QJsonObject>
#include <functional>
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
namespace visionflow {
class EdgePointEditor : public QWidget {
public:
    EdgePointEditor(const QJsonObject&,QWidget* parent=nullptr);
    QJsonObject parameters() const;
    std::function<void()> onChanged;
private:
    QJsonObject original_;
    QComboBox *polarity_,*selection_;
    QCheckBox* subpixel_;
};
}
