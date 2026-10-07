#include "vision/matching/TemplateMatcher.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

int main() {
    std::cerr << "OpenCV " << CV_VERSION << ", threads=" << cv::getNumThreads()
              << ", synthetic CV_8UC1, match+minMaxLoc, 10 warmup/100 measured\n";
    std::cout << "method,template_px,search,width,height,median_ms,p95_ms,p99_ms,max_ms,correct,total\n";
    cv::Mat image(1944, 2592, CV_8UC1);
    cv::RNG rng(20261007);
    rng.fill(image, cv::RNG::UNIFORM, 0, 256);
    const char* names[] = {"TM_SQDIFF", "TM_SQDIFF_NORMED", "TM_CCORR",
                           "TM_CCORR_NORMED", "TM_CCOEFF", "TM_CCOEFF_NORMED"};
    for (int size : {32, 128}) {
        const cv::Point expected(800, 600);
        const cv::Mat model = image(cv::Rect(expected.x, expected.y, size, size)).clone();
        for (bool full : {true, false}) {
            const cv::Rect roi = full ? cv::Rect(0,0,image.cols,image.rows) : cv::Rect(700,500,512,512);
            for (int method = 0; method < 6; ++method) {
                cv::Mat scratch;
                for (int i=0; i<10; ++i) visionflow::matchTemplate(image, model, roi, scratch, method);
                std::vector<double> times;
                int correct = 0;
                for (int i=0; i<100; ++i) {
                    const auto start = std::chrono::steady_clock::now();
                    const auto result = visionflow::matchTemplate(image, model, roi, scratch, method);
                    const auto end = std::chrono::steady_clock::now();
                    times.push_back(std::chrono::duration<double,std::milli>(end-start).count());
                    correct += result.topLeft == expected;
                }
                std::sort(times.begin(),times.end());
                std::cout << names[method] << ',' << size << ',' << (full?"full":"roi") << ','
                          << roi.width << ',' << roi.height << ',' << times[49] << ',' << times[94]
                          << ',' << times[98] << ',' << times.back() << ',' << correct << ",100\n";
            }
        }
    }
}
