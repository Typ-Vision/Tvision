#pragma once
#include <QWidget>
#include <QJsonObject>
#include <functional>
class QComboBox;class QListWidget;class QDoubleSpinBox;class QLabel;
namespace visionflow {
class PreprocessEditor:public QWidget {
public:
    explicit PreprocessEditor(const QJsonObject&,QWidget* parent=nullptr);
    QJsonObject parameters() const;
    std::function<void()> onChanged;
private:
    QComboBox *channel_,*operation_;
    QListWidget* steps_;
    QComboBox* size_;
    QLabel* valueLabel_;
    QDoubleSpinBox* value_;
    bool loading_=false;
    void loadStep();void saveStep();void changed();void configureValue();
};
}
