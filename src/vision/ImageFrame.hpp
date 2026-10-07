#pragma once
#include <QImage>
#include <QDateTime>
#include <QString>
#include <opencv2/core.hpp>
#include <optional>
#include <cstddef>
namespace visionflow {
enum class PixelFormat { Gray8, Gray16, Bgr8, Bgra8, Rgb8, Rgba8 };
struct FrameMetadata {
    QString sourceId,frameId,cycleId,coordinateSystemId;
    QDateTime completedAtUtc;
    std::optional<QDateTime> exposureAtUtc;
    std::optional<quint64> deviceTimestampTicks;
    QString deviceClockId;
};
class ImageFrame {
public:
    // Copies valid rows into owned storage. No SDK buffer/view escapes this boundary.
    static ImageFrame fromBuffer(const void* data,std::size_t bytes,int width,int height,
                                 std::size_t stride,PixelFormat format,const FrameMetadata& metadata={},bool preview=true);
    ImageFrame withMetadata(const FrameMetadata& metadata) const;
    const FrameMetadata& metadata() const {return metadata_;}
    const QImage& preview() const {return preview_;}
    QSize size() const {return {pixels_.cols,pixels_.rows};}
    std::size_t stride() const {return pixels_.step;}
    PixelFormat format() const {return pixelFormat_;}
    const QString& path() const {return path_;}
    const QString& pixelFormat() const {return format_;}
    QString pixelText(int x,int y) const;
private:
    friend class ImageFileService;
    friend class MatchService;
    friend class ToolExecutors;
    QImage preview_;
    cv::Mat pixels_;
    QString path_,format_;
    PixelFormat pixelFormat_=PixelFormat::Gray8;
    FrameMetadata metadata_;
};
}
