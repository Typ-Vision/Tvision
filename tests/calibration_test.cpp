#include "application/CalibrationService.hpp"
#include "application/ProductionRecipeService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextStream>
#include <QUuid>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F> void reject(F action,const char* text){try{action();}catch(const std::exception&){return;}throw std::runtime_error(text);}
QByteArray read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"read fixture");return f.readAll();}
void write(const QString& path,const QByteArray& b){QFile f(path);check(f.open(QIODevice::WriteOnly),"write fixture");check(f.write(b)==b.size(),"fixture bytes");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    QTemporaryDir dir(QDir::currentPath()+"/calibration-fixture-XXXXXX");check(dir.isValid(),"build fixture");
    ProjectService project;project.createProject("Calibration test","",dir.path()+"/test.vfproject","ImageFile");
    const auto recipeId=project.createRecipe("Part","",ApplicationType::Measurement,"ImageFile");
    const auto source=project.addTool("image_file");auto sourceTool=project.tools()[0];sourceTool.parameters["path"]="fixture.png";project.updateTool(sourceTool);
    CalibrationDependency setup{"scale",1,"ignored","source","lens",{1000,800},"plane",{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYUp}};
    setup.cameraId=source;
    CalibrationInstallation installation{setup.cameraId,setup.lensId,setup.workingPlaneId,setup.resolution,setup.source,setup.target};const auto originalInstallation=installation;
    auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,setup.source},{b,setup.source},mm};};
    const auto teaching=sample("teach",{0,0},{400,0},10);
    const QVector<CalibrationSample> evidence{sample("x",{10,30},{810,30},20),sample("y",{40,10},{40,610},15)};
    MeasurementService math;const auto candidate=math.teach(setup,teaching);const auto c1=math.validate(candidate,evidence,0.001);
    const auto path=dir.path()+"/calibrations.json";CalibrationService catalog(path,[&](const QString&){return installation;});
    reject([&]{catalog.current("scale");},"missing current rejected");
    reject([&]{catalog.save(candidate);},"candidate rejected");catalog.save(c1);
    check(catalog.load("scale",1).dependency()==c1.dependency(),"version/evidence roundtrip");
    CalibrationService reopened(path,[&](const QString&){return installation;});check(reopened.current("scale")==c1.dependency(),"restart current independent resolution");
    reject([&]{CalibrationService(path).current("scale");},"no provider denied");
    const auto originalBytes=read(path);reject([&]{catalog.save(c1);},"overwrite rejected");check(read(path)==originalBytes,"old commit preserved");
    for(int i=0;i<7;++i){installation=originalInstallation;
        if(i==0)installation.cameraId="other";if(i==1)installation.lensId="other";if(i==2)installation.workingPlaneId="other";
        if(i==3)installation.resolution={800,600};if(i==4)installation.source.id="other";if(i==5)installation.target.id="other";if(i==6)installation.target.axes=AxisConvention::XRightYDown;
        reject([&]{catalog.current("scale");},"installation changes deny current");
    }installation=originalInstallation;
    project.save();check(project.activeRecipe()->schemaVersion==1,"legacy unbound remains schema 1");
    reject([&]{catalog.resolve(*project.activeRecipe());},"legacy name is not exact binding");
    catalog.bind(project,"scale");check(project.dirty() && project.activeRecipe()->schemaVersion==2 && project.activeRecipe()->status==RecipeStatus::Draft,"bind upgrades and drafts");
    project.save();catalog.bind(project,"scale");check(!project.dirty(),"same binding no-op");
    ProjectService reload;reload.openProject(project.path());reload.selectRecipe(recipeId);
    check(reload.activeRecipe()->calibrationBinding==c1.dependency() && catalog.resolve(*reload.activeRecipe()).validated(),"project restart binding");
    reload.editRecipe(recipeId,"Renamed","metadata","ImageFile","scale");check(reload.activeRecipe()->calibrationBinding.has_value(),"metadata preserves binding");reload.discardChanges();reload.selectRecipe(recipeId);
    ProductionRecipeService production([](const QString&,ProductionAction){return true;},[&](const Recipe& r){ProductionDependencies d;d.calibration=catalog.resolve(r).dependency();return d;});
    const auto validation=production.validate("engineer",*reload.activeRecipe(),dir.path());
    auto setup2=setup;setup2.version=2;const auto c2=math.validate(math.teach(setup2,teaching),evidence,0.002);
    {QLockFile lock(path+".lock");check(lock.tryLock(),"fixture lock");reject([&]{catalog.save(c2);},"lock preserves commit");}check(read(path)==originalBytes,"locked commit unchanged");
    auto skipped=setup;skipped.version=3;reject([&]{catalog.save(math.validate(math.teach(skipped,teaching),evidence,0.001));},"version gap rejected");
    catalog.save(c2);check(catalog.load("scale",1).dependency()==c1.dependency(),"old version retained");
    reject([&]{catalog.resolve(*reload.activeRecipe());},"old recipe invalidated by new current version");
    reject([&]{production.activate("engineer",validation.id,*reload.activeRecipe(),dir.path());},"old production validation invalidated");
    catalog.bind(reload,"scale");check(reload.dirty() && reload.activeRecipe()->calibrationBinding==c2.dependency(),"explicit rebind drafts");
    reload.save();
    const auto boundProjectBytes=read(reload.path());
    for(int i=0;i<4;++i){auto root=QJsonDocument::fromJson(boundProjectBytes).object();auto recipes=root["recipes"].toArray();auto r=recipes[0].toObject();
        if(i==0)r["schemaVersion"]=3;if(i==1)r.remove("calibrationBinding");if(i==2)r["calibrationBinding"]=true;
        if(i==3)r["calibrationReference"]="foreign";
        recipes[0]=r;root["recipes"]=recipes;write(dir.path()+"/invalid.vfproject",QJsonDocument(root).toJson());
        reject([&]{ProjectService invalid;invalid.openProject(dir.path()+"/invalid.vfproject");},"malformed/future binding schema rejected");
    }
    auto foreign=*reload.activeRecipe();auto foreignTools=editorTools(foreign.definition);foreignTools[0].id=QUuid::createUuid().toString();foreign.definition=withEditorTools(foreign.definition,foreignTools);
    reject([&]{catalog.resolve(foreign);},"foreign acquisition source rejects binding");
    auto renamedSource=reload.tools()[0];renamedSource.name="Metadata only";reload.updateTool(renamedSource);
    check(reload.activeRecipe()->calibrationBinding.has_value(),"source metadata preserves calibration");reload.discardChanges();reload.selectRecipe(recipeId);
    auto changedSource=reload.tools()[0];changedSource.parameters["path"]="another.png";reload.updateTool(changedSource);
    check(!reload.activeRecipe()->calibrationBinding && reload.dirty(),"source parameters clear binding and draft");reload.discardChanges();reload.selectRecipe(recipeId);
    const auto good=read(path);
    for(int i=0;i<8;++i){auto root=QJsonDocument::fromJson(good).object();auto records=root["records"].toArray();auto record=records[0].toObject();
        if(i==0)record["mmPerPx"]=0.026;if(i==1)record["validated"]=false;if(i==2)record["schemaVersion"]=2;if(i==3)record["extra"]=true;
        if(i==4)record["rmsMm"]=1;if(i==5){auto d=record["dependency"].toObject();d["contentId"]="forged";record["dependency"]=d;}
        if(i==6){auto h=record["holdout"].toArray();auto s=h[0].toObject();s["knownMm"]=21;h[0]=s;record["holdout"]=h;}
        if(i==7)record["maxErrorMm"]=QJsonValue(QJsonValue::Null);
        records[0]=record;root["records"]=records;write(path,QJsonDocument(root).toJson());reject([&]{catalog.load("scale",1);},"tampered evidence rejected");
    }write(path,good);
    auto excessive=QJsonDocument::fromJson(good).object();QJsonArray many;for(int i=0;i<129;++i)many.append(math.encode(c1));excessive["records"]=many;
    write(path,QJsonDocument(excessive).toJson());reject([&]{catalog.current("scale");},"record limit");write(path,good);
    write(path,QByteArray(4*1024*1024+1,' '));reject([&]{catalog.current("scale");},"byte limit");write(path,good);
    reload.editRecipe(recipeId,"Part","","DifferentSource","scale");check(!reload.activeRecipe()->calibrationBinding,"camera metadata change clears binding");
    CalibrationService bad(dir.path()+"/missing/catalog.json");reject([&]{bad.save(c1);},"failed save no file");
    check(read(path)==good,"other failed save preserves catalog");
    QTextStream(stdout)<<"PASS: evidence replay, atomic version history, independent setup, recipe binding and production invalidation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
