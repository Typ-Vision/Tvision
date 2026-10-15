#include "application/CalibrationWizardSession.hpp"
#include "domain/EdgePointConfig.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtConcurrent/QtConcurrentRun>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
template<class F>void reject(F f){++checks;try{f();}catch(const std::exception&){return;}throw std::runtime_error("Expected rejection");}
ImageFrame raster(bool blank=false,const QString& frame="one"){
    QImage im(640,200,QImage::Format_Grayscale8);im.fill(20);
    if(!blank)for(int y=0;y<200;++y)for(int x=120;x<520;++x)im.scanLine(y)[x]=220;
    FrameMetadata m{"source",frame,"cycle","image",QDateTime::currentDateTimeUtc()};
    return ImageFrame::fromBuffer(im.constBits(),im.sizeInBytes(),640,200,im.bytesPerLine(),PixelFormat::Gray8,m);
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/wizard-XXXXXX");check(dir.isValid(),"build fixture");
    MeasurementSetup setup{"bench","image_file",CalibrationWizardSession::configurationId({{"path","stripe.png"}}),"lens","plane",{640,200},{"image"},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
    CalibrationWizardSession s;s.useFrame(raster(),setup,"acquired");
    ToolRoi roi{setup.source,ScanSegment{{20,100},{620,100},4}};auto p=edgePointDefaults();p["polarity"]="rising";
    auto job=s.scanEdge("left",roi,p);auto r=CalibrationWizardSession::run(job);check(r.error().isEmpty() && r.selected() && r.candidates().size()==1,"real rising edge");
    check(s.accept(r) && !s.accept(r) && s.points().size()==1,"single use opaque completion");
    const auto left=s.points().first();check(std::abs(left.pixel.x()-119.5)<.01 && left.pixel.y()==100,"independent raster oracle, not ROI endpoint");
    check(left.image.pixelHash==QString::fromLatin1(s.frame()->contentId().toHex()) && left.roi==roi && left.parameters["subpixel"].toBool() && !left.detectionId.isEmpty(),"point carries actual frame hash/ROI/options/detection identity");
    p["polarity"]="falling";check(s.accept(CalibrationWizardSession::run(s.scanEdge("right",roi,p))),"real falling edge");
    const auto right=s.points().last();check(std::abs(right.pixel.x()-519.5)<.01,"falling oracle");
    s.selectPair(left.id,right.id);s.setKnownMillimetres(10);auto record=s.calculate();
    check(std::abs(*s.distancePixels()-400)<.01 && std::abs(record.model().millimetresPerPixel()-.025)<1e-9,"selected real 400 px / 10 mm");
    check(!record.model().validated() && !record.controlAccepted() && record.residualsMillimetres().isEmpty(),"single pair no invented physical acceptance");
    check(CalibrationModelRecord::decode(record.encode()).encode()==record.encode(),"model strict replay");
    s.renamePoint(left.id,"Left side");check(s.calculated().has_value(),"rename preserves math");
    ToolRoi other{setup.source,ScanSegment{{20,60},{620,60},4}};p["polarity"]="any";
    r=CalibrationWizardSession::run(s.scanEdge("many",other,p));check(r.candidates().size()==2 && s.accept(r) && s.points().size()==3,"multiple actual candidates and scans");
    s.chooseCandidate("many",1);check(std::abs(s.points().last().pixel.x()-519.5)<.01,"explicit overlay candidate selection");reject([&]{s.chooseCandidate("many",2);});
    s.selectPair(left.id,right.id);check(std::abs(s.calculate().model().millimetresPerPixel()-.025)<1e-9,"unselected third point excluded from teaching");
    reject([&]{s.selectPair(left.id,left.id);});check(!s.calculated() && !s.distancePixels(),"same point clears old result");
    reject([&]{s.selectPair(left.id,"missing");});
    s.selectPair(left.id,right.id);for(double mm:{0.,-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){reject([&]{s.setKnownMillimetres(mm);});reject([&]{s.calculate();});}
    s.setKnownMillimetres(10);s.calculate();s.removeScan("right");check(!s.distancePixels() && !s.calculated(),"deleted point invalidates selected pair");reject([&]{s.selectPair(left.id,right.id);});
    EdgePairOptions options;options.profile={.5,1,1};
    r=CalibrationWizardSession::run(s.scanPair("caliper",roi,options));check(r.error().isEmpty() && r.selected() && r.pairs().size()==1 && s.accept(r),"real caliper endpoints");
    check(std::abs(*s.distancePixels()-400)<.01 && s.points()[s.points().size()-2].pairId==s.points().last().pairId,"caliper relation and auto selection");
    check(std::abs(s.calculate().model().millimetresPerPixel()-.025)<1e-9,"same one-pair math");
    p["minContrast"]=5100;r=CalibrationWizardSession::run(s.scanEdge("left",roi,p));check(!r.selected() && s.accept(r),"not-found search retires old point");
    for(const auto& pt:s.points())check(pt.id!=left.id,"no stale scan endpoint");
    auto pending=s.scanPair("caliper",roi,options);auto completed=CalibrationWizardSession::run(pending);s.cancel();check(!s.accept(completed) && !s.calculated() && !s.distancePixels(),"late Stop after completed worker");
    pending=s.scanPair("caliper",roi,options);pending.cancel();r=CalibrationWizardSession::run(pending);check(!r.error().isEmpty() && r.candidates().isEmpty() && !s.accept(r),"cooperative cancelled result no values");
    s.cancel();pending=s.scanPair("caliper",roi,options);s.useFrame(raster(false,"two"),setup,"acquired");check(!s.accept(CalibrationWizardSession::run(pending)) && s.points().isEmpty(),"new frame ID invalidation");
    pending=s.scanPair("caliper",roi,options);s.useFrame(raster(true,"two"),setup,"acquired");check(!s.accept(CalibrationWizardSession::run(pending)) && s.points().isEmpty(),"same ID changed raster/hash invalidation");
    r=CalibrationWizardSession::run(s.scanPair("caliper",roi,options));check(!r.selected() && s.accept(r) && s.points().isEmpty(),"blank raster never substitutes ROI ends");
    s.useFrame(raster(),setup,"acquired");pending=s.scanPair("caliper",roi,options);auto newer=s.scanPair("newer",roi,options);check(!s.accept(CalibrationWizardSession::run(pending)) && s.accept(CalibrationWizardSession::run(newer)),"superseded job rejection");
    CalibrationWizardSession foreign;foreign.useFrame(raster(),setup,"acquired");check(!foreign.accept(completed),"cross-session completion rejected");
    pending=s.scanPair("newer",roi,options);auto future=QtConcurrent::run([pending]{return CalibrationWizardSession::run(pending);});s.clear();future.waitForFinished();check(!s.accept(future.result()) && !s.frame(),"real async worker isolated from cleared session");
    s.useFrame(raster(),setup,"acquired");auto bad=setup;bad.resolution={10,10};reject([&]{s.useFrame(raster(),bad,"acquired");});check(!s.frame() && s.points().isEmpty(),"invalid replacement clears old draft");
    const auto path=dir.filePath("stripe.png");check(raster().preview().save(path),"file fixture");s.importFile(path,setup,false);s.accept(CalibrationWizardSession::run(s.scanPair("file",roi,options)));s.setKnownMillimetres(10);record=s.calculate();check(record.image().origin=="trial_file" && !record.activatable(),"file trial provenance cannot activate");
    s.importFile(path,setup,true);s.accept(CalibrationWizardSession::run(s.scanPair("file",roi,options)));s.setKnownMillimetres(10);record=s.calculate();check(record.image().origin=="setup_file" && record.activatable() && record.image().sourceToolId.startsWith("calibration-test:"),"explicit setup declaration retains file source");
    const auto fileFrame=*s.frame();reject([&]{s.useFrame(fileFrame,setup,"acquired");});check(!s.frame(),"test source cannot be relabelled");
    s.useFrame(raster(),setup,"acquired");s.useFrame(*s.frame(),setup,"acquired");check(s.frame() && s.frame()->metadata().frameId=="one","aliased own frame safe across clear");
    s.accept(CalibrationWizardSession::run(s.scanPair("alias",roi,options)));auto aliasJob=s.scanPair(s.points().last().scanId,s.points().last().roi,options);
    check(s.accept(CalibrationWizardSession::run(aliasJob)) && s.points().size()==2,"aliased scan inputs copied before old points retire");s.removeScan(s.points().last().scanId);check(s.points().isEmpty(),"aliased remove identity safe");
    reject([&]{s.importFile(dir.filePath("missing.png"),setup,true);});check(!s.frame(),"missing file never preserves old frame");
    QTextStream(stdout)<<"Calibration wizard: "<<checks<<" checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" (check "<<checks<<")\n";return 1;}}
