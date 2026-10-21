#include "ImageFileService.hpp"
#include "domain/MeasurementSetupContract.hpp"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
namespace visionflow {
ImageFrame ImageFileService::load(const QString& path,bool experimentalEditorReplay) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(("Görüntü açılamadı: "+file.errorString()).toUtf8().constData());
    if (file.size()<=0 || file.size()>256*1024*1024) throw std::runtime_error("Görüntü dosyası boş veya 256 MB sınırını aşıyor.");
    const auto bytes=file.readAll();
    const bool marked=bytes.startsWith("P5\n# VisionFlow-OECS-Experimental-Unverified\n");
    QJsonObject evidence;
    // A sidecar without its in-band marker is also rejected: removing the marker
    // must not turn a known experimental capture into an ordinary file source.
    if(marked || QFileInfo::exists(path+".json")) {
        QFile sidecar(path+".json");
        if(marked && !experimentalEditorReplay)throw std::runtime_error("Deneysel dosya için Editor deneysel replay seçimini açın.");
        const bool readable=sidecar.open(QIODevice::ReadOnly) && sidecar.size()<=1024*1024;
        if(marked && !readable)throw std::runtime_error("Deneysel raster kanıtı eksik veya geçersiz.");
        QJsonParseError error;const auto doc=QJsonDocument::fromJson(readable ? sidecar.readAll():QByteArray{},&error);
        if(marked && (error.error!=QJsonParseError::NoError || !doc.isObject()))throw std::runtime_error("Raster JSON kanıtı geçersiz.");
        const auto obj=doc.object();
        if(marked || obj["schema"]=="visionflow.oecs-experimental1") {
            if(!marked || !experimentalEditorReplay || obj["schema"]!="visionflow.oecs-experimental1" || obj["policy"]!="UnverifiedNextArrival" ||
               obj["triggerAssociationVerified"]!=false || obj["productionAdmission"]!=false ||
               obj["vendor"]!="Opto Engineering Srl" || obj["model"]!="COE-050-M-USB-050-IR-C" || obj["serial"].toString().trimmed().isEmpty() ||
               obj["width"]!=2448 || obj["height"]!=2048 || obj["originX"]!=0 || obj["originY"]!=0 ||
               obj["pixelFormat"]!="Mono8" || obj["embeddedMetadataEnabled"]!=false || obj["readback"].toObject().isEmpty() ||
               obj["profileSnapshot"].toObject().isEmpty() || obj["acquisitionId"].toString().isEmpty())
                throw std::runtime_error("Deneysel raster kökeni/geometrisi geçersiz.");
            const auto actual=AcquisitionReadback::fromJson(obj["readback"].toObject());
            const auto profile=AcquisitionProfile::fromJson(obj["profileSnapshot"].toObject());
            const auto geometry=actual.geometry.descriptor();auto requested=profile.requested;
            if(actual.settings.exposureTimeUs==double(float(requested.exposureTimeUs)))requested.exposureTimeUs=actual.settings.exposureTimeUs;
            if(actual.settings.gain==double(float(requested.gain)))requested.gain=actual.settings.gain;
            if(geometry.resolution!=QSize(2448,2048) || actual.geometry.offset!=QPoint(0,0) ||
               actual.geometry.binning!=QSize(1,1) || actual.geometry.decimation!=QSize(1,1) || actual.geometry.reverse!=GeometryMirror{} ||
               actual.settings!=requested || actual.settings.trigger!=TriggerSettings{true,"FrameBurstStart","Software"} ||
               profile.id.value!=obj["profileId"].toString() || double(profile.revision)!=obj["profileRevision"].toDouble())
                throw std::runtime_error("Deneysel profil/readback kanıtı eşleşmedi.");
            evidence=obj;
        }
    }
    if (file.error()!=QFileDevice::NoError) throw std::runtime_error("Görüntü dosyası okunamadı.");
    ImageFrame frame;
    try {
        const cv::Mat encoded(1,static_cast<int>(bytes.size()),CV_8UC1,const_cast<char*>(bytes.constData()));
        frame.pixels_=cv::imdecode(encoded,cv::IMREAD_UNCHANGED);
        if (frame.pixels_.empty()) throw std::runtime_error("Görüntü formatı tanınmadı veya dosya bozuk.");
        const auto& pixels=frame.pixels_;
        if (pixels.total()>100000000) throw std::runtime_error("Görüntü 100 MP sınırını aşıyor.");
        PixelFormat format;
        if(pixels.type()==CV_8UC1)format=PixelFormat::Gray8;
        else if(pixels.type()==CV_16UC1)format=PixelFormat::Gray16;
        else if(pixels.type()==CV_8UC3)format=PixelFormat::Bgr8;
        else if(pixels.type()==CV_8UC4)format=PixelFormat::Bgra8;
        else throw std::runtime_error("Bu piksel formatı desteklenmiyor. Gray8/Gray16/RGB8/RGBA8 kullanın.");
        frame=ImageFrame::fromBuffer(pixels.data,(pixels.rows-1)*pixels.step+pixels.cols*pixels.elemSize(),pixels.cols,pixels.rows,pixels.step,format);
    } catch (const cv::Exception& e) { throw std::runtime_error(("Görüntü decode hatası: "+QString::fromUtf8(e.what())).toUtf8().constData()); }
    if(!evidence.isEmpty()) {
        if(frame.format()!=PixelFormat::Gray8 || frame.size()!=QSize(2448,2048) ||
           evidence["contentId"].toString()!=QString::fromLatin1(frame.contentId().toHex()))
            throw std::runtime_error("Deneysel raster piksel hash/geometri eşleşmedi.");
        frame.metadata_.experimentalAcquisition=evidence;
    }
    frame.path_=QFileInfo(path).absoluteFilePath(); return frame;
}
}
