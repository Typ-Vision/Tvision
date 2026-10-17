#include "application/ProjectService.hpp"
#include "application/BackendCapabilities.hpp"
#include "application/ToolDependencyResolver.hpp"
#include "application/ProductionRecipeService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
QByteArray read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"read fixture");return f.readAll();}
void write(const QString& path,QByteArray bytes){QFile f(path);check(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size(),"write fixture");}
QString digest(QByteArray b){return QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex());}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("migration-XXXXXX"));check(dir.isValid(),"isolated build fixture");
        ProjectService project;const auto path=dir.filePath("test.vfproject");project.createProject("Migration","",path,"ImageFile");
        auto id=project.createRecipe("Part","",ApplicationType::Measurement,"ImageFile");project.addTool("image_file");auto t=project.tools()[0];t.parameters["path"]="input.pgm";project.updateTool(t);project.save();
        write(dir.filePath("input.pgm"),QByteArray("P5\n2 2\n255\n\0\0\0\0",15)); // real independent small image asset
        const auto version=project.activeRecipe()->recipeVersion;const auto original=read(path);const auto originalHash=digest(original);
        project.openProject(path);project.selectRecipe(id);project.updateTool(project.tools()[0]);project.save();
        check(read(path)==original && digest(read(path))==originalHash && project.activeRecipe()->recipeVersion==version,"legacy no-op preserves exact bytes/hash/revision");
        check(project.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==6,"legacy remains schema 6");
        const QByteArray model="independent model bytes";write(dir.filePath("model.bin"),model);
        t=project.tools()[0];t.dependencies={{ToolDependencyKind::Model,"model","fixture/1",digest(model),"model.bin"},{ToolDependencyKind::Asset,"input","raw/1",digest(read(dir.filePath("input.pgm"))),"input.pgm"}};
        project.updateTool(t);check(project.dirty() && project.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==7,"dependency edit opts into schema 7");
        project.save();project.openProject(path);project.selectRecipe(id);
        check(project.tools()[0].dependencies==t.dependencies && project.activeRecipe()->recipeVersion==version+1,"typed dependencies reopen with one revision");
        const auto v7bytes=read(path);project.updateTool(project.tools()[0]);project.save();check(read(path)==v7bytes,"schema 7 no-op exact bytes");
        const auto resolved=ToolDependencyResolver::resolve(project.tools()[0],dir.path());check(resolved.size()==2,"model/asset hashes independently resolved");
        const auto authorize=[](const QString&,ProductionAction){return true;};ProductionRecipeService runtime(authorize);
        auto recipe=*project.activeRecipe();auto validation=runtime.validate("engineer",recipe,dir.path());runtime.activate("engineer",validation.id,recipe,dir.path());
        auto signature=ExecutionService::toolSignature(recipe.definition,dir.path(),t.id,"scope");
        write(dir.filePath("model.bin"),"tampered");
        reject([&]{runtime.beginCycle("engineer");},"tampered model blocks immutable snapshot execution");
        check(ExecutionService::toolSignature(recipe.definition,dir.path(),t.id,"scope")!=signature,"tampered dependency invalidates target cache");
        reject([&]{ToolDependencyResolver::resolve(t,dir.path());},"tamper hash rejected");write(dir.filePath("model.bin"),model);
        auto lease=runtime.beginCycle("engineer");runtime.abandonCycle(lease.ticket,"test boundary");
        QFile::remove(dir.filePath("model.bin"));reject([&]{runtime.beginCycle("engineer");},"missing model blocks snapshot");write(dir.filePath("model.bin"),model);
        auto bad=t;bad.dependencies[0].path="../outside.bin";reject([&]{encodeToolDependencies(bad.dependencies);},"path traversal rejected");
        bad=t;bad.dependencies[0].contentId="forged";reject([&]{encodeToolDependencies(bad.dependencies);},"malformed content ID rejected");
        bad=t;bad.dependencies.append(bad.dependencies[0]);reject([&]{encodeToolDependencies(bad.dependencies);},"duplicate dependency rejected");
        bad=t;bad.dependencies={{ToolDependencyKind::Backend,"missing-reader","1",QString(64,'a'),""}};
        reject([&]{ToolDependencyResolver::resolve(bad,dir.path());},"missing backend rejected");
        auto cap=BackendCapabilities::get("zxing-reader");
        if(cap.available){bad.dependencies={{ToolDependencyKind::Backend,cap.id,cap.version,cap.contentId,""}};check(ToolDependencyResolver::resolve(bad,dir.path()).size()==1,"actual backend identity accepted");bad.dependencies[0].contentId=QString(64,'0');reject([&]{ToolDependencyResolver::resolve(bad,dir.path());},"backend hash tamper rejected");}
        else {bad.dependencies={{ToolDependencyKind::Backend,"zxing-reader","2.3.0",QString(64,'a'),""}};reject([&]{ToolDependencyResolver::resolve(bad,dir.path());},"reader-free build rejects required reader");}
        auto definition=project.activeRecipe()->definition;auto editor=definition["editor"].toObject();editor["schemaVersion"]=10;definition["editor"]=editor;reject([&]{validateEditorDefinition(definition);},"future editor schema rejected");
        definition=project.activeRecipe()->definition;editor=definition["editor"].toObject();auto tools=editor["tools"].toArray();auto tool=tools[0].toObject();tool["enabled"]=false;auto deps=tool["dependencies"].toArray();auto dep=deps[0].toObject();dep["unknown"]=1;deps[0]=dep;tool["dependencies"]=deps;tools[0]=tool;editor["tools"]=tools;definition["editor"]=editor;
        reject([&]{validateEditorDefinition(definition);},"unknown config in disabled tool rejected");
        t=project.tools()[0];t.dependencies.clear();project.updateTool(t);check(project.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==7,"schema 7 never silently downgraded");
        QTextStream(stdout)<<checks<<" checks passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}
}
