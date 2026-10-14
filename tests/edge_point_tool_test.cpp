#include "application/EditorSession.hpp"
#include "application/RuntimeService.hpp"
#include "application/ProductionResultCodec.hpp"
#include "application/ToolResultCodec.hpp"
#include "domain/EdgePointConfig.hpp"
#include "domain/ToolRoi.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"expected configuration rejection");}
const ToolResult& result(const CycleResult& c,const QString& id){for(const auto& t:c.tools)if(t.toolId==id)return t;throw std::runtime_error("missing tool result");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/edge-tool-XXXXXX");check(dir.isValid(),"isolated build fixture");
    auto p=edgePointDefaults();validateEdgePoint(p);p["unknown"]=1;reject([&]{validateEdgePoint(p);});p=edgePointDefaults();p["selection"]="future";reject([&]{validateEdgePoint(p);});p=edgePointDefaults();p["subpixel"]=1;reject([&]{validateEdgePoint(p);});p=edgePointDefaults();p["minContrast"]=0;reject([&]{validateEdgePoint(p);});
    ProjectService project;project.createProject("Edges","",dir.filePath("part.vfproject"),"file");project.createRecipe("Part","",ApplicationType::Measurement,"file");
    QImage image(128,64,QImage::Format_Grayscale8);image.fill(20);for(int y=0;y<64;++y)for(int x=35;x<95;++x)image.scanLine(y)[x]=220;const auto path=dir.filePath("stripe.png");check(image.save(path),"stripe fixture");
    const auto source=project.setImageFile(path),pre=project.addTool("preprocess"),left=project.addTool("edge_point"),right=project.addTool("edge_point"),calc=project.addTool("calculate");
    const auto raw=source+"/image";
    auto setEdge=[&](const QString& id,const QString& polarity){for(auto t:project.tools())if(t.id==id){t.parameters["scanRoi"]=ToolRoi{{raw},ScanSegment{{5,32},{122,32},4}}.toJson();t.parameters["sigmaPx"]=1;t.parameters["polarity"]=polarity;project.updateTool(t);}};
    setEdge(left,"rising");setEdge(right,"falling");project.setConnections({{source,"image",pre,"image"},{pre,"image",left,"image"},{pre,"image",right,"image"},{left,"point",calc,"p1"},{right,"point",calc,"p2"}});project.save();const auto recipe=project.activeRecipe()->id;const auto revision=project.activeRecipe()->recipeVersion;
    project.openProject(project.path());project.selectRecipe(recipe);project.save();check(project.activeRecipe()->recipeVersion==revision && !project.dirty(),"save reopen no-op schema7");
    EditorSession session;auto context=[&]{return EditorContext{project.project().id,project.path(),recipe,project.activeRecipe()->definition,{}};};auto c=context();
    auto full=ExecutionService{}.run(c.definition,dir.path(),recipe,session.signature(c),{});check(full.state==ExecutionState::Done,"real stripe DAG");const auto& edge=result(full,left);check(edge.outputs["point"].valid && std::get<bool>(edge.outputs["found"].data) && edge.decision==PartDecision::NotEvaluated,"real point without global decision");
    check(std::abs(std::get<QPointF>(edge.outputs["point"].data).x()-34.5)<.01 && std::abs(std::get<double>(result(full,calc).outputs["value"].data)-60)<.01,"ground truth 60 pixel stripe distance");
    check(edge.edgeDiagnostics && edge.edgeDiagnostics->sampleCount==235 && edge.edgeDiagnostics->profile.size()==235 && edge.edgeDiagnostics->candidates.size()==2,"complete live small profile diagnostics");
    int selectedCandidates=0,rejectedCandidates=0;for(const auto& candidate:edge.edgeDiagnostics->candidates){if(candidate.selected){++selectedCandidates;check(candidate.accepted && std::abs(candidate.point.x()-34.5)<.01,"selected diagnostic matches actual point");}if(!candidate.accepted){++rejectedCandidates;check(candidate.reason=="polarity","rejected diagnostic actual polarity reason");}}check(selectedCandidates==1 && rejectedCandidates==1,"selected and rejected candidates distinct");
    auto cancelledPresentation=full;EditorSession::suppressCancelled(cancelledPresentation);for(const auto& t:cancelledPresentation.tools)check(!t.edgeDiagnostics && t.outputs.isEmpty(),"late Stop removes live diagnostic and result");
    check(session.acceptCycle(full,session.signature(c),c),"editor cache accepts full");const auto hash=session.frame->contentId();
    for(auto t:project.tools())if(t.id==left){t.parameters["selection"]="first";project.updateTool(t);}c=context();auto target=ExecutionService{}.runTool(c.definition,dir.path(),recipe,session.signature(c),{},left,session.evaluation(c));check(target.executedTools==QStringList{left} && target.acquisitions==0 && session.frame->contentId()==hash,"TargetOnly reuses processed upstream and raw frame");
    for(auto t:project.tools())if(t.id==pre){t.parameters["steps"]=QJsonArray{QJsonObject{{"op","threshold"},{"size",3},{"value",128}}};project.updateTool(t);}c=context();target=ExecutionService{}.runTool(c.definition,dir.path(),recipe,session.signature(c),{},left,session.evaluation(c));check(target.executedTools.isEmpty(),"preprocess edit invalidates downstream cache");
    project.save();RuntimeService runtime(dir.filePath("archive"),project.path()+".calibrations.json",RuntimeIdentity::localEngineer());runtime.prepare(*project.activeRecipe(),dir.path());
    for(auto t:project.tools())if(t.id==left){t.parameters["minContrast"]=5100;project.updateTool(t);}
    auto lease=runtime.begin();auto cycle=RuntimeService::execute(lease,project.path()+".calibrations.json",{});check(cycle.state==ExecutionState::Done && std::get<bool>(result(cycle,left).outputs["found"].data),"immutable Runtime ignores draft edge threshold");runtime.finish(lease.ticket,cycle);const auto record=runtime.records().first();ProductionResultCodec::validate(record);check(ProductionResultCodec::csv({record}).contains("profile-edge") && ProductionStore(dir.filePath("archive")).recent("cycles").first()==record,"terminal typed edge method evidence/restart/CSV");
    c=context();auto missing=ExecutionService{}.run(c.definition,dir.path(),recipe,session.signature(c),{});const auto& no=result(missing,left);check(no.state==ExecutionState::Done && no.outputs["found"].valid && !std::get<bool>(no.outputs["found"].data) && !no.outputs["point"].valid && !no.outputs["strength"].valid && no.points.isEmpty() && no.edgeDiagnostics.has_value(),"not found successful search with invalid metrics");check(result(missing,calc).state==ExecutionState::Skipped && result(missing,calc).outputs.isEmpty(),"missing point cannot make downstream number");
    const auto replay=ToolResultCodec::decode(ToolResultCodec::encode(no.outputs["point"]));check(!replay.valid && replay.invalidReason=="NOT_FOUND","invalid point null replay");
    auto cancel=std::make_shared<std::atomic_bool>(true);auto stopped=ExecutionService{}.run(c.definition,dir.path(),recipe,session.signature(c),cancel);check(stopped.state==ExecutionState::Skipped,"Stop skipped");for(const auto& t:stopped.tools)check(t.outputs.isEmpty(),"Stop clears outputs");
    for(auto t:project.tools())if(t.id==left){t.parameters["scanRoi"]=QJsonObject{};project.updateTool(t);}reject([&]{runtime.checkConfiguration(*project.activeRecipe(),dir.path());});c=context();auto invalid=ExecutionService{}.run(c.definition,dir.path(),recipe,session.signature(c),{});check(result(invalid,left).state==ExecutionState::Error && result(invalid,left).outputs.isEmpty(),"untaught ROI is error not found=false");
    for(auto t:project.tools())if(t.id==left){t.parameters["scanRoi"]=ToolRoi{{"wrong/raw"},ScanSegment{{5,32},{122,32},4}}.toJson();project.updateTool(t);}c=context();invalid=ExecutionService{}.run(c.definition,dir.path(),recipe,session.signature(c),{});check(result(invalid,left).state==ExecutionState::Error,"wrong raw frame rejected");
    // A larger independent stripe raster exercises display limits without changing search.
    QImage many(9000,32,QImage::Format_Grayscale8);for(int y=0;y<many.height();++y)for(int x=0;x<many.width();++x)many.scanLine(y)[x]=(x/3)%2 ? 220:20;
    const auto manyPath=dir.filePath("many.png");check(many.save(manyPath),"large diagnostic fixture");
    ProjectService large;large.createProject("Many","",dir.filePath("many.vfproject"),"file");large.createRecipe("Many","",ApplicationType::Measurement,"file");const auto ls=large.setImageFile(manyPath),le=large.addTool("edge_point");for(auto t:large.tools())if(t.id==le){t.parameters["scanRoi"]=ToolRoi{{ls+"/image"},ScanSegment{{10,16},{8990,16},2}}.toJson();t.parameters["sigmaPx"]=0;large.updateTool(t);}large.setConnections({{ls,"image",le,"image"}});
    const auto largeCycle=ExecutionService{}.run(large.activeRecipe()->definition,dir.path(),large.activeRecipe()->id,"large",{});const auto& largeEdge=result(largeCycle,le);check(largeEdge.state==ExecutionState::Done && largeEdge.edgeDiagnostics,"large edge executes");const auto& diag=*largeEdge.edgeDiagnostics;check(diag.sampleCount>2048 && diag.candidateCount>2048 && diag.profile.size()==2048 && diag.derivative.size()==2048 && diag.candidates.size()==2048,"bounded and counted live display data");int countSelected=0;for(const auto& candidate:diag.candidates)if(candidate.selected){++countSelected;check(candidate.point==std::get<QPointF>(largeEdge.outputs["point"].data),"bounded display retains selected actual edge");}check(countSelected==1 && diag.profile.first().distance==0 && diag.profile.last().distance==8980,"display bounds include scan endpoints and selected candidate");
    QTextStream(stdout)<<checks<<" edge tool checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
