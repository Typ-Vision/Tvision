#pragma once
#include <QWidget>
#include <QJsonObject>
#include <functional>
#include "domain/ToolRoi.hpp"
namespace visionflow {
class MetrologyEditor:public QWidget {
public:
    MetrologyEditor(const QString& kind,const QJsonObject&,const QString& frameId,bool hasFrame,QWidget* parent=nullptr);
    QJsonObject parameters() const;
    void setImageFrame(const QString& frameId,bool ready);
    std::function<void()> onChanged,onDraw;
    std::function<void(const ToolRoi&)> onRoi,onPreview;
    std::optional<ToolRoi> coordinateRoi() const;
private:
    QString kind_,frameId_;
    QJsonObject original_;
};
}
