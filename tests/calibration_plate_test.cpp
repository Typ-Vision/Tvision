#include "application/CalibrationWizardSession.hpp"
#include "application/CircleGridService.hpp"
#include "application/CalibrationTargetLibrary.hpp"
#include "application/CalibrationService.hpp"
#include "application/CalibrationStudyStore.hpp"
#include "application/ImageFileService.hpp"
#include "vision/ImageAccess.hpp"
#include <opencv2/features.hpp>
#include <opencv2/imgproc.hpp>
#include "chessboard_fixture.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTextStream>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
template<class F>void reject(F f){++checks;try{f();}catch(const std::exception&){return;}throw std::runtime_error("Expected plate rejection");}
double norm(QPointF p){return std::hypot(p.x(),p.y());}
// Fixture is rendered by an analytic inverse, independent of OpenCV's H solver,
// detector, drawing, warp or fitted centres. Supersampling covers pixel centres.
QPointF project(QPointF p,bool perspective){
    double w=perspective ? 1+.0015*p.x()+.0006*p.y():1;
    return {(5*p.x()+.65*p.y()+130)/w,(.2*p.x()+5.6*p.y()+100)/w};
}
QPointF inverse(QPointF p,bool perspective,bool severe=false){
    double g=perspective ? (severe ? .004:.0015):0,h=perspective ? (severe ? .0015:.0006):0;
    double a=5-g*p.x(),b=.65-h*p.x(),c=p.x()-130,d=.2-g*p.y(),e=5.6-h*p.y(),f=p.y()-100;
    return {(c*e-b*f)/(a*e-b*d),(a*f-c*d)/(a*e-b*d)};
}
QImage circles(bool perspective,double radius,bool partial=false,bool severe=false){
    QImage im(640,460,QImage::Format_Grayscale8);
    for(int y=0;y<im.height();++y)for(int x=0;x<im.width();++x){int dark=0;
        for(int sy=0;sy<3;++sy)for(int sx=0;sx<3;++sx){auto w=inverse({x+(sx-1)/3.,y+(sy-1)/3.},perspective,severe);
            int col=int(std::round(w.x()/10)),row=int(std::round(w.y()/10));
            if(col>=0 && col<8 && row>=0 && row<6 && !(partial && col==3 && row==2) && norm(w-QPointF(col*10,row*10))<=radius)++dark;}
        im.scanLine(y)[x]=uchar(245-dark*225/9);
    }return im;
}
ImageFrame frame(const QImage& im,const QString& fid="fit"){
    FrameMetadata m{"source",fid,"cycle","image",QDateTime::currentDateTimeUtc()};
    return ImageFrame::fromBuffer(im.constBits(),im.sizeInBytes(),im.width(),im.height(),im.bytesPerLine(),PixelFormat::Gray8,m);
}
MeasurementSetup setup(const QSize& size){return {"bench","image_file",CalibrationWizardSession::configurationId({{"file","plate.png"}}),"lens","plane",size,{"image"},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};}
struct Orientation {bool flipX=false,flipY=false;QPointF local(QPointF p) const{return {flipX ? 70-p.x():p.x(),flipY ? 50-p.y():p.y()};}};
Orientation orientation(const CalibrationGridObservation& o,bool perspective){
    Orientation best;double error=1e100;
    for(bool x:{false,true})for(bool y:{false,true}){Orientation candidate{x,y};double sum=0;
        for(int i=0;i<o.pixels.size();++i){auto p=project(candidate.local({(i%8)*10.,(i/8)*10.}),perspective);sum+=norm(o.pixels[i]-p);}
        if(sum<error){error=sum;best=candidate;}}
    return best;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);
    // Explicit manual raster inspection; never a hardware CTest or activation.
    if(argc>1){
        QJsonObject report{{"physicalAccuracyAccepted",false},{"modelActivated",false}};
        try{
            if(argc!=7 || QString::fromLocal8Bit(argv[1])!="--inspect-circles")
                throw std::runtime_error("usage: --inspect-circles raw-image columns rows spacing-mm new-report.json");
            bool columnsOk=false,rowsOk=false,spacingOk=false;
            const int columns=QString::fromLocal8Bit(argv[3]).toInt(&columnsOk),rows=QString::fromLocal8Bit(argv[4]).toInt(&rowsOk);
            const double spacing=QString::fromLocal8Bit(argv[5]).toDouble(&spacingOk);
            if(!columnsOk || !rowsOk || !spacingOk)throw std::runtime_error("Numeric target dimensions required");
            const CalibrationGridTarget target{"operator-declared-plate",columns,rows,spacing,CalibrationGridPattern::SymmetricCircles};
            target.validate();report["target"]=target.toJson();
            auto raster=ImageFileService{}.load(QString::fromLocal8Bit(argv[2]),true);
            auto metadata=raster.metadata();metadata.sourceId="manual-plate-inspection";
            metadata.frameId=QString::fromLatin1(raster.contentId().toHex());metadata.cycleId=metadata.frameId;
            metadata.coordinateSystemId="image";metadata.completedAtUtc=QDateTime::currentDateTimeUtc();
            raster=raster.withMetadata(metadata);
            report["pixelHash"]=QString::fromLatin1(raster.contentId().toHex());
            report["acquisition"]=metadata.experimentalAcquisition;
            report["inputPath"]=QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
            report["origin"]="offline_raster_inspection";
            try{
                const auto observation=CircleGridService::detect(raster,target);
                const auto mapping=PlaneMappingService::fit(observation);
                report["status"]="GRID_FIT_REQUIRES_OPERATOR_REVIEW";
                report["mapping"]=PlaneMappingService::encode(mapping);
            }catch(const std::exception& e){
                report["status"]="DETECTION_REJECTED";report["error"]=QString::fromUtf8(e.what());
                // Diagnostic positions only, using the production detector's gates.
                // These do not bypass full-grid order/fit or create a model.
                if(raster.format()==PixelFormat::Gray8){
                    cv::SimpleBlobDetector::Params p;p.minThreshold=10;p.maxThreshold=240;p.thresholdStep=10;p.minRepeatability=2;
                    p.filterByColor=true;p.blobColor=0;p.filterByArea=true;p.minArea=9;p.maxArea=1000000;
                    p.filterByCircularity=false;p.filterByConvexity=true;p.minConvexity=.8;p.filterByInertia=true;p.minInertiaRatio=.1;
                    std::vector<cv::KeyPoint> blobs;cv::SimpleBlobDetector::create(p)->detect(ImageAccess(raster).ownedCopy(),blobs);
                    QJsonArray positions;for(const auto& blob:blobs)positions.append(QJsonArray{blob.pt.x,blob.pt.y,blob.size});
                    report["diagnosticBlobs"]=positions;
                    QJsonArray trials;
                    const auto gray=ImageAccess(raster).ownedCopy();
                    for(int step:{2,5,10})for(int area:{9,1000}){
                        auto trial=p;trial.thresholdStep=step;trial.minArea=area;
                        std::vector<cv::KeyPoint> found;cv::SimpleBlobDetector::create(trial)->detect(gray,found);
                        trials.append(QJsonObject{{"step",step},{"minArea",area},{"count",int(found.size())}});
                        if(step==2 && area==1000){QJsonArray pts;for(const auto& b:found)pts.append(QJsonArray{b.pt.x,b.pt.y,b.size});report["fineDiagnosticBlobs"]=pts;}
                    }
                    report["diagnosticTrials"]=trials;
                }
            }
            QFile output(QString::fromLocal8Bit(argv[6]));
            if(!output.open(QIODevice::WriteOnly|QIODevice::NewOnly))throw std::runtime_error("New report file required");
            const auto bytes=QJsonDocument(report).toJson();
            if(output.write(bytes)!=bytes.size() || !output.flush())throw std::runtime_error("Report write failed");
            QTextStream(stdout)<<bytes;
            return report.contains("error")?1:0;
        }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}
    }
    try{
    QTemporaryDir dir(QDir::currentPath()+"/plate-XXXXXX");check(dir.isValid(),"build fixture");dir.setAutoRemove(false);
    CalibrationGridTarget target{"circle plate",8,6,10,CalibrationGridPattern::SymmetricCircles};
    check(CalibrationGridTarget::fromJson(target.toJson()).toJson()==target.toJson(),"typed target descriptor roundtrip");
    for(int n=0;n<13;++n){auto bad=target.toJson();
        if(n==0)bad["schemaVersion"]=2;if(n==1)bad["kind"]="asymmetric_circle_grid";if(n==2)bad["kind"]="chessboard";
        if(n==3)bad["origin"]="corner_1";if(n==4)bad["axes"]="robot";if(n==5)bad["spacingMm"]=0;if(n==6)bad["spacingMm"]=-1;
        if(n==7)bad["columns"]=3.5;if(n==8)bad["rows"]=33;if(n==9)bad["unknown"]=true;if(n==10)bad.remove("spacingMm");if(n==11)bad["spacingMm"]="10";if(n==12){bad["columns"]=32;bad["rows"]=32;}
        reject([&]{CalibrationGridTarget::fromJson(bad);});}
    // Descriptor v2 is explicit; v1 aggregates/aliases and bytes stay unchanged.
    CalibrationGridTarget framed{"framed-7x7",7,7,12,CalibrationGridPattern::FramedCircles,FramedPlateGeometry{}};
    check(CalibrationGridTarget::fromJson(framed.toJson()).toJson()==framed.toJson(),"framed descriptor v2 roundtrip with unknown physical sizes absent");
    framed.framed->frameThicknessMm=3;framed.framed->circleDiameterMm=4;
    check(CalibrationGridTarget::fromJson(framed.toJson()).toJson()==framed.toJson(),"optional physical geometry roundtrip");
    const QJsonObject legacyTarget{{"schemaVersion",1},{"id","circle plate"},{"kind","symmetric_circle_grid"},{"columns",8},{"rows",6},{"spacingMm",10},{"origin","centre_1"},{"axes","columns_rows"}};
    check(QJsonDocument(target.toJson()).toJson(QJsonDocument::Compact)==QJsonDocument(legacyTarget).toJson(QJsonDocument::Compact),"legacy descriptor byte contract unchanged");
    for(int n=0;n<7;++n){auto bad=framed.toJson();auto geometry=bad["geometry"].toObject();
        if(n==0)bad["schemaVersion"]=3;if(n==1)bad["origin"]="centre_1";if(n==2)geometry["unknown"]=1;
        if(n==3)geometry["circleDiameterMm"]="4";if(n==4)geometry["polarity"]="light_on_dark";
        if(n==5)geometry.remove("orientation");if(n==6)bad["unknown"]=1;bad["geometry"]=geometry;
        reject([&]{CalibrationGridTarget::fromJson(bad);});}
    for(int n=0;n<9;++n){auto bad=framed;
        if(n==0)bad.framed.reset();if(n==1)bad.columns=8;if(n==2)bad.framed->circleDiameterMm=0;
        if(n==3)bad.framed->innerWidthMm=std::numeric_limits<double>::infinity();
        if(n==4)bad.framed->polarity=static_cast<PlatePolarity>(99);
        if(n==5)bad.framed->orientation=PlateOrientation::Unmarked;if(n==6)bad.pattern=CalibrationGridPattern::SymmetricCircles;
        if(n==7)bad.framed->frameThicknessMm=-1;if(n==8)bad.framed->innerHeightMm=std::numeric_limits<double>::quiet_NaN();
        reject([&]{bad.validate();});}
    reject([&]{CalibrationTargetLibrary(dir.filePath("framed-target-library.json")).save("framed",framed);});
    auto unsupported=CircleGridService::observe(frame(circles(false,1.6)),framed);
    check(unsupported.diagnostics().status==PlateDetectionStatus::Unsupported && !unsupported.observation(),"new framed profile cannot silently use generic detector");
    reject([&]{PlateDetectionResult::failure(PlateDetectionStatus::Ready,"not an observation");});
    CalibrationWizardSession unsupportedSession;auto uf=frame(circles(false,1.6));unsupportedSession.useFrame(uf,setup(uf.size()),"acquired");
    const auto ur=CalibrationWizardSession::run(unsupportedSession.scanPlate(framed));
    check(ur.diagnostics().status==PlateDetectionStatus::Unsupported && unsupportedSession.accept(ur) && !unsupportedSession.plate(),"wizard preserves typed unsupported result");
    reject([&]{unsupportedSession.confirmPlate();});reject([&]{unsupportedSession.calculatePlate();});
    // Independent non-projective raw raster. Each column has a sinusoidal y
    // displacement; this is an observation/model boundary test, not a lens solver.
    QImage bent(640,460,QImage::Format_Grayscale8);bent.fill(245);
    cv::Mat bentPixels(bent.height(),bent.width(),CV_8UC1,bent.bits(),bent.bytesPerLine());
    for(int r=0;r<6;++r)for(int c=0;c<8;++c)cv::circle(bentPixels,{80+65*c,80+55*r+int(std::round(5*std::sin(c*1.3)))},9,cv::Scalar(20),-1);
    const auto bf=frame(bent,"bent");const auto detected=CircleGridService::observe(bf,target);
    check(detected.diagnostics().status==PlateDetectionStatus::Ready && detected.observation(),"non-H grid remains a ready raw observation");
    const auto raw=*detected.observation();reject([&]{PlaneMappingService::fit(raw);});
    check(raw.coordinateSpace()=="raw_pixel_centres/1" && raw.orientation().kind==PlateOrientation::Unmarked,"raw identity and legacy unmarked direction explicit");
    for(int n=0;n<17;++n){auto grid=raw.grid();auto centres=raw.centres();auto direction=raw.orientation();QString space=raw.coordinateSpace(),algorithm=raw.algorithmVersion();
        if(n==0)centres[1].column=0;if(n==1)centres.removeLast();if(n==2)centres[0].rawPixel.setX(0);
        if(n==3)centres[0].uncertaintyPixels=-1;if(n==4)centres[0].uncertaintyPixels=std::numeric_limits<double>::quiet_NaN();
        if(n==5)grid.pixelHash="bad";if(n==6)grid.source.frameId.clear();if(n==7)grid.resolution={16000001,1};
        if(n==8)grid.pixels[1]=grid.pixels[0];if(n==9)grid.worldMillimetres[1]={999,0};
        if(n==10)grid.pixels[0].setX(std::numeric_limits<double>::infinity());if(n==11)direction.kind=PlateOrientation::TriangleCorner;
        if(n==12)space="undistorted_pixel_centres/1";if(n==13)algorithm.clear();if(n==14)grid.source.coordinateSystemId="plane/mm";
        if(n==15)direction.triangleRaw=QPointF(0,0);if(n==16)direction.quarterTurns=4;
        reject([&]{RawPlateObservation::create(grid,centres,direction,algorithm,space);});}
    // A framed observation can exist before its future detector/codec/solver.
    auto fg=raw.grid();fg.target=framed;fg.pixels.clear();fg.worldMillimetres.clear();QVector<PlateCentre> fc;
    for(int r=0;r<7;++r)for(int c=0;c<7;++c){QPointF p{80.+60*c,60.+50*r+5*std::sin(c*1.3)};fg.pixels.append(p);fg.worldMillimetres.append({12.*c,12.*r});fc.append({r,c,p,.2});}
    auto oriented=RawPlateObservation::create(fg,fc,{PlateOrientation::TriangleCorner,0,false,QPointF(40,30)},"contract_fixture/1");
    check(oriented.centres().size()==49,"complete identified framed raw observation independent from H");
    reject([&]{RawPlateObservation::create(fg,fc,{},"contract_fixture/1");});
    reject([&]{RawPlateObservation::create(fg,fc,{PlateOrientation::TriangleCorner,0,false,QPointF(500,400)},"contract_fixture/1");});
    reject([&]{RawPlateObservation::create(fg,fc,{PlateOrientation::TriangleCorner,0,true,QPointF(40,30)},"contract_fixture/1");});
    reject([&]{PlaneMappingService::fit(oriented);});reject([&]{PlaneMappingService::fit(fg);});
    CalibrationWizardSession bentSession;bentSession.useFrame(bf,setup(bf.size()),"acquired");
    const auto br=CalibrationWizardSession::run(bentSession.scanPlate(target));
    check(br.error().isEmpty() && bentSession.accept(br) && bentSession.plate(),"worker and accept retain non-H raw observation");
    bentSession.confirmPlate();check(!bentSession.calculated(),"session confirmation is not model acceptance");
    reject([&]{bentSession.calculatePlate();});check(bentSession.plate() && !bentSession.calculated(),"fit rejection preserves observation but no model");
    bent.save(dir.filePath("non-projective-raw.png"));
    double lowBias=0,highBias=0;QJsonArray metrics;
    for(bool perspective:{false,true})for(double radius:{1.6,3.4}){
        const auto im=circles(perspective,radius);const auto f=frame(im);const auto o=CircleGridService::detect(f,target);
        im.save(dir.filePath(QString("circle-%1-%2.png").arg(perspective).arg(radius)));
        check(o.pixels.size()==48 && o.worldMillimetres.size()==48 && o.target.pattern==CalibrationGridPattern::SymmetricCircles,"real full circle grid");
        check(o.pixelHash==QString::fromLatin1(f.contentId().toHex()) && o.worldMillimetres.last()==QPointF(70,50),"real raster/hash/typed world grid");
        const auto order=orientation(o,perspective);double maxBias=0;
        for(int i=0;i<o.pixels.size();++i){const auto truth=project(order.local({(i%8)*10.,(i/8)*10.}),perspective);const double bias=norm(o.pixels[i]-truth);maxBias=std::max(maxBias,bias);
            if(bias>=.7)QTextStream(stderr)<<"oracle p="<<perspective<<" r="<<radius<<" i="<<i<<" got="<<o.pixels[i].x()<<','<<o.pixels[i].y()<<" truth="<<truth.x()<<','<<truth.y()<<" bias="<<bias<<" flips="<<order.flipX<<','<<order.flipY<<" raster="<<dir.path()<<'\n';
            check(bias<.7,"independent projected centre oracle");}
        const auto m=PlaneMappingService::fit(o);double maxWorldError=0;
        for(int y=0;y<4;++y)for(int x=0;x<6;++x){QPointF world{7.25+x*10,6.75+y*10};auto raw=project(world,perspective);auto got=PlaneMappingService::map(m,raw);double error=norm(got-order.local(world));maxWorldError=std::max(maxWorldError,error);check(error<.18,"independent off-grid H oracle mm");}
        auto j=PlaneMappingService::encode(m);check(j["schemaVersion"]==2 && j["target"].toObject()["kind"]=="symmetric_circle_grid" && !j["target"].toObject().contains("squareMm"),"circle distinct schema/centre spacing");
        check(PlaneMappingService::encode(PlaneMappingService::decode(j))==j,"exact circle H replay");
        check(norm(PlaneMappingService::map(m,o.pixels[0]))<.18,"detected hull corner accepted");reject([&]{PlaneMappingService::map(m,{0,0});});reject([&]{PlaneMappingService::inverseMap(m,{-10,20});});
        for(int n=0;n<9;++n){auto bad=j;
            if(n==0)bad["schemaVersion"]=3;if(n==1)bad["model"]="raw_planar_homography/1";
            if(n==2){auto t=bad["target"].toObject();t["kind"]="chessboard";t["origin"]="corner_1";bad["target"]=t;}
            if(n==3){auto h=bad["H"].toArray();h[0]=h[0].toDouble()+.1;bad["H"]=h;}
            if(n==4)bad["fitRmsMm"]=0.123;if(n==5)bad["pixelHash"]="bad";if(n==6)bad["coordinateSpace"]="undistorted_pixels";
            if(n==7){auto a=bad["pixels"].toArray();auto v=a[0];a[0]=a[1];a[1]=v;bad["pixels"]=a;}
            if(n==8){auto a=bad["worldMm"].toArray();a[1]=QJsonArray{99,0};bad["worldMm"]=a;}
            reject([&]{PlaneMappingService::decode(bad);});}
        auto wrong=o;std::swap(wrong.pixels[1],wrong.pixels[2]);reject([&]{PlaneMappingService::fit(wrong);});wrong=o;wrong.pixels[1]=wrong.pixels[0];reject([&]{PlaneMappingService::fit(wrong);});wrong=o;wrong.pixels.removeLast();reject([&]{PlaneMappingService::fit(wrong);});
        CalibrationWizardSession session;session.useFrame(f,setup(f.size()),"acquired");auto job=session.scanPlate(target);auto found=CalibrationWizardSession::run(job);
        check(found.error().isEmpty() && session.accept(found) && !session.accept(found),"opaque plate completion once");reject([&]{session.calculatePlate();});session.confirmPlate();const auto record=session.calculatePlate();
        check(record.activatable() && !record.model().validated() && record.residualsMillimetres().isEmpty() && !record.controlAccepted(),"confirmed grid ready without fabricated holdout");
        check(CalibrationModelRecord::decode(record.encode()).encode()==record.encode(),"circle wrapper exact replay");
        auto mismatched=record.image();mismatched.pixelHash=QString(64,'f');reject([&]{CalibrationModelRecord::create(record.model(),record.setup(),mismatched);});
        const QPointF wa{13.25,12.5},wb{54.75,36.25};CalibrationSample independent{"independent",{project(wa,perspective),record.setup().source},{project(wb,perspective),record.setup().source},norm(wa-wb)};
        const auto controlled=session.calculatePlate({{independent},.3});check(controlled.controlAccepted()==true && controlled.residualsMillimetres().size()==1 && !controlled.model().validated(),"actual independent control separate from legacy validated");
        independent.knownMillimetres+=3;const auto failed=session.calculatePlate({{independent},.1});check(failed.controlAccepted()==false && !failed.activatable(),"failed real control blocks activation");
        CalibrationSample fitPoints{"fit-as-control",{o.pixels[0],record.setup().source},{o.pixels[1],record.setup().source},10};reject([&]{session.calculatePlate({{fitPoints},.1});});
        ActiveCalibrationService catalog(dir.filePath(QString("catalog-%1-%2.json").arg(perspective).arg(radius)),"project",[]{return true;});catalog.save(record);check(!catalog.active(),"circle candidate save does not activate");catalog.activate(record.model().dependency().calibrationId,1,record.setup());check(catalog.resolve(record.setup()).record.contentId()==record.contentId(),"circle catalog exact resolution");
        auto changed=target;changed.rows=5;job=session.scanPlate(changed);check(!session.plate() && !session.calculated(),"target edit retires old grid/model");found=CalibrationWizardSession::run(job);check(!found.error().isEmpty() && session.accept(found) && !session.plate(),"wrong count no stale result");reject([&]{session.confirmPlate();});
        job=session.scanPlate(target);found=CalibrationWizardSession::run(job);session.cancel();check(!session.accept(found),"late cancelled plate result");
        job=session.scanPlate(target);session.useFrame(f,setup(f.size()),"acquired");check(!session.accept(CalibrationWizardSession::run(job)),"replacement frame invalidates plate job");
        if(perspective){if(radius<2)lowBias=maxBias;else highBias=maxBias;}
        metrics.append(QJsonObject{{"perspective",perspective},{"radiusMm",radius},{"maxCentreBiasPx",maxBias},{"maxIndependentWorldErrorMm",maxWorldError},{"fitRmsMm",m.fitRmsMillimetres()},{"maxInverseFitPx",m.maxInversePixels()},{"raster",dir.filePath(QString("circle-%1-%2.png").arg(perspective).arg(radius))}});
        check(im.save(dir.filePath(QString("circle-%1-%2.png").arg(perspective).arg(radius))),"save independent raster");
    }
    check(highBias>lowBias && highBias>.05,"perspective centre bias measured independently of fit");
    auto f=frame(circles(false,1.6));auto wrong=target;wrong.columns=7;reject([&]{CircleGridService::detect(f,wrong);});wrong=target;wrong.pattern=CalibrationGridPattern::Chessboard;reject([&]{CircleGridService::detect(f,wrong);});reject([&]{ChessboardService::detect(f,target);});
    reject([&]{CircleGridService::detect(frame(circles(false,1.6,true)),target);});auto blank=circles(false,1.6);blank.fill(245);reject([&]{CircleGridService::detect(frame(blank),target);});
    auto changedSpacing=target;changedSpacing.squareMillimetres=20;const auto changed=CircleGridService::detect(f,changedSpacing);check(changed.worldMillimetres.last()==QPointF(140,100),"physical spacing cannot be inferred from raster");
    auto cancelled=std::make_shared<std::atomic_bool>(true);reject([&]{CircleGridService::detect(f,target,cancelled);});
    const auto severe=frame(circles(true,1.6,false,true));CalibrationWizardSession severeSession;severeSession.useFrame(severe,setup(severe.size()),"acquired");
    const auto severeResult=CalibrationWizardSession::run(severeSession.scanPlate(target));check(severeSession.accept(severeResult),"severe grid completion accounted");
    // A strong perspective can exceed the detector's ordering capability. The
    // completed job may only offer a replayable non-duplicate grid or an error.
    if(severeResult.observation()){severeResult.observation()->validateRaw();check(!severeSession.calculated(),"strong grid observation never implies accepted H");}
    else check(!severeResult.error().isEmpty() && !severeSession.plate() && !severeSession.calculated(),"strong malformed ordering yields no stale observation/model");
    metrics.append(QJsonObject{{"severePerspectiveAccepted",bool(severeResult.observation())},{"error",severeResult.error()}});
    const auto gray=circles(false,1.6);
    for(int n=0;n<3;++n){ImageFrame colour;
        if(n==0){QVector<quint16> pixels(gray.width()*gray.height());for(int y=0;y<gray.height();++y)for(int x=0;x<gray.width();++x)pixels[y*gray.width()+x]=quint16(gray.constScanLine(y)[x])*257;
            colour=ImageFrame::fromBuffer(pixels.constData(),pixels.size()*2,gray.width(),gray.height(),gray.width()*2,PixelFormat::Gray16,f.metadata());}
        else {const auto rgb=gray.convertToFormat(n==1 ? QImage::Format_RGB888:QImage::Format_RGBA8888);
            colour=ImageFrame::fromBuffer(rgb.constBits(),rgb.sizeInBytes(),rgb.width(),rgb.height(),rgb.bytesPerLine(),n==1 ? PixelFormat::Rgb8:PixelFormat::Rgba8,f.metadata());}
        check(CircleGridService::detect(colour,target).pixels.size()==48,"Gray16/RGB/RGBA owned raw format");}
    auto metadata=f.metadata();metadata.frameId.clear();reject([&]{CircleGridService::detect(f.withMetadata(metadata),target);});
    for(double spacing:{0.,-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){auto t=target;t.squareMillimetres=spacing;reject([&]{CircleGridService::detect(f,t);});}
    for(bool perspective:{false,true}){
        const auto board=frame(chessboardFixture(perspective));CalibrationWizardSession session;session.useFrame(board,setup(board.size()),"acquired");
        CalibrationGridTarget boardTarget{"board",9,6,10};auto found=CalibrationWizardSession::run(session.scanPlate(boardTarget));check(found.error().isEmpty() && session.accept(found) && session.plate()->pixels.size()==54,"same session real chessboard path");
        session.confirmPlate();auto record=session.calculatePlate();auto encoded=PlaneMappingService::encode(*record.model().mapping());check(encoded["schemaVersion"]==1 && encoded["target"].toObject()["squareMm"]==10 && !encoded.contains("pixelHash"),"legacy chessboard mapping shape preserved");
        check(CalibrationModelRecord::decode(record.encode()).encode()==record.encode(),"legacy board wrapper replay");
        found=CalibrationWizardSession::run(session.scanPlate(session.plate()->target));check(found.observation().has_value() && session.accept(found),"aliased target copied before retiring observation");
    }
    CalibrationWizardSession legacyGuard;legacyGuard.useFrame(f,setup(f.size()),"acquired");legacyGuard.accept(CalibrationWizardSession::run(legacyGuard.scanPlate(target)));legacyGuard.confirmPlate();auto guarded=legacyGuard.calculatePlate();
    const auto rawFrame=guarded.setup().source;
    QVector<CalibrationSample> samples{{"x",{project({13,12},false),rawFrame},{project({53,12},false),rawFrame},40},{"y",{project({15,14},false),rawFrame},{project({15,34},false),rawFrame},20}};
    const auto validatedCircle=MeasurementService{}.validate(guarded.model(),samples,.3);
    reject([&]{CalibrationService(dir.filePath("legacy.json"),{}).save(validatedCircle);});
    const auto legacyPath=dir.filePath("circle-study.png");check(gray.save(legacyPath),"circle study guard asset");const auto imported=ChessboardService::importImage(legacyPath);
    reject([&]{CalibrationStudyStore::save(dir.filePath("circle.vfcalstudy"),imported,validatedCircle);});
    QFile report(QDir::currentPath()+"/kui3-plate-metrics.json");check(report.open(QIODevice::WriteOnly),"metrics file");report.write(QJsonDocument(metrics).toJson());
    QTextStream(stdout)<<"Calibration plate: "<<checks<<" checks\n"<<QJsonDocument(metrics).toJson(QJsonDocument::Compact)<<'\n';return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" (check "<<checks<<")\n";return 1;}}
