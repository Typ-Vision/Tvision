#pragma once
#include <QWidget>
#include <QJsonObject>
#include <QPointF>
#include <functional>
namespace visionflow {
class PoseMatchEditor : public QWidget {
public:
    PoseMatchEditor(const QString& kind,const QJsonObject&,QWidget* parent=nullptr);
    QJsonObject parameters() const;
    QPointF teachingAnchor() const;
    std::function<void()> onChanged;
private:
    QJsonObject original_;
};
}
