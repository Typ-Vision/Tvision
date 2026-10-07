#include "vision/matching/TemplateMatcher.hpp"
#include <stdexcept>

namespace visionflow {
TemplateMatchResult matchTemplate(const cv::Mat& gray, const cv::Mat& model,
                                 const cv::Rect& roi, cv::Mat& scratch, int method) {
    if (gray.empty() || model.empty() || gray.type() != CV_8UC1 || model.type() != CV_8UC1)
        throw std::invalid_argument("Matching requires nonempty CV_8UC1 images");
    if (roi.width <= 0 || roi.height <= 0 ||
        (roi & cv::Rect(0, 0, gray.cols, gray.rows)) != roi ||
        model.cols > roi.width || model.rows > roi.height)
        throw std::invalid_argument("Invalid search ROI or template dimensions");
    if (method < cv::TM_SQDIFF || method > cv::TM_CCOEFF_NORMED)
        throw std::invalid_argument("Unsupported matching method");
    cv::matchTemplate(gray(roi), model, scratch, method);
    double minScore, maxScore;
    cv::Point minPoint, maxPoint;
    cv::minMaxLoc(scratch, &minScore, &maxScore, &minPoint, &maxPoint);
    const bool minimum = method == cv::TM_SQDIFF || method == cv::TM_SQDIFF_NORMED;
    return {(minimum ? minPoint : maxPoint) + roi.tl(), minimum ? minScore : maxScore};
}
}
