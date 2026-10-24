#include "application/FramedCircleGridService.hpp"
#include "application/CircleGridService.hpp"
#include "application/ImageFileService.hpp"
#include "application/PlaneMappingService.hpp"
#include "application/CalibrationWizardSession.hpp"
#include "vision/ImageAccess.hpp"
#include <QGuiApplication>
#include <QTransform>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QTextStream>
#include <opencv2/imgproc.hpp>
#include <thread>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
CalibrationGridTarget target{"fixture-7x7",7,7,12,CalibrationGridPattern::FramedCircles,FramedPlateGeometry{}};
ImageFrame frame(const QImage& im){FrameMetadata m{"fixture","frame","cycle","image",QDateTime::currentDateTimeUtc()};return ImageFrame::fromBuffer(im.constBits(),im.sizeInBytes(),im.width(),im.height(),im.bytesPerLine(),PixelFormat::Gray8,m);}
struct Options {bool missing=false,marker=true,gradient=false,clutter=false,fake=false,duplicate=false,bent=false,occluded=false,secondMarker=false;int columns=7;};
// Independently rasterized analytic disk/frame/notch predicates, 2x2 coverage.
// QTransform supplies a predetermined projective oracle, never a fitted model.
QImage raster(const QTransform& transform,const Options& o={}){
    auto inv=transform.inverted();QImage im(900,900,QImage::Format_Grayscale8);
    for(int y=0;y<900;++y)for(int x=0;x<900;++x){double sum=0;
        for(double dy:{-.25,.25})for(double dx:{-.25,.25}){auto p=inv.map(QPointF(x+dx,y+dy));double u=p.x(),v=p.y();
            bool dark=u>=80 && u<=720 && v>=80 && v<=720 && (u<=100 || u>=700 || v<=100 || v>=700);
            if(o.marker && u>=100 && v>=100 && u+v<=250)dark=true;
            if(o.fake && u>=110 && v>=110 && u+v<=270)dark=true;
            if(o.secondMarker && u<=700 && v<=700 && u+v>=1350)dark=true;
            int c=int(std::round((u-160)/80));double bend=o.bent ? 7*std::sin(c*1.2):0;int r=int(std::round((v-160-bend)/80));
            if(c>=0 && c<o.columns && r>=0 && r<7 && !(o.missing && r==2 && c==3) && std::hypot(u-(160+80*c),v-(160+80*r+bend))<(o.duplicate?12:22))dark=true;
            if(o.clutter){for(int k=0;k<9;++k)if(std::hypot(u-(180+58*k),v-118)<7)dark=true;}
            if(o.duplicate && std::hypot(u-187,v-160)<12)dark=true;
            if(o.occluded && u>400 && u<430 && v>290 && v<350)dark=false;
            double bright=o.gradient ? 95+140*x/900.:235;sum+=dark ? 20:bright;}
        im.scanLine(y)[x]=uchar(std::round(sum/4));
    }return im;
}
QJsonArray points(const QVector<QPointF>& ps){QJsonArray a;for(auto p:ps)a.append(QJsonArray{p.x(),p.y()});return a;}
QJsonObject report(const PlateDetectionResult& result,const ImageFrame& f){const auto& d=result.diagnostics();
    QJsonObject j{{"algorithm",d.algorithmVersion},{"message",d.message},{"ready",bool(result.observation())},{"parameters",d.parameters},{"stageMs",d.stageMilliseconds},{"candidates",points(d.partialCandidates)},{"assigned",points(d.assignedCandidates)},{"rejected",points(d.rejectedCandidates)},{"frame",points(d.frameBoundary)},{"pixelHash",QString::fromLatin1(f.contentId().toHex())},{"physicalAccuracyAccepted",false},{"modelActivated",false},
        {"target",target.toJson()},{"rawResolution",QJsonArray{f.size().width(),f.size().height()}},{"acquisition",f.metadata().experimentalAcquisition}};
    if(result.observation()){const auto& o=*result.observation();j["quarterTurns"]=o.orientation().quarterTurns;j["mirrored"]=o.orientation().mirrored;j["triangle"]=points({*o.orientation().triangleRaw});}return j;
}
void evidence(const QString& stem,const QImage& input,const PlateDetectionResult& result,const ImageFrame& f){
    QFile file(stem+".json");check(file.open(QIODevice::WriteOnly),"report open");file.write(QJsonDocument(report(result,f)).toJson());
    QImage overlay=input.convertToFormat(QImage::Format_RGB32);QPainter painter(&overlay);painter.setPen(QPen(Qt::yellow,2));
    for(auto p:result.diagnostics().partialCandidates){painter.drawLine(p+QPointF(-4,0),p+QPointF(4,0));painter.drawLine(p+QPointF(0,-4),p+QPointF(0,4));}
    painter.setPen(QPen(Qt::cyan,2));auto boundary=result.diagnostics().frameBoundary;for(int k=0;k<boundary.size();++k)painter.drawLine(boundary[k],boundary[(k+1)%boundary.size()]);
    painter.setPen(QPen(Qt::green,2));for(auto p:result.diagnostics().assignedCandidates)painter.drawEllipse(p,6,6);
    if(result.observation()){const auto& ps=result.observation()->grid().pixels;painter.setPen(QPen(Qt::red,3));painter.drawLine(ps[0],ps[6]);painter.drawLine(ps[0],ps[42]);}
    painter.end();
    // Built-in glyphs keep headless CI overlays readable without system fonts.
    cv::Mat pixels(overlay.height(),overlay.width(),CV_8UC4,overlay.bits(),overlay.bytesPerLine());
    auto label=[&](QPointF p,const std::string& text,cv::Scalar color){cv::Point at(int(p.x()),int(p.y()));cv::putText(pixels,text,at,cv::FONT_HERSHEY_SIMPLEX,.6,cv::Scalar(0,0,0,255),3,cv::LINE_AA);cv::putText(pixels,text,at,cv::FONT_HERSHEY_SIMPLEX,.6,color,1,cv::LINE_AA);};
    int index=0;for(auto p:result.diagnostics().assignedCandidates)label(p+QPointF(8,-5),std::to_string(++index),{0,255,0,255});
    if(result.observation()){const auto& ps=result.observation()->grid().pixels;for(auto item:{std::pair<QPointF,std::string>{ps[0]+QPointF(8,25),"P1"},{ps[6]+QPointF(8,25),"+X"},{ps[42]+QPointF(8,25),"+Y"}})label(item.first,item.second,{0,0,255,255});}
    check(overlay.save(stem+".png"),"overlay save");
}
}
int main(int argc,char** argv){qputenv("QT_QPA_PLATFORM","offscreen");QGuiApplication app(argc,argv);try{
    if(argc==3 && QString(argv[1])=="--external"){
        QFile manifest(QString::fromLocal8Bit(argv[2]));check(manifest.open(QIODevice::ReadOnly),"external fixture manifest required");
        const auto entries=QJsonDocument::fromJson(manifest.readAll()).object()["fixtures"].toArray();QDir root=QFileInfo(manifest).absoluteDir();root.cdUp();root.cdUp();
        bool missing=false;QDir().mkpath("external-evidence");
        for(auto entry:entries){auto e=entry.toObject();QString path=root.filePath(e["sourcePath"].toString());
            if(!QFileInfo::exists(path)){QTextStream(stdout)<<"SKIP missing external raw: "<<path<<'\n';missing=true;continue;}
            for(auto pair:{std::pair<QString,QString>{path,"fileSha256"},{path+".json","sidecarSha256"}}){QFile bytes(pair.first);check(bytes.open(QIODevice::ReadOnly),"raw and sidecar required");check(QString::fromLatin1(QCryptographicHash::hash(bytes.readAll(),QCryptographicHash::Sha256).toHex())==e[pair.second].toString(),"immutable external raw/sidecar hash");}
            auto f=ImageFileService{}.load(path,true);auto m=f.metadata();m.sourceId="external-fixture";m.frameId="replay";m.cycleId="replay";m.coordinateSystemId="image";m.completedAtUtc=QDateTime::currentDateTimeUtc();f=f.withMetadata(m);
            auto result=CircleGridService::observe(f,target);evidence("external-evidence/"+e["id"].toString(),f.preview(),result,f);
            check(bool(result.observation())==e["expectedReady"].toBool(),"real-raster expected readiness");
            if(result.observation()){QFile oracle(root.filePath(e["oraclePath"].toString()));check(oracle.open(QIODevice::ReadOnly),"independent oracle required");auto reference=QJsonDocument::fromJson(oracle.readAll()).object();auto ps=reference["centresRaw"].toArray();check(ps.size()==49,"oracle complete");double error=0;
                for(int i=0;i<49;++i){auto xy=ps[i].toArray();auto p=result.observation()->grid().pixels[i];error=std::max(error,std::hypot(p.x()-xy[0].toDouble(),p.y()-xy[1].toDouble()));}check(error<reference["localizationTolerancePx"].toDouble(),"real-raster independent area-centroid oracle");QTextStream(stdout)<<e["id"].toString()<<" max independent centroid delta "<<error<<" px\n";
                auto generic=target;generic.pattern=CalibrationGridPattern::SymmetricCircles;generic.framed.reset();check(!CircleGridService::observe(f,generic).observation(),"old generic false/partial 49 case remains rejected");
            }
        }
        if(missing)return 77;QTextStream(stdout)<<"External framed plate: "<<checks<<" checks\n";return 0;
    }
    if(argc==4 && QString(argv[1])=="--inspect-framed"){
        auto f=ImageFileService{}.load(QString::fromLocal8Bit(argv[2]),true);auto m=f.metadata();m.sourceId="offline-inspection";m.frameId=QString::fromLatin1(f.contentId().toHex());m.cycleId="inspection";m.coordinateSystemId="image";m.completedAtUtc=QDateTime::currentDateTimeUtc();f=f.withMetadata(m);
        auto result=CircleGridService::observe(f,target);evidence(QString::fromLocal8Bit(argv[3]),f.preview(),result,f);QTextStream(stdout)<<QJsonDocument(report(result,f)).toJson();return result.observation()?0:2;
    }
    QDir().mkpath("framed-evidence");QJsonArray localizationMetrics;
    auto run=[&](const QString& name,const QTransform& transform,Options options,bool ready){auto im=raster(transform,options);auto f=frame(im);auto result=CircleGridService::observe(f,target);evidence("framed-evidence/"+name,im,result,f);
        if(bool(result.observation())!=ready)QTextStream(stderr)<<name<<": "<<result.diagnostics().message<<" candidates="<<result.diagnostics().partialCandidates.size()<<'\n';
        check(bool(result.observation())==ready,"fixture readiness");
        if(ready){const auto& o=*result.observation();double error=0;for(int r=0;r<7;++r)for(int c=0;c<7;++c){auto expected=transform.map(QPointF(160+80*c,160+80*r+(options.bent?7*std::sin(c*1.2):0)));error=std::max(error,std::hypot(o.grid().pixels[r*7+c].x()-expected.x(),o.grid().pixels[r*7+c].y()-expected.y()));}
            if(error>=.8)QTextStream(stderr)<<name<<" max localization error "<<error<<'\n';check(error<.8,"independent raw centre/identity oracle within .8 pixel");
            localizationMetrics.append(QJsonObject{{"fixture",name},{"maxErrorPx",error},{"tolerancePx",.8}});
            check(o.grid().pixelHash==QString::fromLatin1(f.contentId().toHex()),"immutable raw hash");
            auto again=CircleGridService::observe(f,target);check(again.observation() && again.observation()->grid().pixels==o.grid().pixels,"deterministic ordering");
        }return result;};
    for(int degrees:{0,90,180,270,23,45}){QTransform t;t.translate(450,450);t.rotate(degrees);t.scale(.82,.82);t.translate(-400,-400);auto rotated=run(QString("rotation-%1").arg(degrees),t,{},true);if(degrees%90==0)check(rotated.observation()->orientation().quarterTurns==degrees/90,"four rotation identities and orientation metadata");}
    QTransform identity;run("clean",identity,{},true);
    Options clutter;clutter.clutter=true;auto extra=run("58-synthetic-dots-including-dirt",identity,clutter,true);
    check(extra.diagnostics().partialCandidates.size()==58 && extra.diagnostics().assignedCandidates.size()==49 && extra.diagnostics().rejectedCandidates.size()==9,"58 synthetic candidates select 49 real disks; nine small dirt blobs excluded");
    Options gradient;gradient.gradient=true;run("gradient",identity,gradient,true);
    QTransform small;small.translate(200,200);small.scale(.45,.45);run("small",small,{},true);
    QTransform perspective(.88,.035,.00012,.08,.98,.00008,40,25,1);run("perspective",perspective,{},true);
    Options missing;missing.missing=true;run("missing",identity,missing,false);
    Options marker;marker.marker=false;run("marker-missing",identity,marker,false);
    marker.fake=true;run("detached-fake-triangle",identity,marker,false);
    Options secondMarker;secondMarker.secondMarker=true;run("ambiguous-two-markers",identity,secondMarker,false);
    Options occluded;occluded.occluded=true;run("occluded-dot",identity,occluded,false);
    Options wrong;wrong.columns=6;run("wrong-grid",identity,wrong,false);
    Options duplicate;duplicate.duplicate=true;run("duplicate",identity,duplicate,false);
    Options bent;bent.bent=true;auto bentResult=run("limited-curvature",identity,bent,true);
    auto bentGrid=bentResult.observation()->grid();bentGrid.target.pattern=CalibrationGridPattern::SymmetricCircles;bentGrid.target.framed.reset();bool fitRejected=false;try{PlaneMappingService::fit(bentGrid);}catch(const std::exception&){fitRejected=true;}check(fitRejected,"non-H raw observation does not relax existing inverse fit gate");
    auto clean=raster(identity);auto f=frame(clean);
    CalibrationWizardSession session;MeasurementSetup setup{"bench","image_file",QString(64,'a'),"lens","plane",f.size(),{"image"},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
    session.useFrame(f,setup,"setup_file");auto job=session.scanPlate(target);check(session.accept(CalibrationWizardSession::run(job)),"framed wizard accepts typed observation");session.confirmPlate();
    const auto record=session.calculatePlate();check(record.model().mapping() && PlaneMappingService::encode(*record.model().mapping())["schemaVersion"]==3,"wizard retains orientation through model creation");
    check(CalibrationModelRecord::decode(record.encode()).encode()==record.encode(),"wizard record persistence");
    auto changed=target;changed.squareMillimetres=13;auto stale=session.scanPlate(changed);check(!session.plate() && !session.calculated(),"target spacing scan invalidates observation/model");session.invalidatePlate();check(!session.accept(CalibrationWizardSession::run(stale)),"cancelled typed completion cannot restore model");
    auto roiResult=FramedCircleGridService::observe(f,target,{},QRect(50,50,720,720));check(roiResult.observation() && roiResult.observation()->grid().pixels==CircleGridService::observe(f,target).observation()->grid().pixels,"ROI raw coordinates preserved");
    auto cropped=FramedCircleGridService::observe(f,target,{},QRect(120,120,600,600));evidence("framed-evidence/cropped",clean,cropped,f);check(!cropped.observation(),"clipped frame/marker rejected");
    QImage blank(900,900,QImage::Format_Grayscale8);blank.fill(255);check(!CircleGridService::observe(frame(blank),target).observation(),"blank saturation rejected");
    cv::Mat blurred=ImageAccess(f).ownedCopy();cv::GaussianBlur(blurred,blurred,{61,61},15);QImage bim(blurred.data,blurred.cols,blurred.rows,int(blurred.step),QImage::Format_Grayscale8);auto blur=CircleGridService::observe(frame(bim),target);evidence("framed-evidence/blur",bim,blur,frame(bim));check(!blur.observation(),"excessive blur rejected");
    auto token=std::make_shared<std::atomic_bool>(true);check(CircleGridService::observe(f,target,token).diagnostics().status==PlateDetectionStatus::Cancelled,"pre-cancel");
    token->store(false);std::thread canceller([&]{std::this_thread::sleep_for(std::chrono::milliseconds(2));token->store(true);});auto cancelled=CircleGridService::observe(f,target,token);canceller.join();check(cancelled.diagnostics().status==PlateDetectionStatus::Cancelled && !cancelled.observation(),"in-flight cancel");
    // Two independent framed plates must not silently select the first.
    QTransform left;left.translate(10,240);left.scale(.48,.48);QTransform right;right.translate(450,240);right.scale(.48,.48);auto a=raster(left),b=raster(right);for(int y=0;y<900;++y)for(int x=0;x<900;++x)a.scanLine(y)[x]=std::min(a.constScanLine(y)[x],b.constScanLine(y)[x]);auto multi=CircleGridService::observe(frame(a),target);evidence("framed-evidence/multi",a,multi,frame(a));check(!multi.observation(),"multiple plates rejected");
    // Every supported storage format exercises the same raw localization.
    auto gray=ImageAccess(f).ownedCopy();for(auto fmt:{PixelFormat::Gray16,PixelFormat::Rgb8,PixelFormat::Rgba8,PixelFormat::Bgr8,PixelFormat::Bgra8}){cv::Mat converted;int channels=fmt==PixelFormat::Rgb8 || fmt==PixelFormat::Bgr8 ? 3:4;
        if(fmt==PixelFormat::Gray16)gray.convertTo(converted,CV_16U,257);else cv::cvtColor(gray,converted,channels==3?cv::COLOR_GRAY2RGB:cv::COLOR_GRAY2RGBA);
        auto ff=ImageFrame::fromBuffer(converted.data,converted.total()*converted.elemSize(),converted.cols,converted.rows,converted.step,fmt,f.metadata());check(CircleGridService::observe(ff,target).observation().has_value(),"storage format preserved");}
    auto bad=f.withMetadata({});check(CircleGridService::observe(bad,target).diagnostics().status==PlateDetectionStatus::InvalidInput,"invalid raw provenance");
    QImage oversized(4001,4000,QImage::Format_Grayscale8);oversized.fill(235);check(CircleGridService::observe(frame(oversized),target).diagnostics().status==PlateDetectionStatus::InvalidInput,"16 MP limit");
    auto processed=ImageAccess::processed(f,gray,PixelFormat::Gray8,"fixture",1,QByteArray(32,'x'));check(!CircleGridService::observe(processed,target).observation(),"processed raster cannot masquerade as raw");
    check(FramedCircleGridService::observe(f,target,{},QRect(-1,0,900,900)).diagnostics().status==PlateDetectionStatus::InvalidInput,"invalid search ROI");
    QFile metrics("framed-evidence/localization-metrics.json");check(metrics.open(QIODevice::WriteOnly),"localization metrics report");metrics.write(QJsonDocument(localizationMetrics).toJson());
    QTextStream(stdout)<<"Framed plate: "<<checks<<" checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" (check "<<checks<<")\n";return 1;}}
