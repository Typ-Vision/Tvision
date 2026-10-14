#pragma once
#include "domain/ToolRoi.hpp"
#include <opencv2/core.hpp>
#include <atomic>
#include <memory>
namespace visionflow {
// Crop bounds are storage only: membership uses the rotated rectangle at raw pixel centres.
class RectangleMask {
public:
    RectangleMask(const ToolRoi&,const QSize&,const std::shared_ptr<std::atomic_bool>& cancel={});
    bool contains(QPointF raw) const;
    QRect bounds;
    cv::Mat pixels;
private:
    RotatedRectangle rectangle;
    double cosine=1,sine=0;
};
}
