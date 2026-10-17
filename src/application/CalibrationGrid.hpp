#pragma once
#include "vision/ImageFrame.hpp"
#include <QJsonObject>
#include <QPointF>
#include <QVector>
namespace visionflow {
enum class CalibrationGridPattern { Chessboard, SymmetricCircles };
struct CalibrationGridTarget {
    QString id;
    int columns=9,rows=6;
    // Historical field name retained for source compatibility. For circles this
    // is centre-to-centre spacing, never diameter. Codec uses spacingMm + kind.
    double squareMillimetres=0;
    CalibrationGridPattern pattern=CalibrationGridPattern::Chessboard;
    void validate() const;
    QJsonObject toJson() const;
    static CalibrationGridTarget fromJson(const QJsonObject&);
};
struct CalibrationGridObservation {
    CalibrationGridTarget target;
    FrameMetadata source;
    QSize resolution;
    QVector<QPointF> pixels,worldMillimetres;
    QString pixelHash;
};
// Existing callers and legacy chessboard codec keep their source/byte contract.
using ChessboardTarget=CalibrationGridTarget;
using ChessboardObservation=CalibrationGridObservation;
}
