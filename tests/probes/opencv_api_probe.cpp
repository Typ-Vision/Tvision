#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(double actual, double expected, double tolerance = 1e-5) {
    return std::isfinite(actual) && std::abs(actual - expected) <= tolerance;
}
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: opencv_api_probe output-directory\n"; return 1; }
    const std::filesystem::path output(argv[1]);
    std::filesystem::create_directories(output);
    std::ofstream buildInfo(output / "opencv-build-information.txt");
    buildInfo << cv::getBuildInformation();
    if (!buildInfo) return 1;
    buildInfo.close();
    cv::FileStorage report((output / "opencv-api-results.json").string(),
                           cv::FileStorage::WRITE | cv::FileStorage::FORMAT_JSON);
    if (!report.isOpened()) return 1;
    report << "compiledVersion" << CV_VERSION << "runtimeVersion" << cv::getVersionString();
    report << "fixtureScope" << "synthetic API probes; QR uses OpenCV encoder, not an independent corpus";
    report << "cases" << "[";
    int failures = 0;
    const auto run = [&](const char* name, const std::function<void()>& probe) {
        report << "{" << "name" << name;
        try {
            probe();
            report << "state" << "PASS";
            std::cout << name << ": PASS\n";
        } catch (const std::exception& error) {
            ++failures;
            report << "state" << "FAIL" << "reason" << error.what();
            std::cerr << name << ": FAIL: " << error.what() << '\n';
        }
        report << "}";
    };
    run("filters", [&] {
        cv::Mat input(32, 32, CV_8UC1, cv::Scalar(100)), gaussian, median, binary, adaptive, morph;
        cv::GaussianBlur(input, gaussian, {3, 3}, 0);
        cv::medianBlur(input, median, 3);
        cv::threshold(input, binary, 99, 255, cv::THRESH_BINARY);
        cv::adaptiveThreshold(input, adaptive, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 3, 1);
        cv::morphologyEx(binary, morph, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, {3, 3}));
        require(cv::norm(input, gaussian, cv::NORM_INF) == 0 && cv::norm(input, median, cv::NORM_INF) == 0,
                "constant image changed by blur");
        require(cv::countNonZero(morph) == 1024 && cv::countNonZero(adaptive) == 1024,
                "threshold/morphology oracle mismatch");
    });
    run("remap", [&] {
        cv::Mat input(8, 8, CV_32FC1), mapX(8, 8, CV_32FC1), mapY(8, 8, CV_32FC1), mapped;
        for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x) {
            input.at<float>(y, x) = static_cast<float>(x + 10 * y);
            mapX.at<float>(y, x) = static_cast<float>(x) + 0.5f;
            mapY.at<float>(y, x) = static_cast<float>(y) + 0.5f;
        }
        cv::remap(input, mapped, mapX, mapY, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(-1));
        require(near(mapped.at<float>(2, 3), 28.5), "bilinear ramp sample mismatch");
    });
    run("fitLine", [&] {
        std::vector<cv::Point2f> points{{0, 1}, {1, 3}, {2, 5}, {3, 7}, {4, 9}};
        cv::Vec4f line;
        cv::fitLine(points, line, cv::DIST_L2, 0, 0.01, 0.01);
        require(near(std::abs(line[1] / line[0]), 2), "analytic line slope mismatch");
        require(near(line[3] - 2 * line[2], 1), "analytic line intercept mismatch");
    });
    run("fitEllipse", [&] {
        std::vector<cv::Point2f> points;
        for (int i = 0; i < 72; ++i) {
            const double angle = i * 2 * CV_PI / 72;
            points.emplace_back(static_cast<float>(50 + 20 * std::cos(angle)),
                                static_cast<float>(40 + 10 * std::sin(angle)));
        }
        for (const auto& fit : {cv::fitEllipse(points), cv::fitEllipseAMS(points), cv::fitEllipseDirect(points)}) {
            require(near(fit.center.x, 50, 1e-3) && near(fit.center.y, 40, 1e-3), "ellipse centre mismatch");
            require(near(std::min(fit.size.width, fit.size.height), 20, 1e-3) &&
                    near(std::max(fit.size.width, fit.size.height), 40, 1e-3), "ellipse axes mismatch");
        }
    });
    run("contour", [&] {
        cv::Mat input = cv::Mat::zeros(32, 32, CV_8UC1);
        cv::rectangle(input, {5, 7}, {15, 17}, cv::Scalar(255), cv::FILLED);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(input, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        require(contours.size() == 1, "rectangle contour count mismatch");
        require(near(cv::contourArea(contours[0]), 100) && near(cv::arcLength(contours[0], true), 40),
                "pixel-centre rectangle geometry mismatch");
        const auto moments = cv::moments(contours[0]);
        require(near(moments.m10 / moments.m00, 10) && near(moments.m01 / moments.m00, 12),
                "rectangle centroid mismatch");
    });
    run("homography", [&] {
        const std::vector<cv::Point2f> source{{0, 0}, {10, 0}, {10, 10}, {0, 10}};
        const std::vector<cv::Point2f> target{{3, 4}, {23, 4}, {23, 34}, {3, 34}};
        const cv::Mat h = cv::findHomography(source, target, 0);
        std::vector<cv::Point2f> mapped;
        cv::perspectiveTransform(std::vector<cv::Point2f>{{2, 3}, {7, 8}}, mapped, h);
        require(mapped.size() == 2 && near(mapped[0].x, 7) && near(mapped[0].y, 13) &&
                near(mapped[1].x, 17) && near(mapped[1].y, 28), "independent holdout mapping mismatch");
    });
    run("qr_decode", [&] {
        const std::string expected = "VISIONFLOW-T0.1";
        cv::Mat modules, bordered, image, corners;
        cv::QRCodeEncoder::create()->encode(expected, modules);
        cv::copyMakeBorder(modules, bordered, 4, 4, 4, 4, cv::BORDER_CONSTANT, cv::Scalar(255));
        cv::resize(bordered, image, {}, 8, 8, cv::INTER_NEAREST);
        require(cv::imwrite((output / "qr-opencv-roundtrip.png").string(), image), "QR fixture write failed");
        cv::QRCodeDetector reader;
        const auto decoded = reader.detectAndDecode(image, corners);
        report << "expectedText" << expected << "decodedText" << decoded << "cornerCount" << static_cast<int>(corners.total());
        require(decoded == expected && corners.total() == 4, "QR decode unavailable or payload/corners mismatch");
        const cv::Mat blank(image.size(), CV_8UC1, cv::Scalar(255));
        require(reader.detectAndDecode(blank).empty(), "blank image unexpectedly decoded");
    });
    report << "]" << "failures" << failures;
    report.release();
    return failures == 0 ? 0 : 2;
}
