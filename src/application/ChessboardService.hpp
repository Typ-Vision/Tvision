#pragma once
#include "CalibrationGrid.hpp"
namespace visionflow {
// Raw pixel centres; world origin is detected corner 1, +X along columns,
// +Y along rows. A symmetric board has no physically identifiable orientation.
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
