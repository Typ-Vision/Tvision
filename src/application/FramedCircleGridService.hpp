#pragma once
#include "CalibrationGrid.hpp"
#include <atomic>
#include <memory>
#include <QRect>
namespace visionflow {
// Search ROI is temporary raw-pixel localization context, never a raster crop.
class FramedCircleGridService {
public:
    static PlateDetectionResult observe(const ImageFrame&,const CalibrationGridTarget&,
        const std::shared_ptr<std::atomic_bool>& cancel={},const std::optional<QRect>& searchRoi={});
};
}
