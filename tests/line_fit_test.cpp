#include "vision/metrology/LineFitter.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double e,const char* why){check(std::isfinite(a) && std::abs(a-b)<=e,why);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"expected line rejection");}
double cross(QPointF a,QPointF b){return a.x()*b.y()-a.y()*b.x();}
QVector<LineScanPoint> samples(int count=21){QVector<LineScanPoint> r;for(int i=0;i<count;++i)r.append({QPointF(i*5,20)});return r;}
ImageFrame frame(const cv::Mat& m){return ImageFrame::fromBuffer(m.data,m.rows*m.step,m.cols,m.rows,m.step,m.depth()==CV_16U ? PixelFormat::Gray16:PixelFormat::Gray8,{"source","frame","cycle","raw",QDateTime::currentDateTimeUtc()},false);}
ToolRoi box(double angle=0,double width=120,double height=50){return {{"raw"},RotatedRectangle{{95.2,96.35},{width,height},angle}};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    {
        QVector<LineScanPoint> missing(21);missing[0].rejections.polarity=2;
        bool caught=false;try{LineFitter::fit(missing,{1,0});}catch(const FitDiagnosticError& e){caught=true;check(e.evidence.code=="LINE_POINTS" && e.evidence.stage=="edge_search" && e.evidence.requested==21 && e.evidence.found==0 && e.evidence.required==6 && e.evidence.rejections.polarity==2,"line structured missing evidence");}
        check(caught,"line structured rejection");
    }
    auto points=samples();LineFitOptions o;auto r=LineFitter::fit(points,{1,0},o);near(r.angleDegrees,0,1e-8,"horizontal angle");near(r.span,100,1e-8,"measured support span");near(r.supportStart.x(),0,1e-6,"support begins at observation");near(r.supportEnd.x(),100,1e-6,"support ends at observation");near(r.rms,0,1e-8,"exact line residual");check(r.usedCount==21 && r.rejectedCount==0 && r.missingCount==0 && r.coverage==1 && r.spanCoverage==1,"complete coverage");
    auto reverse=LineFitter::fit(points,{-1,0},o);near(reverse.angleDegrees,-180,1e-8,"directed angle [-180,180)");near(QLineF(reverse.supportStart,r.supportEnd).length(),0,1e-6,"reverse support orientation");
    for(int i:{4,10,17})points[i].point=*points[i].point+QPointF(0,8);
    const auto globalRng=cv::theRNG().state;r=LineFitter::fit(points,{1,0},o);check(cv::theRNG().state==globalRng,"no global OpenCV RNG mutation");check(r.usedCount==18 && r.rejectedCount==3,"robust outlier counts");near(r.angleDegrees,0,1e-6,"outliers do not tilt line");
    for(int i:{4,10,17})check(!r.scans[i].used && r.scans[i].reason=="outlier" && std::abs(*r.scans[i].signedResidual)>7.9,"outlier residual retained");
    const auto replay=LineFitter::fit(points,{1,0},o);check(replay.origin==r.origin && replay.tangent==r.tangent && replay.rms==r.rms,"deterministic repeated fit");
    points=samples();for(int i=0;i<6;++i)points[i].point=*points[i].point+QPointF(5000,1000);
    auto robust=o;robust.ransac=false;reject([&]{LineFitter::fit(points,{1,0},robust);});r=LineFitter::fit(points,{1,0},o);check(r.usedCount==15 && r.rejectedCount==6,"RANSAC recovers high leverage outliers");near(r.angleDegrees,0,1e-6,"RANSAC independent horizontal truth");
    // Independent symmetric orthogonal residuals with exactly zero mean and
    // covariance: the expected TLS line and RMS are analytic, not another fit.
    points=samples(20);for(int i=0;i<points.size();++i)points[i].point=*points[i].point+QPointF(0,(i%4==0 || i%4==3) ? .2:-.2);
    o.ransac=false;r=LineFitter::fit(points,{1,0},o);near(r.rms,.2,1e-7,"analytic symmetric RMS");near(r.maxResidual,.2,1e-7,"analytic maximum residual");near(r.angleDegrees,0,1e-6,"analytic TLS orientation");
    auto strict=o;strict.maxRms=.19;reject([&]{LineFitter::fit(points,{1,0},strict);});strict=o;strict.maxResidual=.19;reject([&]{LineFitter::fit(points,{1,0},strict);});
    points=samples();points[0].point.reset();points[20].point.reset();r=LineFitter::fit(points,{1,0});check(r.missingCount==2 && r.scans[0].reason=="not_found" && !r.scans[0].signedResidual && r.usedCount==19,"missing distinct from outlier");near(r.span,90,1e-6,"support never extends across missing ends");near(r.spanCoverage,.9,1e-12,"scan span coverage");
    points=samples();for(int i=6;i<21;++i)points[i].point.reset();reject([&]{LineFitter::fit(points,{1,0});});
    points=samples();for(int i=0;i<21;++i)if(i<4 || i>16)points[i].point.reset();strict={};strict.minSpanCoverage=.8;reject([&]{LineFitter::fit(points,{1,0},strict);});
    points=samples();strict={};strict.minSpan=101;reject([&]{LineFitter::fit(points,{1,0},strict);});
    points=samples();reject([&]{LineFitter::fit(points,{0,1});});reject([&]{LineFitter::fit(points,{0,0});});
    points[1].point=points[0].point;reject([&]{LineFitter::fit(points,{1,0});});points=samples();points[1].point=QPointF(NAN,1);reject([&]{LineFitter::fit(points,{1,0});});
    points=samples();for(auto& p:points)p.point=QPointF(2,2);reject([&]{LineFitter::fit(points,{1,0});});
    points.clear();for(int i=0;i<24;++i){double a=i*2*std::numbers::pi/24;points.append({QPointF(10*std::cos(a),10*std::sin(a))});}strict={};strict.inlierDistance=30;strict.maxResidual=30;strict.maxRms=30;reject([&]{LineFitter::fit(points,{1,0},strict);});
    points=samples();for(auto& p:points)p.point=*p.point+QPointF(1e9,-1e9);r=LineFitter::fit(points,{1,0});near(r.span,100,1e-5,"large coordinate normalization span");near(r.origin.y(),-1e9+20,1e-5,"large coordinate normalization offset");
    for(int i=0;i<9;++i){strict={};if(i==0)strict.minPoints=2;if(i==1)strict.minCoverage=1.1;if(i==2)strict.minSpanCoverage=0;if(i==3)strict.inlierDistance=NAN;if(i==4)strict.maxRms=0;if(i==5)strict.maxResidual=-1;if(i==6)strict.ransacIterations=1025;if(i==7)strict.minVarianceRatio=1;if(i==8)strict.minSpan=INFINITY;reject([&]{LineFitter::fit(samples(),{1,0},strict);});}
    auto stopped=std::make_shared<std::atomic_bool>(true);reject([&]{LineFitter::fit(samples(),{1,0},{},stopped);});
    QString report;QTextStream out(&report);double sum=0,maximum=0,angleMax=0,noisySum=0,noisyMax=0;int count=0;
    for(double angle:{0.,45.,90.,135.,17.3,78.1,167.2})for(double phase:{-.37,0.,.29})for(bool falling:{false,true}) {
        const double a=angle*std::numbers::pi/180;const QPointF u(std::cos(a),std::sin(a)),n(-u.y(),u.x()),centre(95.2,96.35);const auto truth=centre+n*phase;
        auto raster=[&](double sigma,bool noise){cv::Mat m(192,192,CV_16UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){const double d=(x-truth.x())*n.x()+(y-truth.y())*n.y();const unsigned h=(unsigned(x)*73856093u)^(unsigned(y)*19349663u)^1234567u;const double v=.5+(falling ? -.4:.4)*std::erf(d/(std::sqrt(2.)*sigma))+(noise ? (int(h%13)-6)/255.:0);m.at<quint16>(y,x)=quint16(std::lround(v*65535));}return m;};
        LineScanOptions scan;scan.edge.profile={.5,1,1};scan.edge.polarity=falling ? EdgePolarity::Falling:EdgePolarity::Rising;scan.edge.minContrast=10;scan.fit.minSpan=80;
        const auto image=frame(raster(1.4,false));const auto hash=image.contentId();const auto found=LineFitter::find(image,box(angle),scan);
        check(found.origin.frameId=="frame" && found.inputContentId==hash && found.processingId==image.processingId() && image.contentId()==hash,"line provenance/raw immutability");
        const auto& line=found.line;const double e=std::abs(cross(line.origin-truth,u)),angleError=std::atan2(std::abs(cross(line.tangent,u)),line.tangent.x()*u.x()+line.tangent.y()*u.y())*180/std::numbers::pi;
        sum+=e*e;maximum=std::max(maximum,e);angleMax=std::max(angleMax,angleError);++count;
        near(e,0,.30,"clean line offset gate");near(angleError,0,.1,"clean line angle gate");check(line.usedCount==21 && line.coverage==1 && line.span>117,"full analytic line support");
        for(const auto& s:line.scans)near(std::abs(cross(*s.point-truth,u)),0,.30,"independent per-scan endpoint");
        const auto noisy=LineFitter::find(frame(raster(2.6,true)),box(angle),scan);const double ne=std::abs(cross(noisy.line.origin-truth,u));noisySum+=ne*ne;noisyMax=std::max(noisyMax,ne);near(ne,0,.5,"separate degraded line offset gate");
        out<<"angle="<<angle<<" phase="<<phase<<" falling="<<falling<<" offsetPx="<<e<<" angleErrorDeg="<<angleError<<" noisyOffsetPx="<<ne<<"\n";
    }
    check(std::sqrt(sum/count)<=.15,"clean line RMS offset gate");out<<"clean count="<<count<<" offsetRMS="<<std::sqrt(sum/count)<<" offsetMax="<<maximum<<" px angleMax="<<angleMax<<" deg\n";
    out<<"degraded count="<<count<<" blurSigma=2.6 noise=+/-6 seed=1234567 offsetRMS="<<std::sqrt(noisySum/count)<<" offsetMax="<<noisyMax<<" px\n";
    // Actual raster corruption: scan-local spurious high contrast steps and no
    // edge bands. The fit must retain real support and label discarded scans.
    cv::Mat damaged(192,192,CV_8UC1,cv::Scalar(20));damaged(cv::Rect(0,96,192,96)).setTo(220);
    LineScanOptions scan;scan.edge.profile={.5,1,1};scan.edge.polarity=EdgePolarity::Rising;scan.edge.minContrast=10;
    for(int id:{4,10,17}){const int x=int(std::lround(95.2+118*(double(id)/20-.5)));damaged(cv::Rect(x-2,65,5,65)).setTo(20);damaged(cv::Rect(x-2,106,5,24)).setTo(220);}
    for(int id:{2,14}){const int x=int(std::lround(95.2+118*(double(id)/20-.5)));damaged(cv::Rect(x-2,65,5,65)).setTo(20);}
    auto damagedLine=LineFitter::find(frame(damaged),box(),scan).line;check(damagedLine.rejectedCount==3 && damagedLine.missingCount==2 && damagedLine.usedCount==16,"raster displaced and missing scans distinguished");near(damagedLine.origin.y(),95.5,.05,"raster real line retained");
    reject([&]{LineFitter::find(frame(cv::Mat(192,192,CV_8UC1,cv::Scalar(0))),box(),scan);});
    auto image=frame(damaged);reject([&]{LineFitter::find(image,box(),scan,stopped);});
    auto wrong=box();wrong.frame.id="other";reject([&]{LineFitter::find(image,wrong,scan);});wrong=box();wrong.geometry=ScanSegment{{1,1},{20,20},2};reject([&]{LineFitter::find(image,wrong,scan);});
    reject([&]{LineFitter::find(image,box(0,400,50),scan);});
    for(int i=0;i<5;++i){auto bad=scan;if(i==0)bad.scanCount=4097;if(i==1)bad.scanWidthPx=120;if(i==2)bad.edge.profile.stepPx=0;if(i==3){bad.scanCount=4096;bad.edge.profile.stepPx=.05;}if(i==4)bad.scanWidthPx=NAN;reject([&]{LineFitter::find(image,box(),bad);});}
    auto hashHeavy=scan;hashHeavy.scanCount=4096;reject([&]{LineFitter::find(frame(cv::Mat(512,512,CV_8UC1,cv::Scalar(0))),box(),hashHeavy);});
    auto tinyThreshold=LineFitOptions{};tinyThreshold.inlierDistance=1e-20;reject([&]{LineFitter::fit(samples(),{1,0},tinyThreshold);});
    reject([&]{LineFitter::fit(QVector<LineScanPoint>(4097),{1,0});});
    out<<checks<<" checks passed; raw pixel software corpus, physical/world/UI acceptance pending\n";
    if(argc==2){QFile file(QString::fromLocal8Bit(argv[1]));check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"report output");file.write(report.toUtf8());}
    QTextStream(stdout)<<report;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
