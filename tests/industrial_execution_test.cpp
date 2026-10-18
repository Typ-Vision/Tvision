#include "IndustrialProviderFixture.hpp"
#include "application/IndustrialExecution.hpp"
#include "application/ProjectService.hpp"
#include "application/ToolExecutors.hpp"
#include "application/MeasurementSourceIdentity.hpp"
#include <QTemporaryDir>
#include <QJsonArray>
using namespace industrial_provider_fixture;
namespace {
const ToolResult& result(const CycleResult& c,const QString& id){for(const auto& r:c.tools)if(r.toolId==id)return r;throw std::runtime_error("missing tool");}
class Webcam final:public AcquisitionAdapter {
public:
    AcquisitionCapabilities capabilities() const override{return {"webcam",true};}
    ImageFrame acquire(const AcquisitionRequest&) const override{const unsigned char p[6]={6,5,4,3,2,1};return ImageFrame::fromBuffer(p,6,3,2,3,PixelFormat::Gray8);}
};
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("icm07b-XXXXXX"));check(dir.isValid(),"fixture directory");
    const auto req=request();const MeasurementSetupContract contract{{req.camera},{req.setup},{req.profile}};
    int recipes=0;
    for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        auto state=std::make_shared<State>();IndustrialExecutionInputs inputs;
        inputs.contract=contract;inputs.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));
        inputs.discover=[req](int,const auto&){return IndustrialDiscoverySnapshot{{req.device},{},false};};
        int webcamCalls=0;
        ExecutionService execution(inputs,[&](const ToolDefinition& t)->std::unique_ptr<AcquisitionAdapter>{check(t.kind!="industrial_camera","industrial cannot use generic injection");if(t.kind=="webcam"){++webcamCalls;return std::make_unique<Webcam>();}return {};});
        // Constructor must freeze the contract before caller state changes.
        inputs.contract.profiles[0].requested.gain=9;
        ProjectService p;p.createProject("Execution","",dir.filePath(QString::number(recipes++)+".vfproject"),"file");
        const auto recipe=p.createRecipe("Mixed","",type,"file");
        const auto industrial=p.addTool("industrial_camera");p.bindIndustrialSource(industrial,contract,req.profile.id,1000);
        p.bindSourceSetup(industrial,SetupReference{req.camera.id,req.setup.id,req.setup.revision}.toJson());
        const auto pre=p.addTool("preprocess");
        const auto file=p.addTool("image_file");QImage image(3,2,QImage::Format_Grayscale8);image.fill(44);const auto path=dir.filePath("image.png");check(image.save(path),"save image");p.setImageFile(path,file);
        const auto filePre=p.addTool("preprocess");const auto webcam=p.addTool("webcam");
        p.setConnections({{industrial,"image",pre,"image"},{file,"image",filePre,"image"}});
        const auto definition=p.activeRecipe()->definition;
        auto run=[&](QString target={},std::shared_ptr<std::atomic_bool> cancel={},ExecutionLimits limits={}){return execution.run(definition,dir.path(),recipe,"cfg",cancel,target,{}, {},nullptr,{},limits);};
        auto acquired=run();check(acquired.state==ExecutionState::Done && acquired.acquisitions==3 && acquired.frames.size()==3 && webcamCalls==1,"mixed industrial/file/webcam");
        check(acquired.industrialEvidence.size()==1,"actual evidence per industrial source");
        const auto evidence=acquired.industrialEvidence[industrial];
        check(evidence->request.device.identity==req.device.identity && evidence->actual.settings==req.profile.requested,"actual device/readback");
        check(evidence->acquired.frameId!=evidence->execution.frameId && evidence->execution.cycleId==acquired.cycleId,"backend identity retained separately");
        const auto raw=std::get<ImageFrame>(result(acquired,industrial).outputs["image"].data);
        check(evidence->pixelHash==raw.contentId() && raw.processing().isEmpty() && !state->hardware->running && !state->hardware->held,"owned raw survives cleanup");
        EditorContext context{p.project().id,p.path(),recipe,definition};EditorSession editor;
        check(editor.acceptCycle(acquired,editor.signature(context),context),"accept industrial cycle");
        const int commands=state->hardware->commands;
        auto evaluation=editor.evaluation(context,pre);
        auto target=execution.runTool(definition,dir.path(),recipe,"cfg",{},pre,evaluation);
        check(target.state==ExecutionState::Done && target.acquisitions==0 && state->hardware->commands==commands,"TargetOnly acquisition0");
        auto wrong=evaluation;wrong.frame=editor.sourceFrames[file];
        check(execution.runTool(definition,dir.path(),recipe,"cfg",{},pre,wrong).state==ExecutionState::Error,"source swap rejected");
        wrong=evaluation;unsigned char pixels[6]={9,9,9,9,9,9};wrong.frame=ImageFrame::fromBuffer(pixels,6,3,2,3,PixelFormat::Gray8,wrong.frame.metadata());
        check(execution.runTool(definition,dir.path(),recipe,"cfg",{},pre,wrong).state==ExecutionState::Error,"same metadata pixel swap rejected");
        wrong=evaluation;wrong.cache[industrial].result.outputs["image"].origin.sourceId=file;
        check(execution.runTool(definition,dir.path(),recipe,"cfg",{},pre,wrong).state==ExecutionState::Error,"cache source swap rejected");
        const auto other=editor.sourceFrames[file].metadata().frameId;
        state->hardware->pixels=QByteArray::fromHex("010203040506");
        auto refresh=run(industrial);check(refresh.state==ExecutionState::Done && refresh.acquisitions==1 && editor.acceptCycle(refresh,editor.signature(context),context),"source refresh");
        check(refresh.industrialEvidence[industrial]->pixelHash==evidence->pixelHash && refresh.industrialEvidence[industrial]->execution.frameId!=evidence->execution.frameId,"same pixels still form a new acquired frame");
        check(editor.sourceFrames[file].metadata().frameId==other && editor.evaluation(context,filePre).cache.contains(filePre) && !editor.evaluation(context,pre).cache.contains(pre),"refresh clears own branch only");
        auto changed=context;auto tools=editorTools(definition);for(auto& t:tools)if(t.id==industrial){auto ref=IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject());ref.timeoutMs=2000;t.parameters["sourceReference"]=ref.toJson();}changed.definition=withEditorTools(definition,tools);editor.reconcile(changed);
        check(!editor.sourceFrames.contains(industrial) && editor.sourceFrames.contains(file),"profile reference edit invalidates own branch");
        for(int mismatch=0;mismatch<3;++mismatch){state->foreign=mismatch==0;state->geometry=mismatch==1;state->settings=mismatch==2;auto bad=run(industrial);check(bad.state==ExecutionState::Error && bad.frames.isEmpty() && bad.industrialEvidence.isEmpty() && result(bad,industrial).outputs.isEmpty(),"actual identity/geometry/settings mismatch");}state->foreign=state->geometry=state->settings=false;
        for(const auto scenario:{"late-epoch","duplicate-trigger","missing-proof","timeout"}){state->hardware->scenario=scenario;const auto bad=run(industrial);check(bad.state==ExecutionState::Error && bad.frames.isEmpty() && bad.industrialEvidence.isEmpty(),"bad correlation rejected");}state->hardware->scenario.clear();
        auto cancel=std::make_shared<std::atomic_bool>(false);state->hardware->hook=[cancel](const QString& call){if(call=="wait")cancel->store(true);};
        auto stopped=run(industrial,cancel);check(stopped.state==ExecutionState::Skipped && stopped.frames.isEmpty() && stopped.industrialEvidence.isEmpty(),"Stop during read suppresses evidence");for(const auto& r:stopped.tools)check(r.outputs.isEmpty() && r.decision==PartDecision::NotEvaluated,"Stop no output/decision");state->hardware->hook={};
        state->delayMs=100;auto late=run(industrial,{}, {40,512ull*1024*1024});check(late.state==ExecutionState::Error && late.frames.isEmpty() && late.industrialEvidence.isEmpty(),"late frame total deadline");state->delayMs=0;
        const auto before=state->hardware->commands;auto budget=run(industrial,{}, {30000,10});check(budget.state==ExecutionState::Error && state->hardware->commands==before,"preflight raster budget no trigger");
        auto busy=inputs.factory.open(req,CameraLeaseUse::Calibration,FrameQueueMode::ProductionFifo,1);auto blocked=run(industrial);check(blocked.state==ExecutionState::Error && state->hardware->commands==before,"shared calibration/editor lease");busy.reset();
        check(ExecutionService{}.run(definition,dir.path(),recipe,"cfg",{},industrial).state==ExecutionState::Error,"default composition closed");
        auto normalInputs=inputs;normalInputs.contract=contract;normalInputs.factory=CameraProviderFactory{};
        check(ExecutionService(normalInputs).run(definition,dir.path(),recipe,"cfg",{},industrial).state==ExecutionState::Error,"normal factory rejects test identity");
        auto unavailable=normalInputs;unavailable.factory=inputs.factory;unavailable.discover=[](int,const auto&){return IndustrialDiscoverySnapshot{};};
        check(ExecutionService(unavailable).run(definition,dir.path(),recipe,"cfg",{},industrial).state==ExecutionState::Error,"offline exact device rejected");
        unavailable.contract.profiles[0].revision++;
        check(ExecutionService(unavailable).run(definition,dir.path(),recipe,"cfg",{},industrial).state==ExecutionState::Error,"stale profile revision rejected");
        reject([&]{ToolExecutors::get("industrial_camera");});
        auto substituted=execution.run(definition,dir.path(),recipe,"cfg",{},industrial,raw);check(substituted.state==ExecutionState::Error && substituted.frames.isEmpty(),"snapshot bypass rejected");
        const auto duplicate=p.addTool("industrial_camera");p.bindIndustrialSource(duplicate,contract,req.profile.id,1000);
        auto duplicated=execution.run(p.activeRecipe()->definition,dir.path(),recipe,"cfg",{},industrial);check(duplicated.state==ExecutionState::Error && duplicated.acquisitions==0,"duplicate physical camera preflight on source refresh");
    }
    QTextStream(stdout)<<"Industrial execution: mixed sources, evidence, TargetOnly, cache, Stop and negative corpus passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
