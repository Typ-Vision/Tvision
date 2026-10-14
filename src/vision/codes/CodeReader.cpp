#include "CodeReader.hpp"
#include "vision/Preprocessing.hpp"
#include "vision/RectangleMask.hpp"
#include "domain/CodeReadConfig.hpp"
#include "application/BackendCapabilities.hpp"
#include <QElapsedTimer>
#include <QSet>
#include <algorithm>
#include <stdexcept>
#ifdef VISIONFLOW_WITH_ZXING
#include <ZXing/ReadBarcode.h>
#include <ZXing/Barcode.h>
#include <ZXing/ReaderOptions.h>
#endif
namespace visionflow {
CodeResults CodeReader::read(const ImageFrame& frame,const CodeReaderOptions& options,const std::shared_ptr<std::atomic_bool>& cancel) {
    QElapsedTimer timer;timer.start();
    auto check=[&]{if(cancel && cancel->load())throw std::runtime_error("Code read cancelled");if(timer.elapsed()>=options.budgetMs)throw std::runtime_error("Code read budget exceeded; result discarded");};
    if(options.maxCount<1 || options.maxCount>128 || options.budgetMs<1 || options.budgetMs>60000 || options.formats.isEmpty())throw std::runtime_error("Invalid reader options");
    QSet<QString> seen;for(const auto& f:options.formats){if(!codeFormats().contains(f) || seen.contains(f))throw std::runtime_error("Invalid/duplicate code format");seen.insert(f);}
    auto roi=options.roi;if(roi==QRect{})roi=QRect(QPoint(0,0),frame.size());
    std::optional<RectangleMask> mask;
    if(options.trackingRoi){
        if(options.roi!=QRect{})throw std::runtime_error("Conflicting code ROI modes");
        if(options.trackingRoi->frame.id!=frame.metadata().coordinateSystemId)throw std::runtime_error("Code tracking frame mismatch");
        mask.emplace(*options.trackingRoi,frame.size(),cancel);roi=mask->bounds;
    }
    if(roi.isEmpty() || !QRect(QPoint(0,0),frame.size()).contains(roi))throw std::runtime_error("Code ROI outside raw image");
    check();if(!BackendCapabilities::get("zxing-reader").available)throw std::runtime_error("ZXing reader unavailable");
#ifdef VISIONFLOW_WITH_ZXING
    auto gray=Preprocessing::gray8(frame);auto crop=gray(cv::Rect(roi.x(),roi.y(),roi.width(),roi.height()));
    if(mask){crop=crop.clone();crop.setTo(255,mask->pixels==0);}
    ZXing::BarcodeFormats formats;for(const auto& f:options.formats)formats|=ZXing::BarcodeFormatFromString(f.toStdString());
    check();
    auto decoded=ZXing::ReadBarcodes(ZXing::ImageView(crop.data,crop.cols,crop.rows,ZXing::ImageFormat::Lum,int(crop.step)),ZXing::ReaderOptions().setFormats(formats).setTextMode(ZXing::TextMode::Plain).setTryRotate(true).setTryInvert(true).setMaxNumberOfSymbols(options.maxCount+1));
    check();
    // ZXing inversion is format-dependent. Retry a fully inverted raster on no-read,
    // including linear formats. Pixel geometry and ROI offset remain unchanged.
    if(decoded.empty()) {
        cv::Mat inverted;cv::bitwise_not(crop,inverted);check();
        decoded=ZXing::ReadBarcodes(ZXing::ImageView(inverted.data,inverted.cols,inverted.rows,ZXing::ImageFormat::Lum,int(inverted.step)),ZXing::ReaderOptions().setFormats(formats).setTextMode(ZXing::TextMode::Plain).setTryRotate(true).setTryInvert(true).setMaxNumberOfSymbols(options.maxCount+1));
    }
    check();CodeResults result;
    for(const auto& code:decoded)if(code.isValid()) {
        // A partially masked barcode is never accepted merely because error correction decoded it.
        bool contained=true;for(const auto& corner:code.position())if(mask && !mask->contains({double(corner.x+roi.x()),double(corner.y+roi.y())}))contained=false;
        if(!contained)continue;
        if(result.items.size()>=options.maxCount)throw std::runtime_error("Code count limit exceeded; no truncated result");
        const auto text=code.text();const auto& bytes=code.bytes();
        if(text.size()>65536 || bytes.size()>65536)throw std::runtime_error("Code payload limit exceeded");
        CodeResult item;item.text=QString::fromUtf8(text.data(),qsizetype(text.size()));item.bytes=QByteArray(reinterpret_cast<const char*>(bytes.data()),qsizetype(bytes.size()));item.symbology=QString::fromStdString(ZXing::ToString(code.format())).remove('-');if(item.symbology=="QRCode")item.symbology="QR";
        for(const auto& corner:code.position()){QPointF p(corner.x+roi.x(),corner.y+roi.y());if(p.x()<roi.left() || p.y()<roi.top() || p.x()>roi.right() || p.y()>roi.bottom())throw std::runtime_error("Reader corner outside ROI");item.corners.append(p);}result.items.append(std::move(item));
    }
    // Preserve physical duplicates; stable spatial order is the selection contract.
    std::stable_sort(result.items.begin(),result.items.end(),[](const auto& a,const auto& b){const auto pa=a.corners.first(),pb=b.corners.first();if(pa.y()!=pb.y())return pa.y()<pb.y();if(pa.x()!=pb.x())return pa.x()<pb.x();if(a.symbology!=b.symbology)return a.symbology<b.symbology;return a.bytes<b.bytes;});
    ToolValues::encode(result);check();return result;
#else
    throw std::runtime_error("ZXing reader unavailable");
#endif
}
}
