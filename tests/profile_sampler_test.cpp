#include "vision/metrology/ProfileSampler.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <future>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <thread>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double tolerance,const char* why){check(std::abs(a-b)<=tolerance,why);}
template<class F>void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
FrameMetadata metadata{"source","frame","cycle","camera/raw"};
ToolRoi roi(ScanSegment scan){return {{"camera/raw"},scan};}
ImageFrame gray(const cv::Mat& m){return ImageFrame::fromBuffer(m.data,m.step*m.rows,m.cols,m.rows,m.step,m.depth()==CV_16U ? PixelFormat::Gray16:PixelFormat::Gray8,metadata,false);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QString report;QTextStream out(&report);out<<"T4.1 profile sampler: independent raster/analytic oracles; no edge selection or physical accuracy claim.\n";
    // A bilinear polynomial is reproduced exactly by raster interpolation. Cross-strip averaging
    // on an oblique line adds a known second moment, independently of the interpolation code.
    cv::Mat ramp(96,96,CV_16UC1);
    for(int y=0;y<ramp.rows;++y)for(int x=0;x<ramp.cols;++x)ramp.at<quint16>(y,x)=quint16(100+101*x+203*y+3*x*y);
    auto image=gray(ramp);const auto original=image.contentId();double maxError=0;
    for(double angle:{0.,45.,90.,135.,17.3,78.1,167.2}){
        const double radians=angle*std::numbers::pi/180;const QPointF d(std::cos(radians),std::sin(radians));
        ScanSegment scan{{47.25,46.5},{},6};scan.start-=d*25;scan.end=scan.start+d*50;
        auto p=ProfileSampler::sample(image,roi(scan),{.7,1.5,0});
        near(p.samples.last().distancePx,50,1e-10,"exact endpoint distance");check(p.samples.last().point==scan.end,"exact endpoint position");
        check(p.metadata.frameId==metadata.frameId && p.metadata.cycleId==metadata.cycleId && p.inputContentId==original,"input identity and metadata");
        // Width offsets [-3,-1.5,0,1.5,3] have second moment 4.5.
        for(const auto& s:p.samples){const double x=s.point.x(),y=s.point.y();const double expected=(100+101*x+203*y+3*(x*y-4.5*d.x()*d.y()))*(255.0/65535.0);maxError=std::max(maxError,std::abs(s.intensity-expected));near(s.intensity,expected,1e-10,"oblique bilinear polynomial and full width average");}
        for(const auto& v:p.derivatives)near(v.value,(101*d.x()+203*d.y()+3*(d.x()*v.point.y()+d.y()*v.point.x()))*255/65535,1e-9,"analytic polynomial derivative at midpoint");
        auto reversed=scan;std::swap(reversed.start,reversed.end);auto r=ProfileSampler::sample(image,roi(reversed),{.7,1.5,0});
        check(r.samples.size()==p.samples.size(),"reverse sample count");
        for(int i=0;i<p.samples.size();++i)near(r.samples[i].intensity,p.samples[p.samples.size()-1-i].intensity,1e-10,"reverse intensity sequence");
        for(int i=0;i<p.derivatives.size();++i){const auto& a=p.derivatives[i];const auto& b=r.derivatives[r.derivatives.size()-1-i];near(a.value,-b.value,1e-9,"reverse signed derivative");near(QLineF(a.point,b.point).length(),0,1e-10,"reverse derivative raw coordinate");}
        out<<"angle="<<angle<<" samples="<<p.samples.size()<<" step="<<p.actualStepPx<<"\n";
    }
    out<<"polynomial max intensity error="<<QString::number(maxError,'g',16)<<"\n";
    check(image.contentId()==original,"raw buffer unchanged");
    // A quarter pixel cross term needs all four neighbours; fractional results must survive.
    cv::Mat checker(4,4,CV_8UC1,cv::Scalar(0));checker.at<uchar>(1,2)=200;checker.at<uchar>(2,1)=100;
    auto c=ProfileSampler::sample(gray(checker),roi({{1.5,1.5},{2,1.5},.5}),{.25,.25,0});
    near(c.samples.first().intensity,75,1e-12,"four-neighbour interpolation with cross-strip average");
    // Six formats, padded source stride, semantic RGB/BGR and alpha ignored.
    for(auto format:{PixelFormat::Gray8,PixelFormat::Gray16,PixelFormat::Rgb8,PixelFormat::Bgr8,PixelFormat::Rgba8,PixelFormat::Bgra8}){
        const int channels=format==PixelFormat::Rgb8 || format==PixelFormat::Bgr8 ? 3:format==PixelFormat::Rgba8 || format==PixelFormat::Bgra8 ? 4:1;
        const int bpp=format==PixelFormat::Gray16 ? 2:channels,stride=8*bpp+12;QByteArray bytes(stride*8,char(0xee));
        for(int y=0;y<8;++y)for(int x=0;x<8;++x){auto p=reinterpret_cast<unsigned char*>(bytes.data())+y*stride+x*bpp;if(format==PixelFormat::Gray16){quint16 v=10001;std::memcpy(p,&v,2);}else if(channels==1)*p=77;else{const bool rgb=format==PixelFormat::Rgb8 || format==PixelFormat::Rgba8;p[rgb ? 0:2]=100;p[1]=50;p[rgb ? 2:0]=10;if(channels==4)p[3]=uchar(x*20);}}
        auto f=ImageFrame::fromBuffer(bytes.data(),bytes.size(),8,8,stride,format,metadata,false);bytes.fill(0);
        auto p=ProfileSampler::sample(f,roi({{1,3},{6,3},2}));const double expected=format==PixelFormat::Gray16 ? 10001*255.0/65535:channels==1 ? 77:.299*100+.587*50+.114*10;
        for(const auto& s:p.samples)near(s.intensity,expected,1e-12,"format/stride/alpha and Gray16 precision");for(const auto& d:p.derivatives)near(d.value,0,1e-12,"flat image has no border edge");
    }
    // Gaussian: impulse energy/symmetry and exact affine reproduction, without a copied kernel oracle.
    cv::Mat impulse(7,101,CV_8UC1,cv::Scalar(0));impulse.col(50).setTo(200);
    auto smooth=ProfileSampler::sample(gray(impulse),roi({{0,3},{100,3},2}),{1,1,2});
    check(smooth.smoothingRadius==6 && !smooth.samples.first().smoothed && !smooth.samples.last().smoothed,"no synthetic Gaussian border support");
    double sum=0;for(const auto& s:smooth.samples)if(s.smoothed)sum+=*s.smoothed;near(sum,200,1e-10,"Gaussian impulse energy");
    for(int k=0;k<=6;++k)near(*smooth.samples[50-k].smoothed,*smooth.samples[50+k].smoothed,1e-12,"Gaussian symmetry");
    check(*smooth.samples[50].smoothed<200 && *smooth.samples[49].smoothed>0,"smoothing spreads impulse");
    check(smooth.derivatives.first().distancePx==6.5 && smooth.derivatives.last().distancePx==93.5,"derivative support excludes Gaussian borders");
    auto smoothReverse=ProfileSampler::sample(gray(impulse),roi({{100,3},{0,3},2}),{1,1,2});
    for(int i=0;i<smooth.derivatives.size();++i)near(smooth.derivatives[i].value,-smoothReverse.derivatives[smooth.derivatives.size()-1-i].value,1e-12,"Gaussian reverse signed gradient");
    cv::Mat affine(7,101,CV_8UC1);for(int y=0;y<7;++y)for(int x=0;x<=100;++x)affine.at<uchar>(y,x)=uchar(20+2*x);
    auto linear=ProfileSampler::sample(gray(affine),roi({{0,3},{100,3},2}),{.5,1,1.2});
    for(const auto& s:linear.samples)if(s.smoothed)near(*s.smoothed,20+2*s.point.x(),1e-10,"Gaussian preserves affine ramp");
    for(const auto& d:linear.derivatives)near(d.value,2,1e-10,"signed derivative units intensity per pixel");
    auto tinySigma=ProfileSampler::sample(gray(affine),roi({{0,3},{100,3},2}),{1,1,1e-200});
    for(const auto& d:tinySigma.derivatives)near(d.value,2,1e-10,"tiny finite sigma cannot underflow to NaN kernel");
    cv::Mat noisy(7,101,CV_8UC1,cv::Scalar(100));
    for(int y=1;y<=5;++y)for(int x=0;x<=100;++x)noisy.at<uchar>(y,x)=uchar(100+(y-3)*((x%2)==0 ? 9:-9));
    auto averaged=ProfileSampler::sample(gray(noisy),roi({{0,3},{100,3},4}));
    for(const auto& s:averaged.samples)near(s.intensity,100,1e-12,"zero mean transverse noise cancels through width average");
    cv::Mat blurred(7,101,CV_16UC1);
    for(int y=0;y<7;++y)for(int x=0;x<=100;++x)blurred.at<quint16>(y,x)=quint16(std::lround(65535*(.5+.4*std::erf((x-50.25)/3.0))));
    auto blur=ProfileSampler::sample(gray(blurred),roi({{0,3},{100,3},2}),{.5,1,.75});
    double blurIntegral=0;for(const auto& d:blur.derivatives){check(d.value>=-1e-10,"blurred antialiased step polarity");blurIntegral+=d.value*blur.actualStepPx;}
    const double rasterContrast=(int(blurred.at<quint16>(3,100))-int(blurred.at<quint16>(3,0)))*255.0/65535;
    near(blurIntegral,rasterContrast,1e-9,"blurred step integral equals independently quantized endpoint contrast");
    near(blurIntegral,204,255.0/65535+1e-9,"ideal blurred contrast within one Gray16 quantization unit");
    // Rising/falling step, multiple edges and antialiased blur remain profiles, not selected edges.
    for(bool falling:{false,true}){cv::Mat step(7,101,CV_8UC1);for(int y=0;y<7;++y)for(int x=0;x<=100;++x)step.at<uchar>(y,x)=uchar((x<50)==falling ? 220:20);
        auto p=ProfileSampler::sample(gray(step),roi({{0,3},{100,3},2}),{.5,1,1});double area=0;for(const auto& d:p.derivatives){check(falling ? d.value<=1e-10:d.value>=-1e-10,"step derivative polarity");area+=d.value*p.actualStepPx;}near(area,falling ? -200:200,1e-9,"step derivative integral preserves contrast");}
    cv::Mat stripes(7,101,CV_8UC1,cv::Scalar(20));stripes.colRange(25,75).setTo(220);auto stripe=ProfileSampler::sample(gray(stripes),roi({{0,3},{100,3},2}));near(stripe.derivatives[24].value,200,1e-12,"stripe rising position");near(stripe.derivatives[74].value,-200,1e-12,"stripe falling position");
    auto processed=ImageAccess::processed(image,ImageAccess(image).ownedCopy(),image.format(),"identity",1,QCryptographicHash::hash("params",QCryptographicHash::Sha256),false);
    auto processedProfile=ProfileSampler::sample(processed,roi({{10,10},{50,10},2}));check(processedProfile.processingId==processed.processingId() && !processedProfile.processingId.isEmpty(),"processed provenance distinct from same pixels");
    // Domain boundary fits exactly; outside strips are rejected as a whole.
    auto border=ProfileSampler::sample(gray(affine),roi({{0,1},{100,1},2}));check(border.samples.size()==101,"exact image boundary supported");
    reject([&]{ProfileSampler::sample(gray(affine),roi({{0,.999},{100,.999},2}));},"outside footprint cannot be padded");
    reject([&]{ProfileSampler::sample(image,roi({{10,10},{10,10},1}));},"zero arrow");
    reject([&]{ProfileSampler::sample(image,roi({{10,10},{20,20},0}));},"zero width");
    reject([&]{ProfileSampler::sample(image,roi({{10,10},{20,20},1}),{std::numeric_limits<double>::quiet_NaN(),1,0});},"NaN step");
    reject([&]{ProfileSampler::sample(image,roi({{10,10},{20,20},1}),{1,1,std::numeric_limits<double>::infinity()});},"infinite sigma");
    reject([&]{ProfileSampler::sample(image,roi({{10,10},{20,20},std::numeric_limits<double>::infinity()}));},"nonfinite footprint");
    for(auto o:{ProfileOptions{0,1,0},ProfileOptions{17,1,0},ProfileOptions{1,0,0},ProfileOptions{1,1,-1},ProfileOptions{1,1,17}})reject([&]{ProfileSampler::sample(image,roi({{10,10},{20,20},1}),o);},"invalid options");
    reject([&]{ProfileSampler::sample(image,roi({{10,10},{11,10},1}),{1,1,1});},"not enough smoothing support");
    reject([&]{ProfileSampler::sample(image,ToolRoi{{"other/raw"},ScanSegment{{10,10},{20,20},1}});},"frame mismatch");
    reject([&]{ProfileSampler::sample(image,ToolRoi{{"camera/raw",LengthUnit::Millimetre},ScanSegment{{10,10},{20,20},1}});},"world is not raw");
    reject([&]{ProfileSampler::sample(image,ToolRoi{{"camera/raw"},Annulus{{40,40},5,10}});},"wrong ROI kind");
    reject([&]{ProfileSampler::sample({},roi({{1,1},{2,2},1}));},"empty image");
    cv::Mat large(1026,4096,CV_8UC1,cv::Scalar(55));auto big=gray(large);auto broad=roi({{0,513},{4095,513},1024});
    reject([&]{ProfileSampler::sample(big,broad);},"across count limit");
    reject([&]{ProfileSampler::sample(big,broad,{.05,2,0});},"length sample count limit");
    reject([&]{ProfileSampler::sample(big,broad,{.1,2,0});},"total tap limit");
    reject([&]{ProfileSampler::sample(big,roi({{0,513},{4095,513},2}),{.1,1,16});},"Gaussian convolution work limit");
    auto cancelled=std::make_shared<std::atomic_bool>(true);reject([&]{ProfileSampler::sample(image,roi({{10,10},{20,20},1}),{},cancelled);},"pre-cancelled job");
    std::promise<void> started;auto ready=started.get_future();
    cancelled->store(false);auto worker=std::async(std::launch::async,[&]{started.set_value();try{ProfileSampler::sample(big,broad,{1,2,0},cancelled);return false;}catch(const std::exception& e){return QString::fromUtf8(e.what()).contains("cancelled");}});
    ready.wait();std::this_thread::sleep_for(std::chrono::milliseconds(2));cancelled->store(true);
    check(worker.get(),"in-flight cooperative cancellation returns no partial result");
    out<<"checks="<<checks<<"; PASS (software profile contract only)\n";
    if(argc>1){QFile file(QString::fromLocal8Bit(argv[1]));check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"report write");file.write(report.toUtf8());}
    QTextStream(stdout)<<report;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
