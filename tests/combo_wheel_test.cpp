#include "app/ComboWheelPolicy.hpp"
#include <QDialog>
#include <QDir>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTest>
#include <QVBoxLayout>
#include <cstdio>
#include <stdexcept>

namespace {
void check(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
    std::printf("PASS: %s\n",why);
}
void wheel(QWidget* target, QPoint angle = {0,-120}, QPoint pixels = {}) {
    const QPoint local = target->rect().center();
    QWheelEvent event(local, target->mapToGlobal(local), pixels, angle,
                      Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
    QApplication::sendEvent(target, &event);
    QApplication::processEvents();
}
void fill(QComboBox* combo) {
    for (int i=0;i<100;++i) combo->addItem(QString("Camera / setting %1").arg(i));
    combo->setCurrentIndex(5);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");
    visionflow::ComboWheelPolicy policy(app);
    try {
        QDialog dialog;
        dialog.setWindowTitle("EFR-02 Combo wheel interaction acceptance");
        auto* layout = new QVBoxLayout(&dialog);
        layout->addWidget(new QLabel("Wheel scrolls the form; explicit click / Enter selects."));
        auto* outside = new QComboBox; fill(outside); layout->addWidget(outside);
        auto* focus = new QPushButton("Focus outside combo"); layout->addWidget(focus);
        auto* outer = new QScrollArea; layout->addWidget(outer);
        auto* body = new QWidget; auto* bodyLayout = new QVBoxLayout(body);
        outer->setWidget(body); outer->setWidgetResizable(true);
        auto* inner = new QScrollArea; bodyLayout->addWidget(inner);
        inner->setFixedHeight(240);
        auto* content = new QWidget; auto* form = new QVBoxLayout(content);
        inner->setWidget(content); inner->setWidgetResizable(true);
        auto* combo = new QComboBox; fill(combo); combo->setMaximumWidth(400); form->addWidget(combo);
        auto* editable = new QComboBox; fill(editable); editable->setEditable(true); editable->setMaximumWidth(400); form->addWidget(editable);
        form->addStretch();
        content->setMinimumSize(1800,900);
        bodyLayout->addSpacing(1200);
        dialog.resize(1000,720); dialog.show(); app.processEvents();
        QSignalSpy changed(combo,&QComboBox::currentIndexChanged);
        QSignalSpy activated(combo,&QComboBox::activated);
        QSignalSpy edits(editable,&QComboBox::currentTextChanged);
        int writes=0; QObject::connect(combo,&QComboBox::currentIndexChanged,[&]{++writes;});
        for (const bool focused : {false,true}) {
            if (focused) combo->setFocus(); else focus->setFocus();
            inner->verticalScrollBar()->setValue(100);
            outer->verticalScrollBar()->setValue(0);
            int innerMoves=0, outerMoves=0;
            auto c1=QObject::connect(inner->verticalScrollBar(),&QScrollBar::valueChanged,[&]{++innerMoves;});
            auto c2=QObject::connect(outer->verticalScrollBar(),&QScrollBar::valueChanged,[&]{++outerMoves;});
            wheel(combo);
            check(innerMoves==1 && outerMoves==0,"closed wheel scrolls nearest area exactly once");
            QObject::disconnect(c1); QObject::disconnect(c2);
        }
        inner->verticalScrollBar()->setValue(inner->verticalScrollBar()->maximum());
        outer->verticalScrollBar()->setValue(0); wheel(combo);
        check(outer->verticalScrollBar()->value()>0,"nested boundary forwards to outer area");
        inner->horizontalScrollBar()->setValue(100); wheel(combo,{-120,0});
        check(inner->horizontalScrollBar()->value()>100,"horizontal angle input scrolls");
        inner->verticalScrollBar()->setValue(100); wheel(combo,{}, {0,-17});
        check(inner->verticalScrollBar()->value()==117,"pixel-only trackpad input");
        inner->horizontalScrollBar()->setValue(100); wheel(combo,{}, {-13,0});
        check(inner->horizontalScrollBar()->value()==113,"horizontal pixel input");
        combo->setEnabled(false); wheel(combo); combo->setEnabled(true);
        wheel(outside); check(outside->currentIndex()==5,"no scroll ancestor preserves value");
        wheel(editable->lineEdit(),{}, {0,-15});
        check(edits.count()==0 && editable->currentText()=="Camera / setting 5","editable text preserved");
        check(changed.count()==0 && activated.count()==0 && writes==0 && combo->currentIndex()==5,
              "all closed wheel inputs preserve index and cause zero setting writes");
        // Popup wheel scroll never commits. Escape restores the existing selection.
        outer->verticalScrollBar()->setValue(0); inner->verticalScrollBar()->setValue(0);
        combo->showPopup(); app.processEvents();
        auto* view=combo->view();
        view->verticalScrollBar()->setValue(0); wheel(view->viewport());
        check(view->verticalScrollBar()->value()>0 && changed.count()==0 && writes==0,"popup scroll without commit");
        QTest::keyClick(view,Qt::Key_Escape); app.processEvents();
        check(combo->currentIndex()==5 && changed.count()==0,"popup scroll then Escape");
        combo->showPopup(); app.processEvents(); wheel(view->viewport());
        const QModelIndex chosen=combo->model()->index(8,0); view->scrollTo(chosen); app.processEvents();
        QTest::mouseClick(view->viewport(),Qt::LeftButton,Qt::NoModifier,view->visualRect(chosen).center());
        check(combo->currentIndex()==8 && writes==1 && activated.count()==1,"scroll then click commits once");
        combo->showPopup(); app.processEvents(); wheel(view->viewport());
        view->setCurrentIndex(combo->model()->index(8,0)); view->scrollTo(view->currentIndex());
        QTest::keyClick(view,Qt::Key_Down); QTest::keyClick(view,Qt::Key_Return); app.processEvents();
        check(combo->currentIndex()==9 && writes==2 && activated.count()==2,"popup arrow then Enter commits once");
        combo->setFocus(); QTest::keyClick(combo,Qt::Key_Down);
        check(combo->currentIndex()==10 && writes==3,"closed keyboard selection preserved");
        editable->setFocus(); editable->lineEdit()->selectAll(); QTest::keyClicks(editable->lineEdit(),"typed");
        check(editable->currentText()=="typed","editable keyboard text entry preserved");
        QDialog modal(&dialog); modal.setModal(true); auto* modalLayout=new QVBoxLayout(&modal);
        auto* lateCombo=new QComboBox; fill(lateCombo); modalLayout->addWidget(lateCombo);
        modal.show(); app.processEvents(); QSignalSpy late(lateCombo,&QComboBox::currentIndexChanged); wheel(lateCombo);
        check(lateCombo->currentIndex()==5 && late.count()==0,"late modal combo covered"); modal.close();
        QSpinBox spin; spin.setRange(0,100); spin.setValue(50); wheel(&spin);
        check(spin.value()!=50,"spinbox wheel unchanged");
        QScrollArea ordinary; auto* large=new QWidget; large->setMinimumSize(100,1000); ordinary.setWidget(large); ordinary.show(); app.processEvents();
        wheel(ordinary.viewport()); check(ordinary.verticalScrollBar()->value()>0,"non-combo scroll untouched"); ordinary.close();
        const QDir output(qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS","build/efr/efr-02")); QDir().mkpath(output.absolutePath());
        for (auto size : {QSize(1000,720),QSize(650,480)}) {
            dialog.resize(size); app.processEvents();
            outer->verticalScrollBar()->setValue(0); inner->verticalScrollBar()->setValue(0);
            inner->horizontalScrollBar()->setValue(0); app.processEvents();
            check(dialog.grab().save(output.filePath(QString("combo-%1x%2.png").arg(size.width()).arg(size.height()))),"save visual evidence");
            combo->showPopup(); app.processEvents(); wheel(view->viewport());
            check(dialog.grab().save(output.filePath(QString("popup-%1x%2.png").arg(size.width()).arg(size.height()))),"save popup context");
            check(combo->view()->window()->grab().save(output.filePath(QString("popup-list-%1.png").arg(size.width()))),"save popup list");
            QTest::keyClick(view,Qt::Key_Escape);
        }
        std::puts("PASS: focused/unfocused/closed/open/editable/disabled/nested/boundary/modal/dynamic; vertical/horizontal angle/pixel; zero wheel writes; click/Enter/keyboard commit; spinbox/scroll regression");
    } catch (const std::exception& e) { std::fprintf(stderr,"FAIL: %s\n",e.what()); return 1; }
    return 0;
}
