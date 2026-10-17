#include "application/ScopedActiveCalibration.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QTextStream>
#include <QFile>
#include <future>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
template<class F>void reject(F f){++checks;try{f();}catch(const std::exception&){return;}throw std::runtime_error("Expected rejection");}
class Adapter:public AcquisitionAdapter {
public:AcquisitionCapabilities capabilities() const override{return {"webcam",true};}
ImageFrame acquire(const AcquisitionRequest&) const override {QImage im(640,200,QImage::Format_Grayscale8);im.fill(20);for(int y=0;y<200;++y)for(int x=120;x<520;++x)im.scanLine(y)[x]=220;return ImageFrame::fromBuffer(im.constBits(),im.sizeInBytes(),640,200,im.bytesPerLine(),PixelFormat::Gray8);}
};
void capture(IndependentCalibrationSession& session,const MeasurementSetupContract& c,int i){
IndependentCaptureRequest req;req.origin=SourceOrigin::CameraAcquired;req.camera=c.cameras[i];req.setup=c.setups[i];
IndependentCaptureBackend b{[&]{return QVector<DeviceBinding>{c.cameras[i].device};},[](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<Adapter>();}};
check(session.accept(IndependentCalibrationSession::run(session.begin(req),b)),"accepted fixture probe");
}
CalibrationModelRecord calculate(IndependentCalibrationSession& session,double mm){auto& m=session.math();EdgePairOptions o;o.profile={.5,1,1};check(m.accept(CalibrationWizardSession::run(m.scanPair("pair",{{"image"},ScanSegment{{20,100},{620,100},4}},o))),"actual edge math");m.setKnownMillimetres(mm);return m.calculate();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
QTemporaryDir dir(QDir::currentPath()+"/scoped-XXXXXX");check(dir.isValid(),"build fixture dir");
for(int n:{1,2,8,10}){
MeasurementEnvironmentRepository repo(dir.filePath(QString::number(n)+".vfproject"),"p");MeasurementEnvironmentRegistry reg(repo);MeasurementSetupContract c;
for(int i=0;i<n;++i){const auto id=QString::number(i);c.cameras.append({{"c"+id},"Same camera name",{"webcam","provider-"+id,DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}});c.setups.append({{"s"+id},c.cameras.last().id,"Same setup name","lens","plane",1,1,{{640,200},{},{},true}});}reg.replace(c);
bool ready=true;ScopedActiveCalibrationService service(repo,[&]{return ready;});QVector<SetupReference> refs;QVector<QString> contents;
for(int i=0;i<n;++i){SetupReference ref{c.cameras[i].id,c.setups[i].id,1};refs.append(ref);service.setOperational(ref.camera,ref.setup);IndependentCalibrationSession s;capture(s,c,i);auto r=calculate(s,10+i);contents.append(r.contentId());service.save(ref,r);check(service.environment().selections.size()==i,"save is candidate only");service.activate(ref,r.contentId(),s);check(service.snapshots({ref})[0].calibration.record.contentId()==r.contentId(),"exact activation");reject([&]{service.activate(ref,r.contentId(),s);});}
auto all=service.snapshots(refs);check(all.size()==n,"all immutable snapshots");for(int i=0;i<n;++i)check(std::abs(all[i].calibration.record.model().millimetresPerPixel()-(10.+i)/400)<1e-8,"different real scales");
ScopedActiveCalibrationService restart(repo,[]{return true;});reject([&]{restart.snapshots({refs[0]});});check(restart.environment().selections.size()==n,"restart remembers only");
IndependentCalibrationSession probe;capture(probe,c,0);ready=false;reject([&]{service.activate(refs[0],contents[0],probe);});reject([&]{service.deactivate(refs[0].setup);});ready=true;
service.activate(refs[0],contents[0],probe,MeasurementEnvironmentRepository::Fault::None);
IndependentCalibrationSession invalid;capture(invalid,c,0);auto base=calculate(invalid,10);auto image=base.image();image.origin="trial_file";
auto trial=CalibrationModelRecord::create(base.model(),base.setup(),image);service.save(refs[0],trial);reject([&]{service.activate(refs[0],trial.contentId(),invalid);});
capture(invalid,c,0);base=calculate(invalid,10);
CalibrationControl control;control.limitMillimetres=0.01;control.samples.append({"independent",{{10,10},base.setup().source},{{20,10},base.setup().source},100});
auto failedControl=CalibrationModelRecord::create(base.model(),base.setup(),base.image(),control);check(!failedControl.activatable(),"control rejection distinct from mathematical readiness");
service.save(refs[0],failedControl);reject([&]{service.activate(refs[0],failedControl.contentId(),invalid);});
capture(probe,c,0);auto replacement=calculate(probe,30);service.save(refs[0],replacement);reject([&]{service.activate(refs[0],replacement.contentId(),probe,MeasurementEnvironmentRepository::Fault::BeforeCommit);});check(service.snapshots(refs).size()==n,"failed commit preserves active selection");
if(n>1){const auto prior=service.snapshots({refs[1]})[0];reject([&]{service.activate(refs[1],contents[0],probe);});service.deactivate(refs[0].setup);check(service.snapshots({refs[1]})[0].calibration.selection==prior.calibration.selection,"A change leaves B revision and record intact");}
reg.reload();auto alternate=reg.snapshot().current;auto extra=alternate.setups[0];extra.id={"alternate"};alternate.setups.append(extra);reg.replace(alternate);service.setOperational(refs[0].camera,extra.id);reject([&]{service.snapshots({refs[0]});});service.setOperational(refs[0].camera,refs[0].setup);reject([&]{service.snapshots({refs[0]});});
service.disconnect(refs.last().camera);reject([&]{service.snapshots({refs.last()});});check(all.size()==n && all.last().calibration.record.contentId()==contents.last(),"already copied snapshots immutable");
// External registry tamper must fail closed, preserving the last valid bytes.
QFile file(repo.path());check(file.open(QIODevice::ReadOnly),"read registry");auto bytes=file.readAll();file.close();check(file.open(QIODevice::WriteOnly),"tamper fixture");file.write("{}");file.close();reject([&]{service.snapshots({refs.last()});});check(file.open(QIODevice::WriteOnly),"restore fixture");file.write(bytes);file.close();
capture(probe,c,n-1);service.activate(refs.last(),contents.last(),probe);service.refreshAvailability({});reject([&]{service.snapshots({refs.last()});});
auto other=std::async(std::launch::async,[&]{try{service.environment();}catch(const std::exception&){return true;}return false;});check(other.get(),"owner thread");
// Same dimensions/name cannot adopt another physical installation.
auto wrong=all[0].calibration.record;reject([&]{service.save(refs.last(),wrong);});
}
QTextStream(stdout)<<checks<<" scoped calibration checks passed; camera adapter fixtures\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
