#include "application/ActiveCalibrationService.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLockFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
template<class F> void reject(F f,const char* s){++checks;try{f();}catch(const std::exception&){return;}throw std::runtime_error(s);}
QByteArray read(const QString& p){QFile f(p);check(f.open(QIODevice::ReadOnly),"read fixture");return f.readAll();}
void write(const QString& p,const QByteArray& b){QFile f(p);check(f.open(QIODevice::WriteOnly) && f.write(b)==b.size(),"write fixture");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir::currentPath()+"/active-calibration-XXXXXX");check(dir.isValid(),"build fixture");
    MeasurementSetup setup{"bench-A","image_file",QString(64,'a'),"lens","plane",{1000,800},{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYUp}};
    CalibrationDependency dependency{"scale",1,"pending","recipe-source-A",setup.lensId,setup.resolution,setup.planeId,setup.source,setup.target};
    CalibrationSample teach{"teach",{{0,0},setup.source},{{400,0},setup.source},10};
    CalibrationSample holdout{"check",{{20,40},setup.source},{{820,40},setup.source},20};
    MeasurementService math;const auto model=math.teach(dependency,teach);const auto legacy=math.encode(model);
    CalibrationImageEvidence image{"acquired",dependency.cameraId,"frame-A",QString(64,'b')};
    const auto record=CalibrationModelRecord::create(model,setup,image);
    check(!record.model().validated() && record.residualsMillimetres().isEmpty() && !record.controlAccepted() && record.activatable(),"single pair ready without fabricated validation");
    check(record.model().millimetresPerPixel()==0.025,"400 px / 10 mm actual scale");
    check(math.encode(record.model())==legacy,"legacy model bytes/identity unchanged");
    check(CalibrationModelRecord::decode(record.encode()).encode()==record.encode(),"strict replay roundtrip");
    const auto controlled=CalibrationModelRecord::create(model,setup,image,{{holdout},std::nullopt});
    check(controlled.residualsMillimetres()==QVector<double>{0} && !controlled.controlAccepted() && !controlled.model().validated(),"one real optional control no invented acceptance");
    const auto accepted=CalibrationModelRecord::create(model,setup,image,{{holdout},0.001});check(accepted.controlAccepted()==true && accepted.activatable(),"explicit control bound accepted");
    auto wrong=holdout;wrong.knownMillimetres=22;
    const auto failed=CalibrationModelRecord::create(model,setup,image,{{wrong},0.01});check(failed.controlAccepted()==false && !failed.activatable() && failed.residualsMillimetres()[0]==2,"failed control saved as actual evidence blocks activation");
    reject([&]{CalibrationModelRecord::create(model,setup,image,{{teach},0.1});},"teaching control rejected");auto reverse=teach;std::swap(reverse.first,reverse.second);reverse.id="reversed";
    reject([&]{CalibrationModelRecord::create(model,setup,image,{{reverse},0.1});},"reversed teaching control rejected");
    reject([&]{CalibrationModelRecord::create(model,setup,image,{{holdout,holdout},0.1});},"duplicate control rejected");
    reject([&]{CalibrationModelRecord::create(model,setup,image,{{},0.1});},"bound without evidence rejected");
    for(int n=0;n<10;++n){auto bad=record.encode();
        if(n==0)bad["modelReady"]=false;if(n==1)bad["modelReady"]="true";if(n==2)bad["schemaVersion"]=4;if(n==3)bad["unknown"]=true;if(n==4)bad.remove("image");if(n==5)bad["contentId"]=QString(64,'c');
        if(n==6){auto m=bad["model"].toObject();m["validated"]=true;bad["model"]=m;}
        if(n==7){auto c=bad["control"].toObject();c["accepted"]=true;bad["control"]=c;}
        if(n==8){auto c=bad["control"].toObject();c["limitMm"]="0.01";bad["control"]=c;}
        if(n==9){auto s=bad["setup"].toObject();s["width"]=1000.5;bad["setup"]=s;}
        reject([&]{CalibrationModelRecord::decode(bad);},"forged readiness/control/schema rejected");
    }
    bool idle=true;const auto path=dir.path()+"/active.json";ActiveCalibrationService service(path,"project-A",[&]{return idle;});
    reject([&]{service.resolve(setup);},"no fallback without explicit activation");service.save(record);check(!service.active() && !service.remembered(),"candidate save never activates");
    service.activate("scale",1,setup);const auto first=service.resolve(setup);check(first.selection.version==1 && first.selection.revision==1,"explicit activation snapshot");
    dependency.version=2;auto second=CalibrationModelRecord::create(math.teach(dependency,teach),setup,image);service.save(second);
    check(service.resolve(setup).selection==first.selection,"newer candidate leaves exact old active model");
    auto otherSource=dependency;otherSource.calibrationId="other-source";otherSource.version=1;otherSource.cameraId="recipe-source-B";auto imageB=image;imageB.sourceToolId=otherSource.cameraId;
    auto other=CalibrationModelRecord::create(math.teach(otherSource,teach),setup,imageB);service.save(other);service.activate("other-source",1,setup);
    check(service.resolve(setup).record.model().dependency().cameraId=="recipe-source-B","shared setup independent of acquisition tool ID");
    for(int n=0;n<9;++n){auto changed=setup;if(n==0)changed.id="bench-B";if(n==1)changed.adapter="webcam";if(n==2)changed.configId=QString(64,'d');if(n==3)changed.lensId="other";if(n==4)changed.planeId="other";if(n==5)changed.resolution={800,600};if(n==6)changed.source.id="other";if(n==7)changed.target.id="other";if(n==8)changed.target.axes=AxisConvention::XRightYDown;
        reject([&]{service.resolve(changed);},"installation change invalidates active");reject([&]{service.activate("scale",1,changed);},"incompatible activation rejected");}
    const auto before=service.active();const auto saved=read(path);idle=false;
    reject([&]{service.activate("scale",1,setup);},"busy activation rejected");reject([&]{service.deactivate();},"busy deactivation rejected");idle=true;check(service.active()==before && read(path)==saved,"busy rejection preserves active and disk");
    {QLockFile lock(path+".lock");check(lock.tryLock(0),"test lock");reject([&]{service.activate("scale",1,setup);},"locked activation rejected");reject([&]{service.deactivate();},"locked deactivation rejected");}
    check(service.active()==before && read(path)==saved,"failed commit preserves active selection");
    ActiveCalibrationService reopened(path,"project-A",[]{return true;});check(!reopened.active() && reopened.remembered()==before,"restart remembers without active permission");reject([&]{reopened.resolve(setup);},"restart requires explicit activation");
    reject([&]{ActiveCalibrationService wrongProject(path,"project-B",[]{return true;});},"project isolation");
    reject([&]{service.save(second);},"duplicate record rejected");check(read(path)==saved,"failed candidate save preserves catalog");
    auto trialImage=image;trialImage.origin="trial_file";dependency.calibrationId="trial";dependency.version=1;
    auto trial=CalibrationModelRecord::create(math.teach(dependency,teach),setup,trialImage);service.save(trial);reject([&]{service.activate("trial",1,setup);},"trial cannot activate");
    dependency.calibrationId="bad-control";service.save(CalibrationModelRecord::create(math.teach(dependency,teach),setup,image,{{wrong},0.01}));reject([&]{service.activate("bad-control",1,setup);},"failed optional control cannot activate");
    dependency.calibrationId="declared-file";trialImage.origin="setup_file";service.save(CalibrationModelRecord::create(math.teach(dependency,teach),setup,trialImage));service.activate("declared-file",1,setup);check(service.resolve(setup).record.image().origin=="setup_file","file declaration retains file provenance");
    auto legacyValidated=math.validate(model,{holdout,{"y",{{40,10},setup.source},{{40,610},setup.source},15}},0.001);const auto legacyBytes=math.encode(legacyValidated);
    const auto migrated=CalibrationModelRecord::create(legacyValidated,setup,image);check(math.encode(migrated.model())==legacyBytes && migrated.model().validated() && migrated.residualsMillimetres().isEmpty(),"legacy accepted evidence preserved separately from new control");
    auto manualDependency=model.dependency();manualDependency.calibrationId="manual";
    const auto manual=math.manualScale(manualDependency,40,{"acquired",image.sourceToolId,image.frameId,setup.resolution});
    const auto manualRecord=CalibrationModelRecord::create(manual,setup,image);
    check(CalibrationModelRecord::decode(manualRecord.encode()).model().millimetresPerPixel()==0.025 && !manualRecord.model().validated(),"manual legacy candidate replay without fabricated validation");
    auto hSetup=setup;hSetup.target.axes=AxisConvention::XRightYDown;auto hd=model.dependency();hd.calibrationId="homography";hd.target=hSetup.target;
    ChessboardObservation observation{{"board",9,6,10},{image.sourceToolId,image.frameId,"fit-cycle",setup.source.id,QDateTime::currentDateTimeUtc()},setup.resolution,{},{}};
    for(int y=0;y<6;++y)for(int x=0;x<9;++x){observation.worldMillimetres.append({x*10.0,y*10.0});observation.pixels.append({100+x*50.0,100+y*50.0});}
    const auto homography=math.teachHomography(hd,PlaneMappingService::fit(observation));
    CalibrationSample hControl{"independent",{{125,125},setup.source},{{225,125},setup.source},20};
    const auto hRecord=CalibrationModelRecord::create(homography,hSetup,image,{{hControl},0.001});
    check(hRecord.controlAccepted()==true && CalibrationModelRecord::decode(hRecord.encode()).encode()==hRecord.encode(),"homography model/control exact replay");
    auto outside=hControl;outside.second.position={900,700};reject([&]{CalibrationModelRecord::create(homography,hSetup,image,{{outside},std::nullopt});},"homography hull protection");
    auto fitControl=hControl;fitControl.first.position=observation.pixels[0];reject([&]{CalibrationModelRecord::create(homography,hSetup,image,{{fitControl},std::nullopt});},"fit corner is not independent control");
    auto wrongFrame=image;wrongFrame.frameId="other-frame";reject([&]{CalibrationModelRecord::create(homography,hSetup,wrongFrame);},"mapping frame evidence mismatch");
    const auto good=read(path);auto root=QJsonDocument::fromJson(good).object();root["revision"]=999;write(path,QJsonDocument(root).toJson());reject([&]{service.resolve(setup);},"persisted selection tamper rejected");write(path,good);
    service.deactivate();check(!service.active() && !service.remembered(),"explicit deactivation clears active");reject([&]{service.resolve(setup);},"no legacy fallback after deactivation");
    const auto modelBytes=math.encode(model);check(math.encode(math.decode(modelBytes))==modelBytes,"legacy no-op codec preserved");
    QTextStream(stdout)<<"PASS: "<<checks<<" active calibration contract checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
