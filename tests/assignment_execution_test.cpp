#include "application/EditorSession.hpp"
#include "application/ToolExecutors.hpp"
#include "application/ToolCatalog.hpp"
#include "application/ToolResultCodec.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QImage>
#include <QTextStream>
#include <QUuid>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0,consumed=0;
void check(bool b,const char* m){++checks;if(!b)throw std::runtime_error(m);}
template<class F> void rejects(F f,const char* m){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,m);}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
const ToolResult& tool(const CycleResult& c,const QString& id){for(const auto& t:c.tools)if(t.toolId==id)return t;throw std::runtime_error("Missing tool result");}
ToolDefinition assignment(const QString& name,const AssignmentConfig& c){return {c.producerId,"set_variables",name,{},true,encodeAssignmentConfig(c)};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/assignment-XXXXXX");check(dir.isValid(),"Fixture directory");
    QImage image(30,30,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("part.png")),"Actual image fixture");
    check(!ToolCatalog::canAdd("set_variables"),"Assignment UI stays closed until its UI card");
    QTcpServer endpoint;check(endpoint.listen(QHostAddress::LocalHost,0),"Network observer");int networkCalls=0;QObject::connect(&endpoint,&QTcpServer::newConnection,[&]{++networkCalls;});
    ToolRegistry::registerType({"bundle_consumer","Bundle consumer",1,{},{{"values",PortType::AssignmentBundle,"","cycle"}},{},[](const ToolDefinition&){},false,false,{},7});
    ToolExecutors::registerExecutor("bundle_consumer",[](const ToolDefinition& t,const QMap<QString,PortValue>& input,const QString&,const QString&,const QString& cycle,const std::shared_ptr<std::atomic_bool>&){const auto b=std::get<AssignmentBundle>(input["values"].data);check(b.committed && b.cycleId==cycle,"Consumer sees current committed typed bundle");++consumed;ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;return r;});
    VariableDefinition x{id(),"X"},y{id(),"Y"};const auto sourceId=id(),blobId=id(),pId=id(),qId=id(),consumerId=id();
    ToolDefinition source{sourceId,"image_file","Source",{},true,{{"path","part.png"}}};
    ToolDefinition blob{blobId,"blob","Blob",{},true,defaultParameters("blob")};
    AssignmentConfig p{pId,1,{{x.id,ToolOutputAssignmentSource{blobId,"found",4}}}};
    AssignmentConfig q{qId,3,{{y.id,BundleAssignmentSource{pId,x.id,1}}}};
    ToolDefinition consumer{consumerId,"bundle_consumer","Consumer",{},true,{}};
    auto make=[&](AssignmentConfig a,AssignmentConfig b){auto d=withRecipeVariables({}, {x,y});d=withEditorTools(d,{consumer,assignment("Second",b),assignment("First",a),blob,source});return withEditorConnections(d,{{sourceId,"image",blobId,"image"},{qId,"bundle",consumerId,"values"}});};
    auto definition=make(p,q);const auto graph=validateGraph(editorTools(definition),editorConnections(definition));check(graph.valid(),"Assignment graph valid");check(graph.order.indexOf(blobId)<graph.order.indexOf(pId) && graph.order.indexOf(pId)<graph.order.indexOf(qId) && graph.order.indexOf(qId)<graph.order.indexOf(consumerId),"Dependencies enforce upstream-assignment-consumer order");
    check(editorConnections(definition).size()==2 && executionConnections(definition).size()==4,"Assignment dependencies derived without persisted hidden variable links");
    auto wrong=p;std::get<ToolOutputAssignmentSource>(wrong.rows[0].source).revision=99;check(!validateGraph(editorTools(make(wrong,q)),editorConnections(definition)).valid(),"Wrong output version rejected");
    wrong=p;std::get<ToolOutputAssignmentSource>(wrong.rows[0].source).port="missing";check(!validateGraph(editorTools(make(wrong,q)),editorConnections(definition)).valid(),"Missing output port rejected");
    wrong=p;wrong.rows[0].source=BundleAssignmentSource{qId,y.id,3};check(!validateGraph(editorTools(make(wrong,q)),editorConnections(definition)).valid(),"Cyclic assignment rejected");
    auto wrongBundle=q;std::get<BundleAssignmentSource>(wrongBundle.rows[0].source).revision=2;check(!validateGraph(editorTools(make(p,wrongBundle)),editorConnections(definition)).valid(),"Bundle revision rejected");
    wrongBundle=q;std::get<BundleAssignmentSource>(wrongBundle.rows[0].source).fieldId=id();check(!validateGraph(editorTools(make(p,wrongBundle)),editorConnections(definition)).valid(),"Missing bundle field rejected");
    ValueEnvelope constant;constant.value=false;constant.validity=ValueValidity::Valid;
    auto branch=q;branch.rows={{x.id,ConstantAssignmentSource{constant}}};check(!validateGraph(editorTools(make(p,branch)),editorConnections(definition)).valid(),"Unordered branch writes rejected");
    branch=q;branch.rows={{x.id,BundleAssignmentSource{pId,x.id,1}}};check(validateGraph(editorTools(make(p,branch)),editorConnections(definition)).valid(),"Explicit ordered write allowed");
    auto parameters=encodeAssignmentConfig(p);parameters["extra"]=true;rejects([&]{decodeAssignmentConfig(parameters,pId);},"Unknown config fields rejected");
    auto duplicate=p;duplicate.rows.append(p.rows.first());rejects([&]{encodeAssignmentConfig(duplicate);},"Duplicate target rejected");
    auto malformed=encodeAssignmentConfig(p);malformed["revision"]=1.5;rejects([&]{decodeAssignmentConfig(malformed,pId);},"Fractional revision rejected");
    EditorSession session;EditorContext context{"project",dir.filePath("project.vfproject"),"recipe",definition,{}};
    auto full=[&]{const auto signature=session.signature(context);auto c=ExecutionService{}.run(context.definition,dir.path(),context.recipeId,signature,{});check(c.state==ExecutionState::Done && c.acquisitions==1,"Full flow acquires once");check(session.acceptCycle(c,signature,context),"Session accepts assignment full flow");return c;};
    auto cycle=full();check(consumed==1,"Consumer executes once");auto bundlePort=tool(cycle,qId).outputs["bundle"];const auto bundle=std::get<AssignmentBundle>(bundlePort.data);check(bundle.revision==3 && std::get<bool>(*std::get<ValueEnvelope>(bundle.fields[y.id]).value),"Upstream actual Boolean reaches final assignment");
    auto oversizedIdentity=bundle;oversizedIdentity.configId=QString(129,'c');rejects([&]{encodeAssignmentBundle(oversizedIdentity);},"Bundle identity length bounded at encode");
    auto largeBundle=bundle;largeBundle.fields.clear();
    for(int f=0;f<64;++f){auto field=id();ValueEnvelope e=std::get<ValueEnvelope>(bundle.fields[y.id]);e.producerPort=field;e.dependencyIds.clear();for(int n=0;n<256;++n)e.dependencyIds.append(QString::number(n)+QString(128,'d'));largeBundle.fields[field]=e;}
    auto largePort=bundlePort;largePort.data=largeBundle;const auto largeEncoded=ToolResultCodec::encode(largePort);check(QJsonDocument(largeEncoded).toJson(QJsonDocument::Compact).size()>ToolValues::maxJsonBytes,"Large legal bundle crosses legacy typed payload limit");check(ToolResultCodec::encode(ToolResultCodec::decode(largeEncoded))==largeEncoded,"Full bounded assignment bundle roundtrip");
    const auto encoded=ToolResultCodec::encode(bundlePort);auto decoded=ToolResultCodec::decode(encoded);check(ToolResultCodec::encode(decoded)==encoded,"Bundle typed port roundtrip");auto bad=encoded;auto b=bad["value"].toObject();b["cycleId"]="foreign";bad["value"]=b;rejects([&]{ToolResultCodec::decode(bad);},"Bundle field cycle mismatch rejected");
    auto partial=[&](const QString& target){return ExecutionService{}.runTool(context.definition,dir.path(),context.recipeId,session.signature(context),{},target,session.evaluation(context,target));};
    // Editing only this target leaves upstream immutable bundle cache usable.
    auto changedQ=q;changedQ.revision=4;context.definition=make(p,changedQ);
    session.reconcile(context);
    auto result=partial(qId);check(result.state==ExecutionState::Done && result.executedTools==QStringList{qId} && result.acquisitions==0,"Target config edit recertifies unchanged upstream cache");
    context.definition=definition;
    result=partial(qId);check(result.state==ExecutionState::Done && result.acquisitions==0 && result.executedTools==QStringList{qId},"TargetOnly assignment uses cached bundle without acquisition/upstream rerun");check(session.acceptTool(result,session.signature(context),context),"Session accepts target assignment");
    result=partial(consumerId);check(result.state==ExecutionState::Done && result.acquisitions==0 && result.executedTools==QStringList{consumerId},"TargetOnly consumer uses cached bundle");
    const auto priorSignature=ExecutionService::toolSignature(context.definition,dir.path(),qId,"scope");
    auto renamed=x;renamed.name="Renamed X";context.definition=withRecipeVariables(context.definition,{renamed,y});check(ExecutionService::toolSignature(context.definition,dir.path(),qId,"scope")!=priorSignature,"Variable catalog edits invalidate assignment signature");session.reconcile(context);result=partial(qId);check(result.executedTools.isEmpty() && tool(result,qId).state==ExecutionState::Skipped && result.acquisitions==0,"Stale upstream has no full-flow fallback");full();
    auto tools=editorTools(context.definition);for(auto& t:tools)if(t.id==blobId)t.parameters["threshold"]=200;context.definition=withEditorTools(context.definition,tools);session.reconcile(context);result=partial(qId);check(result.executedTools.isEmpty() && tool(result,qId).outputs.isEmpty(),"Transitive upstream edit invalidates downstream bundle");
    cycle=full();session.invalidateTool(context,blobId);result=partial(qId);check(result.executedTools.isEmpty(),"Explicit invalidation traverses assignment dependency edges");
    auto unknown=q;unknown.rows.append({id(),ConstantAssignmentSource{constant}});context.definition=make(p,unknown);cycle=ExecutionService{}.run(context.definition,dir.path(),"recipe","config",{});check(cycle.state==ExecutionState::Error && tool(cycle,qId).outputs.isEmpty() && tool(cycle,consumerId).state==ExecutionState::Skipped,"Atomic assignment failure leaves no bundle and skips consumer");
    context.definition=withMultiSourceSchema(definition);cycle=full();result=partial(qId);check(result.state==ExecutionState::Done && result.acquisitions==0,"Schema9 assignment and TargetOnly preserve cycle identity");
    auto secondSource=source;secondSource.id=id();secondSource.name="Source2";auto secondBlob=blob;secondBlob.id=id();secondBlob.name="Blob2";
    auto joined=p;joined.rows.append({y.id,ToolOutputAssignmentSource{secondBlob.id,"found",4}});
    auto multiTools=editorTools(withMultiSourceSchema(make(joined,q)));multiTools.append(secondSource);multiTools.append(secondBlob);
    auto joinedDefinition=withEditorTools(withMultiSourceSchema(make(joined,q)),multiTools);auto links=editorConnections(joinedDefinition);links.append({secondSource.id,"image",secondBlob.id,"image"});joinedDefinition=withEditorConnections(joinedDefinition,links);
    check(validateSourceGraph(editorTools(joinedDefinition),editorConnections(joinedDefinition)).valid(),"Cycle data bundle may join two geometric sources");
    cycle=ExecutionService{}.run(joinedDefinition,dir.path(),"recipe","config",{});check(cycle.state==ExecutionState::Done && cycle.acquisitions==2 && std::get<AssignmentBundle>(tool(cycle,pId).outputs["bundle"].data).fields.size()==2,"Multi-source assignment full flow commits both actual values");
    ValueEnvelope large;large.type=ScalarType::Int64;large.value=qint64(9007199254740993LL);large.validity=ValueValidity::Valid;
    auto largeConfig=p;largeConfig.rows={{x.id,ConstantAssignmentSource{large}}};const auto roundtrip=decodeAssignmentConfig(encodeAssignmentConfig(largeConfig),pId);check(std::get<qint64>(*std::get<ValueEnvelope>(std::get<ConstantAssignmentSource>(roundtrip.rows[0].source).value).value)==9007199254740993LL,"Config preserves Int64 above JSON double precision");
    for(auto type:{ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement}){Recipe recipe;recipe.applicationType=type;recipe.definition=definition;cycle=ExecutionService{}.run(recipe.definition,dir.path(),"recipe","config",{});check(cycle.state==ExecutionState::Done,"All application types use same assignment engine");}
    QCoreApplication::processEvents();check(networkCalls==0 && !endpoint.hasPendingConnections(),"Editor Run Tool/Flow network calls zero");
    QTextStream(stdout)<<checks<<" assignment DAG/execution/session checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
