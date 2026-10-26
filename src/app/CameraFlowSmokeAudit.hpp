#pragma once
// Acceptance-only checks called by fixture/smoke screenshot paths.
#include "ImageViewer.hpp"
#include <QApplication>
#include <QWheelEvent>
#include <QEventLoop>
#include <QTimer>
#include <cmath>
#include <QAbstractButton>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QWidget>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <stdexcept>
namespace visionflow {
struct CameraFlowSmokeAudit {
    // Runtime uses the read-only layer collection, never the editable ROI slot.
    // Return rendered raw scan vertices so fixtures retain independent oracles.
    static QVector<QPointF> runtimeScan(const ImageViewer& viewer,const QString& tool) {
        if(!viewer.hasImage() || viewer.roi() || viewer.roiGestureActive())return {};
        for(const auto* item:viewer.scene()->items())if(item->data(0)=="runtimeOverlay" && item->data(1)==tool)
            if(const auto* scan=qgraphicsitem_cast<const QGraphicsPathItem*>(item))if(scan->pen().style()==Qt::DashLine){
                QVector<QPointF> points;const auto path=scan->path();for(int i=0;i<path.elementCount();++i){const auto p=path.elementAt(i);points<<QPointF(p.x,p.y);}if(points.size()>=5)return points;
            }
        return {};
    }
    static bool wheel(ImageViewer& viewer,QWidget& root,const QString& path) {
        if(!viewer.hasImage())throw std::runtime_error("Wheel acceptance needs a real fixture frame");
        viewer.actualSize();viewer.zoomBy(4);viewer.centerOn(viewer.sceneRect().center());QApplication::processEvents();
        const QPointF at(viewer.viewport()->width()*.65,viewer.viewport()->height()*.35);
        const auto before=viewer.viewportTransform().inverted().map(at);const auto roi=viewer.roi();
        QWheelEvent event(at,viewer.viewport()->mapToGlobal(at.toPoint()),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(viewer.viewport(),&event);QApplication::processEvents();
        const auto error=viewer.viewportTransform().map(before)-at;
        if(std::abs(error.x())>1 || std::abs(error.y())>1 || viewer.roi()!=roi)throw std::runtime_error(QString("Screen wheel anchor/ROI regression: error=%1,%2 zoom=%3 viewport=%4x%5").arg(error.x()).arg(error.y()).arg(viewer.zoomFactor()).arg(viewer.viewport()->width()).arg(viewer.viewport()->height()).toStdString());
        const bool saved=save(root,path);
        // Opt-in acceptance pause: permits native wheel input on these exact
        // populated screens without opening device connections or production.
        if(qEnvironmentVariableIsSet("VISIONFLOW_WHEEL_NATIVE_HOLD")){
            QEventLoop loop;QTimer::singleShot(45000,&loop,&QEventLoop::quit);loop.exec();
            auto nativePath=path;nativePath.replace("wheel-","native-after-");save(root,nativePath);
        }
        viewer.fitImage();return saved;
    }
    static void text(const QString& value,const QString& object) {
        for(const auto& removed:{QStringLiteral("Deneysel OECS"),QStringLiteral("Experimental OECS"),
                QStringLiteral("Tetik–kare"),QStringLiteral("Tetik-kare"),
                QStringLiteral("eşleşmesi doğrulan"),QStringLiteral("Önceki akış"),
                QStringLiteral("eski kanıt ekran")})
            if(value.contains(removed,Qt::CaseInsensitive))
                throw std::runtime_error(("Removed camera UI text in "+object+": "+value).toStdString());
    }
    static bool save(QWidget& root,const QString& path) {
        for(const auto name:{"cameraExperimental","cameraExperimentalNotice","overviewExperimental",
                "experimentalEditor","experimentalRuntime","calibrationLegacy"})
            if(root.findChild<QWidget*>(name))throw std::runtime_error("Removed camera UI widget exists");
        auto widgets=root.findChildren<QWidget*>();widgets.prepend(&root);
        for(auto* widget:widgets)if(widget==&root || widget->isVisibleTo(&root)){
            const auto id=widget->objectName();text(widget->toolTip(),id);
            if(auto* label=qobject_cast<QLabel*>(widget))text(label->text(),id);
            if(auto* button=qobject_cast<QAbstractButton*>(widget))text(button->text(),id);
            if(auto* combo=qobject_cast<QComboBox*>(widget))for(int i=0;i<combo->count();++i){
                text(combo->itemText(i),id);text(combo->itemData(i,Qt::ToolTipRole).toString(),id);
            }
            if(auto* table=qobject_cast<QTableWidget*>(widget))
                for(int r=0;r<table->rowCount();++r)for(int c=0;c<table->columnCount();++c)
                    if(auto* item=table->item(r,c)){text(item->text(),id);text(item->toolTip(),id);}
        }
        return root.grab().save(path);
    }
};
}
