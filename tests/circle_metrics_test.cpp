#include "vision/metrology/CircleMetrics.hpp"
#include "application/RadialScanService.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <chrono>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <thread>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double e,const char* why){if(!std::isfinite(a) || std::abs(a-b)>e)QTextStream(stderr)<<why<<" actual="<<QString::number(a,'g',17)<<" expected="<<QString::number(b,'g',17)<<" limit="<<e<<'\n';check(std::isfinite(a) && std::abs(a-b)<=e,why);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"expected metric rejection");}
RadialSearch ring(int count=72,double sweep=360,QPointF centre={},double radius=40,double start=0){RadialSearch s;s.fullCircle=std::abs(sweep)==360;
    for(int i=0;i<count;++i){double angle=start+sweep*i/(s.fullCircle ? count:count-1),a=angle*std::numbers::pi/180;RadialScanPoint p;p.scanId=i;p.angleDegrees=angle;p.point=centre+QPointF(std::cos(a),std::sin(a))*radius;s.scans.append(p);}return s;
}
FittedCircle fit(const RadialSearch& s,CircleFitOptions o={}){QVector<CircleFitPoint> p;for(auto v:s.scans)p.append({v.point});return CircleFitter::fit(p,o);}
void radial(RadialSearch& s,int i,double radius){double a=s.scans[i].angleDegrees*std::numbers::pi/180;s.scans[i].point=QPointF(std::cos(a),std::sin(a))*radius;}
ImageFrame frame(const cv::Mat& m,QString coordinate="raw"){return ImageFrame::fromBuffer(m.data,m.rows*m.step,m.cols,m.rows,m.step,PixelFormat::Gray16,{"source","frame","cycle",coordinate,QDateTime::currentDateTimeUtc()},false);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QString report;QTextStream out(&report);auto search=ring();auto fitted=fit(search);auto r=CircleMetrics::measure(search,fitted);
    check(r.scope==CircleMetricScope::FullCircle && r.deviation && r.contour.metrics && r.qualityReasons.isEmpty(),"qualified complete ring");
    near(r.radius,40,1e-9,"radius");near(r.diameter,80,1e-9,"diameter");near(r.deviation->peakToValley,0,1e-9,"ideal radial deviation");near(r.coverage,1,0,"coverage");near(r.maxGapDegrees,5,1e-12,"seam gap");
    const double regularIndex=std::numbers::pi/(72*std::tan(std::numbers::pi/72));
    near(r.contour.metrics->circularityIndex,regularIndex,1e-12,"regular polygon index independent analytic oracle");check(r.contour.metrics->circularityIndex<1,"polygon not fabricated ideal circle");
    near(r.contour.metrics->area,72*1600*std::sin(2*std::numbers::pi/72)/2,1e-8,"regular polygon area");
    near(r.contour.metrics->perimeter,72*80*std::sin(std::numbers::pi/72),1e-8,"regular polygon perimeter");
    for(int i=0;i<72;++i)radial(search,i,40+(i%2 ? -.2:.2));r=CircleMetrics::measure(search,fit(search));
    near(r.fitRms,.2,1e-9,"symmetric fit RMS");near(r.deviation->minimumRadius,39.8,1e-9,"minimum radius");near(r.deviation->maximumRadius,40.2,1e-9,"maximum radius");near(r.deviation->peakToValley,.4,1e-9,"all-sample range");near(r.deviation->maxInward,.2,1e-9,"inward sign");near(r.deviation->maxOutward,.2,1e-9,"outward sign");
    search=ring();radial(search,0,45);radial(search,36,37);fitted=fit(search);r=CircleMetrics::measure(search,fitted);
    check(r.fitUsedCount==70 && r.fitRejectedCount==2 && r.foundCount==72 && r.missingCount==0,"outliers remain found for coverage");
    near(r.fitRms,0,1e-8,"robust fit clean RMS");near(r.deviation->peakToValley,8,1e-8,"burr/chip not removed from defect metric");near(r.deviation->maxInward,3,1e-8,"chip retained");near(r.deviation->maxOutward,5,1e-8,"burr retained");
    check(!r.samples[0].fitUsed && r.samples[0].point && r.samples[0].signedResidual,"outlier sample evidence retained");near(*r.samples[0].signedResidual,5,1e-8,"signed outlier residual");
    // Polar area and cosine-law edge lengths provide an independent polygon
    // oracle for the actual damaged contour, rather than the fitted circle.
    double area=0,perimeter=0,delta=2*std::numbers::pi/72;
    for(int i=0;i<72;++i){auto radius=[](int k){return k==0 ? 45.:(k==36 ? 37.:40.);};double a=radius(i),b=radius((i+1)%72);area+=a*b*std::sin(delta)/2;perimeter+=std::sqrt(a*a+b*b-2*a*b*std::cos(delta));}
    near(r.contour.metrics->circularityIndex,4*std::numbers::pi*area/(perimeter*perimeter),1e-12,"actual damaged polygon circularity");check(r.contour.metrics->circularityIndex<regularIndex-.01,"burr reduces actual circularity");
    search=ring();for(int i=0;i<72;++i)radial(search,i,40+.8*std::cos(i*6*std::numbers::pi/72));
    auto permissive=CircleFitOptions{};permissive.ransac=false;permissive.inlierDistance=10;permissive.maxRms=10;permissive.maxResidual=10;
    r=CircleMetrics::measure(search,fit(search,permissive));near(r.deviation->peakToValley,1.6,1e-8,"lobed radial range");
    search=ring();double meanRadius=0;for(int i=0;i<72;++i){double a=i*2*std::numbers::pi/72,rad=1/std::sqrt(std::pow(std::cos(a)/48,2)+std::pow(std::sin(a)/40,2));radial(search,i,rad);meanRadius+=rad/72;}
    // T6.2's bounded LM convergence is numerical, not exact symbolic equality.
    // This 1e-6-unit oracle tolerance is separate from raster/quality gates.
    r=CircleMetrics::measure(search,fit(search,permissive));near(r.radius,meanRadius,1e-6,"ellipse geometric mean radius");near(r.deviation->peakToValley,8,1e-8,"ellipse not hidden by fitted circle");
    search=ring();search.scans[0].point.reset();r=CircleMetrics::measure(search,fit(search));
    check(r.deviation && !r.contour.metrics && r.contour.unavailableReason=="missing_contour_samples","small hole never auto-closes polygon");near(r.maxGapDegrees,10,1e-12,"missing seam gap");check(!r.samples[0].radialDistance && !r.samples[0].signedResidual,"missing not interpolated");
    for(int i=1;i<8;++i)search.scans[i].point.reset();r=CircleMetrics::measure(search,fit(search));
    check(!r.deviation && r.scope==CircleMetricScope::Unavailable && r.qualityReasons.contains("insufficient_scan_coverage") && r.qualityReasons.contains("excessive_nominal_gap"),"failed coverage and gap invalidate qualified metric");
    near(r.observedDeviation.peakToValley,0,1e-8,"observed diagnostics remain separate");check(r.radius>0 && r.fitUsedCount==64 && !r.contour.metrics,"fit remains diagnostic on quality failure");near(r.maxGapDegrees,45,1e-12,"wrap missing wedge");
    // Inclusive boundaries are checked against the same measured double, then
    // nextafter tests the adjacent excluded representable value.
    auto gate=CircleQualityOptions{};gate.minCoverage=r.coverage;gate.maxGapDegrees=r.maxGapDegrees;
    check(CircleMetrics::measure(search,fit(search),gate).deviation.has_value(),"inclusive quality boundaries");
    auto badGate=gate;badGate.minCoverage=std::nextafter(gate.minCoverage,1.);check(!CircleMetrics::measure(search,fit(search),badGate).deviation,"nextafter coverage outside");
    badGate=gate;badGate.maxGapDegrees=std::nextafter(gate.maxGapDegrees,0.);check(!CircleMetrics::measure(search,fit(search),badGate).deviation,"nextafter gap outside");
    gate={};gate.minFoundPoints=73;check(CircleMetrics::measure(ring(),fit(ring()),gate).qualityReasons.contains("insufficient_found_points"),"quality min found gate");
    for(double sweep:{180.,-180.,270.,-270.}){search=ring(72,sweep,{},40,330);r=CircleMetrics::measure(search,fit(search));
        check(r.scope==CircleMetricScope::ObservedArc && r.deviation && !r.contour.metrics && r.contour.unavailableReason=="open_arc","arc scope only");near(r.maxGapDegrees,std::abs(sweep)/71,1e-10,"arc has no invented closing gap");
        search.scans[0].point.reset();search.scans[1].point.reset();r=CircleMetrics::measure(search,fit(search));near(r.maxGapDegrees,2*std::abs(sweep)/71,1e-10,"arc missing boundary gap");}
    for(double sweep:{360.,-360.}){search=ring(72,sweep,{},40,330);r=CircleMetrics::measure(search,fit(search));near(r.contour.metrics->circularityIndex,regularIndex,1e-12,"signed cross-seam polygon");}
    search=ring();std::swap(search.scans[0].point,search.scans[36].point);r=CircleMetrics::measure(search,fit(search));
    check(r.deviation && !r.contour.metrics && r.contour.unavailableReason=="self_intersecting_contour","topology invalidates circularity independently of radial metric");
    // Standalone polygon: exact square/triangle, translation/scale/winding,
    // near contact, crossing, overlap, duplicate closure and degenerate cases.
    for(double scale:{.01,1.,10000.})for(auto offset:{QPointF(0,0),QPointF(1e9,-1e9)})for(bool reverse:{false,true}){
        QVector<QPointF> polygon{{0,0},{2,0},{2,2},{0,2}};for(auto& p:polygon)p=offset+p*scale;if(reverse)std::reverse(polygon.begin(),polygon.end());auto c=CircleMetrics::closedContour(polygon);
        check(c.metrics.has_value(),"square valid");near(c.metrics->circularityIndex,std::numbers::pi/4,1e-10,"square analytic index");near(c.metrics->area,4*scale*scale,std::max(1e-7,scale*scale*1e-5),"square area unit squared");near(c.metrics->perimeter,8*scale,std::max(1e-6,scale*1e-5),"square perimeter units");}
    auto triangle=CircleMetrics::closedContour({{0,0},{2,0},{1,std::sqrt(3.)}});near(triangle.metrics->circularityIndex,std::numbers::pi*std::sqrt(3.)/9,1e-12,"equilateral triangle index");
    for(QVector<QPointF> polygon:{QVector<QPointF>{},{{0,0},{1,1}},{{0,0},{1,0},{2,0}},{{0,0},{2,2},{0,2},{2,0}},{{0,0},{1,0},{1,1},{0,0}},{{0,0},{2,0},{1,0},{1,1}},{{0,0},{2,0},{2,2},{1,0},{0,2}}})
        check(!CircleMetrics::closedContour(polygon).metrics,"invalid contour unavailable");
    reject([&]{CircleMetrics::closedContour({{0,0},{NAN,1},{1,0}});});reject([&]{CircleMetrics::closedContour(QVector<QPointF>(4097));});
    check(!CircleMetrics::closedContour({{0,0},{2,0},{2,2},{1,1e-14},{0,2}}).metrics,"near contact conservative ret");
    check(CircleMetrics::closedContour({{0,0},{1,0},{2,0},{2,2},{0,2}}).metrics.has_value(),"forward collinear adjacent edges allowed");
    search=ring();fitted=fit(search);search.coverage=0;search.maxGapDegrees=999;search.foundCount=0;search.missingCount=72;fitted.rms=123;fitted.scans[0].signedResidual=999;
    r=CircleMetrics::measure(search,fitted);near(r.coverage,1,0,"coverage recomputed from slots");near(r.maxGapDegrees,5,1e-12,"gap recomputed");near(r.fitRms,0,1e-8,"fit RMS recomputed");near(*r.samples[0].signedResidual,0,1e-8,"residual recomputed");
    auto subtlyDifferent=search;subtlyDifferent.scans[0].point->setX(std::nextafter(subtlyDifferent.scans[0].point->x(),INFINITY));
    reject([&]{CircleMetrics::measure(subtlyDifferent,fitted);});
    for(int i=0;i<7;++i){auto s=search;auto f=fitted;if(i==0)s.scans[4].point=QPointF(1,2);if(i==1)s.scans[4].scanId=7;if(i==2)s.scans[4].angleDegrees+=.1;if(i==3)for(auto& p:s.scans)p.angleDegrees*=.5;if(i==4)f.radius=NAN;if(i==5)f.scans[0].point.reset();if(i==6)f.scans.resize(3);reject([&]{CircleMetrics::measure(s,f);});}
    for(int i=0;i<7;++i){gate={};if(i==0)gate.minCoverage=0;if(i==1)gate.minCoverage=1.1;if(i==2)gate.maxGapDegrees=0;if(i==3)gate.maxGapDegrees=NAN;if(i==4)gate.maxGapDegrees=361;if(i==5)gate.minFoundPoints=5;if(i==6)gate.minFoundPoints=4097;reject([&]{CircleMetrics::measure(search,fitted,gate);});}
    auto stopped=std::make_shared<std::atomic_bool>(true);reject([&]{CircleMetrics::measure(search,fitted,{},stopped);});reject([&]{CircleMetrics::closedContour({{0,0},{1,0},{0,1}},stopped);});
    QVector<QPointF> huge;for(const auto& p:ring(4096).scans)huge.append(*p.point);auto cancel=std::make_shared<std::atomic_bool>(false);
    std::jthread stopper([cancel]{std::this_thread::sleep_for(std::chrono::milliseconds(5));cancel->store(true);});bool cancelled=false;
    try{CircleMetrics::closedContour(huge,cancel);}catch(const std::exception& e){cancelled=QString::fromUtf8(e.what())=="Circle metrics cancelled";}check(cancelled,"topology cooperative cancellation");
    // Actual T4/T6.1 sampling -> T6.2 fit -> T6.3 metric, independent raster.
    double radialMax=0,pvMax=0,indexMin=1;for(double radius:{24.,40.,60.})for(bool dark:{false,true}){
        QPointF c(95.2,96.35);cv::Mat m(192,192,CV_16UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){double d=std::hypot(x-c.x(),y-c.y())-radius;m.at<quint16>(y,x)=quint16(std::lround((.5+(dark ? -.4:.4)*std::erf(d/(std::sqrt(2.)*1.4)))*65535));}
        auto image=frame(m);auto hash=image.contentId();RadialScanOptions options;options.edge.profile={.5,1,1};options.edge.minContrast=10;options.edge.polarity=dark ? EdgePolarity::Falling:EdgePolarity::Rising;
        auto s=RadialScanner::find(image,{{"raw"},Annulus{c,radius-12,radius+12}},options);auto metric=CircleMetrics::measure(s,fit(s));
        check(metric.scope==CircleMetricScope::FullCircle && metric.contour.metrics && metric.origin.frameId==s.origin.frameId && metric.origin.cycleId==s.origin.cycleId && metric.origin.completedAtUtc==s.origin.completedAtUtc && metric.inputContentId==hash && metric.processingId==s.processingId && image.contentId()==hash,"raster metrics/provenance/raw preserved");
        for(const auto& sample:metric.samples){double e=std::abs(*sample.radialDistance-radius);radialMax=std::max(radialMax,e);near(e,0,.03,"raster analytic radial gate");}
        pvMax=std::max(pvMax,metric.deviation->peakToValley);indexMin=std::min(indexMin,metric.contour.metrics->circularityIndex);near(metric.deviation->peakToValley,0,.04,"raster independent peak-to-valley gate");near(metric.contour.metrics->circularityIndex,regularIndex,1e-5,"raster polygon index vs analytic regular contour");
    }
    out<<"clean Gray16 erf fixtures=6 radius24/40/60 both polarity, sigma1.4 step.5 width1 smoothing1; radial max="<<radialMax<<" px peak-to-valley max="<<pvMax<<" px index min="<<indexMin<<" regular72 oracle="<<regularIndex<<"\n";
    auto project=[](QPointF q){double w=1+.004*q.x()+.002*q.y();return QPointF((3.5*q.x()+.4*q.y()+20)/w,(.2*q.x()+4*q.y()+20)/w);};
    auto unproject=[](QPointF p){double a=3.5-.004*p.x(),b=.4-.002*p.x(),c=.2-.004*p.y(),d=4-.002*p.y(),det=a*d-b*c;return QPointF(((p.x()-20)*d-b*(p.y()-20))/det,(a*(p.y()-20)-c*(p.x()-20))/det);};
    ChessboardObservation observation{{"metrics-projective",7,7,5},{"source","fit-frame","fit-cycle","image",QDateTime::currentDateTimeUtc()},{192,192},{},{}};
    for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);observation.worldMillimetres.append(q);observation.pixels.append(project(q));}
    auto mapping=PlaneMappingService::fit(observation);MeasurementService service;
    CalibrationDependency setup{"metrics",1,"pending","source","lens",{192,192},"plane",{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
    auto holdout=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{project(a),setup.source},{project(b),setup.source},QLineF(a,b).length()};};
    auto accepted=service.validate(service.teachHomography(setup,mapping),{holdout("hx",{2,3},{27,3}),holdout("hy",{3,2},{3,28})},.0001);
    cv::Mat perspective(192,192,CV_16UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){double d=QLineF(unproject({double(x),double(y)}),{15,15}).length()-7;perspective.at<quint16>(y,x)=quint16(std::lround((.5+.4*std::erf(d/(std::sqrt(2.)*.4)))*65535));}
    RadialScanOptions scan;scan.unitsPerSample=.25;scan.scanWidth=.25;scan.edge.profile={.5,1,1};scan.edge.minContrast=10;scan.edge.polarity=EdgePolarity::Rising;
    RadialScanCalibration context{accepted,accepted.dependency()};ArcRoi roi{{{15,15},3,11},17.3,360};auto image=frame(perspective,"image");auto worldSearch=RadialScanService::scan(image,roi,scan,context);
    auto worldOptions=CircleFitOptions{};worldOptions.inlierDistance=.1;worldOptions.maxRms=.03;worldOptions.maxResidual=.1;
    auto world=CircleMetrics::measure(worldSearch.search,fit(worldSearch.search,worldOptions));
    check(world.scope==CircleMetricScope::FullCircle && world.contour.metrics,"world metrics qualified");near(world.diameter,14,.06,"world diameter independent truth");near(world.deviation->peakToValley,0,.02,"world radial range independent gate");
    near(world.contour.metrics->circularityIndex,regularIndex,1e-5,"world polygon index independent regular oracle");
    auto raw=worldSearch.search;for(auto& s:raw.scans)s.point=s.rawPoint;auto rawMetric=CircleMetrics::measure(raw,fit(raw,permissive));
    check(rawMetric.deviation->peakToValley>1,"raw perspective shape is not a world circle");
    out<<"non-affine calibrated Gray16 world radius7mm diameter="<<world.diameter<<" mm peak-to-valley="<<world.deviation->peakToValley<<" mm polygon circularity="<<world.contour.metrics->circularityIndex<<" raw PV="<<rawMetric.deviation->peakToValley<<" px (negative control)\n";
    auto stale=context;++stale.current.version;reject([&]{RadialScanService::scan(image,roi,scan,stale);});
    out<<checks<<" checks; sampled radial metrics and actual closed polygon only; physical/GD&T/UI acceptance pending\n";
    if(argc>1){QFile file(QString::fromLocal8Bit(argv[1]));check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"report output");file.write(report.toUtf8());}QTextStream(stdout)<<report;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
