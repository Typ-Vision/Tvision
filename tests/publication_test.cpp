#include "application/PublicationService.hpp"
#include "application/ProjectService.hpp"
#include "application/ExecutionService.hpp"
#include "application/EditorSession.hpp"
#include "application/RuntimeService.hpp"
#include "application/MultiSourceRuntimeService.hpp"
#include "application/ToolCatalog.hpp"
#include "application/RuntimeCoordinator.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QTcpServer>
#include <QImage>
#include <QTextStream>
#include <QUuid>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f,const char* s){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,s);}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(!ToolCatalog::canAdd("publish_data"),"core tool stays hidden until publication UI card");
    QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/publication-XXXXXX");check(dir.isValid(),"fixture");
    QTcpServer observer;check(observer.listen(QHostAddress::LocalHost,0),"observer");
    CommunicationProfiles store(dir.filePath("machine.json"));EndpointProfile profile;profile.id=id();profile.enabled=true;profile.port=observer.serverPort();profile.credentialRef="vault:machine-token";profile.fieldMapping={{"accepted","physical/address"}};
    store.save({{profile.id,profile}});check(store.load().value(profile.id).credentialRef==profile.credentialRef,"machine local roundtrip");
    auto changed=profile;changed.port=profile.port==65535 ? 5000:profile.port+1;
    rejects([&]{store.save({{changed.id,changed}});},"profile edit requires revision advance");
    VariableDefinition variable{id(),"Accepted"};ValueEnvelope constant;constant.type=ScalarType::Boolean;constant.validity=ValueValidity::Valid;constant.value=false;
    ProjectService project;project.createProject("Publication","",dir.filePath("project.vfproject"),"");const auto recipe=project.createRecipe("Part","",ApplicationType::DefectInspection);
    QImage image(16,16,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("part.png")),"image");project.setImageFile(dir.filePath("part.png"));variable.id.clear();variable.id=project.addVariable(variable);
    const auto producer=project.addTool("set_variables");for(auto t:project.tools())if(t.id==producer){t.parameters=encodeAssignmentConfig({producer,1,{{variable.id,ConstantAssignmentSource{constant}}}});project.updateTool(t);}
    OutputGroup a{id(),profile.id,1,true,"vf.fields/1",producer,1,{{"accepted",variable.id}}};auto b=a;b.id=id();b.required=false;
    project.setOutputGroups({a,b});const auto publisher=project.addTool("publish_data");for(auto t:project.tools())if(t.id==publisher){t.parameters=encodePublishConfig({{a.id,b.id}});t.enabled=true;project.updateTool(t);}check(project.moveTool(publisher,-1),"publisher can precede producer in editor order");project.save();
    const auto d=project.activeRecipe()->definition;const auto frozenRecipe=*project.activeRecipe();const auto content=ProductionRecipeService::recipeContent(frozenRecipe,dir.path());project.save();check(project.activeRecipe()->definition==d && !project.dirty(),"no-op Save");project.openProject(project.path());project.selectRecipe(recipe);check(project.activeRecipe()->definition==d,"project reopen");
    const auto bytes=QJsonDocument(d).toJson();check(!bytes.contains(profile.credentialRef.toUtf8()) && !bytes.contains("physical/address") && !bytes.contains("127.0.0.1"),"recipe contains logical data only");
    auto snapshot=PublicationService::capture(d,store);check(!observer.hasPendingConnections(),"capture does not connect");
    auto execution=ExecutionService{}.withPublicationSnapshot(snapshot);auto cycle=execution.run(d,dir.path(),recipe,"config",{});
    check(cycle.state==ExecutionState::Done && cycle.publicationIntents.size()==2,"one bundle two groups");
    check(cycle.publicationIntents[0].required && !cycle.publicationIntents[1].required && cycle.publicationIntents[0].id!=cycle.publicationIntents[1].id,"required optional intent identities");
    check(std::get<bool>(*std::get<ValueEnvelope>(cycle.publicationIntents[0].fields["accepted"]).value)==false,"false is valid publication value");
    check(execution.run(d,dir.path(),recipe,"config",std::make_shared<std::atomic_bool>(true)).publicationIntents.isEmpty(),"cancel no intents");
    check(ExecutionService{}.run(d,dir.path(),recipe,"config",{}).state==ExecutionState::Error,"unresolved profile is explicit execution error");
    EditorSession editor;EditorContext context{project.project().id,project.path(),recipe,d,{}};auto signature=editor.signature(context);auto full=execution.run(d,dir.path(),recipe,signature,{});check(editor.acceptCycle(full,signature,context),"Editor cache");auto partial=execution.runTool(d,dir.path(),recipe,signature,{},publisher,editor.evaluation(context,publisher));check(partial.partial && partial.acquisitions==0 && partial.publicationIntents.size()==2,"Editor Run Tool intent only");
    editor.invalidateTool(context,producer);check(editor.cycle && editor.cycle->publicationIntents.isEmpty(),"upstream invalidation removes old Editor intents");
    app.processEvents();check(!observer.hasPendingConnections(),"Editor Flow Tool and Load have zero network side effects");
    auto invalid=a;invalid.producerRevision=2;rejects([&]{project.setOutputGroups({invalid,b});},"producer revision mismatch");invalid=a;invalid.fields[0].fieldId=id();rejects([&]{project.setOutputGroups({invalid,b});},"field mismatch");invalid=a;for(int i=0;i<64;++i)invalid.fields.append({"field"+QString::number(i),variable.id});rejects([&]{encodeOutputGroup(invalid);},"65 fields");auto groups=QVector<OutputGroup>{};for(int i=0;i<17;++i){auto g=a;g.id=id();groups.append(g);}rejects([&]{withRecipeOutputGroups(d,groups);},"17 groups");
    AssignmentBundle bundle;for(const auto& t:cycle.tools)if(t.toolId==producer)bundle=std::get<AssignmentBundle>(t.outputs.value("bundle").data);
    QMap<QString,AssignmentBundle> bundles{{producer,bundle}};bundle.cycleId="other";rejects([&]{PublicationService::evaluate({{a.id}},d,{{producer,bundle}},cycle.cycleId,"636f6e666967",*snapshot);},"mixed cycle");
    bundle=bundles[producer];bundle.revision=2;rejects([&]{PublicationService::evaluate({{a.id}},d,{{producer,bundle}},cycle.cycleId,"636f6e666967",*snapshot);},"bundle revision");bundle=bundles[producer];auto v=std::get<ValueEnvelope>(bundle.fields[variable.id]);v.validity=ValueValidity::Missing;v.value.reset();bundle.fields[variable.id]=v;rejects([&]{PublicationService::evaluate({{a.id}},d,{{producer,bundle}},cycle.cycleId,"636f6e666967",*snapshot);},"missing required field");
    bundle=bundles[producer];v=std::get<ValueEnvelope>(bundle.fields[variable.id]);v.type=ScalarType::Int32;v.value=qint32(1);bundle.fields[variable.id]=v;rejects([&]{PublicationService::evaluate({{a.id}},d,{{producer,bundle}},cycle.cycleId,"636f6e666967",*snapshot);},"catalog type mismatch");
    bundle=bundles[producer];v=std::get<ValueEnvelope>(bundle.fields[variable.id]);v.unit=ScalarUnit::Millimetre;bundle.fields[variable.id]=v;rejects([&]{PublicationService::evaluate({{a.id}},d,{{producer,bundle}},cycle.cycleId,"636f6e666967",*snapshot);},"catalog unit mismatch");
    auto again=PublicationService::evaluate({{a.id,b.id}},d,bundles,cycle.cycleId,"636f6e666967",*snapshot);check(again[0].id==cycle.publicationIntents[0].id,"retry identity stable without acquisition");
    auto large=a;large.fields.clear();auto largeProfile=profile;largeProfile.revision=2;
    for(int i=0;i<64;++i){const auto key="field"+QString::number(i);large.fields.append({key,variable.id});largeProfile.fieldMapping[key]="address/"+QString::number(i);}largeProfile.fieldMapping.remove("accepted");
    // Keep the second group's original mapping available within the 64 mapping limit.
    largeProfile.fieldMapping.remove("field63");largeProfile.fieldMapping["accepted"]="physical/address";large.fields[63].name="accepted";
    store.save({{largeProfile.id,largeProfile}});auto largeVariable=variable;largeVariable.type=VariableType::String;auto largeDefinition=withRecipeOutputGroups(withRecipeVariables(d,{largeVariable}),{large,b});auto largeSnapshot=PublicationService::capture(largeDefinition,store);auto largeBundle=bundles[producer];auto text=std::get<ValueEnvelope>(largeBundle.fields[variable.id]);text.type=ScalarType::String;text.value=QString(2048,'x');largeBundle.fields[variable.id]=text;
    rejects([&]{PublicationService::evaluate({{large.id}},largeDefinition,{{producer,largeBundle}},cycle.cycleId,"636f6e666967",*largeSnapshot);},"64 KiB bounded payload");
    auto disabled=profile;disabled.enabled=false;disabled.revision=3;store.save({{disabled.id,disabled}});rejects([&]{PublicationService::capture(d,store);},"disabled required/optional profile explicit error");rejects([&]{PublicationService::requireCurrent(*snapshot,d,store);},"changed profile requires reload");check(execution.run(d,dir.path(),recipe,"config",{}).publicationIntents.size()==2,"existing immutable evaluation is unchanged");check(content==ProductionRecipeService::recipeContent(frozenRecipe,dir.path()),"local profile edit does not silently rewrite recipe content hash");
    changed=profile;changed.revision=4;changed.contractRevision=2;store.save({{changed.id,changed}});rejects([&]{PublicationService::capture(d,store);},"contract revision mismatch");store.save({});rejects([&]{PublicationService::capture(d,store);},"file transfer unresolved profile");
    changed=profile;changed.revision=4;changed.port=profile.port;store.save({{changed.id,changed}});
    ProductionRecipeService production([](const QString&,ProductionAction){return true;},[&](const Recipe& r){ProductionDependencies deps;deps.publication=PublicationService::capture(r.definition,store);return deps;});auto validation=production.validate("engineer",frozenRecipe,dir.path());production.activate("engineer",validation.id,frozenRecipe,dir.path());check(bool(production.active()->publication()),"production snapshot freezes local dependency");check(!QJsonDocument(production.active()->dependencies()).toJson().contains(profile.credentialRef.toUtf8()),"dependency logs exclude credentials");
    changed.revision=5;changed.timeoutMs+=1;store.save({{changed.id,changed}});rejects([&]{production.requireReady("engineer");},"active profile edit denies new admission");check(production.active()->validation().contentId==validation.contentId,"active config identity frozen");
    // Whole-set Runtime keeps cycle tools out of source branches.
    project.enableMultiSource();auto multi=*project.activeRecipe();MultiSourceRuntimeService runtime(RuntimeIdentity::localEngineer(),[&](const Recipe& r){MultiSourceDependencies deps;for(const auto& t:editorTools(r.definition))if(t.enabled && t.kind=="image_file")deps.sources[t.id]={};deps.publication=PublicationService::capture(r.definition,store);return deps;});runtime.prepare(multi,dir.path());auto lease=runtime.begin();auto result=RuntimeService::execute(lease,dir.filePath("catalog.json"),{});check(result.state==ExecutionState::Done && result.publicationIntents.size()==2,"Runtime snapshot executes publication");auto forged=result;forged.publicationIntents[0].required=false;rejects([&]{runtime.finish(lease.ticket,forged);},"intent mutation breaks trusted completion");rejects([&]{runtime.finish(lease.ticket,result);},"legacy terminal cannot silently discard publication intents before COMM-13");app.processEvents();check(!observer.hasPendingConnections(),"Runtime intent never sends");
    QTextStream(stdout)<<checks<<" publication checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}}
