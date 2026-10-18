#pragma once
#include <QLabel>
#include <QPainter>

namespace visionflow {
// Keep full source/frame identity available to accessibility and in the tooltip.
class ElidedLabel final : public QLabel {
public:
    explicit ElidedLabel(QWidget* parent=nullptr):QLabel(parent) {
        setTextFormat(Qt::PlainText);setWordWrap(false);
        setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);setMinimumWidth(0);
    }
    QSize sizeHint() const override {return {120,fontMetrics().height()+6};}
    QSize minimumSizeHint() const override {return {0,fontMetrics().height()+6};}
protected:
    void paintEvent(QPaintEvent*) override {
        setToolTip(text());
        QPainter painter(this);painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(contentsRect(),Qt::AlignVCenter|Qt::AlignLeft,fontMetrics().elidedText(text().simplified(),Qt::ElideRight,contentsRect().width()));
    }
};
}
