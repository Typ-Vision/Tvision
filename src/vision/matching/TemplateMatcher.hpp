#pragma once
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace visionflow {
struct TemplateMatchResult {
    cv::Point topLeft; // Full image pixel coordinates, not calibrated coordinates.
    double rawScore{}; // Method-specific; not a probability or universal percentage.
};
// Fixed angle and scale only. ROI is an absolute image rectangle.
TemplateMatchResult matchTemplate(const cv::Mat& gray, const cv::Mat& model,
                                 const cv::Rect& searchRoi, cv::Mat& scratch,
                                 int method = cv::TM_CCOEFF_NORMED);
}
