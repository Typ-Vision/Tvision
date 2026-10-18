#include "IndustrialProviderFixture.hpp"
#include "application/IndependentCalibrationCapture.hpp"
#include "domain/EdgePointConfig.hpp"
#include "chessboard_fixture.hpp"
#include <future>
#include <QTemporaryDir>
#include <QDir>
#include <cmath>
using namespace industrial_provider_fixture;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    auto r=request();auto state=std::make_shared<State>();
    auto setRaster=[&](QImage im){state->raster=im;r.setup.geometry.resolution=im.size();r.setup.geometry.sensorRoi=QRect(QPoint(0,0),im.size());};
    QImage stripe(640,200,QImage::Format_Grayscale8);stripe.fill(20);for(int y=0;y<200;++y)for(int x=120;x<520;++x)stripe.scanLine(y)[x]=220;setRaster(stripe);
    IndependentCaptureRequest req;req.origin=SourceOrigin::CameraAcquired;req.camera=r.camera;req.setup=r.setup;req.profile=r.profile;
    IndependentCaptureBackend backend;backend.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));backend.industrialDiscover=[&]{return IndustrialDiscoverySnapshot{{r.device},{},false};};
    IndependentCalibrationSession session;
    auto capture=[&]{return IndependentCalibrationSession::run(session.begin(req),backend);};
    check(session.accept(capture()),"recipe-free industrial owned frame");
    auto& m=session.math();auto edge=CalibrationWizardSession::run(m.scanEdge("edge",{{"image"},ScanSegment{{20,100},{620,100},4}},edgePointDefaults()));check(m.accept(edge),"industrial edge math");
    check(m.accept(CalibrationWizardSession::run(m.scanPair("pair",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"industrial caliper math");
    check(std::abs(*m.distancePixels()-400)<.01,"independent 400 pixel oracle");m.setKnownMillimetres(10);auto record=m.calculate();
    check(std::abs(record.model().millimetresPerPixel()-.025)<1e-9 && record.image().origin=="acquired","real scale candidate without activation");
    check(record.setup()==IndependentCaptureRequest::installation(r.camera,r.setup),"exact physical/geometry math basis");
    {auto held=backend.factory.open(r,CameraLeaseUse::Editor,FrameQueueMode::ProductionFifo);check(!session.accept(capture()),"calibration shares exclusive lease");}
    for(int mode=0;mode<5;++mode){state->foreign=mode==0;state->geometry=mode==1;state->settings=mode==2;state->delayMs=mode==3?30:0;state->hardware->scenario=mode==4?"old":"";req.timeoutMs=mode==3?10:5000;
        const auto result=capture();check(!result.error().isEmpty() && !session.accept(result) && !session.math().frame(),"identity/geometry/profile/deadline/stale frame rejected");}
    state->foreign=state->geometry=state->settings=false;state->delayMs=0;state->hardware->scenario="";req.timeoutMs=5000;
    auto job=session.begin(req);auto completed=IndependentCalibrationSession::run(job,backend);session.cancel();check(!session.accept(completed),"late completion after Stop suppressed");
    job=session.begin(req);job.cancel();check(!session.accept(IndependentCalibrationSession::run(job,backend)),"cancel before open");
    QTemporaryDir dir(QDir::currentPath()+"/industrial-capture-XXXXXX");check(dir.isValid(),"build-only file evidence");const auto path=dir.filePath("stripe.png");check(stripe.save(path),"file fixture");
    auto file=req;file.origin=SourceOrigin::SetupFile;file.filePath=path;
    check(session.accept(IndependentCalibrationSession::run(session.begin(file))),"industrial setup file accepted offline");
    check(session.evidence()->origin==SourceOrigin::SetupFile,"setup file retains file provenance");
    reject([&]{session.math().useFrame(*session.math().frame(),*session.acceptedSetup(),"acquired");});
    file.origin=SourceOrigin::TrialFile;file.camera.reset();file.setup.reset();file.profile.reset();check(session.accept(IndependentCalibrationSession::run(session.begin(file))) && !session.evidence()->target,"trial has no physical claim");
    auto product=backend;product.factory=CameraProviderFactory{};check(!session.accept(IndependentCalibrationSession::run(session.begin(req),product)),"normal product composition cannot acquire fixture");
    auto wrong=backend;wrong.industrialDiscover=[&]{auto d=r.device;d.identity.serial="other";return IndustrialDiscoverySnapshot{{d},{},false};};check(!session.accept(IndependentCalibrationSession::run(session.begin(req),wrong)),"no discovery index fallback");
    auto invalid=req;invalid.profile.reset();reject([&]{session.begin(invalid);});
    setRaster(chessboardFixture());req.setup=r.setup;check(session.accept(capture()),"industrial owned chessboard");
    auto plate=CalibrationWizardSession::run(m.scanPlate({"grid",9,6,10}));check(m.accept(plate) && m.plate()->pixels.size()==54,"industrial plate detection");m.confirmPlate();check(m.calculatePlate().model().mapping().has_value(),"industrial real homography");
    QTextStream(stdout)<<"PASS: recipe-free industrial Edge/Caliper/plate, exact identity/readback, shared lease, cancellation, stale/deadline rejection; fixture only\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
