#pragma once
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>

namespace visionflow {
// Reserves layout space only; the navigation is a sibling overlay in the host.
class WorkspacePanel final : public QWidget {
public:
    WorkspacePanel(QWidget* host, QWidget* panel, const QString& settings)
        : QWidget(host), panel_(panel), settings_(settings) {
        setObjectName("workspaceRail");
        panel_->setParent(host);
        panel_->setFocusPolicy(Qt::StrongFocus);
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);
        rail_=new QPushButton(this);rail_->setObjectName("workspaceReveal");
        QPixmap bars(16,16);bars.fill(Qt::transparent);{QPainter p(&bars);p.setPen(QPen(QColor("#8fa0b5"),1.5));for(int y:{4,8,12})p.drawLine(2,y,14,y);}rail_->setIcon(QIcon(bars));
        rail_->setAccessibleName("Workspace aç");rail_->setToolTip("Workspace aç (Enter)");
        rail_->setFixedWidth(28);rail_->setStyleSheet("padding: 6px 0px;");
        layout->addWidget(rail_);layout->addStretch();
        pin_=new QPushButton(panel_);pin_->setObjectName("workspacePin");
        pin_->setFixedSize(28,28);pin_->setStyleSheet("padding: 0px;");
        pin_->setCheckable(true);
        auto* side=qobject_cast<QVBoxLayout*>(panel_->layout());
        side->insertWidget(0,pin_,0,Qt::AlignRight);
        open_.setSingleShot(true);open_.setInterval(150);
        close_.setSingleShot(true);close_.setInterval(350);
        connect(&open_,&QTimer::timeout,this,[this]{reveal();});
        connect(&close_,&QTimer::timeout,this,[this]{
            if(held()) {close_.start();return;}
            collapse(false);
        });
        connect(rail_,&QPushButton::clicked,this,[this]{reveal();panel_->setFocus();});
        connect(pin_,&QPushButton::clicked,this,[this](bool pinned){setPinned(pinned);});
        qApp->installEventFilter(this);
        setPinned(QSettings(settings_,QSettings::IniFormat).value("workspace/pinned",true).toBool());
    }
    bool pinned() const {return pinned_;}
    bool expanded() const {return panel_->isVisible();}
    void setAvailable(bool available) {available_=available;open_.stop();close_.stop();setVisible(available);panel_->setVisible(available && pinned_);}
    void setPinned(bool pinned) {
        pinned_=pinned;open_.stop();close_.stop();
        pin_->setChecked(pinned);
        QPixmap glyph(16,16);glyph.fill(Qt::transparent);{QPainter p(&glyph);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(QColor(pinned ? "#66e3c4":"#8fa0b5"),1.5));p.drawLine(5,3,11,3);p.drawLine(6,3,6,8);p.drawLine(10,3,10,8);p.drawLine(3,9,13,9);p.drawLine(8,9,8,15);if(!pinned)p.drawLine(2,14,14,2);}pin_->setIcon(QIcon(glyph));
        pin_->setToolTip(pinned ? "Otomatik gizle":"Sabitle");pin_->setAccessibleName(pin_->toolTip());
        setFixedWidth(pinned ? 215:28);rail_->setVisible(!pinned);
        panel_->setVisible(pinned);place();
        QSettings(settings_,QSettings::IniFormat).setValue("workspace/pinned",pinned);
        if(onPinnedChanged)onPinnedChanged(pinned);
    }
    void reveal() {if(available_ && !pinned_){close_.stop();panel_->show();place();panel_->raise();}}
    std::function<void(bool)> onPinnedChanged;
protected:
    bool eventFilter(QObject* target,QEvent* event) override {
        if(!panel_ || !available_)return false;
        if(target==parentWidget() && (event->type()==QEvent::Resize || event->type()==QEvent::LayoutRequest))place();
        const bool inside=target==panel_ || target==this || target==rail_;
        if(inside && event->type()==QEvent::Enter){close_.stop();if(!pinned_)open_.start();}
        if(inside && event->type()==QEvent::Leave){open_.stop();if(!pinned_)close_.start();}
        if(event->type()==QEvent::FocusOut && !pinned_ && expanded())close_.start();
        if(event->type()==QEvent::KeyPress && !pinned_ && expanded()) {
            auto* widget=qobject_cast<QWidget*>(target);
            if(widget && (widget==panel_ || panel_->isAncestorOf(widget)) && static_cast<QKeyEvent*>(event)->key()==Qt::Key_Escape && !QApplication::activePopupWidget() && !QApplication::activeModalWidget()) {
                collapse(true);return true;
            }
        }
        return QWidget::eventFilter(target,event);
    }
private:
    bool held() const {
        auto* focus=QApplication::focusWidget();
        return (focus && (focus==panel_ || panel_->isAncestorOf(focus))) ||
            QApplication::activePopupWidget() || QApplication::activeModalWidget() ||
            QApplication::mouseButtons()!=Qt::NoButton || panel_->underMouse() || underMouse();
    }
    void collapse(bool keyboard) {open_.stop();close_.stop();panel_->hide();if(keyboard)rail_->setFocus();}
    void place() {panel_->setGeometry(x(),y(),215,parentWidget()->height());if(panel_->isVisible())panel_->raise();}
    QPointer<QWidget> panel_;
    QPushButton *rail_{},*pin_{};
    QString settings_;
    QTimer open_,close_;
    bool pinned_=true;
    bool available_=true;
};
}
