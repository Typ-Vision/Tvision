#pragma once
#include "ImageGeometry.hpp"
#include <QImage>
#include <QJsonObject>
#include <QDateTime>
#include <QString>
#include <opencv2/core.hpp>
#include <optional>
#include <cstddef>
#include <QVector>
namespace visionflow {
struct ProcessingStep {
    QByteArray inputContentId,parametersId;
    QString method;
    int version=0;
};
enum class PixelFormat { Gray8, Gray16, Bgr8, Bgra8, Rgb8, Rgba8 };
struct FrameMetadata {
    QString sourceId,frameId,cycleId,coordinateSystemId;
    QDateTime completedAtUtc;
    std::optional<QDateTime> exposureAtUtc;
    std::optional<quint64> deviceTimestampTicks;
    QString deviceClockId;
    // Experimental camera provenance; never trusted trigger/terminal evidence.
    QJsonObject experimentalAcquisition;
};
class ImageFrame {
public:
    // Copies valid rows into owned storage. No SDK buffer/view escapes this boundary.
    static ImageFrame fromBuffer(const void* data,std::size_t bytes,int width,int height,
                                 std::size_t stride,PixelFormat format,const FrameMetadata& metadata={},bool preview=true);
    ImageFrame withMetadata(const FrameMetadata& metadata) const;
    const FrameMetadata& metadata() const {return metadata_;}
    const ImageGeometry& geometry() const {return geometry_;}
    const QImage& preview() const {return preview_;}
    QSize size() const {return {pixels_.cols,pixels_.rows};}
    std::size_t stride() const {return pixels_.step;}
    PixelFormat format() const {return pixelFormat_;}
    const QString& path() const {return path_;}
    const QString& pixelFormat() const {return format_;}
    QByteArray contentId() const;
    QString pixelText(int x,int y) const;
    const QVector<ProcessingStep>& processing() const {return processing_;}
    QByteArray processingId() const;
private:
    friend class ImageAccess;
    friend class ImageFileService;
    friend class MatchService;
    friend class ToolExecutors;
    friend class ChessboardService;
    QImage preview_;
    cv::Mat pixels_;
    QString path_,format_;
    PixelFormat pixelFormat_=PixelFormat::Gray8;
    // Computed before publication; copies and worker/UI readers share immutable identity.
    QByteArray contentId_;
    FrameMetadata metadata_;
    ImageGeometry geometry_;
    QVector<ProcessingStep> processing_;
};
}
