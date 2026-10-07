#include "application/CalibrationWorkflow.hpp"
#include "application/EditorSession.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QTextStream>
#include <cmath>
using namespace visionflow;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    QTemporaryDir dir(QDir::currentPath()+"/measurement-dag-XXXXXX");check(dir.isValid(),"build fixture");
    ProjectService project;project.createProject("Measurement","",dir.path()+"/test.vfproject","file");project.createRecipe("Part","",ApplicationType::Measurement,"file");
    const auto source=project.addTool("image_file"),left=project.addTool("edge"),right=project.addTool("edge"),measurement=project.addTool("measurement");
    for(auto t:project.tools()){if(t.id==right)t.parameters["direction"]="right_to_left";if(t.id==measurement){t.parameters["lowerMm"]=9.97;t.parameters["upperMm"]=9.98;}project.updateTool(t);}
    project.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",measurement,"p1"},{right,"point",measurement,"p2"}});
    project.setCalibrationSetup("lens","plane");
    cv::Mat pixels(600,1000,CV_8UC1,cv::Scalar(0));pixels(cv::Rect(200,100,400,300)).setTo(255);
    const auto snapshot=ImageFrame::fromBuffer(pixels.data,pixels.total(),1000,600,pixels.step,PixelFormat::Gray8,{},false);
    auto acquisition=ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),project.activeRecipe()->id,"cfg",{},source,snapshot);
    const auto frame=std::get<ImageFrame>(acquisition.tools.first().outputs["image"].data);const auto installation=CalibrationWorkflow::installation(*project.activeRecipe(),frame);
    auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,installation.source},{b,installation.source},mm};};
    const auto teaching=sample("teach",{0,0},{400,0},10);const QVector<CalibrationSample> holdout{sample("x",{10,30},{810,30},20),sample("y",{40,10},{40,510},12.5)};
    const auto candidate=CalibrationWorkflow::teach(*project.activeRecipe(),frame,"scale",1,teaching);const auto accepted=MeasurementService{}.validate(candidate,holdout,0.001);
    const auto catalogPath=project.path()+".calibrations.json";CalibrationService(catalogPath).save(accepted);CalibrationWorkflow::bind(project,frame,catalogPath,"scale");project.save();
    auto run=[&](const std::optional<ImageFrame>& image=std::nullopt){const auto recipe=*project.activeRecipe();return ExecutionService{}.run(recipe.definition,dir.path(),recipe.id,"cfg",{}, {},image ? image : std::optional<ImageFrame>(snapshot),[recipe,catalogPath](const ImageFrame& acquired){return CalibrationWorkflow::resolve(recipe,acquired,catalogPath);});};
    auto find=[&](const CycleResult& c){for(const auto& t:c.tools)if(t.toolId==measurement)return t;throw std::runtime_error("measurement tool absent");};
    auto cycle=run();auto result=find(cycle);
    check(cycle.state==ExecutionState::Done && result.state==ExecutionState::Done && result.decision==PartDecision::Pass && std::abs(std::get<double>(result.outputs["value"].data)-9.975)<1e-12,"real Edge DAG calibrated inclusive PASS");
    check(result.calibration==accepted.dependency() && result.outputs["value"].port.unit=="mm" && result.outputs["pixels"].port.unit=="px" && result.outputs["value"].origin.cycleId==cycle.cycleId,"typed outputs calibration and provenance");
    for(auto t:project.tools())if(t.id==measurement){t.parameters["upperMm"]=9.9749;project.updateTool(t);}
    result=find(run());check(result.state==ExecutionState::Done && result.decision==PartDecision::Fail && !std::get<bool>(result.outputs["pass"].data),"FAIL stays DONE, raw value rather than rounding");
    EditorSession session;EditorContext context{"project",project.path(),project.activeRecipe()->id,project.activeRecipe()->definition,project.activeRecipe()->calibrationBinding->toJson()};auto changed=context;changed.calibrationBinding={};check(session.signature(context)!=session.signature(changed),"binding included in stale-result signature");
    auto unbound=*project.activeRecipe();unbound.calibrationBinding.reset();
    cycle=ExecutionService{}.run(unbound.definition,dir.path(),unbound.id,"cfg",{}, {},snapshot,[unbound,catalogPath](const ImageFrame& f){return CalibrationWorkflow::resolve(unbound,f,catalogPath);});result=find(cycle);
    check(cycle.state==ExecutionState::Error && result.errorCode=="CALIBRATION_INVALID" && result.outputs.isEmpty() && result.decision==PartDecision::NotEvaluated && !result.calibration,"unbound blocks all numeric/decision outputs");
    cv::Mat blank(600,1000,CV_8UC1,cv::Scalar(0));auto blankFrame=ImageFrame::fromBuffer(blank.data,blank.total(),1000,600,blank.step,PixelFormat::Gray8,{},false);result=find(run(blankFrame));
    check(result.state==ExecutionState::Skipped && result.outputs.isEmpty() && result.decision==PartDecision::NotEvaluated,"missing edges skip measurement with no part decision");
    cv::Mat resized(300,500,CV_8UC1,cv::Scalar(0));resized(cv::Rect(100,20,200,200)).setTo(255);auto small=ImageFrame::fromBuffer(resized.data,resized.total(),500,300,resized.step,PixelFormat::Gray8,{},false);result=find(run(small));
    check(result.state==ExecutionState::Error && result.outputs.isEmpty() && result.errorCode=="CALIBRATION_INVALID","actual acquisition resolution invalidates calibration");
    auto cancelled=std::make_shared<std::atomic_bool>(true);cycle=ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),project.activeRecipe()->id,"cfg",cancelled,{},snapshot);
    for(const auto& t:cycle.tools)check(t.decision==PartDecision::NotEvaluated && t.outputs.isEmpty() && !t.calibration,"cancel clears outputs and decision");
    const auto revision=CalibrationWorkflow::teach(*project.activeRecipe(),frame,"scale",2,teaching);CalibrationService(catalogPath).save(MeasurementService{}.validate(revision,holdout,0.001));result=find(run());check(result.errorCode=="CALIBRATION_INVALID" && result.outputs.isEmpty(),"latest catalog reread invalidates old binding");
    project.setCalibrationSetup("new-lens","plane");check(!project.activeRecipe()->calibrationBinding && project.dirty(),"installation change clears binding and drafts");
    auto preview=ImageFrame::fromBuffer(pixels.data,pixels.total(),1000,600,pixels.step,PixelFormat::Gray8,{},false);check(!CalibrationWorkflow::resolve(*project.activeRecipe(),preview,catalogPath).calibration,"preview without acquisition provenance cannot validate");
    QTextStream(stdout)<<"PASS: shared Measurement DAG, real Edge outputs, calibration resolver, raw FAIL, invalidation, cancellation and binding stale guard\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
