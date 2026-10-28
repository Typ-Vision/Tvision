#include "ProfileSampler.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool bounded(double v,double low,double high){return std::isfinite(v) && v>=low && v<=high;}
// Read immutable rows directly; neither a full-image copy nor an 8-bit conversion is needed.
double pixel(const ImageAccess& access,PixelFormat format,int x,int y) {
    const auto row=access.row(y);
    if(format==PixelFormat::Gray8)return std::to_integer<unsigned char>(row[x]);
    if(format==PixelFormat::Gray16){quint16 value;std::memcpy(&value,row.data()+2*x,2);return value*(255.0/65535.0);}
    const bool rgb=format==PixelFormat::Rgb8 || format==PixelFormat::Rgba8;
    const int channels=format==PixelFormat::Rgb8 || format==PixelFormat::Bgr8 ? 3:4;
    const auto p=row.data()+channels*x;
    return .299*std::to_integer<unsigned char>(p[rgb ? 0:2])+.587*std::to_integer<unsigned char>(p[1])+.114*std::to_integer<unsigned char>(p[rgb ? 2:0]);
}
double bilinear(const ImageAccess& access,const ImageFrame& frame,QPointF p) {
    const auto size=frame.size();
    // Only roundoff at an already validated boundary may be snapped, never an outside ROI.
    require(p.x()>=-1e-9 && p.y()>=-1e-9 && p.x()<=size.width()-1+1e-9 && p.y()<=size.height()-1+1e-9,"Profile sample outside image");
    const double x=std::clamp(p.x(),0.0,double(size.width()-1)),y=std::clamp(p.y(),0.0,double(size.height()-1));
    const int x0=int(std::floor(x)),y0=int(std::floor(y));
    const int x1=int(std::ceil(x)),y1=int(std::ceil(y));
    const double fx=x-x0,fy=y-y0;
    return (1-fy)*((1-fx)*pixel(access,frame.format(),x0,y0)+fx*pixel(access,frame.format(),x1,y0))
           +fy*((1-fx)*pixel(access,frame.format(),x0,y1)+fx*pixel(access,frame.format(),x1,y1));
}
}
SampledProfile ProfileSampler::sample(const ImageFrame& frame,const ToolRoi& roi,const ProfileOptions& options,
                                      const std::shared_ptr<std::atomic_bool>& cancel) {
    roi.validate();
    require(std::holds_alternative<ScanSegment>(roi.geometry),"Profile requires scan ROI");
    require(roi.frame.id==frame.metadata().coordinateSystemId,"Profile coordinate frame mismatch");
    require(roi.inside(frame.size()),"Entire profile footprint must be inside pixel-centre domain");
    return sampleMapped(frame,std::get<ScanSegment>(roi.geometry),[](QPointF p){return p;},options,cancel);
}
SampledProfile ProfileSampler::sampleMapped(const ImageFrame& frame,const ScanSegment& scan,const ProfileRawMap& map,
                                           const ProfileOptions& options,const std::shared_ptr<std::atomic_bool>& cancel) {
    auto stop=[&]{if(cancel && cancel->load(std::memory_order_relaxed))throw std::runtime_error("Profile sampling cancelled");};
    stop();require(bool(map),"Missing profile raw map");
    ToolRoi{{"sampling"},scan}.validate();
    require(qint64(frame.size().width())*frame.size().height()<=16*1024*1024,"Profile image limit: 16 Mi pixels");
    require(bounded(options.stepPx,.05,16) && bounded(options.widthStepPx,.25,16) && bounded(options.sigmaPx,0,16),"Invalid profile step/sigma");
    const double length=QLineF(scan.start,scan.end).length();
    require(scan.width<=1024,"Profile width limit: 1024 px");
    const double intervals=std::ceil(length/options.stepPx),widthIntervals=std::ceil(scan.width/options.widthStepPx);
    require(intervals<=65535 && widthIntervals<=1023,"Profile sample count limit");
    const int count=int(intervals)+1,across=int(widthIntervals)+1;
    require(qint64(count)*across<=4*1024*1024,"Profile tap limit: 4 Mi");
    SampledProfile result;result.scan=scan;result.options=options;result.metadata=frame.metadata();
    result.actualStepPx=length/(count-1);result.actualWidthStepPx=scan.width/(across-1);result.widthSamples=across;
    const double radius=options.sigmaPx==0 ? 0:std::ceil(3*options.sigmaPx/result.actualStepPx);
    require(radius<=1024 && count>=2*radius+2,"Insufficient profile support or smoothing radius limit");
    result.smoothingRadius=int(radius);
    require(qint64(count-2*result.smoothingRadius)*(2*result.smoothingRadius+1)<=16*1024*1024,"Profile convolution limit: 16 Mi taps");
    const auto direction=(scan.end-scan.start)/length;
    const QPointF normal(-direction.y(),direction.x());
    // Check mapped corners before reading; all actual taps are checked as well.
    auto raw=[&](QPointF p){auto q=map(p);require(std::isfinite(q.x()) && std::isfinite(q.y()) && q.x()>=0 && q.y()>=0 && q.x()<=frame.size().width()-1 && q.y()<=frame.size().height()-1,"Mapped profile outside raw domain");return q;};
    for(auto p:{scan.start,scan.end})for(double side:{-.5,.5})raw(p+normal*(scan.width*side));
    ImageAccess access(frame);result.samples.reserve(count);
    for(int i=0;i<count;++i) {
        stop();const double t=double(i)/(count-1);
        const auto centre=i==count-1 ? scan.end:scan.start+(scan.end-scan.start)*t;
        double intensity=0;
        for(int j=0;j<across;++j){if((j&63)==0)stop();const double offset=scan.width*(double(j)/(across-1)-.5);intensity+=bilinear(access,frame,raw(centre+normal*offset));}
        result.samples.push_back({length*t,centre,intensity/across,{}});
    }
    QVector<double> kernel;kernel.reserve(2*result.smoothingRadius+1);double total=0;
    for(int k=-result.smoothingRadius;k<=result.smoothingRadius;++k){const double q=radius==0 ? 0:k*result.actualStepPx/options.sigmaPx;const double w=std::exp(-.5*q*q);kernel<<w;total+=w;}
    for(auto& w:kernel)w/=total;
    for(int i=result.smoothingRadius;i<count-result.smoothingRadius;++i){stop();double value=0;for(int k=0;k<kernel.size();++k)value+=kernel[k]*result.samples[i+k-result.smoothingRadius].intensity;result.samples[i].smoothed=value;}
    result.derivatives.reserve(count-2*result.smoothingRadius-1);
    for(int i=result.smoothingRadius;i<count-result.smoothingRadius-1;++i){stop();const auto& a=result.samples[i];const auto& b=result.samples[i+1];result.derivatives.push_back({(a.distancePx+b.distancePx)/2,(a.point+b.point)/2,(*b.smoothed-*a.smoothed)/result.actualStepPx});}
    stop();result.inputContentId=frame.contentId();result.processingId=frame.processingId();stop();return result;
}
}
