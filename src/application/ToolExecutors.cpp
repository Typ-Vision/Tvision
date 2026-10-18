#include "PoseMatchService.hpp"
#include "ShapeMatchService.hpp"
#include "CircleToolService.hpp"
#include "CircleToolService.hpp"
#include "ToolExecutors.hpp"
#include "GeometryMeasurementService.hpp"
#include "CodeReadService.hpp"
#include "EdgePointService.hpp"
#include "MetrologyService.hpp"
#include "vision/Preprocessing.hpp"
#include "vision/RectangleMask.hpp"
#include "vision/metrology/ProfileSampler.hpp"
#include <numbers>
#include "domain/ToolRegistry.hpp"
#include "domain/Coordinates.hpp"
#include "MatchService.hpp"
#include "AcquisitionService.hpp"
#include "WebcamAdapter.hpp"
#include "MeasurementService.hpp"
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
struct BlobParameters {int threshold,connectivity,minArea,maxArea;bool dark;};
struct EdgeParameters {bool horizontal,reverse; double contrast;};
BlobParameters blobParameters(const ToolDefinition& t) {ToolRegistry::validate(t);return {t.parameters["threshold"].toInt(),t.parameters["connectivity"].toInt(),t.parameters["minArea"].toInt(),t.parameters["maxArea"].isNull() ? 2147483647 : t.parameters["maxArea"].toInt(),t.parameters["polarity"]=="dark"};}
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
ToolResult ToolExecutors::webcam(const ToolDefinition& t,const QMap<QString,PortValue>&,const QString&,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel) {
    ToolRegistry::validate(t);
    const WebcamAdapter adapter({t.parameters["deviceIndex"].toInt(),t.parameters["backend"].toString()});
    const auto acquired=AcquisitionService{}.acquire(adapter,{t.id,cycleId,frameId,TriggerMode::Software,t.parameters["timeoutMs"].toInt(),cancel});
    ToolResult result;result.toolId=t.id;
    if(acquired.state==AcquisitionState::Cancelled){result.state=ExecutionState::Skipped;result.message=acquired.error;return result;}
    if(acquired.state!=AcquisitionState::Done)throw std::runtime_error(acquired.error.toUtf8().constData());
    result.state=ExecutionState::Done;
    result.outputs["image"]={outputPorts(t.kind).first(),acquired.frame->image,true,frameId,cycleId,acquired.frame->image.metadata()};return result;
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
ToolResult ToolExecutors::blob(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel,const std::optional<ToolRoi>& resolvedRoi) {
    if(t.parameters.contains("poseTracking"))throw std::runtime_error("Pose tracking must be resolved by execution engine");
    if(resolvedRoi && (inputs.contains("region") || inputs.contains("reference")))throw std::runtime_error("Conflicting tracking search modes");
    ToolResult r; r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](const QString& id,PortData data,bool valid=true) {for (const auto& p:outputPorts(t.kind)) if(p.id==id) {
        FrameMetadata origin;
        if(auto* image=std::get_if<ImageFrame>(&data))origin=image->metadata();
        else if(inputs.contains("image"))origin=inputs.value("image").origin;
        else if(!inputs.isEmpty())origin=inputs.first().origin;
        r.outputs[id]={p,std::move(data),valid,frameId,cycleId,origin};
    }};
    const auto frame=std::get<ImageFrame>(inputs["image"].data);
    std::optional<RectangleMask> trackingMask;
    if(resolvedRoi && resolvedRoi->frame.id!=frame.metadata().coordinateSystemId)throw std::runtime_error("Blob tracking frame mismatch");
    if(resolvedRoi)trackingMask.emplace(*resolvedRoi,frame.size(),cancel);
    const auto region=trackingMask ? trackingMask->bounds:searchRegion(t,inputs,frame);
    r.searchRegion=region;
    cv::Mat gray;
    if(frame.pixels_.channels()==1) {if(frame.pixels_.depth()==CV_16U)frame.pixels_.convertTo(gray,CV_8U,255.0/65535.0);else gray=frame.pixels_;}
    else cv::cvtColor(frame.pixels_,gray,frame.pixels_.channels()==3 ? cv::COLOR_BGR2GRAY : cv::COLOR_BGRA2GRAY);
    const cv::Mat crop=gray(cv::Rect(region.x(),region.y(),region.width(),region.height()));
        const auto settings=blobParameters(t);
        // Bound label + mask workspace; never silently truncate the segmentation.
        if(qint64(crop.rows)*crop.cols>16*1024*1024)throw std::runtime_error("Blob ROI sınırı 16 Mi piksel.");
        auto stopped=[&]{return cancel && cancel->load();};
        if(stopped()){r.state=ExecutionState::Skipped;return r;}
        cv::Mat binary,labels,stats,centers;cv::threshold(crop,binary,settings.threshold,255,settings.dark ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY);
        if(trackingMask)cv::bitwise_and(binary,trackingMask->pixels,binary);
        const int n=cv::connectedComponentsWithStats(binary,labels,stats,centers,settings.connectivity,CV_32S);int largest=0,count=0;
        QVector<uchar> accepted(n,0);QPoint firstLargest;
        // First raster pixel makes equal-area selection independent of OpenCV label ordering.
        QVector<QPoint> first(n,QPoint(-1,-1));
        for(int y=0;y<labels.rows;++y){if(stopped()){r.state=ExecutionState::Skipped;return r;}for(int x=0;x<labels.cols;++x){const int label=labels.at<int>(y,x);if(label && first[label].x()<0)first[label]={x,y};}}
        for(int i=1;i<n;++i) {
            if((i%4096)==0 && stopped()){r.regions.clear();r.state=ExecutionState::Skipped;return r;}
            const auto area=stats.at<int>(i,cv::CC_STAT_AREA);if(area<settings.minArea || area>settings.maxArea)continue;
            accepted[i]=1;++count;
            const auto p=first[i];
            if(!largest || area>stats.at<int>(largest,cv::CC_STAT_AREA) || (area==stats.at<int>(largest,cv::CC_STAT_AREA) && (p.y()<firstLargest.y() || (p.y()==firstLargest.y() && p.x()<firstLargest.x())))){largest=i;firstLargest=p;}
            if(r.regions.size()<2048)r.regions.append(QRect(region.x()+stats.at<int>(i,cv::CC_STAT_LEFT),region.y()+stats.at<int>(i,cv::CC_STAT_TOP),stats.at<int>(i,cv::CC_STAT_WIDTH),stats.at<int>(i,cv::CC_STAT_HEIGHT)));
        }
        r.mask.size=region.size();r.mask.origin=region.topLeft();r.mask.pixels=QByteArray(crop.rows*crop.cols,0);
        for(int y=0;y<labels.rows;++y){if(stopped()){r.mask={};r.regions.clear();r.state=ExecutionState::Skipped;return r;}auto* row=r.mask.pixels.data()+y*crop.cols;for(int x=0;x<labels.cols;++x)row[x]=accepted[labels.at<int>(y,x)] ? char(255) : 0;}
        r.message=QString("Aday: %1 • Kabul: %2 • Kırmızı: kabul edilen bölge (parça kararı değildir)").arg(n-1).arg(count);
        if(count>2048)r.message+=" • Bbox gösterimi ilk 2048 ile sınırlı; maske ve sayım tam.";
        put("candidateCount",double(n-1));put("count",double(count));put("found",largest>0);put("area",largest ? double(stats.at<int>(largest,cv::CC_STAT_AREA)) : 0.0);
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
namespace {
ToolResult referenceEdge(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const ToolRoi& roi,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel) {
    ToolRegistry::validate(t);
    if(inputs.contains("region") || inputs.contains("reference") || inputs.contains("poseReference"))throw std::runtime_error("Conflicting reference search modes");
    const auto image=std::get<ImageFrame>(inputs.value("image").data);
    if(roi.frame.id!=image.metadata().coordinateSystemId || !roi.inside(image.size()) || !std::holds_alternative<RotatedRectangle>(roi.geometry))throw std::runtime_error("Reference Edge requires bounded raw rotated rectangle");
    const auto box=std::get<RotatedRectangle>(roi.geometry);const auto settings=edgeParameters(t);
    const double angle=box.angleDegrees*std::numbers::pi/180.;
    const QPointF axis=settings.horizontal ? QPointF(std::cos(angle),std::sin(angle)):QPointF(-std::sin(angle),std::cos(angle));
    const double length=settings.horizontal ? box.size.width():box.size.height(),width=settings.horizontal ? box.size.height():box.size.width();
    const auto delta=axis*(length/2)*(settings.reverse ? -1:1);
    const ToolRoi scan{roi.frame,ScanSegment{box.centre-delta,box.centre+delta,width}};
    const auto profile=ProfileSampler::sample(image,scan,{},cancel);
    ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](const QString& id,PortData data,bool valid=true){for(const auto& p:outputPorts(t.kind))if(p.id==id){PortValue v{p,std::move(data),valid,frame,cycle,image.metadata()};v.method="reference-local-difference";v.methodVersion=1;r.outputs[id]=v;}};
    double strength=0;std::optional<QPointF> point;
    for(int i=1;i<profile.samples.size();++i){if(cancel && cancel->load()){r.state=ExecutionState::Skipped;return r;}const auto diff=std::abs(profile.samples[i].intensity-profile.samples[i-1].intensity);if(diff>=settings.contrast){strength=diff;point=profile.samples[i].point;break;}}
    put("found",point.has_value());put("strength",strength);put("point",point.value_or(QPointF{}),point.has_value());if(point)r.points={*point};return r;
}
QMutex executorMutex; QMap<QString,ToolExecutor> extensions; }
void ToolExecutors::registerExecutor(const QString& kind,ToolExecutor executor) {
    ToolRegistry::get(kind);
    QMutexLocker lock(&executorMutex);
    if(!executor || extensions.contains(kind) || QStringList{"shape_pose","pattern_pose","image_file","webcam","template_match","blob","edge","calculate","measurement","preprocess","code_read","edge_point","edge_pair","line_fit","circle_fit","circle_fit","geometry_measurement"}.contains(kind))throw std::runtime_error("Geçersiz/tekrar yürütücü kaydı.");
    extensions.insert(kind,std::move(executor));
}
ToolExecutor ToolExecutors::get(const QString& kind,const MeasurementExecutionContext& context,const std::optional<ToolRoi>& resolvedRoi) {
    if(kind=="industrial_camera")throw std::runtime_error("Industrial Camera edinim bağlantısı henüz kullanılamıyor; kaynak yalnız çevrimdışı düzenlenebilir.");
    auto select=[&]()->ToolExecutor {
    if(kind=="edge" && resolvedRoi)return [resolvedRoi](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return referenceEdge(t,inputs,*resolvedRoi,frame,cycle,cancel);};
    if(kind=="blob")return [resolvedRoi](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& dir,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return blob(t,inputs,dir,frame,cycle,cancel,resolvedRoi);};
    if(kind=="shape_pose")return [](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return ShapeMatchService::execute(t,inputs,frame,cycle,cancel);};
    if(kind=="pattern_pose")return [](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return PoseMatchService::execute(t,inputs,frame,cycle,cancel);};
    if(kind=="geometry_measurement")return [context](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return GeometryMeasurementService::execute(t,inputs,frame,cycle,cancel,context);};
    if(kind=="circle_fit")return [context](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return CircleToolService::execute(t,inputs,frame,cycle,cancel,context);};
    if(kind=="circle_fit")return [context](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return CircleToolService::execute(t,inputs,frame,cycle,cancel,context);};
    if(kind=="edge_pair" || kind=="line_fit")return [context](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& dir,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return MetrologyService::execute(t,inputs,dir,frame,cycle,cancel,context);};
    if(kind=="edge_point")return [context](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){
        auto r=EdgePointService::execute(t,inputs,directory,frame,cycle,cancel);const auto& point=r.outputs["point"];PortValue physical{{"pointMm",PortType::Point,"mm","plane"},QPointF{},false,frame,cycle,point.origin};physical.method="profile-edge-mm";physical.methodVersion=1;physical.invalidReason=point.valid ? (context.error.isEmpty() ? "NO_ACTIVE_CALIBRATION":context.error):"NOT_FOUND";
        if(point.valid && context.calibration && context.current)try{physical.data=CircleToolService::mapPoint(std::get<ImageFrame>(inputs["image"].data),context,std::get<QPointF>(point.data),false);physical.valid=true;physical.invalidReason.clear();r.calibration=context.calibration->dependency();}catch(const std::exception& e){physical.invalidReason=QString::fromUtf8(e.what());}
        r.outputs["pointMm"]=physical;return r;
    };
    if(kind=="code_read")return [resolvedRoi](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& dir,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){return CodeReadService::execute(t,inputs,dir,frame,cycle,cancel,resolvedRoi);};
    if(kind=="preprocess")return [](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel){
        ToolRegistry::validate(t);const auto output=Preprocessing::run(std::get<ImageFrame>(inputs.value("image").data),t.parameters,cancel);
        ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;r.outputs["image"]={outputPorts(t.kind).first(),output,true,frameId,cycleId,output.metadata()};return r;
    };
    if(kind=="measurement")return [context](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>&) {
        ToolResult r;r.toolId=t.id;r.state=ExecutionState::Error;r.errorCode="CALIBRATION_INVALID";
        if(!context.calibration || !context.current) {r.message=context.error.isEmpty() ? "Kalibrasyon bağlantısı/güncel kurulum çözümlenmedi." : context.error;return r;}
        ToolResult a,b;a.state=b.state=ExecutionState::Done;a.outputs["point"]=inputs.value("p1");b.outputs["point"]=inputs.value("p2");
        const auto value=MeasurementService{}.distance(a,"point",b,"point",*context.calibration,*context.current,context.rawSize,t.parameters["lowerMm"].toDouble(),t.parameters["upperMm"].toDouble());
        r.state=value.state;r.decision=value.decision;r.errorCode=value.errorCode;r.message=value.error;
        if(value.state!=ExecutionState::Done)return r;
        r.calibration=value.calibration;
        for(const auto& p:outputPorts(t.kind)) {
            const PortData data=p.id=="value" ? PortData(*value.millimetres) : p.id=="pixels" ? PortData(*value.pixels) : PortData(value.decision==PartDecision::Pass);
            r.outputs[p.id]={p,data,true,frameId,cycleId,value.origin};
        }
        r.points={std::get<QPointF>(inputs.value("p1").data),std::get<QPointF>(inputs.value("p2").data)};
        r.message=QString("%1 • [%2, %3] mm • Calibration %4 v%5").arg(value.decision==PartDecision::Pass ? "PASS" : "FAIL").arg(t.parameters["lowerMm"].toDouble()).arg(t.parameters["upperMm"].toDouble()).arg(value.calibration.calibrationId).arg(value.calibration.version);
        return r;
    };
    static const QMap<QString,ToolExecutor> executors={{"image_file",imageFile},{"webcam",webcam},{"calculate",calculate},{"template_match",match},{"edge",edge}};
    ToolRegistry::get(kind);
    QMutexLocker lock(&executorMutex);
    if(extensions.contains(kind))return extensions.value(kind);
    const auto it=executors.constFind(kind);
    if(it==executors.cend())throw std::runtime_error("Tool yürütücüsü kayıtlı değil.");
    return *it;
    };
    auto executor=select();
    return [executor](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>& cancel){
        if(t.parameters.contains("referenceBinding"))throw std::runtime_error("Reference binding must be resolved by execution engine");
        return executor(t,inputs,directory,frame,cycle,cancel);
    };
}
}
