#include "application/ProductionRecipeService.hpp"
#include "application/MatchService.hpp"
#include <QCoreApplication>
#include <QImage>
#include <QDir>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonArray>
#include <QUuid>
#include <QTextStream>
#include <thread>
using namespace visionflow;
void check(bool ok,const char* text) {if(!ok)throw std::runtime_error(text);}
template<class F> void rejects(F action,ProductionError expected,const char* text) {
    try {action();}catch(const ProductionException& e){check(e.code()==expected,text);return;}throw std::runtime_error(text);
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir directory(QDir::currentPath()+"/production-fixture-XXXXXX");check(directory.isValid(),"build fixture directory");
        QImage image(32,24,QImage::Format_Grayscale8);image.fill(0);
        for(int y=4;y<12;++y)for(int x=6;x<16;++x)image.scanLine(y)[x]=(x+y)%2 ? 200 : 250;
        check(image.save(directory.path()+"/input.png"),"real fixture PNG");
        ToolDefinition source{QUuid::createUuid().toString(),"image_file","Source","",true,{{"path","input.png"}}};
        ToolDefinition blob{QUuid::createUuid().toString(),"blob","Blob","",true,defaultParameters("blob")};
        Recipe draft;draft.id=QUuid::createUuid().toString();draft.name="Part";draft.cameraReference="ImageFile";draft.applicationType=ApplicationType::Measurement;
        draft.createdAt=draft.modifiedAt=QDateTime::currentDateTimeUtc();
        draft.definition=withEditorConnections(withEditorTools({}, {source,blob}),{{source.id,"image",blob.id,"image"}});
        auto authorize=[](const QString& principal,ProductionAction action){return principal=="engineer" || (principal=="operator" && action==ProductionAction::Run);};
        ProductionDependencies dependencies;dependencies.assetHashes["fixture-asset"]=QString(64,'a');
        ProductionRecipeService service(authorize,[&](const Recipe&){return dependencies;});
        ProductionRecipeService denied;
        rejects([&]{denied.validate("engineer",draft,directory.path());},ProductionError::Unauthorized,"deny by default");
        draft.status=RecipeStatus::Valid;
        rejects([&]{service.activate("engineer","forged",draft,directory.path());},ProductionError::UnknownValidation,"disk VALID cannot activate");
        const auto v1=service.validate("engineer",draft,directory.path());
        check(v1.scope=="execution_configuration" && v1.checkedAtUtc.timeSpec()==Qt::UTC,"explicit validation scope and UTC");
        rejects([&]{service.activate("operator",v1.id,draft,directory.path());},ProductionError::Unauthorized,"activation role enforced");
        service.activate("engineer",v1.id,draft,directory.path());
        rejects([&]{service.beginCycle("guest");},ProductionError::Unauthorized,"run role enforced");
        rejects([&]{service.rollback("operator",v1.id);},ProductionError::Unauthorized,"rollback role enforced");
        rejects([&]{service.validate("operator",draft,directory.path());},ProductionError::Unauthorized,"validation role enforced");
        const auto lease=service.beginCycle("operator");
        Recipe edited=draft;edited.name="Changed";edited.recipeVersion=2;
        auto editedBlob=blob;editedBlob.parameters["threshold"]=230;
        edited.definition=withEditorTools(edited.definition,{source,editedBlob});
        const auto v2=service.validate("engineer",edited,directory.path());
        check(lease.snapshot->recipe().name=="Part" && editorTools(lease.snapshot->recipe().definition)[1].parameters["threshold"]==128,"draft changes preserve snapshot");
        rejects([&]{service.activate("engineer",v2.id,edited,directory.path());},ProductionError::Busy,"switch waits for boundary");
        rejects([&]{service.rollback("engineer",v1.id);},ProductionError::Busy,"rollback waits for boundary");
        rejects([&]{service.beginCycle("operator");},ProductionError::Busy,"one in-flight cycle");
        const auto result=ExecutionService{}.run(lease.snapshot->recipe().definition,lease.snapshot->projectDirectory(),draft.id,v1.contentId,{});
        check(result.state==ExecutionState::Done && result.frames.size()==1,"actual file execution");
        auto wrong=result;wrong.configId="foreign";
        rejects([&]{service.finishCycle(lease.ticket,wrong);},ProductionError::InvalidCompletion,"foreign result rejected");
        wrong=result;wrong.frames[0].cycleId="foreign";
        rejects([&]{service.finishCycle(lease.ticket,wrong);},ProductionError::InvalidCompletion,"foreign frame rejected");
        service.finishCycle(lease.ticket,result);
        rejects([&]{service.finishCycle(lease.ticket,result);},ProductionError::InvalidCompletion,"duplicate completion rejected");
        const auto log=service.cycleRecords().last();
        check(log["recipeVersion"]==1 && log["validationId"]==v1.id && log["cycleId"]==result.cycleId && log["frames"].toArray()[0].toObject()["sourceId"]==source.id && log["dependencies"].toObject()["engine"]==ExecutionService::version(),"cycle version engine/source trace");
        check(!log.contains("partDecision") && !log.contains("pose"),"no invented PASS or pose");
        check(log["tools"].toArray()[1].toObject()["outputs"].toObject()["area"].toObject()["value"]==80,"actual numeric result in cycle record");
        auto replay=service.beginCycle("operator");
        rejects([&]{service.finishCycle(replay.ticket,result);},ProductionError::InvalidCompletion,"duplicate cycle ID rejects replay on new ticket");
        service.abandonCycle(replay.ticket,"replay rejected");
        rejects([&]{service.activate("engineer",v1.id,edited,directory.path());},ProductionError::DependencyChanged,"old validation rejects changed draft");
        service.activate("engineer",v2.id,edited,directory.path());service.rollback("engineer",v1.id);
        check(service.active()->recipe().recipeVersion==1,"rollback to retained immutable version");
        dependencies.assetHashes["fixture-asset"]=QString(64,'b');
        rejects([&]{service.beginCycle("operator");},ProductionError::DependencyChanged,"changed asset blocks next cycle");
        rejects([&]{service.rollback("engineer",v2.id);},ProductionError::DependencyChanged,"rollback rechecks dependencies");
        dependencies.assetHashes["fixture-asset"]=QString(64,'a');dependencies.engineVersion="future-engine";
        rejects([&]{service.beginCycle("operator");},ProductionError::DependencyChanged,"engine mismatch blocks cycle");
        dependencies.engineVersion=ExecutionService::version();
        auto stopped=service.beginCycle("operator");service.abandonCycle(stopped.ticket,"worker failed");
        check(service.cycleRecords().last()["state"]=="ERROR" && !service.busy(),"abandoned cycle records error");
        bool threadRejected=false;std::thread foreign([&]{try{service.active();}catch(const ProductionException& e){threadRejected=e.code()==ProductionError::WrongThread;}});foreign.join();check(threadRejected,"owner thread enforced");
        auto invalid=draft;invalid.definition=withEditorConnections(invalid.definition,{});
        rejects([&]{service.validate("engineer",invalid,directory.path());},ProductionError::InvalidConfiguration,"missing input rejects validation");
        invalid=draft;auto disabled=source;disabled.enabled=false;invalid.definition=withEditorTools(invalid.definition,{disabled,blob});
        rejects([&]{service.validate("engineer",invalid,directory.path());},ProductionError::InvalidConfiguration,"disabled dependency rejects validation");
        ToolDefinition match{QUuid::createUuid().toString(),"template_match","Match","",true,defaultParameters("template_match")};
        invalid=draft;invalid.definition=withEditorConnections(withEditorTools({}, {source,match}),{{source.id,"image",match.id,"image"}});
        rejects([&]{service.validate("engineer",invalid,directory.path());},ProductionError::InvalidConfiguration,"missing model rejects validation");
        match.parameters["modelPng"]="bm90IGEgcG5n";match.parameters["modelRoi"]=MatchService::rectangle({0,0,4,4});
        invalid.definition=withEditorTools(invalid.definition,{source,match});
        rejects([&]{service.validate("engineer",invalid,directory.path());},ProductionError::InvalidConfiguration,"corrupt model rejects validation");
        auto frame=ImageFileService{}.load(directory.path()+"/input.png");
        match.parameters["modelRoi"]=MatchService::rectangle({6,4,10,8});
        match.parameters["modelPng"]=MatchService{}.teach(frame,{6,4,10,8});
        invalid.definition=withEditorTools(invalid.definition,{source,match});
        const auto modelValidation=service.validate("engineer",invalid,directory.path());
        auto changedModel=match;changedModel.parameters["modelRoi"]=MatchService::rectangle({7,4,10,8});
        auto movedModel=invalid;movedModel.definition=withEditorTools(invalid.definition,{source,changedModel});
        rejects([&]{service.activate("engineer",modelValidation.id,movedModel,directory.path());},ProductionError::DependencyChanged,"model geometry bound to validation");
        ProductionRecipeService small(authorize,{},1);small.validate("engineer",draft,directory.path());
        rejects([&]{small.validate("engineer",edited,directory.path());},ProductionError::HistoryFull,"history bounded without eviction");
        ProductionRecipeService fresh(authorize);
        rejects([&]{fresh.activate("engineer",v1.id,draft,directory.path());},ProductionError::UnknownValidation,"validation not trusted across restart");
        dependencies.calibration=CalibrationDependency{"calibration",1,"cal-content","camera","lens",{32,24},"plane",{"image"},{"plane",LengthUnit::Millimetre,AxisConvention::XRightYUp}};
        auto calibrated=draft;calibrated.calibrationReference="calibration";
        auto vc=service.validate("engineer",calibrated,directory.path());service.activate("engineer",vc.id,calibrated,directory.path());
        ++dependencies.calibration->version;
        rejects([&]{service.beginCycle("operator");},ProductionError::DependencyChanged,"calibration version invalidates");
        dependencies.calibration.reset();
        rejects([&]{service.validate("engineer",calibrated,directory.path());},ProductionError::DependencyChanged,"unresolved calibration rejects validation");
        service.rollback("engineer",v1.id);
        for(const auto type:{ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement}) {
            auto typed=draft;typed.applicationType=type;
            const auto validation=service.validate("engineer",typed,directory.path());
            service.activate("engineer",validation.id,typed,directory.path());
            check(service.active()->recipe().applicationType==type,"three types share snapshot service");
        }
        service.rollback("engineer",v1.id);
        check(QFile::remove(directory.path()+"/input.png"),"remove cycle source fixture");
        auto failedLease=service.beginCycle("operator");
        auto failedResult=ExecutionService{}.run(failedLease.snapshot->recipe().definition,failedLease.snapshot->projectDirectory(),draft.id,v1.contentId,{});
        check(failedResult.state==ExecutionState::Error,"missing dynamic acquisition source is execution error");
        service.finishCycle(failedLease.ticket,failedResult);
        check(service.cycleRecords().last()["state"]=="ERROR" && !service.cycleRecords().last().contains("partDecision"),"acquisition error stays distinct from part decision");
        for(int i=0;i<130;++i){auto cycle=service.beginCycle("operator");service.abandonCycle(cycle.ticket,"fixture cancellation");}
        check(service.cycleRecords().size()==128,"transient cycle log bounded");
        QTextStream(stdout)<<"Production recipe contract passed\n";return 0;
    } catch(const std::exception& e) {QTextStream(stderr)<<e.what()<<"\n";return 1;}
}
