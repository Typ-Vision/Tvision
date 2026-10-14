#pragma once
#include "ImageAccess.hpp"
#include <QJsonObject>
#include <atomic>
#include <memory>
namespace visionflow {
class Preprocessing {
public:
    static cv::Mat gray8(const ImageFrame&,const QString& channel="gray");
    static ImageFrame run(const ImageFrame&,const QJsonObject&,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
