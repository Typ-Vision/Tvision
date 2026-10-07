#include "ToolExecutors.hpp"
#include "domain/ToolRegistry.hpp"
#include "domain/Coordinates.hpp"
#include "MatchService.hpp"
#include "AcquisitionService.hpp"
#include <QDir>
#include <QElapsedTimer>
#include <QUuid>
#include <QSet>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <QMutex>
#include <QMutexLocker>
namespace visionflow {
namespace {
struct BlobParameters {int threshold;};
struct EdgeParameters {bool horizontal,reverse; double contrast;};
BlobParameters blobParameters(const ToolDefinition& t) {ToolRegistry::validate(t);return {t.parameters["threshold"].toInt()};}
EdgeParameters edgeParameters(const ToolDefinition& t) {
    ToolRegistry::validate(t);const auto d=t.parameters["direction"].toString();
    return {d=="left_to_right" || d=="right_to_left",d=="right_to_left" || d=="bottom_to_top",t.parameters["minContrast"].toDouble()};
}
}

QRect searchRegion(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const ImageFrame& frame) {
    const auto parameters=t.parameters;
    QRect region=MatchService::rectangle(parameters["searchRoi"].toObject());
    if(inputs.contains("region"))region=std::get<QRect>(inputs["region"].data);
    if(inputs.contains("reference")) {
        const auto origin=parameters["referenceOrigin"].toObject();
        if(region.isEmpty() || origin.isEmpty())throw std::runtime_error("Referansa bağlı arama için Search ROI çizip referans ilişkisini öğretin.");
        const auto current=std::get<QPointF>(inputs["reference"].data);
        const auto& reference=inputs["reference"];
        const auto& image=inputs["image"];
        if(reference.port.type!=PortType::Point || reference.port.unit!="px" || reference.port.frame!="image" ||
           reference.origin.coordinateSystemId!=image.origin.coordinateSystemId ||
           reference.origin.frameId!=image.origin.frameId || reference.origin.cycleId!=image.origin.cycleId ||
           reference.origin.sourceId!=image.origin.sourceId)
            throw std::runtime_error("Tracking ROI referans frame/birim uyumsuz.");
        const CoordinateFrame coordinates{frame.metadata().coordinateSystemId,LengthUnit::Pixel,AxisConvention::XRightYDown};
        region=translationTrackingRoi(region,{{origin["x"].toDouble(),origin["y"].toDouble()},coordinates},{current,coordinates},frame.size());
    }
    if(region.isEmpty())region=QRect(QPoint(0,0),frame.size());
    if(!QRect(QPoint(0,0),frame.size()).contains(region))throw std::runtime_error("ROI görüntü sınırı dışında.");
    return region;
}
ToolResult ToolExecutors::imageFile(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel) {
    ToolResult r; r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](const QString& id,PortData data,bool valid=true) {for (const auto& p:outputPorts(t.kind)) if(p.id==id) {
        FrameMetadata origin;
        if(auto* image=std::get_if<ImageFrame>(&data))origin=image->metadata();
        else if(inputs.contains("image"))origin=inputs.value("image").origin;
        else if(!inputs.isEmpty())origin=inputs.first().origin;
        r.outputs[id]={p,std::move(data),valid,frameId,cycleId,origin};
    }};
        const auto path=t.parameters["path"].toString();if(path.isEmpty())throw std::runtime_error("Görüntü yolu boş.");
        const ImageFileAdapter adapter(QDir(directory).absoluteFilePath(path));
        const auto acquired=AcquisitionService{}.acquire(adapter,{t.id,cycleId,frameId,TriggerMode::Software,5000,cancel});
        if(acquired.state!=AcquisitionState::Done) {
            if(acquired.state==AcquisitionState::Cancelled) {r.state=ExecutionState::Skipped;r.message=acquired.error;return r;}
            throw std::runtime_error(acquired.error.toUtf8().constData());
        }
        put("image",acquired.frame->image);return r;
}
ToolResult ToolExecutors::calculate(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel) {
    ToolResult r; r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](const QString& id,PortData data,bool valid=true) {for (const auto& p:outputPorts(t.kind)) if(p.id==id) {
        FrameMetadata origin;
        if(auto* image=std::get_if<ImageFrame>(&data))origin=image->metadata();
        else if(inputs.contains("image"))origin=inputs.value("image").origin;
        else if(!inputs.isEmpty())origin=inputs.first().origin;
        r.outputs[id]={p,std::move(data),valid,frameId,cycleId,origin};
    }};
        const auto a=std::get<QPointF>(inputs["p1"].data),b=std::get<QPointF>(inputs["p2"].data);
        put("value",std::hypot(a.x()-b.x(),a.y()-b.y()));r.points={a,b};return r;
}
ToolResult ToolExecutors::match(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel) {
    ToolResult r; r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](const QString& id,PortData data,bool valid=true) {for (const auto& p:outputPorts(t.kind)) if(p.id==id) {
        FrameMetadata origin;
        if(auto* image=std::get_if<ImageFrame>(&data))origin=image->metadata();
        else if(inputs.contains("image"))origin=inputs.value("image").origin;
        else if(!inputs.isEmpty())origin=inputs.first().origin;
        r.outputs[id]={p,std::move(data),valid,frameId,cycleId,origin};
    }};
    const auto frame=std::get<ImageFrame>(inputs["image"].data);
    const auto region=searchRegion(t,inputs,frame);
    r.searchRegion=region;
        auto settings=MatchService::parameters(t.parameters);settings.searchRoi=region;const auto m=MatchService{}.run(frame,settings);
        put("score",m.score);put("found",m.accepted);put("region",m.rectangle,m.accepted);put("point",QPointF(m.rectangle.x()+(m.rectangle.width()-1)/2.0,m.rectangle.y()+(m.rectangle.height()-1)/2.0),m.accepted);
        if(m.accepted)r.regions={m.rectangle};r.message=m.accepted ? "Eşleşme bulundu" : "Eşik altında";return r;
}
ToolResult ToolExecutors::blob(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel) {
    ToolResult r; r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](const QString& id,PortData data,bool valid=true) {for (const auto& p:outputPorts(t.kind)) if(p.id==id) {
        FrameMetadata origin;
        if(auto* image=std::get_if<ImageFrame>(&data))origin=image->metadata();
        else if(inputs.contains("image"))origin=inputs.value("image").origin;
        else if(!inputs.isEmpty())origin=inputs.first().origin;
        r.outputs[id]={p,std::move(data),valid,frameId,cycleId,origin};
    }};
    const auto frame=std::get<ImageFrame>(inputs["image"].data);
    const auto region=searchRegion(t,inputs,frame);
    r.searchRegion=region;
    cv::Mat gray;
    if(frame.pixels_.channels()==1) {if(frame.pixels_.depth()==CV_16U)frame.pixels_.convertTo(gray,CV_8U,255.0/65535.0);else gray=frame.pixels_;}
    else cv::cvtColor(frame.pixels_,gray,frame.pixels_.channels()==3 ? cv::COLOR_BGR2GRAY : cv::COLOR_BGRA2GRAY);
    const cv::Mat crop=gray(cv::Rect(region.x(),region.y(),region.width(),region.height()));
        cv::Mat binary,labels,stats,centers;cv::threshold(crop,binary,blobParameters(t).threshold,255,cv::THRESH_BINARY);
        const int n=cv::connectedComponentsWithStats(binary,labels,stats,centers,8,CV_32S);int largest=0;
        for(int i=1;i<n;++i) {if(!largest || stats.at<int>(i,cv::CC_STAT_AREA)>stats.at<int>(largest,cv::CC_STAT_AREA))largest=i;
            if(r.regions.size()<2048)r.regions.append(QRect(region.x()+stats.at<int>(i,cv::CC_STAT_LEFT),region.y()+stats.at<int>(i,cv::CC_STAT_TOP),stats.at<int>(i,cv::CC_STAT_WIDTH),stats.at<int>(i,cv::CC_STAT_HEIGHT)));}
        put("count",double(n-1));put("found",largest>0);put("area",largest ? double(stats.at<int>(largest,cv::CC_STAT_AREA)) : 0.0);
        if(largest) {auto box=QRect(region.x()+stats.at<int>(largest,cv::CC_STAT_LEFT),region.y()+stats.at<int>(largest,cv::CC_STAT_TOP),stats.at<int>(largest,cv::CC_STAT_WIDTH),stats.at<int>(largest,cv::CC_STAT_HEIGHT));
            auto point=QPointF(region.x()+centers.at<double>(largest,0),region.y()+centers.at<double>(largest,1));put("region",box);put("point",point);r.points={point};}
        else {put("region",QRect{},false);put("point",QPointF{},false);}return r;
}
ToolResult ToolExecutors::edge(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel) {
    ToolResult r; r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](const QString& id,PortData data,bool valid=true) {for (const auto& p:outputPorts(t.kind)) if(p.id==id) {
        FrameMetadata origin;
        if(auto* image=std::get_if<ImageFrame>(&data))origin=image->metadata();
        else if(inputs.contains("image"))origin=inputs.value("image").origin;
        else if(!inputs.isEmpty())origin=inputs.first().origin;
        r.outputs[id]={p,std::move(data),valid,frameId,cycleId,origin};
    }};
    const auto frame=std::get<ImageFrame>(inputs["image"].data);
    const auto region=searchRegion(t,inputs,frame);
    r.searchRegion=region;
    cv::Mat gray;
    if(frame.pixels_.channels()==1) {if(frame.pixels_.depth()==CV_16U)frame.pixels_.convertTo(gray,CV_8U,255.0/65535.0);else gray=frame.pixels_;}
    else cv::cvtColor(frame.pixels_,gray,frame.pixels_.channels()==3 ? cv::COLOR_BGR2GRAY : cv::COLOR_BGRA2GRAY);
    const cv::Mat crop=gray(cv::Rect(region.x(),region.y(),region.width(),region.height()));
    const auto settings=edgeParameters(t);const bool horizontal=settings.horizontal;
    const bool reverse=settings.reverse;cv::Mat profile;cv::reduce(crop,profile,horizontal ? 0 : 1,cv::REDUCE_AVG,CV_64F);
    const int length=horizontal ? crop.cols : crop.rows;double strength=0;int found=-1;
    auto sample=[&](int index){return horizontal ? profile.at<double>(0,index) : profile.at<double>(index,0);};
    for(int k=1;k<length;++k) {int i=reverse ? length-1-k : k;int previous=reverse ? i+1 : i-1;const auto difference=std::abs(sample(i)-sample(previous));if(difference>=settings.contrast) {found=i;strength=difference;break;}}
    put("found",found>=0);put("strength",strength);
    if(found>=0) {const QPointF point(horizontal ? region.x()+found : region.x()+(region.width()-1)/2.0,horizontal ? region.y()+(region.height()-1)/2.0 : region.y()+found);put("point",point);r.points={point};r.lines={horizontal ? QLineF(point.x(),region.top(),point.x(),region.bottom()) : QLineF(region.left(),point.y(),region.right(),point.y())};}
    else put("point",QPointF{},false);return r;
}
namespace { QMutex executorMutex; QMap<QString,ToolExecutor> extensions; }
void ToolExecutors::registerExecutor(const QString& kind,ToolExecutor executor) {
    ToolRegistry::get(kind);
    QMutexLocker lock(&executorMutex);
    if(!executor || extensions.contains(kind) || QStringList{"image_file","template_match","blob","edge","calculate"}.contains(kind))throw std::runtime_error("Geçersiz/tekrar yürütücü kaydı.");
    extensions.insert(kind,std::move(executor));
}
ToolExecutor ToolExecutors::get(const QString& kind) {
    static const QMap<QString,ToolExecutor> executors={{"image_file",imageFile},{"calculate",calculate},{"template_match",match},{"blob",blob},{"edge",edge}};
    ToolRegistry::get(kind);
    QMutexLocker lock(&executorMutex);
    if(extensions.contains(kind))return extensions.value(kind);
    const auto it=executors.constFind(kind);
    if(it==executors.cend())throw std::runtime_error("Tool yürütücüsü kayıtlı değil.");
    return *it;
}
}
