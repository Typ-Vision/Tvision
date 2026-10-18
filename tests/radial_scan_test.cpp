#include "application/RadialScanService.hpp"
#include "application/MetrologyService.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <numbers>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double e,const char* why){check(std::isfinite(a) && std::abs(a-b)<=e,why);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"expected radial rejection");}
ImageFrame frame(const cv::Mat& m,QString coordinate="raw",QString source="source") {return ImageFrame::fromBuffer(m.data,m.rows*m.step,m.cols,m.rows,m.step,m.depth()==CV_16U ? PixelFormat::Gray16:PixelFormat::Gray8,{source,"frame","cycle",coordinate,QDateTime::currentDateTimeUtc()},false);}
cv::Mat raster(QPointF centre,double radius,bool dark=false,double sigma=1.4,bool noisy=false) {
    cv::Mat m(192,192,CV_16UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){const double d=std::hypot(x-centre.x(),y-centre.y())-radius;const unsigned h=(unsigned(x)*73856093u)^(unsigned(y)*19349663u)^1234567u;const double n=noisy ? (int(h%13)-6)/255.:0;const double v=.5+(dark ? -.4:.4)*std::erf(d/(std::sqrt(2.)*sigma))+n;m.at<quint16>(y,x)=quint16(std::lround(v*65535));}return m;
}
ToolRoi ring(QPointF c={95.2,96.35},double r=40){return {{"raw"},Annulus{c,r-12,r+12}};}
RadialScanOptions options(){RadialScanOptions o;o.edge.profile={.5,1,1};o.edge.minContrast=10;o.edge.polarity=EdgePolarity::Rising;return o;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    for(int count:{10,128})for(double sweep:{360.,180.,90.,-90.,-360.})for(bool inward:{false,true}) {
        const ToolRoi roi{{"raw"},ArcRoi{{{96,96},30,50},17,sweep}};
        auto p=defaultParameters("circle_fit");p["scanCount"]=count;p["direction"]=inward ? "inward":"outward";
        const auto lines=MetrologyService::scanGuides("circle_fit",p,roi);check(lines.size()==count,"exact nominal guide count");
        for(int i=0;i<count;++i){double a=(17+sweep*i/(std::abs(sweep)==360 ? count:count-1))*std::numbers::pi/180;QPointF u(std::cos(a),std::sin(a));
            check(QLineF(lines[i].p1(),QPointF(96,96)+u*(inward ? 50:30)).length()<1e-9,"nominal start and signed angle");
            check(QLineF(lines[i].p2(),QPointF(96,96)+u*(inward ? 30:50)).length()<1e-9,"nominal end direction");}
        check(lines.first()!=lines.last(),"no duplicate full ring endpoint");
    }
    {auto p=defaultParameters("line_fit");p["scanCount"]=10;p["scanWidthPx"]=2;
        const auto lines=MetrologyService::scanGuides("line_fit",p,ToolRoi{{"raw"},RotatedRectangle{{100,100},{80,40},90}});
        check(lines.size()==10,"rotated line exact count");check(QLineF(lines.first().p1(),{120,61}).length()<1e-9 && QLineF(lines.last().p2(),{80,139}).length()<1e-9,"rotated line actual scan direction and strip margins");}
    {auto p=defaultParameters("line_fit");p["scanCount"]=10;p["scanWidthPx"]=2;p["poseTracking"]=QJsonObject{{"mmPerSample",.1}};
        const auto lines=MetrologyService::scanGuides("line_fit",p,ToolRoi{{"world",LengthUnit::Millimetre},RotatedRectangle{{10,10},{8,4},90}});
        check(QLineF(lines.first().p1(),{12,6.1}).length()<1e-9,"world strip uses synthetic scan width in nominal mm");}
    QString report;QTextStream out(&report);double sum=0,maximum=0,nsum=0,nmax=0;int count=0,ncount=0;double narrowSum=0,narrowMax=0;int narrowCount=0;
    for(double radius:{24.,40.,60.})for(QPointF centre:{QPointF(95.2,96.35),QPointF(95.57,96.06)})for(bool dark:{false,true})for(bool inward:{false,true}) {
        auto o=options();o.direction=inward ? RadialDirection::Inward:RadialDirection::Outward;o.edge.polarity=(dark!=inward) ? EdgePolarity::Falling:EdgePolarity::Rising;
        auto image=frame(raster(centre,radius,dark));const auto hash=image.contentId();const auto r=RadialScanner::find(image,ring(centre,radius),o);
        auto params=defaultParameters("circle_fit");params["scanCount"]=o.scanCount;params["direction"]=inward ? "inward":"outward";
        const auto guides=MetrologyService::scanGuides("circle_fit",params,ring(centre,radius));
        for(int i=0;i<guides.size();++i)check(guides[i]==QLineF(r.scans[i].rawStart,r.scans[i].rawEnd),"detector and guide endpoints identical");
        check(r.fullCircle && r.foundCount==72 && r.missingCount==0 && r.coverage==1,"all clean scans found");near(r.maxGapDegrees,5,1e-12,"complete full circle max gap is grid spacing");
        check(image.contentId()==hash && r.inputContentId==hash && r.origin.frameId=="frame" && r.origin.cycleId=="cycle" && r.processingId==image.processingId(),"raw immutable/provenance");
        for(const auto& s:r.scans){check(s.scanId>=0 && s.point && s.rawPoint==s.point && s.reason.isEmpty() && s.strength && s.signedStrength,"selected diagnostics");const double e=std::hypot(s.point->x()-centre.x(),s.point->y()-centre.y())-radius;sum+=e*e;maximum=std::max(maximum,std::abs(e));++count;near(e,0,.30,"clean independent radial position max gate");check((*s.signedStrength>0)==(o.edge.polarity==EdgePolarity::Rising),"scan-direction polarity");}
        const auto degradedImage=frame(raster(centre,radius,dark,2.6,true));
        const auto narrow=RadialScanner::find(degradedImage,ring(centre,radius),o);
        check(narrow.foundCount==72,"narrow noisy scans retained for characterization");
        for(auto s:narrow.scans){double e=QLineF(*s.point,centre).length()-radius;narrowSum+=e*e;narrowMax=std::max(narrowMax,std::abs(e));++narrowCount;}
        auto averaged=o;averaged.scanWidth=4;
        const auto noisy=RadialScanner::find(degradedImage,ring(centre,radius),averaged);
        check(noisy.foundCount==72,"degraded scans found");for(const auto& s:noisy.scans){const double e=QLineF(*s.point,centre).length()-radius;nsum+=e*e;nmax=std::max(nmax,std::abs(e));++ncount;near(e,0,.6,"separate degraded max gate");}
    }
    check(std::sqrt(sum/count)<=.15,"clean radial RMS gate");
    out<<"clean fixtures=24 scans="<<count<<" sigma=1.4 width=1 step=.5 smoothing=1 RMS="<<std::sqrt(sum/count)<<" max="<<maximum<<" px\n";
    out<<"narrow degraded width=1 RMS="<<std::sqrt(narrowSum/narrowCount)<<" max="<<narrowMax<<" px; characterization only, .6px acceptance failed initially\n";
    out<<"degraded width=4 fixtures=24 scans="<<ncount<<" sigma=2.6 noise=+/-6 seed=1234567 RMS="<<std::sqrt(nsum/ncount)<<" max="<<nmax<<" px\n";
    // Independent affine intensity plane: bilinear interpolation is exact.
    // A non-affine raw map makes transverse endpoint-only mapping incorrect.
    cv::Mat ramp(100,100,CV_8UC1);for(int y=0;y<100;++y)for(int x=0;x<100;++x)ramp.at<uchar>(y,x)=uchar(x+y);
    auto warp=[](QPointF p){return QPointF((p.x()+20)/(1+.05*p.y()),(p.y()+20)/(1+.05*p.y()));};
    auto mappedProfile=ProfileSampler::sampleMapped(frame(ramp),ScanSegment{{10,0},{30,0},2},warp,{1,1,0});
    // At x=20, average the three mapped world-width taps y=-1,0,+1.
    near(mappedProfile.samples[10].intensity,((59/.95)+60+(61/1.05))/3,1e-10,"each transverse world tap is inverse mapped before bilinear sampling");
    auto o=options();const auto image=frame(raster({95.2,96.35},40));auto r=RadialScanner::find(image,ring(),o);
    near(r.scans.first().angleDegrees,0,0,"full starts zero");near(r.scans.last().angleDegrees,355,0,"full has no duplicate endpoint");
    ToolRoi arc{{"raw"},ArcRoi{{{95.2,96.35},28,52},170,40}};o.scanCount=9;r=RadialScanner::find(image,arc,o);check(!r.fullCircle && r.foundCount==9,"cross-wrap partial arc");near(r.scans.last().angleDegrees,210,0,"partial includes endpoint");near(r.maxGapDegrees,5,1e-12,"arc spacing");
    std::get<ArcRoi>(arc.geometry).sweepDegrees=-120;r=RadialScanner::find(image,arc,o);near(r.scans.last().angleDegrees,50,0,"negative sweep ordered");near(r.maxGapDegrees,15,1e-12,"negative gap absolute");
    std::get<ArcRoi>(arc.geometry).sweepDegrees=-360;r=RadialScanner::find(image,arc,o);check(r.fullCircle,"negative full circle");near(r.scans.last().angleDegrees,-150,1e-12,"negative full excludes end");
    auto flat=frame(cv::Mat(192,192,CV_8UC1,cv::Scalar(20)));o=options();r=RadialScanner::find(flat,ring(),o);check(r.foundCount==0 && r.missingCount==72 && r.coverage==0,"flat keeps missing slots");near(r.maxGapDegrees,360,0,"no found full gap");for(auto s:r.scans)check(!s.point && !s.rawPoint && !s.strength && s.reason=="not_found","missing no invented geometry");
    r=RadialScanner::find(flat,arc,options());near(r.maxGapDegrees,360,0,"missing negative full circle gap");
    auto emptyArc=arc;std::get<ArcRoi>(emptyArc.geometry).sweepDegrees=-120;
    r=RadialScanner::find(flat,emptyArc,options());near(r.maxGapDegrees,120,0,"missing partial arc includes both boundary gaps");
    auto oneArc=emptyArc;std::get<ArcRoi>(oneArc.geometry).sweepDegrees=1e-10;reject([&]{RadialScanner::find(image,oneArc,options());});
    const auto repeatA=RadialScanner::find(image,ring(),options()),repeatB=RadialScanner::find(image,ring(),options());
    for(int i=0;i<72;++i)check(repeatA.scans[i].point==repeatB.scans[i].point && repeatA.scans[i].strength==repeatB.scans[i].strength,"deterministic radial selection");
    // Independent quadrant erasure includes the 360/0 seam. Grid step is 5 deg.
    cv::Mat damaged=raster({96,96},40);for(int y=0;y<192;++y)for(int x=0;x<192;++x)if(x>96 && std::abs(y-96)<25)damaged.at<quint16>(y,x)=10000;
    r=RadialScanner::find(frame(damaged),ring({96,96}),o);check(r.missingCount>6 && r.foundCount>40 && r.maxGapDegrees>50,"wraparound missing wedge counted");
    int first=-1,last=-1;double expected=0;for(int i=0;i<72;++i)if(r.scans[i].point){if(first<0)first=i;if(last>=0)expected=std::max(expected,(i-last)*5.);last=i;}expected=std::max(expected,(72-last+first)*5.);near(r.maxGapDegrees,expected,0,"circular gap independent index oracle");
    // Two boundaries: first/last follow radial travel, not global x/y order.
    cv::Mat band(192,192,CV_16UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){double d=std::hypot(x-96.,y-96.);band.at<quint16>(y,x)=quint16(std::lround((.1+.4*(std::erf((d-32)/2)-std::erf((d-48)/2)))*65535));}
    o.edge.polarity=EdgePolarity::Any;o.edge.selection=EdgeSelection::First;r=RadialScanner::find(frame(band),ring({96,96}),o);near(QLineF(*r.scans[0].point,{96,96}).length(),32,.1,"first inner boundary");o.edge.selection=EdgeSelection::Last;r=RadialScanner::find(frame(band),ring({96,96}),o);near(QLineF(*r.scans[0].point,{96,96}).length(),48,.1,"last outer boundary");o.direction=RadialDirection::Inward;o.edge.selection=EdgeSelection::First;r=RadialScanner::find(frame(band),ring({96,96}),o);near(QLineF(*r.scans[0].point,{96,96}).length(),48,.1,"inward first outer boundary");
    auto stopped=std::make_shared<std::atomic_bool>(true);reject([&]{RadialScanner::find(image,ring(),options(),stopped);});
    for(int i=0;i<10;++i){auto bad=options();if(i==0)bad.scanCount=2;if(i==1)bad.scanCount=4097;if(i==2)bad.scanWidth=0;if(i==3)bad.scanWidth=NAN;if(i==4)bad.direction=RadialDirection(99);if(i==5)bad.edge.profile.stepPx=0;if(i==6)bad.edge.profile.sigmaPx=INFINITY;if(i==7)bad.unitsPerSample=0;if(i==8)bad.unitsPerSample=2;if(i==9)bad.edge.polarity=EdgePolarity(99);reject([&]{RadialScanner::find(image,ring(),bad);});}
    auto wrong=ring();wrong.frame.id="other";reject([&]{RadialScanner::find(image,wrong,options());});wrong=ring({1,1});reject([&]{RadialScanner::find(image,wrong,options());});wrong=ring();wrong.geometry=ScanSegment{{1,1},{50,50},1};reject([&]{RadialScanner::find(image,wrong,options());});
    auto huge=options();huge.scanCount=4096;reject([&]{RadialScanner::find(frame(cv::Mat(512,512,CV_8UC1,cv::Scalar(0))),ring(),huge);});
    huge=options();huge.scanCount=4096;huge.scanWidth=1024;huge.edge.profile.widthStepPx=.25;reject([&]{RadialScanner::find(image,ring(),huge);});
    ArcRoi nominal{{{95.2,96.35},28,52},0,360};reject([&]{RadialScanner::findMapped(image,nominal,{},options());});reject([&]{RadialScanner::findMapped(image,nominal,[](QPointF){return QPointF(NAN,2);},options());});
    auto midCancel=std::make_shared<std::atomic_bool>(false);int maps=0;reject([&]{RadialScanner::findMapped(image,nominal,[&](QPointF p){if(++maps==500)midCancel->store(true);return p;},options(),midCancel);});check(maps>=500,"cooperative cancel during mapped sampling");
    MeasurementService service;
    CalibrationDependency setup{"radial",1,"pending","source","lens",{192,192},"plane",{"raw",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYUp}};
    auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,setup.source},{b,setup.source},mm};};
    auto candidate=service.teach(setup,sample("teach",{0,0},{100,0},10));auto accepted=service.validate(candidate,{sample("h1",{4,20},{104,20},10),sample("h2",{30,4},{30,104},10)},.001);
    auto calibratedOptions=options();calibratedOptions.unitsPerSample=.1;calibratedOptions.scanWidth=.1;calibratedOptions.edge.polarity=EdgePolarity::Rising;
    ArcRoi world{{{9.52,-9.635},2.8,5.2},0,360};RadialScanCalibration context{accepted,accepted.dependency()};auto calibrated=RadialScanService::scan(image,world,calibratedOptions,context);
    check(calibrated.search.foundCount==72 && calibrated.calibration==accepted.dependency(),"validated uniform radial");for(auto s:calibrated.search.scans){near(QLineF(*s.point,world.ring.centre).length(),4,.03,"uniform mm radius observation");near(s.rawPoint->y(),-s.point->y()*10,1e-9,"Y-up inverse scale");}
    auto manual=service.validate(service.manualScale(setup,10,{"acquired","source","teach-frame",{192,192}}),{sample("mx",{4,20},{104,20},10),sample("my",{30,4},{30,104},10)},.001);RadialScanCalibration manualContext{manual,manual.dependency()};check(RadialScanService::scan(image,world,calibratedOptions,manualContext).search.foundCount==72,"manual inverse scale");
    for(int i=0;i<5;++i){auto bad=context;if(i==0)bad.calibration=candidate;if(i==1)++bad.current.version;if(i==2)bad.current.contentId="changed";if(i==3)bad.current.resolution={96,96};if(i==4)bad.current.cameraId="other";reject([&]{RadialScanService::scan(flat,world,calibratedOptions,bad);});}
    reject([&]{RadialScanService::scan(frame(raster({95.2,96.35},40),"raw","foreign"),world,calibratedOptions,context);});reject([&]{RadialScanService::scan(image,world,calibratedOptions,context,stopped);});
    auto testSetup=setup;testSetup.cameraId="calibration-test:radial";
    auto testCal=service.validate(service.manualScale(testSetup,10,{"file_test",testSetup.cameraId,"test-frame",{192,192}}),{sample("fx",{4,20},{104,20},10),sample("fy",{30,4},{30,104},10)},.001);
    RadialScanCalibration fileContext{testCal,testCal.dependency()};reject([&]{RadialScanService::scan(frame(raster({95.2,96.35},40),"raw",testSetup.cameraId),world,calibratedOptions,fileContext);});
    reject([&]{RadialScanService::scan(frame(cv::Mat(128,128,CV_8UC1,cv::Scalar(0))),world,calibratedOptions,context);});
    auto badMetadata=image.metadata();badMetadata.completedAtUtc={};
    const auto bytes=raster({95.2,96.35},40);auto noTime=ImageFrame::fromBuffer(bytes.data,bytes.rows*bytes.step,bytes.cols,bytes.rows,bytes.step,PixelFormat::Gray16,badMetadata,false);
    reject([&]{RadialScanService::scan(noTime,world,calibratedOptions,context);});
    // Independent non-affine analytic world -> raw and inverse raster oracle.
    auto project=[](QPointF q){double w=1+.004*q.x()+.002*q.y();return QPointF((3.5*q.x()+.4*q.y()+20)/w,(.2*q.x()+4*q.y()+20)/w);};
    auto unproject=[](QPointF p){double a=3.5-.004*p.x(),b=.4-.002*p.x(),c=.2-.004*p.y(),d=4-.002*p.y(),det=a*d-b*c;return QPointF(((p.x()-20)*d-b*(p.y()-20))/det,(a*(p.y()-20)-c*(p.x()-20))/det);};
    ChessboardObservation observation{{"radial-projective",7,7,5},{"source","fit-frame","fit-cycle","image",QDateTime::currentDateTimeUtc()},{192,192},{},{}};for(int y=0;y<7;++y)for(int x=0;x<7;++x){QPointF q(x*5,y*5);observation.worldMillimetres.append(q);observation.pixels.append(project(q));}
    const auto mapping=PlaneMappingService::fit(observation);for(QPointF q:{QPointF(3.2,4.7),QPointF(25.1,24.6),QPointF(15,15)})near(QLineF(PlaneMappingService::inverseMap(mapping,q),project(q)).length(),0,1e-4,"inverse independent projective oracle");
    reject([&]{PlaneMappingService::inverseMap(mapping,{-10,15});});reject([&]{PlaneMappingService::inverseMap(mapping,{NAN,15});});reject([&]{PlaneMappingService::inverseMap(mapping,{-250,0});});
    auto hSetup=setup;hSetup.source.id="image";auto holdout=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{project(a),hSetup.source},{project(b),hSetup.source},QLineF(a,b).length()};};
    auto hAccepted=service.validate(service.teachHomography(hSetup,mapping),{holdout("hx",{2,3},{27,3}),holdout("hy",{3,2},{3,28})},.0001);RadialScanCalibration hContext{hAccepted,hAccepted.dependency()};
    cv::Mat perspective(192,192,CV_16UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){auto q=unproject({double(x),double(y)});double dist=QLineF(q,{15,15}).length()-7;perspective.at<quint16>(y,x)=quint16(std::lround((.5+.4*std::erf(dist/(std::sqrt(2.)*.4)))*65535));}
    ArcRoi hRoi{{{15,15},3,11},17.3,360};auto ho=options();ho.unitsPerSample=.25;ho.scanWidth=.25;ho.edge.profile={.5,1,1};auto hi=frame(perspective,"image");auto hr=RadialScanService::scan(hi,hRoi,ho,hContext);
    check(hr.search.foundCount==72,"world projective circle all scans");double hmax=0;for(auto s:hr.search.scans){double e=QLineF(*s.point,{15,15}).length()-7;hmax=std::max(hmax,std::abs(e));near(e,0,.10,"world radius independent analytic oracle");near(QLineF(*s.rawPoint,project(*s.point)).length(),0,1e-4,"selected point inverse mapped exactly");}
    out<<"projective world radius max error="<<hmax<<" mm (sigma=.4mm synthetic step=.5 x .25mm, width=.25mm)\n";
    check(std::abs(QLineF(project({22,15}),project({15,15})).length()-QLineF(project({15,22}),project({15,15})).length())>1,"raw circle assumption negative control");
    auto outside=hRoi;outside.ring.centre={2,2};reject([&]{RadialScanService::scan(hi,outside,ho,hContext);});
    auto invalid=hContext;++invalid.current.version;reject([&]{RadialScanService::scan(frame(cv::Mat(192,192,CV_8UC1,cv::Scalar(0)),"image"),hRoi,ho,invalid);});
    if(argc>1){QFile file(QString::fromLocal8Bit(argv[1]));check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"open radial report");file.write(report.toUtf8());}
    QTextStream(stdout)<<"PASS radial scan "<<checks<<" checks\n"<<report;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}}
