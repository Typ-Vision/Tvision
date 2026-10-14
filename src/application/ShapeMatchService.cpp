#include "ShapeMatchService.hpp"
#include "vision/matching/ShapeMatcher.hpp"
#include "vision/ImageAccess.hpp"
#include <stdexcept>
namespace visionflow {
QJsonObject ShapeMatchService::teach(const ImageFrame& image,const QRect& roi,QPointF anchor,const QJsonObject& p){
    auto taught=PoseMatchService::teach(image,roi,anchor,p);
    ShapeMatcher::validateModel(ImageAccess(image).ownedCopy()(cv::Rect(roi.x(),roi.y(),roi.width(),roi.height())));
    return taught;
}
ToolResult ShapeMatchService::execute(const ToolDefinition& t,const QMap<QString,PortValue>& i,const QString& f,const QString& c,const std::shared_ptr<std::atomic_bool>& cancel){
    if(t.kind!="shape_pose")throw std::runtime_error("Shape service requires shape_pose");
    return PoseMatchService::execute(t,i,f,c,cancel);
}
}
