#include "IndustrialProviderFixture.hpp"
#include "FramedModelFixture.hpp"
#include "OecsExperimentalFixture.hpp"
#include "application/MeasurementWorkspace.hpp"
#include "application/ProjectService.hpp"
#include "application/RuntimeCoordinator.hpp"
#include <QTemporaryDir>
#include <QFile>
#include <QDirIterator>
#include <thread>
using namespace visionflow;
namespace {
int checks=0;
void expect(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
template<class F> void refuse(F f){++checks;try{f();}catch(const std::exception&){return;}throw std::runtime_error("Expected rejection");}
// Error class of a ProductionException, or -1 when nothing (or something else) was thrown.
template<class F> int codeOf(F f){try{f();}catch(const ProductionException& e){return int(e.code());}catch(...){}return -1;}
template<class F> QString reasonOf(F f){try{f();}catch(const std::exception& e){return QString::fromUtf8(e.what());}return {};}
QImage stripe(){QImage image(640,200,QImage::Format_Grayscale8);image.fill(20);for(int y=0;y<200;++y)for(int x=120;x<520;++x)image.scanLine(y)[x]=220;return image;}
class Adapter:public AcquisitionAdapter {
public:
    AcquisitionCapabilities capabilities()const override{return {"webcam",true};}
    ImageFrame acquire(const AcquisitionRequest&)const override{auto image=stripe();return ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),640,200,image.bytesPerLine(),PixelFormat::Gray8);}
};
using Svc=SourceVerificationService;
Svc::Result worker(const Svc::Job& job,const IndependentCaptureBackend& backend){Svc::Result result;std::thread([&]{result=Svc::run(job,backend);}).join();return result;}
void drain(Svc& svc){Deadline d(5000);while(!svc.poll()){if(d.expired())throw std::runtime_error("Close drain timeout");std::this_thread::sleep_for(std::chrono::milliseconds(5));}expect(true,"Close drained");}
QByteArray bytes(const QString& path){QFile f(path);return f.open(QIODevice::ReadOnly) ? f.readAll():QByteArray("missing");}
// Every file under a directory with its size: proves nothing was created or rewritten.
QStringList listing(const QString& root){QStringList out;QDirIterator it(root,QDir::Files,QDirIterator::Subdirectories);while(it.hasNext()){it.next();out<<it.filePath()+":"+QString::number(it.fileInfo().size());}out.sort();return out;}
void teach(MeasurementWorkspace& workspace,const MeasurementCamera& camera,const MeasurementSetupDescriptor& setup){
    IndependentCaptureRequest request;request.origin=SourceOrigin::CameraAcquired;request.camera=camera;request.setup=setup;
    IndependentCaptureBackend backend{[&]{return QVector<DeviceBinding>{camera.device};},[](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<Adapter>();}};
    IndependentCalibrationSession session;expect(session.accept(IndependentCalibrationSession::run(session.begin(request),backend)),"Teach actual probe");
    auto& math=session.math();expect(math.accept(CalibrationWizardSession::run(math.scanPair("width",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"Teach caliper");math.setKnownMillimetres(10);
    auto record=math.calculate().named("Scale");SetupReference ref{camera.id,setup.id,setup.revision};workspace.calibrations().save(ref,record);workspace.calibrations().setOperational(camera.id,setup.id);workspace.calibrations().activate(ref,record.contentId(),session);
}
// n webcam sources (the last one raw when mixed). sameSession keeps the teaching session permission;
// otherwise the verifying workspace is fresh, exactly like a restarted application.
struct Rig {
    int n;bool mixed;QString dir,path,runtimeDir;
    MeasurementSetupContract contract;ProjectService project;Recipe current;QStringList sources;
    std::unique_ptr<MeasurementWorkspace> teaching,fresh;MeasurementWorkspace* workspace=nullptr;
    bool idle=true;int discoveries=0;QString failDevice;IndependentCaptureBackend backend;
    std::unique_ptr<RuntimeService> runtime;std::unique_ptr<RuntimeCoordinator> coordinator;
    Rig(const QString& root,const QString& name,int count,bool mixedSet,bool sameSession=false,ApplicationType type=ApplicationType::Measurement):n(count),mixed(mixedSet),dir(root),path(root+"/"+name+".vfproject"),runtimeDir(root+"/runtime-"+name){
        project.createProject("Verify","",path,"file");project.save();
        teaching=std::make_unique<MeasurementWorkspace>(path,project.project().id,[]{return true;});
        for(int i=0;i<n;++i){const auto id=QString::number(i);contract.cameras.append({{"c"+id},"Camera",{"webcam","provider-"+id,DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}});contract.setups.append({{"s"+id},contract.cameras.last().id,"Setup","lens","plane",1,1,{{640,200},{},{},true}});}
        teaching->replace(contract);for(int i=0;i<n;++i)teach(*teaching,contract.cameras[i],contract.setups[i]);
        project.createRecipe("Recipe","",type,"file");if(n>1)project.enableMultiSource();
        QVector<ToolConnection> links;
        for(int i=0;i<n;++i){const auto source=project.addTool("webcam");sources.append(source);
            const auto left=project.addTool("edge_point"),right=project.addTool("edge_point"),metric=project.addTool(mixed && i==n-1 ? "calculate":"measurement");
            for(auto tool:project.tools())if(tool.id==left || tool.id==right){tool.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();tool.parameters["polarity"]=tool.id==left ? "rising":"falling";project.updateTool(tool);}
            links<<ToolConnection{source,"image",left,"image"}<<ToolConnection{source,"image",right,"image"}<<ToolConnection{left,"point",metric,"p1"}<<ToolConnection{right,"point",metric,"p2"};project.bindSourceSetup(source,SetupReference{contract.cameras[i].id,contract.setups[i].id,1}.toJson());
        }
        project.setConnections(links);project.save();current=*project.activeRecipe();
        if(sameSession)workspace=teaching.get();else{fresh=std::make_unique<MeasurementWorkspace>(path,project.project().id,[this]{return idle;});workspace=fresh.get();}
        runtime=std::make_unique<RuntimeService>(runtimeDir,path+".calibrations.json",RuntimeIdentity::localEngineer(),RuntimeService::SystemProvider{},[this](const Recipe& r){return workspace->forRecipe(r);},[this](const Recipe& r){return workspace->acquisition(r);},[this](const Recipe& r){return workspace->productionSources(r);});
        runtime->setConfigurationProviders([this](const Recipe& r){return workspace->configurationForRecipe(r);},[this](const Recipe& r){return workspace->configurationSources(r);});
        runtime->setSourceVerification(workspace->sourceVerification());coordinator=std::make_unique<RuntimeCoordinator>(*runtime);
        backend={[this]{++discoveries;QVector<DeviceBinding> devices;for(const auto& camera:contract.cameras)if(camera.device.providerId!=failDevice)devices.append(camera.device);return devices;},[](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<Adapter>();}};
    }
    SetupReference ref(int i) const {return {contract.cameras[i].id,contract.setups[i].id,1};}
    Svc& svc(){return *workspace->sourceVerification();}
    Svc::Job begin(){return workspace->verifySources([this]{return current;},dir,backend);}
    // One full worker round trip on the owner thread's accept/poll.
    bool verify(){auto job=begin();return svc().accept(worker(job,backend));}
    void rawOnly(){auto tools=editorTools(current.definition);for(auto& tool:tools)if(tool.kind=="measurement"){tool.kind="calculate";tool.parameters=defaultParameters("calculate");}current.definition=withEditorTools(current.definition,tools);}
};
// Industrial fixture provider whose camera close can be held and whose creations are counted.
class HoldBackend final:public IndustrialCameraBackend {
    std::unique_ptr<IndustrialCameraBackend> inner;std::shared_ptr<std::atomic_bool> hold;
    void wait() const {Deadline d(10000);while(hold->load() && !d.expired())std::this_thread::sleep_for(std::chrono::milliseconds(2));}
public:
    HoldBackend(std::unique_ptr<IndustrialCameraBackend> backend,std::shared_ptr<std::atomic_bool> flag):inner(std::move(backend)),hold(std::move(flag)){}
    PhysicalDeviceIdentity actualIdentity() const override {return inner->actualIdentity();}
    AcquisitionReadback readback() const override {return inner->readback();}
    CameraWorkerResult execute(const CameraTicket& ticket) override {if(ticket.operation==CameraOperation::Disconnect)wait();return inner->execute(ticket);}
    void shutdown() override {wait();inner->shutdown();}
};
class CountingProvider final:public IndustrialCameraProvider {
    std::shared_ptr<const IndustrialCameraProvider> inner;std::shared_ptr<std::atomic_bool> hold;std::shared_ptr<std::atomic_int> created;
public:
    CountingProvider(std::shared_ptr<const IndustrialCameraProvider> provider,std::shared_ptr<std::atomic_bool> flag,std::shared_ptr<std::atomic_int> count):inner(std::move(provider)),hold(std::move(flag)),created(std::move(count)){}
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice& device) const override {return inner->capabilities(device);}
    std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest& request) const override {++*created;return std::make_unique<HoldBackend>(inner->create(request),hold);}
};
// One physical industrial fixture camera, one remembered model; profileOf[i] picks the profile (0/1) of logical source i.
struct IndustrialRig {
    IndustrialCameraRequest r=industrial_provider_fixture::request();
    std::shared_ptr<industrial_provider_fixture::State> state=std::make_shared<industrial_provider_fixture::State>();
    std::shared_ptr<std::atomic_bool> hold=std::make_shared<std::atomic_bool>(false);std::shared_ptr<std::atomic_int> created=std::make_shared<std::atomic_int>(0);
    QString dir,path;ProjectService project;Recipe current;QStringList sources;SetupReference ref;bool idle=true;
    std::unique_ptr<MeasurementWorkspace> teaching,workspace;IndependentCaptureBackend backend;IndustrialExecutionInputs inputs;
    IndustrialRig(const QString& root,const QString& name,QVector<int> profileOf):dir(root),path(root+"/"+name+".vfproject"){
        const auto raster=stripe();state->raster=raster;r.setup.geometry.resolution=raster.size();r.setup.geometry.sensorRoi=QRect(QPoint(0,0),raster.size());
        auto second=r.profile;second.id={"profile2"};second.name="Second profile";ref={r.camera.id,r.setup.id,1};
        project.createProject("Industrial verify","",path,"file");project.save();
        teaching=std::make_unique<MeasurementWorkspace>(path,project.project().id,[]{return true;});teaching->replace({{r.camera},{r.setup},{r.profile,second}});
        backend.factory=CameraProviderFactory::forTests(std::make_shared<CountingProvider>(std::make_shared<industrial_provider_fixture::Provider>(state),hold,created));
        backend.industrialDiscover=[device=r.device]{return IndustrialDiscoverySnapshot{{device},{},false};};
        IndependentCaptureRequest request;request.origin=SourceOrigin::CameraAcquired;request.camera=r.camera;request.setup=r.setup;request.profile=r.profile;
        IndependentCalibrationSession session;expect(session.accept(IndependentCalibrationSession::run(session.begin(request),backend)),"Teach industrial probe");
        auto& math=session.math();expect(math.accept(CalibrationWizardSession::run(math.scanPair("width",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"Teach industrial caliper");math.setKnownMillimetres(10);
        teaching->calibrations().setOperational(r.camera.id,r.setup.id);
        expect(teaching->saveAndUseCalibration({ref,teaching->environment().revision,session.evidence()->generation},math.calculate(),session,[]{return true;}).state==CalibrationSaveUseState::Activated,"Teach industrial activation");
        project.createRecipe("Industrial","",ApplicationType::Measurement,"file");if(profileOf.size()>1)project.enableMultiSource();
        QVector<ToolConnection> links;const auto contract=teaching->environment().current;
        for(const int profile:profileOf){const auto source=project.addTool("industrial_camera");sources.append(source);project.bindIndustrialSource(source,contract,profile ? second.id:r.profile.id,1000);project.bindSourceSetup(source,ref.toJson());
            const auto left=project.addTool("edge_point"),right=project.addTool("edge_point"),metric=project.addTool("measurement");
            for(auto tool:project.tools())if(tool.id==left || tool.id==right){tool.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();tool.parameters["polarity"]=tool.id==left ? "rising":"falling";project.updateTool(tool);}
            links<<ToolConnection{source,"image",left,"image"}<<ToolConnection{source,"image",right,"image"}<<ToolConnection{left,"point",metric,"p1"}<<ToolConnection{right,"point",metric,"p2"};
        }
        project.setConnections(links);project.save();current=*project.activeRecipe();
        inputs.contract=contract;inputs.factory=backend.factory;inputs.discover=[device=r.device](int,const auto&){return IndustrialDiscoverySnapshot{{device},{},false};};
        workspace=std::make_unique<MeasurementWorkspace>(path,project.project().id,[this]{return idle;});created->store(0); // teaching creations do not count
    }
    ~IndustrialRig(){hold->store(false);}
    Svc& svc(){return *workspace->sourceVerification();}
    Svc::Job begin(const IndependentCaptureBackend& b){return workspace->verifySources([this]{return current;},dir,b);}
};
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
QTemporaryDir dir(QDir::currentPath()+"/int05-source-verification-XXXXXX");expect(dir.isValid(),"Build fixture directory");
for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance})for(int n:{1,2,10}){
    Rig rig(dir.path(),QString("ready-%1-%2").arg(int(type)).arg(n),n,false,false,type);auto& svc=rig.svc();
    expect(svc.admissionAllowed() && svc.state()==Svc::State::Waiting,"Never-invoked verification is Waiting but admits the legacy path");
    const auto environmentFile=rig.path+".measurement-environment.json",catalogFile=rig.path+".active-calibrations.json";
    const auto environmentBefore=bytes(environmentFile),catalogBefore=bytes(catalogFile);const auto filesBefore=listing(dir.path());
    refuse([&]{rig.workspace->calibrations().snapshots({rig.ref(0)});});
    auto job=rig.begin();expect(job.probeCount()==n,"One probe per physical webcam source");
    const auto result=worker(job,rig.backend);expect(svc.accept(result),"Completion consumed");
    const QByteArray readyMessage=QString("%1-source set ready").arg(n).toUtf8();
    expect(svc.state()==Svc::State::Ready && svc.probeCount()==n && svc.admissionAllowed() && svc.errors().isEmpty() && rig.workspace->productionSources(rig.current).sources.size()==n,readyMessage.constData());
    for(int i=0;i<n;++i)expect(rig.workspace->calibrations().snapshots({rig.ref(i)}).size()==1,"Every source permission renewed");
    expect(bytes(environmentFile)==environmentBefore && bytes(catalogFile)==catalogBefore && listing(dir.path())==filesBefore && !rig.runtime->active() && rig.runtime->records().isEmpty(),"verification has no production side effects");
    expect(!svc.accept(result),"Duplicate completion rejected");
    if(n==2){
        rig.coordinator->load(rig.current,dir.path());rig.coordinator->stop();
        expect(svc.state()==Svc::State::Ready && svc.admissionAllowed(),"Stop keeps completed Ready");
        expect(rig.workspace->productionSources(rig.current).sources.size()==2,"Ready set still admits after Stop");
    }
}
// Raw sources never probe and never receive a sibling's model.
for(int n:{1,2,10}){
    Rig rig(dir.path(),QString("mixed-%1").arg(n),n,true);auto& svc=rig.svc();const int probes=n-1;
    rig.workspace->calibrations().disconnect(rig.contract.cameras[n-1].id);
    auto job=rig.begin();bool ok=job.probeCount()==probes;expect(svc.accept(worker(job,rig.backend)) && svc.state()==Svc::State::Ready,"Mixed raw+metric set completes");
    refuse([&]{rig.workspace->calibrations().snapshots({rig.ref(n-1)});});
    if(probes)ok=ok && rig.workspace->calibrations().snapshots({rig.ref(0)}).size()==1;
    expect(ok,"raw sources never probe");
}
{
    Rig rig(dir.path(),"allraw",2,false);rig.rawOnly();rig.workspace->calibrations().disconnect(rig.contract.cameras[0].id);rig.workspace->calibrations().disconnect(rig.contract.cameras[1].id);
    auto job=rig.begin();const bool none=job.probeCount()==0;expect(rig.svc().accept(worker(job,rig.backend)),"Raw-only completion consumed");
    expect(none && rig.svc().state()==Svc::State::Ready && rig.workspace->productionSources(rig.current).sources.size()==2 && rig.discoveries==0,"raw sources never probe");
    refuse([&]{rig.workspace->calibrations().snapshots({rig.ref(0)});});
}
// Partial success never admits the whole set.
{
    Rig rig(dir.path(),"partial",3,false);auto& svc=rig.svc();rig.failDevice=rig.contract.cameras[2].device.providerId;
    auto job=rig.begin();expect(svc.accept(worker(job,rig.backend)),"Failed completion still consumed");
    const auto& errors=svc.errors();
    expect(svc.state()==Svc::State::Failed && errors.contains(rig.sources[2]) && !errors.contains(rig.sources[0]) && !svc.admissionAllowed()
        && rig.workspace->calibrations().snapshots({rig.ref(0),rig.ref(1)}).size()==2 && !reasonOf([&]{rig.workspace->productionSources(rig.current);}).isEmpty()
        && codeOf([&]{rig.runtime->requireReady();})==int(ProductionError::DependencyChanged) && codeOf([&]{rig.runtime->begin();})==int(ProductionError::DependencyChanged)
        && codeOf([&]{rig.runtime->prepare(rig.current,dir.path());})==int(ProductionError::DependencyChanged),"partial success never admits the set");
    rig.failDevice.clear();job=rig.begin();expect(svc.accept(worker(job,rig.backend)) && svc.state()==Svc::State::Ready,"Retry restores complete readiness");
}
// busy gate
{
    Rig rig(dir.path(),"busy",2,false);auto& svc=rig.svc();
    rig.idle=false;const auto gateReason=reasonOf([&]{rig.begin();});rig.idle=true;
    auto job=rig.begin();
    const bool blocked=!gateReason.isEmpty() && svc.busy() && svc.state()==Svc::State::Verifying && !rig.workspace->changeAllowed() && !reasonOf([&]{rig.begin();}).isEmpty()
        && codeOf([&]{rig.runtime->requestCameraPreparation(rig.current,dir.path());})==int(ProductionError::Busy) && codeOf([&]{rig.runtime->prepare(rig.current,dir.path());})==int(ProductionError::Busy);
    refuse([&]{rig.workspace->replace(rig.contract);});
    refuse([&]{rig.workspace->calibrations().setExperimental(true);});
    expect(reasonOf([&]{rig.workspace->discover();})=="Source verification worker owns acquisition","discovery refused while verification owns acquisition");
    auto result=worker(job,rig.backend);rig.idle=false;expect(svc.accept(result),"Completion consumed under closed gate");rig.idle=true;
    expect(blocked && !svc.busy() && svc.state()==Svc::State::Failed && svc.errors().contains("sourceSet") && rig.workspace->changeAllowed(),"busy gate");
}
// Stop while the worker is pending keeps ownership until the worker returns.
{
    Rig rig(dir.path(),"stop",2,false);auto& svc=rig.svc();rig.coordinator->load(rig.current,dir.path());
    auto job=rig.begin();auto result=worker(job,rig.backend);rig.coordinator->stop();
    const bool owned=svc.busy() && rig.coordinator->state()==RuntimeState::Stopping && codeOf([&]{rig.coordinator->load(rig.current,dir.path());})==int(ProductionError::Busy) && !reasonOf([&]{rig.begin();}).isEmpty();
    expect(svc.accept(result),"Cancelled completion consumed");
    expect(owned && !svc.busy() && svc.state()==Svc::State::Cancelled && !svc.admissionAllowed(),"cancel keeps ownership until worker returns");
    refuse([&]{rig.workspace->calibrations().snapshots({rig.ref(0)});});rig.coordinator->poll();
    expect(rig.coordinator->state()==RuntimeState::LoadedStopped,"Stop ends only after verification owner releases acquisition");rig.coordinator->load(rig.current,dir.path());
}
// Late and duplicate completions.
{
    Rig rig(dir.path(),"stale",2,false);auto& svc=rig.svc();
    auto first=rig.begin();auto old=worker(first,rig.backend);svc.cancel();expect(svc.accept(old),"Old completion drains the cancelled job");
    auto second=rig.begin();const bool lateRejected=!svc.accept(old) && svc.busy() && svc.state()==Svc::State::Verifying;
    auto fresh=worker(second,rig.backend);const bool consumed=svc.accept(fresh);const bool duplicate=!svc.accept(fresh);
    expect(lateRejected && consumed && duplicate && svc.state()==Svc::State::Ready,"stale completion rejected");
}
// Fingerprint invalidates Ready.
{
    Rig rig(dir.path(),"fingerprint",2,false);auto& svc=rig.svc();expect(rig.verify() && svc.state()==Svc::State::Ready,"Fingerprint baseline ready");
    auto foreign=rig.current;foreign.name="Different recipe";const bool foreignRefused=!svc.admissionAllowedFor(foreign,dir.path()) && svc.admissionAllowedFor(rig.current,dir.path());
    rig.current.name="Recipe changed";const bool recipeWaiting=svc.state()==Svc::State::Waiting && !svc.admissionAllowed() && !reasonOf([&]{rig.workspace->productionSources(rig.current);}).isEmpty();rig.current.name="Recipe";
    const bool restored=svc.state()==Svc::State::Ready;
    auto job=rig.begin();auto result=worker(job,rig.backend);rig.current.name="Recipe changed";expect(svc.accept(result),"Changed-recipe completion consumed");rig.current.name="Recipe";
    const bool completion=svc.state()==Svc::State::Failed && svc.errors().contains("sourceSet");
    expect(rig.verify() && svc.state()==Svc::State::Ready,"Re-verification after failure");
    auto changed=rig.contract;changed.cameras[0].name="External dependency change";MeasurementEnvironmentRegistry(MeasurementEnvironmentRepository(rig.path,rig.project.project().id)).replace(changed);
    const bool registry=svc.state()==Svc::State::Waiting && !svc.admissionAllowed();
    expect(foreignRefused && recipeWaiting && restored && completion && registry,"fingerprint invalidates Ready");
    rig.workspace->calibrations().disconnect(rig.contract.cameras[0].id);expect(rig.verify() && svc.state()==Svc::State::Ready,"Verification renews a disconnected permission");
    rig.workspace->calibrations().disconnect(rig.contract.cameras[0].id);expect(svc.state()==Svc::State::Waiting,"Permission invalidation returns to Waiting");
}
// Legacy same-session path is unchanged until verification is invoked.
{
    Rig rig(dir.path(),"legacy",2,false,true);auto& svc=rig.svc();
    rig.coordinator->load(rig.current,dir.path());rig.runtime->prepare(rig.current,dir.path());rig.runtime->requireReady();
    expect(svc.admissionAllowed() && !svc.busy() && rig.workspace->productionSources(rig.current).sources.size()==2 && rig.workspace->changeAllowed(),"legacy path unchanged without verification");
}
// Two logical industrial sources on one physical camera.
{
    IndustrialRig rig(dir.path(),"shared",{0,0});auto& svc=rig.svc();
    auto job=rig.begin(rig.backend);const bool single=job.probeCount()==1;
    const bool consumed=svc.accept(worker(job,rig.backend));drain(svc);expect(consumed && svc.state()==Svc::State::Ready && svc.errors().isEmpty(),"Shared camera completes");
    expect(single && svc.probeCount()==1 && rig.workspace->productionSources(rig.current,rig.inputs).sources.size()==2 && rig.workspace->calibrations().snapshots({rig.ref}).size()==1,"shared camera single probe");
    IndustrialRig conflict(dir.path(),"conflict",{0,1});
    const auto reason=reasonOf([&]{conflict.begin(conflict.backend);});
    expect(reason.contains("Conflicting setup/profile") && !conflict.svc().busy() && conflict.created->load()==0 && conflict.svc().state()==Svc::State::Waiting,"conflicting profile rejected");
}
// The physical close must really drain before the set is admitted.
{
    IndustrialRig rig(dir.path(),"drain",{0});auto& svc=rig.svc();
    RuntimeService runtime(dir.path()+"/runtime-drain",{},RuntimeIdentity::localEngineer(),{}, {}, {},[&](const Recipe& r){return rig.workspace->productionSources(r,rig.inputs);});runtime.setSourceVerification(rig.workspace->sourceVerification());
    rig.hold->store(true);auto job=rig.begin(rig.backend);auto result=worker(job,rig.backend);
    expect(svc.accept(result),"Held completion consumed");
    const bool held=svc.busy() && !svc.poll() && svc.state()==Svc::State::Verifying && !rig.workspace->changeAllowed() && svc.admissionAllowed()==false
        && codeOf([&]{runtime.requestCameraPreparation(rig.current,dir.path());})==int(ProductionError::Busy) && codeOf([&]{runtime.prepare(rig.current,dir.path());})==int(ProductionError::Busy);
    refuse([&]{rig.workspace->calibrations().snapshots({rig.ref});});
    rig.hold->store(false);drain(svc);
    expect(held && !svc.busy() && svc.state()==Svc::State::Ready && rig.workspace->calibrations().snapshots({rig.ref}).size()==1,"Ready only after real drain");
}
// Independent preview/Editor/Runtime owners must block verification via the physical lease.
for(auto use:{CameraLeaseUse::Preview,CameraLeaseUse::Editor,CameraLeaseUse::Runtime}){
    IndustrialRig rig(dir.path(),QString("lease-%1").arg(int(use)),{0});auto& svc=rig.svc();
    auto outsideRequest=rig.r;if(use==CameraLeaseUse::Preview)outsideRequest.profile.requested.trigger={false,{},{}};
    auto outside=rig.backend.factory.open(outsideRequest,use,use==CameraLeaseUse::Preview ? FrameQueueMode::PreviewLatest:FrameQueueMode::ProductionFifo,1);
    auto job=rig.begin(rig.backend);expect(svc.accept(worker(job,rig.backend)),"Busy camera verification completion consumed");drain(svc);
    expect(svc.state()==Svc::State::Failed && !svc.admissionAllowed(),"Preview/Editor/Runtime lease rejects second camera owner");
    outside.reset();job=rig.begin(rig.backend);expect(svc.accept(worker(job,rig.backend)),"Verification retry after owner closes");drain(svc);
    expect(svc.state()==Svc::State::Ready,"New verification succeeds after real external owner drain");
}
// Disconnect during a held close is as invalidating as disconnect before accept.
{
    IndustrialRig rig(dir.path(),"disconnect-during-drain",{0});auto& svc=rig.svc();rig.hold->store(true);
    auto job=rig.begin(rig.backend);expect(svc.accept(worker(job,rig.backend)) && svc.busy(),"Hold verification drain after successful capture");
    rig.workspace->calibrations().disconnect(rig.r.camera.id);rig.hold->store(false);drain(svc);
    expect(svc.state()==Svc::State::Failed && !svc.admissionAllowed(),"Disconnect during drain retires the old probe");
    refuse([&]{rig.workspace->calibrations().snapshots({rig.ref});});
}
// A provider that fails grants no permission; an unclean close fails the whole set.
{
    IndustrialRig rig(dir.path(),"native",{0});auto& svc=rig.svc();auto native=rig.backend;native.factory=CameraProviderFactory{};
    const bool begun=reasonOf([&]{auto job=rig.begin(native);expect(svc.accept(worker(job,native)),"Failed native completion consumed");drain(svc);}).isEmpty();
    expect(begun && svc.state()==Svc::State::Failed && svc.errors().contains(rig.sources[0]) && !svc.admissionAllowed(),"unsupported native probe rejected");
    refuse([&]{rig.workspace->calibrations().snapshots({rig.ref});});
    rig.state->cleanup=true;auto job=rig.begin(rig.backend);expect(svc.accept(worker(job,rig.backend)),"Unclean close completion consumed");drain(svc);
    expect(svc.state()==Svc::State::Failed && svc.errors().value("sourceSet")=="Camera close failed" && !svc.admissionAllowed(),"Unclean close fails the whole set");
}
// Normal OECS cameras take the experimental path, never the webcam probe.
{
    namespace of=oecs_experimental_fixture;
    auto r=of::request();const auto path=dir.filePath("oecs.vfproject");ProjectService project;project.createProject("OECS verify","",path,"file");project.save();
    MeasurementWorkspace teaching(path,project.project().id,[]{return true;});teaching.replace({{r.camera},{r.setup},{r.profile}});auto& active=teaching.calibrations();const SetupReference ref{r.camera.id,r.setup.id,1};active.setOperational(ref.camera,ref.setup);
    const auto hardware=of::state();hardware->pixels.fill(char(20));for(int y=0;y<2048;++y)for(int x=120;x<520;++x)hardware->pixels[y*2448+x]=char(220);
    IndependentCaptureRequest request;request.origin=SourceOrigin::CameraAcquired;request.camera=r.camera;request.setup=r.setup;request.profile=r.profile;request.policy=CalibrationCapturePolicy::Automatic;request.timeoutMs=3000;
    int webcamCalls=0;IndependentCaptureBackend backend;backend.industrialDiscover=[device=r.device]{return IndustrialDiscoverySnapshot{{device},{},false};};backend.experimentalApi=[hardware]{return std::make_unique<of::Api>(hardware);};
    backend.discover=[&]{++webcamCalls;return QVector<DeviceBinding>{};};backend.create=[&](const DeviceBinding&,const GeometryDescriptor&){++webcamCalls;return std::unique_ptr<AcquisitionAdapter>();};
    IndependentCalibrationSession probe;const auto captured=IndependentCalibrationSession::run(probe.begin(request),backend);if(!captured.error().isEmpty())throw std::runtime_error(captured.error().toStdString());expect(probe.accept(captured),"OECS teach probe");
    auto& math=probe.math();expect(math.accept(CalibrationWizardSession::run(math.scanPair("pair",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"OECS teach caliper");math.setKnownMillimetres(10);
    const auto record=test::framedModel(math.calculate(),*math.frame(),10).named("Framed fixture");
    expect(active.saveAndUse({ref,active.environment().revision,probe.evidence()->generation,CalibrationCapturePolicy::ExperimentalOecs},record,probe,[]{return true;}).state==CalibrationSaveUseState::Activated,"OECS teach activation");
    project.createRecipe("OECS","",ApplicationType::Measurement,"file");const auto source=project.addTool("industrial_camera");project.bindIndustrialSource(source,teaching.environment().current,r.profile.id,3000);project.bindSourceSetup(source,ref.toJson());
    const auto left=project.addTool("edge_point"),right=project.addTool("edge_point"),metric=project.addTool("measurement");
    for(auto tool:project.tools())if(tool.id==left || tool.id==right){tool.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();tool.parameters["polarity"]=tool.id==left ? "rising":"falling";project.updateTool(tool);}
    project.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",metric,"p1"},{right,"point",metric,"p2"}});project.save();const auto recipe=*project.activeRecipe();
    MeasurementWorkspace workspace(path,project.project().id,[]{return true;});auto& svc=*workspace.sourceVerification();
    const auto plan=workspace.verificationPlan(recipe,dir.path());const bool routed=plan.probes.size()==1 && plan.probes[0].experimental && plan.probes[0].request.camera && plan.probes[0].request.camera->physical;
    auto job=workspace.verifySources([&]{return recipe;},dir.path(),backend);expect(svc.accept(worker(job,backend)),"OECS completion consumed");drain(svc);
    expect(routed && webcamCalls==0 && svc.state()==Svc::State::Ready && workspace.calibrations().experimentalSnapshots({ref}).size()==1,"normal OECS not routed to webcam probe");
    // Reuse the same permitted installation as a typed experimental offline replay.
    const auto image=*probe.math().frame();const auto metadata=image.metadata();const auto pathReplay=dir.filePath("verify-oecs.pgm");
    ExperimentalOecsAcquisition::save({image,metadata.sourceId,metadata.cycleId,metadata.frameId,metadata.completedAtUtc},pathReplay);
    auto replayRecipe=recipe;auto replayTools=editorTools(replayRecipe.definition);for(auto& tool:replayTools)if(tool.id==source){tool.kind="image_file";tool.parameters=defaultParameters("image_file");tool.parameters["path"]=pathReplay;}replayRecipe.definition=withEditorTools(replayRecipe.definition,replayTools);
    job=workspace.verifySources([&]{return replayRecipe;},dir.path(),backend);expect(job.probeCount()==0 && svc.accept(worker(job,backend)) && svc.state()==Svc::State::Ready,"Experimental replay validates without a live probe");
    QFile sidecar(pathReplay+".json");expect(sidecar.open(QIODevice::ReadOnly),"Read experimental sidecar");auto sidecarObject=QJsonDocument::fromJson(sidecar.readAll()).object();sidecar.close();
    sidecarObject["acquisitionId"]="changed-acquisition";expect(sidecar.open(QIODevice::WriteOnly|QIODevice::Truncate),"Change experimental sidecar");sidecar.write(QJsonDocument(sidecarObject).toJson());sidecar.close();
    expect(svc.state()==Svc::State::Waiting && !svc.admissionAllowed(),"Experimental sidecar identity invalidates Ready");
}
// Offline replay cannot renew a physical permission.
for(bool permitted:{false,true}){
    Rig rig(dir.path(),permitted ? "replay-permitted":"replay-cold",1,false,permitted);auto& svc=rig.svc();
    const auto file=dir.filePath("replay.png");expect(stripe().save(file),"Replay raster");
    auto tools=editorTools(rig.current.definition);for(auto& tool:tools)if(tool.kind=="webcam"){tool.kind="image_file";tool.parameters=defaultParameters("image_file");tool.parameters["path"]=file;}rig.current.definition=withEditorTools(rig.current.definition,tools);
    if(!permitted){const auto refusal=reasonOf([&]{rig.begin();});expect(refusal.contains("Offline replay cannot renew physical source permission") && !svc.busy() && svc.state()==Svc::State::Waiting,"offline replay cannot renew permission");continue;}
    auto job=rig.begin();const bool none=job.probeCount()==0;expect(svc.accept(worker(job,rig.backend)),"Replay completion consumed");
    expect(none && svc.state()==Svc::State::Ready && rig.discoveries==0 && rig.workspace->productionSources(rig.current).sources.size()==1,"offline replay cannot renew permission");
    rig.workspace->calibrations().disconnect(rig.contract.cameras[0].id);expect(svc.state()==Svc::State::Waiting && !svc.admissionAllowed(),"Replay Ready follows the current session permission");
}
// An inventory/disconnect event after capture must retire the outstanding evidence.
for(bool policyChange:{false,true}){
    Rig rig(dir.path(),policyChange ? "policy-epoch":"camera-epoch",2,false);
    // A separately owned verifier also supports a calibration service whose gate
    // can change policy during a job; workspace-owned verification blocks that mutation.
    Svc independent(rig.workspace->calibrations(),[]{return true;});auto& service=policyChange ? independent:rig.svc();
    auto job=service.begin([&]{return rig.workspace->verificationPlan(rig.current,dir.path());},rig.backend);const auto result=worker(job,rig.backend);
    if(policyChange){rig.workspace->calibrations().setExperimental(true);rig.workspace->calibrations().setExperimental(false);}
    else rig.workspace->observeInventory({});
    expect(service.accept(result),"Invalidated completion consumed for drain");
    expect(service.state()==Svc::State::Failed && !service.admissionAllowed(),"In-flight camera/policy invalidation rejects old probes");
    refuse([&]{rig.workspace->calibrations().snapshots({rig.ref(0)});});
    expect(rig.verify() && rig.svc().state()==Svc::State::Ready,"New probe after invalidation can renew permission");
}
// A Ready set for B must never authorize an already prepared raw snapshot A.
{
    Rig rig(dir.path(),"runtime-exact-recipe",1,false);rig.rawOnly();
    expect(rig.verify(),"Verify original raw recipe");rig.runtime->prepare(rig.current,dir.path());rig.runtime->requireReady();
    const auto original=rig.current;const auto validation=rig.runtime->active()->validation().id;
    rig.current.name="Other recipe";expect(rig.verify(),"Verify another raw recipe");
    expect(!rig.svc().admissionAllowedFor(original,dir.path()),"Prepared original recipe differs from Ready set");
    expect(codeOf([&]{rig.runtime->requireReady();})==int(ProductionError::DependencyChanged)
        && codeOf([&]{rig.runtime->begin();})==int(ProductionError::DependencyChanged),"Ready must match the immutable execution recipe");
    rig.runtime->prepare(rig.current,dir.path());rig.runtime->requireReady();
    rig.runtime->rollback(validation);
    expect(codeOf([&]{rig.runtime->begin();})==int(ProductionError::DependencyChanged),"Rollback cannot borrow another recipe's Ready");
    rig.current=original;expect(rig.verify(),"Verify rolled back original");rig.runtime->requireReady();
    expect(codeOf([&]{rig.runtime->prepare(rig.current,dir.path()+"/other-directory");})==int(ProductionError::DependencyChanged),"Ready is bound to project directory");
}
// Metric effective recipes include a derived binding; compare their original verification identity.
{
    Rig rig(dir.path(),"runtime-metric-identity",1,false);expect(rig.verify(),"Verify metric recipe");
    rig.runtime->prepare(rig.current,dir.path());rig.runtime->requireReady();expect(true,"Metric derived snapshot retains original verification identity");
}
// Replay validation is read-only, but decodes geometry and binds Ready to the actual content.
for(bool raw:{false,true}){
    Rig rig(dir.path(),raw ? "raw-replay-content":"metric-replay-content",1,false,true);if(raw)rig.rawOnly();
    const auto file=dir.filePath(raw ? "raw-replay.png":"metric-replay.png");expect(stripe().save(file),"Replay content fixture");
    auto tools=editorTools(rig.current.definition);for(auto& tool:tools)if(tool.kind=="webcam"){tool.kind="image_file";tool.parameters=defaultParameters("image_file");tool.parameters["path"]=file;}rig.current.definition=withEditorTools(rig.current.definition,tools);
    expect(rig.verify() && rig.svc().state()==Svc::State::Ready,"Valid replay Ready");
    auto changed=stripe();changed.scanLine(0)[0]=77;expect(changed.save(file),"Change replay pixels without changing geometry");
    expect(rig.svc().state()==Svc::State::Waiting && !rig.svc().admissionAllowed(),"Pixel change invalidates replay Ready");
    expect(rig.verify() && rig.svc().state()==Svc::State::Ready,"Explicit verification accepts compatible changed raster");
    QFile sidecar(file+".json");expect(sidecar.open(QIODevice::WriteOnly),"Add replay sidecar");sidecar.write("{}");sidecar.close();
    expect(rig.svc().state()==Svc::State::Waiting && !rig.svc().admissionAllowed(),"Sidecar addition invalidates Ready");
    expect(rig.verify(),"Explicit verification accepts harmless sidecar");
    auto job=rig.begin();auto result=worker(job,rig.backend);changed.scanLine(0)[0]=88;expect(changed.save(file),"Change replay during verification");
    expect(rig.svc().accept(result) && rig.svc().state()==Svc::State::Failed,"Replay change retires outstanding completion");
    QImage wrong(32,32,QImage::Format_Grayscale8);wrong.fill(0);expect(wrong.save(file),"Wrong replay geometry");
    if(!raw)refuse([&]{rig.begin();});
    QFile broken(file);expect(broken.open(QIODevice::WriteOnly|QIODevice::Truncate),"Open corrupt replay");broken.write("not an image");broken.close();
    refuse([&]{rig.begin();});
}
QTextStream(stdout)<<checks<<" checks passed\n";return 0;
}catch(const std::exception& error){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}}
