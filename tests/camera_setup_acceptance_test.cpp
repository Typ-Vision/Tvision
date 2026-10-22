#include "OecsExperimentalFixture.hpp"
#include "chessboard_fixture.hpp"
#include "application/MeasurementWorkspace.hpp"
#include "application/MeasurementProjectTransfer.hpp"
#include <cstring>
using namespace oecs_experimental_fixture;
namespace {
class IdentityApi final : public Api {
    PhysicalDeviceIdentity identity_;
public:
    IdentityApi(std::shared_ptr<State> state, PhysicalDeviceIdentity identity)
        : Api(std::move(state)), identity_(std::move(identity)) {}
    int enumerateUsb(QVector<OecsDeviceInfo>& devices) override {
        devices={{identity_.vendor,identity_.model,identity_.serial}};return 0;
    }
};
void pixels(const std::shared_ptr<State>& hardware,bool plate=false) {
    hardware->pixels.fill(plate ? char(255):char(20));
    if(plate){const auto board=chessboardFixture();for(int y=0;y<board.height();++y)
        std::memcpy(hardware->pixels.data()+y*2448,board.constScanLine(y),board.width());}
    else for(int y=0;y<2048;++y)for(int x=120;x<520;++x)hardware->pixels[y*2448+x]=char(220);
}
void write(const QString& path,const QByteArray& bytes) {
    QFile file(path);check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"evidence write");
    check(file.write(bytes)==bytes.size(),"evidence write complete");
}
void scenario(const QString& directory,int cameraCount,QJsonArray& matrix) {
    QDir().mkpath(directory);
    ProjectService project;project.createProject("CCUX unified fixture","",directory+"/line.vfproject","file");project.save();
    check(project.project().recipes.isEmpty(),"camera/calibration before any recipe");
    MeasurementWorkspace workspace(project.path(),project.project().id,[]{return true;});
    auto& active=workspace.calibrations();
    QVector<IndustrialCameraRequest> cameras;IndustrialDiscoverySnapshot discovery;
    for(int i=0;i<cameraCount;++i){auto r=request();r.device.identity.serial="CCUX19-"+QString::number(i);discovery.devices<<r.device;cameras<<r;}
    auto inventory=CameraService::combine({},discovery);workspace.observeInventory(inventory);
    for(int i=0;i<cameraCount;++i){const auto added=workspace.registerCamera(inventory.devices[i],"CAM_"+QString::number(i+1),"Fixture camera",workspace.environment().revision);
        auto e=workspace.environment();auto& r=cameras[i];for(const auto& c:e.current.cameras)if(c.id==added.camera)r.camera=c;
        const auto defaults=e.defaults(added.camera);check(defaults.setup.has_value() && defaults.profile.has_value(),"automatic default setup/profile");r.setup.id=*defaults.setup;r.setup.camera=added.camera;r.profile.id=*defaults.profile;r.profile.camera=added.camera;r.profile.setup=r.setup.id;
        // Explicit optical fixture preparation; unknown real installations remain pending.
        for(auto& s:e.current.setups)if(s.id==r.setup.id){r.setup=s.revised(r.setup);s=r.setup;}
        for(auto& p:e.current.profiles)if(p.id==r.profile.id){r.profile=p.revised(r.profile);p=r.profile;}
        workspace.replace(e.current);
        const auto duplicate=workspace.registerCamera(inventory.devices[i],{},"Duplicate",workspace.environment().revision);
        check(duplicate.camera==added.camera && workspace.environment().current.cameras.size()==i+1,"duplicate physical camera reuses immutable registration");
    }
    if(cameraCount>1)rejects([&]{workspace.renameCamera(cameras[1].camera.id,"CAM_1","duplicate ID",workspace.environment().revision);});
    check(project.project().recipes.isEmpty(),"registration/defaults require no recipe");
    auto hardware=state();hardware->nodes["DeviceSerialNumber"].value=cameras[0].device.identity.serial;
    auto opener=[&]{pixels(hardware);return ExperimentalOecsAcquisition::open(cameras[0],directory+"/fixture.dll",[&]{return std::make_unique<IdentityApi>(hardware,cameras[0].device.identity);});};
    CameraPreviewSession preview;preview.start(opener,false,3000);
    until([&]{preview.poll();return preview.drained();});check(preview.stats().completed==1 && preview.takeLatest().has_value(),"single preview real fixture frame");
    preview.start(opener,true,3000);until([&]{preview.poll();return preview.stats().completed>=3;});
    check(preview.stats().fps>0 && preview.takeLatest().has_value(),"bounded live preview and measured completion FPS");
    preview.stop();until([&]{preview.poll();return preview.drained();});check(!preview.takeLatest(),"Stop retires preview generation before calibration");
    QStringList replayPaths;QVector<SetupReference> references;
    for(int i=0;i<cameraCount;++i){const auto r=cameras[i];hardware->nodes["DeviceSerialNumber"].value=r.device.identity.serial;
        const SetupReference ref{r.camera.id,r.setup.id,r.setup.revision};references<<ref;active.setOperational(ref.camera,ref.setup);
        IndependentCaptureRequest input;input.origin=SourceOrigin::CameraAcquired;input.camera=r.camera;input.setup=r.setup;input.profile=r.profile;input.policy=CalibrationCapturePolicy::Automatic;input.timeoutMs=3000;
        IndependentCaptureBackend backend;backend.industrialDiscover=[r]{return IndustrialDiscoverySnapshot{{r.device},{},false};};backend.experimentalApi=[hardware,r]{return std::make_unique<IdentityApi>(hardware,r.device.identity);};
        IndependentCalibrationSession probe;
        for(const bool plate : i==0 ? QVector<bool>{true,false}:QVector<bool>{false}){
            pixels(hardware,plate);const auto captured=IndependentCalibrationSession::run(probe.begin(input),backend);
            if(!captured.error().isEmpty())throw std::runtime_error(captured.error().toStdString());check(probe.accept(captured),"selected camera independent acquisition");
            auto& math=probe.math();CalibrationModelRecord model=[&]{if(plate){check(math.accept(CalibrationWizardSession::run(math.scanPlate({"fixture board",9,6,10}))),"existing chessboard detection");math.confirmPlate();return math.calculatePlate();}
                check(math.accept(CalibrationWizardSession::run(math.scanPair("pair",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"known length uses detected edges");math.setKnownMillimetres(10*(i+1));return math.calculate();}();
            const auto saved=workspace.saveAndUseCalibration({ref,workspace.environment().revision,probe.evidence()->generation,CalibrationCapturePolicy::Automatic},model,probe,[]{return true;});
            if(!saved.error.isEmpty())throw std::runtime_error(saved.error.toStdString());check(saved.state==CalibrationSaveUseState::Activated && saved.experimental && !saved.independentControlAccepted,"one save/use command without invented physical acceptance");
        }
        const auto image=*probe.math().frame();const auto meta=image.metadata();const auto path=directory+QString("/camera-%1.pgm").arg(i);
        ExperimentalOecsAcquisition::save({image,meta.sourceId,meta.cycleId,meta.frameId,meta.completedAtUtc},path);replayPaths<<path;
    }
    check(project.project().recipes.isEmpty(),"both calibrations completed without recipe");
    for(const auto type:{ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement}){
        const auto id=project.createRecipe("Shared editor "+QString::number(int(type)),"",type);check(project.tools().isEmpty(),"new recipe is empty DRAFT without global references");project.enableMultiSource();
        QStringList sources,metrics;QVector<ToolConnection> links;
        for(int i=0;i<cameraCount;++i){const auto source=project.addTool("image_file");sources<<source;project.setImageFile(replayPaths[i],source);project.bindCameraSource(source,workspace.environment(),cameras[i].camera.id);
            const auto a=project.addTool("edge_point"),b=project.addTool("edge_point"),m=project.addTool("measurement");metrics<<m;
            for(auto t:project.tools())if(t.id==a || t.id==b){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();t.parameters["polarity"]=t.id==a?"rising":"falling";project.updateTool(t);}
            links<<ToolConnection{source,"image",a,"image"}<<ToolConnection{source,"image",b,"image"}<<ToolConnection{a,"point",m,"p1"}<<ToolConnection{b,"point",m,"p2"};
        }
        const auto raw=project.addTool("image_file"),blob=project.addTool("blob");project.setImageFile(replayPaths[0],raw);for(auto t:project.tools())if(t.id==blob){t.parameters["threshold"]=128;project.updateTool(t);}links<<ToolConnection{raw,"image",blob,"image"};project.setConnections(links);check(project.dirty(),"camera selection is dirty without autosave");project.save();
        const auto recipe=*project.activeRecipe();const auto dependencies=workspace.productionSources(recipe,{},true);
        auto resolve=[&](const ImageFrame& frame){const auto source=frame.metadata().sourceId;if(!dependencies.experimentalCalibrations.contains(source))return MeasurementExecutionContext{};return SystemCalibration::resolveExperimental(*dependencies.experimentalCalibrations[source],SystemCalibration::branch(recipe,source),frame);};
        QJsonObject acquisition;for(const auto& source:sources)acquisition[source]=workspace.acquisition(SystemCalibration::branch(recipe,source));
        EditorSession editor;EditorContext context{project.project().id,project.path(),id,recipe.definition,{}, {}};const auto signature=editor.signature(context);
        const auto result=ExecutionService{}.run(recipe.definition,directory,id,signature,{}, {},{},resolve,nullptr,{{"sources",acquisition}});
        if(result.state!=ExecutionState::Done)throw std::runtime_error(result.error.toStdString());const auto ticket=editor.begin(SessionJob::Execution);check(editor.acceptCycle(result,signature,context),"Editor Run Flow accepts only actual source results");editor.finish(ticket);
        const auto archive=directory+"/runtime-"+QString::number(int(type));RuntimeService runtime(archive,{},RuntimeIdentity::localEngineer());
        bool foreign=true;runtime.setAutomaticAcquisition([&](const Recipe& recipe){auto d=workspace.productionSources(recipe,{},true);if(foreign){auto wrong=std::make_shared<ExperimentalScopedCalibrationSnapshot>(*d.experimentalCalibrations[sources[0]]);wrong->snapshot.target.camera={"foreign-camera"};d.experimentalCalibrations[sources[0]]=wrong;}return d;});
        rejects([&]{runtime.prepare(recipe,directory);});foreign=false;runtime.prepare(recipe,directory);check(ProductionStore(archive).recent("experimental_cycles").isEmpty(),"Load/Run preparation produces no result");
        rejects([&]{runtime.begin({{"plc",true}});});const auto lease=runtime.begin();const auto cycle=RuntimeService::execute(lease,{},{});check(cycle.state==ExecutionState::Done,"same recipe Runtime executes");
        for(int i=0;i<metrics.size();++i){bool actual=false;for(const auto& t:cycle.tools)if(t.toolId==metrics[i])actual=t.outputs["value"].valid && t.outputs["value"].port.unit=="mm" && std::abs(std::get<double>(t.outputs["value"].data)-10*(i+1))<.001 && t.decision==PartDecision::NotEvaluated;check(actual,"each source uses its own 10/20/30/40/50 mm fixture model");}
        bool rawPixels=false;for(const auto& t:cycle.tools)if(t.toolId==blob)rawPixels=t.outputs["area"].valid && t.outputs["area"].port.unit=="px2" && !t.calibration;check(rawPixels,"uncalibrated raw branch retains px2");
        runtime.finish(lease.ticket,cycle);const auto record=ProductionStore(archive).recent("experimental_cycles").first();ExperimentalRuntime::validateRecord(record);
        const auto csv=ExperimentalRuntime::csv({record});check(csv.contains("mm") && csv.contains("px2") && !ExperimentalRuntime::summary(record).contains("PASS"),"Logs/CSV real metric values, no part PASS");
        write(archive+"/record.json",QJsonDocument(record).toJson());write(archive+"/results.csv",csv);
        const auto late=runtime.begin();auto stopped=RuntimeService::execute(late,{},{});RuntimeService::suppressCancelled(late,stopped);runtime.finish(late.ticket,stopped);check(stopped.frames.isEmpty(),"late Stop suppresses outputs");rejects([&]{runtime.begin();});
        RuntimeService restart(archive,{},RuntimeIdentity::localEngineer());check(!restart.active() && !restart.experimental() && ProductionStore(archive).recent("experimental_cycles").size()==2,"restart preserves Logs only");
        matrix.append(QJsonObject{{"cameras",cameraCount},{"applicationType",int(type)},{"metricSources",cameraCount},{"rawSources",1},{"state","DONE"},{"automaticPolicy",true},{"physicalAcceptance",false},{"record",archive+"/record.json"}});
    }
    // The very same registered/calibrated project exercises transfer and permission retirement.
    const auto before=workspace.environment();const auto copied=directory+"/copy.vfproject";MeasurementProjectTransfer::copy(project.path(),copied,project.project().id);
    MeasurementWorkspace copy(copied,project.project().id,[]{return true;});check(copy.environment().registrations==before.registrations && copy.environment().selections==before.selections,"Save As exact registered defaults and selected models");rejects([&]{copy.calibrations().experimentalSnapshots(references);});
    MeasurementWorkspace imported(directory+"/import.vfproject","imported",[]{return true;});imported.importDependencies(workspace.exportDependencies());check(imported.environment().registrations==before.registrations,"explicit import exact IDs");rejects([&]{imported.importDependencies(workspace.exportDependencies());});
    ProjectService reopened;reopened.openProject(project.path());reopened.save();QFile file(project.path());check(file.open(QIODevice::ReadOnly),"read no-op project");const auto bytes=file.readAll();file.close();reopened.openProject(project.path());reopened.save();check(file.open(QIODevice::ReadOnly) && file.readAll()==bytes,"no-op open/save keeps project bytes");
    active.disconnect(cameras[0].camera.id);check(!active.status(references[0]).sessionPermitted,"unplug retires this camera permission");if(cameraCount>1)check(active.status(references[1]).sessionPermitted,"unplug keeps other source permission");
    rejects([&]{workspace.productionSources(*project.activeRecipe(),{},true);});
    auto settings=workspace.environment();for(auto& p:settings.current.profiles)if(p.id==cameras.last().profile.id){auto next=p;next.requested.exposureTimeUs+=100;p=p.revised(next);}workspace.replace(settings.current);
    check(!active.status(references.last()).sessionPermitted && active.status(references.last()).mathematicsCompatible,"settings invalidate session permission while retaining mathematics");
    workspace.notifyOpticalChange(cameras.last().camera.id,workspace.environment().revision);check(!active.status(references.last()).sessionPermitted,"optical/model invalidation prevents stale metric reuse");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto root=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS",QDir::currentPath()+"/../ccux/ccux-19");QDir().mkpath(root);
    QTemporaryDir temp(root+"/e2e-XXXXXX");check(temp.isValid(),"e2e evidence directory");temp.setAutoRemove(false);QJsonArray matrix;
    for(int count:{1,5})scenario(temp.path()+QString("/%1-camera").arg(count),count,matrix);
    write(root+"/e2e-matrix.json",QJsonDocument(QJsonObject{{"scope","software fixture only"},{"scenarios",matrix}}).toJson());
    QTextStream(stdout)<<"PASS: saved project -> 1/5 registrations -> single/live/Stop -> automatic policy without opt-in -> two recipe-free calibration methods -> three application types -> Editor -> source-scoped Runtime -> Logs/CSV; transfer, duplicate, restart, unplug and invalidation; fixture only\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
