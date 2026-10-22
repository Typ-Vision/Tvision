#include "OecsExperimentalFixture.hpp"
#include "application/ScopedActiveCalibration.hpp"
#include "application/SystemCalibration.hpp"
#include "application/ProjectService.hpp"
#include "application/EditorSession.hpp"
using namespace oecs_experimental_fixture;
namespace {
class Adapter final:public AcquisitionAdapter {
public:
    AcquisitionCapabilities capabilities() const override{return {"webcam",true};}
    ImageFrame acquire(const AcquisitionRequest&) const override {
        QImage im(640,200,QImage::Format_Grayscale8);im.fill(20);
        for(int y=0;y<200;++y)for(int x=120;x<520;++x)im.scanLine(y)[x]=220;
        return ImageFrame::fromBuffer(im.constBits(),im.sizeInBytes(),640,200,im.bytesPerLine(),PixelFormat::Gray8);
    }
};
CalibrationModelRecord calculate(IndependentCalibrationSession& session,double mm=10){
    auto& m=session.math();check(m.accept(CalibrationWizardSession::run(m.scanPair("pair",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"fixture caliper");
    m.setKnownMillimetres(mm);return m.calculate();
}
CalibrationSaveUseRequest command(ScopedActiveCalibrationService& s,const SetupReference& ref,const IndependentCalibrationSession& probe,CalibrationCapturePolicy policy=CalibrationCapturePolicy::Trusted){
    return {ref,s.environment().revision,probe.evidence()->generation,policy};
}
void nativeChecks(const QString& path){
    MeasurementEnvironmentRepository repo(path,"native");MeasurementEnvironmentRegistry reg(repo);
    MeasurementCamera camera{{"A"},"A",{"webcam","provider-A",DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}};
    MeasurementSetupDescriptor setup{{"S"},camera.id,"S","lens","plane",1,1,{{640,200},{},{},true}};
    auto other=camera;other.id={"B"};other.device.providerId="provider-B";auto otherSetup=setup;otherSetup.id={"T"};otherSetup.camera=other.id;
    reg.replace({{camera,other},{setup,otherSetup},{}});bool ready=true;
    ScopedActiveCalibrationService service(repo,[&]{return ready;});const SetupReference ref{camera.id,setup.id,1};service.setOperational(camera.id,setup.id);
    IndependentCaptureRequest request;request.origin=SourceOrigin::CameraAcquired;request.camera=camera;request.setup=setup;
    IndependentCaptureBackend backend{[&]{return QVector<DeviceBinding>{camera.device};},[](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<Adapter>();}};
    IndependentCalibrationSession probe;auto capture=[&]{check(probe.accept(IndependentCalibrationSession::run(probe.begin(request),backend)),"native adapter fixture acquired");};
    capture();auto first=calculate(probe);auto input=command(service,ref,probe);
    auto result=service.saveAndUse(input,first,probe,[]{return true;});
    check(result.state==CalibrationSaveUseState::Activated && !result.duplicate && !result.independentControlAccepted,"single command native fixture, control absent");
    const auto frozen=service.snapshots({ref}).first().calibration;const auto committed=service.environment().encode();
    result=service.saveAndUse(input,first,probe,[]{return true;});
    check(result.state==CalibrationSaveUseState::Activated && result.duplicate && service.records(ref).size()==1 && service.environment().encode()==committed,"identical completed command idempotent despite its own revision increment");
    check(service.saveAndUse(command(service,ref,probe),first,probe,[]{return true;}).state==CalibrationSaveUseState::Activated,"idempotent caller may refresh its environment revision");
    capture();auto replacement=calculate(probe,20);input=command(service,ref,probe);
    for(auto fault:{MeasurementEnvironmentRepository::Fault::AfterCandidate,MeasurementEnvironmentRepository::Fault::BeforeWrite,MeasurementEnvironmentRepository::Fault::BeforeCommit}){
        result=service.saveAndUse(input,replacement,probe,[]{return true;},fault);
        check(result.state==CalibrationSaveUseState::CandidateSaved && !result.error.isEmpty() && service.environment().encode()==committed && service.snapshots({ref}).first().calibration.selection==frozen.selection,"candidate persisted but failed activation preserves previous selection/permission");
    }
    int otherCalls=0;result=service.saveAndUse(input,replacement,probe,[&]{if(++otherCalls==3)service.disconnect(other.id);return true;});
    check(result.state==CalibrationSaveUseState::Activated && result.duplicate && service.records(ref).size()==2,"retry activates unchanged candidate once");
    check(otherCalls==3,"another camera disconnect does not retire this camera command");
    check(CalibrationModelRecord::decode(replacement.encode()).contentId()==replacement.contentId() && !replacement.controlAccepted(),"hash roundtrip does not create physical acceptance");
    capture();auto next=calculate(probe,30);input=command(service,ref,probe);const auto before=service.environment().encode();
    {QLockFile lock(repo.catalogPath()+".lock");check(lock.tryLock(0),"catalog lock fixture");result=service.saveAndUse(input,next,probe,[]{return true;});check(result.state==CalibrationSaveUseState::Failed && service.records(ref).size()==2,"candidate write failure is distinct");}
    auto stale=input;--stale.environmentRevision;check(service.saveAndUse(stale,next,probe,[]{return true;}).state==CalibrationSaveUseState::Failed,"stale revision rejected before write");
    auto foreign=input;foreign.target={other.id,otherSetup.id,1};check(service.saveAndUse(foreign,next,probe,[]{return true;}).state==CalibrationSaveUseState::Failed,"foreign camera target rejected");
    auto generation=input;++generation.generation;check(service.saveAndUse(generation,next,probe,[]{return true;}).state==CalibrationSaveUseState::Failed,"old capture generation rejected");
    ready=false;check(service.saveAndUse(input,next,probe,[]{return true;}).state==CalibrationSaveUseState::Failed,"Runtime busy rejected");ready=true;
    int calls=0;result=service.saveAndUse(input,next,probe,[&]{return ++calls<3;});
    check(result.state==CalibrationSaveUseState::CandidateSaved && service.environment().encode()==before && service.snapshots({ref}).first().calibration.record.contentId()==replacement.contentId(),"project/current target rechecked at final commit boundary");
    calls=0;result=service.saveAndUse(input,next,probe,[&]{if(++calls==3)ready=false;return true;});ready=true;
    check(result.state==CalibrationSaveUseState::CandidateSaved && service.environment().encode()==before,"late Runtime busy preserves previous selection");
    calls=0;result=service.saveAndUse(input,next,probe,[&]{if(++calls==3)probe.cancel();return true;});
    check(result.state==CalibrationSaveUseState::CandidateSaved && service.environment().encode()==before,"late Stop preserves previous selection");
    check(service.saveAndUse(input,next,probe,[]{return true;}).state==CalibrationSaveUseState::Failed,"retired probe cannot retry");
    capture();input=command(service,ref,probe);calls=0;result=service.saveAndUse(input,next,probe,[&]{if(++calls==3)service.disconnect(camera.id);return true;});
    check(result.state==CalibrationSaveUseState::CandidateSaved && service.environment().encode()==before,"late disconnect preserves remembered choice and prevents publishing permission");
    rejects([&]{service.snapshots({ref});});check(frozen.record.encode()==first.encode(),"copied Runtime snapshot immutable");
    capture();input=command(service,ref,probe);CalibrationControl control;control.limitMillimetres=.001;control.samples.append({"holdout",{{10,10},next.setup().source},{{20,10},next.setup().source},100});
    auto rejected=CalibrationModelRecord::create(next.model(),next.setup(),next.image(),control);
    result=service.saveAndUse(input,rejected,probe,[]{return true;});
    // Same model/version, different content cannot replace its immutable candidate.
    check(result.state==CalibrationSaveUseState::Failed && service.environment().encode()==before,"immutable version conflict cannot overwrite control or selection");
    capture();auto controlledBase=calculate(probe,40);rejected=CalibrationModelRecord::create(controlledBase.model(),controlledBase.setup(),controlledBase.image(),control);input=command(service,ref,probe);
    result=service.saveAndUse(input,rejected,probe,[]{return true;});
    check(result.state==CalibrationSaveUseState::CandidateSaved && result.independentControlAccepted==std::optional(false) && service.environment().encode()==before,"failed independent control retained and cannot activate");
    capture();auto last=calculate(probe,50);input=command(service,ref,probe);calls=0;
    result=service.saveAndUse(input,last,probe,[&]{if(++calls==3){reg.reload();auto contract=reg.snapshot().current;auto changed=contract.setups[0];changed.workingPlane="new-plane";contract.setups[0]=contract.setups[0].revised(changed);reg.replace(contract);}return true;});
    check(result.state==CalibrationSaveUseState::CandidateSaved && service.environment().selections.first().contentId==replacement.contentId(),"late setup revision cannot select stale candidate");
    check(service.saveAndUse(input,last,probe,[]{return true;}).state==CalibrationSaveUseState::Failed,"stale exact setup rejected");
}
void experimentalChecks(const QString& path){
    auto r=request();auto deviceState=state();
    MeasurementEnvironmentRepository repo(path,"experimental");MeasurementEnvironmentRegistry reg(repo);reg.replace({{r.camera},{r.setup},{r.profile}});
    ScopedActiveCalibrationService service(repo,[]{return true;});const SetupReference ref{r.camera.id,r.setup.id,1};service.setOperational(ref.camera,ref.setup);
    IndependentCaptureRequest capture;capture.origin=SourceOrigin::CameraAcquired;capture.camera=r.camera;capture.setup=r.setup;capture.profile=r.profile;capture.policy=CalibrationCapturePolicy::ExperimentalOecs;capture.timeoutMs=3000;
    IndependentCaptureBackend backend;backend.industrialDiscover=[r]{return IndustrialDiscoverySnapshot{{r.device},{},false};};backend.experimentalApi=[deviceState]{return std::make_unique<Api>(deviceState);};
    IndependentCalibrationSession probe;auto acquire=[&]{
        // The SDK fixture poisons its released buffer; each new sensor exposure
        // must receive its own stripe before acquisition, just like CCUX-07.
        deviceState->pixels.fill(char(20));for(int y=0;y<2048;++y)for(int x=120;x<520;++x)deviceState->pixels[y*2448+x]=char(220);
        const auto result=IndependentCalibrationSession::run(probe.begin(capture),backend);if(!result.error().isEmpty())throw std::runtime_error(result.error().toStdString());check(probe.accept(result),"experimental live fixture");
    };
    acquire();auto model=calculate(probe);auto input=command(service,ref,probe,CalibrationCapturePolicy::ExperimentalOecs);
    auto result=service.saveAndUse(input,model,probe,[]{return true;});
    check(result.state==CalibrationSaveUseState::Activated && result.experimental && !result.independentControlAccepted,"separate experimental activation");
    const auto snapshot=service.experimentalSnapshots({ref}).first().snapshot.calibration;
    check(snapshot.record.encode()==model.encode() && snapshot.acquisition["triggerAssociationVerified"]==false && snapshot.acquisition["productionAdmission"]==false && snapshot.acquisition["physicalAcceptance"]==false,"experimental selection retains immutable model and false admission");
    check(snapshot.acquisition["probe"].toObject()["experimental"].toObject()["acquisition"].toObject()["triggerAssociationVerified"]==false,"underlying unverified trigger never upgraded");
    rejects([&]{service.snapshots({ref});});rejects([&]{service.activate(ref,model.contentId(),probe);});rejects([&]{SystemCalibration::evidence(snapshot,"source","source/image");});
    {
        const auto captured=*probe.math().frame();const auto meta=captured.metadata();const auto replay=path+"-editor.pgm";ExperimentalOecsAcquisition::save({captured,meta.sourceId,meta.cycleId,meta.frameId,meta.completedAtUtc},replay);
        ProjectService project;project.createProject("Experimental Editor","",path+"-editor.vfproject","file");const auto recipe=project.createRecipe("Metric","",ApplicationType::Measurement,"file");const auto source=project.setImageFile(replay);project.enableMultiSource();project.bindCameraSource(source,service.environment(),ref.camera);
        const auto left=project.addTool("edge_point"),right=project.addTool("edge_point"),metric=project.addTool("measurement");
        for(auto t:project.tools())if(t.id==left || t.id==right){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();t.parameters["polarity"]=t.id==left ? "rising":"falling";project.updateTool(t);}
        project.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",metric,"p1"},{right,"point",metric,"p2"}});
        const auto typed=service.experimentalSnapshots({ref}).first();const auto branch=SystemCalibration::branch(*project.activeRecipe(),source);const auto resolve=[typed,branch](const ImageFrame& frame){return SystemCalibration::resolveExperimental(typed,branch,frame);};
        EditorSession editor;EditorContext context{"experimental",project.path(),recipe,project.activeRecipe()->definition,{},{{source,QJsonObject{{"experimental",true},{"contentId",typed.snapshot.calibration.selection.contentId}}}}};const auto signature=editor.signature(context);
        const auto execution=ExecutionService{}.withExperimentalEditorReplay();const QJsonObject acquisition{{"sources",QJsonObject{{source,QJsonObject{{"target",ref.toJson()},{"device",r.camera.device.toJson()},{"geometry",r.setup.geometry.toJson()}}}}}};const auto cycle=execution.run(context.definition,QFileInfo(path).absolutePath(),recipe,signature,{}, {},{},resolve,nullptr,acquisition);
        if(cycle.state!=ExecutionState::Done){QTextStream(stderr)<<cycle.error<<"\n";for(const auto& t:cycle.tools)QTextStream(stderr)<<t.toolId<<": "<<t.message<<" "<<t.errorCode<<"\n";}check(cycle.state==ExecutionState::Done,"Experimental Editor replay metric execution");bool measured=false;for(const auto& result:cycle.tools)if(result.toolId==metric)measured=result.outputs["value"].valid && std::abs(std::get<double>(result.outputs["value"].data)-10)<.001 && !result.outputs["value"].origin.experimentalAcquisition.isEmpty();check(measured,"Experimental Editor real fixture 10mm preserves origin");
        const auto ticket=editor.begin(SessionJob::Execution);check(editor.acceptCycle(cycle,signature,context),"Experimental Editor accepts actual replay");editor.finish(ticket);const auto target=execution.runTool(context.definition,QFileInfo(path).absolutePath(),recipe,signature,{},metric,editor.evaluation(context,metric),resolve);check(target.state==ExecutionState::Done && target.acquisitions==0,"Experimental mm TargetOnly does not acquire");
        auto single=branch;auto singleEditor=single.definition["editor"].toObject();singleEditor["schemaVersion"]=8;single.definition["editor"]=singleEditor;validateEditorDefinition(single.definition);const auto singleResolver=[typed,single](const ImageFrame& frame){return SystemCalibration::resolveExperimental(typed,single,frame);};const auto singleCycle=execution.run(single.definition,QFileInfo(path).absolutePath(),recipe,"single",{}, {},{},singleResolver,nullptr,acquisition["sources"].toObject()[source].toObject());check(singleCycle.state==ExecutionState::Done,"Legacy single-source experimental replay metric execution");
        auto liveRecipe=branch;auto liveTools=editorTools(liveRecipe.definition);for(auto& tool:liveTools)if(tool.id==source){tool.kind="industrial_camera";tool.parameters={{"sourceReference",IndustrialSourceReference::bind(service.environment().current,r.profile.id,5000).toJson()}};}liveRecipe.definition=withEditorTools(liveRecipe.definition,liveTools);auto liveMeta=captured.metadata();liveMeta.sourceId=source;liveMeta.coordinateSystemId=source+"/image";const auto liveFrame=captured.withMetadata(liveMeta);check(SystemCalibration::resolveExperimental(typed,liveRecipe,liveFrame).calibration!=nullptr,"Experimental actual live fixture resolves same metric model");
        auto forged=liveMeta;forged.experimentalAcquisition["serial"]="wrong";check(!SystemCalibration::resolveExperimental(typed,liveRecipe,captured.withMetadata(forged)).calibration,"Wrong serial cannot resolve experimental live mm");
        auto wrong=typed;wrong.snapshot.target.camera={"wrong-camera"};const auto rejected=SystemCalibration::resolveExperimental(wrong,branch,*editor.frame);check(!rejected.calibration && !rejected.error.isEmpty(),"Foreign experimental camera cannot resolve mm");
        check(!SystemCalibration::resolve(snapshot,branch,QFileInfo(path).absolutePath(),*editor.frame).calibration,"Experimental model cannot enter trusted resolver");
        auto raw=*editor.frame;auto metadata=raw.metadata();metadata.experimentalAcquisition={};raw=raw.withMetadata(metadata);check(!resolve(raw).calibration,"Unmarked raster cannot use experimental mm");
        auto changed=context;changed.systemCalibration[source]=QJsonObject{{"contentId","new-model"}};check(editor.evaluation(changed,metric).cache.isEmpty(),"Experimental model identity invalidates cached mm");
    }
    check(service.status(ref).experimental && service.status(ref).sessionPermitted,"status labels experimental permission");
    check(service.saveAndUse(input,model,probe,[]{return true;}).state==CalibrationSaveUseState::Activated,"experimental duplicate idempotent");
    ScopedActiveCalibrationService restart(repo,[]{return true;});check(restart.status(ref).experimental && restart.status(ref).mathematicsCompatible && !restart.status(ref).sessionPermitted,"restart retains experimental mathematics without opt-in/permission");rejects([&]{restart.experimentalSnapshots({ref});});
    acquire();restart.prepareRemembered(ref,probe);check(restart.status(ref).sessionPermitted,"normal fresh probe restores remembered mathematics without opt-in");restart.disconnect(ref.camera);
    service.setExperimental(true);const auto previous=service.environment().encode();acquire();auto next=calculate(probe,20);input=command(service,ref,probe,CalibrationCapturePolicy::ExperimentalOecs);int calls=0;
    result=service.saveAndUse(input,next,probe,[&]{if(++calls==3)service.setExperimental(false);return true;});
    check(result.state==CalibrationSaveUseState::CandidateSaved && service.environment().encode()==previous,"late opt-in change preserves previous selection and revokes permission");rejects([&]{service.experimentalSnapshots({ref});});
    service.setExperimental(true);check(!service.status(ref).sessionPermitted,"re-enabling opt-in cannot resurrect old permission");
    acquire();input=command(service,ref,probe,CalibrationCapturePolicy::ExperimentalOecs);result=service.saveAndUse(input,next,probe,[]{return true;});
    check(result.state==CalibrationSaveUseState::Activated && result.duplicate,"fresh experimental probe can retry immutable candidate");
    const auto image=*probe.math().frame();const auto meta=image.metadata();const auto pgm=path+".pgm";ExperimentalOecsAcquisition::save({image,meta.sourceId,meta.cycleId,meta.frameId,meta.completedAtUtc},pgm);
    auto replay=capture;replay.origin=SourceOrigin::SetupFile;replay.filePath=pgm;IndependentCalibrationSession file;
    check(file.accept(IndependentCalibrationSession::run(file.begin(replay),backend)),"exact PGM JSON replay");auto replayModel=calculate(file,30);
    auto fileInput=command(service,ref,file,CalibrationCapturePolicy::ExperimentalOecs);result=service.saveAndUse(fileInput,replayModel,file,[]{return true;});
    check(result.state==CalibrationSaveUseState::CandidateSaved,"replay candidate still requires actual live activation probe");
    acquire();input=command(service,ref,probe,CalibrationCapturePolicy::ExperimentalOecs);result=service.saveAndUse(input,replayModel,probe,[]{return true;});
    check(result.state==CalibrationSaveUseState::Activated && result.duplicate && service.experimentalSnapshots({ref}).first().snapshot.calibration.record.image().origin=="experimental_setup_file","replay model plus independent live probe retains file origin");
    MeasurementEnvironmentRepository imported(path+".import","copy");imported.importBundle(repo.exportBundle());ScopedActiveCalibrationService copy(imported,[]{return true;});check(copy.records(ref).last().encode()==replayModel.encode() && !copy.status(ref).sessionPermitted,"experimental hash/selection roundtrip without imported permission");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir directory(QDir::currentPath()+"/ccux08-XXXXXX");check(directory.isValid(),"build fixtures");nativeChecks(directory.filePath("native.vfproject"));experimentalChecks(directory.filePath("experimental.vfproject"));
    QTextStream(stdout)<<"PASS: save/use native and experimental fixtures, partial persistence/retry/duplicate, commit guards, isolation, immutable hash; no physical acceptance\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
