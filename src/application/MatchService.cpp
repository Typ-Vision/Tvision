#include "MatchService.hpp"
#include "vision/matching/TemplateMatcher.hpp"
#include <opencv2/imgcodecs.hpp>
#include <QElapsedTimer>
#include <QBuffer>
#include <QImageReader>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool value,const char* message) {if (!value) throw std::runtime_error(message);}
void textured(const cv::Mat& model) {
    cv::Scalar mean,deviation; cv::meanStdDev(model,mean,deviation);
    require(deviation[0]>0.01,"Model sabit renkli; ayırt edici doku içeren ROI seçin.");
}
}
QRect MatchService::rectangle(const QJsonObject& o) {return o.isEmpty() ? QRect{} : QRect(o["x"].toInt(),o["y"].toInt(),o["width"].toInt(),o["height"].toInt());}
QJsonObject MatchService::rectangle(const QRect& r) {return r.isEmpty() ? QJsonObject{} : QJsonObject{{"x",r.x()},{"y",r.y()},{"width",r.width()},{"height",r.height()}};}
cv::Mat MatchService::gray(const ImageFrame& frame) const {
    require(!frame.pixels_.empty(),"Görüntü yok."); cv::Mat output;
    if (frame.pixels_.channels()==3) cv::cvtColor(frame.pixels_,output,cv::COLOR_BGR2GRAY);
    else if (frame.pixels_.channels()==4) cv::cvtColor(frame.pixels_,output,cv::COLOR_BGRA2GRAY);
    else if (frame.pixels_.depth()==CV_16U) frame.pixels_.convertTo(output,CV_8U,255.0/65535.0);
    else output=frame.pixels_;
    return output;
}
QString MatchService::teach(const ImageFrame& frame,const QRect& roi) const {
    require(!roi.isEmpty() && QRect(QPoint(0,0),frame.size()).contains(roi),"Model ROI görüntü sınırları dışında.");
    require(roi.width()<=1024 && roi.height()<=1024,"Model ROI en fazla 1024 × 1024 px olabilir.");
    auto image=gray(frame); auto model=image(cv::Rect(roi.x(),roi.y(),roi.width(),roi.height())); textured(model);
    std::vector<uchar> bytes; require(cv::imencode(".png",model,bytes),"Model kodlanamadı.");
    return QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(bytes.data()),static_cast<qsizetype>(bytes.size())).toBase64());
}
MatchParameters MatchService::parameters(const QJsonObject& p) {
    return {rectangle(p["modelRoi"].toObject()),rectangle(p["searchRoi"].toObject()),p["modelPng"].toString(),p["minScore"].toDouble(-1)};
}
MatchOutcome MatchService::run(const ImageFrame& frame,const QJsonObject& p) const {return run(frame,parameters(p));}
namespace {
cv::Mat decodeModel(const MatchParameters& p) {
    const auto encoded=p.modelPng.toLatin1();
    require(!encoded.isEmpty() && encoded.size()<=4*1024*1024,"Önce model öğretin.");
    const auto decoded=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
    require(bool(decoded) && !decoded.decoded.isEmpty(),"Model base64 geçersiz.");
    // Validate dimensions before OpenCV decoding an embedded model.
    QBuffer buffer; buffer.setData(decoded.decoded); buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer,"PNG"); const auto size=reader.size();
    require(size.isValid() && size.width()<=1024 && size.height()<=1024,"Model PNG geçersiz veya çok büyük.");
    std::vector<uchar> bytes(decoded.decoded.begin(),decoded.decoded.end());
    const auto model=cv::imdecode(bytes,cv::IMREAD_UNCHANGED);
    require(model.type()==CV_8UC1 && !model.empty(),"Model Gray8 PNG olmalı."); textured(model);
    const auto taught=p.modelRoi;
    require(taught.size()==QSize(model.cols,model.rows),"Model ROI ve PNG boyutu uyuşmuyor.");
    return model;
}
}
QSize MatchService::validateModel(const MatchParameters& p) {
    const auto model=decodeModel(p);return {model.cols,model.rows};
}
MatchOutcome MatchService::run(const ImageFrame& frame,const MatchParameters& p) const {
    QElapsedTimer timer; timer.start();
    const auto model=decodeModel(p);
    const auto roi=p.searchRoi.isEmpty() ? QRect(QPoint(0,0),frame.size()) : p.searchRoi;
    require(QRect(QPoint(0,0),frame.size()).contains(roi) && roi.width()>=model.cols && roi.height()>=model.rows,"Search ROI geçersiz veya modelden küçük.");
    const auto score=p.minScore; require(score>=0 && score<=1,"Minimum skor geçersiz.");
    auto image=gray(frame); cv::Mat scratch;
    const auto result=matchTemplate(image,model,{roi.x(),roi.y(),roi.width(),roi.height()},scratch);
    return {{result.topLeft.x,result.topLeft.y,model.cols,model.rows},result.rawScore,timer.nsecsElapsed()/1e6,result.rawScore>=score};
}
}
