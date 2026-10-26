#pragma once
#include "domain/ToolValues.hpp"
#include <QColor>
#include <QRect>
#include <QLineF>
namespace visionflow {
struct RuntimeOverlayLayer {
    QString toolId,name,state,detail;
    QColor color;
    QVector<QVector<QPointF>> scans;
    QVector<QLineF> guides,lines;
    QVector<QRect> regions;
    QVector<QPointF> points,rejected;
    struct Mask {QByteArray pixels;QSize size;QPoint origin;};
    Mask mask;
    CodeResults codes;
};
struct RuntimeOverlayCollection {
    QByteArray snapshotId;
    QString sourceId,frameId,cycleId,space;
    QSize rawSize;
    QVector<RuntimeOverlayLayer> layers;
};
}
