#include "vision/metrology/CircleFitter.hpp"
#include "application/RadialScanService.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <thread>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double e,const char* why){check(std::isfinite(a) && std::abs(a-b)<=e,why);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"expected circle rejection");}
double distance(QPointF a,QPointF b){return std::hypot(a.x()-b.x(),a.y()-b.y());}
QVector<CircleFitPoint> samples(QPointF c={12.3,-4.7},double radius=40,int count=72,double start=17.3,double span=360){
    QVector<CircleFitPoint> result;for(int i=0;i<count;++i){double a=(start+span*i/(std::abs(span)==360 ? count:count-1))*std::numbers::pi/180;result.append({c+QPointF(std::cos(a),std::sin(a))*radius});}return result;
}
QVector<CircleFitPoint> observations(const RadialSearch& s){QVector<CircleFitPoint> result;for(const auto& p:s.scans)result.append({p.point});return result;}
ImageFrame frame(const cv::Mat& m,QString coordinate="raw"){return ImageFrame::fromBuffer(m.data,m.rows*m.step,m.cols,m.rows,m.step,PixelFormat::Gray16,{"source","frame","cycle",coordinate,QDateTime::currentDateTimeUtc()},false);}
cv::Mat raster(QPointF c,double radius,bool dark,double sigma,bool noisy){cv::Mat m(192,192,CV_16UC1);
    for(int y=0;y<192;++y)for(int x=0;x<192;++x){const double d=distance({double(x),double(y)},c)-radius;unsigned h=(unsigned(x)*73856093u)^(unsigned(y)*19349663u)^1234567u;
        double v=.5+(dark ? -.4:.4)*std::erf(d/(std::sqrt(2.)*sigma))+(noisy ? (int(h%13)-6)/255.:0);m.at<quint16>(y,x)=quint16(std::lround(v*65535));}return m;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QString report;QTextStream out(&report);auto points=samples();auto r=CircleFitter::fit(points);
    near(distance(r.centre,{12.3,-4.7}),0,1e-9,"analytic centre");near(r.radius,40,1e-9,"analytic radius");near(r.rms,0,1e-9,"analytic residual");near(r.condition,2,1e-9,"full circle normal-matrix condition");near(r.angularSpanDegrees,355,1e-9,"observed span excludes seam interval");
    check(r.usedCount==72 && r.rejectedCount==0 && r.missingCount==0 && r.coverage==1,"full support");
    // An independently symmetric radial perturbation has exact optimum centre
    // and mean radius, nonzero geometric RMS, and a zero geometric gradient.
    points=samples({0,0},40,72,0);for(int i=0;i<72;++i)*points[i].point*=1+(i%2 ? -.2:.2)/40;
    CircleFitOptions noRansac;noRansac.ransac=false;r=CircleFitter::fit(points,noRansac);
    near(distance(r.centre,{0,0}),0,1e-9,"geometric optimum centre");near(r.radius,40,1e-9,"geometric mean radius rather than algebraic radius");near(r.rms,.2,1e-9,"analytic geometric RMS");near(r.maxResidual,.2,1e-9,"analytic max radial residual");
    auto strict=noRansac;strict.maxRms=.19;reject([&]{CircleFitter::fit(points,strict);});strict=noRansac;strict.maxResidual=.19;reject([&]{CircleFitter::fit(points,strict);});
    // Translation, radius scale, clockwise, cross-seam and partial support.
    for(auto c:{QPointF(0,0),QPointF(1e9,-1e9),QPointF(-240.1,301.7)})for(double radius:{.01,7.,10000.})for(double span:{360.,-360.,120.,-150.,270.}){
        auto o=CircleFitOptions{};o.inlierDistance=radius*.02;o.maxRms=radius*.01;o.maxResidual=radius*.02;
        auto fit=CircleFitter::fit(samples(c,radius,72,330,span),o);
        near(distance(fit.centre,c),0,2e-6,"normalized translated circle centre");near(fit.radius,radius,2e-6,"normalized translated circle radius");check(fit.usedCount==72,"exact arc support");
    }
    points=samples();for(int i:{3,12,30,51,65})points[i].point=*points[i].point+QPointF(5000,1000);
    points[0].point.reset();points[71].point.reset();const auto rng=cv::theRNG().state;r=CircleFitter::fit(points);
    check(cv::theRNG().state==rng,"local RNG leaves global state");check(r.usedCount==65 && r.rejectedCount==5 && r.missingCount==2,"missing/outlier distinct");near(r.radius,40,1e-8,"high leverage robust radius");near(distance(r.centre,{12.3,-4.7}),0,1e-8,"high leverage robust centre");
    for(int i:{3,12,30,51,65})check(!r.scans[i].used && r.scans[i].reason=="outlier" && *r.scans[i].signedResidual>1000,"outlier measurements retained");
    check(!r.scans[0].signedResidual && r.scans[0].reason=="not_found","missing has no invented residual");
    auto replay=CircleFitter::fit(points);check(r.centre==replay.centre && r.radius==replay.radius && r.rms==replay.rms,"deterministic fit");
    // A burr remains in the returned observations even though excluded from fit.
    points=samples({0,0},40,72,0);points[0].point=QPointF(45,0);r=CircleFitter::fit(points);near(r.radius,40,1e-8,"burr excluded from radius");near(*r.scans[0].signedResidual,5,1e-8,"burr residual not hidden");
    points=samples();std::reverse(points.begin(),points.end());r=CircleFitter::fit(points);near(r.radius,40,1e-8,"reversed ordering");
    // Irregular, noisy support: verify the geometric stationary equations
    // directly, using neither SVD nor another fitted expected circle.
    points=samples({2,-3},40,73,11,210);for(int i=0;i<points.size();++i)*points[i].point+=QPointF(.17*std::sin(i*1.7),.23*std::cos(i*.9));
    r=CircleFitter::fit(points,noRansac);QPointF gradient;double radialGradient=0;
    for(const auto& p:points){auto d=r.centre-*p.point;double len=distance(r.centre,*p.point),e=len-r.radius;gradient+=d*(e/len);radialGradient+=e;}
    near(distance(gradient,{}),0,1e-5,"geometric centre stationary equations");near(radialGradient,0,1e-5,"geometric radius stationary equation");
    points=samples({0,0},40,72,0);for(int i=0;i<points.size();++i)*points[i].point*=1+.8*std::cos(i*6*std::numbers::pi/72)/40;
    strict=noRansac;strict.inlierDistance=10;reject([&]{CircleFitter::fit(points,strict);});
    points=samples({0,0},40);for(auto& p:points)p.point->setX(p.point->x()*1.2);reject([&]{CircleFitter::fit(points,strict);});
    reject([&]{CircleFitter::fit(samples({},40,72,0,10));});
    strict={};strict.minAngularSpanDegrees=1;strict.maxCondition=1000;reject([&]{CircleFitter::fit(samples({},40,72,0,30),strict);});
    strict={};strict.minAngularSpanDegrees=180;reject([&]{CircleFitter::fit(samples({},40,72,0,120),strict);});
    strict={};strict.maxCondition=1.5;reject([&]{CircleFitter::fit(samples(),strict);});
    points.clear();for(int i=0;i<72;++i)points.append({QPointF(i,3*i+2)});reject([&]{CircleFitter::fit(points);});
    points=samples();points[1].point=points[0].point;reject([&]{CircleFitter::fit(points);});
    points=samples();points[1].point=QPointF(NAN,1);reject([&]{CircleFitter::fit(points);});
    points=samples();points[1].point=QPointF(1,INFINITY);reject([&]{CircleFitter::fit(points);});
    points=samples();points[1].point=QPointF(1e13,0);reject([&]{CircleFitter::fit(points);});
    points=samples();for(int i=5;i<72;++i)points[i].point.reset();reject([&]{CircleFitter::fit(points);});
    points=samples();for(int i=0;i<72;i+=2)points[i].point.reset();reject([&]{CircleFitter::fit(points);});
    reject([&]{CircleFitter::fit(QVector<CircleFitPoint>(4097));});reject([&]{CircleFitter::fit(samples({},1e-14));});
    for(int i=0;i<13;++i){strict={};if(i==0)strict.minPoints=3;if(i==1)strict.minPoints=4097;if(i==2)strict.minCoverage=0;if(i==3)strict.minCoverage=1.01;if(i==4)strict.minAngularSpanDegrees=360;if(i==5)strict.minAngularSpanDegrees=NAN;if(i==6)strict.maxCondition=1;if(i==7)strict.inlierDistance=0;if(i==8)strict.maxRms=INFINITY;if(i==9)strict.maxResidual=-1;if(i==10)strict.ransacIterations=0;if(i==11)strict.ransacIterations=1025;if(i==12)strict.inlierDistance=1e-20;reject([&]{CircleFitter::fit(samples(),strict);});}
    auto stopped=std::make_shared<std::atomic_bool>(true);reject([&]{CircleFitter::fit(samples(),{},stopped);});
    auto cancel=std::make_shared<std::atomic_bool>(false);auto large=samples({},40,4096);auto longFit=CircleFitOptions{};longFit.ransacIterations=1024;
    std::jthread stopper([cancel]{std::this_thread::sleep_for(std::chrono::milliseconds(5));cancel->store(true);});
    bool cancelled=false;try{CircleFitter::fit(large,longFit,cancel);}catch(const std::exception& e){cancelled=QString::fromUtf8(e.what())=="Circle fit cancelled";}check(cancelled,"cooperative cancellation during fit");
    double diameterSum=0,diameterMax=0,centreMax=0,noisyMax=0;int count=0;
    for(double radius:{24.,40.,60.})for(QPointF c:{QPointF(95.2,96.35),QPointF(96.57,95.81)})for(bool dark:{false,true})for(bool inward:{false,true}){
        RadialScanOptions scan;scan.edge.profile={.5,1,1};scan.edge.minContrast=10;
        scan.direction=inward ? RadialDirection::Inward:RadialDirection::Outward;
        scan.edge.polarity=(dark!=inward) ? EdgePolarity::Falling:EdgePolarity::Rising;
        ToolRoi roi{{"raw"},Annulus{c,radius-12,radius+12}};
        auto image=frame(raster(c,radius,dark,1.4,false));auto hash=image.contentId();auto search=RadialScanner::find(image,roi,scan);auto fit=CircleFitter::fit(observations(search));
        const double e=2*(fit.radius-radius),ce=distance(fit.centre,c);diameterSum+=e*e;diameterMax=std::max(diameterMax,std::abs(e));centreMax=std::max(centreMax,ce);++count;
        near(e,0,.30,"independent raster diameter gate");near(ce,0,.15,"independent raster centre gate");check(fit.usedCount==72 && image.contentId()==hash && search.inputContentId==hash,"raster support/hash preserved");
        scan.scanWidth=4;auto noisy=CircleFitter::fit(observations(RadialScanner::find(frame(raster(c,radius,dark,2.6,true)),roi,scan)));
        noisyMax=std::max(noisyMax,std::abs(2*(noisy.radius-radius)));near(2*(noisy.radius-radius),0,.6,"separate degraded diameter gate");
    }
    near(std::sqrt(diameterSum/count),0,.15,"raster diameter RMS gate");
    out<<"clean fixtures="<<count<<" Gray16 erf sigma=1.4 width=1 step=.5 smoothing=1 diameter RMS="<<std::sqrt(diameterSum/count)<<" max="<<diameterMax<<" px centre max="<<centreMax<<" px\n";
    out<<"degraded fixtures="<<count<<" sigma=2.6 noise=+/-6 seed=1234567 width=4 diameter max="<<noisyMax<<" px\n";
    // Calibrated non-affine raster: independent analytic projection/inverse,
    // accepted holdouts and T6.1 service. Fit nominal world observations, never
    // transform a raw fitted centre or multiply raw radius by a scale.
    auto project=[](QPointF q){double w=1+.004*q.x()+.002*q.y();return QPointF((3.5*q.x()+.4*q.y()+20)/w,(.2*q.x()+4*q.y()+20)/w);};
    auto unproject=[](QPointF p){double a=3.5-.004*p.x(),b=.4-.002*p.x(),c=.2-.004*p.y(),d=4-.002*p.y(),det=a*d-b*c;return QPointF(((p.x()-20)*d-b*(p.y()-20))/det,(a*(p.y()-20)-c*(p.x()-20))/det);};
    ChessboardObservation observation{{"circle-projective",7,7,5},{"source","fit-frame","fit-cycle","image",QDateTime::currentDateTimeUtc()},{192,192},{},{}};
    for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);observation.worldMillimetres.append(q);observation.pixels.append(project(q));}
    auto mapping=PlaneMappingService::fit(observation);MeasurementService service;
    CalibrationDependency setup{"circle",1,"pending","source","lens",{192,192},"plane",{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
    auto holdout=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{project(a),setup.source},{project(b),setup.source},distance(a,b)};};
    auto accepted=service.validate(service.teachHomography(setup,mapping),{holdout("hx",{2,3},{27,3}),holdout("hy",{3,2},{3,28})},.0001);
    cv::Mat perspective(192,192,CV_16UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){double d=distance(unproject({double(x),double(y)}),{15,15})-7;perspective.at<quint16>(y,x)=quint16(std::lround((.5+.4*std::erf(d/(std::sqrt(2.)*.4)))*65535));}
    RadialScanOptions scan;scan.unitsPerSample=.25;scan.scanWidth=.25;scan.edge.profile={.5,1,1};scan.edge.minContrast=10;scan.edge.polarity=EdgePolarity::Rising;
    RadialScanCalibration context{accepted,accepted.dependency()};ArcRoi roi{{{15,15},3,11},17.3,360};auto image=frame(perspective,"image");auto search=RadialScanService::scan(image,roi,scan,context);
    auto worldOptions=CircleFitOptions{};worldOptions.inlierDistance=.1;worldOptions.maxRms=.03;worldOptions.maxResidual=.1;
    auto world=CircleFitter::fit(observations(search.search),worldOptions);
    near(distance(world.centre,{15,15}),0,.03,"projective calibrated centre truth");near(2*world.radius,14,.06,"projective calibrated diameter truth");check(world.usedCount==72,"projective all scans used");
    out<<"non-affine calibrated Gray16 world circle diameter error="<<2*(world.radius-7)<<" mm centre error="<<distance(world.centre,{15,15})<<" mm\n";
    auto raw=observations(search.search);for(int i=0;i<raw.size();++i)raw[i].point=search.search.scans[i].rawPoint;
    auto permissive=CircleFitOptions{};permissive.inlierDistance=10;permissive.maxResidual=10;permissive.maxRms=10;
    auto rawFit=CircleFitter::fit(raw,permissive);check(distance(unproject(rawFit.centre),world.centre)>.05,"mapping a raw circle centre is a negative control");
    auto stale=context;++stale.current.version;reject([&]{RadialScanService::scan(image,roi,scan,stale);});
    out<<checks<<" checks; software core acceptance only; full-circle defect quality/UI/physical acceptance pending\n";
    if(argc>1){QFile file(QString::fromLocal8Bit(argv[1]));check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"report output");file.write(report.toUtf8());}
    QTextStream(stdout)<<report;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
