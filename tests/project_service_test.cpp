#include "application/ProjectService.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>

using namespace visionflow;
namespace {
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class F> void rejected(F action, const char* message) {
    bool failed=false; try { action(); } catch (const std::exception&) { failed=true; } check(failed,message);
}
void write(const QString& path, const QByteArray& bytes) {
    QFile file(path); check(file.open(QIODevice::WriteOnly),"fixture open"); check(file.write(bytes)==bytes.size(),"fixture write");
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("project-test-XXXXXX"));
        check(dir.isValid(),"temporary directory");
        const QString path=dir.filePath("test.vfproject");
        ProjectService service; service.createProject("Test Projesi","Açıklama",path,"ImageFile","RobotModel","TCP Socket","Cal_1");
        const auto projectId=service.project().id;
        rejected([&]{service.openProject(path);},"dirty project must not be replaced");
        service.save(); check(!service.dirty(),"initial project saved");
        QStringList ids;
        for (int type=0;type<3;++type) {
            ids.append(service.createRecipe("Part_"+QString::number(type),"Tür testi",static_cast<ApplicationType>(type),"Camera_1","Cal_1"));
            check(service.activeRecipe()->status==RecipeStatus::Draft,"new recipe must be DRAFT");
        }
        rejected([&]{service.createRecipe("part_0","",ApplicationType::Measurement,"ImageFile");},"case insensitive unique name");
        check(service.project().recipes.size()==3,"failed validation is non-mutating");
        rejected([&]{service.editRecipe(ids[0],"Part_0","","","");},"empty camera must fail");
        rejected([&]{service.createRecipe("","",ApplicationType::Measurement,"ImageFile");},"empty name must fail");
        service.save(); service.openProject(path);
        check(service.project().id==projectId && service.project().recipes.size()==3,"project identity / recipes roundtrip");
        for (int type=0;type<3;++type) {
            const auto& r=service.project().recipes[type];
            check(r.id==ids[type] && r.recipeVersion==1 && r.schemaVersion==1 && r.applicationType==static_cast<ApplicationType>(type),"recipe roundtrip");
        }
        service.selectRecipe(ids[1]);
        service.editRecipe(ids[1],"Renamed","Updated","Camera_2","Cal_2");
        check(service.activeRecipe()->id==ids[1] && service.activeRecipe()->recipeVersion==1 && service.dirty(),"edit stays dirty before commit");
        {
            QLockFile lock(path+".lock"); check(lock.tryLock(0),"test lock");
            rejected([&]{service.save();},"concurrent lock must prevent save");
            check(service.dirty() && service.activeRecipe()->recipeVersion==1,"failed save retains edits and version");
        }
        service.save(); check(service.activeRecipe()->recipeVersion==2 && !service.dirty(),"revision increments on successful save");
        service.save(); check(service.activeRecipe()->recipeVersion==2,"no-op save preserves revision");
        ProjectService reopened; reopened.openProject(path); reopened.selectRecipe(ids[1]);
        check(reopened.activeRecipe()->name=="Renamed" && reopened.activeRecipe()->cameraReference=="Camera_2" &&
              reopened.activeRecipe()->calibrationReference=="Cal_2" && reopened.activeRecipe()->recipeVersion==2,"edited recipe persists");
        service.editRecipe(ids[1],"Temporary","","ImageFile",""); service.discardChanges();
        check(!service.dirty() && service.activeRecipe()->name=="Renamed","discard restores last saved snapshot");
        auto object=QJsonDocument::fromJson(ProjectRepository::encode(service.project())).object();
        const QString bad=dir.filePath("bad.vfproject");
        write(bad,"{broken"); rejected([&]{service.openProject(bad);},"invalid JSON rejected");
        check(service.project().id==projectId && service.activeRecipe()->id==ids[1],"failed open preserves active state");
        auto changed=object; changed["schemaVersion"]=2;
        write(bad,QJsonDocument(changed).toJson()); rejected([&]{service.openProject(bad);},"future schema rejected");
        changed=object; changed["extra"]=true;
        write(bad,QJsonDocument(changed).toJson()); rejected([&]{service.openProject(bad);},"unknown fields must not be silently dropped");
        changed=object; auto recipes=changed["recipes"].toArray(); auto recipe=recipes[0].toObject(); recipe["applicationType"]="unknown"; recipes[0]=recipe; changed["recipes"]=recipes;
        write(bad,QJsonDocument(changed).toJson()); rejected([&]{service.openProject(bad);},"unknown type rejected");
        changed=object; recipes=changed["recipes"].toArray(); recipes.append(recipes[0]); changed["recipes"]=recipes;
        write(bad,QJsonDocument(changed).toJson()); rejected([&]{service.openProject(bad);},"duplicate IDs rejected");
        // Nested future tool configuration roundtrips without being interpreted by Step 2.
        auto extended=service.project(); extended.recipes[0].definition=QJsonObject{{"toolConfig",QJsonObject{{"threshold",42}}}};
        ProjectRepository repository; repository.save(bad,extended);
        check(repository.load(bad).recipes[0].definition==extended.recipes[0].definition,"definition roundtrip");
        rejected([&]{repository.save(dir.path(),extended);},"invalid save destination rejected");
        check(repository.load(path).recipes[1].recipeVersion==2,"other failed writes leave original intact");
        // A staged/abandoned write must never replace the last committed project.
        const auto committed=ProjectRepository::encode(repository.load(path));
        {
            QSaveFile staged(path); staged.setDirectWriteFallback(false);
            check(staged.open(QIODevice::WriteOnly),"staged save opens");
            check(staged.write("{partial")==8,"partial staged bytes");
            check(ProjectRepository::encode(repository.load(path))==committed,"uncommitted bytes invisible to readers");
            staged.cancelWriting();
            check(!staged.commit(),"cancelled commit rejected");
        }
        check(ProjectRepository::encode(repository.load(path))==committed,"abandoned write preserves committed bytes");
        auto oversized=service.project();
        oversized.recipes[0].definition=QJsonObject{{"large",QString(16*1024*1024,'x')}};
        rejected([&]{repository.save(path,oversized);},"oversized save rejected before replacing target");
        check(ProjectRepository::encode(repository.load(path))==committed,"size rejection preserves previous project");
        write(bad,QByteArray(16*1024*1024+1,' '));
        rejected([&]{service.openProject(bad);},"oversized input rejected");
        check(service.project().id==projectId && !service.dirty(),"oversized open preserves session");
        service.editRecipe(ids[1],"Pending","","ImageFile","");
        write(path,"external change"); rejected([&]{service.save();},"external change detected");
        check(service.dirty() && service.activeRecipe()->name=="Pending" && service.activeRecipe()->recipeVersion==2,"conflict retains pending edits");
        ProjectService fresh; fresh.createProject("New","",dir.filePath("new.vfproject"),"ImageFile"); fresh.discardChanges();
        check(!fresh.hasProject() && !fresh.dirty(),"discard new unsaved project");
        QTextStream(stdout)<<"PASS: three types, IDs, revisions, discard, validation, failed save/open and external conflict\n";
        return 0;
    } catch (const std::exception& e) { QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
