#include "application/EdgePairService.hpp"
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
template<class F>void reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"expected pair rejection");}
ImageFrame frame(const cv::Mat& m,QString source="source",QString coordinate="raw") {
    return ImageFrame::fromBuffer(m.data,m.rows*m.step,m.cols,m.rows,m.step,m.depth()==CV_16U ? PixelFormat::Gray16:PixelFormat::Gray8,
        {source,"frame","cycle",coordinate,QDateTime::currentDateTimeUtc()},false);
}
ToolRoi roi(QPointF a,QPointF b,double width=2){return {{"raw"},ScanSegment{a,b,width}};}
ImageFrame row(const QVector<int>& values){cv::Mat m(5,values.size(),CV_8UC1);for(int y=0;y<5;++y)for(int x=0;x<m.cols;++x)m.at<uchar>(y,x)=uchar(values[x]);return frame(m);}
EdgePairSearch find(const QVector<int>& values,const EdgePairOptions& o){return EdgePairFinder::find(row(values),roi({0,2},{double(values.size()-1),2}),o);}
const EdgePairCandidate& selected(const EdgePairSearch& r){check(bool(r.selected),"pair found");return r.candidates[*r.selected];}
void emptyError(const EdgePairResult& r){check(r.state==ExecutionState::Error && !r.found && !r.first && !r.second && !r.centre && !r.widthPx && !r.widthMm && !r.calibration && !r.error.isEmpty(),"error leaves no metric geometry");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    EdgePairOptions o;o.minContrast=10;
    const QVector<int> stripes{0,0,40,40,40,0,0,0,100,100,100,100,0,0};
    auto r=find(stripes,o);auto c=selected(r);near(c.widthPx,4,1e-12,"stronger complete second stripe");near(r.edges.candidates[c.firstEdge].point.x(),7.5,1e-12,"strongest pair start");
    o.selection=EdgeSelection::First;r=find(stripes,o);c=selected(r);near(c.widthPx,3,1e-12,"first stripe width");
    o.selection=EdgeSelection::Last;r=find(stripes,o);near(selected(r).widthPx,4,1e-12,"last stripe");
    o.polarity=StripePolarity::Dark;r=find(stripes,o);c=selected(r);near(c.widthPx,3,1e-12,"dark gap width");near(r.edges.candidates[c.firstEdge].point.x(),4.5,1e-12,"dark starts falling");
    o.polarity=StripePolarity::Any;r=find(stripes,o);near(selected(r).widthPx,4,1e-12,"any stripe polarity");
    o={};o.minContrast=10;o.minWidthPx=4;o.maxWidthPx=4;r=find(stripes,o);near(selected(r).widthPx,4,1e-12,"inclusive exact width");
    o.minWidthPx=std::nextafter(4.,5.);o.maxWidthPx=5;r=find(stripes,o);check(!r.selected,"nextafter lower excludes width");
    o.minWidthPx=3.5;o.maxWidthPx=std::nextafter(4.,3.);r=find(stripes,o);check(!r.selected,"nextafter upper excludes width");
    o={};o.minContrast=10;r=find({0,0,50,50,0,0,50,50,0,0},o);c=selected(r);near(r.edges.candidates[c.firstEdge].point.x(),1.5,1e-12,"strongest pair tie first");
    // Two individually strongest edges belong to different objects. No crossing.
    r=find({0,0,100,100,80,80,180,180,0,0},o);c=selected(r);near(r.edges.candidates[c.firstEdge].point.x(),5.5,1e-12,"complete pair beats independent strongest endpoints");near(c.widthPx,2,1e-12,"pair cannot bridge objects");
    o.minContrast=30;r=find({0,0,100,100,80,80,180,180,0,0},o);check(!r.edges.candidates[1].accepted && r.edges.candidates[1].reason=="min_contrast","weak endpoint diagnostic reason");near(selected(r).widthPx,2,1e-12,"accepted intermediate edge cannot be skipped");
    o.minWidthPx=3;o.maxWidthPx=7;r=find({0,0,100,100,80,80,180,180,0,0},o);check(!r.selected,"width cannot force cross-object pair");
    o={};r=find({0,0,0,0,0},o);check(!r.selected && r.candidates.isEmpty(),"flat no pair");r=find({0,0,100,100,100},o);check(!r.selected,"single edge no pair");
    r=find({0,100,100,0,0},o);check(!r.selected && r.edges.candidates[0].reason=="border_support","border edge cannot form stripe");
    o.polarity=StripePolarity::Dark;r=find({0,0,100,100,0,0},o);check(!r.selected && r.candidates[0].reason=="polarity","wrong stripe polarity");
    o={};for(int i=0;i<7;++i){auto bad=o;if(i==0)bad.minWidthPx=-1;if(i==1)bad.maxWidthPx=0;if(i==2)bad.minWidthPx=bad.maxWidthPx+1;if(i==3)bad.maxWidthPx=INFINITY;if(i==4)bad.minWidthPx=NAN;if(i==5)bad.polarity=StripePolarity(99);if(i==6)bad.selection=EdgeSelection(99);reject([&]{find(stripes,bad);});}
    auto f=row(stripes);reject([&]{EdgePairFinder::find(f,roi({0,2},{0,2}),o);});reject([&]{EdgePairFinder::find(f,roi({-1,2},{12,2}),o);});
    auto wrong=roi({0,2},{12,2});wrong.frame.id="other";reject([&]{EdgePairFinder::find(f,wrong,o);});
    auto stopped=std::make_shared<std::atomic_bool>(true);reject([&]{EdgePairFinder::find(f,roi({0,2},{12,2}),o,stopped);});
    emptyError(EdgePairService::measure(f,roi({0,2},{12,2}),o,EdgePairMode::PixelDebug,nullptr,stopped));
    QString report;QTextStream out(&report);double sum=0,maxError=0,degradedSum=0,degradedMax=0;int count=0;
    // Independently rasterized erf stripe; analytic endpoints are +/-12px.
    for(double angle:{0.,45.,90.,135.,17.3,78.1,167.2})for(double phase:{-.37,0.,.29})for(bool dark:{false,true}) {
        const double a=angle*std::numbers::pi/180;const QPointF d(std::cos(a),std::sin(a)),centre(63.2,64.35);const QPointF mid=centre+d*phase;
        auto raster=[&](double sigma,bool noise){cv::Mat m(128,128,CV_16UC1);for(int y=0;y<128;++y)for(int x=0;x<128;++x){const double s=(x-mid.x())*d.x()+(y-mid.y())*d.y();const double stripe=.5*(std::erf((s+12)/(std::sqrt(2.)*sigma))-std::erf((s-12)/(std::sqrt(2.)*sigma)));const unsigned h=(unsigned(x)*73856093u)^(unsigned(y)*19349663u)^1234567u;const double n=noise ? (int(h%13)-6)/255.:0;const double v=.1+.8*(dark ? 1-stripe:stripe)+n;m.at<quint16>(y,x)=quint16(std::lround(v*65535));}return m;};
        o={};o.profile={.5,1,1};o.polarity=dark ? StripePolarity::Dark:StripePolarity::Bright;o.minContrast=10;o.minWidthPx=20;o.maxWidthPx=28;
        const auto scan=roi(centre-d*40,centre+d*40,4);const auto image=frame(raster(1.4,false));const auto hash=image.contentId();const auto result=EdgePairService::measure(image,scan,o,EdgePairMode::PixelDebug);
        if(!result.found){const auto diagnostic=EdgePairFinder::find(image,scan,o);QTextStream err(stderr);err<<"angle="<<angle<<" phase="<<phase<<" dark="<<dark<<" error="<<result.error<<" edges="<<diagnostic.edges.candidates.size()<<"\n";for(const auto& e:diagnostic.edges.candidates)err<<e.distancePx<<" "<<e.signedStrength<<" "<<e.reason<<"\n";for(const auto& pair:diagnostic.candidates)err<<"pair "<<pair.widthPx<<" "<<pair.reason<<"\n";}
        check(result.state==ExecutionState::Done && result.found && !result.widthMm && !result.calibration,"pixel debug real geometry only");
        near(QLineF(*result.first,mid-d*12).length(),0,.30,"first analytic endpoint");near(QLineF(*result.second,mid+d*12).length(),0,.30,"second analytic endpoint");near(QLineF(*result.centre,mid).length(),0,.30,"analytic centre");
        const double error=std::abs(*result.widthPx-24);sum+=error*error;maxError=std::max(maxError,error);++count;near(error,0,.30,"clean width gate");check(image.contentId()==hash,"raw immutable");
        const auto reverse=EdgePairService::measure(image,roi(centre+d*40,centre-d*40,4),o,EdgePairMode::PixelDebug);check(reverse.found,"reversed stripe polarity invariant");near(*reverse.widthPx,*result.widthPx,1e-8,"reverse width");near(QLineF(*reverse.first,*result.second).length(),0,1e-8,"reverse endpoints swapped");
        const auto noisy=EdgePairService::measure(frame(raster(2.6,true)),scan,o,EdgePairMode::PixelDebug);check(noisy.found,"degraded stripe found");const double e=std::abs(*noisy.widthPx-24);degradedSum+=e*e;degradedMax=std::max(degradedMax,e);near(e,0,.6,"separate degraded width gate");
        out<<"angle="<<angle<<" phase="<<phase<<" dark="<<dark<<" widthErrorPx="<<error<<" degraded="<<e<<"\n";
    }
    check(std::sqrt(sum/count)<=.15,"clean width RMS gate");out<<"clean count="<<count<<" RMS="<<std::sqrt(sum/count)<<" max="<<maxError<<" px\ndegraded count="<<count<<" sigma=2.6 noise=+/-6 seed=1234567 RMS="<<std::sqrt(degradedSum/count)<<" max="<<degradedMax<<" px\n";
    // Existing independently validated calibration and actual frame provenance.
    MeasurementService service;CalibrationDependency setup{"pair-scale",1,"pending","source","lens",{128,128},"plane",{"raw",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYUp}};
    auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,setup.source},{b,setup.source},mm};};
    const auto candidate=service.teach(setup,sample("teach",{0,0},{100,0},10));const auto accepted=service.validate(candidate,{sample("h1",{4,20},{104,20},10),sample("h2",{30,4},{30,104},10)},.001);
    cv::Mat stripe(128,128,CV_8UC1,cv::Scalar(0));stripe(cv::Rect(40,0,24,128)).setTo(200);auto image=frame(stripe);const auto scan=roi({10,64},{110,64});o={};o.minContrast=10;
    EdgePairCalibration context{accepted,accepted.dependency()};auto result=EdgePairService::measure(image,scan,o,EdgePairMode::Calibrated,&context);
    check(result.found && result.calibration==accepted.dependency(),"calibrated result dependency");near(*result.widthMm,2.4,1e-12,"24px to 2.4mm");
    emptyError(EdgePairService::measure(image,scan,o,EdgePairMode::Calibrated));
    for(int i=0;i<6;++i){auto bad=context;if(i==0)bad.calibration=candidate;if(i==1)++bad.current.version;if(i==2)bad.current.cameraId="other";if(i==3)bad.current.resolution={64,64};if(i==4)bad.current.source.id="other";if(i==5)bad.current.contentId="other";emptyError(EdgePairService::measure(image,scan,o,EdgePairMode::Calibrated,&bad));}
    emptyError(EdgePairService::measure(frame(stripe,"foreign"),scan,o,EdgePairMode::Calibrated,&context));
    auto none=EdgePairService::measure(frame(cv::Mat(128,128,CV_8UC1,cv::Scalar(0))),scan,o,EdgePairMode::Calibrated,&context);check(none.state==ExecutionState::Done && !none.found && !none.widthPx && !none.widthMm && !none.first,"no found has no metrics");
    // Analytic projective world->raw raster geometry; endpoint mapping is not
    // equivalent to multiplying the width by one nominal scale.
    auto project=[](QPointF q){const double w=1+.01*q.x();return QPointF((4*q.x()+20)/w,(4*q.y()+20)/w);};
    auto unproject=[](QPointF p){const double x=(p.x()-20)/(4-.01*p.x());return QPointF(x,(p.y()*(1+.01*x)-20)/4);};
    ChessboardObservation observation{{"pair-projective",7,6,5},{"source","fit-frame","fit-cycle","image",QDateTime::currentDateTimeUtc()},{128,128},{},{}};
    for(int y=0;y<6;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);observation.worldMillimetres.append(q);observation.pixels.append(project(q));}
    auto hSetup=setup;hSetup.source.id="image";const auto mapping=PlaneMappingService::fit(observation);
    auto holdout=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{project(a),hSetup.source},{project(b),hSetup.source},QLineF(a,b).length()};};
    const auto hAccepted=service.validate(service.teachHomography(hSetup,mapping),{holdout("hx",{2,3},{27,3}),holdout("hy",{3,2},{3,23})},.0001);
    EdgePairCalibration hContext{hAccepted,hAccepted.dependency()};auto hScan=scan;hScan.frame.id="image";const auto hImage=frame(stripe,"source","image");
    result=EdgePairService::measure(hImage,hScan,o,EdgePairMode::Calibrated,&hContext);check(result.found && result.widthMm.has_value(),"projective pair measured");
    const double expected=QLineF(unproject({39.5,64}),unproject({63.5,64})).length();near(*result.widthMm,expected,1e-5,"independent projective endpoint oracle");check(std::abs(expected-24./4)>.5,"nominal scale negative control");
    cv::Mat outside(128,128,CV_8UC1,cv::Scalar(0));outside(cv::Rect(5,0,24,128)).setTo(200);auto outsideScan=roi({1,64},{110,64});outsideScan.frame.id="image";
    emptyError(EdgePairService::measure(frame(outside,"source","image"),outsideScan,o,EdgePairMode::Calibrated,&hContext));
    auto invalidContext=context;++invalidContext.current.version;emptyError(EdgePairService::measure(frame(cv::Mat(128,128,CV_8UC1,cv::Scalar(0))),scan,o,EdgePairMode::Calibrated,&invalidContext));
    const auto manual=service.validate(service.manualScale(setup,10,{"acquired","source","teaching-frame",{128,128}}),{sample("mx",{4,20},{104,20},10),sample("my",{30,4},{30,104},10)},.001);
    EdgePairCalibration manualContext{manual,manual.dependency()};result=EdgePairService::measure(image,scan,o,EdgePairMode::Calibrated,&manualContext);check(result.found,"manual pair");near(*result.widthMm,2.4,1e-12,"manual px/mm");
    auto fileSetup=setup;fileSetup.cameraId="calibration-test:pair";
    const auto fileCalibration=service.validate(service.manualScale(fileSetup,10,{"file_test",fileSetup.cameraId,"teaching-frame",{128,128}}),{sample("fx",{4,20},{104,20},10),sample("fy",{30,4},{30,104},10)},.001);
    EdgePairCalibration fileContext{fileCalibration,fileCalibration.dependency()};emptyError(EdgePairService::measure(frame(stripe,fileSetup.cameraId),scan,o,EdgePairMode::Calibrated,&fileContext));
    out<<checks<<" checks passed; software corpus, no physical acceptance\n";
    if(argc==2){QFile file(QString::fromLocal8Bit(argv[1]));check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"report output");file.write(report.toUtf8());}
    QTextStream(stdout)<<report;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
