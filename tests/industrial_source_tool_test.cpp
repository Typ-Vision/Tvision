#include "application/ProjectService.hpp"
#include "application/ProductionRecipeService.hpp"
#include "application/SystemCalibration.hpp"
#include "application/ToolExecutors.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f){try{f();}catch(const std::exception&){return;}throw std::runtime_error("expected rejection");}
QByteArray bytes(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"read project");return f.readAll();}
MeasurementSetupContract fixture(){
    MeasurementSetupContract c;
    c.cameras.append({{"camera"},"Camera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},PhysicalDeviceIdentity{"vendor","model","serial",DeviceIdentityConfidence::SerialReported}});
    c.setups.append({{"setup"},{"camera"},"Setup","lens","plane",1,1,{{640,480},QRect(0,0,640,480),QSize(1,1),true,QSize(1,1),GeometryMirror{}}});
    AcquisitionSettings settings;settings.pixelFormat="Mono8";settings.exposureTimeUs=10000;settings.trigger={true,"FrameStart","Software"};
    c.profiles.append({{"profile"},{"camera"},{"setup"},"Profile",1,settings});c.validate();return c;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("icm07a-XXXXXX"));check(dir.isValid(),"build fixtures");
    const auto descriptor=ToolRegistry::get("industrial_camera");
    check(descriptor.version==1 && descriptor.minimumEditorSchema==9 && descriptor.acquisitionSource && descriptor.inputs.isEmpty(),"source kind/version/schema");
    check(descriptor.outputs.size()==1 && descriptor.outputs[0].type==PortType::Image && descriptor.outputs[0].unit=="px","Image/px output");
    const auto contract=fixture();const auto environment=contract.toJson();int index=0;
    for(int schema=1;schema<=8;++schema){
        ProjectService old;const auto path=dir.filePath(QString("legacy-%1.vfproject").arg(schema));
        old.createProject("Legacy","",path,"file");const auto id=old.createRecipe("Recipe","",ApplicationType::Measurement,"file");old.addTool("image_file");
        auto project=old.project();auto definition=project.recipes[0].definition;auto editor=definition["editor"].toObject();
        auto tool=editor["tools"].toArray()[0].toObject();if(schema>=7)tool["dependencies"]=QJsonArray{};else tool.remove("dependencies");
        editor["schemaVersion"]=schema;editor["tools"]=QJsonArray{tool};definition["editor"]=editor;project.recipes[0].definition=definition;
        ProjectRepository{}.save(path,project);const auto original=bytes(path);ProjectService loaded;loaded.openProject(path);loaded.selectRecipe(id);
        const auto signature=ExecutionService::toolSignature(definition,dir.path(),tool["id"].toString(),"legacy");
        loaded.updateTool(loaded.tools()[0]);loaded.save();check(bytes(path)==original && loaded.activeRecipe()->definition==definition,"editor1-8 no-op bytes/schema");
        check(ExecutionService::toolSignature(loaded.activeRecipe()->definition,dir.path(),tool["id"].toString(),"legacy")==signature,"editor1-8 hash stable");
    }
    for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        ProjectService s;s.createProject("Industrial","",dir.filePath(QString::number(index++)+".vfproject"),"file");
        const auto recipe=s.createRecipe("Recipe","",type,"file");const auto file=s.addTool("image_file");s.save();
        const auto legacy=bytes(s.path());const auto legacyDefinition=s.activeRecipe()->definition;
        s.openProject(s.path());s.selectRecipe(recipe);s.save();check(bytes(s.path())==legacy && s.activeRecipe()->definition==legacyDefinition,"legacy no-op bytes/schema");
        const auto source=s.addTool("industrial_camera");check(s.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==9,"explicit addition upgrades editor9");
        check(s.tools()[0].id==file && s.tools()[0].kind=="image_file","no implicit source migration");
        s.bindIndustrialSource(source,contract,{"profile"},5000);check(contract.toJson()==environment,"no environment writes");
        const auto branch=SystemCalibration::branch(*s.activeRecipe(),source);check(SystemCalibration::source(branch).id==source,"controlled calibration source selection");
        const auto edge=s.addTool("edge_point");check(editorConnections(s.activeRecipe()->definition).isEmpty(),"ambiguous source has no implicit edge binding");
        s.setConnections({{source,"image",edge,"image"}});const auto graph=validateSourceGraph(s.tools(),editorConnections(s.activeRecipe()->definition));
        check(graph.valid() && graph.toolSources[edge]==source,"industrial source graph origin");
        auto tool=s.tools()[1];check(tool.id==source,"source fixture");
        for(int schema=1;schema<9;++schema){auto bad=s.activeRecipe()->definition;auto editor=bad["editor"].toObject();editor["schemaVersion"]=schema;bad["editor"]=editor;reject([&]{validateEditorDefinition(bad);});}
        for(auto value:{QJsonValue(true),QJsonValue("profile"),QJsonValue(QJsonObject{})}){auto bad=tool;bad.parameters["sourceReference"]=value;reject([&]{s.updateTool(bad);});}
        for(const auto key:{"backend","producer","permission"}){auto bad=tool;bad.parameters[key]="unknown";reject([&]{s.updateTool(bad);});}
        auto unsupported=contract;unsupported.profiles[0].requested.trigger.selector="unknown";
        const auto before=s.activeRecipe()->definition;reject([&]{s.bindIndustrialSource(source,unsupported,{"profile"},5000);});reject([&]{s.bindIndustrialSource(source,contract,{"missing"},5000);});
        check(s.activeRecipe()->definition==before,"failed bind is atomic");
        s.bindSourceSetup(source,SetupReference{{"camera"},{"setup"},1}.toJson());
        reject([&]{s.bindSourceSetup(source,SetupReference{{"other"},{"setup"},1}.toJson());});
        s.save();const auto saved=bytes(s.path());const auto hash=ExecutionService::toolSignature(s.activeRecipe()->definition,dir.path(),source,"scope");
        s.openProject(s.path());s.selectRecipe(recipe);s.bindIndustrialSource(source,contract,{"profile"},5000);s.updateTool(s.tools()[1]);s.save();
        check(bytes(s.path())==saved && !s.dirty(),"new source roundtrip/no-op exact bytes");
        check(ExecutionService::toolSignature(s.activeRecipe()->definition,dir.path(),source,"scope")==hash,"reopen hash stable");
        s.bindCalibration({"cal",1,"content",source,"lens",{640,480},"plane",{source+"/image"},{"plane",LengthUnit::Millimetre,AxisConvention::XRightYDown}});
        s.bindIndustrialSource(source,contract,{"profile"},5000);check(s.activeRecipe()->calibrationBinding.has_value(),"no-op bind preserves calibration");
        s.bindIndustrialSource(source,contract,{"profile"},6000);check(ExecutionService::toolSignature(s.activeRecipe()->definition,dir.path(),source,"scope")!=hash && !s.activeRecipe()->calibrationBinding,"explicit timeout invalidates source signature and calibration binding");
        const auto cycle=ExecutionService{}.run(branch.definition,dir.path(),recipe,"config",std::make_shared<std::atomic_bool>(false));
        check(cycle.state==ExecutionState::Error && cycle.frames.isEmpty(),"industrial execution fails without real frames");
        reject([]{ToolExecutors::get("industrial_camera");});
        ProductionRecipeService runtime([](const QString&,ProductionAction){return true;});reject([&]{runtime.validate("engineer",*s.activeRecipe(),dir.path());});
    }
    QTextStream(stdout)<<"PASS: explicit industrial source, editor9 migration, portable binding, mixed graph, legacy/new no-op hashes and closed execution/production\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
