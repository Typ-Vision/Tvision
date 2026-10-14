#include "Preprocessing.hpp"
#include "domain/PreprocessConfig.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
namespace visionflow {
cv::Mat Preprocessing::gray8(const ImageFrame& frame,const QString& channel) {
    if(frame.size().isEmpty() || qint64(frame.size().width())*frame.size().height()>16*1024*1024)throw std::runtime_error("Image limit: 16 Mi pixels");
    auto image=ImageAccess(frame).ownedCopy();cv::Mat gray;
    if(image.channels()==1) {
        if(channel!="gray")throw std::runtime_error("Requested colour channel is absent");
        if(image.depth()==CV_16U)image.convertTo(gray,CV_8U,255.0/65535.0);else gray=image;
    } else {
        const bool rgb=frame.format()==PixelFormat::Rgb8 || frame.format()==PixelFormat::Rgba8;
        if(channel=="gray")cv::cvtColor(image,gray,image.channels()==3 ? (rgb ? cv::COLOR_RGB2GRAY : cv::COLOR_BGR2GRAY) : (rgb ? cv::COLOR_RGBA2GRAY : cv::COLOR_BGRA2GRAY));
        else {
            const int index=channel=="red" ? (rgb ? 0:2) : channel=="green" ? 1 : channel=="blue" ? (rgb ? 2:0) : channel=="alpha" ? 3 : -1;
            if(index<0 || index>=image.channels())throw std::runtime_error("Requested colour channel is absent");
            cv::extractChannel(image,gray,index);
        }
    }return gray;
}
ImageFrame Preprocessing::run(const ImageFrame& input,const QJsonObject& parameters,const std::shared_ptr<std::atomic_bool>& cancel) {
    validatePreprocess(parameters);
    auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Preprocess cancelled");};stop();
    auto image=gray8(input,parameters["channel"].toString());
    for(const auto& v:parameters["steps"].toArray()) {
        stop();const auto s=v.toObject();const auto op=s["op"].toString();const int k=s["size"].toInt();const double value=s["value"].toDouble();cv::Mat out;
        if(op=="gaussian")cv::GaussianBlur(image,out,{k,k},value,value,cv::BORDER_REPLICATE);
        else if(op=="median")cv::medianBlur(image,out,k);
        else if(op=="threshold")cv::threshold(image,out,value,255,cv::THRESH_BINARY);
        else if(op.startsWith("adaptive"))cv::adaptiveThreshold(image,out,255,op=="adaptive_mean" ? cv::ADAPTIVE_THRESH_MEAN_C : cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY,k,value);
        else {const int method=op=="erode" ? cv::MORPH_ERODE : op=="dilate" ? cv::MORPH_DILATE : op=="open" ? cv::MORPH_OPEN : cv::MORPH_CLOSE;
            cv::morphologyEx(image,out,method,cv::getStructuringElement(cv::MORPH_RECT,{k,k}),{-1,-1},1,cv::BORDER_REPLICATE);}
        image=std::move(out);
    }stop();
    return ImageAccess::processed(input,image,PixelFormat::Gray8,"preprocess",1,QCryptographicHash::hash(QJsonDocument(parameters).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256));
}
}
