#include "application/RuntimeCoordinator.hpp"
#include "application/WebcamAdapter.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QLockFile>
#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QTextStream>
#include <future>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void rejects(F f,ProductionError code){try{f();}catch(const ProductionException& e){check(e.code()==code,"rejection code");return;}throw std::runtime_error("Expected rejection");}
void write(const QString& path,const QJsonObject& json){QFile file(path);check(file.open(QIODevice::WriteOnly),"manifest open");const auto bytes=QJsonDocument(json).toJson();check(file.write(bytes)==bytes.size(),"manifest write");}
class FailedRead final:public WebcamCapture {
public:bool open(const WebcamSettings&)override{return true;}
    bool read(cv::Mat&)override{return false;}
};
bool has(const QJsonObject& trace,const QString& action,const QString& outcome={}){
    for(const auto& value:trace["events"].toArray()){const auto e=value.toObject();if(e["action"]==action && (outcome.isEmpty() || e["outcome"]==outcome))return true;}return false;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto root=app.applicationDirPath();QTemporaryDir dir(root+"/a1-fixture-XXXXXX");check(dir.isValid(),"fixture directory");dir.setAutoRemove(false);
    ProjectService project;project.createProject("A1 synthetic diagnostics","",dir.filePath("part.vfproject"),"file");
    project.createRecipe("File","",ApplicationType::DefectInspection,"file");
    QImage image(64,48,QImage::Format_Grayscale8);image.fill(120);const auto imagePath=dir.filePath("synthetic.png");check(image.save(imagePath),"synthetic fixture");
    project.setImageFile(imagePath);project.save();const auto recipe=*project.activeRecipe();
    const RuntimeIdentity identity{"fixture",[](const QString&,ProductionAction){return true;}};
    const auto archive=project.path()+".production";RuntimeService runtime(archive,"",identity);RuntimeCoordinator c(runtime);
    c.load(recipe,dir.path());c.start();const auto success=c.begin(RuntimeTrigger::Manual);
    rejects([&]{c.begin(RuntimeTrigger::Manual);},ProductionError::Busy);
    const auto successCancel=c.cancellation();
    auto done=std::async(std::launch::async,[&]{return RuntimeService::execute(success,"",successCancel);});
    auto result=done.get();c.finish(success.ticket,result);
    check(result.state==ExecutionState::Done && c.state()==RuntimeState::Running,"successful cycle keeps Running");
    const auto successful=runtime.diagnostics()->snapshot();check(successful["firstFailure"].toObject().isEmpty(),"Busy and success do not pin a failure");
    check(has(successful,"admission","accepted") && has(successful,"acquisition") && has(successful,"completion") && has(successful,"record","committed"),"successful chain");
    const auto failing=c.begin(RuntimeTrigger::Manual);check(QFile::remove(imagePath),"inject missing file after admission");
    result=RuntimeService::execute(failing,"",c.cancellation());check(result.state==ExecutionState::Error,"real acquisition failure");
    const auto first=runtime.diagnostics()->snapshot()["firstFailure"].toObject();
    check(first["action"]=="acquisition" && first["ticket"]==QString::number(failing.ticket) && first["cycleId"]==result.cycleId && !first["sourceId"].toString().isEmpty() && !first["reason"].toString().isEmpty(),"first source cause correlated to lease and cycle");
    check(image.save(imagePath),"restore synthetic fixture");
    rejects([&]{c.finish(failing.ticket+100,result);},ProductionError::InvalidCompletion);
    QLockFile lock(archive+"/archive.lock");check(lock.tryLock(),"inject record lock");
    rejects([&]{c.finish(failing.ticket,result);},ProductionError::PersistenceFailure);
    check(c.recordingPending() && runtime.busy(),"pending record gate unchanged");lock.unlock();c.finish(failing.ticket,result);
    check(c.state()==RuntimeState::Fault,"invalid completion closes admission even after corrected terminal record");c.stop();c.start();
    auto changed=recipe;auto tools=editorTools(changed.definition);tools.first().parameters["path"]=dir.filePath("changed.png");changed.definition=withEditorTools(changed.definition,tools);runtime.observeInstallation(changed);
    rejects([&]{c.begin(RuntimeTrigger::Manual);},ProductionError::DependencyChanged);
    const auto failed=runtime.diagnostics()->snapshot();check(failed["firstFailure"].toObject()==first,"completion, record and admission errors preserve original cause");
    bool invalid=false,pending=false,fault=false;
    for(const auto& value:failed["events"].toArray()){const auto e=value.toObject();
        if(e["action"]=="record" && e["failure"].toBool()){check(e["ticket"].toString()==QString::number(failing.ticket) || e["ticket"].toString()==QString::number(failing.ticket+100),"record ticket");check(e["cycleId"]==result.cycleId,"record cycle correlation");invalid|=e["errorCode"].toInt()==int(ProductionError::InvalidCompletion);pending|=e["recordPending"].toBool();}
        fault|=e["action"]=="begin" && e["stateBefore"]=="Running" && e["stateAfter"]=="Fault";
    }check(invalid && pending && fault,"distinct completion, persistence and admission diagnostics");

    // Explicit editor9 raw source, injected through the existing adapter factory.
    project.createRecipe("Webcam read injection","",ApplicationType::DefectInspection,"webcam");project.enableMultiSource();const auto camera=project.addTool("webcam"),blob=project.addTool("blob");project.setConnections({{camera,"image",blob,"image"}});project.save();
    RuntimeService cameraRuntime(dir.filePath("camera.production"),"",identity,{},{},{},[&](const Recipe&){MultiSourceDependencies d;d.sources.insert(camera,{});return d;});RuntimeCoordinator cameraCoordinator(cameraRuntime);
    cameraCoordinator.load(*project.activeRecipe(),dir.path());cameraCoordinator.start();const auto lease=cameraCoordinator.begin(RuntimeTrigger::Manual);const auto cancel=cameraCoordinator.cancellation();
    const auto cameraError=std::async(std::launch::async,[&]{return RuntimeService::execute(lease,"",cancel,[](const ToolDefinition&){return std::make_unique<WebcamAdapter>(WebcamSettings{},[]{return std::make_unique<FailedRead>();});});}).get();
    check(cameraError.state==ExecutionState::Error,"webcam read fixture error");cameraCoordinator.finish(lease.ticket,cameraError);
    const auto cameraTrace=cameraRuntime.diagnostics()->snapshot();check(cameraTrace["firstFailure"].toObject()["sourceId"]==camera && cameraTrace["firstFailure"].toObject()["reason"].toString().contains("Webcam"),"webcam source and original reason preserved");
    check(cameraCoordinator.state()==RuntimeState::Running && cameraRuntime.records().size()==1,"read failure does not change coordinator policy");

    QLoggingCategory::setFilterRules("visionflow.runtime.info=false"); // avoid duplicating bounded stress text in CTest output
    RuntimeDiagnostics bounded;bounded.append({{"action","original"},{"reason","first"}},true);const QJsonValue pinned=bounded.snapshot()["firstFailure"];
    for(int i=0;i<270;++i)bounded.append({{"action","later"},{"reason",QString(3000,'x')}},true);
    check(bounded.snapshot()["events"].toArray().size()==256 && bounded.snapshot()["firstFailure"]==pinned,"bounded ring retains first failure after eviction");
    check(bounded.snapshot()["events"].toArray().last().toObject()["reason"].toString().size()==2048,"bounded error text");
    const auto manifest=failed["manifest"].toObject();check(manifest["exeSha256"].toString().size()==64 && manifest["buildType"]=="Release","actual executable hash and Release configuration");
    QFile exe(manifest["exePath"].toString());check(exe.open(QIODevice::ReadOnly),"verify executable");QCryptographicHash hash(QCryptographicHash::Sha256);check(hash.addData(&exe) && QString::fromLatin1(hash.result().toHex())==manifest["exeSha256"],"manifest hash independently verified");
    const QJsonObject report{{"fixtureDirectory",dir.path()},{"projectId",project.project().id},{"plc",QJsonObject{{"listening",false},{"pendingAck",false}}},{"success",successful},{"failure",failed},{"webcamFailure",cameraTrace},{"checks",checks}};
    write(root+"/a1-runtime-diagnostics.json",report);
    QTextStream(stdout)<<"PASS: "<<checks<<" diagnostic checks; "<<root<<"/a1-runtime-diagnostics.json\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
