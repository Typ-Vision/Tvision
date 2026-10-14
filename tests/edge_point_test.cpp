#include "vision/metrology/EdgeFinder.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <numbers>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double e,const char* why){check(std::abs(a-b)<=e,why);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"expected edge rejection");}
ImageFrame frame(const cv::Mat& m){return ImageFrame::fromBuffer(m.data,m.rows*m.step,m.cols,m.rows,m.step,m.depth()==CV_16U ? PixelFormat::Gray16:PixelFormat::Gray8,{"source","frame","cycle","raw"},false);}
ToolRoi roi(QPointF a,QPointF b,double width=2){return {{"raw"},ScanSegment{a,b,width}};}
EdgeSearch row(const QVector<int>& values,const EdgeOptions& o){cv::Mat m(5,values.size(),CV_8UC1);for(int y=0;y<m.rows;++y)for(int x=0;x<m.cols;++x)m.at<uchar>(y,x)=uchar(values[x]);return EdgeFinder::find(frame(m),roi({0,2},{double(m.cols-1),2}),o);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    EdgeOptions o;o.minContrast=10;auto p=row({0,0,10,40,60,60},o);
    check(p.selected && p.candidates[*p.selected].refined,"single peak has parabolic refinement");
    near(p.candidates[*p.selected].point.x(),2.5+1.0/6,1e-12,"independent three-point parabola vertex");
    o.subpixel=false;p=row({0,0,10,40,60,60},o);near(p.candidates[*p.selected].point.x(),2.5,1e-12,"explicit discrete midpoint mode");
    o.subpixel=true;o.minContrast=20;p=row({0,0,20,40,40},o);check(p.selected && !p.candidates[*p.selected].refined && p.candidates[*p.selected].refinement=="plateau_midpoint","plateau quality explicit");near(p.candidates[*p.selected].point.x(),2,1e-12,"plateau geometric midpoint");
    o.minContrast=std::nextafter(20.,21.);p=row({0,0,20,40,40},o);check(!p.selected && p.candidates[0].reason=="min_contrast","inclusive sampled peak threshold nextafter");
    o.minContrast=20;p=row({0,20,20,20},o);check(!p.selected && p.candidates[0].reason=="border_support","first derivative lacks refinement support");
    p=row({0,0,0,20},o);check(!p.selected && p.candidates[0].reason=="border_support","last derivative lacks support");
    p=row({0,10,20,30,40},o);check(!p.selected,"constant slope is not an edge peak");p=row({30,30,30,30},o);check(!p.selected && p.candidates.isEmpty(),"flat no edge");
    reject([&]{row({0,20,20},o);});
    const QVector<int> multiple{0,0,40,40,40,0,0,0,40,40,40,40};
    o.selection=EdgeSelection::First;p=row(multiple,o);near(p.candidates[*p.selected].point.x(),1.5,1e-12,"first peak in scan order");
    o.selection=EdgeSelection::Last;p=row(multiple,o);near(p.candidates[*p.selected].point.x(),7.5,1e-12,"last peak in scan order");
    o.selection=EdgeSelection::Strongest;p=row(multiple,o);near(p.candidates[*p.selected].point.x(),1.5,1e-12,"equal strength tie first");
    o.polarity=EdgePolarity::Falling;p=row(multiple,o);near(p.candidates[*p.selected].point.x(),4.5,1e-12,"falling peak");check(p.candidates[*p.selected].signedStrength<0,"falling signed strength");
    o.polarity=EdgePolarity::Rising;p=row({100,100,60,60,60},o);check(!p.selected && p.candidates[0].reason=="polarity","wrong polarity does not invent found");
    o.polarity=EdgePolarity::Any;p=row({0,0,20,20,0,0,0,60,60,60},o);near(p.candidates[*p.selected].point.x(),6.5,1e-12,"strongest later distinct peak");
    QString report;QTextStream out(&report);double sum=0,maxError=0,noisySum=0,noisyMax=0;int count=0,noisyCount=0;
    // Independent analytic blurred step raster: quantized Gray16, arbitrary subpixel phase.
    for(double angle:{0.,45.,90.,135.,17.3,78.1,167.2})for(double phase:{-.37,0.,.29})for(bool falling:{false,true}){
        const double a=angle*std::numbers::pi/180;const QPointF d(std::cos(a),std::sin(a)),centre(63.2,64.35);
        const QPointF edge=centre+d*phase;cv::Mat m(128,128,CV_16UC1);
        for(int y=0;y<128;++y)for(int x=0;x<128;++x){const double distance=(x-edge.x())*d.x()+(y-edge.y())*d.y();const double v=.5+(falling ? -.4:.4)*std::erf(distance/(std::sqrt(2.)*1.4));m.at<quint16>(y,x)=quint16(std::lround(v*65535));}
        o={};o.profile={.5,1,1};o.polarity=falling ? EdgePolarity::Falling:EdgePolarity::Rising;o.minContrast=10;
        auto f=frame(m);const auto hash=f.contentId();const QPointF start=centre-d*40,end=centre+d*40;
        const auto r=EdgeFinder::find(f,roi(start,end),o);check(bool(r.selected),"analytic blurred edge found");
        const auto& c=r.candidates[*r.selected];const double error=QLineF(c.point,edge).length();sum+=error*error;maxError=std::max(maxError,error);++count;
        check(c.refined,"clean analytic peak subpixel quality");near(error,0,.30,"clean edge maximum localization target");check(f.contentId()==hash,"edge finder cannot mutate raw image");
        auto reversed=o;reversed.polarity=falling ? EdgePolarity::Rising:EdgePolarity::Falling;const auto reverse=EdgeFinder::find(f,roi(end,start),reversed);check(bool(reverse.selected),"reverse polarity edge found");near(QLineF(c.point,reverse.candidates[*reverse.selected].point).length(),0,1e-8,"reverse physical edge invariant");
        out<<"angle="<<angle<<" phase="<<phase<<" falling="<<falling<<" errorPx="<<error<<"\n";
        // A second, degraded corpus: wider blur and deterministic +/-6 gray-level noise.
        for(int y=0;y<128;++y)for(int x=0;x<128;++x){const double distance=(x-edge.x())*d.x()+(y-edge.y())*d.y();
            const unsigned hashNoise=(unsigned(x)*73856093u)^(unsigned(y)*19349663u)^1234567u;
            const double noise=(int(hashNoise%13)-6)/255.0;
            const double v=.5+(falling ? -.4:.4)*std::erf(distance/(std::sqrt(2.)*2.6))+noise;m.at<quint16>(y,x)=quint16(std::lround(v*65535));}
        const auto degraded=EdgeFinder::find(frame(m),roi(start,end,4),o);check(bool(degraded.selected),"degraded blurred/noisy edge found");
        const double noisyError=QLineF(degraded.candidates[*degraded.selected].point,edge).length();noisySum+=noisyError*noisyError;noisyMax=std::max(noisyMax,noisyError);++noisyCount;
        near(noisyError,0,.5,"separate degraded corpus max error gate");
    }
    const double rms=std::sqrt(sum/count);check(rms<=.15,"clean edge RMS target");out<<"count="<<count<<" RMS="<<rms<<" max="<<maxError<<" px\n";
    out<<"degraded count="<<noisyCount<<" blurSigma=2.6 noise=+/-6 gray levels seed=1234567 RMS="<<std::sqrt(noisySum/noisyCount)<<" max="<<noisyMax<<" px (separate 0.5px gate)\n";
    o.minContrast=0;reject([&]{row(multiple,o);});o.minContrast=std::numeric_limits<double>::quiet_NaN();reject([&]{row(multiple,o);});o.minContrast=20;o.polarity=EdgePolarity(9);reject([&]{row(multiple,o);});
    auto stop=std::make_shared<std::atomic_bool>(true);o={};cv::Mat blank(32,32,CV_8UC1,cv::Scalar(0));reject([&]{EdgeFinder::find(frame(blank),roi({2,16},{29,16}),o,stop);});
    out<<checks<<" checks passed (software corpus; physical acceptance pending)\n";
    if(argc==2){QFile file(QString::fromLocal8Bit(argv[1]));check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"report output");file.write(report.toUtf8());}
    QTextStream(stdout)<<report;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
