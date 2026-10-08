#pragma once
#include "vision/ImageFrame.hpp"
#include <QVector>
#include <QPointF>
namespace visionflow {
// Raw pixel centres; world origin is detected corner 1, +X along columns,
// +Y along rows. A symmetric board has no physically identifiable orientation.
struct ChessboardTarget {
    QString id;
    int columns=9,rows=6;
    double squareMillimetres=0;
};
struct ChessboardObservation {
    ChessboardTarget target;
    FrameMetadata source;
    QSize resolution;
    QVector<QPointF> pixels,worldMillimetres;
};
struct CalibrationTestImage {
    ImageFrame frame;
    QString contentSha256;
};
class ChessboardService {
public:
    static CalibrationTestImage importImage(const QString& path);
    static ChessboardObservation detect(const ImageFrame&,const ChessboardTarget&);
};
}
