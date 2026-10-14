#pragma once
#include "PoseMatcher.hpp"
namespace visionflow {
// Unsigned Sobel normals: contrast inversion preserves shape orientation.
// Score is bounded mean oriented chamfer similarity, never a probability.
class ShapeMatcher {
public:
    static void validateModel(const cv::Mat&);
    static PoseSearchResult find(const cv::Mat&,const cv::Mat&,cv::Point2d,
        const cv::Rect&,const PoseSearch&,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
