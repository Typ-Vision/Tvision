#pragma once
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QWheelEvent>

namespace visionflow {
// One application filter also covers editable children and dynamically created
// modal dialogs. Never forward through QComboBox::wheelEvent (even with focus).
class ComboWheelPolicy final : public QObject {
    bool forwarding_ = false;
    static QScrollBar* scrollBar(QAbstractScrollArea* area, const QWheelEvent& wheel) {
        const auto delta = wheel.pixelDelta().isNull() ? wheel.angleDelta() : wheel.pixelDelta();
        const bool horizontal = qAbs(delta.x()) > qAbs(delta.y());
        auto* bar = horizontal ? area->horizontalScrollBar() : area->verticalScrollBar();
        const int amount = horizontal ? delta.x() : delta.y();
        if (!bar->isEnabled() || !amount) return nullptr;
        return (amount < 0 && bar->value() < bar->maximum()) ||
               (amount > 0 && bar->value() > bar->minimum()) ? bar : nullptr;
    }
    void forward(QScrollBar* bar, const QWheelEvent& wheel) {
        QScopedValueRollback<bool> guard(forwarding_, true);
        if (!wheel.pixelDelta().isNull()) {
            // QScrollBar uses angleDelta; preserve high-resolution pixel input.
            const auto delta = wheel.pixelDelta();
            bar->setValue(bar->value() - (bar->orientation() == Qt::Horizontal ? delta.x() : delta.y()));
        } else {
            // QScrollBar reverses native horizontal angle input. Normalize to
            // its vertical-wheel path so boundary routing and pixel direction agree.
            const QPoint angle = bar->orientation() == Qt::Horizontal
                ? QPoint(0, wheel.angleDelta().x()) : wheel.angleDelta();
            QWheelEvent copy(bar->mapFromGlobal(wheel.globalPosition().toPoint()), wheel.globalPosition(),
                             {}, angle, wheel.buttons(), wheel.modifiers(),
                             wheel.phase(), wheel.inverted(), wheel.source(), wheel.pointingDevice());
            copy.setTimestamp(wheel.timestamp());
            QApplication::sendEvent(bar, &copy);
        }
    }
    bool eventFilter(QObject* receiver, QEvent* event) override {
        if (forwarding_ || event->type() != QEvent::Wheel) return false;
        auto* target = qobject_cast<QWidget*>(receiver);
        auto* combo = static_cast<QComboBox*>(nullptr);
        for (auto* widget = target; widget; widget = widget->parentWidget()) {
            if ((combo = qobject_cast<QComboBox*>(widget))) break;
        }
        if (!combo) return false; // viewer/spinbox/other scroll input is untouched
        auto& wheel = *static_cast<QWheelEvent*>(event);
        bool handled = false;
        if (combo->view()->isVisible()) {
            if (auto* bar = scrollBar(combo->view(), wheel)) forward(bar, wheel);
            handled = true; // a popup boundary must not scroll the form underneath
        } else {
            for (auto* parent = combo->parentWidget(); parent; parent = parent->parentWidget()) {
                if (auto* area = qobject_cast<QAbstractScrollArea*>(parent)) {
                    if (auto* bar = scrollBar(area, wheel)) { forward(bar, wheel); handled = true; break; }
                }
            }
        }
        wheel.setAccepted(handled);
        return true; // consume once; no propagation through the combo or duplicate scroll
    }
public:
    explicit ComboWheelPolicy(QApplication& app) : QObject(&app) { app.installEventFilter(this); }
};
}
