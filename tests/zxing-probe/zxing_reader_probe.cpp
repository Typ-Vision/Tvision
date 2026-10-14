#include <ZXing/ReadBarcode.h>
#include <ZXing/Barcode.h>
#include <ZXing/ReaderOptions.h>
#include <ZXing/Version.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifndef ZXING_READERS
#error Reader support required
#endif
#ifdef ZXING_WRITERS
#error Writer support must be disabled
#endif
static_assert(sizeof(void*) == 8, "x64 probe required");
static_assert(ZXING_VERSION_MAJOR == 2 && ZXING_VERSION_MINOR == 3 && ZXING_VERSION_PATCH == 0);

namespace {
void require(bool value, const std::string& message)
{
    if (!value) throw std::runtime_error(message);
}
ZXing::Barcodes read(const cv::Mat& image, ZXing::BarcodeFormat format)
{
    return ZXing::ReadBarcodes(ZXing::ImageView(image.data, image.cols, image.rows,
        ZXing::ImageFormat::Lum, static_cast<int>(image.step)),
        ZXing::ReaderOptions().setFormats(format).setTextMode(ZXing::TextMode::Plain)
            .setMaxNumberOfSymbols(8));
}
void check(const cv::Mat& image, ZXing::BarcodeFormat format, const std::string& payload)
{
    const auto results = read(image, format);
    require(results.size() == 1 && results.front().isValid(), "expected one valid code");
    const auto& result = results.front();
    require(result.format() == format, "wrong symbology");
    require(result.text() == payload, "wrong text: " + result.text());
    require(std::string(result.bytes().begin(), result.bytes().end()) == payload, "wrong raw bytes");
    for (const auto& point : result.position())
        require(point.x >= 0 && point.y >= 0 && point.x < image.cols && point.y < image.rows,
            "corner outside image");
    // A linear code may report collinear corners; only 2D needs polygon area.
    if (format == ZXing::BarcodeFormat::QRCode || format == ZXing::BarcodeFormat::DataMatrix) {
        const auto& p = result.position();
        long long twiceArea = 0;
        for (int i = 0; i < 4; ++i)
            twiceArea += static_cast<long long>(p[i].x) * p[(i + 1) % 4].y -
                static_cast<long long>(p[i].y) * p[(i + 1) % 4].x;
        require(twiceArea != 0, "degenerate 2D corners");
    }
}
}
int main(int argc, char** argv)
{
    if (argc != 4) return 2;
    std::ofstream report(argv[3]);
    if (!report) return 2;
    int failures = 0, checks = 0, fixtures = 0;
    report << "{\"backend\":\"ZXing\",\"version\":\"" << ZXING_VERSION_STR
           << "\",\"writer\":false,\"fixtures\":[";
    try {
        std::ifstream definitions(argv[1]);
        require(static_cast<bool>(definitions), "cannot read definitions");
        std::string line;
        while (std::getline(definitions, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::istringstream fields(line);
            std::string id, symbology, formatName, payload;
            require(static_cast<bool>(std::getline(fields, id, '\t')) &&
                static_cast<bool>(std::getline(fields, symbology, '\t')) &&
                static_cast<bool>(std::getline(fields, formatName, '\t')) &&
                static_cast<bool>(std::getline(fields, payload)), "invalid definition");
            const auto format = ZXing::BarcodeFormatFromString(formatName);
            require(format != ZXing::BarcodeFormat::None, "unknown fixture format");
            const auto image = cv::imread((std::filesystem::path(argv[2]) / (id + ".pgm")).string(), cv::IMREAD_GRAYSCALE);
            require(!image.empty(), "missing fixture " + id);
            if (fixtures++) report << ',';
            int fixtureFailures = 0;
            for (int test = 0; test < 4; ++test) {
                ++checks;
                try {
                    if (test == 0) check(image, format, payload);
                    if (test == 1) {
                        cv::Mat rotated;
                        cv::rotate(image, rotated, cv::ROTATE_90_CLOCKWISE);
                        check(rotated, format, payload);
                    }
                    if (test == 2) {
                        // Non-contiguous row stride, unchanged source content.
                        cv::Mat padded(image.rows, image.cols + 31, CV_8U, cv::Scalar(255));
                        auto view = padded(cv::Rect(7, 0, image.cols, image.rows));
                        image.copyTo(view);
                        check(view, format, payload);
                    }
                    if (test == 3) {
                        const auto excluded = format == ZXing::BarcodeFormat::QRCode ?
                            ZXing::BarcodeFormat::Code39 : ZXing::BarcodeFormat::QRCode;
                        require(read(image, excluded).empty(), "excluded format returned a code");
                    }
                } catch (const std::exception& error) {
                    ++failures;
                    ++fixtureFailures;
                    std::cerr << id << " test " << test << ": " << error.what() << '\n';
                }
            }
            report << "{\"id\":\"" << id << "\",\"format\":\"" << formatName
                   << "\",\"checks\":4,\"failures\":" << fixtureFailures << '}';
        }
        require(fixtures == 8, "expected eight independent fixtures");
        ++checks;
        require(read(cv::Mat(320, 640, CV_8U, cv::Scalar(255)), ZXing::BarcodeFormat::Any).empty(),
            "blank must be no-read");
    } catch (const std::exception& error) {
        ++failures;
        std::cerr << error.what() << '\n';
    }
    report << "],\"checks\":" << checks << ",\"failures\":" << failures << "}\n";
    if (!report) return 2;
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
