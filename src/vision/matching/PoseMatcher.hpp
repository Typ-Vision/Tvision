#pragma once
#include <opencv2/core.hpp>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>
namespace visionflow {
struct PoseSearch {
    double angleMin=-30,angleMax=30,angleStep=10;
    double scaleMin=1,scaleMax=1,scaleStep=.1;
    double minScore=.8,ambiguityGap=.03,nmsDistance=8;
    int pyramidLevels=2,maxCandidates=16;
    bool selectBest=false;
    void validate() const;
};
// Angle is radians, +X toward +Y (clockwise in raw image coordinates).
// Scale is separate from the rigid pose. Anchor is model-local pixel centre.
struct PoseCandidate {cv::Point2d anchor;double angleRadians=0,scale=1,score=0;};
struct PoseSearchResult {
    std::vector<PoseCandidate> candidates;
    std::optional<PoseCandidate> selected;
    bool ambiguous=false,truncated=false;
};
class PoseMatcher {
public:
    static PoseSearchResult find(const cv::Mat& gray,const cv::Mat& model,cv::Point2d anchor,
        const cv::Rect& search,const PoseSearch&,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
