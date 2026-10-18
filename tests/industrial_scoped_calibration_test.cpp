#include "IndustrialProviderFixture.hpp"
#include "application/MeasurementWorkspace.hpp"
#include "application/ProjectService.hpp"
#include <QTemporaryDir>
#include <QDir>
#include <cmath>
using namespace industrial_provider_fixture;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/industrial-scope-XXXXXX");check(dir.isValid(),"build fixture");
    auto r=request();auto state=std::make_shared<State>();QImage im(640,200,QImage::Format_Grayscale8);im.fill(20);for(int y=0;y<200;++y)for(int x=120;x<520;++x)im.scanLine(y)[x]=220;
    state->raster=im;r.setup.geometry.resolution=im.size();r.setup.geometry.sensorRoi=QRect(QPoint(0,0),im.size());
    MeasurementWorkspace ws(dir.filePath("project.vfproject"),"p",[]{return true;});ws.replace({{r.camera},{r.setup},{r.profile}});auto& service=ws.calibrations();SetupReference ref{r.camera.id,r.setup.id,1};
    IndependentCaptureRequest req;req.origin=SourceOrigin::CameraAcquired;req.camera=r.camera;req.setup=r.setup;req.profile=r.profile;
    IndependentCaptureBackend backend;backend.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));backend.industrialDiscover=[&]{return IndustrialDiscoverySnapshot{{r.device},{},false};};IndependentCalibrationSession session;
    auto capture=[&]{check(session.accept(IndependentCalibrationSession::run(session.begin(req),backend)),"actual fixture probe");};capture();
    auto& math=session.math();check(math.accept(CalibrationWizardSession::run(math.scanPair("width",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"real caliper");math.setKnownMillimetres(10);const auto model=math.calculate();service.save(ref,model);service.setOperational(r.camera.id,r.setup.id);reject([&]{service.snapshots({ref});});service.activate(ref,model.contentId(),session);const auto original=service.snapshots({ref}).first().calibration;
    check(original.scope["schemaVersion"]==2 && original.scope["physical"].toObject()==r.camera.physical->toJson(),"industrial scope carries exact physical identity");
    const auto proof=SystemCalibration::evidence(original,"source","source/image");check(SystemCalibration::decodeEvidence(proof).record.contentId()==model.contentId(),"industrial evidence codec replay");
    for(int mode=0;mode<3;++mode){auto forged=original;auto scope=forged.scope;
        if(mode==0){auto id=*r.camera.physical;id.serial="foreign";scope["physical"]=id.toJson();}
        else {auto setup=r.setup;if(mode==1)setup.geometry.sensorRoi->translate(1,0);else setup.geometry.reverse->x=true;scope["setup"]=setup.toJson();scope["geometryId"]=setup.geometryId();}
        forged.scope=scope;reject([&]{SystemCalibration::evidence(forged,"source","source/image");});}
    auto inventory=CameraService::combine({},{{r.device},{},false});ws.observeInventory(inventory);check(service.snapshots({ref}).size()==1,"valid inventory retains permission");
    ws.observeInventory(CameraService::combine({},{}));reject([&]{service.snapshots({ref});});ws.observeInventory(inventory);reject([&]{service.snapshots({ref});});capture();service.activate(ref,model.contentId(),session);
    MeasurementWorkspace restart(ws.projectPath(),"p",[]{return true;});reject([&]{restart.calibrations().snapshots({ref});});check(restart.calibrations().records(ref).size()==1,"restart remembers math only");
    auto env=ws.environment();auto next=env.current.profiles[0];next.requested.exposureTimeUs=200;next=env.current.profiles[0].revised(next);env.current.profiles[0]=next;ws.replace(env.current,env.revision);
    reject([&]{service.snapshots({ref});});reject([&]{service.activate(ref,model.contentId(),session);});check(service.records(ref).first().contentId()==model.contentId(),"exposure retains immutable model math");
    req.profile=next;capture();service.activate(ref,model.contentId(),session);const auto changed=service.snapshots({ref}).first().calibration;check(SystemCalibration::binding(original,"source","image").contentId!=SystemCalibration::binding(changed,"source","image").contentId,"exposure invalidates cache/runtime dependency");
    const auto oldBinding=SystemCalibration::binding(changed,"source","image");env=ws.environment();env.current.profiles[0].name="Renamed";ws.replace(env.current,env.revision);check(SystemCalibration::binding(service.snapshots({ref}).first().calibration,"source","image")==oldBinding,"rename preserves permission and dependency");
    auto replaced=r.device;replaced.backend.runtimeSha256=QString(64,'b');ws.observeInventory(CameraService::combine({},{{replaced},{},false}));reject([&]{service.snapshots({ref});});ws.observeInventory(inventory);capture();service.activate(ref,model.contentId(),session);
    MeasurementEnvironmentRepository target(dir.filePath("import.vfproject"),"other");target.importBundle(ws.exportDependencies());ScopedActiveCalibrationService imported(target,[]{return true;});check(imported.records(ref).first().contentId()==model.contentId(),"transfer preserves industrial math and remembered selection");reject([&]{imported.snapshots({ref});});
    env=ws.environment();auto g=env.current.setups[0];g.geometry.sensorRoi->translate(1,0);env.current.setups[0]=env.current.setups[0].revised(g);ws.replace(env.current,env.revision);reject([&]{service.snapshots({ref});});SetupReference newer{r.camera.id,r.setup.id,2};check(service.records(newer).isEmpty(),"same size different offset cannot reuse model");
    env=ws.environment();g=env.current.setups[0];g.geometry=r.setup.geometry;env.current.setups[0]=env.current.setups[0].revised(g);ws.replace(env.current,env.revision);check(service.records({r.camera.id,r.setup.id,3}).isEmpty(),"geometry A to B to A cannot revive old model");
    ProjectService project;project.createProject("Raw-only","",dir.filePath("raw.vfproject"),"file");project.createRecipe("Raw","",ApplicationType::Measurement,"file");const auto file=dir.filePath("stripe.png");check(im.save(file),"raw file fixture");project.setImageFile(file);
    check(ws.productionSources(*project.activeRecipe()).sources.size()==1,"unused unavailable industrial calibration cannot block raw-only required source set");
    check(service.snapshots({}).isEmpty(),"unused offline calibration does not gate empty required model set");check(SystemCalibration::decodeEvidence(proof).record.contentId()==model.contentId(),"old evidence replays without live registry/backend");
    QTextStream(stdout)<<"PASS: industrial activation, actual profile, offset/reverse/identity rejection, exposure versus math, unplug/replug/restart, scope codec/transfer; fixture only\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
