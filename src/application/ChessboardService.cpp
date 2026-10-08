#include "ChessboardService.hpp"
#include "ImageFileService.hpp"
#include <QCryptographicHash>
#include <QUuid>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <stdexcept>
namespace visionflow {
CalibrationTestImage ChessboardService::importImage(const QString& path) {
    auto frame=ImageFileService{}.load(path);
    // Hash the owned decoded pixels, not a second read of a mutable file.
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(frame.pixelFormat().toUtf8());
    hash.addData(QByteArray::number(frame.size().width())+"x"+QByteArray::number(frame.size().height()));
    for(int y=0;y<frame.pixels_.rows;++y)
        hash.addData(QByteArrayView(reinterpret_cast<const char*>(frame.pixels_.ptr(y)),frame.pixels_.cols*frame.pixels_.elemSize()));
    const auto content=QString::fromLatin1(hash.result().toHex());
    FrameMetadata metadata;metadata.sourceId="calibration-test:"+content;
    metadata.frameId=QUuid::createUuid().toString(QUuid::WithoutBraces);
    metadata.cycleId=metadata.frameId;metadata.coordinateSystemId="image";metadata.completedAtUtc=QDateTime::currentDateTimeUtc();
    return {frame.withMetadata(metadata),content};
}
ChessboardObservation ChessboardService::detect(const ImageFrame& frame,const ChessboardTarget& target) {
    if(target.id.trimmed().isEmpty() || target.id.size()>128 || target.columns<3 || target.rows<3 ||
       target.columns>32 || target.rows>32 || target.columns*target.rows>512 ||
       !std::isfinite(target.squareMillimetres) || target.squareMillimetres<=0 || target.squareMillimetres>1000000)
        throw std::runtime_error("Target ID, 3–32 iç köşe (en çok 512) ve pozitif kare kenarı mm gerekli.");
    if(frame.size().isEmpty() || frame.size().width()*qint64(frame.size().height())>16000000 ||
       frame.metadata().frameId.isEmpty() || frame.metadata().sourceId.isEmpty() || (frame.metadata().coordinateSystemId!="image" && frame.metadata().coordinateSystemId!=frame.metadata().sourceId+"/image"))
        throw std::runtime_error("Raw image kökenli kare gerekli; köşe tespiti sınırı 16 MP.");
    cv::Mat gray;
    switch(frame.format()) {
    case PixelFormat::Gray8:gray=frame.pixels_;break;
    case PixelFormat::Gray16:frame.pixels_.convertTo(gray,CV_8U,1.0/257.0);break;
    case PixelFormat::Bgr8:cv::cvtColor(frame.pixels_,gray,cv::COLOR_BGR2GRAY);break;
    case PixelFormat::Bgra8:cv::cvtColor(frame.pixels_,gray,cv::COLOR_BGRA2GRAY);break;
    case PixelFormat::Rgb8:cv::cvtColor(frame.pixels_,gray,cv::COLOR_RGB2GRAY);break;
    case PixelFormat::Rgba8:cv::cvtColor(frame.pixels_,gray,cv::COLOR_RGBA2GRAY);break;
    }
    std::vector<cv::Point2f> corners;
    if(!cv::findChessboardCornersSB(gray,{target.columns,target.rows},corners,cv::CALIB_CB_NORMALIZE_IMAGE) ||
       corners.size()!=static_cast<size_t>(target.columns*target.rows))
        throw std::runtime_error("Chessboard bulunamadı. İç köşe sayısını ve görüntüyü kontrol edin.");
    ChessboardObservation observation{target,frame.metadata(),frame.size(),{}, {}};
    for(int i=0;i<int(corners.size());++i) {
        const auto p=corners[i];
        if(!std::isfinite(p.x) || !std::isfinite(p.y) || p.x<0 || p.y<0 || p.x>=gray.cols || p.y>=gray.rows)
            throw std::runtime_error("Geçersiz köşe koordinatı.");
        observation.pixels.append({p.x,p.y});
        observation.worldMillimetres.append({(i%target.columns)*target.squareMillimetres,(i/target.columns)*target.squareMillimetres});
    }
    return observation;
}
}
