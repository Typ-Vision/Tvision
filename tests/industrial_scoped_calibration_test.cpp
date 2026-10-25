#include "IndustrialProviderFixture.hpp"
#include "application/MeasurementWorkspace.hpp"
#include "application/ProjectService.hpp"
#include "application/IndustrialExecution.hpp"
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
    auto& math=session.math();check(math.accept(CalibrationWizardSession::run(math.scanPair("width",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"real caliper");math.setKnownMillimetres(10);const auto model=math.calculate();service.setOperational(r.camera.id,r.setup.id);reject([&]{service.snapshots({ref});});
    const auto saved=ws.saveAndUseCalibration({ref,ws.environment().revision,session.evidence()->generation},model,session,[]{return true;});
    check(saved.state==CalibrationSaveUseState::Activated && !saved.experimental && !saved.independentControlAccepted,"single application command preserves accepted industrial native probe checks");const auto original=service.snapshots({ref}).first().calibration;
    check(original.scope["schemaVersion"]==2 && original.scope["physical"].toObject()==r.camera.physical->toJson(),"industrial scope carries exact physical identity");
    const auto proof=SystemCalibration::evidence(original,"source","source/image");check(proof["schemaVersion"]==3 && SystemCalibration::decodeEvidence(proof).record.contentId()==model.contentId(),"industrial session evidence codec replay");
    auto legacy=original;legacy.acquisition={};const auto legacyProof=SystemCalibration::evidence(legacy,"source","source/image");check(legacyProof["schemaVersion"]==2 && SystemCalibration::evidence(SystemCalibration::decodeEvidence(legacyProof),"source","source/image")==legacyProof,"old scope2 evidence remains byte-equivalent");
    check(service.status(ref).state==CalibrationAvailability::Valid && service.status(ref).mathematicsCompatible && service.status(ref).sessionPermitted,"validity separates preference mathematics and permission");
    {auto forged=proof;auto acquisition=forged["acquisition"].toObject();auto p=acquisition["probe"].toObject();p["origin"]="setup_file";acquisition["probe"]=p;forged["acquisition"]=acquisition;reject([&]{SystemCalibration::decodeEvidence(forged);});}
    {
        ProjectService project;project.createProject("Industrial Editor","",dir.filePath("editor.vfproject"),"file");
        const auto id=project.createRecipe("Industrial","",ApplicationType::Measurement,"file");
        const auto source=project.addTool("industrial_camera");project.bindIndustrialSource(source,ws.environment().current,r.profile.id,1000);project.bindSourceSetup(source,ref.toJson());
        const auto left=project.addTool("edge_point"),right=project.addTool("edge_point"),metric=project.addTool("measurement");
        for(auto tool:project.tools())if(tool.id==left || tool.id==right){tool.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();tool.parameters["polarity"]=tool.id==left ? "rising":"falling";project.updateTool(tool);}
        project.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",metric,"p1"},{right,"point",metric,"p2"}});
        const auto branch=SystemCalibration::branch(*project.activeRecipe(),source);
        IndustrialExecutionInputs inputs;inputs.contract=ws.environment().current;inputs.factory=backend.factory;inputs.discover=[r](int,const auto&){return IndustrialDiscoverySnapshot{{r.device},{},false};};
        auto resolver=[&](const ImageFrame& frame){return SystemCalibration::resolve(original,branch,dir.path(),frame);};
        const auto cycle=ExecutionService(inputs).run(branch.definition,dir.path(),id,"cfg",{}, {},{},resolver);
        check(cycle.state==ExecutionState::Done && cycle.industrialEvidence.size()==1,"industrial editor acquisition with scoped resolver");
        const auto raw=std::get<ImageFrame>(cycle.tools.first().outputs["image"].data);
        check(resolver(raw).calibration!=nullptr,"industrial branch model resolves actual frame");
        bool measured=false;for(const auto& tool:cycle.tools)if(tool.toolId==metric){const auto value=tool.outputs.value("value");measured=value.valid && std::holds_alternative<double>(value.data) && std::abs(std::get<double>(value.data)-10)<.01;}
        check(measured,"industrial ROI and scoped model produce actual 10 mm result");
        {
            MeasurementWorkspace cold(ws.projectPath(),"p",[]{return true;});
            RuntimeService offline(dir.filePath("offline-runtime"),{},RuntimeIdentity::localEngineer(),{}, {}, {},[&](const Recipe& recipe){return cold.productionSources(recipe,inputs);});
            offline.setConfigurationProviders({},[&](const Recipe& recipe){return cold.configurationSources(recipe,inputs);});
            const auto persisted=cold.exportDependencies();offline.checkConfiguration(*project.activeRecipe(),dir.path());
            check(!offline.active() && cold.exportDependencies()==persisted && !cold.calibrations().status(ref).sessionPermitted,"industrial offline exact model Load grants no permission and writes nothing");
            reject([&]{offline.prepare(*project.activeRecipe(),dir.path());});
            auto missing=*project.activeRecipe();auto def=missing.definition;auto tools=editorTools(def);for(auto& t:tools)if(t.id==source){auto reference=t.parameters["sourceReference"].toObject();reference["profileRevision"]=999;t.parameters["sourceReference"]=reference;}missing.definition=withEditorTools(def,tools);reject([&]{offline.checkConfiguration(missing,dir.path());});
        }
        MultiSourceRuntimeService runtime({"engineer",[](const QString&,ProductionAction){return true;}},[&](const Recipe& recipe){return ws.productionSources(recipe,inputs);});
        runtime.prepare(*project.activeRecipe(),dir.path());const auto lease=runtime.begin();
        service.disconnect(r.camera.id); // An admitted worker owns the original model; next begin must reprepare.
        const auto actual=RuntimeService::execute(lease,"",{});bool runtimeMeasured=false;
        for(const auto& tool:actual.tools)if(tool.toolId==metric)runtimeMeasured=tool.outputs.value("value").valid && std::abs(std::get<double>(tool.outputs["value"].data)-10)<.01;
        check(actual.state==ExecutionState::Done && runtimeMeasured,"Runtime frozen industrial model survives live selection invalidation");
        runtime.finish(lease.ticket,actual);reject([&]{runtime.begin();});
        capture();service.activate(ref,model.contentId(),session);
        auto foreign=branch;auto tools=editorTools(foreign.definition);auto reference=tools[0].parameters["sourceReference"].toObject();auto physical=reference["physical"].toObject();physical["serial"]="foreign";reference["physical"]=physical;tools[0].parameters["sourceReference"]=reference;foreign.definition=withEditorTools(foreign.definition,tools);
        check(!SystemCalibration::resolve(original,foreign,dir.path(),raw).calibration,"foreign physical source cannot use branch model");
    }
    for(int mode=0;mode<3;++mode){auto forged=original;auto scope=forged.scope;
        if(mode==0){auto id=*r.camera.physical;id.serial="foreign";scope["physical"]=id.toJson();}
        else {auto setup=r.setup;if(mode==1)setup.geometry.sensorRoi->translate(1,0);else setup.geometry.reverse->x=true;scope["setup"]=setup.toJson();scope["geometryId"]=setup.geometryId();}
        forged.scope=scope;reject([&]{SystemCalibration::evidence(forged,"source","source/image");});}
    auto inventory=CameraService::combine({},{{r.device},{},false});ws.observeInventory(inventory);check(service.snapshots({ref}).size()==1,"valid inventory retains permission");
    ws.observeInventory(CameraService::combine({},{}));reject([&]{service.snapshots({ref});});ws.observeInventory(inventory);reject([&]{service.snapshots({ref});});capture();service.activate(ref,model.contentId(),session);
    MeasurementWorkspace restart(ws.projectPath(),"p",[]{return true;});reject([&]{restart.calibrations().snapshots({ref});});check(restart.calibrations().records(ref).size()==1,"restart remembers math only");
    check(restart.calibrations().status(ref).state==CalibrationAvailability::Remembered && restart.calibrations().status(ref).mathematicsCompatible && !restart.calibrations().status(ref).sessionPermitted,"restart displays remembered compatible mathematics without permission");
    {IndependentCalibrationSession unavailable;auto native=backend;native.factory=CameraProviderFactory{};check(!unavailable.accept(IndependentCalibrationSession::run(unavailable.begin(req),native)),"discovery does not grant unavailable native capture");reject([&]{restart.calibrations().prepareRemembered(ref,unavailable);});check(!restart.calibrations().status(ref).sessionPermitted,"failed native check cannot restore permission or experimental opt-in");}
    const auto restartBytes=restart.environment().encode();capture();restart.calibrations().prepareRemembered(ref,session);const auto reopened=restart.calibrations().snapshots({ref}).first().calibration;
    check(restart.environment().encode()==restartBytes && reopened.record.encode()==original.record.encode() && SystemCalibration::binding(reopened,"source","image")!=SystemCalibration::binding(original,"source","image"),"fresh restart probe creates new acquisition dependency without catalog/preference rewrite");
    reject([&]{restart.calibrations().prepareRemembered(ref,session);});
    auto env=ws.environment();auto next=env.current.profiles[0];next.requested.exposureTimeUs=200;next=env.current.profiles[0].revised(next);env.current.profiles[0]=next;ws.replace(env.current,env.revision);
    reject([&]{service.snapshots({ref});});reject([&]{service.activate(ref,model.contentId(),session);});check(service.records(ref).first().contentId()==model.contentId(),"exposure retains immutable model math");
    check(service.status(ref).state==CalibrationAvailability::Remembered && service.status(ref).mathematicsCompatible,"exposure needs recheck not refit");
    req.profile=next;capture();service.prepareRemembered(ref,session);const auto changed=service.snapshots({ref}).first().calibration;check(changed.record.encode()==original.record.encode() && changed.acquisition!=original.acquisition && SystemCalibration::binding(original,"source","image").contentId!=SystemCalibration::binding(changed,"source","image").contentId,"exposure preserves math and refreshes acquisition/cache/runtime dependency");
    env=ws.environment();next=env.current.profiles[0];next.requested.gain=3;env.current.profiles[0]=env.current.profiles[0].revised(next);ws.replace(env.current,env.revision);reject([&]{service.prepareRemembered(ref,session);});req.profile=env.current.profiles[0];capture();service.prepareRemembered(ref,session);check(service.snapshots({ref}).first().calibration.record.contentId()==model.contentId(),"gain recheck retains math");
    const auto oldBinding=SystemCalibration::binding(service.snapshots({ref}).first().calibration,"source","image");env=ws.environment();env.current.profiles[0].name="Renamed";ws.replace(env.current,env.revision);check(SystemCalibration::binding(service.snapshots({ref}).first().calibration,"source","image")==oldBinding,"rename preserves permission and dependency");
    auto replaced=r.device;replaced.backend.runtimeSha256=QString(64,'b');ws.observeInventory(CameraService::combine({},{{replaced},{},false}));reject([&]{service.snapshots({ref});});ws.observeInventory(inventory);capture();service.activate(ref,model.contentId(),session);
    MeasurementEnvironmentRepository target(dir.filePath("import.vfproject"),"other");target.importBundle(ws.exportDependencies());ScopedActiveCalibrationService imported(target,[]{return true;});check(imported.records(ref).first().contentId()==model.contentId(),"transfer preserves industrial math and remembered selection");reject([&]{imported.snapshots({ref});});
    env=ws.environment();auto g=env.current.setups[0];g.geometry.sensorRoi->translate(1,0);env.current.setups[0]=env.current.setups[0].revised(g);ws.replace(env.current,env.revision);reject([&]{service.snapshots({ref});});SetupReference newer{r.camera.id,r.setup.id,2};check(service.records(newer).isEmpty(),"same size different offset cannot reuse model");
    env=ws.environment();g=env.current.setups[0];g.geometry=r.setup.geometry;env.current.setups[0]=env.current.setups[0].revised(g);ws.replace(env.current,env.revision);check(service.records({r.camera.id,r.setup.id,3}).isEmpty(),"geometry A to B to A cannot revive old model");
    check(service.status({r.camera.id,r.setup.id,3}).state==CalibrationAvailability::RecalibrationRequired && !service.status({r.camera.id,r.setup.id,3}).mathematicsCompatible,"historical model is not current mathematics");
    for(int mode=0;mode<3;++mode){env=ws.environment();g=env.current.setups[0];if(mode==0){g.geometry.resolution={320,200};g.geometry.sensorRoi=QRect(0,0,320,200);}else if(mode==1)g.geometry.reverse->x=true;else {g.geometry.binning=QSize(2,1);g.geometry.sensorRoi=QRect(0,0,640,200);}env.current.setups[0]=env.current.setups[0].revised(g);ws.replace(env.current,env.revision);const auto current=env.current.setups[0];check(service.status({r.camera.id,r.setup.id,current.revision}).state==CalibrationAvailability::RecalibrationRequired,"crop reverse binning cannot reuse math");}
    env=ws.environment();reject([&]{service.notifyOpticalChange(r.camera.id,env.revision-1);});ws.notifyOpticalChange(r.camera.id,env.revision);const auto pending=ws.environment();check(pending.current.setups[0].pending && !pending.selections.isEmpty() && service.historyRecords(r.setup.id).size()==1,"optical change retires declaration and preserves history");
    ProjectService project;project.createProject("Raw-only","",dir.filePath("raw.vfproject"),"file");project.createRecipe("Raw","",ApplicationType::Measurement,"file");const auto file=dir.filePath("stripe.png");check(im.save(file),"raw file fixture");project.setImageFile(file);
    check(ws.productionSources(*project.activeRecipe()).sources.size()==1,"unused unavailable industrial calibration cannot block raw-only required source set");
    check(service.snapshots({}).isEmpty(),"unused offline calibration does not gate empty required model set");check(SystemCalibration::decodeEvidence(proof).record.contentId()==model.contentId(),"old evidence replays without live registry/backend");
    QTextStream(stdout)<<"PASS: industrial activation, actual profile, offset/reverse/identity rejection, exposure versus math, unplug/replug/restart, scope codec/transfer; fixture only\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
