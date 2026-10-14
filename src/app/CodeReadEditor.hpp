#pragma once
#include <QWidget>
#include <QJsonObject>
#include <functional>
class QListWidget;
class QComboBox;
class QLineEdit;
class QSpinBox;
namespace visionflow {
class CodeReadEditor : public QWidget {
public:
    explicit CodeReadEditor(const QJsonObject&, QWidget* parent=nullptr);
    QJsonObject parameters() const;
    std::function<void()> onChanged;
private:
    QJsonObject original_;
    QListWidget* formats_;
    QComboBox *rule_, *selection_;
    QLineEdit* expected_;
    QSpinBox *maximum_, *budget_, *minimumExpected_, *maximumExpected_, *index_, *minimumLength_, *maximumLength_;
    void changed();
};
}
