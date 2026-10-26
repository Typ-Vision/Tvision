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
#include <QUuid>
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
QByteArray bytes(const QString& path) {QFile f(path);check(f.open(QIODevice::ReadOnly),"read fixture");return f.readAll();}
void variablesContract(const QString& path) {
    ProjectService service;service.createProject("Variables","",path,"");
    const auto recipe=service.createRecipe("Part","",ApplicationType::Measurement);service.save();
    const auto legacy=bytes(path);service.openProject(path);service.selectRecipe(recipe);service.save();
    check(bytes(path)==legacy && !service.activeRecipe()->definition.contains("variableCatalog"),"legacy no-op preserves exact bytes without catalog");
    VariableDefinition d;d.name="Length";d.type=VariableType::Float64;d.unit=ScalarUnit::Millimetre;d.comment="Real measurement";
    const auto id=service.addVariable(d);check(service.dirty() && service.recipeDirty(recipe) && service.activeRecipe()->recipeVersion==1,"variable edit stages revision");
    check(service.variables()[0].id==id && service.activeRecipe()->status==RecipeStatus::Draft,"generated stable UUID and DRAFT");
    const auto candidate=ProjectRepository::encode(service.project());
    d.name="length";rejected([&]{service.addVariable(d);},"case-fold duplicate rejected");
    d.name="Other";d.type=VariableType(99);rejected([&]{service.addVariable(d);},"illegal type rejected");
    d.type=VariableType::Boolean;rejected([&]{service.addVariable(d);},"Boolean with mm rejected");
    d.unit=ScalarUnit::Unitless;d.name=" ";rejected([&]{service.addVariable(d);},"blank variable rejected");
    rejected([&]{service.removeVariable("unknown");},"missing removal rejected");
    check(ProjectRepository::encode(service.project())==candidate,"rejected commands preserve candidate atomically");
    service.save();service.openProject(path);service.selectRecipe(recipe);
    check(service.variables()[0].id==id && service.variables()[0].comment=="Real measurement" && service.activeRecipe()->recipeVersion==2,"save/reopen definition and UUID");
    const auto saved=bytes(path);service.renameVariable(id,"Length");service.updateVariable(service.variables()[0]);service.save();
    check(!service.dirty() && bytes(path)==saved,"no-op variable edits preserve exact bytes");
    service.renameVariable(id,"Renamed");service.discardChanges();check(service.variables()[0].name=="Length" && !service.dirty(),"discard restores variable catalog");
    // A future consumer binds by UUID; rename retains binding, contract edits fail closed.
    auto referenced=service.project();referenced.recipes[0].status=RecipeStatus::Valid;
    referenced.recipes[0].definition["consumerFixture"]=QJsonObject{{"variableId",id}};
    ProjectRepository{}.save(path,referenced);service.openProject(path);service.selectRecipe(recipe);
    service.renameVariable(id,"Renamed");check(service.activeRecipe()->status==RecipeStatus::Draft && service.activeRecipe()->definition["consumerFixture"].toObject()["variableId"]==id,"rename preserves UUID binding and invalidates validation");
    const auto before=ProjectRepository::encode(service.project());
    rejected([&]{service.removeVariable(id);},"referenced deletion rejected");
    auto changed=service.variables()[0];changed.type=VariableType::Int64;
    rejected([&]{service.updateVariable(changed);},"referenced type edit rejected");changed=service.variables()[0];changed.unit=ScalarUnit::Metre;
    rejected([&]{service.updateVariable(changed);},"referenced unit edit rejected");
    check(ProjectRepository::encode(service.project())==before,"reference guard failure is atomic");service.save();
    QVector<VariableDefinition> list;
    for(int i=0;i<maxRecipeVariables;++i){VariableDefinition v;v.id=QUuid::createUuid().toString(QUuid::WithoutBraces);v.name="V"+QString::number(i);list.append(v);}
    auto catalog=withRecipeVariables({},list);check(recipeVariables(catalog)==list,"256 variable boundary");
    auto full=service.project();full.recipes[0].definition=catalog;
    const auto fullPath=path+".full";ProjectRepository{}.save(fullPath,full);
    ProjectService capped;capped.openProject(fullPath);capped.selectRecipe(recipe);
    VariableDefinition overflow;overflow.name="Overflow";rejected([&]{capped.addVariable(overflow);},"service rejects 257th variable");
    check(!capped.dirty() && capped.variables().size()==256,"limit failure leaves service clean");
    auto extra=list[0];extra.id=QUuid::createUuid().toString(QUuid::WithoutBraces);extra.name="Extra";list.append(extra);
    rejected([&]{withRecipeVariables({},list);},"257 variables rejected");list.removeLast();list[1].id=list[0].id;
    rejected([&]{withRecipeVariables({},list);},"duplicate variable UUID rejected");
    for(int type=0;type<=int(VariableType::SpatialTransform);++type) {
        VariableDefinition v;v.id=id;v.name="Typed";v.type=VariableType(type);
        if(v.type==VariableType::Enum){v.enumDefinitionId="decision/1";v.enumCodes={-1,0,2};}
        if(v.type==VariableType::SpatialTransform){v.unit=ScalarUnit::Millimetre;FrameRef f{"camera","","",id,1};v.frameConstraint=VariableFrameConstraint{f,f};}
        check(VariableDefinition::fromJson(v.toJson())==v,"all supported type metadata roundtrip");
    }
    auto invalid=service.variables()[0].toJson();invalid["value"]=123;
    rejected([&]{VariableDefinition::fromJson(invalid);},"runtime value cannot enter definition codec");invalid.remove("value");invalid["frameConstraint"]=false;
    rejected([&]{VariableDefinition::fromJson(invalid);},"malformed frame constraint rejected");
    invalid=service.variables()[0].toJson();invalid["id"]="not-a-uuid";
    rejected([&]{VariableDefinition::fromJson(invalid);},"invalid variable UUID rejected");
    invalid=service.variables()[0].toJson();invalid["type"]="Pose2D";
    rejected([&]{VariableDefinition::fromJson(invalid);},"legacy Pose2D is not a spatial variable type");
    invalid=service.variables()[0].toJson();invalid["enumCodes"]=QJsonArray{1};
    rejected([&]{VariableDefinition::fromJson(invalid);},"foreign enum metadata rejected");
    invalid=service.variables()[0].toJson();invalid.remove("unit");
    rejected([&]{VariableDefinition::fromJson(invalid);},"missing definition field rejected");
    auto bad=service.project();bad.recipes[0].definition["variableCatalog"]=QJsonObject{{"schemaVersion",2},{"variables",QJsonArray{}}};
    rejected([&]{ProjectRepository{}.save(path,bad);},"future catalog rejected on save");
    auto disk=QJsonDocument::fromJson(bytes(path)).object();auto recipes=disk["recipes"].toArray();auto r=recipes[0].toObject();r["definition"]=bad.recipes[0].definition;recipes[0]=r;disk["recipes"]=recipes;
    const auto badPath=path+".bad";write(badPath,QJsonDocument(disk).toJson());
    const auto current=ProjectRepository::encode(service.project());rejected([&]{service.openProject(badPath);},"future catalog rejected on load");
    check(ProjectRepository::encode(service.project())==current,"bad catalog open preserves active project");
    // Removing last variable keeps opt-in catalog; never silently migrates back.
    auto clean=service.project();clean.recipes[0].definition.remove("consumerFixture");ProjectRepository{}.save(path,clean);service.openProject(path);service.selectRecipe(recipe);
    service.removeVariable(id);service.save();service.openProject(path);service.selectRecipe(recipe);
    check(service.variables().isEmpty() && service.activeRecipe()->definition.contains("variableCatalog"),"empty opted-in catalog persists");
    d.name="Editable";const auto editable=service.addVariable(d);auto edit=service.variables()[0];edit.type=VariableType::Int64;
    service.updateVariable(edit);check(service.variables()[0].type==VariableType::Int64 && service.variables()[0].id==editable,"unreferenced type edit preserves UUID");
    service.discardChanges();check(service.variables().isEmpty(),"discard add and type edit");
    QTextStream(stdout)<<"PASS: COMM-04 variable commands, identity, references, limits, strict codec and legacy bytes\n";
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("project-test-XXXXXX"));
        check(dir.isValid(),"temporary directory");
        variablesContract(dir.filePath("variables.vfproject"));
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
        rejected([&]{service.editRecipeMetadata(ids[0],"part_1","");},"metadata duplicate name must fail");
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
        // CCUX-11: references are optional legacy metadata, never inherited by new recipes.
        ProjectService empty; empty.createProject("No references","",dir.filePath("empty.vfproject"),"");
        for(int type=0;type<3;++type) {
            const auto id=empty.createRecipe("Empty_"+QString::number(type),"",static_cast<ApplicationType>(type));
            check(empty.activeRecipe()->cameraReference.isEmpty() && empty.activeRecipe()->calibrationReference.isEmpty() &&
                  empty.activeRecipe()->definition.isEmpty() && empty.activeRecipe()->status==RecipeStatus::Draft,"three types start with zero sources and no global reference");
            empty.save();empty.editRecipeMetadata(id,empty.activeRecipe()->name,"");check(!empty.dirty(),"empty recipe metadata no-op");
        }
        empty.updateProject("No references","","Old camera","","","Old calibration");
        empty.createRecipe("No inheritance","",ApplicationType::Measurement);
        check(empty.activeRecipe()->cameraReference.isEmpty() && empty.activeRecipe()->calibrationReference.isEmpty(),"project references are not inherited");
        rejected([&]{empty.editRecipeMetadata(empty.activeRecipe()->id,"  ","");},"blank metadata name rejected");
        rejected([&]{empty.editRecipeMetadata(empty.activeRecipe()->id,"empty_0","");},"case-fold metadata duplicate rejected");
        rejected([&]{empty.editRecipeMetadata("missing","Missing","");},"unknown metadata recipe rejected");
        empty.save();
        auto portable=ProjectRepository{}.load(empty.path());
        ProjectRepository{}.save(dir.filePath("portable-copy.vfproject"),portable);
        check(ProjectRepository::encode(ProjectRepository{}.load(dir.filePath("portable-copy.vfproject")))==ProjectRepository::encode(portable),"value copy and project export/import preserve empty references and three types");
        ProjectService legacy; legacy.createProject("Legacy","",dir.filePath("metadata.vfproject"),"Old project");
        const auto legacyId=legacy.createRecipe("Legacy recipe","old",ApplicationType::Measurement,"Old camera","cal");
        legacy.bindCalibration({"cal",1,"content","source","lens",{640,480},"plane",{"source/image"},{"plane",LengthUnit::Millimetre,AxisConvention::XRightYDown}});
        legacy.save();
        const auto before=*legacy.activeRecipe();const auto encoded=ProjectRepository::encode(legacy.project());
        legacy.editRecipeMetadata(legacyId,"Legacy recipe","old");legacy.save();
        check(!legacy.dirty() && ProjectRepository::encode(legacy.project())==encoded,"legacy metadata no-op preserves bytes and revisions");
        legacy.editRecipeMetadata(legacyId,"Renamed legacy","old");
        check(legacy.activeRecipe()->cameraReference==before.cameraReference && legacy.activeRecipe()->calibrationReference==before.calibrationReference &&
              legacy.activeRecipe()->calibrationBinding==before.calibrationBinding && legacy.activeRecipe()->definition==before.definition,"name-only edit preserves legacy fields and binding");
        legacy.save();legacy.openProject(legacy.path());legacy.selectRecipe(legacyId);
        check(legacy.activeRecipe()->calibrationBinding==before.calibrationBinding && legacy.activeRecipe()->recipeVersion==2,"legacy metadata edit roundtrip");
        QTextStream(stdout)<<"PASS: three types, IDs, revisions, discard, validation, failed save/open and external conflict\n";
        return 0;
    } catch (const std::exception& e) { QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
