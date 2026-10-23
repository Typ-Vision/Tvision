#include "CircleGridService.hpp"
#include "FramedCircleGridService.hpp"
#include "vision/ImageAccess.hpp"
#include <opencv2/features.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace visionflow {
CalibrationGridObservation CircleGridService::detect(const ImageFrame& frame,const CalibrationGridTarget& t,const std::shared_ptr<std::atomic_bool>& cancel){
    auto require=[](bool b,const char* s){if(!b)throw std::runtime_error(s);};
    auto stop=[&]{require(!cancel || !cancel->load(),"Circle grid detection cancelled");};stop();t.validate();
    require(t.pattern==CalibrationGridPattern::SymmetricCircles,"Circle detector requires symmetric circle grid target");
    const auto& m=frame.metadata();require(!frame.size().isEmpty() && qint64(frame.size().width())*frame.size().height()<=16000000 && frame.processing().isEmpty() &&
        !m.sourceId.isEmpty() && !m.frameId.isEmpty() && !m.cycleId.isEmpty() && m.completedAtUtc.isValid() &&
        (m.coordinateSystemId=="image" || m.coordinateSystemId==m.sourceId+"/image"),"Raw circle grid frame/provenance required, limit 16 MP");
    const auto raw=ImageAccess(frame).ownedCopy();cv::Mat gray;
    switch(frame.format()){
    case PixelFormat::Gray8:gray=raw;break;
    case PixelFormat::Gray16:raw.convertTo(gray,CV_8U,1./257);break;
    case PixelFormat::Bgr8:cv::cvtColor(raw,gray,cv::COLOR_BGR2GRAY);break;
    case PixelFormat::Bgra8:cv::cvtColor(raw,gray,cv::COLOR_BGRA2GRAY);break;
    case PixelFormat::Rgb8:cv::cvtColor(raw,gray,cv::COLOR_RGB2GRAY);break;
    case PixelFormat::Rgba8:cv::cvtColor(raw,gray,cv::COLOR_RGBA2GRAY);break;
    }
    cv::SimpleBlobDetector::Params p;p.minThreshold=10;p.maxThreshold=240;p.thresholdStep=10;p.minRepeatability=2;
    p.filterByColor=true;p.blobColor=0;p.filterByArea=true;p.minArea=9;p.maxArea=1000000;
    p.filterByCircularity=false;p.filterByConvexity=true;p.minConvexity=.8;p.filterByInertia=true;p.minInertiaRatio=.1;
    const auto detector=cv::SimpleBlobDetector::create(p);std::vector<cv::KeyPoint> blobs;detector->detect(gray,blobs);stop();
    if(blobs.size()!=size_t(t.columns*t.rows))throw std::runtime_error(
        QString("Circle count differs from declared full grid: detected %1, expected %2 (partial/clutter unsupported)")
            .arg(qulonglong(blobs.size())).arg(t.columns*t.rows).toStdString());
    // A returned count alone is insufficient: some cluster assignments repeat
    // a blob or fold rows under perspective. Validate topology without fitting H.
    for(int flags:{int(cv::CALIB_CB_SYMMETRIC_GRID),int(cv::CALIB_CB_SYMMETRIC_GRID | cv::CALIB_CB_CLUSTERING)}){
        stop();std::vector<cv::Point2f> centres;
        if(!cv::findCirclesGrid(gray,{t.columns,t.rows},centres,flags,detector))continue;
        stop();if(centres.size()!=size_t(t.columns*t.rows))continue;
        CalibrationGridObservation o{t,m,frame.size(),{},{},QString::fromLatin1(frame.contentId().toHex())};
        for(int i=0;i<int(centres.size());++i){const auto q=centres[i];require(std::isfinite(q.x) && std::isfinite(q.y) && q.x>=0 && q.y>=0 && q.x<=gray.cols-1 && q.y<=gray.rows-1,"Invalid circle centre");
            o.pixels.append({q.x,q.y});o.worldMillimetres.append({(i%t.columns)*t.squareMillimetres,(i/t.columns)*t.squareMillimetres});}
        try{o.validateRaw();}catch(const std::exception&){continue;}
        stop();return o;
    }
    // OpenCV 5 may return repeated assignments even for an axis-aligned full
    // grid. A narrow row fallback uses actual detected centres, never inferred
    // points. It intentionally excludes perspective/rotated row recovery.
    cv::Mat mask,labels,stats,centroids;cv::threshold(gray,mask,127,255,cv::THRESH_BINARY_INV);
    const int count=cv::connectedComponentsWithStats(mask,labels,stats,centroids,8);
    require(count==t.columns*t.rows+1,"Circle components differ from declared grid");blobs.clear();
    for(int i=1;i<count;++i){const double area=stats.at<int>(i,cv::CC_STAT_AREA),width=stats.at<int>(i,cv::CC_STAT_WIDTH),height=stats.at<int>(i,cv::CC_STAT_HEIGHT);
        require(area>=9 && width/height>=.9 && width/height<=1.1 && area/(width*height)>=.6 && area/(width*height)<=.88,"Axis-aligned circle shape gate failed");
        blobs.emplace_back(cv::Point2f(float(centroids.at<double>(i,0)),float(centroids.at<double>(i,1))),1);}
    std::sort(blobs.begin(),blobs.end(),[](const auto& a,const auto& b){return a.pt.y<b.pt.y;});
    CalibrationGridObservation aligned{t,m,frame.size(),{},{},QString::fromLatin1(frame.contentId().toHex())};
    bool regular=true;double spacing=0;
    for(int row=0;row<t.rows;++row){auto begin=blobs.begin()+row*t.columns;std::sort(begin,begin+t.columns,[](const auto& a,const auto& b){return a.pt.x<b.pt.x;});
        for(int col=0;col<t.columns;++col){const auto q=(begin+col)->pt;aligned.pixels.append({q.x,q.y});aligned.worldMillimetres.append({col*t.squareMillimetres,row*t.squareMillimetres});
            if(col==1 && row==0)spacing=q.x-begin->pt.x;
            if(col>0 && (spacing<=1 || std::abs(q.x-(begin+col-1)->pt.x-spacing)>.05*spacing))regular=false;
            if(std::abs(q.y-begin->pt.y)>.05*spacing)regular=false;
            if(row>0 && std::abs(q.x-blobs[col].pt.x)>.05*spacing)regular=false;
        }
    }
    if(regular){stop();aligned.validateRaw();return aligned;}
    throw std::runtime_error("Full symmetric circle grid with consistent order/geometry not found");
}
}

namespace visionflow {
PlateDetectionResult CircleGridService::observe(const ImageFrame& frame,const CalibrationGridTarget& target,const std::shared_ptr<std::atomic_bool>& cancel){
    try{target.validate();}catch(const std::exception& e){return PlateDetectionResult::failure(PlateDetectionStatus::InvalidInput,QString::fromUtf8(e.what()));}
    if(cancel && cancel->load())return PlateDetectionResult::failure(PlateDetectionStatus::Cancelled,"Plate detection cancelled");
    if(target.pattern==CalibrationGridPattern::FramedCircles)return FramedCircleGridService::observe(frame,target,cancel);
    if(target.pattern!=CalibrationGridPattern::SymmetricCircles)return PlateDetectionResult::failure(PlateDetectionStatus::Unsupported,"Circle detector requires a circle target");
    try{
        auto grid=detect(frame,target,cancel);QVector<PlateCentre> centres;
        for(int i=0;i<grid.pixels.size();++i)centres.append({i/target.columns,i%target.columns,grid.pixels[i],{}});
        return PlateDetectionResult::ready(RawPlateObservation::create(std::move(grid),std::move(centres),{},"generic_circle_grid/1"));
    }catch(const std::exception& e){return PlateDetectionResult::failure(cancel && cancel->load() ? PlateDetectionStatus::Cancelled:PlateDetectionStatus::NotFound,QString::fromUtf8(e.what()));}
}
}
